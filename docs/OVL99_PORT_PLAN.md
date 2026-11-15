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

## Work that is explicitly *not* part of the initial branch

- No changes to Windows LINK99, Small-C language semantics or BDOS.
- No automatic redistribution of functions across pages yet.
- No removal of `OVLSTUBS.A99` until each existing caller is accounted
  for and the compiler has passed regression tests.
- No merge into SMALLC99 `main` before independent cross-build,
  emulator and SBC checks.
