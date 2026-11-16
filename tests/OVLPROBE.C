/* OVLPROBE.C - Test 5: add int garr[10].\n   Keep SMALLC99.EXE unchanged while isolating CTEST failure. */

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
int garr[10];    /* Test 5: the only new declaration */

main()
{
    putchar('O');
    putchar('K');
    putchar(10);
}
