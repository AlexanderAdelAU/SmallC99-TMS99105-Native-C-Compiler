#asm
    AORG 9000H
#endasm

/*
** CC_ENUM.C: second page of OVL_STRD, physical page 1 at >9000.
** The enum parser shares OVL_STRD with dostruct, dfindtag and dnewtag
** in CC_STRD.C (>8000). Both pages are mapped simultaneously, so
** calls between them are direct and preserve the ownership contract.
**
** doenum() is the DREL anchor for this page.
*/
#define BPW       2
#define ENUMCONST 5
#define INT       8
#define AUTOMATIC 1
#define STATIC    2
#define VARIABLE  1
#define ARRAY     2
#define POINTER   3
#define TYPEDEFNAME 6
#define CHR       4
#define UCHR      5
#define UINT      9
#define IDENT     0
#define TYPE      1
#define SIZE      3
#define OFFSET    5
#define NAMESIZE  16
#define NUMTAGS   16
#define TAGSIZ    20
#define TAGSZ     16
#define TAGFIRST  18
#define TAGCNT    19

extern char tagtab[];
extern char *glbptr, *locptr;
extern int ntags, eof, ch;
extern char *lptr;

	/*
	** enum specifier: an enum has the representation of a 16-bit int.
	** Tag names reuse the existing tag namespace (TAGCNT=255 marks
	** enum, TAGFIRST marks a completed definition). Enumerators are
	** ordinary scoped symbols with immediate values in OFFSET.
	**
	** scope: 1 global, 2 local, 0 when used in a member type.
	** R_STRUCT's framed trampoline restores this page after a
	** constexpr() maps the expression overlay.
	*/
	doenum(scope) int scope; {
	  int tag, value, n, made;
	  char tname[NAMESIZE], ename[NAMESIZE];
	  char *t;
	  tname[0] = 0;
	  blanks();
	  if(alpha(ch)) symname(tname);
	  tag = 0;
	  made = 0;
	  if(tname[0]) {
	    tag = dfindtag(tname);
	    if(tag) {
	      t = tagtab + (tag - 1) * TAGSIZ;
	      if((t[TAGCNT] & 255) != 255)
	        error("tag already a struct");
	      }
	    else if(ntags >= NUMTAGS) error("too many enum tags");
	    else {
	      dnewtag(tname);
	      tag = ntags;
	      made = 1;
	      t = tagtab + (tag - 1) * TAGSIZ;
	      t[TAGCNT] = 255;
	      putint(BPW, t + TAGSZ, 2);
	      }
	    }
	  if(match("{")) {
	    if(tag) {
	      if(t[TAGFIRST] && !made) error("enum redefined");
	      t[TAGFIRST] = 1;
	      }
	    value = 0;
	    while(match("}") == 0) {
	      if(eof) return INT;
	      if(symname(ename) == 0) {
	        error("need enumerator name");
	        kill();
	        return INT;
	        }
	      if(match("=")) {
	        n = 0;
	        constexpr(&n);
	        value = n;
	        }
	      if(scope == 2) {
	        if(findloc(ename)) error("enum name redefined");
	        else addsym(ename, ENUMCONST, INT, BPW, value,
	                    &locptr, AUTOMATIC);
	        }
	      else {
	        if(findglb(ename)) error("enum name redefined");
	        else addsym(ename, ENUMCONST, INT, BPW, value,
	                    &glbptr, STATIC);
	        }
	      ++value;
	      if(match(",") == 0) { need("}"); break; }
	      if(match("}")) break;
	      }
	    }
	  else if(tag == 0) error("need enum tag or body");
	  return INT;
	  }

/* Shared declaration services are also in this simultaneously mapped
** OVL_STRD page. Public typeintro/aliasfix trampolines preserve callers' pages.
*/
/*
** Shared declaration type-specifier parser on OVL_STRD page 2. Declaration overlays ask
** for the base type and, for typedef names, the type's pre-existing
** VARIABLE/POINTER/ARRAY shape. Does not create storage.
**
** The resident layer owns lookup, while R_STRUCT switches into STRD
** for aggregate and enum specifiers. A failed identifier probe restores
** the source cursor for the old K&R function parser.
*/
typecore(tp, idp, sizep, local) int *tp, *idp, *sizep, local;
{
    char name[16];
    char *save;
    char *sym;
    int t;

    *idp = VARIABLE;
    if(amatch("char", 4)) t = CHR;
    else if(amatch("unsigned", 8)) {
        if(amatch("char", 4)) t = UCHR;
        else { amatch("int", 3); t = UINT; }
    }
    else if(amatch("int", 3)) t = INT;
    else if(amatch("struct", 6)) t = R_STRUCT(0, local == 2 ? 0 : 1);
    else if(amatch("union", 5)) t = R_STRUCT(1, local == 2 ? 0 : 1);
    else if(amatch("enum", 4)) t = R_STRUCT(3, local == 1 ? 2 : 1);
    else {
        blanks();
        if(alpha(ch) == 0) return 0;
        save = lptr;
        symname(name);
        sym = 0;
        if(local == 1) sym = findloc(name);
        if(sym == 0) sym = findglb(name);
        if(sym == 0 || sym[IDENT] != TYPEDEFNAME) {
            bump(save - lptr);
            return 0;
        }
        *tp = sym[TYPE];
        *idp = getint(sym + OFFSET, 2);
        *sizep = getint(sym + SIZE, 2);
        return 1;
    }
    *tp = t;
    *sizep = elsize(t);
    return 1;
}

/*
** Compose a declarator with a previously named typedef's shape.
** A pointer alias adds one implicit pointer level. An existing array
** alias can be named directly; forming an array OF that alias needs
** richer declarator descriptors and is diagnosed for now.
**
** parameter: array typedefs used as parameter types decay to pointers.
*/
aliascore(baseid, basesize, tp, idp, szp, parameter)
  int baseid, basesize, *tp, *idp, *szp, parameter;
{
    int n;
    if(baseid == POINTER) {
        if(*idp == VARIABLE) {
            *idp = POINTER;
            *szp = BPW;
        }
        else if(*idp == POINTER) {
            *tp += 64;
            *szp = BPW;
        }
        else if(*idp == ARRAY) {
            n = *szp / elsize(*tp);
            *tp += 64;
            *szp = n * BPW;
        }
    }
    else if(baseid == ARRAY) {
        if(*idp != VARIABLE) error("unsupported array typedef composition");
        else if(parameter) {
            *idp = POINTER;
            *szp = BPW;
        }
        else {
            *idp = ARRAY;
            *szp = basesize;
        }
    }
    if(*tp > 255) error("too many pointer levels");
}


