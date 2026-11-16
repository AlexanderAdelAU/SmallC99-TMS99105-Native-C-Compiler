/* OVLPROBE.C - staged SMALLC99 OVL99 compiler regression.
   Start with the exact CTEST conditional-macro prelude.
   Add one CTEST declaration at a time between extern and main.
   Compile with the SAME SMALLC99.EXE for every stage. */

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

/* INSERT ONE DECLARATION HERE PER TEST. */

main()
{
    putchar('O');
    putchar('K');
    putchar(10);
}
