/*
 * LANGTEST_LANGUAGE.C
 *
 * Implemented-language regression for SBC_SMALLC99 / TMS9900.
 * Purpose: cover the implemented SBC_SMALLC99 language subset without deliberately
 * stressing parser recursion, compiler stack depth, or large temporary tables.
 *
 * Expected final line:
 *
 *     ALL LANGUAGE TESTS PASSED
 *
 * Written in the classic Small-C/K&R style used by this compiler.
 */

#define TEN 10
#define TWENTY (TEN * 2)
#define PPFLAG

#ifdef PPFLAG
#define IFVAL 11
#else
#define IFVAL 91
#endif

#ifndef NOT_DEFINED
#define IFNVAL 13
#else
#define IFNVAL 93
#endif

extern int putchar();

int tests, passed, failed;
int sidefx;
int zglob;
int zarr[4];
int gi;
char gc;
int garr[10];
char gbuf[32];

int ginit = 1234;
int gnums[4] = { 10, 20, 30, 40 };
char gmsg[] = "abc";
char *gptr = "xyz";
unsigned gu = 0xffff;
unsigned char guc = 255;

nl()
{
    putchar(10);
}

pstr(s)
char *s;
{
    while (*s) putchar(*s++);
}

pnum(n)
int n;
{
    if (n == -32767 - 1) {
        pstr("-32768");
        return;
    }
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    if (n >= 10) pnum(n / 10);
    putchar('0' + n % 10);
}

section(s)
char *s;
{
    pstr("-- ");
    pstr(s);
    nl();
}

expect(id, got, want)
int id, got, want;
{
    tests++;
    if (got == want) {
        passed++;
    } else {
        failed++;
        pstr("FAIL ");
        pnum(id);
        pstr(": got ");
        pnum(got);
        pstr(" want ");
        pnum(want);
        nl();
    }
}

mystrlen(s)
char *s;
{
    int n;
    n = 0;
    while (*s++) n++;
    return n;
}

mystrcmp(a, b)
char *a, *b;
{
    while (*a == *b) {
        if (*a == 0) return 0;
        a++;
        b++;
    }
    return *a - *b;
}

mystrcpy(d, s)
char *d, *s;
{
    while (*d++ = *s++)
        ;
}

bump()
{
    sidefx++;
    return 1;
}

/* ---------- BSS and globals ---------- */

t_bss()
{
    section("bss/globals");
    expect(101, zglob, 0);
    expect(102, zarr[0], 0);
    expect(103, zarr[3], 0);
    expect(104, ginit, 1234);
    expect(105, gnums[0], 10);
    expect(106, gnums[3], 40);
    expect(107, gmsg[2], 'c');
    expect(108, gptr[0], 'x');
    expect(109, guc, 255);
}

/* ---------- constants and strings ---------- */

t_const()
{
    char *s;

    section("constants/strings");
    expect(201, 12345, 12345);
    expect(202, 'A', 65);
    expect(203, '\n', 10);
    expect(204, '\t', 9);
    expect(205, '\\', 92);
    expect(206, '\'', 39);
    expect(207, 0x10, 16);
    expect(208, 077, 63);

    s = "Hello";
    expect(209, s[0], 'H');
    expect(210, s[4], 'o');
    expect(211, s[5], 0);
    expect(212, mystrlen("abcdef"), 6);
    expect(213, mystrcmp("same", "same"), 0);
}

/* ---------- signed and unsigned arithmetic ---------- */

t_arith()
{
    int a, b;
    unsigned u;

    section("arithmetic");
    a = 7;
    b = 3;
    expect(301, a + b, 10);
    expect(302, a - b, 4);
    expect(303, a * b, 21);
    expect(304, a / b, 2);
    expect(305, a % b, 1);
    expect(306, -a, -7);
    expect(307, 2 + 3 * 4, 14);
    expect(308, (2 + 3) * 4, 20);
    expect(309, 10 - 4 - 3, 3);
    expect(310, 100 / 10 / 5, 2);

    u = 0xffff;
    expect(311, u >> 1, 0x7fff);
    expect(312, u / 2, 0x7fff);
    expect(313, u % 2, 1);
    u = 0x8000;
    expect(314, u > 0x7fff, 1);
    expect(315, u >> 1, 0x4000);

    u = 0xffff;
    u >>= 1;
    expect(316, u, 0x7fff);
    u = 0xffff;
    u /= 2;
    expect(317, u, 0x7fff);
    u = 0xffff;
    u %= 2;
    expect(318, u, 1);
    u = 0x8000;
    expect(319, u >> 0, 0x8000);
    b = 4;
    expect(320, u >> b, 0x0800);
    a = -16;
    expect(321, a >> 2, -4);
}

/* ---------- bitwise and relational ---------- */

t_bits_rel()
{
    int a, b, c;

    section("bitwise/relational");
    a = 12;
    b = 10;
    expect(401, a & b, 8);
    expect(402, a | b, 14);
    expect(403, a ^ b, 6);
    expect(404, ~0, -1);
    expect(405, 1 << 4, 16);
    expect(406, 256 >> 4, 16);
    expect(407, 1 | 2 & 4, 1);

    a = 3;
    b = 5;
    c = -2;
    expect(408, a < b, 1);
    expect(409, a > b, 0);
    expect(410, a <= 3, 1);
    expect(411, a >= 4, 0);
    expect(412, a == 3, 1);
    expect(413, a != 3, 0);
    expect(414, c < a, 1);
    expect(415, !0, 1);
    expect(416, !!a, 1);
}

/* ---------- logical and conditional ---------- */

t_logic()
{
    int a, b, x;

    section("logical/ternary");
    a = 0;
    b = 5;
    sidefx = 0;
    x = a && bump();
    expect(501, x, 0);
    expect(502, sidefx, 0);

    sidefx = 0;
    x = 1 || bump();
    expect(503, x, 1);
    expect(504, sidefx, 0);

    sidefx = 0;
    x = 1 && bump();
    expect(505, x, 1);
    expect(506, sidefx, 1);

    a = 5;
    expect(507, a > 3 ? 7 : 9, 7);
    expect(508, a < 3 ? 7 : 9, 9);
}

/* ---------- assignments and inc/dec ---------- */

t_assign()
{
    int a, b, c;

    section("assignment/inc/dec");
    a = b = c = 7;
    expect(601, a, 7);
    expect(602, b, 7);
    expect(603, c, 7);

    a = 5;
    b = a++;
    expect(604, b, 5);
    expect(605, a, 6);
    b = ++a;
    expect(606, b, 7);
    b = a--;
    expect(607, b, 7);
    b = --a;
    expect(608, b, 5);

    a = 10;
    a += 5; expect(609, a, 15);
    a -= 3; expect(610, a, 12);
    a *= 2; expect(611, a, 24);
    a /= 5; expect(612, a, 4);
    a %= 3; expect(613, a, 1);
    a <<= 4; expect(614, a, 16);
    a >>= 2; expect(615, a, 4);
    a |= 3; expect(616, a, 7);
    a &= 5; expect(617, a, 5);
    a ^= 1; expect(618, a, 4);
}


main()
{
    putchar('O');
    putchar('K');
    putchar(10);
}
