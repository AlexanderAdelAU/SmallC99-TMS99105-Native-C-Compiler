#asm
        AORG 08000H
#endasm

/*
** CC_CG99.C -- M38e native TMS99000 codegen engine, page 1 of OVL_CGEN
**
** OVL_CGEN is now two pages mapped simultaneously: this >8000 page owns
** ccout(), badcode(), and the hot code[] pointer table. CC_CG99T.C at
** >9000 owns setcodes() and every template literal. Calls/references between
** the two pages are direct because OVLMGR maps both under OVL_CGEN ID 6.
**
** DREL ANCHOR: ccout() -> P_CCOUT. Physical page 11, virtual segment 8.
** The code[] table intentionally stays with ccout(); only initialization and
** template strings moved, so the per-pcode hot path remains on the original page.
*/

#define PCODEMAX 100
#define NAME     7
#define YES      1
#define NO       0

#define ADD12     1
#define ADDSP     2
#define AND12     3
#define ANEG1     4
#define ARGCNTn   5
#define ASL12     6
#define ASR12     7
#define CALL1     8
#define CALLm     9
#define BYTE_    10
#define BYTEn    11
#define BYTEr0   12
#define COM1     13
#define DBL1     14
#define DBL2     15
#define DIV12    16
#define DIV12u   17
#define ENTER    18
#define EQ10f    19
#define EQ12     20
#define GE10f    21
#define GE12     22
#define GE12u    23
#define POINT1l  24
#define POINT1m  25
#define GETb1m   26
#define GETb1mu  27
#define GETb1p   28
#define GETb1pu  29
#define GETw1m   30
#define GETw1n   31
#define GETw1p   32
#define GETw2n   33
#define GT10f    34
#define GT12     35
#define GT12u    36
#define WORD_    37
#define WORDn    38
#define WORDr0   39
#define JMPm     40
#define LABm     41
#define LE10f    42
#define LE12     43
#define LE12u    44
#define LNEG1    45
#define LT10f    46
#define LT12     47
#define LT12u    48
#define MOD12    49
#define MOD12u   50
#define MOVE21   51
#define MUL12    52
#define MUL12u   53
#define NE10f    54
#define NE12     55
#define NEARm    56
#define OR12     57
#define POINT1s  58
#define POP2     59
#define PUSH1    60
#define PUTbm1   61
#define PUTbp1   62
#define PUTwm1   63
#define PUTwp1   64
#define rDEC1    65
#define REFm     66
#define RETURN   67
#define rINC1    68
#define SUB12    69
#define SWAP12   70
#define SWAP1s   71
#define SWITCH   72
#define XOR12    73
#define LSR12    74

/* optimizer-generated (peephole rules in CC_CG99.C) */
#define ADD1n    75
#define ADD21    76
#define ADD2n    77
#define ADDbpn   78
#define ADDwpn   79
#define DECbp    80
#define DECwp    81
#define POINT2m  82
#define GETb1s   83
#define GETb1su  84
#define GETw1s   85
#define GETw2m   86
#define GETw2p   87
#define GETw2s   88
#define INCbp    89
#define INCwp    90
#define POINT2s  91
#define PUSH2    92
#define PUSHm    93
#define PUSHp    94
#define PUSHs    95
#define rDEC2    96
#define rINC2    97
#define SUBbpn   98
#define SUBwpn   99

extern int litlab;
extern int errflag;
extern int tmsfail;
extern int tmsbad;
extern pstr();
extern pchar();
extern pdec();
extern pnl();
extern error();


/*
** ==== Peephole optimizer: J. E. Hendrix's Small-C 2.x CCC4 peep() ====
**
** dumpstage() (resident) hands each staged expression to qdump(), which
** tries the rules below at every p-code before writing it with ccout().
** A rule is a pattern, 0, then actions, 0:
**
**   pattern  p-codes to match in order, and checks at the position after
**            the last one matched: pfree / sfree (R4 / R3 not needed
**            later: isfree), _pop (the POP2 that matches a push).  any,
**            rpri (a p-code that only loads R4) and comm (a commutative
**            p-code) each match one entry.
**   actions  a p-code sets the entry at the cursor; go|n moves the cursor
**            n entries; gc|n / gv|n copy the code / value from n entries
**            away; sum|n adds that value; neg negates; topop|p turns the
**            matching POP2 into p-code p; ife|n / ifl|n do what follows
**            up to a 0 only if the value == n / < n.
**
** The rules are Hendrix's, minus those that need 8086 addressing, with
** pfree / sfree added wherever a rule drops a value the original left in
** R4 or R3 (his code generator could assume it; this one's step() keeps
** the address in R3 after a fetch).  Register effects come from the first
** character of each template: '@' + 010 (uses R4) 020 (sets R4) 001 (uses
** R3) 002 (sets R3).
*/

#define any    0x00FF
#define _pop   0x00FE
#define pfree  0x00FD
#define sfree  0x00FC
#define comm   0x00FB
#define rpri   0x00FA
#define go     0x0100
#define gc     0x0200
#define gv     0x0300
#define sum    0x0400
#define neg    0x0500
#define ife    0x0600
#define ifl    0x0700
#define topop  0x0900
#define p1     0x0001
#define p2     0x0002
#define p3     0x0003
#define m1     0x00FF
#define m2     0x00FE
#define m3     0x00FD
#define PRI    0x18     /* smallcp has no octal: 030 */
#define SEC    0x03
#define USES   0x09     /* 011 */
#define ZAPS   0x12     /* 022 */
#define NRULES 31

extern int stage[];
extern int usexpr;

int *qnext, *qtail;
int *rule[NRULES];

extern setrules();


ispush(pc) int pc;
{
    return pc == PUSH1 || pc == PUSH2 || pc == PUSHm
        || pc == PUSHp || pc == PUSHs || pc == ENTER;
}

iscomm(pc) int pc;
{
    return pc == ADD12 || pc == AND12 || pc == EQ12 || pc == MUL12
        || pc == MUL12u || pc == NE12 || pc == OR12 || pc == XOR12;
}

/*
** Is R4 (PRI) or R3 (SEC) free at pp: set before it is used again?
** At the end of the expression R3 is free, and R4 is free only if
** the statement does not use the value (usexpr).
*/
isfree(reg, pp) int reg, *pp;
{
    int r;
    while(pp < qtail) {
        r = regs(*pp);
        if(r & USES & reg) return 0;
        if(r & ZAPS & reg) return 1;
        pp = pp + 2;
    }
    if(usexpr) return reg & 1;
    return 1;
}

/*
** The POP2 that takes the value pushed just before next, or 0.
** Arguments to a call are not popped; ADDSP discards them.
*/
getpop(next) int *next;
{
    int level;
    level = 0;
    while(1) {
        if(next >= qtail) return 0;
        if(*next == POP2) {
            if(level) --level;
            else return next;
        }
        else if(*next == ADDSP) {
            level = level - (next[1] >> 1);
            if(level < 0) return 0;
        }
        else if(ispush(*next)) ++level;
        next = next + 2;
    }
}

/* try one rule at qnext; YES if it changed anything */
peep(seq) int *seq;
{
    int *next, *pop, n, k, skip, reply;
    next = qnext;
    pop = 0;
    while(*seq) {
        k = *seq;
        if(k == pfree) {
            if(isfree(PRI, next) == 0) return 0;
        }
        else if(k == sfree) {
            if(isfree(SEC, next) == 0) return 0;
        }
        else if(k == _pop) {
            if((pop = getpop(next)) == 0) return 0;
        }
        else {
            if(next >= qtail) return 0;
            if(k == rpri) {
                if(regs(*next) != 0x10) return 0;   /* only sets R4 */
            }
            else if(k == comm) {
                if(iscomm(*next) == 0) return 0;
            }
            else if(k != any) {
                if(*next != k) return 0;
            }
            next = next + 2;
        }
        ++seq;
    }
    reply = skip = 0;
    while(*(++seq) || skip) {
        if(skip) {
            if(*seq == 0) skip = 0;
            continue;
        }
        k = *seq & 0xFF00;
        if(k == 0) {                    /* a p-code for the cursor */
            qnext[0] = *seq;
            reply = 1;
            continue;
        }
        n = *seq & 0xFF;
        if(n > 127) n = n - 256;
        if(k == ife) {
            if(qnext[1] != n) skip = 1;
        }
        else if(k == ifl) {
            if(qnext[1] >= n) skip = 1;
        }
        else if(k == go) {
            qnext = qnext + n + n;
            reply = 1;
        }
        else {
            if(k == gc) qnext[0] = qnext[n + n];
            else if(k == gv) qnext[1] = qnext[n + n + 1];
            else if(k == sum) qnext[1] = qnext[1] + qnext[n + n + 1];
            else if(k == neg) qnext[1] = -qnext[1];
            else if(k == topop) {
                pop[0] = n;
                pop[1] = qnext[1];
            }
            reply = 1;
        }
    }
    return reply;
}

/*
** Write the staged p-codes from stage[] to end, optimized.  Called by
** the resident dumpstage() through R_DUMP.
*/
qdump(end) int *end;
{
    int i, tries;
    if(rule[0] == 0) setrules();
    qnext = stage;
    qtail = end;
    while(qnext < qtail) {
        i = 0;
        tries = 0;
        while(i < NRULES) {
            if(peep(rule[i])) {
                if(++tries < 40) {      /* a change: try them all again */
                    i = 0;
                    continue;
                }
            }
            ++i;
        }
        if(qnext < qtail) ccout(qnext[0], qnext[1]);
        qnext = qnext + 2;
    }
}

/*
** code[] must stay clear of >80C0->80DF.  The mapper also decodes a
** MEMORY write there (A11-A14 pick the segment), so a store into that
** window reprograms a map register.  As the first global, code[] sat
** at >8000-80C7 once it grew to 100 entries, and setcodes() writing
** code[96..99] remapped segments 0-3 - the resident compiler - so every
** compile died at once.  It therefore follows the optimiser code below
** rather than heading the module.  Keep writable data out of the window.
*/

char *code[PCODEMAX];

#define BLPFX  "\tBL @_cc"		/*  shared helper-call prefix   */

badcode(pcode)
int pcode;
{
    if(tmsfail == 0)
        tmsbad = pcode;
    tmsfail = 1;
    errflag = 1;
}

ccout(pcode, value)
int pcode;
int value;
{
    int part;
    int skip;
    int count;
    char *cp;
    char *back;
    char *tp;

    if(pcode < 1 || pcode >= PCODEMAX) {
        badcode(pcode);
        return;
    }

    tp = code[pcode];
    if(tp == 0) {
        badcode(pcode);
        return;
    }

    part = 0;
    back = 0;
    skip = NO;
    cp = tp + 1;

    while(*cp) {
        if(*cp == '<') {
            ++cp;
            if(skip == NO) {
                if(*cp == 'm') {
                    pstr(value + NAME);
                }
                else if(*cp == 'n') pdec(value);
                else if(*cp == 'l') pdec(litlab);
                /*  <e> emits R99's external marker.  It cannot be
                    written literally: '#' opens the repeat construct
                    below, so a bare ## in a template is swallowed and
                    the reference assembles as undefined.  */
                else if(*cp == 'e') { pchar('#'); pchar('#'); }
                /*  <b> is the shared call.A99 helper prefix.  Helpers
                    return directly in primary R4; no reload suffix exists.  */
                else if(*cp == 'b') pstr(BLPFX);
            }
            cp += 2;
        }
        else if(*cp == '?') {
            ++part;
            if(part == 1) {
                if(value == 0) skip = YES;
            }
            else if(part == 2) skip = !skip;
            else if(part == 3) {
                part = 0;
                skip = NO;
            }
            ++cp;
        }
        else if(*cp == '#') {
            ++cp;
            if(back == 0) {
                count = value;
                if(count < 1) {
                    while(*cp && *cp++ != '#') ;
                }
                else back = cp;
            }
            else {
                --count;
                if(count > 0) cp = back;
                else back = 0;
            }
        }
        else if(skip == NO) pchar(*cp++);
        else ++cp;
    }
}

/* the register-effect bits of a p-code's template; unknown = uses both */
regs(pc) int pc;
{
    char *cp;
    if(pc < 1 || pc >= PCODEMAX) return USES;
    cp = code[pc];
    if(cp == 0) return USES;
    return *cp - '@';
}
