# OVL99 generator for SMALLC99's existing multi-segment overlay architecture.
# OVLMAP.TXT declares the existing DREL exports and physical pages.
# OVLTHUNKS.TXT declares legacy-compatible resident public dispatch names.
# All generated *normal* dispatches call the same resident OVCALL gate.
# The two original non-returning top-level dispatches remain tail calls.
param(
    [Parameter(Mandatory=$true)][string]$Map,
    [Parameter(Mandatory=$true)][string]$Calls,
    [Parameter(Mandatory=$true)][string]$Defs,
    [Parameter(Mandatory=$true)][string]$Addresses,
    [Parameter(Mandatory=$true)][string]$Table,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

function Fail([string]$why) { throw "*** OVL99 thunk generation: $why" }
foreach ($path in @($Map,$Calls,$Defs,$Addresses,$Table)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { Fail "missing $path" }
}

# Read the existing OVLDEFS IDs; do not renumber a working compiler.
$ids = @{}
foreach ($line in (Get-Content -LiteralPath $Defs)) {
    $m = [regex]::Match($line,'^[ \t]*OVL_([A-Z][A-Z0-9_]*)[ \t]+EQU[ \t]+([0-9]+)\b')
    if ($m.Success) { $ids[$m.Groups[1].Value] = [int]$m.Groups[2].Value }
}
if ($ids.Count -eq 0) { Fail "no overlay IDs in $Defs" }

# Track each DREL entry alias and its logical overlay, including
# the two exported names that both target E_EXPR.
$aliasOwner = @{}
foreach ($raw in (Get-Content -LiteralPath $Map)) {
    $line = ($raw -replace '#.*$', '').Trim()
    if ($line -eq '') { continue }
    $f = @($line -split '[ \t]+' | Where-Object { $_ -ne '' })
    if ($f.Count -ne 5) { Fail "invalid overlay-map record: $line" }
    $owner = $f[2]
    if (-not $ids.ContainsKey($owner) -or $ids[$owner] -ne [int]$f[3]) {
        Fail "map ID for $owner does not match OVLDEFS"
    }
    foreach ($part in @($f[4] -split ',')) {
        $m = [regex]::Match($part,'^([A-Za-z_][A-Za-z0-9_]*):([A-Za-z_][A-Za-z0-9_]*|-)$')
        if (-not $m.Success) { Fail "bad DREL export $part" }
        $alias = $m.Groups[2].Value
        if ($alias -eq '-') { continue }
        if ($aliasOwner.ContainsKey($alias)) { Fail "duplicate DREL alias $alias" }
        $aliasOwner[$alias] = $owner
    }
}

# Inspect DREL's ACTUAL final table, including the existing STMT page-14
# correction. Every address called must be backed by that owner's mapping.
$segments = @{}
foreach ($line in (Get-Content -LiteralPath $Table)) {
    $m = [regex]::Match($line,'^[ \t]*WORD[ \t]+([0-9, \t]+)[ \t]*;[^\r\n]*\bID[ \t]+([0-9]+)\b',
        [Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if (-not $m.Success) { continue }
    $id = [int]$m.Groups[2].Value
    if ($segments.ContainsKey($id)) { Fail "duplicate OVLTABLE row for ID $id" }
    $words = @($m.Groups[1].Value -split ',' | ForEach-Object { [int]$_.Trim() })
    if ($words.Count -ne 10) { Fail "OVLTABLE ID $id has $($words.Count) words; expected 10" }
    $mapped = @()
    for ($i=0; $i -lt 10; $i+=2) {
        if ($words[$i] -ne 0) { $mapped += $words[$i] }
    }
    $segments[$id] = $mapped
}

$addrText = Get-Content -LiteralPath $Addresses -Raw
$written = @{}
$entries = @()
foreach ($raw in (Get-Content -LiteralPath $Calls)) {
    $line = ($raw -replace '#.*$', '').Trim()
    if ($line -eq '') { continue }
    $f = @($line -split '[ \t]+' | Where-Object { $_ -ne '' })
    if ($f.Count -lt 3 -or $f.Count -gt 4) {
        Fail "expected public-name overlay alias [tail]: $line"
    }
    $public = $f[0]
    $owner = $f[1]
    $alias = $f[2]
    $tail = ($f.Count -eq 4 -and $f[3] -eq 'tail')
    if ($f.Count -eq 4 -and -not $tail) { Fail "unknown dispatch mode on $public" }
    if ($public -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or $public.Length -gt 12) {
        Fail "invalid public assembler name '$public'"
    }
    if ($written.ContainsKey($public)) { Fail "duplicate public export $public" }
    if (-not $ids.ContainsKey($owner)) { Fail "unknown overlay $owner for $public" }
    if (-not $aliasOwner.ContainsKey($alias) -or $aliasOwner[$alias] -ne $owner) {
        Fail "$public references $alias, not a DREL entry of overlay $owner"
    }
    $pattern = '(?im)^[ \t]*'+[regex]::Escape($alias)+'[ \t]+EQU[ \t]+0*([0-9A-F]+)H\b'
    $anchor = [regex]::Match($addrText,$pattern)
    if (-not $anchor.Success) { Fail "DREL address for $public/$alias is missing" }
    $address = [Convert]::ToInt32($anchor.Groups[1].Value,16)
    if ($address -lt 0x8000 -or $address -gt 0xBFFF) {
        Fail "$public/$alias resolves outside the code window"
    }
    $id = $ids[$owner]
    if (-not $segments.ContainsKey($id)) { Fail "no OVLTABLE row for $owner (ID $id)" }
    $segment = $address -shr 12
    if ($segments[$id] -notcontains $segment) {
        Fail "$public/$alias points at segment $segment, absent from $owner's runtime mapping"
    }
    # Preserve the two original top-level no-frame tail entries, and do
    # not let future "tail" flags become accidental cross-overlay returns.
    if ($tail -and @('R_DODECL','R_DOFN') -notcontains $public) {
        Fail "unsafe tail dispatch $public (only R_DODECL/R_DOFN allowed)"
    }
    if ((@('R_DODECL','R_DOFN') -contains $public) -and -not $tail) {
        Fail "original top-level $public must remain a tail call"
    }
    $entries += [pscustomobject]@{
        Public=$public; Owner=$owner; Alias=$alias; Tail=$tail
    }
    $written[$public] = $true
}

$required = @('R_DODECL','R_DOFN','R_STMT','R_PREP','R_DEFINE','R_INCLUDE',
              'R_DOASM','R_CCOUT','R_SETCODES','R_ERROR','R_STRUCT','R_TEST',
              'R_EXPR','R_CEXPR','R_EXPRVAL','R_GETOPTS')
foreach ($public in $required) {
    if (-not $written.ContainsKey($public)) { Fail "required legacy export $public is missing" }
}
if ($written.Count -ne $required.Count) { Fail "unexpected extra legacy dispatch exports" }

$a = @(
    '; GENERATED BY tools/ovlthunks.ps1 - DO NOT EDIT.',
    '; All normal R_* symbols are resident entry thunks into common OVCALL.',
    '; Multi-page overlays are mapped/restored as a whole by OVLMGR.',
    '        DXOP CALL,6',
    '        DXOP RET,7',
    'R1      EQU 1',
    'R2      EQU 2',
    '        INCL "OVLDEFS.INC"',
    '        INCL "OVLADDR.INC"',
    '        EXT OVCALL',
    '        EXT OVLMGR',
    ''
)
foreach ($e in $entries) {
    $a += "$($e.Public):"
    $a += "        LI R1,OVL_$($e.Owner)"
    if ($e.Tail) {
        # Equivalent to the two original tail branches: no extra frame
        # when the resident parse driver dispatches DECL or DFUN.
        $a += '        CALL @OVLMGR'
        $a += "        B @$($e.Alias)"
    } else {
        $a += "        LI R2,$($e.Alias)"
        $a += '        B @OVCALL'
    }
    $a += ''
}
foreach ($e in $entries) { $a += "        ENT $($e.Public)" }
$a += '        END'
[IO.File]::WriteAllText($Output,
    ([string]::Join([Environment]::NewLine,$a)+[Environment]::NewLine),
    [Text.Encoding]::ASCII)
Write-Output ("OVL99: generated {0} resident dispatch entries (2 top-level tail, {1} shared OVCALL)" -f $entries.Count,($entries.Count-2))
