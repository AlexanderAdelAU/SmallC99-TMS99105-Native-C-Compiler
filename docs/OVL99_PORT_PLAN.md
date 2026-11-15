# OVL99 integration into SMALLC99 — staged acceptance plan

**Scope:** `ovl99-smallc99-integration` only. `main` remains the known-good
SMALLC99 implementation; the validated FileEdit integration remains frozen in
the separate TMS99105-Native-Toolchain repository.

## Why SMALLC99 is the second reference application

FileEdit proves transparent nested calls through a **single** 4 KiB code
segment (virtual `>8000–>8FFF`) while document data occupies segments 9–B
and optionally C–D.

SMALLC99 already has a working, manually written overlay architecture.
Its code window extends across **four** 4 KiB virtual segments (8–B,
`>8000–>BFFF`), with one logical overlay allowed to map several physical
pages *simultaneously*. The expression engine uses all four virtual segments;
STMT and CGEN have two-page arrangements. The current resident
`OVLSTUBS.A99` already has a 16-frame `TRSTACK` (two words per frame) and
restores the previous `CUR_WINA_ID` after cross-overlay calls.

**The objective is to unify the calling and generation mechanism, not to
redesign a working compiler or force FileEdit's single-segment restriction
onto SMALLC99.**

## Current baseline as inspected

- Eight logical overlays: EXPR (1), STMT (2), DECL (3), PREP (4),
  DFUN (5), CGEN (6), CLI (7), STRD (8).
- Fourteen physical code modules are defined in the current build.
- EXPR: physical pages 4, 5, 7, 8; virtual 8, 9, A, B.
- STMT: physical pages 3, 14; virtual 8 and 9. Current DREL 4.0 does
  **not** emit physical page 14 in that row; the existing build script
  explicitly patches the row. Preserve this until its actual cause is fixed.
- CGEN: physical pages 11, 13; virtual 8 and 9; exported entry anchors
  `P_CCOUT` and `P_SETCODES` must stay in their respective segments.
- CLI (ID 7): documented one-shot exception to the usual segment-8
  ownership invariant. Do not silently treat it as an ordinary nested
  overlay without proving its owner/restore semantics.
- No overlay code may cross into shell, BDOS, or ROM. Never pass pointers
  into an overlay's own static data to a callee that replaces its window.
- Existing resident helpers, state, `PORT_INIT` order, return-value ABI
  (R4), and frame-pointer conventions must be preserved.

See `docs/OVERLAY_OWNERSHIP_CONTRACT.md` and the existing `Readme.md`
for the authoritative SMALLC99-specific invariants.

## Milestones and merge gates

### 0. Freeze a reproducible SMALLC99 baseline — initial work

Run the current Windows cross-build and retain `SMALLC99.EXE` as an
artifact on this branch. Set up repeatable comparisons against `main`.
No compiler runtime changes are necessary at this point.

### 1. Make the existing overlay map declarative

Capture the current module/page/ID/entry descriptors in `OVLMAP.TXT`
and teach the build to load those declarations without changing the
generated DREL input or code layout. Keep existing `OVLSTUBS.A99`
and hand-maintained `OVLDEFS.INC` at this stage.

**Gate:** successful cross-build; unchanged DREL rows, entries,
resident symbol bindings and executable output compared to baseline
where deterministic output permits.

### 2. Generate equivalent resident OVL99 thunks

Extend FileEdit's generator for multi-segment logical overlays
(including same-overlay direct calls between co-resident pages).
Generate entry thunks for actual exported functions with the existing
Small-C ABI. Keep manual special cases (such as CLI) isolated and
retain a compatibility layer for `R_*` entry names until each caller
is migrated. Do not bypass resident switching from overlay code.

**Gate:** no unresolved symbols; every exported callable entry binds
to resident code; window ownership tables and 16-frame return behavior
validated under nested calls, including restore of *all* previous
pages of the caller's logical overlay.

### 3. Exercise the real compiler unchanged

Use the existing PC emulator/toolkit and compare generated `.A99`
outputs byte-for-byte against the known-good compiler for
`LANGTEST_LANGUAGE.C`, `STRUCTTEST.C`, `PTRTEST.C`,
`ARRTEST.C` and other regression fixtures. Rebuild and run those
programs, including erroneous-input tests; then repeat on the SBC.

Measure overlay-page budgets, resident space and compile time.

### 4. Publish the shared architecture

When both FileEdit and SMALLC99 pass, factor out reusable OVCALL/
generation code and write the general TMS99105 overlay contract.
Keep automatic module/page placement as a **separate unimplemented
feature**, not part of this port's acceptance claim.

## Phase-1 progress (15 November 2026)

- Created the isolated integration branch and a Windows GitHub Actions
  cross-build workflow. The legacy R99 assembler assumes a C: build drive;
  made the source/home flags depend on the actual build volume. This
  retains the original `SCHC` flags when building on C: but supports D:
  on GitHub's runner.
- The **unchanged** legacy compiler successfully cross-built once that
  host path issue was corrected
  ([baseline build](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler/actions/runs/43710678745)).
- Introduced `OVLMAP.TXT`, checked that its **14 descriptors are identical
  in order and content** to the previously hard-coded `$OVERLAYS` table,
  and loaded it via the validated `tools/ovlmanifest.ps1` parser.
  `OVLDEFS.INC` remains authoritative for the original fixed IDs;
  disagreement causes a pre-build error.
- Rebuilt successfully after the manifest substitution
  ([phase-1 build](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler/actions/runs/43710796883)).
  DREL's existing STMT page-14 repair and CGEN page-11/13 ownership checks
  still run and pass.
- **No runtime assembly, C compiler source or linker has been replaced.**
  The manually written stubs remain active. Executable byte-for-byte
  identity with the baseline has not yet been independently demonstrated;
  neither the emulator nor SBC runtime test has run on this branch.

This first step proves that the existing multi-page layout can be maintained
as an explicit manifest without disturbing the current Windows cross-build.
It is not yet the completed OVL99 runtime port.

## Phase-2 progress: replace active handwritten trampolines (15 November 2026)

The branch now contains a **cross-built, linked OVL99 dispatch replacement**:

- `OVLTHUNKS.TXT` declares all 16 legacy public `R_*` names,
  mapping each to a logical overlay ID and its existing DREL entry alias.
- `tools/ovlthunks.ps1` checks the aliases against `OVLMAP.TXT`,
  `OVLDEFS.INC`, DREL's published `OVLADDR.INC`, and the final
  `OVLTABLE.INC` after the known STMT page-14 repair. A declared
  entry must actually occupy a segment mapped by its logical owner.
- The generator creates `OVLTHUNK.A99`, resident entry names linked to
  the *same* C-facing symbols used by the original compiler.
- `src/resident/OVCALL.A99` is now the **one resident framed gate**:
  it saves the caller's return address and previous segment-8 logical
  owner on a 16-frame stack, calls the existing multi-page `OVLMGR`,
  runs the published implementation, then restores the caller's full
  logical overlay mapping and original return address. R4 and Small-C
  stack arguments retain their original semantics.
- The two original top-level `R_DODECL` and `R_DOFN` entry points are
  generated as **tail calls**, preserving their deliberate no-extra-frame
  dispatch behaviour. CLI's ID 7 is still a segment-A-only exception,
  and `R_GETOPTS`/`R_ERROR` now use the shared gate rather than
  `CLISTUB` / manually framed `OVLSTUBS` bodies.
- For SMALLC99, the generic OVCALL deliberately **does not skip the
  mapping/restore on equal window-owner IDs**. Unlike FileEdit,
  SMALLC99 permits CLI to transiently remap segment A without becoming
  the segment-8 owner; this makes that shortcut unsafe in some nested
  situations. The original framed trampolines always remapped.
- `make_smallc99.ps1` no longer stages, assembles or links
  `OVLSTUBS.A99` and `CLISTUB.A99`. Those source files remain in
  the repository as historical references only. The retired
  `R_CODEON` no-op is defined in the single gate to preserve its
  compatibility symbol.
- Linker output is audited: referenced `R_*` symbols must resolve
  into resident `>1000–>7FFF`, **not** the remappable
  `>8000–>BFFF` code window.

**Latest build proof:**
[Windows Actions run 43712761872](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler/actions/runs/43712761872)
completed successfully, reporting:

```text
OVL99: generated 16 resident dispatch entries (2 top-level tail, 14 shared OVCALL)
OVL99 linker audit: 15 referenced R_* names resolve in resident memory.
BUILD SUCCESSFUL - MILESTONE 38e TWO-PAGE CODEGEN OVERLAY -
```

This is the **replacement of active handwritten trampolines** at
build/link level. It does **not** yet establish runtime equivalence.
Next acceptance gate: run the existing SMALLC99 emulator regression
suite against the new EXE, compare generated assembly to the known-good
compiler, and check on hardware (especially nested expressions,
error reporting/CLI's segment-A-only mapping, switch cases, structs,
and the multi-page CGEN backend). No merge to `main` before that.

## First SBC runtime result: INT2 during CTEST compile (15 Nov 2026)

User subsequently confirmed `CTEST.A99` is created but empty. Because the
output stream may be buffered until close, zero bytes does not determine
whether the failure occurred before or during `parse()`.

A temporary diagnostic build now prints stage markers `OVT A` through
`OVT G` immediately around compiler initialization, file opening,
firstline preprocessing, header generation, parsing and trailer generation.
This is *instrumentation*, not an attempted runtime fix. The Windows
cross-build of that diagnostic version passed:
[run 43715676825](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler/actions/runs/43715676825).
The next SBC observation needed is the **last `OVT` marker before INT2**.
Remove these temporary messages after the fault is localized.

User reported that the newly generated-dispatch SMALLC99 EXE loaded and
began the command:

```text
%smallc99 ctest.c -v
SMALLC99: compiling CTEST.C -> CTEST.A99
INT2 PC:
```

The SBC appeared to hang after printing `INT2 PC:`. The original compiler
successfully passes this same LANGUAGE test, per the user's clarification.
The new compiler had
been running for a noticeable period, but its current phase and the
source line at failure are not yet established. No complete INT2 register
dump was captured. The locally supplied Windows build log *does* contain
`OVL99: generated 16 resident dispatch entries`,
`OVL99 linker audit: 15 referenced R_* names resolve in resident memory`,
and `BUILD SUCCESSFUL`. This confirms link-stage success but **not**
runtime correctness.

**Acceptance status: SBC runtime FAIL / investigation required. Do not
merge into main.**

Immediate controlled triage:
1. Reset the SBC; test a very small, known-good C program with this
   same OVL99 executable (prefer `M38B.C` or another short fixture).
2. **Confirmed by user:** `CTEST.C` is the principal `LANGTEST_LANGUAGE.C`
   regression renamed; it compiles successfully with the original
   known-good SMALLC99. Treat the INT2 as specific to the replacement
   runtime until contradicted. Do not request a redundant baseline retest.
3. After resetting, inspect whether `CTEST.A99` exists and how much
   was written. Do not infer it reached the end based only on elapsed
   time. Avoid shell `TYPE` for a long file while its separate
   intermittent INT2 issue remains under investigation.
4. If new build only fails, inspect common OVCALL nested frame
   integrity, entry dispatch and CLI segment-A exception with focused
   instrumentation or emulator traces. If both fail, isolate shell,
   runtime or source-specific causes before changing overlay code.

## Work that is explicitly *not* part of the initial branch

- No changes to Windows LINK99, Small-C language semantics or BDOS.
- No automatic redistribution of functions across pages yet.
- No removal of `OVLSTUBS.A99` until each existing caller is accounted
  for and the compiler has passed regression tests.
- No merge into SMALLC99 `main` before independent cross-build,
  emulator and SBC checks.


## OVL99 author's implementation cautions and exact INT2 locus

The OVL99 author supplied five important constraints during the first SBC
regression investigation:

1. **LINK99 3.9.59 can silently misplace external references** if all four
   `CC_EXPR` files are linked as *one* module spanning four pages (`-P4-7`).
   They measured 110/189 wrong relocations and have an incomplete patch.
   **Do not adopt that consolidation for this port**. The current build
   deliberately links `CC_EXPR_A`, `CC_EXPR_B`, `CC_EXPR_C` and
   `CC_EXPR_D` as separate R99 modules in separate physical pages. The
   reported defect therefore does not, by itself, explain this run.
2. **CLI ID 7 maps only segment A** and must remain non-reentrant: CLI
   execution must never invoke a new thunk while this exception is active.
   The current `OVCALL` has no same-ID shortcut; still audit its callers
   and transient ownership carefully.
3. **Overlay-owned data pointers may be invalidated by a remap**:
   `swstab`, `macn`/`macq`, `code[]`, and overlay-local literals.
   Specifically audit `docase()` → `constexpr()` and any
   `error("...")` call originating on a `>A000` code page (CLI maps
   that same segment). This is a general safety rule independent of
   the new OVCALL's syntax.
4. **Keep public links bound to resident thunks** if thunk/public
   implementation symbols are aliased. Current SMALLC99 intentionally
   uses separate `R_*` exported symbols, and its post-link resident
   thunk audit passed; never weaken that check.
5. **Function addresses taken inside an overlay** refer to the mapped
   window, not automatically to a resident thunk. Audit for indirect
   calls and escapes before treating function pointers as transparent.

The first stage-marked SBC crash was after `OVT E: header emitted`
and before `OVT F: parse complete`. The monitor reports
`INT2 PC:92E4 WP:0230 ST:C487 SEG:0009 PAGE:0000`.

The Windows CI artifact from run 43716634380 contains an
`OVLTABLE.INC` row for logical STMT ID 2:
`8,3,9,14,0,...`. The linker log resolves `dogoto` at `>92D2`,
so `>92E4` is 18 bytes into the statement overlay's page-2
`dogoto()` code. This makes **segment-9 mapping / restoration**
a high-priority hypothesis. The monitor's `PAGE:0000` reading
appears inconsistent with page 14, but confirm what that monitor
register displays during INT2 before assuming it reports the
pre-fault mapping. It is not yet proof of a specific OVCALL bug.

**Next diagnostic, not yet a fix:** capture/check the expected segment-9
physical page and saved `CUR_WINA_ID` around a nested statement →
preprocessor/expression/CLI transition. Compare with the legacy stub
and identify whether the page was never mapped, improperly restored,
or remapped afterward. Keep the FileEdit runtime frozen and both
SMALLC99 and LINK99 production branches untouched.


## Focused diagnostic for SBC INT2 at >92E4 (15 November 2026)

Artifact review found `dogoto` published at `>92D2` in the generated
`CC_STMT_R.A99`. With its early instruction sizes, `>92E4` falls at
`MOV R4,*SP` in the branch that constructs the argument for
`error("not allowed with block-locals")`. This makes corrupted SP or
mapping a plausible hypothesis, **not** a confirmed explanation.

A temporary `MAPDIAG` entry has been added to resident
`OVLMGR.A99` and called at the start of `dogoto()`. It writes four
16-bit words via WHEX (no code or data mappings are changed):

1. `D900` — diagnostic tag
2. `CUR_WINA_ID` — expected logical owner `0002` (STMT)
3. `CURRENT_PAGE[9]` — expected tracked physical page `000E` (14)
4. `SP` — compiler stack pointer at the diagnostic call

This is a **software tracker**, not a direct read of the CRU mapping
register, so even `0002/000E` does not by itself establish actual
hardware page selection. The original interrupt monitor's `PAGE:0000`
may be post-interrupt state and must not be assumed to reflect the
last executing instruction's page.

The diagnostic source and assembled artifact have passed Windows CI
[run 43718260384](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler/actions/runs/43718260384);
the build log confirms `MAPDIAG` from `CC_STMT_R` resolves to
resident `>178A`, and the normal resident thunk audit also passes.
It has **not** been run on the SBC yet.

**Next SBC test:** Pull `ovl99-smallc99-integration`, rebuild, transfer
the new `SMALLC99.EXE`, run `SMALLC99 CTEST.C -V`, and capture the
last OVT marker plus any four WHEX words starting with `D900` and
the INT2 register report. This is diagnostic only. Do not merge into
`main` or treat the fault as resolved.


## Second SBC regression: same INT2 without D900; verified build stamp

After the first `MAPDIAG` diagnostic rebuild, the user ran
`SMALLC99 CTEST.C -V` and again received `OVT A` through `OVT E`,
then `INT2 PC:92E4 WP:0230 ST:C487 SEG:0009 PAGE:0000`. No `D900`
words were visible. This **does not prove** the probe was absent: a
wrong overlay mapping could prevent execution from reaching the probe
even if the updated binary is on the SBC.

The verified Windows build artifact with the probe **was different**
from the earlier executable: its `CC_STMT_R.A99` has
`CALL @MAPDIAG` as the first instruction in `dogoto()`. The
`dogoto` linker anchor remains `>92D2`, so the unchanged fault PC
is insufficient to establish either an old binary or an actual
`dogoto` entry. No runtime fix has been made yet.

To separate SBC deployment/version from a pre-probe execution fault,
added two distinct **resident, console-only** markers:
- `OVT H: MAPDIAG-BUILD-2` immediately after `OVT E`, before
  calling `parse()`
- `OVT P: entered parse` at the beginning of resident `parse()`

This build passed Windows CI
[run 43719536877](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler/actions/runs/43719536877).
The downloadable `SMALLC99.EXE` was checked and **contains both
marker strings** (they were absent from the previous diagnostic EXE).
The instrumentation at `dogoto` (`D900`) remains present.

Next SBC test: pull the integration branch, rebuild and transfer that
fresh EXE, run CTEST, and report whether **H** and **P** appear before
INT2. If H is absent, investigate branch/build/deployment identity.
If H and P appear but D900 does not, investigate execution/mapping
within `parse` before entering the real `dogoto` code. Do not
conclude `dogoto` was executed merely from fault PC `>92E4`.


## First complete SBC CTEST compile with generated OVL99 gate (16 November 2026)

**Observed successful hardware run of the diagnostic build.** After a
fresh TMS99105 disc-monitor reboot and XMODEM transfer of
`SMALLC99.EXE` (load address `1000`), the user ran:

```text
SMALLC99 CTEST.C -V
```

Tera Term screenshots show:

```text
SMALLC99: compiling CTEST.C -> CTEST.A99
OVT A: entering compinit
OVT B: compinit complete
OVT C: files open
OVT D: first line read
OVT E: header emitted
OVT H: MAPDIAG-BUILD-2
OVT P: entered parse
D900 0002 000E ...  [WHEX emits adjacent 16-bit words]
OVT F: parse complete
OVT G: trailer emitted
SMALLC99: wrote CTEST.A99
```

The user also displayed the tail of `CTEST.A99` and verified
normal generated assembler lines finishing with `EVEN` and `END`.
There was **no INT2** during this compile. `CTEST.C` is their
renamed canonical `LANGTEST_LANGUAGE.C` regression.

The mapping probe's `0002` identifies logical STMT owner and
`000E` identifies tracked segment-9 physical page 14, as expected.
This is software-tracked state and does not independently interrogate
the hardware CRU register.

**Acceptance progress:** full compiler invocation, parser completion,
output-file close and generated assembly are verified on SBC with
generated resident OVL99 dispatch. This is the first SBC PASS for the
integration, but it is **not yet clean production-binary or full
LANGUAGE runtime acceptance**.

**Next gates:** Assemble `CTEST.A99` using `R99 CTEST`, link
`LINK99 CTEST.R99 IOLIB99.LIB`, run `CTEST`, and confirm
`tests: 133  passed: 133  failed: 0` and
`ALL LANGUAGE TESTS PASSED`. Then reproduce the compile with the
temporary `OVT` and `MAPDIAG` instrumentation removed from
the integration branch, rebuilding and re-transferring the exact EXE.
The earlier reproducible INT2 on the uninstrumented/incompletely
identified build remains unexplained. The layout change and/or
fresh transfer might account for the difference; do not claim
the defect is fixed until the clean build also passes. No merge to
`main` before those gates.
