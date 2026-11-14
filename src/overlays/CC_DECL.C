#asm
	AORG 8000H
#endasm

	/*
	** CC_DECL.C
	**
	** Declaration overlay (OVL_DECL) -- global declarations only.
	** Function-definition half now lives in CC_DFUN.C (OVL_DFUN).
	** Error helpers kill/illname/multidef and needsub moved to
	** CC_RESIDENT.C -- they are called from BOTH halves and a
	** direct same-window cross-page call is never legal.
	**
	** Overlay origin: segment >4000.
	*/

	#define BPW       2

	#define IDENT     0
	#define TYPE      1
	#define CLASS     2
	#define SIZE      3
	#define OFFSET    5

	#define LABEL     0
	#define VARIABLE  1
	#define ARRAY     2
	#define POINTER   3
	#define FUNCTION  4

	#define CHR       4
	#define INT       8
	#define UCHR      5
	#define UINT      9

	#define AUTOMATIC 1
	#define STATIC    2
	#define EXTERNAL  3
	#define AUTOEXT   4

	#define DATASEG   1

	#define BYTE_    10
	#define BYTEr0   12
	#define WORD_    37
	#define WORDr0   39

	#define LITMAX   256
	#define NAMESIZE  16
	#define CHRPTR   68        /* type byte of a char pointer: CHR + one level */

	extern char symtab[];
	int ityp = 0;               /* type byte of the object being initialised */
	int npstr = 0;              /* pointer-array elements written so far     */
	int pbase = 0;              /* their labels are _pbase_0, _pbase_1 ...   */
	int ptrcon = 0;             /* a pointer was given a constant: a WORD    */
	extern char litq[];

	extern char
	  *glbptr,
	  *locptr,
	  *line,
	  *lptr,
	   ssname[NAMESIZE];

	extern int
	  argstk,
	  argtop,
	  csp,
	  errflag,
	  lastst,
	  litlab,
	  litptr,
	  nogo,
	  noloc,
	  eof,
	  monitor;

	/*
	** struct|union: the parser is its own overlay (CC_STRD); reach it
	** through the framed trampoline R_STRUCT(isunion, outer).
	*/
	#define STRUCTBIT 2
	/*
	** Storage for a global struct object (or array of them): size bytes,
	** zero-filled.  Struct initialisers are phase 2.
	*/
	dstrobj(ident, size) int ident, size; {
	  public(ident);
	  if(match("=")) error("no struct initialiser yet");
	  gen(BYTEr0, size);
	  }

	/*
	** Test for global declarations.
	*/
	dodeclare(class) int class; {
	  if     (amatch("char",     4)) declglb(CHR,  class);
	  else if(amatch("unsigned", 8)) {
	    if   (amatch("char",     4)) declglb(UCHR, class);
	    else {
	      amatch("int", 3);
	      declglb(UINT, class);
	      }
	    }
	  else if(amatch("struct", 6))
	    declglb(R_STRUCT(0, 1), class);
	  else if(amatch("union", 5))
	    declglb(R_STRUCT(1, 1), class);
	  else if(amatch("int", 3) || class == EXTERNAL)
	    declglb(INT, class);
	  else
	    return 0;

	  ns();
	  return 1;
	  }

	/*
	** Declare a global/static object.
	*/
	declglb(type, class) int type, class; {
	  int id, dim, t, n;

	  while(1) {
	    if(endst()) return;
	    t = type;                 /* per declarator: char *a, b; */

	    if(match("*")) {
	      id  = POINTER;
	      dim = 0;
	      while(match("*")) t = t + 64;   /* pointer depth, bits 6-7 */
	      }
	    else {
	      id  = VARIABLE;
	      dim = 1;
	      }

	    if(symname(ssname) == 0) illname();
	    if(findglb(ssname)) multidef(ssname);

	    if(id == VARIABLE) {
	      if(match("(")) {
	        id = FUNCTION;
	        need(")");
	        }
	      else if(match("[")) {
	        id  = ARRAY;
	        dim = needsub();
	        t   = R_STRUCT(2, t);   /* further [..]: rows (CC_STRD) */
	        }
	      }
	    else if(match("[")) {     /* *name[n]: array of pointers */
	      t   = t + 64;
	      id  = ARRAY;
	      dim = needsub();
	      t   = R_STRUCT(2, t);
	      }
	    if(t > 255) error("too many *");

	    if(class == EXTERNAL)
	      external(ssname, elsize(t), id);
	    else if(id == FUNCTION)
	      ;
	    else if((t & 0xC2) == STRUCTBIT && id != POINTER)
	      dstrobj(id, dim * elsize(t));
	    else if(t & STRUCTBIT) {
	      ityp = 0;
	      n = initials(BPW, id, dim);
	      }
	    else {
	      ityp = t;
	      n = initials(elsize(t), id, dim);
	      }
	    if(id == ARRAY && dim == 0)   /* a[] = {...}: sized by its initialiser */
	      dim = n;

	    if(id == POINTER)
	      addsym(ssname, id, t, BPW, 0, &glbptr, class);
	    else
	      addsym(ssname, id, t, dim * elsize(t),
	             0, &glbptr, class);

	    if(match(",") == 0) return;
	    }
	  }

	/*
	** Initialise a global object.
	*/
	initials(size, ident, dim) int size, ident, dim; {
	  int savedim, pa;

	  litptr = 0;
	  npstr = 0;
	  ptrcon = 0;
	  if(dim == 0) dim = -1;
	  savedim = dim;
	  pa = (ident == ARRAY && (ityp & 0xC0));   /* an array of pointers: */
	  if(pa == 0) public(ident);                /* its elements go out   */
	                                            /* first, then its name  */

	  if(match("=")) {
	    if(match("{")) {
	      while(dim) {
	        init(size, ident, &dim);
	        if(match(",") == 0) break;
	        }
	      need("}");
	      }
	    else
	      init(size, ident, &dim);
	    }

	  if(savedim > 0 && dim < 0) {       /* more than was declared    */
	    if(dim == -1 && size == 1) {     /* char a[5] = "HELLO": as C, */
	      litptr--;                      /* no room for the 0 - drop it */
	      dim = 0;
	      }
	    else error("too many initialisers");
	    }

	  if(savedim == -1 && dim == -1) {
	    if(ident == ARRAY) error("need array size");
	    dstowlit(0, size = BPW);
	    }

	  if(pa) {
	    if(npstr) {              /* strings went out first: realign */
	      pstr("\tEVEN");
	      pnl();
	      }
	    public(ident);
	    }
	  if(ptrcon) size = BPW;     /* char *p = 0xF000: a word, whatever it */
	                             /* points at (the type byte says char)   */
	  if(npstr) dlabs();         /* char *a[]: WORD _pbase_0,_pbase_1... */
	  else ddumplit(size);
	  ddumpzer(size, dim);
	  if((litptr + (dim > 0 ? dim * size : 0)) & 1) {
	    pstr("\tEVEN");   /* TMS: realign after an odd */
	    pnl();            /* sized global object       */
	    }
	  if(savedim == -1) return -1 - dim;    /* a[]: elements given */
	  return savedim;
	  }

	/*
	** Evaluate one initializer.
	*/
	init(size, ident, dim) int size, ident, *dim; {
	  int value;

	  if(string(&value)) {
	    if(ident == ARRAY && (ityp & 0xFE) == CHRPTR) {
	      pel(1, 0);                   /* char *a[] = {"...", ...} */
	      *dim -= 1;
	      return;
	      }
	    if(ident == VARIABLE || size != 1)
	      error("must assign to char pointer or char array");

	    *dim -= litptr - value;

	    if(ident == POINTER)
	      dpoint();
	    }
	  else if(constexpr(&value)) {
	    if(ident == ARRAY && (ityp & 0xC0))
	      pel(0, value);               /* pointer array: 0 or an address */
	    else if(ident == POINTER) {    /* a pointer may take a constant: */
	      dstowlit(value, BPW);        /* 0, or an address like 0xF000   */
	      ptrcon = 1;
	      }
	    else
	      dstowlit(value, size);
	    *dim -= 1;
	    }
	  }

	/*
	** One element of a pointer-array initialiser, written at once: a
	** string as  _pbase_k: BYTE ...  (and the queue emptied), a constant
	** as  _pbase_k EQU value.  The array itself follows (dlabs).
	*/
	pel(isstr, v) int isstr, v; {
	  if(npstr == 0) pbase = getlabel();
	  pchar('_');
	  pdec(pbase);
	  pchar('_');
	  pdec(npstr);
	  ++npstr;
	  if(isstr) {
	    pchar(':');
	    pnl();
	    ddumplit(1);
	    litptr = 0;
	    }
	  else {
	    pstr("\tEQU\t");
	    pdec(v);
	    pnl();
	    }
	  }

	dlabs() {
	  int k;
	  k = 0;
	  while(k < npstr) {
	    if((k & 7) == 0) {
	      if(k) pnl();
	      gen(WORD_, 0);
	      }
	    else pchar(',');
	    pchar('_');
	    pdec(pbase);
	    pchar('_');
	    pdec(k);
	    ++k;
	    }
	  pnl();
	  }

	/*
	** M36b4 data-output helpers.
	**
	** litq[] is resident static storage. These routines live with the
	** declaration grammar because all global initializers call them and
	** this overlay has ample page headroom. They are the baseline CCC3/
	** CCC4 algorithms with resident output primitives substituted for
	** stdio. The function-literal pool has its own copy in CC_DFUN.C so
	** no same-window cross-overlay call is introduced.
	*/
	dstowlit(value, size) int value, size; {
	  if((litptr + size) >= LITMAX) {
	    error("literal queue overflow");
	    return;
	    }
	  putint(value, litq + litptr, size);
	  litptr += size;
	  }

	ddumplit(size) int size; {
	  int j, k;

	  k = 0;
	  while(k < litptr) {
	    if(size == 1)
	         gen(BYTE_, 0);
	    else gen(WORD_, 0);

	    j = 10;
	    while(j--) {
	      pdec(getint(litq + k, size));
	      k += size;
	      if(j == 0 || k >= litptr) {
	        pnl();
	        break;
	        }
	      pchar(',');
	      }
	    }
	  }

	ddumpzer(size, count) int size, count; {
	  if(count > 0) {
	    if(size == 1)
	         gen(BYTEr0, count);
	    else gen(WORDr0, count * BPW);
	    }
	  }

	dpoint() {
	  pstr(" WORD $+2");
	  pnl();
	  }
