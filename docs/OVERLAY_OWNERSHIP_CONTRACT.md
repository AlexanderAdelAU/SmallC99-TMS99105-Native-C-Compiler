# SMALLC99 overlay ownership contract

## The invariant

The framed overlay stubs save and restore one logical window owner: `CUR_WINA_ID`.
`OVLMGR` assigns that owner when an overlay maps virtual segment 8. Therefore a
normal/nestable overlay must map segment 8, and segment 8 must be its first table
entry.

In short:

```text
segment-8 owner == CUR_WINA_ID == logical active overlay
```

A nested service overlay must not execute only from segments A/B while another
overlay continues to own segment 8. That creates mixed ownership which cannot be
represented by the single saved `CUR_WINA_ID` used by the framed stubs.

## CGEN regression

The bad CGEN topology was:

```text
OVL_EXPR: 8=4, 9=5, A=7, B=8
OVL_CGEN: A=11, B=13
```

CGEN could therefore execute while `CUR_WINA_ID` still identified EXPR.

The corrected topology is:

```text
OVL_EXPR: 8=4, 9=5, A=7, B=8
OVL_CGEN: 8=11, 9=13
```

Mapping CGEN now makes ID 6 the logical window owner. Restoring the saved caller
ID then restores the complete expression mapping.

## Startup invariant

`PORT_INIT` must establish a known logical owner before any overlay dispatch:

```asm
CALL @OVLMGR_INIT
CLR  @CUR_WINA_ID
CALL @STUBINIT
```

This prevents a previous invocation's owner ID from being consumed as current
state.

## Build-time enforcement

LINK99 (`-YOVLMAP.TXT`, 3.10.0 or later) builds the overlay table and fails
the link if:

1. Any overlay row does not start with segment 8, or maps it more than once,
   except an overlay the map declares `guest` (CLI), which must NOT map it.
2. An overlay's segments are not contiguous, leave >8000-BFFF, or a physical
   page is used twice.

`make_smallc99.ps1` then fails the build, from the linker's own output, if:

3. CGEN is not exactly segment/page `8/11 9/13`, STMT not `8/3 9/14`, or CLI
   not `A/12`.
4. `ccout` does not resolve inside `>8000..>8FFF`, or `setcodes` inside
   `>9000..>9FFF`.
5. `PORT_INIT` does not clear `CUR_WINA_ID` immediately after `OVLMGR_INIT` and
   before `STUBINIT`.
6. Any `R_*` reference resolves outside resident memory.

A future overlay that genuinely cannot own segment 8 must not simply be declared
a guest. Its call/restore semantics must first be shown not to use the
framed `CUR_WINA_ID` ownership model. Exceptions are architecture decisions, not
space-allocation conveniences.

## Maintenance question

Whenever an overlay is added or moved, ask:

> Who owns segment 8 while this code is executing, and does `CUR_WINA_ID` agree?

The build now asks the same question automatically for the generated overlay
table.

The guard does not alter the OVLMGR mapping algorithm or the framed stub semantics.

## Deterministic regression proof

The ownership invariant was exercised directly with temporary runtime assertions.
With the corrected CGEN 8/9 layout, STRESS completed with 713 successful CGEN
ownership checks and 712 successful non-zero caller restores (`C60002C9` /
`C60102C8`). Repeating the run produced the same counts. Restoring only the old
A/B CGEN topology caused the first CGEN transition to fail immediately with
`C6E10000`, proving that the old layout did not establish CGEN as the segment-8
owner. The temporary runtime proof instrumentation is not part of production.
