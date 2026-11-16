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


main()
{
    putchar('O');
    putchar('K');
    putchar(10);
}
