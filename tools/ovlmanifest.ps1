# Parse SMALLC99's multi-page overlay manifest and reproduce the exact M38e
# DREL descriptors. First migration step: preserve all existing dispatch code,
# fixed overlay IDs, page layout and special one-shot CLI semantics.
param(
    [Parameter(Mandatory=$true)][string]$Map,
    [Parameter(Mandatory=$true)][string]$Defs
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

function Bad([string]$message) { throw "*** OVL99 manifest: $message" }

if (-not (Test-Path -LiteralPath $Map -PathType Leaf)) { Bad "missing $Map" }
if (-not (Test-Path -LiteralPath $Defs -PathType Leaf)) { Bad "missing $Defs" }

# Small-C's source-level overlay IDs are still hand-kept in OVLDEFS.INC.
# Cross-check them instead of silently changing them in the first phase.
$knownIds = @{}
foreach ($line in (Get-Content -LiteralPath $Defs)) {
    $m = [regex]::Match($line, '^[ \t]*OVL_([A-Z][A-Z0-9_]*)[ \t]+EQU[ \t]+([0-9]+)\b')
    if ($m.Success) {
        $knownIds[$m.Groups[1].Value] = [int]$m.Groups[2].Value
    }
}
if ($knownIds.Count -eq 0) { Bad "no logical overlay IDs found in $Defs" }

$modules = @{}
$pages = @{}
$entries = @{}
$aliases = @{}
$rows = @()
$lineNo = 0
foreach ($raw in (Get-Content -LiteralPath $Map)) {
    $lineNo++
    $line = ($raw -replace '#.*$', '').Trim()
    if ($line -eq '') { continue }
    $f = @($line -split '[ \t]+' | Where-Object { $_ -ne '' })
    if ($f.Count -ne 5) {
        Bad "line $lineNo needs module, physical page, overlay name, ID, entries"
    }
    $module = $f[0]
    $overlay = $f[2]
    $page = 0
    $id = 0
    if ($module -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or
        $overlay -cnotmatch '^[A-Z][A-Z0-9_]*$') {
        Bad "invalid module or overlay name at line $lineNo"
    }
    if (-not [int]::TryParse($f[1], [ref]$page) -or $page -lt 1 -or $page -gt 15) {
        Bad "invalid physical page at line $lineNo"
    }
    if (-not [int]::TryParse($f[3], [ref]$id) -or $id -lt 1 -or $id -gt 15) {
        Bad "invalid logical overlay ID at line $lineNo"
    }
    if (-not $knownIds.ContainsKey($overlay) -or $knownIds[$overlay] -ne $id) {
        Bad "line $lineNo overlay $overlay/$id disagrees with OVLDEFS.INC"
    }
    if ($modules.ContainsKey($module)) { Bad "duplicate module $module" }
    if ($pages.ContainsKey($page)) { Bad "physical page $page reused by $module" }
    $modules[$module] = $true
    $pages[$page] = $module

    $pieces = @($f[4] -split ',')
    if ($pieces.Count -eq 0) { Bad "line $lineNo has no DREL descriptors" }
    foreach ($piece in $pieces) {
        $match = [regex]::Match($piece, '^([A-Za-z_][A-Za-z0-9_]*):([A-Za-z_][A-Za-z0-9_]*|-)$')
        if (-not $match.Success) { Bad "invalid DREL entry '$piece' on line $lineNo" }
        $entry = $match.Groups[1].Value
        $alias = $match.Groups[2].Value
        if ($entries.ContainsKey($entry)) { Bad "duplicate entry $entry" }
        $entries[$entry] = $true
        if ($alias -ne '-') {
            if ($aliases.ContainsKey($alias)) { Bad "duplicate public alias $alias" }
            $aliases[$alias] = $true
        }
    }
    $rows += [pscustomobject]@{
        Name       = $module
        Page       = $page
        Overlay    = $overlay
        Id         = $id
        Descriptor = ("{0}.R99={1},{2},{3}" -f $module, $page, $id, $f[4])
    }
}
if ($rows.Count -eq 0) { Bad 'empty OVLMAP.TXT' }
foreach ($key in $knownIds.Keys) {
    if (-not (@($rows | Where-Object { $_.Overlay -eq $key }).Count -gt 0)) {
        Bad "overlay $key from OVLDEFS.INC has no module"
    }
}
# PowerShell writes each descriptor record on the success stream, without
# informational strings. The caller treats any parser error as a build failure.
$rows
