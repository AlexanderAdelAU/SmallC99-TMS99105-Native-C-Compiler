#asm
        AORG 0A000H
#endasm

/*
** CC_CG99R.C -- peephole rules, page 3 of OVL_CGEN (>A000)
**
** The rule tables for qdump() in CC_CG99.C, J. E. Hendrix's Small-C 2.x
** CCC4 sequences adapted to this code generator.  OVL_CGEN maps this page
** with CC_CG99 (>8000) and CC_CG99T (>9000), so qdump() reads the tables
** directly; setrules() fills its rule[] table on first use.
*/

#define ADD12    1
#define ADDSP    2
#define AND12    3
#define ANEG1    4
#define ARGCNTn  5
#define ASL12    6
#define ASR12    7
#define CALL1    8
#define CALLm    9
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
#define NRULES   31

/*
** ==== Peephole rules (see qdump() in CC_CG99.C, which runs them) ====
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

extern int *rule[];

int r00[] = {ADD12,MOVE21,pfree,0,          go|p1,ADD21,0};
int r01[] = {ADD1n,0,                       ifl|m2,0,ifl|0,rDEC1,neg,0,ifl|p3,rINC1,0,0};
int r02[] = {ADD2n,0,                       ifl|m2,0,ifl|0,rDEC2,neg,0,ifl|p3,rINC2,0,0};
int r03[] = {rDEC1,PUTbp1,rINC1,0,          go|p2,ife|p1,DECbp,0,SUBbpn,0};
int r04[] = {rDEC1,PUTwp1,rINC1,0,          go|p2,ife|p1,DECwp,0,SUBwpn,0};
int r05[] = {GETw1m,MOVE21,pfree,0,         go|p1,GETw2m,gv|m1,0};
int r06[] = {GETw1m,PUSH1,pfree,0,          go|p1,PUSHm,gv|m1,0};
int r07[] = {GETw1p,PUSH1,pfree,0,          go|p1,PUSHp,gv|m1,0};
int r08[] = {GETw1s,GETw2n,ADD12,MOVE21,pfree,0,
                                            go|p3,ADD2n,gv|m2,go|m1,GETw2s,gv|m2,0};
int r09[] = {GETw1s,MOVE21,pfree,0,         go|p1,GETw2s,gv|m1,0};
int r10[] = {GETw2n,ADD12,sfree,0,          go|p1,ADD1n,gv|m1,0};
int r11[] = {rINC1,PUTbp1,rDEC1,0,          go|p2,ife|p1,INCbp,0,ADDbpn,0};
int r12[] = {rINC1,PUTwp1,rDEC1,0,          go|p2,ife|p1,INCwp,0,ADDwpn,0};
int r13[] = {MOVE21,GETw1n,SUB12,sfree,0,   go|p2,ADD1n,gv|m1,neg,0};
int r14[] = {MOVE21,GETw1n,comm,sfree,0,    go|p1,GETw2n,0};
int r15[] = {POINT1m,MOVE21,pfree,0,        go|p1,POINT2m,gv|m1,0};
int r16[] = {POINT1m,PUSH1,pfree,_pop,0,    topop|POINT2m,go|p2,0};
int r17[] = {POINT1s,GETw2n,ADD12,MOVE21,pfree,0,
                                            sum|p1,go|p3,POINT2s,gv|m3,0};
int r18[] = {POINT1s,PUSH1,MOVE21,pfree,0,  go|p1,POINT2s,gv|m1,go|p1,PUSH2,go|m1,0};
int r19[] = {POINT1s,PUSH1,pfree,_pop,0,    topop|POINT2s,go|p2,0};
int r20[] = {POINT1s,MOVE21,pfree,0,        go|p1,POINT2s,gv|m1,0};
int r21[] = {POINT2m,GETb1p,sfree,0,        go|p1,GETb1m,gv|m1,0};
int r22[] = {POINT2m,GETb1pu,sfree,0,       go|p1,GETb1mu,gv|m1,0};
int r23[] = {POINT2m,GETw1p,sfree,0,        go|p1,GETw1m,gv|m1,0};
int r24[] = {POINT2s,GETb1p,sfree,0,        sum|p1,go|p1,GETb1s,gv|m1,0};
int r25[] = {POINT2s,GETb1pu,sfree,0,       sum|p1,go|p1,GETb1su,gv|m1,0};
int r26[] = {POINT2s,GETw1p,PUSH1,pfree,sfree,0,
                                            sum|p1,go|p2,PUSHs,gv|m2,0};
int r27[] = {POINT2s,GETw1p,sfree,0,        sum|p1,go|p1,GETw1s,gv|m1,0};
int r28[] = {PUSH1,rpri,POP2,0,             go|p2,gc|m1,gv|m1,go|m1,MOVE21,0};
int r29[] = {PUSHm,_pop,0,                  topop|GETw2m,go|p1,0};
int r30[] = {PUSHs,_pop,0,                  topop|GETw2s,go|p1,0};

setrules()
{
    rule[0] = r00;   rule[1] = r01;   rule[2] = r02;   rule[3] = r03;
    rule[4] = r04;   rule[5] = r05;   rule[6] = r06;   rule[7] = r07;
    rule[8] = r08;   rule[9] = r09;   rule[10] = r10;  rule[11] = r11;
    rule[12] = r12;  rule[13] = r13;  rule[14] = r14;  rule[15] = r15;
    rule[16] = r16;  rule[17] = r17;  rule[18] = r18;  rule[19] = r19;
    rule[20] = r20;  rule[21] = r21;  rule[22] = r22;  rule[23] = r23;
    rule[24] = r24;  rule[25] = r25;  rule[26] = r26;  rule[27] = r27;
    rule[28] = r28;  rule[29] = r29;  rule[30] = r30;
}
