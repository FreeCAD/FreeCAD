/* trte.f -- translated by f2c (version 20100827).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

/*
 Since Mefisto is using very little real fortran API calls only the definition 
 portion of the f2c header is used and the fortran write() calls are commented out:

 	//io___187.ciunit = unites_2.imprim;
	//s_wsle(&io___187);
	//do_lio(&c__9, &c__1, "pas de triangle d'abscisse minimale", (ftnlen)
	//	35);
	//e_wsle();

This calls print out error information and are not needed in FreeCAD....

JR 2014
*/

// start F2c.h content
typedef long int integer;
typedef unsigned long int uinteger;
typedef char *address;
typedef short int shortint;
typedef float real;
typedef double doublereal;
typedef struct { real r, i; } complex;
typedef struct { doublereal r, i; } doublecomplex;
typedef long int logical;
typedef short int shortlogical;
typedef char logical1;
typedef char integer1;
#ifdef INTEGER_STAR_8	/* Adjust for integer*8. */
typedef long long longint;		/* system-dependent */
typedef unsigned long long ulongint;	/* system-dependent */
#define qbit_clear(a,b)	((a) & ~((ulongint)1 << (b)))
#define qbit_set(a,b)	((a) |  ((ulongint)1 << (b)))
#endif

#define TRUE_ (1)
#define FALSE_ (0)

/* Extern is for use with -E */
#ifndef Extern
#define Extern extern
#endif

/* I/O stuff */

#ifdef f2c_i2
/* for -i2 */
typedef short flag;
typedef short ftnlen;
typedef short ftnint;
#else
typedef long int flag;
typedef long int ftnlen;
typedef long int ftnint;
#endif

/*external read, write*/
typedef struct
{	flag cierr;
	ftnint ciunit;
	flag ciend;
	char *cifmt;
	ftnint cirec;
} cilist;

/*internal read, write*/
typedef struct
{	flag icierr;
	char *iciunit;
	flag iciend;
	char *icifmt;
	ftnint icirlen;
	ftnint icirnum;
} icilist;

/*open*/
typedef struct
{	flag oerr;
	ftnint ounit;
	char *ofnm;
	ftnlen ofnmlen;
	char *osta;
	char *oacc;
	char *ofm;
	ftnint orl;
	char *oblnk;
} olist;

/*close*/
typedef struct
{	flag cerr;
	ftnint cunit;
	char *csta;
} cllist;

/*rewind, backspace, endfile*/
typedef struct
{	flag aerr;
	ftnint aunit;
} alist;

/* inquire */
typedef struct
{	flag inerr;
	ftnint inunit;
	char *infile;
	ftnlen infilen;
	ftnint	*inex;	/*parameters in standard's order*/
	ftnint	*inopen;
	ftnint	*innum;
	ftnint	*innamed;
	char	*inname;
	ftnlen	innamlen;
	char	*inacc;
	ftnlen	inacclen;
	char	*inseq;
	ftnlen	inseqlen;
	char 	*indir;
	ftnlen	indirlen;
	char	*infmt;
	ftnlen	infmtlen;
	char	*inform;
	ftnint	informlen;
	char	*inunf;
	ftnlen	inunflen;
	ftnint	*inrecl;
	ftnint	*innrec;
	char	*inblank;
	ftnlen	inblanklen;
} inlist;

#define VOID void

union Multitype {	/* for multiple entry points */
	integer1 g;
	shortint h;
	integer i;
	/* longint j; */
	real r;
	doublereal d;
	complex c;
	doublecomplex z;
	};

typedef union Multitype Multitype;

/*typedef long int Long;*/	/* No longer used; formerly in Namelist */

struct Vardesc {	/* for Namelist */
	char *name;
	char *addr;
	ftnlen *dims;
	int  type;
	};
typedef struct Vardesc Vardesc;

struct Namelist {
	char *name;
	Vardesc **vars;
	int nvars;
	};
typedef struct Namelist Namelist;

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define dabs(x) (doublereal)abs(x)
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#define dmin(a,b) (doublereal)min(a,b)
#define dmax(a,b) (doublereal)max(a,b)
#define bit_test(a,b)	((a) >> (b) & 1)
#define bit_clear(a,b)	((a) & ~((uinteger)1 << (b)))
#define bit_set(a,b)	((a) |  ((uinteger)1 << (b)))

/* procedure parameter types for -A and -C++ */

#define F2C_proc_par_types 1
#ifdef NIX
typedef int /* Unknown procedure type */ (*U_fp)(...);
typedef shortint (*J_fp)(...);
typedef integer (*I_fp)(...);
typedef real (*R_fp)(...);
typedef doublereal (*D_fp)(...), (*E_fp)(...);
typedef /* Complex */ VOID (*C_fp)(...);
typedef /* Double Complex */ VOID (*Z_fp)(...);
typedef logical (*L_fp)(...);
typedef shortlogical (*K_fp)(...);
typedef /* Character */ VOID (*H_fp)(...);
typedef /* Subroutine */ int (*S_fp)(...);
#else
typedef int /* Unknown procedure type */ (*U_fp)();
typedef shortint (*J_fp)();
typedef integer (*I_fp)();
typedef real (*R_fp)();
typedef doublereal (*D_fp)(), (*E_fp)();
typedef /* Complex */ VOID (*C_fp)();
typedef /* Double Complex */ VOID (*Z_fp)();
typedef logical (*L_fp)();
typedef shortlogical (*K_fp)();
typedef /* Character */ VOID (*H_fp)();
typedef /* Subroutine */ int (*S_fp)();
#endif
/* E_fp is for real functions when -R is not specified */
typedef VOID C_f;	/* complex function */
typedef VOID H_f;	/* character function */
typedef VOID Z_f;	/* double complex function */
typedef doublereal E_f;	/* real function with -R not specified */

/* undef any lower-case symbols that your C compiler predefines, e.g.: */

#ifndef Skip_f2c_Undefs
#undef cray
#undef gcos
#undef mc68010
#undef mc68020
#undef mips
#undef pdp11
#undef sgi
#undef sparc
#undef sun
#undef sun2
#undef sun3
#undef sun4
#undef u370
#undef u3b
#undef u3b2
#undef u3b5
#undef unix
#undef vax
#endif

// stop F2c.h content


union {
    struct {
	integer lecteu, imprim, nunite[30];
    } _1;
    struct {
	integer lecteu, imprim, intera, nunite[29];
    } _2;
} unites_;

#define unites_1 (unites_._1)
#define unites_2 (unites_._2)

/* Table of constant values */

static integer c__9 = 9;
static integer c__1 = 1;
static integer c__3 = 3;
static integer c__2 = 2;
static integer c__5 = 5;
static doublereal c_b357 = .1;
static integer c_n1 = -1;
static integer c__0 = 0;
static integer c__512 = 512;
static integer c__32 = 32;

/*  MEFISTO2: a library to compute 2D triangulation from segmented boundaries */

/*  Copyright (C) 2006  Laboratoire J.-L. Lions UPMC Paris */

/*  This library is free software; you can redistribute it and/or */
/*  modify it under the terms of the GNU Lesser General Public */
/*  License as published by the Free Software Foundation; either */
/*  version 2.1 of the License. */

/*  This library is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/*  Lesser General Public License for more details. */

/*  You should have received a copy of the GNU Lesser General Public */
/*  License along with this library; if not, write to the Free Software */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */

/*  See http://www.ann.jussieu.fr/~perronnet or email perronnet@ann.jussieu.fr */

/*  File   : trte.f    le Fortran du trianguleur plan */
/*  Module : SMESH */
/*  Author : Alain PERRONNET */
/*  Date   : 13 novembre 2006 */
doublereal diptdr_(doublereal *pt, doublereal *p1dr, doublereal *p2dr)
{
    /* System generated locals */
    doublereal ret_val, d__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal a, b, c__;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++012 */
/* but : calculer la distance entre un point et une droite */
/* ----- definie par 2 points p1dr et p2dr */

/* entrees : */
/* --------- */
/* pt        : le point de R ** 2 */
/* p1dr p2dr : les 2 points de R ** 2  de la droite */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++012 */
/* programmeur : alain perronnet analyse numrique paris  janvier 1986 */
/* ....................................................................012 */

/*     les coefficients de la droite a x + by + c =0 */
    /* Parameter adjustments */
    --p2dr;
    --p1dr;
    --pt;

    /* Function Body */
    a = p2dr[2] - p1dr[2];
    b = p1dr[1] - p2dr[1];
    c__ = -a * p1dr[1] - b * p1dr[2];

/*     la distance = | a * x + b * y + c | / sqrt( a*a + b*b ) */
    ret_val = (d__1 = a * pt[1] + b * pt[2] + c__, abs(d__1)) / sqrt(a * a + 
	    b * b);
    return ret_val;
} /* diptdr_ */

/* Subroutine */ int qutr2d_(doublereal *p1, doublereal *p2, doublereal *p3, 
	doublereal *qualite)
{
    /* System generated locals */
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal a, b, c__, p;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :     calculer la qualite d'un triangle de r**2 */
/* -----     2 coordonnees des 3 sommets en double precision */

/* entrees : */
/* --------- */
/* p1,p2,p3 : les 3 coordonnees des 3 sommets du triangle */
/*            sens direct pour une surface et qualite >0 */
/* sorties : */
/* --------- */
/* qualite: valeur de la qualite du triangle entre 0 et 1 (equilateral) */
/*          1 etant la qualite optimale */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique upmc paris     janvier 1995 */
/* 2345x7..............................................................012 */
/*                  d2uxr3 = 2 * sqrt(3) */

/*     la longueur des 3 cotes */
    /* Parameter adjustments */
    --p3;
    --p2;
    --p1;

    /* Function Body */
/* Computing 2nd power */
    d__1 = p2[1] - p1[1];
/* Computing 2nd power */
    d__2 = p2[2] - p1[2];
    a = sqrt(d__1 * d__1 + d__2 * d__2);
/* Computing 2nd power */
    d__1 = p3[1] - p2[1];
/* Computing 2nd power */
    d__2 = p3[2] - p2[2];
    b = sqrt(d__1 * d__1 + d__2 * d__2);
/* Computing 2nd power */
    d__1 = p1[1] - p3[1];
/* Computing 2nd power */
    d__2 = p1[2] - p3[2];
    c__ = sqrt(d__1 * d__1 + d__2 * d__2);

/*     demi perimetre */
    p = (a + b + c__) * .5;

    if (a * b * c__ != 0.) {
/*        critere : 2 racine(3) * rayon_inscrit / plus longue arete */
/* Computing MAX */
	d__2 = max(a,b);
	*qualite = sqrt((d__1 = (p - a) / p * (p - b) * (p - c__), abs(d__1)))
		 * 3.4641016151377544f / max(d__2,c__);
    } else {
	*qualite = 0.;
    }


/*     autres criteres possibles: */
/*     critere : 2 * rayon_inscrit / rayon_circonscrit */
/*     qualite = 8d0 * (p-a) * (p-b) * (p-c) / (a * b * c) */

/*     critere : 3*sqrt(3.) * ray_inscrit / demi perimetre */
/*     qualite = 3*sqrt(3.) * sqrt ((p-a)*(p-b)*(p-c) / p**3) */

/*     critere : 2*sqrt(3.) * ray_inscrit / max( des aretes ) */
/*     qualite = 2*sqrt(3.) * sqrt( (p-a)*(p-b)*(p-c) / p ) / max(a,b,c) */
    return 0;
} /* qutr2d_ */

doublereal surtd2_(doublereal *p1, doublereal *p2, doublereal *p3)
{
    /* System generated locals */
    doublereal ret_val;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but : calcul de la surface d'un triangle defini par 3 points de R**2 */
/* ----- */
/* parametres d entree : */
/* --------------------- */
/* p1 p2 p3 : les 3 fois 2 coordonnees des sommets du triangle */

/* parametre resultat : */
/* -------------------- */
/* surtd2 : surface du triangle */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique upmc paris     fevrier 1992 */
/* 2345x7..............................................................012 */

/*     la surface du triangle */
    /* Parameter adjustments */
    --p3;
    --p2;
    --p1;

    /* Function Body */
    ret_val = ((p2[1] - p1[1]) * (p3[2] - p1[2]) - (p2[2] - p1[2]) * (p3[1] - 
	    p1[1])) * .5;
    return ret_val;
} /* surtd2_ */

integer nopre3_(integer *i__)
{
    /* System generated locals */
    integer ret_val;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   numero precedent i dans le sens circulaire  1 2 3 1 ... */
/* ----- */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */
    if (*i__ == 1) {
	ret_val = 3;
    } else {
	ret_val = *i__ - 1;
    }
    return ret_val;
} /* nopre3_ */

integer nosui3_(integer *i__)
{
    /* System generated locals */
    integer ret_val;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   numero suivant i dans le sens circulaire  1 2 3 1 ... */
/* ----- */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */
    if (*i__ == 3) {
	ret_val = 1;
    } else {
	ret_val = *i__ + 1;
    }
    return ret_val;
} /* nosui3_ */

/* Subroutine */ int provec_(doublereal *v1, doublereal *v2, doublereal *v3)
{
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    v3 vecteur = produit vectoriel de 2 vecteurs de r ** 3 */
/* ----- */
/* entrees: */
/* -------- */
/* v1, v2 : les 2 vecteurs de 3 composantes */

/* sortie : */
/* -------- */
/* v3     : vecteur = v1  produit vectoriel v2 */
/* c++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : perronnet alain upmc analyse numerique paris        mars 1987 */
/* 2345x7..............................................................012 */

    /* Parameter adjustments */
    --v3;
    --v2;
    --v1;

    /* Function Body */
    v3[1] = v1[2] * v2[3] - v1[3] * v2[2];
    v3[2] = v1[3] * v2[1] - v1[1] * v2[3];
    v3[3] = v1[1] * v2[2] - v1[2] * v2[1];

    return 0;
} /* provec_ */

/* Subroutine */ int norme1_(integer *n, doublereal *v, integer *ierr)
{
    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__;
    static doublereal s;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   normalisation euclidienne a 1 d un vecteur v de n composantes */
/* ----- */
/* entrees : */
/* --------- */
/* n       : nombre de composantes du vecteur */

/* modifie : */
/* --------- */
/* v       : le vecteur a normaliser a 1 */

/* sortie  : */
/* --------- */
/* ierr    : 1 si la norme de v est egale a 0 */
/*           0 si pas d'erreur */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique paris             mars 1987 */
/* ...................................................................... */

    /* Parameter adjustments */
    --v;

    /* Function Body */
    s = 0.;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	s += v[i__] * v[i__];
/* L10: */
    }

/*     test de nullite de la norme du vecteur */
/*     -------------------------------------- */
    if (s <= 0.) {
/*        norme nulle du vecteur non normalisable a 1 */
	*ierr = 1;
	return 0;
    }

    s = 1. / sqrt(s);
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	v[i__] *= s;
/* L20: */
    }

    *ierr = 0;
    return 0;
} /* norme1_ */

/* Subroutine */ int insoar_(integer *mxsomm, integer *mosoar, integer *
	mxsoar, integer *n1soar, integer *nosoar)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, i__1;

    /* Local variables */
    static integer i__;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    initialiser le tableau nosoar pour le hachage des aretes */
/* ----- */

/* entrees: */
/* -------- */
/* mxsomm : plus grand numero de sommet d'une arete au cours du calcul */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          avec mxsoar>=3*mxsomm */

/* sorties: */
/* -------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/*          chainage des aretes vides amont et aval */
/*          l'arete vide qui precede=nosoar(4,i) */
/*          l'arete vide qui suit   =nosoar(5,i) */
/* nosoar : numero des 2 sommets, no ligne, 2 triangles de l'arete, */
/*          chainage momentan'e d'aretes, chainage du hachage des aretes */
/*          hachage des aretes = min( nosoar(1), nosoar(2) ) */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*     initialisation des aretes 1 a mxsomm */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;

    /* Function Body */
    i__1 = *mxsomm;
    for (i__ = 1; i__ <= i__1; ++i__) {

/*        sommet 1 = 0 <=> temoin d'arete vide pour le hachage */
	nosoar[i__ * nosoar_dim1 + 1] = 0;

/*        arete sur aucune ligne */
	nosoar[i__ * nosoar_dim1 + 3] = 0;

/*        la position de l'arete interne ou frontaliere est inconnue */
	nosoar[i__ * nosoar_dim1 + 6] = -2;

/*        fin de chainage du hachage pas d'arete suivante */
	nosoar[*mosoar + i__ * nosoar_dim1] = 0;

/* L10: */
    }

/*     la premiere arete vide chainee est la mxsomm+1 du tableau */
/*     car ces aretes ne sont pas atteignables par le hachage direct */
    *n1soar = *mxsomm + 1;

/*     initialisation des aretes vides et des chainages */
    i__1 = *mxsoar;
    for (i__ = *n1soar; i__ <= i__1; ++i__) {

/*        sommet 1 = 0 <=> temoin d'arete vide pour le hachage */
	nosoar[i__ * nosoar_dim1 + 1] = 0;

/*        arete sur aucune ligne */
	nosoar[i__ * nosoar_dim1 + 3] = 0;

/*        chainage sur l'arete vide qui precede */
/*        (si arete occupee cela deviendra le no du triangle 1 de l'arete) */
	nosoar[i__ * nosoar_dim1 + 4] = i__ - 1;

/*        chainage sur l'arete vide qui suit */
/*        (si arete occupee cela deviendra le no du triangle 2 de l'arete) */
	nosoar[i__ * nosoar_dim1 + 5] = i__ + 1;

/*        chainages des aretes frontalieres ou internes ou ... */
	nosoar[i__ * nosoar_dim1 + 6] = -2;

/*        fin de chainage du hachage */
	nosoar[*mosoar + i__ * nosoar_dim1] = 0;

/* L20: */
    }

/*     la premiere arete vide n'a pas de precedent */
    nosoar[*n1soar * nosoar_dim1 + 4] = 0;

/*     la derniere arete vide est mxsoar sans arete vide suivante */
    nosoar[*mxsoar * nosoar_dim1 + 5] = 0;
    return 0;
} /* insoar_ */

/* Subroutine */ int azeroi_(integer *l, integer *ntab)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i__;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but : initialisation a zero d un tableau ntab de l variables entieres */
/* ----- */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique upmc paris septembre 1988 */
/* 23456---------------------------------------------------------------012 */
    /* Parameter adjustments */
    --ntab;

    /* Function Body */
    i__1 = *l;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ntab[i__] = 0;
/* L1: */
    }
    return 0;
} /* azeroi_ */

/* Subroutine */ int fasoar_(integer *ns1, integer *ns2, integer *nt1, 
	integer *nt2, integer *nolign, integer *mosoar, integer *mxsoar, 
	integer *n1soar, integer *nosoar, integer *noarst, integer *noar, 
	integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, n, nu2sar[2];
    extern /* Subroutine */ int hasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___13 = { 0, 0, 0, 0, 0 };
    static cilist io___14 = { 0, 0, 0, 0, 0 };
    static cilist io___15 = { 0, 0, 0, 0, 0 };
    static cilist io___18 = { 0, 0, 0, 0, 0 };
    static cilist io___19 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former l'arete de sommet ns1-ns2 dans le hachage du tableau */
/* -----    nosoar des aretes de la triangulation */

/* entrees: */
/* -------- */
/* ns1 ns2: numero pxyd des 2 sommets de l'arete */
/* nt1    : numero du triangle auquel appartient l'arete */
/*          nt1=-1 si numero inconnu */
/* nt2    : numero de l'eventuel second triangle de l'arete si connu */
/*          nt2=-1 si numero inconnu */
/* nolign : numero de la ligne de l'arete dans ladefi(wulftr-1+nolign) */
/*          =0 si l'arete n'est une arete de ligne */
/*          ce numero est ajoute seulement si l'arete est creee */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/*          chainage des aretes vides amont et aval */
/*          l'arete vide qui precede=nosoar(4,i) */
/*          l'arete vide qui suit   =nosoar(5,i) */
/* nosoar : numero des 2 sommets, no ligne, 2 triangles de l'arete, */
/*          chainage momentan'e d'aretes, chainage du hachage des aretes */
/*          hachage des aretes = min( nosoar(1), nosoar(2) ) */
/* noarst : noarst(np) numero d'une arete du sommet np */

/* ierr   : si < 0  en entree pas d'affichage en cas d'erreur du type */
/*         "arete appartenant a plus de 2 triangles et a creer!" */
/*          si >=0  en entree       affichage de ce type d'erreur */

/* sorties: */
/* -------- */
/* noar   : >0 numero de l'arete retrouvee ou ajoutee */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si arete a creer et appartenant a 2 triangles distincts */
/*             des triangles nt1 et nt2 */
/*          =3 si arete appartenant a 2 triangles distincts */
/*             differents des triangles nt1 et nt2 */
/*          =4 si arete appartenant a 2 triangles distincts */
/*             dont le second n'est pas le triangle nt2 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    --noarst;

    /* Function Body */
    *ierr = 0;

/*     ajout eventuel de l'arete s1 s2 dans nosoar */
    nu2sar[0] = *ns1;
    nu2sar[1] = *ns2;

/*     hachage de l'arete de sommets nu2sar */
    hasoar_(mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], nu2sar, noar);
/*     en sortie: noar>0 => no arete retrouvee */
/*                    <0 => no arete ajoutee */
/*                    =0 => saturation du tableau nosoar */

    if (*noar == 0) {

/*        saturation du tableau nosoar */
	//io___13.ciunit = unites_1.imprim;
	//s_wsle(&io___13);
	//do_lio(&c__9, &c__1, "fasoar: tableau nosoar sature", (ftnlen)29);
	//e_wsle();
	*ierr = 1;
	return 0;

    } else if (*noar < 0) {

/*        l'arete a ete ajoutee. initialisation des autres informations */
	*noar = -(*noar);
/*        le numero de la ligne de l'arete */
	nosoar[*noar * nosoar_dim1 + 3] = *nolign;
/*        le triangle 1 de l'arete => le triangle nt1 */
	nosoar[*noar * nosoar_dim1 + 4] = *nt1;
/*        le triangle 2 de l'arete => le triangle nt2 */
	nosoar[*noar * nosoar_dim1 + 5] = *nt2;
/*        le chainage est mis a -1 */
	nosoar[*noar * nosoar_dim1 + 6] = -1;

/*        le sommet appartient a l'arete noar */
	noarst[nu2sar[0]] = *noar;
	noarst[nu2sar[1]] = *noar;

    } else {

/*        l'arete a ete retrouvee. */
/*        si elle appartient a 2 triangles differents de nt1 et nt2 */
/*        alors il y a une erreur */
	if (nosoar[*noar * nosoar_dim1 + 4] > 0 && nosoar[*noar * nosoar_dim1 
		+ 5] > 0) {
	    if ((nosoar[*noar * nosoar_dim1 + 4] != *nt1 && nosoar[*noar * 
		    nosoar_dim1 + 4] != *nt2) || (nosoar[*noar * nosoar_dim1 + 
		    5] != *nt1 && nosoar[*noar * nosoar_dim1 + 5] != *nt2)) {
/*                arete appartenant a plus de 2 triangles => erreur */
		if (*ierr >= 0) {
		 //   io___14.ciunit = unites_1.imprim;
		 //   s_wsle(&io___14);
		 //   do_lio(&c__9, &c__1, "erreur fasoar: arete ", (ftnlen)21);
		 //   do_lio(&c__3, &c__1, (char *)&(*noar), (ftnlen)sizeof(
			//    integer));
		 //   do_lio(&c__9, &c__1, " dans 2 triangles", (ftnlen)17);
		 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 
			//    + 4], (ftnlen)sizeof(integer));
		 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 
			//    + 5], (ftnlen)sizeof(integer));
		 //   do_lio(&c__9, &c__1, " et ajouter", (ftnlen)11);
		 //   do_lio(&c__3, &c__1, (char *)&(*nt1), (ftnlen)sizeof(
			//    integer));
		 //   do_lio(&c__3, &c__1, (char *)&(*nt2), (ftnlen)sizeof(
			//    integer));
		 //   e_wsle();
		 //   io___15.ciunit = unites_1.imprim;
		 //   s_wsle(&io___15);
		 //   do_lio(&c__9, &c__1, "arete", (ftnlen)5);
		 //   do_lio(&c__3, &c__1, (char *)&(*noar), (ftnlen)sizeof(
			//    integer));
		 //   i__1 = *mosoar;
		 //   for (i__ = 1; i__ <= i__1; ++i__) {
			//do_lio(&c__3, &c__1, (char *)&nosoar[i__ + *noar * 
			//	nosoar_dim1], (ftnlen)sizeof(integer));
		 //   }
		 //   e_wsle();
		}

/*                ERREUR. CORRECTION POUR VOIR ... */
		nosoar[*noar * nosoar_dim1 + 4] = *nt1;
		nosoar[*noar * nosoar_dim1 + 5] = *nt2;
/* cc                 ierr = 2 */
/* cc                 return */
	    }
	}

/*        mise a jour du numero des triangles de l'arete noar */
/*        le triangle 2 de l'arete => le triangle nt1 */
	if (nosoar[*noar * nosoar_dim1 + 4] <= 0) {
/*            pas de triangle connu pour cette arete */
	    n = 4;
	} else {
/*            deja un triangle connu. ce nouveau est le second */
	    if (nosoar[*noar * nosoar_dim1 + 5] > 0 && *nt1 > 0 && nosoar[*
		    noar * nosoar_dim1 + 5] != *nt1) {
/*               arete appartenant a plus de 2 triangles => erreur */
		//io___18.ciunit = unites_1.imprim;
		//s_wsle(&io___18);
		//do_lio(&c__9, &c__1, "erreur fasoar: arete ", (ftnlen)21);
		//do_lio(&c__3, &c__1, (char *)&(*noar), (ftnlen)sizeof(integer)
		//	);
		//do_lio(&c__9, &c__1, " dans triangles", (ftnlen)15);
		//do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 4],
		//	 (ftnlen)sizeof(integer));
		//do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 5],
		//	 (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " et ajouter triangle", (ftnlen)20);
		//do_lio(&c__3, &c__1, (char *)&(*nt1), (ftnlen)sizeof(integer))
		//	;
		//e_wsle();
		*ierr = 3;
		return 0;
	    }
	    n = 5;
	}
	nosoar[n + *noar * nosoar_dim1] = *nt1;

/*        cas de l'arete frontaliere retrouvee comme diagonale d'un quadrangle */
	if (*nt2 > 0) {
/*           l'arete appartient a 2 triangles */
	    if (nosoar[*noar * nosoar_dim1 + 5] > 0 && nosoar[*noar * 
		    nosoar_dim1 + 5] != *nt2) {
/*               arete appartenant a plus de 2 triangles => erreur */
		//io___19.ciunit = unites_1.imprim;
		//s_wsle(&io___19);
		//do_lio(&c__9, &c__1, "erreur fasoar: arete ", (ftnlen)21);
		//do_lio(&c__3, &c__1, (char *)&(*noar), (ftnlen)sizeof(integer)
		//	);
		//do_lio(&c__9, &c__1, " de st", (ftnlen)6);
		//do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 1],
		//	 (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, "-", (ftnlen)1);
		//do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 2],
		//	 (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " dans plus de 2 triangles", (ftnlen)25);
		//e_wsle();
		*ierr = 4;
		return 0;
	    }
	    nosoar[*noar * nosoar_dim1 + 5] = *nt2;
	}

    }

/*     pas d'erreur */
    *ierr = 0;
    return 0;
} /* fasoar_ */

/* Subroutine */ int fq1inv_(real *x, real *y, real *s, real *xc, real *yc, 
	integer *ierr)
{
    /* System generated locals */
    real r__1, r__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal a, b, c__, d__;
    static integer i__;
    static doublereal t[2], u, v, w, x0, y0, beta;
    static real dist[2];
    static doublereal gamma, alpha, delta;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   calcul des 2 coordonnees (xc,yc) dans le carre (0,1) */
/* -----   image par f:carre unite-->quadrangle appartenant a q1**2 */
/*         par une resolution directe due a Nicolas Thenault */

/* entrees: */
/* -------- */
/* x,y   : coordonnees du point image dans le quadrangle de sommets s */
/* s     : les 2 coordonnees des 4 sommets du quadrangle */

/* sorties: */
/* -------- */
/* xc,yc : coordonnees dans le carre dont l'image par f vaut (x,y) */
/* ierr  : 0 si calcul sans erreur, 1 si quadrangle degenere */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteurs: thenault tulenew  analyse numerique paris        janvier 1998 */
/* modifs : perronnet alain   analyse numerique paris        janvier 1998 */
/* 234567..............................................................012 */

    /* Parameter adjustments */
    s -= 3;

    /* Function Body */
    a = s[3];
    b = s[5] - s[3];
    c__ = s[9] - s[3];
    d__ = s[3] - s[5] + s[7] - s[9];

    alpha = s[4];
    beta = s[6] - s[4];
    gamma = s[10] - s[4];
    delta = s[4] - s[6] + s[8] - s[10];

    u = beta * c__ - b * gamma;
    if (u == 0.) {
/*        quadrangle degenere */
	*ierr = 1;
	return 0;
    }
    v = delta * c__ - d__ * gamma;
    w = b * delta - beta * d__;

    x0 = c__ * (*y - alpha) - gamma * (*x - a);
    y0 = b * (*y - alpha) - beta * (*x - a);

    a = v * w;
    b = u * u - w * x0 - v * y0;
    c__ = x0 * y0;

    if (a != 0.) {

	delta = sqrt(b * b - a * 4 * c__);
	if (b >= 0.f) {
	    t[1] = -b - delta;
	} else {
	    t[1] = -b + delta;
	}
/*        la racine de plus grande valeur absolue */
/*       (elle donne le plus souvent le point exterieur au carre unite */
/*        donc a tester en second pour reduire les calculs) */
	t[1] /= a * 2;
/*        calcul de la seconde racine a partir de la somme => plus stable */
	t[0] = -b / a - t[1];

	for (i__ = 1; i__ <= 2; ++i__) {

/*           la solution i donne t elle un point interne au carre unite? */
	    *xc = (x0 - v * t[i__ - 1]) / u;
	    *yc = (w * t[i__ - 1] - y0) / u;
	    if (0.f <= *xc && *xc <= 1.f) {
		if (0.f <= *yc && *yc <= 1.f) {
		    goto L9000;
		}
	    }

/*           le point (xc,yc) n'est pas dans le carre unite */
/*           cela peut etre du aux erreurs d'arrondi */
/*           => choix par le minimum de la distance aux bords du carre */
/* Computing MAX */
	    r__1 = 0.f, r__2 = -(*xc), r__1 = max(r__1,r__2), r__2 = *xc - 
		    1.f, r__1 = max(r__1,r__2), r__2 = -(*yc), r__1 = max(
		    r__1,r__2), r__2 = *yc - 1.f;
	    dist[i__ - 1] = dmax(r__1,r__2);

/* L10: */
	}

	if (dist[0] > dist[1]) {
/*           f(xc,yc) pour la racine 2 est plus proche de x,y */
/*           xc yc sont deja calcules */
	    goto L9000;
	}

    } else if (b != 0.) {
	t[0] = -c__ / b;
    } else {
	t[0] = 0.;
    }

/*     les 2 coordonnees du point dans le carre unite */
    *xc = (x0 - v * t[0]) / u;
    *yc = (w * t[0] - y0) / u;

L9000:
    *ierr = 0;
    return 0;
} /* fq1inv_ */

/* Subroutine */ int ptdatr_(doublereal *point, doublereal *pxyd, integer *
	nosotr, integer *nsigne)
{
    /* System generated locals */
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal d__;
    static integer i__, n, n1, n2, n3;
    static doublereal x1, x2, x3, y1, y2, y3, dd, xp, yp, cb1, cb2, cb3;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    le point est il dans le triangle de sommets nosotr */
/* ----- */

/* entrees: */
/* -------- */
/* point  : les 2 coordonnees du point */
/* pxyd   : les 2 coordonnees et distance souhaitee des points du maillage */
/* nosotr : le numero des 3 sommets du triangle */

/* sorties: */
/* -------- */
/* nsigne : >0 si le point est dans le triangle ou sur une des 3 aretes */
/*          =0 si le triangle est degenere ou indirect ou ne contient pas le poin */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

    /* Parameter adjustments */
    --nosotr;
    pxyd -= 4;
    --point;

    /* Function Body */
    xp = point[1];
    yp = point[2];

    n1 = nosotr[1];
    x1 = pxyd[n1 * 3 + 1];
    y1 = pxyd[n1 * 3 + 2];

    n2 = nosotr[2];
    x2 = pxyd[n2 * 3 + 1];
    y2 = pxyd[n2 * 3 + 2];

    n3 = nosotr[3];
    x3 = pxyd[n3 * 3 + 1];
    y3 = pxyd[n3 * 3 + 2];

/*     2 fois la surface du triangle = determinant de la matrice */
/*     de calcul des coordonnees barycentriques du point p */
    d__ = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);

    if (d__ > 0.) {

/*        triangle non degenere */
/*        ===================== */
/*        calcul des 3 coordonnees barycentriques du */
/*        point xp yp dans le triangle */
	cb1 = ((x2 - xp) * (y3 - yp) - (x3 - xp) * (y2 - yp)) / d__;
	cb2 = ((x3 - xp) * (y1 - yp) - (x1 - xp) * (y3 - yp)) / d__;
	cb3 = 1. - cb1 - cb2;
/* cc         cb3 = ( ( x1-xp ) * ( y2-yp ) - ( x2-xp ) * ( y1-yp ) ) / d */

/* cc         if( cb1 .ge. -0.00005d0 .and. cb1 .le. 1.00005d0 .and. */
	if (cb1 >= 0. && cb1 <= 1. && cb2 >= 0. && cb2 <= 1. && cb3 >= 0. && 
		cb3 <= 1.) {

/*           le triangle nosotr contient le point */
	    *nsigne = 1;
	} else {
	    *nsigne = 0;
	}

    } else {

/*        triangle degenere */
/*        ================= */
/*        le point est il du meme cote que le sommet oppose de chaque arete? */
	*nsigne = 0;
	for (i__ = 1; i__ <= 3; ++i__) {
/*           le sinus de l'angle p1 p2-p1 point */
	    x1 = pxyd[n1 * 3 + 1];
	    y1 = pxyd[n1 * 3 + 2];
	    d__ = (pxyd[n2 * 3 + 1] - x1) * (point[2] - y1) - (pxyd[n2 * 3 + 
		    2] - y1) * (point[1] - x1);
	    dd = (pxyd[n2 * 3 + 1] - x1) * (pxyd[n3 * 3 + 2] - y1) - (pxyd[n2 
		    * 3 + 2] - y1) * (pxyd[n3 * 3 + 1] - x1);
/* Computing 2nd power */
	    d__1 = pxyd[n2 * 3 + 1] - x1;
/* Computing 2nd power */
	    d__2 = pxyd[n2 * 3 + 2] - y1;
	    cb1 = d__1 * d__1 + d__2 * d__2;
/* Computing 2nd power */
	    d__1 = point[1] - x1;
/* Computing 2nd power */
	    d__2 = point[2] - y1;
	    cb2 = d__1 * d__1 + d__2 * d__2;
/* Computing 2nd power */
	    d__1 = pxyd[n3 * 3 + 1] - x1;
/* Computing 2nd power */
	    d__2 = pxyd[n3 * 3 + 2] - y1;
	    cb3 = d__1 * d__1 + d__2 * d__2;
	    if (abs(dd) <= sqrt(cb1 * cb3) * 1e-4f) {
/*              le point 3 est sur l'arete 1-2 */
/*              le point doit y etre aussi */
		if (abs(d__) <= sqrt(cb1 * cb2) * 1e-4f) {
/*                 point sur l'arete */
		    ++(*nsigne);
		}
	    } else {
/*              le point 3 n'est pas sur l'arete . test des signes */
		if (d__ * dd >= 0.) {
		    ++(*nsigne);
		}
	    }
/*           permutation circulaire des 3 sommets et aretes */
	    n = n1;
	    n1 = n2;
	    n2 = n3;
	    n3 = n;
/* L10: */
	}
	if (*nsigne != 3) {
	    *nsigne = 0;
	}
    }
    return 0;
} /* ptdatr_ */

integer nosstr_(doublereal *p, doublereal *pxyd, integer *nt, integer *letree)
{
    /* System generated locals */
    integer ret_val;

    /* Local variables */
    static doublereal d__, x1, y1, x21, y21, x31, y31, xe, ye;
    static integer ns1, ns2, ns3;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    calculer le numero 0 a 3 du sous-triangle te contenant */
/* -----    le point p */

/* entrees: */
/* -------- */
/* p      : point de r**2 contenu dans le te nt de letree */
/* pxyd   : x y distance des points */
/* nt     : numero letree du te de te voisin a calculer */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*      letree(0,0)  no du 1-er te vide dans letree */
/*      letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*      letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*      letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*      si letree(0,.)>0 alors */
/*         letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*      sinon */
/*         letree(0:3,j) :-no pxyd des 1 …a 4 points internes au triangle j */
/*                         0  si pas de point */
/*                       ( j est alors une feuille de l'arbre ) */
/*      letree(4,j) : no letree du sur-triangle du triangle j */
/*      letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*      letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* sorties : */
/* --------- */
/* nosstr : 0 si le sous-triangle central contient p */
/*          i =1,2,3 numero du sous-triangle contenant p */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */

/*     le numero des 3 sommets du triangle */
    /* Parameter adjustments */
    pxyd -= 4;
    --p;

    /* Function Body */
    ns1 = letree[*nt * 9 + 6];
    ns2 = letree[*nt * 9 + 7];
    ns3 = letree[*nt * 9 + 8];

/*     les coordonnees entre 0 et 1 du point p */
    x1 = pxyd[ns1 * 3 + 1];
    y1 = pxyd[ns1 * 3 + 2];

    x21 = pxyd[ns2 * 3 + 1] - x1;
    y21 = pxyd[ns2 * 3 + 2] - y1;

    x31 = pxyd[ns3 * 3 + 1] - x1;
    y31 = pxyd[ns3 * 3 + 2] - y1;

    d__ = 1.f / (x21 * y31 - x31 * y21);

    xe = ((p[1] - x1) * y31 - (p[2] - y1) * x31) * d__;
    ye = ((p[2] - y1) * x21 - (p[1] - x1) * y21) * d__;

    if (xe > .5) {
/*        sous-triangle droit */
	ret_val = 2;
    } else if (ye > .5) {
/*        sous-triangle haut */
	ret_val = 3;
    } else if (xe + ye < .5) {
/*        sous-triangle gauche */
	ret_val = 1;
    } else {
/*        sous-triangle central */
	ret_val = 0;
    }
    return ret_val;
} /* nosstr_ */

integer notrpt_(doublereal *p, doublereal *pxyd, integer *notrde, integer *
	letree)
{
    /* System generated locals */
    integer ret_val;

    /* Local variables */
    static integer nsot;
    extern integer nosstr_(doublereal *, doublereal *, integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    calculer le numero letree du sous-triangle feuille contenant */
/* -----    le point p a partir du te notrde de letree */

/* entrees: */
/* -------- */
/* p      : point de r**2 contenu dans le te nt de letree */
/* pxyd   : x y distance des points */
/* notrde : numero letree du triangle depart de recherche (1=>racine) */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*      letree(0,0)  no du 1-er te vide dans letree */
/*      letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*      letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*      letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*      si letree(0,.)>0 alors */
/*         letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*      sinon */
/*         letree(0:3,j) :-no pxyd des 1 … 4 points internes au triangle j */
/*                         0  si pas de point */
/*                        ( j est alors une feuille de l'arbre ) */
/*      letree(4,j) : no letree du sur-triangle du triangle j */
/*      letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*      letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* sorties : */
/* --------- */
/* notrpt : numero letree du triangle contenant le point p */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */

/*     la racine depart de la recherche */
    /* Parameter adjustments */
    pxyd -= 4;
    --p;

    /* Function Body */
    ret_val = *notrde;

/*     tant que la feuille n'est pas atteinte descendre l'arbre */
L10:
    if (letree[ret_val * 9] > 0) {

/*        recherche du sous-triangle contenant p */
	nsot = nosstr_(&p[1], &pxyd[4], &ret_val, letree);

/*        le numero letree du sous-triangle */
	ret_val = letree[nsot + ret_val * 9];
	goto L10;

    }
    return ret_val;
} /* notrpt_ */

/* Subroutine */ int teajpt_(integer *ns, integer *nbsomm, integer *mxsomm, 
	doublereal *pxyd, integer *letree, integer *ntrp, integer *ierr)
{
    static integer i__;
    extern /* Subroutine */ int te4ste_(integer *, integer *, doublereal *, 
	    integer *, integer *, integer *);
    extern integer notrpt_(doublereal *, doublereal *, integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    ajout du point ns de pxyd dans letree */
/* ----- */

/* entrees: */
/* -------- */
/* ns     : numero du point a ajouter dans letree */
/* mxsomm : nombre maximal de points declarables dans pxyd */
/* pxyd   : tableau des coordonnees des points */
/*          par point : x  y  distance_souhaitee */

/* modifies : */
/* ---------- */
/* nbsomm : nombre actuel de points dans pxyd */

/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*      letree(0,0) : no du 1-er te vide dans letree */
/*      letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*      letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*      letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*      si letree(0,.)>0 alors */
/*         letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*      sinon */
/*         letree(0:3,j) :-no pxyd des 1 …a 4 points internes au triangle j */
/*                         0  si pas de point */
/*                        ( j est alors une feuille de l'arbre ) */
/*      letree(4,j) : no letree du sur-triangle du triangle j */
/*      letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*      letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* sorties : */
/* --------- */
/* ntrp    : numero letree du triangle te ou a ete ajoute le point */
/* ierr    : 0 si pas d'erreur,  51 saturation letree, 52 saturation pxyd */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */

/*     depart de la racine */
    /* Parameter adjustments */
    pxyd -= 4;

    /* Function Body */
    *ntrp = 1;

/*     recherche du triangle contenant le point pxyd(ns) */
L1:
    *ntrp = notrpt_(&pxyd[*ns * 3 + 1], &pxyd[4], ntrp, letree);

/*     existe t il un point libre */
    for (i__ = 0; i__ <= 3; ++i__) {
	if (letree[i__ + *ntrp * 9] == 0) {
/*           la place i est libre */
	    letree[i__ + *ntrp * 9] = -(*ns);
	    *ierr = 0;
	    return 0;
	}
/* L10: */
    }

/*     pas de place libre => 4 sous-triangles sont crees */
/*                           a partir des 3 milieux des aretes */
    te4ste_(nbsomm, mxsomm, &pxyd[4], ntrp, letree, ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout du point ns */
    goto L1;
} /* teajpt_ */

/* Subroutine */ int n1trva_(integer *nt, integer *lar, integer *letree, 
	integer *notrva, integer *lhpile)
{
    static integer ntr, nty, nsut;
    extern integer nopre3_(integer *), nosui3_(integer *);
    static integer lapile[64];

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    calculer le numero letree du triangle voisin du te nt */
/* -----    par l'arete lar (1 a 3 ) de nt */
/*          attention : notrva n'est pas forcement minimal */

/* entrees: */
/* -------- */
/* nt     : numero letree du te de te voisin a calculer */
/* lar    : numero 1 a 3 de l'arete du triangle nt */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*   letree(0,0)  no du 1-er te vide dans letree */
/*   letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*   letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*   letree(0:8,1) : racine de l'arbre  (triangle sans sur-triangle) */
/*   si letree(0,.)>0 alors */
/*      letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*   sinon */
/*      letree(0:3,j) :-no pxyd des 1 a 4 points internes au triangle j */
/*                      0  si pas de point */
/*                     ( j est alors une feuille de l'arbre ) */
/*   letree(4,j) : no letree du sur-triangle du triangle j */
/*   letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*   letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* sorties : */
/* --------- */
/* notrva  : >0 numero letree du te voisin par l'arete lar */
/*           =0 si pas de te voisin (racine , ... ) */
/* lhpile  : =0 si nt et notrva ont meme taille */
/*           >0 nt est 4**lhpile fois plus petit que notrva */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */

/*     initialisation de la pile */
/*     le triangle est empile */
    lapile[0] = *nt;
    *lhpile = 1;

/*     tant qu'il existe un sur-triangle */
L10:
    ntr = lapile[*lhpile - 1];
    if (ntr == 1) {
/*        racine atteinte => pas de triangle voisin */
	*notrva = 0;
	--(*lhpile);
	return 0;
    }

/*     le type du triangle ntr */
    nty = letree[ntr * 9 + 5];
/*     l'eventuel sur-triangle */
    nsut = letree[ntr * 9 + 4];

    if (nty == 0) {

/*        triangle de type 0 => triangle voisin de type precedent(lar) */
/*                              dans le sur-triangle de ntr */
/*                              ce triangle remplace ntr dans lapile */
	lapile[*lhpile - 1] = letree[nopre3_(lar) + nsut * 9];
	goto L20;
    }

/*     triangle ntr de type nty>0 */
    if (nosui3_(&nty) == *lar) {

/*        le triangle voisin par lar est le triangle 0 */
	lapile[*lhpile - 1] = letree[nsut * 9];
	goto L20;
    }

/*     triangle sans voisin direct => passage par le sur-triangle */
    if (nsut == 0) {

/*        ntr est la racine => pas de triangle voisin par cette arete */
	*notrva = 0;
	return 0;
    } else {

/*        le sur-triangle est empile */
	++(*lhpile);
	lapile[*lhpile - 1] = nsut;
	goto L10;
    }

/*     descente aux sous-triangles selon la meme arete */
L20:
    *notrva = lapile[*lhpile - 1];

L30:
    --(*lhpile);
    if (letree[*notrva * 9] <= 0) {
/*        le triangle est une feuille de l'arbre 0 sous-triangle */
/*        lhpile = nombre de differences de niveaux dans l'arbre */
	return 0;
    } else {
/*        le triangle a 4 sous-triangles */
	if (*lhpile > 0) {

/*           bas de pile non atteint */
	    nty = letree[lapile[*lhpile - 1] * 9 + 5];
	    if (nty == *lar) {
/*              l'oppose est suivant(nty) de notrva */
		*notrva = letree[nosui3_(&nty) + *notrva * 9];
	    } else {
/*              l'oppose est precedent(nty) de notrva */
		*notrva = letree[nopre3_(&nty) + *notrva * 9];
	    }
	    goto L30;
	}
    }

/*     meme niveau dans l'arbre lhpile = 0 */
    return 0;
} /* n1trva_ */

/* Subroutine */ int cenced_(doublereal *xy1, doublereal *xy2, doublereal *
	xy3, doublereal *cetria, integer *ierr)
{
    /* Format strings */
    static char fmt_10000[] = "(3(\002 x=\002,g24.16,\002 y=\002,g24.16/)"
	    ",\002 aire*2=\002,g24.16)";

    /* System generated locals */
    doublereal d__1, d__2;

    /* Builtin functions */
 /*   integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void), s_wsfe(cilist *), do_fio(integer *, char *, ftnlen),
	     e_wsfe(void);*/

    /* Local variables */
    static doublereal x1, y1, x21, y21, x31, y31, xc, yc, rot, aire2;

    /* Fortran I/O blocks */
    static cilist io___79 = { 0, 0, 0, 0, 0 };
    static cilist io___80 = { 0, 0, 0, fmt_10000, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but : calcul des coordonnees du centre du cercle circonscrit */
/* ----- du triangle defini par ses 3 sommets de coordonnees */
/*       xy1 xy2 xy3 ainsi que le carre du rayon de ce cercle */

/* entrees : */
/* --------- */
/* xy1 xy2 xy3 : les 2 coordonnees des 3 sommets du triangle */
/* ierr   : <0  => pas d'affichage si triangle degenere */
/*          >=0 =>       affichage si triangle degenere */

/* sortie : */
/* -------- */
/* cetria : cetria(1)=abcisse  du centre */
/*          cetria(2)=ordonnee du centre */
/*          cetria(3)=carre du rayon   1d28 si triangle degenere */
/* ierr   : 0 si triangle non degenere */
/*          1 si triangle degenere */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : perronnet alain upmc analyse numerique paris        juin 1995 */
/* 2345x7..............................................................012 */

/*     le calcul de 2 fois l'aire du triangle */
/*     attention l'ordre des 3 sommets est direct ou non */
    /* Parameter adjustments */
    --cetria;
    --xy3;
    --xy2;
    --xy1;

    /* Function Body */
    x1 = xy1[1];
    x21 = xy2[1] - x1;
    x31 = xy3[1] - x1;

    y1 = xy1[2];
    y21 = xy2[2] - y1;
    y31 = xy3[2] - y1;

    aire2 = x21 * y31 - x31 * y21;

/*     recherche d'un test relatif peu couteux */
/*     pour reperer la degenerescence du triangle */
    if (abs(aire2) <= (abs(x21) + abs(x31)) * 1e-7f * (abs(y21) + abs(y31))) {
/*        triangle de qualite trop faible */
	if (*ierr >= 0) {
/*            nblgrc(nrerr) = 1 */
/*            kerr(1) = 'erreur cenced: triangle degenere' */
/*            call lereur */
	    //io___79.ciunit = unites_1.imprim;
	    //s_wsle(&io___79);
	    //do_lio(&c__9, &c__1, "erreur cenced: triangle degenere", (ftnlen)
		//    32);
	    //e_wsle();
	    //io___80.ciunit = unites_1.imprim;
	    //s_wsfe(&io___80);
	    //do_fio(&c__2, (char *)&xy1[1], (ftnlen)sizeof(doublereal));
	    //do_fio(&c__2, (char *)&xy2[1], (ftnlen)sizeof(doublereal));
	    //do_fio(&c__2, (char *)&xy3[1], (ftnlen)sizeof(doublereal));
	    //do_fio(&c__1, (char *)&aire2, (ftnlen)sizeof(doublereal));
	    //e_wsfe();
	}
	cetria[1] = 0.;
	cetria[2] = 0.;
	cetria[3] = 1e28;
	*ierr = 1;
	return 0;
    }

/*     les 2 coordonnees du centre intersection des 2 mediatrices */
/*     x = (x1+x2)/2 + lambda * (y2-y1) */
/*     y = (y1+y2)/2 - lambda * (x2-x1) */
/*     x = (x1+x3)/2 + rot    * (y3-y1) */
/*     y = (y1+y3)/2 - rot    * (x3-x1) */
/*     ========================================================== */
    rot = ((xy2[1] - xy3[1]) * x21 + (xy2[2] - xy3[2]) * y21) / (aire2 * 2);

    xc = (x1 + xy3[1]) * .5 + rot * y31;
    yc = (y1 + xy3[2]) * .5 - rot * x31;

    cetria[1] = xc;
    cetria[2] = yc;

/*     le carre du rayon */
/* Computing 2nd power */
    d__1 = x1 - xc;
/* Computing 2nd power */
    d__2 = y1 - yc;
    cetria[3] = d__1 * d__1 + d__2 * d__2;

/*     pas d'erreur rencontree */
    *ierr = 0;
    return 0;
} /* cenced_ */

doublereal angled_(doublereal *p1, doublereal *p2, doublereal *p3)
{
    /* System generated locals */
    doublereal ret_val;

    /* Builtin functions */
    double sqrt(doublereal), atan(doublereal), acos(doublereal);

    /* Local variables */
    static doublereal c__, d__, a1, a2, x21, y21, x31, y31;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   calculer l'angle (p1p2,p1p3) en radians */
/* ----- */

/* entrees : */
/* --------- */
/* p1,p2,p3 : les 2 coordonnees des 3 sommets de l'angle */
/*               sens direct pour une surface >0 */
/* sorties : */
/* --------- */
/* angled :  angle (p1p2,p1p3) en radians entre [0 et 2pi] */
/*           0 si p1=p2 ou p1=p3 */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique upmc paris     fevrier 1992 */
/* 2345x7..............................................................012 */

/*     les cotes */
    /* Parameter adjustments */
    --p3;
    --p2;
    --p1;

    /* Function Body */
    x21 = p2[1] - p1[1];
    y21 = p2[2] - p1[2];
    x31 = p3[1] - p1[1];
    y31 = p3[2] - p1[2];

/*     longueur des cotes */
    a1 = x21 * x21 + y21 * y21;
    a2 = x31 * x31 + y31 * y31;
    d__ = sqrt(a1 * a2);
    if (d__ == 0.) {
	ret_val = 0.;
	return ret_val;
    }

/*     cosinus de l'angle */
    c__ = (x21 * x31 + y21 * y31) / d__;
    if (c__ <= -1.) {
/*        tilt sur apollo si acos( -1 -eps ) */
	ret_val = atan(1.) * 4.;
	return ret_val;
    } else if (c__ >= 1.) {
/*        tilt sur apollo si acos( 1 + eps ) */
	ret_val = 0.;
	return ret_val;
    }

    ret_val = acos(c__);
    if (x21 * y31 - x31 * y21 < 0.) {
/*        demi plan inferieur */
	ret_val = atan(1.) * 8. - ret_val;
    }
    return ret_val;
} /* angled_ */

/* Subroutine */ int teajte_(integer *mxsomm, integer *nbsomm, doublereal *
	pxyd, doublereal *comxmi, doublereal *aretmx, integer *mxtree, 
	integer *letree, integer *ierr)
{
    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static doublereal a[2];
    static integer i__, k;
    static doublereal s;
    static integer nt;
    static doublereal rac3;
    static real arete;
    extern /* Subroutine */ int teajpt_(integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *);
    static integer nbsofr, imprim;

    /* Fortran I/O blocks */
    static cilist io___98 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    initialisation des tableaux letree */
/* -----    ajout des sommets 1 a nbsomm (valeur en entree) dans letree */

/* entrees: */
/* -------- */
/* mxsomm : nombre maximal de sommets permis pour la triangulation */
/* mxtree : nombre maximal de triangles equilateraux (te) declarables */
/* aretmx : longueur maximale des aretes des triangles equilateraux */

/* entrees et sorties : */
/* -------------------- */
/* nbsomm : nombre de sommets apres identification */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/*          tableau reel(3,mxsomm) */

/* sorties: */
/* -------- */
/* comxmi : coordonnees minimales et maximales des points frontaliers */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          letree(0,0) : no du 1-er te vide dans letree */
/*          letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*          letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*          letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*          si letree(0,.)>0 alors */
/*             letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3,j) :-no pxyd des 1 a 4 points internes au triangle j */
/*                             0  si pas de point */
/*                             ( j est alors une feuille de l'arbre ) */
/*          letree(4,j) : no letree du sur-triangle du triangle j */
/*          letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* ierr   :  0 si pas d'erreur */
/*          51 saturation letree */
/*          52 saturation pxyd */
/*           7 tous les points sont alignes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    juillet 1994 */
/* ....................................................................012 */

/*     protection du nombre de sommets avant d'ajouter ceux de tetree */
    /* Parameter adjustments */
    pxyd -= 4;
    comxmi -= 4;

    /* Function Body */
    *ierr = 0;
    nbsofr = *nbsomm;
    i__1 = *nbsomm;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing MIN */
	d__1 = comxmi[4], d__2 = pxyd[i__ * 3 + 1];
	comxmi[4] = min(d__1,d__2);
/* Computing MAX */
	d__1 = comxmi[7], d__2 = pxyd[i__ * 3 + 1];
	comxmi[7] = max(d__1,d__2);
/* Computing MIN */
	d__1 = comxmi[5], d__2 = pxyd[i__ * 3 + 2];
	comxmi[5] = min(d__1,d__2);
/* Computing MAX */
	d__1 = comxmi[8], d__2 = pxyd[i__ * 3 + 2];
	comxmi[8] = max(d__1,d__2);
/* L1: */
    }

/*     creation de l'arbre letree */
/*     ========================== */
/*     la premiere colonne vide de letree */
    letree[0] = 2;
/*     chainage des te vides */
    i__1 = *mxtree;
    for (i__ = 2; i__ <= i__1; ++i__) {
	letree[i__ * 9] = i__ + 1;
/* L4: */
    }
    letree[*mxtree * 9] = 0;
/*     les maxima des 2 indices de letree */
    letree[1] = 8;
    letree[2] = *mxtree;

/*     la racine */
/*     aucun point interne au triangle equilateral (te) 1 */
    letree[9] = 0;
    letree[10] = 0;
    letree[11] = 0;
    letree[12] = 0;
/*     pas de sur-triangle */
    letree[13] = 0;
    letree[14] = 0;
/*     le numero pxyd des 3 sommets du te 1 */
    letree[15] = *nbsomm + 1;
    letree[16] = *nbsomm + 2;
    letree[17] = *nbsomm + 3;

/*     calcul de la largeur et hauteur du rectangle englobant */
/*     ====================================================== */
    a[0] = comxmi[7] - comxmi[4];
    a[1] = comxmi[8] - comxmi[5];
/*     la longueur de la diagonale */
/* Computing 2nd power */
    d__1 = a[0];
/* Computing 2nd power */
    d__2 = a[1];
    s = sqrt(d__1 * d__1 + d__2 * d__2);
    for (k = 1; k <= 2; ++k) {
	if (a[k - 1] < s * 1e-4f) {
/*            nblgrc(nrerr) = 1 */
	    //io___98.ciunit = imprim;
	    //s_wsle(&io___98);
	    //do_lio(&c__9, &c__1, "tous les points sont alignes", (ftnlen)28);
	    //e_wsle();
/*            call lereur */
	    *ierr = 7;
	    return 0;
	}
/* L60: */
    }

/*     le maximum des ecarts */
    s += s;

/*     le triangle equilateral englobant */
/*     ================================= */
/*     ecart du rectangle au triangle equilateral */
    rac3 = sqrt(3.);
    arete = a[0] + *aretmx * 2 + (a[1] + *aretmx) * 2 / rac3;

/*     le point nbsomm + 1 en bas a gauche */
    ++(*nbsomm);
    pxyd[*nbsomm * 3 + 1] = (comxmi[4] + comxmi[7]) * .5 - arete * .5;
    pxyd[*nbsomm * 3 + 2] = comxmi[5] - *aretmx;
    pxyd[*nbsomm * 3 + 3] = s;

/*     le point nbsomm + 2 en bas a droite */
    ++(*nbsomm);
    pxyd[*nbsomm * 3 + 1] = pxyd[(*nbsomm - 1) * 3 + 1] + arete;
    pxyd[*nbsomm * 3 + 2] = pxyd[(*nbsomm - 1) * 3 + 2];
    pxyd[*nbsomm * 3 + 3] = s;

/*     le point nbsomm + 3 sommet au dessus */
    ++(*nbsomm);
    pxyd[*nbsomm * 3 + 1] = pxyd[(*nbsomm - 2) * 3 + 1] + arete * .5;
    pxyd[*nbsomm * 3 + 2] = pxyd[(*nbsomm - 2) * 3 + 2] + arete * .5 * rac3;
    pxyd[*nbsomm * 3 + 3] = s;

/*     ajout des sommets des lignes pour former letree */
/*     =============================================== */
    i__1 = nbsofr;
    for (i__ = 1; i__ <= i__1; ++i__) {
/*        ajout du point i de pxyd a letree */
	teajpt_(&i__, nbsomm, mxsomm, &pxyd[4], letree, &nt, ierr);
	if (*ierr != 0) {
	    return 0;
	}
/* L150: */
    }

    return 0;
} /* teajte_ */

/* Subroutine */ int tetaid_(integer *nutysu, doublereal *dx, doublereal *dy, 
	doublereal *longai, integer *ierr)
{
    /* Format strings */
    static char fmt_10000[] = "(\002attention: longueur de areteideale(\002,"
	    "g14.6,\002,\002,g14.6,\002,\002,g14.6,\002)<=0! => rendue >0\002)"
	    ;
    static char fmt_10001[] = "(\002erreur: longueur de areteideale(\002,g14"
	    ".6,\002,\002,g14.6,\002,\002,g14.6,\002)=0!\002)";

    /* Builtin functions */
    //integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static doublereal d0;
    extern /* Subroutine */ int areteideale_(doublereal *);
    static doublereal xyz[3], xyzd[3];

    /* Fortran I/O blocks */
    static cilist io___105 = { 0, 0, 0, fmt_10000, 0 };
    static cilist io___106 = { 0, 0, 0, fmt_10001, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :     calculer la longueur de l'arete ideale longai en dx,dy */
/* ----- */
/* entrees: */
/* -------- */
/* nutysu : numero de traitement de areteideale() selon le type de surface */
/*          0 pas d'emploi de la fonction areteideale() => aretmx active */
/*          1 il existe une fonction areteideale(xyz,xyzdir) */
/*          ... autres options a definir ... */
/* dx, dy : abscisse et ordonnee dans le plan du point (reel2!) */

/* sorties: */
/* -------- */
/* longai : longueur de l'areteideale(xyz,xyzdir) autour du point xyz */
/* ierr   : 0 si pas d'erreur, <>0 sinon */
/*          1 calcul incorrect de areteideale(xyz,xyzdir) */
/*          2 longueur calculee nulle */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*      double precision  areteideale */

    *ierr = 0;
    if (*nutysu > 0) {
	d0 = *longai;
/*        le point ou se calcule la longueur */
	xyz[0] = *dx;
	xyz[1] = *dy;
/*        z pour le calcul de la longueur (inactif ici!) */
	xyz[2] = 0.;
/*        la direction pour le calcul de la longueur (inactif ici!) */
	xyzd[0] = 0.;
	xyzd[1] = 0.;
	xyzd[2] = 0.;
	areteideale_(longai);
/*         (xyz,xyzd) */
	if (*longai < 0.) {
	    //io___105.ciunit = unites_1.imprim;
	    //s_wsfe(&io___105);
	    //do_fio(&c__3, (char *)&xyz[0], (ftnlen)sizeof(doublereal));
	    //e_wsfe();
	    *longai = -(*longai);
	}
	if (*longai == 0.) {
	    //io___106.ciunit = unites_1.imprim;
	    //s_wsfe(&io___106);
	    //do_fio(&c__3, (char *)&xyz[0], (ftnlen)sizeof(doublereal));
	    //e_wsfe();
	    *ierr = 2;
	    *longai = d0;
	}
    }
    return 0;
} /* tetaid_ */

/* Subroutine */ int tehote_(integer *nutysu, integer *nbarpi, integer *
	mxsomm, integer *nbsomm, doublereal *pxyd, doublereal *comxmi, 
	doublereal *aretmx, integer *letree, integer *mxqueu, integer *laqueu,
	 integer *ierr)
{
    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2, d__3, d__4, d__5, d__6, d__7, d__8;
    static integer equiv_2[3];

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static doublereal a;
    static integer i__, j;
    static doublereal s, d2;
#define ns1 (equiv_2)
#define ns2 (equiv_2 + 1)
#define ns3 (equiv_2 + 2)
    static integer nte, nbs0;
    static doublereal dmin__, dmax__, xrmin, yrmin, xrmax, yrmax;
#define nuste (equiv_2)
    static doublereal aretm2;
    static integer nbsom0;
    extern /* Subroutine */ int te4ste_(integer *, integer *, doublereal *, 
	    integer *, integer *, integer *), n1trva_(integer *, integer *, 
	    integer *, integer *, integer *);
    extern integer nosui3_(integer *);
    extern /* Subroutine */ int tetaid_(integer *, doublereal *, doublereal *,
	     doublereal *, integer *);
    static integer nbiter, niveau, noteva, lequeu, lhqueu;
    extern integer notrpt_(doublereal *, doublereal *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___126 = { 0, 0, 0, 0, 0 };
    static cilist io___132 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :     homogeneisation de l'arbre des te a un saut de taille au plus */
/* -----     prise en compte des distances souhaitees autour des sommets initiaux */

/* entrees: */
/* -------- */
/* nutysu : numero de traitement de areteideale() selon le type de surface */
/*          0 pas d'emploi de la fonction areteideale() => aretmx active */
/*          1 il existe une fonction areteideale() */
/*            dont seules les 2 premieres composantes de uv sont actives */
/*          autres options a definir... */
/* nbarpi : nombre de sommets de la frontiere + nombre de points internes */
/*          imposes par l'utilisateur */
/* mxsomm : nombre maximal de sommets permis pour la triangulation  et te */
/* mxqueu : nombre d'entiers utilisables dans laqueu */
/* comxmi : minimum et maximum des coordonnees de l'objet */
/* aretmx : longueur maximale des aretes des triangles equilateraux */
/* permtr : perimetre de la ligne enveloppe dans le plan */
/*          avant mise a l'echelle a 2**20 */

/* modifies: */
/* --------- */
/* nbsomm : nombre de sommets apres identification */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          letree(0,0) : no du 1-er te vide dans letree */
/*          letree(1,0) : maximum du 1-er indice de letree (ici 8) */
/*          letree(2,0) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*          letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*          si letree(0,.)>0 alors */
/*             letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3,j) :-no pxyd des 1 a 4 points internes au triangle j */
/*                             0  si pas de point */
/*                             ( j est alors une feuille de l'arbre ) */
/*          letree(4,j) : no letree du sur-triangle du triangle j */
/*          letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* auxiliaire : */
/* ------------ */
/* laqueu : mxqueu entiers servant de queue pour le parcours de letree */

/* sorties: */
/* -------- */
/* ierr   :  0 si pas d'erreur */
/*          51 si saturation letree dans te4ste */
/*          52 si saturation pxyd   dans te4ste */
/*          >0 si autre erreur */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc      avril 1997 */
/* 2345x7..............................................................012 */


/*                       lequeu : entree dans la queue */
/*                       lhqueu : longueur de la queue */
/*                       gestion circulaire */


    /* Parameter adjustments */
    pxyd -= 4;
    comxmi -= 4;
    --laqueu;

    /* Function Body */
    *ierr = 0;

/*     existence ou non de la fonction 'taille_ideale' des aretes */
/*     autour du point.  ici la carte est supposee isotrope */
/*     ========================================================== */
/*     attention: si la fonction taille_ideale existe */
/*                alors pxyd(3,*) est la taille_ideale dans l'espace initial */
/*                sinon pxyd(3,*) est la distance calculee dans le plan par */
/*                propagation a partir des tailles des aretes de la frontiere */

    if (*nutysu > 0) {

/*        la fonction taille_ideale(x,y,z) existe */
/*        --------------------------------------- */
/*        initialisation de la distance souhaitee autour des points 1 a nbsomm */
	i__1 = *nbsomm;
	for (i__ = 1; i__ <= i__1; ++i__) {
/*           calcul de pxyzd(3,i) */
	    tetaid_(nutysu, &pxyd[i__ * 3 + 1], &pxyd[i__ * 3 + 2], &pxyd[i__ 
		    * 3 + 3], ierr);
	    if (*ierr != 0) {
		goto L9999;
	    }
/* L1: */
	}

    } else {

/*        la fonction taille_ideale(x,y,z) n'existe pas */
/*        --------------------------------------------- */
/*        prise en compte des distances souhaitees dans le plan */
/*        autour des points frontaliers et des points internes imposes */
/*        toutes les autres distances souhaitees ont ete mis a aretmx */
/*        lors de l'execution du sp teqini */
	i__1 = *nbarpi;
	for (i__ = 1; i__ <= i__1; ++i__) {
/*           le sommet i n'est pas un sommet de letree => sommet frontalier */
/*           recherche du sous-triangle minimal feuille contenant le point i */
	    nte = 1;
L2:
	    nte = notrpt_(&pxyd[i__ * 3 + 1], &pxyd[4], &nte, letree);
/*           la distance au sommet le plus eloigne est elle inferieure */
/*           a la distance souhaitee? */
	    *ns1 = letree[nte * 9 + 6];
	    *ns2 = letree[nte * 9 + 7];
	    *ns3 = letree[nte * 9 + 8];
/* Computing MAX */
/* Computing 2nd power */
	    d__3 = pxyd[i__ * 3 + 1] - pxyd[*ns1 * 3 + 1];
/* Computing 2nd power */
	    d__4 = pxyd[i__ * 3 + 2] - pxyd[*ns1 * 3 + 2];
/* Computing 2nd power */
	    d__5 = pxyd[i__ * 3 + 1] - pxyd[*ns2 * 3 + 1];
/* Computing 2nd power */
	    d__6 = pxyd[i__ * 3 + 2] - pxyd[*ns2 * 3 + 2];
/* Computing 2nd power */
	    d__7 = pxyd[i__ * 3 + 1] - pxyd[*ns3 * 3 + 1];
/* Computing 2nd power */
	    d__8 = pxyd[i__ * 3 + 2] - pxyd[*ns3 * 3 + 2];
	    d__1 = d__3 * d__3 + d__4 * d__4, d__2 = d__5 * d__5 + d__6 * 
		    d__6, d__1 = max(d__1,d__2), d__2 = d__7 * d__7 + d__8 * 
		    d__8;
	    d2 = max(d__1,d__2);
/* Computing 2nd power */
	    d__1 = pxyd[i__ * 3 + 3];
	    if (d2 > d__1 * d__1) {
/*              le triangle nte trop grand doit etre subdivise en 4 sous-triangle */
		te4ste_(nbsomm, mxsomm, &pxyd[4], &nte, letree, ierr);
		if (*ierr != 0) {
		    return 0;
		}
		goto L2;
	    }
/* L3: */
	}
    }

/*     le sous-triangle central de la racine est decoupe systematiquement */
/*     ================================================================== */
    nte = 2;
    if (letree[18] <= 0) {
/*        le sous-triangle central de la racine n'est pas subdivise */
/*        il est donc decoupe en 4 soustriangles */
	nbsom0 = *nbsomm;
	te4ste_(nbsomm, mxsomm, &pxyd[4], &nte, letree, ierr);
	if (*ierr != 0) {
	    return 0;
	}
	i__1 = *nbsomm;
	for (i__ = nbsom0 + 1; i__ <= i__1; ++i__) {
/*           mise a jour de taille_ideale des nouveaux sommets de te */
	    tetaid_(nutysu, &pxyd[i__ * 3 + 1], &pxyd[i__ * 3 + 2], &pxyd[i__ 
		    * 3 + 3], ierr);
	    if (*ierr != 0) {
		goto L9999;
	    }
/* L4: */
	}
    }

/*     le carre de la longueur de l'arete de triangles equilateraux */
/*     souhaitee pour le fond de la triangulation */
/* Computing 2nd power */
    d__1 = *aretmx * 1.34;
    aretm2 = d__1 * d__1;

/*     tout te contenu dans le rectangle englobant doit avoir un */
/*     cote < aretmx et etre de meme taille que les te voisins */
/*     s'il contient un point; sinon un seul saut de taille est permis */
/*     =============================================================== */
/*     le rectangle englobant pour selectionner les te "internes" */
/*     le numero des 3 sommets du te englobant racine de l'arbre des te */
    *ns1 = letree[15];
    *ns2 = letree[16];
    *ns3 = letree[17];
    a = *aretmx * .01;
/*     abscisse du milieu de l'arete gauche du te 1 */
    s = (pxyd[*ns1 * 3 + 1] + pxyd[*ns3 * 3 + 1]) / 2;
/* Computing MIN */
    d__1 = s, d__2 = comxmi[4] - *aretmx;
    xrmin = min(d__1,d__2) - a;
/*     abscisse du milieu de l'arete droite du te 1 */
    s = (pxyd[*ns2 * 3 + 1] + pxyd[*ns3 * 3 + 1]) / 2;
/* Computing MAX */
    d__1 = s, d__2 = comxmi[7] + *aretmx;
    xrmax = max(d__1,d__2) + a;
    yrmin = comxmi[5] - *aretmx;
/*     ordonnee de la droite passant par les milieus des 2 aretes */
/*     droite gauche du te 1 */
    s = (pxyd[*ns1 * 3 + 2] + pxyd[*ns3 * 3 + 2]) / 2;
/* Computing MAX */
    d__1 = s, d__2 = comxmi[8] + *aretmx;
    yrmax = max(d__1,d__2) + a;

/*     cas particulier de 3 ou 4 ou peu d'aretes frontalieres */
    if (*nbarpi <= 8) {
/*        tout le triangle englobant (racine) est a prendre en compte */
	xrmin = pxyd[*ns1 * 3 + 1] - a;
	xrmax = pxyd[*ns2 * 3 + 1] + a;
	yrmin = pxyd[*ns1 * 3 + 2] - a;
	yrmax = pxyd[*ns3 * 3 + 2] + a;
    }

    nbs0 = *nbsomm;
    nbiter = -1;

/*     initialisation de la queue */
L5:
    ++nbiter;
    lequeu = 1;
    lhqueu = 0;
/*     la racine de letree initialise la queue */
    laqueu[1] = 1;

/*     tant que la longueur de la queue est >=0 traiter le debut de queue */
L10:
    if (lhqueu >= 0) {

/*        le triangle te a traiter */
	i__ = lequeu - lhqueu;
	if (i__ <= 0) {
	    i__ = *mxqueu + i__;
	}
	nte = laqueu[i__];
/*        la longueur de la queue est reduite */
	--lhqueu;

/*        nte est il un sous-triangle feuille minimal ? */
L15:
	if (letree[nte * 9] > 0) {

/*           non les 4 sous-triangles sont mis dans la queue */
	    if (lhqueu + 4 >= *mxqueu) {
		//io___126.ciunit = unites_2.imprim;
		//s_wsle(&io___126);
		//do_lio(&c__9, &c__1, "tehote: saturation de la queue", (
		//	ftnlen)30);
		//e_wsle();
		*ierr = 7;
		return 0;
	    }
	    for (i__ = 3; i__ >= 0; --i__) {
/*              ajout du sous-triangle i */
		++lhqueu;
		++lequeu;
		if (lequeu > *mxqueu) {
		    lequeu -= *mxqueu;
		}
		laqueu[lequeu] = letree[i__ + nte * 9];
/* L20: */
	    }
	    goto L10;

	}

/*        ici nte est un triangle minimal non subdivise */
/*        --------------------------------------------- */
/*        le te est il dans le cadre englobant de l'objet ? */
	*ns1 = letree[nte * 9 + 6];
	*ns2 = letree[nte * 9 + 7];
	*ns3 = letree[nte * 9 + 8];
	if (pxyd[*ns1 * 3 + 1] > pxyd[*ns2 * 3 + 1]) {
	    dmin__ = pxyd[*ns2 * 3 + 1];
	    dmax__ = pxyd[*ns1 * 3 + 1];
	} else {
	    dmin__ = pxyd[*ns1 * 3 + 1];
	    dmax__ = pxyd[*ns2 * 3 + 1];
	}
	if ((xrmin <= dmin__ && dmin__ <= xrmax) || (xrmin <= dmax__ && dmax__ <=
		 xrmax)) {
	    if (pxyd[*ns1 * 3 + 2] > pxyd[*ns3 * 3 + 2]) {
		dmin__ = pxyd[*ns3 * 3 + 2];
		dmax__ = pxyd[*ns1 * 3 + 2];
	    } else {
		dmin__ = pxyd[*ns1 * 3 + 2];
		dmax__ = pxyd[*ns3 * 3 + 2];
	    }
	    if ((yrmin <= dmin__ && dmin__ <= yrmax) || (yrmin <= dmax__ && 
		    dmax__ <= yrmax)) {

/*              nte est un te feuille et interne au rectangle englobant */
/*              ======================================================= */
/*              le carre de la longueur de l'arete du te de numero nte */
/* Computing 2nd power */
		d__1 = pxyd[*ns1 * 3 + 1] - pxyd[*ns2 * 3 + 1];
/* Computing 2nd power */
		d__2 = pxyd[*ns1 * 3 + 2] - pxyd[*ns2 * 3 + 2];
		d2 = d__1 * d__1 + d__2 * d__2;

		if (*nutysu == 0) {

/*                 il n'existe pas de fonction 'taille_ideale' */
/*                 ------------------------------------------- */
/*                 si la taille effective de l'arete du te est superieure a aretmx */
/*                 alors le te est decoupe */
		    if (d2 > aretm2) {
/*                    le triangle nte trop grand doit etre subdivise */
/*                    en 4 sous-triangles */
			te4ste_(nbsomm, mxsomm, &pxyd[4], &nte, letree, ierr);
			if (*ierr != 0) {
			    return 0;
			}
			goto L15;
		    }

		} else {

/*                 il existe ici une fonction 'taille_ideale' */
/*                 ------------------------------------------ */
/*                 si la taille effective de l'arete du te est superieure au mini */
/*                 des 3 tailles_ideales aux sommets  alors le te est decoupe */
		    for (i__ = 1; i__ <= 3; ++i__) {
/* Computing 2nd power */
			d__1 = pxyd[nuste[i__ - 1] * 3 + 3] * 1.34;
			if (d2 > d__1 * d__1) {
/*                       le triangle nte trop grand doit etre subdivise */
/*                       en 4 sous-triangles */
			    nbsom0 = *nbsomm;
			    te4ste_(nbsomm, mxsomm, &pxyd[4], &nte, letree, 
				    ierr);
			    if (*ierr != 0) {
				return 0;
			    }
			    i__1 = *nbsomm;
			    for (j = nbsom0 + 1; j <= i__1; ++j) {
/*                          mise a jour de taille_ideale des nouveaux sommets de */
				tetaid_(nutysu, &pxyd[j * 3 + 1], &pxyd[j * 3 
					+ 2], &pxyd[j * 3 + 3], ierr);
				if (*ierr != 0) {
				    goto L9999;
				}
/* L27: */
			    }
			    goto L15;
			}
/* L28: */
		    }
		}

/*              recherche du nombre de niveaux entre nte et les te voisins par se */
/*              si la difference de subdivisions excede 1 alors le plus grand des */
/*              ================================================================= */
/* L29: */
		for (i__ = 1; i__ <= 3; ++i__) {

/*                 noteva triangle voisin de nte par l'arete i */
		    n1trva_(&nte, &i__, letree, &noteva, &niveau);
		    if (noteva <= 0) {
			goto L30;
		    }
/*                 il existe un te voisin */
		    if (niveau > 0) {
			goto L30;
		    }
/*                 nte a un te voisin plus petit ou egal */
		    if (letree[noteva * 9] <= 0) {
			goto L30;
		    }
/*                 nte a un te voisin noteva subdivise au moins une fois */

		    if (nbiter > 0) {
/*                    les 2 sous triangles voisins sont-ils subdivises? */
			*ns2 = letree[i__ + noteva * 9];
			if (letree[*ns2 * 9] <= 0) {
/*                       ns2 n'est pas subdivise */
			    *ns2 = letree[nosui3_(&i__) + noteva * 9];
			    if (letree[*ns2 * 9] <= 0) {
/*                          les 2 sous-triangles ne sont pas subdivises */
				goto L30;
			    }
			}
		    }

/*                 saut>1 => le triangle nte doit etre subdivise en 4 sous-triang */
/*                 -------------------------------------------------------------- */
		    nbsom0 = *nbsomm;
		    te4ste_(nbsomm, mxsomm, &pxyd[4], &nte, letree, ierr);
		    if (*ierr != 0) {
			return 0;
		    }
		    if (*nutysu > 0) {
			i__1 = *nbsomm;
			for (j = nbsom0 + 1; j <= i__1; ++j) {
/*                       mise a jour de taille_ideale des nouveaux sommets de te */
			    tetaid_(nutysu, &pxyd[j * 3 + 1], &pxyd[j * 3 + 2]
				    , &pxyd[j * 3 + 3], ierr);
			    if (*ierr != 0) {
				goto L9999;
			    }
/* L32: */
			}
		    }
		    goto L15;

L30:
		    ;
		}
	    }
	}
	goto L10;
    }
    if (nbs0 < *nbsomm) {
	nbs0 = *nbsomm;
	goto L5;
    }
    return 0;

/*     pb dans le calcul de la fonction taille_ideale */
L9999:
    //io___132.ciunit = unites_2.imprim;
    //s_wsle(&io___132);
    //do_lio(&c__9, &c__1, "pb dans le calcul de taille_ideale", (ftnlen)34);
    //e_wsle();
/*      nblgrc(nrerr) = 1 */
/*      kerr(1) = 'pb dans le calcul de taille_ideale' */
/*      call lereur */
    return 0;
} /* tehote_ */

#undef nuste
#undef ns3
#undef ns2
#undef ns1


/* Subroutine */ int tetrte_(doublereal *comxmi, doublereal *aretmx, integer *
	nbarpi, integer *mxsomm, doublereal *pxyd, integer *mxqueu, integer *
	laqueu, integer *letree, integer *mosoar, integer *mxsoar, integer *
	n1soar, integer *nosoar, integer *moartr, integer *mxartr, integer *
	n1artr, integer *noartr, integer *noarst, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;
    doublereal d__1, d__2;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static doublereal a;
    static integer i__;
    static doublereal s;
    static integer ns1, ns2, ns3, nte;
    static doublereal dmin__, dmax__;
    static integer nbtr, nsot, nutr[13];
    static doublereal xrmin, yrmin, xrmax, yrmax;
    extern /* Subroutine */ int f0trte_(integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *), f1trte_(
	    integer *, doublereal *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *);
    extern integer nopre3_(integer *);
    extern /* Subroutine */ int f2trte_(integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *),
	     f3trte_(integer *, doublereal *, integer *, integer *, integer *,
	     integer *, integer *, integer *, integer *, integer *, integer *,
	     integer *, integer *, integer *, integer *), n1trva_(integer *, 
	    integer *, integer *, integer *, integer *);
    static integer nbmili, milieu[3], niveau, noteva, lequeu, lhqueu;

    /* Fortran I/O blocks */
    static cilist io___146 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    trianguler les triangles equilateraux feuilles et */
/* -----    les points de la frontiere et les points internes imposes */

/* attention: la triangulation finale n'est pas de type delaunay! */

/* entrees: */
/* -------- */
/* comxmi : minimum et maximum des coordonnees de l'objet */
/* aretmx : longueur maximale des aretes des triangles equilateraux */
/* nbarpi : nombre de sommets de la frontiere + nombre de points internes */
/*          imposes par l'utilisateur */
/* mxsomm : nombre maximal de sommets declarables dans pxyd */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */

/* mxqueu : nombre d'entiers utilisables dans laqueu */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          letree(0,0) : no du 1-er te vide dans letree */
/*          letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*          letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*          letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*          si letree(0,.)>0 alors */
/*             letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3,j) :-no pxyd des 1 a 4 points internes au triangle j */
/*                             0  si pas de point */
/*                             ( j est alors une feuille de l'arbre ) */
/*          letree(4,j) : no letree du sur-triangle du triangle j */
/*          letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/* noarst : noarst(i) numero d'une arete de sommet i */

/* auxiliaire : */
/* ------------ */
/* laqueu : mxqueu entiers servant de queue pour le parcours de letree */

/* sorties: */
/* -------- */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/*          =3 si aucun des triangles ne contient l'un des points internes d'un t */
/*          =5 si saturation de la queue de parcours de l'arbre des te */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */



/*                       lequeu:entree dans la queue en gestion circulaire */
/*                       lhqueu:longueur de la queue en gestion circulaire */


/*     le rectangle englobant pour selectionner les te "internes" */
/*     le numero des 3 sommets du te englobant racine de l'arbre des te */
    /* Parameter adjustments */
    comxmi -= 4;
    --noarst;
    pxyd -= 4;
    --laqueu;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;

    /* Function Body */
    ns1 = letree[15];
    ns2 = letree[16];
    ns3 = letree[17];
    a = *aretmx * .01;
/*     abscisse du milieu de l'arete gauche du te 1 */
    s = (pxyd[ns1 * 3 + 1] + pxyd[ns3 * 3 + 1]) / 2;
/* Computing MIN */
    d__1 = s, d__2 = comxmi[4] - *aretmx;
    xrmin = min(d__1,d__2) - a;
/*     abscisse du milieu de l'arete droite du te 1 */
    s = (pxyd[ns2 * 3 + 1] + pxyd[ns3 * 3 + 1]) / 2;
/* Computing MAX */
    d__1 = s, d__2 = comxmi[7] + *aretmx;
    xrmax = max(d__1,d__2) + a;
    yrmin = comxmi[5] - *aretmx;
/*     ordonnee de la droite passant par les milieus des 2 aretes */
/*     droite gauche du te 1 */
    s = (pxyd[ns1 * 3 + 2] + pxyd[ns3 * 3 + 2]) / 2;
/* Computing MAX */
    d__1 = s, d__2 = comxmi[8] + *aretmx;
    yrmax = max(d__1,d__2) + a;

/*     cas particulier de 3 ou 4 ou peu d'aretes frontalieres */
    if (*nbarpi <= 8) {
/*        tout le triangle englobant (racine) est a prendre en compte */
	xrmin = pxyd[ns1 * 3 + 1] - a;
	xrmax = pxyd[ns2 * 3 + 1] + a;
	yrmin = pxyd[ns1 * 3 + 2] - a;
	yrmax = pxyd[ns3 * 3 + 2] + a;
    }

/*     initialisation du tableau noartr */
    i__1 = *mxartr;
    for (i__ = 1; i__ <= i__1; ++i__) {
/*        le numero de l'arete est inconnu */
	noartr[i__ * noartr_dim1 + 1] = 0;
/*        le chainage sur le triangle vide suivant */
	noartr[i__ * noartr_dim1 + 2] = i__ + 1;
/* L5: */
    }
    noartr[*mxartr * noartr_dim1 + 2] = 0;
    *n1artr = 1;

/*     parcours des te jusqu'a trianguler toutes les feuilles (triangles eq) */
/*     ===================================================================== */
/*     initialisation de la queue sur les te */
    *ierr = 0;
    lequeu = 1;
    lhqueu = 0;
/*     la racine de letree initialise la queue */
    laqueu[1] = 1;

/*     tant que la longueur de la queue est >=0 traiter le debut de queue */
L10:
    if (lhqueu >= 0) {

/*        le triangle te a traiter */
	i__ = lequeu - lhqueu;
	if (i__ <= 0) {
	    i__ = *mxqueu + i__;
	}
	nte = laqueu[i__];
/*        la longueur est reduite */
	--lhqueu;

/*        nte est il un sous-triangle feuille (minimal) ? */
/* L15: */
	if (letree[nte * 9] > 0) {
/*           non les 4 sous-triangles sont mis dans la queue */
	    if (lhqueu + 4 >= *mxqueu) {
		//io___146.ciunit = unites_2.imprim;
		//s_wsle(&io___146);
		//do_lio(&c__9, &c__1, "tetrte: saturation de la queue", (
		//	ftnlen)30);
		//e_wsle();
		*ierr = 5;
		return 0;
	    }
	    for (i__ = 3; i__ >= 0; --i__) {
/*              ajout du sous-triangle i */
		++lhqueu;
		++lequeu;
		if (lequeu > *mxqueu) {
		    lequeu -= *mxqueu;
		}
		laqueu[lequeu] = letree[i__ + nte * 9];
/* L20: */
	    }
	    goto L10;
	}

/*        ici nte est un triangle minimal non subdivise */
/*        --------------------------------------------- */
/*        le te est il dans le cadre englobant de l'objet ? */
	ns1 = letree[nte * 9 + 6];
	ns2 = letree[nte * 9 + 7];
	ns3 = letree[nte * 9 + 8];
	if (pxyd[ns1 * 3 + 1] > pxyd[ns2 * 3 + 1]) {
	    dmin__ = pxyd[ns2 * 3 + 1];
	    dmax__ = pxyd[ns1 * 3 + 1];
	} else {
	    dmin__ = pxyd[ns1 * 3 + 1];
	    dmax__ = pxyd[ns2 * 3 + 1];
	}
	if ((xrmin <= dmin__ && dmin__ <= xrmax) || (xrmin <= dmax__ && dmax__ <=
		 xrmax)) {
	    if (pxyd[ns1 * 3 + 2] > pxyd[ns3 * 3 + 2]) {
		dmin__ = pxyd[ns3 * 3 + 2];
		dmax__ = pxyd[ns1 * 3 + 2];
	    } else {
		dmin__ = pxyd[ns1 * 3 + 2];
		dmax__ = pxyd[ns3 * 3 + 2];
	    }
	    if ((yrmin <= dmin__ && dmin__ <= yrmax) || (yrmin <= dmax__ && 
		    dmax__ <= yrmax)) {

/*              te minimal et interne au rectangle englobant */
/*              -------------------------------------------- */
/*              recherche du nombre de niveaux entre nte et les te voisins */
/*              par ses aretes */
		nbmili = 0;
		for (i__ = 1; i__ <= 3; ++i__) {

/*                 a priori pas de milieu de l'arete i du te nte */
		    milieu[i__ - 1] = 0;

/*                 recherche de noteva te voisin de nte par l'arete i */
		    n1trva_(&nte, &i__, letree, &noteva, &niveau);
/*                 noteva  : >0 numero letree du te voisin par l'arete i */
/*                           =0 si pas de te voisin (racine , ... ) */
/*                 niveau  : =0 si nte et noteva ont meme taille */
/*                           >0 nte est 4**niveau fois plus petit que noteva */
		    if (noteva > 0) {
/*                    il existe un te voisin */
			if (letree[noteva * 9] > 0) {
/*                       noteva est plus petit que nte */
/*                       => recherche du numero du milieu du cote=sommet du te no */
/*                       le sous-te 0 du te noteva */
			    nsot = letree[noteva * 9];
/*                       le numero dans pxyd du milieu de l'arete i de nte */
			    milieu[i__ - 1] = letree[nopre3_(&i__) + 5 + nsot 
				    * 9];
			    ++nbmili;
			}
		    }

/* L30: */
		}

/*              triangulation du te nte en fonction du nombre de ses milieux */
		switch (nbmili + 1) {
		    case 1:  goto L50;
		    case 2:  goto L100;
		    case 3:  goto L200;
		    case 4:  goto L300;
		}

/*              0 milieu => 1 triangle = le te nte */
/*              ---------------------------------- */
L50:
		f0trte_(&letree[nte * 9], &pxyd[4], mosoar, mxsoar, n1soar, &
			nosoar[nosoar_offset], moartr, mxartr, n1artr, &
			noartr[noartr_offset], &noarst[1], &nbtr, nutr, ierr);
		if (*ierr != 0) {
		    return 0;
		}
		goto L10;

/*              1 milieu => 2 triangles = 2 demi te */
/*              ----------------------------------- */
L100:
		f1trte_(&letree[nte * 9], &pxyd[4], milieu, mosoar, mxsoar, 
			n1soar, &nosoar[nosoar_offset], moartr, mxartr, 
			n1artr, &noartr[noartr_offset], &noarst[1], &nbtr, 
			nutr, ierr);
		if (*ierr != 0) {
		    return 0;
		}
		goto L10;

/*              2 milieux => 3 triangles */
/*              ----------------------------------- */
L200:
		f2trte_(&letree[nte * 9], &pxyd[4], milieu, mosoar, mxsoar, 
			n1soar, &nosoar[nosoar_offset], moartr, mxartr, 
			n1artr, &noartr[noartr_offset], &noarst[1], &nbtr, 
			nutr, ierr);
		if (*ierr != 0) {
		    return 0;
		}
		goto L10;

/*              3 milieux => 4 triangles = 4 quart te */
/*              ------------------------------------- */
L300:
		f3trte_(&letree[nte * 9], &pxyd[4], milieu, mosoar, mxsoar, 
			n1soar, &nosoar[nosoar_offset], moartr, mxartr, 
			n1artr, &noartr[noartr_offset], &noarst[1], &nbtr, 
			nutr, ierr);
		if (*ierr != 0) {
		    return 0;
		}
		goto L10;
	    }
	}
	goto L10;
    }
    return 0;
} /* tetrte_ */

/* Subroutine */ int aisoar_(integer *mosoar, integer *mxsoar, integer *
	nosoar, integer *na1)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, i__1;

    /* Local variables */
    static integer na, na0;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    chainer en colonne lchain les aretes non vides et */
/* -----    non frontalieres du tableau nosoar */

/* entrees: */
/* -------- */
/* mosoar : nombre maximal d'entiers par arete dans le tableau nosoar */
/* mxsoar : nombre maximal d'aretes frontalieres declarables */

/* modifies : */
/* ---------- */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages en + */
/*          nosoar(lchain,i)=arete interne suivante */

/* sortie : */
/* -------- */
/* na1    : numero dans nosoar de la premiere arete interne */
/*          les suivantes sont nosoar(lchain,na1), ... */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

/*     formation du chainage des aretes internes a echanger eventuellement */
/*     recherche de la premiere arete non vide et non frontaliere */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;

    /* Function Body */
    i__1 = *mxsoar;
    for (*na1 = 1; *na1 <= i__1; ++(*na1)) {
	if (nosoar[*na1 * nosoar_dim1 + 1] > 0 && nosoar[*na1 * nosoar_dim1 + 
		3] <= 0) {
	    goto L15;
	}
/* L10: */
    }

/*     protection de la premiere arete non vide et non frontaliere */
L15:
    na0 = *na1;
    i__1 = *mxsoar;
    for (na = *na1 + 1; na <= i__1; ++na) {
	if (nosoar[na * nosoar_dim1 + 1] > 0 && nosoar[na * nosoar_dim1 + 3] 
		<= 0) {
/*           arete interne => elle est chainee a partir de la precedente */
	    nosoar[na0 * nosoar_dim1 + 6] = na;
	    na0 = na;
	}
/* L20: */
    }

/*     la derniere arete interne n'a pas de suivante */
    nosoar[na0 * nosoar_dim1 + 6] = 0;
    return 0;
} /* aisoar_ */

/* Subroutine */ int tedela_(doublereal *pxyd, integer *noarst, integer *
	mosoar, integer *mxsoar, integer *n1soar, integer *nosoar, integer *
	n1ardv, integer *moartr, integer *mxartr, integer *n1artr, integer *
	noartr, integer *modifs)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;
    doublereal d__1, d__2;

    /* Local variables */
    static integer i__, j, n;
    static doublereal r0, a12;
    static integer na;
    static doublereal s12, s34;
    static integer nt, na0;
    static doublereal s123, s142, s143, s234;
    static integer ns1, ns2, ns3, ns4, na34, ierr;
    extern /* Subroutine */ int te2t2t_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *), mt4sqa_(
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *), cenced_(doublereal *, 
	    doublereal *, doublereal *, doublereal *, integer *);
    extern doublereal surtd2_(doublereal *, doublereal *, doublereal *);
    static doublereal cetria[3];

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    pour toutes les aretes chainees dans nosoar(lchain,*) */
/* -----    du tableau nosoar */
/*          echanger la diagonale des 2 triangles si le sommet oppose */
/*          a un triangle ayant en commun une arete appartient au cercle */
/*          circonscrit de l'autre (violation boule vide delaunay) */

/* entrees: */
/* -------- */
/* pxyd   : tableau des x  y  distance_souhaitee de chaque sommet */

/* modifies : */
/* ---------- */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* mosoar : nombre maximal d'entiers par arete dans le tableau nosoar */
/* mxsoar : nombre maximal d'aretes frontalieres declarables */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages en + */
/* n1ardv : numero dans nosoar de la premiere arete du chainage */
/*          des aretes a rendre delaunay */

/* moartr : nombre d'entiers par triangle dans le tableau noartr */
/* mxartr : nombre maximal de triangles declarables dans noartr */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* modifs : nombre d'echanges de diagonales pour maximiser la qualite */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

/*     le nombre d'echanges de diagonales pour minimiser l'aire */
    /* Parameter adjustments */
    pxyd -= 4;
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;

    /* Function Body */
    *modifs = 0;
    r0 = 0.;

/*     la premiere arete du chainage des aretes a rendre delaunay */
    na0 = *n1ardv;

/*     tant que la pile des aretes a echanger eventuellement est non vide */
/*     ================================================================== */
L20:
    if (na0 > 0) {

/*        l'arete a traiter */
	na = na0;
/*        la prochaine arete a traiter */
	na0 = nosoar[na0 * nosoar_dim1 + 6];

/*        l'arete est marquee traitee avec le numero -1 */
	nosoar[na * nosoar_dim1 + 6] = -1;

/*        l'arete est elle active? */
	if (nosoar[na * nosoar_dim1 + 1] == 0) {
	    goto L20;
	}

/*        si arete frontaliere pas d'echange possible */
	if (nosoar[na * nosoar_dim1 + 3] > 0) {
	    goto L20;
	}

/*        existe-t-il 2 triangles ayant cette arete commune? */
	if (nosoar[na * nosoar_dim1 + 4] <= 0 || nosoar[na * nosoar_dim1 + 5] 
		<= 0) {
	    goto L20;
	}

/*        aucun des 2 triangles est-il desactive? */
	if (noartr[nosoar[na * nosoar_dim1 + 4] * noartr_dim1 + 1] == 0 || 
		noartr[nosoar[na * nosoar_dim1 + 5] * noartr_dim1 + 1] == 0) {
	    goto L20;
	}

/*        l'arete appartient a deux triangles actifs */
/*        le numero des 4 sommets du quadrangle des 2 triangles */
	mt4sqa_(&na, moartr, &noartr[noartr_offset], mosoar, &nosoar[
		nosoar_offset], &ns1, &ns2, &ns3, &ns4);
	if (ns4 == 0) {
	    goto L20;
	}

/*        carre de la longueur de l'arete ns1 ns2 */
/* Computing 2nd power */
	d__1 = pxyd[ns2 * 3 + 1] - pxyd[ns1 * 3 + 1];
/* Computing 2nd power */
	d__2 = pxyd[ns2 * 3 + 2] - pxyd[ns1 * 3 + 2];
	a12 = d__1 * d__1 + d__2 * d__2;

/*        comparaison de la somme des aires des 2 triangles */
/*        ------------------------------------------------- */
/*        calcul des surfaces des triangles 123 et 142 de cette arete */
	s123 = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 
		+ 1]);
	s142 = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns4 * 3 + 1], &pxyd[ns2 * 3 
		+ 1]);
	s12 = abs(s123) + abs(s142);
	if (s12 <= a12 * .001f) {
	    goto L20;
	}

/*        calcul des surfaces des triangles 143 et 234 de cette arete */
	s143 = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns4 * 3 + 1], &pxyd[ns3 * 3 
		+ 1]);
	s234 = surtd2_(&pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 1], &pxyd[ns4 * 3 
		+ 1]);
	s34 = abs(s234) + abs(s143);

	if ((d__1 = s34 - s12, abs(d__1)) > s34 * 1e-15) {
	    goto L20;
	}

/*        quadrangle convexe : le critere de delaunay intervient */
/*        ------------------   --------------------------------- */
/*        calcul du centre et rayon de la boule circonscrite a ns123 */
/*        pas d'affichage si le triangle est degenere */
	ierr = -1;
	cenced_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 1], 
		cetria, &ierr);
	if (ierr > 0) {
/*           ierr=1 si triangle degenere  => abandon */
	    goto L20;
	}

/* Computing 2nd power */
	d__1 = cetria[0] - pxyd[ns4 * 3 + 1];
/* Computing 2nd power */
	d__2 = cetria[1] - pxyd[ns4 * 3 + 2];
	if (d__1 * d__1 + d__2 * d__2 < cetria[2]) {

/*           protection contre une boucle infinie sur le meme cercle */
	    if (r0 == cetria[2]) {
		goto L20;
	    }

/*           oui: ns4 est dans le cercle circonscrit a ns1 ns2 ns3 */
/*           => ns3 est aussi dans le cercle circonscrit de ns1 ns2 ns4 */
/*           echange de la diagonale 12 par 34 des 2 triangles */
	    te2t2t_(&na, mosoar, n1soar, &nosoar[nosoar_offset], &noarst[1], 
		    moartr, &noartr[noartr_offset], &na34);
	    if (na34 == 0) {
		goto L20;
	    }
	    r0 = cetria[2];

/*           l'arete na34 est marquee traitee */
	    nosoar[na34 * nosoar_dim1 + 6] = -1;
	    ++(*modifs);

/*           les aretes internes peripheriques des 2 triangles sont enchainees */
	    for (j = 4; j <= 5; ++j) {
		nt = nosoar[j + na34 * nosoar_dim1];
		for (i__ = 1; i__ <= 3; ++i__) {
		    n = (i__1 = noartr[i__ + nt * noartr_dim1], abs(i__1));
		    if (n != na34) {
			if (nosoar[n * nosoar_dim1 + 3] == 0 && nosoar[n * 
				nosoar_dim1 + 6] == -1) {
/*                        cette arete marquee est chainee pour etre traitee */
			    nosoar[n * nosoar_dim1 + 6] = na0;
			    na0 = n;
			}
		    }
/* L50: */
		}
/* L60: */
	    }
	    goto L20;
	}

/*        retour en haut de la pile des aretes a traiter */
	goto L20;
    }

    return 0;
} /* tedela_ */

/* Subroutine */ int terefr_(integer *nbarpi, doublereal *pxyd, integer *
	mosoar, integer *mxsoar, integer *n1soar, integer *nosoar, integer *
	moartr, integer *mxartr, integer *n1artr, integer *noartr, integer *
	noarst, integer *mxarcf, integer *n1arcf, integer *noarcf, integer *
	larmin, integer *notrcf, integer *nbarpe, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;

    /* Local variables */
    static integer ns1, ns2, narete;
    extern /* Subroutine */ int tefoar_(integer *, integer *, doublereal *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   recherche des aretes de la frontiere non dans la triangulation */
/* -----   triangulation frontale pour les reobtenir */

/*         attention: le chainage lchain de nosoar devient celui des cf */

/* entrees: */
/* -------- */
/*          le tableau nosoar */
/* nbarpi : numero du dernier point interne impose par l'utilisateur */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles declarables dans noartr */
/* mxarcf : nombre de variables des tableaux n1arcf, noarcf, larmin, notrcf */

/* modifies: */
/* --------- */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero d'une arete de sommet i */


/* auxiliaires : */
/* ------------- */
/* n1arcf : tableau (0:mxarcf) auxiliaire d'entiers */
/* noarcf : tableau (3,mxarcf) auxiliaire d'entiers */
/* larmin : tableau (mxarcf)   auxiliaire d'entiers */
/* notrcf : tableau (mxarcf)   auxiliaire d'entiers */

/* sortie : */
/* -------- */
/* nbarpe : nombre d'aretes perdues puis retrouvees */
/* ierr   : =0 si pas d'erreur */
/*          >0 si une erreur est survenue */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

    /* Parameter adjustments */
    pxyd -= 4;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --notrcf;
    --larmin;
    noarcf -= 4;

    /* Function Body */
    *ierr = 0;

/*     le nombre d'aretes de la frontiere non arete de la triangulation */
    *nbarpe = 0;

/*     initialisation du chainage des aretes des cf => 0 arete de cf */
    i__1 = *mxsoar;
    for (narete = 1; narete <= i__1; ++narete) {
	nosoar[narete * nosoar_dim1 + 6] = -1;
/* L10: */
    }

/*     boucle sur l'ensemble des aretes actuelles */
/*     ========================================== */
    i__1 = *mxsoar;
    for (narete = 1; narete <= i__1; ++narete) {

	if (nosoar[narete * nosoar_dim1 + 3] > 0) {
/*           arete appartenant a une ligne => frontaliere */

	    if (nosoar[narete * nosoar_dim1 + 4] <= 0 || nosoar[narete * 
		    nosoar_dim1 + 5] <= 0) {
/*              l'arete narete frontaliere n'appartient pas a 2 triangles */
/*              => elle est perdue */
		++(*nbarpe);

/*              le numero des 2 sommets de l'arete frontaliere perdue */
		ns1 = nosoar[narete * nosoar_dim1 + 1];
		ns2 = nosoar[narete * nosoar_dim1 + 2];
/*               write(imprim,10000) ns1,(pxyd(j,ns1),j=1,2), */
/*     %                             ns2,(pxyd(j,ns2),j=1,2) */
/* L10000: */

/*              traitement de cette arete perdue ns1-ns2 */
		tefoar_(&narete, nbarpi, &pxyd[4], mosoar, mxsoar, n1soar, &
			nosoar[nosoar_offset], moartr, mxartr, n1artr, &
			noartr[noartr_offset], &noarst[1], mxarcf, n1arcf, &
			noarcf[4], &larmin[1], &notrcf[1], ierr);
		if (*ierr != 0) {
		    return 0;
		}

/*              fin du traitement de cette arete perdue et retrouvee */
	    }
	}

/* L30: */
    }
    return 0;
} /* terefr_ */

/* Subroutine */ int tesuex_(integer *nblftr, integer *nulftr, integer *
	ndtri0, integer *nbsomm, doublereal *pxyd, integer *nslign, integer *
	mosoar, integer *mxsoar, integer *nosoar, integer *moartr, integer *
	mxartr, integer *n1artr, integer *noartr, integer *noarst, integer *
	nbtria, integer *letrsu, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1, 
	    i__2;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, j, na, nl, nt, na0, na1, ns1, ns2, nt2;
    static doublereal dmin__;
    static integer noar, noar1, ligne, ntmin, ligne0, lsigne;

    /* Fortran I/O blocks */
    static cilist io___187 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    supprimer du tableau noartr les triangles externes au domaine */
/* -----    en annulant le numero de leur 1-ere arete dans noartr */
/*          et en les chainant comme triangles vides */

/* entrees: */
/* -------- */
/* nblftr : nombre de  lignes fermees definissant la surface */
/* nulftr : numero des lignes fermees definissant la surface */
/* ndtri0 : plus grand numero dans noartr d'un triangle */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* nslign : tableau du numero de sommet dans sa ligne pour chaque */
/*          sommet frontalier */
/*          numero du point dans le lexique point si interne impose */
/*          0 si le point est interne non impose par l'utilisateur */
/*         -1 si le sommet est externe au domaine */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles declarables */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero nosoar d'une arete de sommet i */

/* sorties: */
/* -------- */
/* nbtria : nombre de triangles internes au domaine */
/* letrsu : letrsu(nt)=numero du triangle interne, 0 sinon */
/* noarst : noarst(i) numero nosoar d'une arete du sommet i (modifi'e) */
/* ierr   : 0 si pas d'erreur, >0 sinon */
/* c++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc        mai 1999 */
/* 2345x7..............................................................012 */

/*     les triangles sont a priori non marques */
    /* Parameter adjustments */
    --nulftr;
    --letrsu;
    --nslign;
    pxyd -= 4;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;

    /* Function Body */
    i__1 = *ndtri0;
    for (nt = 1; nt <= i__1; ++nt) {
	letrsu[nt] = 0;
/* L5: */
    }

/*     les aretes sont marquees non chainees */
    i__1 = *mxsoar;
    for (noar1 = 1; noar1 <= i__1; ++noar1) {
	nosoar[noar1 * nosoar_dim1 + 6] = -2;
/* L10: */
    }

/*     recherche du sommet de la triangulation de plus petite abscisse */
/*     =============================================================== */
    ntmin = 0;
    dmin__ = 1e38;
    i__1 = *nbsomm;
    for (i__ = 1; i__ <= i__1; ++i__) {
	if (pxyd[i__ * 3 + 1] < dmin__) {
/*           le nouveau minimum */
	    noar1 = noarst[i__];
	    if (noar1 > 0) {
/*              le sommet appartient a une arete de triangle */
		if (nosoar[noar1 * nosoar_dim1 + 4] > 0) {
/*                 le nouveau minimum */
		    dmin__ = pxyd[i__ * 3 + 1];
		    ntmin = i__;
		}
	    }
	}
/* L20: */
    }

/*     une arete de sommet ntmin */
    noar1 = noarst[ntmin];
/*     un triangle d'arete noar1 */
    ntmin = nosoar[noar1 * nosoar_dim1 + 4];
    if (ntmin <= 0) {
/*         nblgrc(nrerr) = 1 */
/*         kerr(1) = 'pas de triangle d''abscisse minimale' */
/*         call lereur */
	//io___187.ciunit = unites_2.imprim;
	//s_wsle(&io___187);
	//do_lio(&c__9, &c__1, "pas de triangle d'abscisse minimale", (ftnlen)
	//	35);
	//e_wsle();
	*ierr = 2;
	goto L9990;
    }

/*     chainage des 3 aretes du triangle ntmin */
/*     ======================================= */
/*     la premiere arete du chainage des aretes traitees */
    noar1 = (i__1 = noartr[ntmin * noartr_dim1 + 1], abs(i__1));
    na0 = (i__1 = noartr[ntmin * noartr_dim1 + 2], abs(i__1));
/*     elle est chainee sur la seconde arete du triangle ntmin */
    nosoar[noar1 * nosoar_dim1 + 6] = na0;
/*     les 2 autres aretes du triangle ntmin sont chainees */
    na1 = (i__1 = noartr[ntmin * noartr_dim1 + 3], abs(i__1));
/*     la seconde est chainee sur la troisieme arete */
    nosoar[na0 * nosoar_dim1 + 6] = na1;
/*     la troisieme n'a pas de suivante */
    nosoar[na1 * nosoar_dim1 + 6] = 0;

/*     le triangle ntmin est a l'exterieur du domaine */
/*     tous les triangles externes sont marques -123 456 789 */
/*     les triangles de l'autre cote d'une arete sur une ligne */
/*     sont marques: no de la ligne de l'arete * signe oppose */
/*     ======================================================= */
    ligne0 = 0;
    ligne = -123456789;

L40:
    if (noar1 != 0) {

/*        l'arete noar1 du tableau nosoar est a traiter */
/*        --------------------------------------------- */
	noar = noar1;
/*        l'arete suivante devient la premiere a traiter ensuite */
	noar1 = nosoar[noar1 * nosoar_dim1 + 6];
/*        l'arete noar est traitee */
	nosoar[noar * nosoar_dim1 + 6] = -3;

	for (i__ = 4; i__ <= 5; ++i__) {

/*           l'un des 2 triangles de l'arete */
	    nt = nosoar[i__ + noar * nosoar_dim1];
	    if (nt > 0) {

/*              triangle deja traite pour une ligne anterieure? */
		if (letrsu[nt] != 0 && (i__1 = letrsu[nt], abs(i__1)) != 
			ligne) {
		    goto L60;
		}

/*              le triangle est marque avec la valeur de ligne */
		letrsu[nt] = ligne;

/*              chainage eventuel des autres aretes de ce triangle */
/*              si ce n'est pas encore fait */
		for (j = 1; j <= 3; ++j) {

/*                 le numero na de l'arete j du triangle nt dans nosoar */
		    na = (i__1 = noartr[j + nt * noartr_dim1], abs(i__1));
		    if (nosoar[na * nosoar_dim1 + 6] != -2) {
			goto L50;
		    }

/*                 le numero de 1 a nblftr dans nulftr de la ligne de l'arete */
		    nl = nosoar[na * nosoar_dim1 + 3];

/*                 si l'arete est sur une ligne fermee differente de celle envelo */
/*                 et non marquee alors examen du triangle oppose */
		    if (nl > 0) {

			if (nl == ligne0) {
			    goto L50;
			}

/*                    arete frontaliere de ligne non traitee */
/*                    => passage de l'autre cote de la ligne */
/*                    le triangle de l'autre cote de la ligne est recherche */
			if (nt == (i__1 = nosoar[na * nosoar_dim1 + 4], abs(
				i__1))) {
			    nt2 = 5;
			} else {
			    nt2 = 4;
			}
			nt2 = (i__1 = nosoar[nt2 + na * nosoar_dim1], abs(
				i__1));
			if (nt2 > 0) {

/*                       le triangle nt2 de l'autre cote est marque avec le */
/*                       avec le signe oppose de celui de ligne */
			    if (ligne >= 0) {
				lsigne = -1;
			    } else {
				lsigne = 1;
			    }
			    letrsu[nt2] = lsigne * nl;

/*                       temoin de ligne a traiter ensuite dans nulftr */
			    nulftr[nl] = -(i__1 = nulftr[nl], abs(i__1));

/*                       l'arete est traitee */
			    nosoar[na * nosoar_dim1 + 6] = -3;

			}

/*                    l'arete est traitee */
			goto L50;

		    }

/*                 arete non traitee => elle est chainee */
		    nosoar[na * nosoar_dim1 + 6] = noar1;
		    noar1 = na;

L50:
		    ;
		}

	    }
L60:
	    ;
	}

	goto L40;
    }
/*     les triangles de la ligne fermee ont tous ete marques */
/*     plus d'arete chainee */

/*     recherche d'une nouvelle ligne fermee a traiter */
/*     =============================================== */
L65:
    i__1 = *nblftr;
    for (nl = 1; nl <= i__1; ++nl) {
	if (nulftr[nl] < 0) {
	    goto L80;
	}
/* L70: */
    }
/*     plus de ligne fermee a traiter */
    goto L110;

/*     tous les triangles de cette composante connexe */
/*     entre ligne et ligne0 vont etre marques */
/*     ============================================== */
/*     remise en etat du numero de ligne */
/*     nl est le numero de la ligne dans nulftr a traiter */
L80:
    nulftr[nl] = -nulftr[nl];
    i__1 = *ndtri0;
    for (nt2 = 1; nt2 <= i__1; ++nt2) {
	if ((i__2 = letrsu[nt2], abs(i__2)) == nl) {
	    goto L92;
	}
/* L90: */
    }

/*     recherche de l'arete j du triangle nt2 avec ce numero de ligne nl */
L92:
    for (j = 1; j <= 3; ++j) {

/*        le numero de l'arete j du triangle dans nosoar */
	noar1 = 0;
	na0 = (i__1 = noartr[j + nt2 * noartr_dim1], abs(i__1));
	if (nl == nosoar[na0 * nosoar_dim1 + 3]) {

/*           na0 est l'arete de ligne nl */
/*           l'arete suivante du triangle nt2 */
	    i__ = j % 3 + 1;
/*           le numero dans nosoar de l'arete i de nt2 */
	    na1 = (i__1 = noartr[i__ + nt2 * noartr_dim1], abs(i__1));
	    if (nosoar[na1 * nosoar_dim1 + 6] == -2) {
/*              arete non traitee => elle est la premiere du chainage */
		noar1 = na1;
/*              pas de suivante dans ce chainage */
		nosoar[na1 * nosoar_dim1 + 6] = 0;
	    } else {
		na1 = 0;
	    }

/*           l'eventuelle seconde arete suivante */
	    i__ = i__ % 3 + 1;
	    na = (i__1 = noartr[i__ + nt2 * noartr_dim1], abs(i__1));
	    if (nosoar[na * nosoar_dim1 + 6] == -2) {
		if (na1 == 0) {
/*                 1 arete non traitee et seule a chainer */
		    noar1 = na;
		    nosoar[na * nosoar_dim1 + 6] = 0;
		} else {
/*                 2 aretes a chainer */
		    noar1 = na;
		    nosoar[na * nosoar_dim1 + 6] = na1;
		}
	    }

	    if (noar1 > 0) {

/*              il existe au moins une arete a visiter pour ligne */
/*              marquage des triangles internes a la ligne nl */
		ligne = letrsu[nt2];
		ligne0 = nl;
		goto L40;

	    } else {

/*              nt2 est le seul triangle de la ligne fermee */
		goto L65;

	    }
	}
/* L95: */
    }

/*     reperage des sommets internes ou externes dans nslign */
/*     nslign(sommet externe au domaine)=-1 */
/*     nslign(sommet interne au domaine)= 0 */
/*     ===================================================== */
L110:
    i__1 = *nbsomm;
    for (ns1 = 1; ns1 <= i__1; ++ns1) {
/*        tout sommet non sur la frontiere ou interne impose */
/*        est suppose externe */
	if (nslign[ns1] == 0) {
	    nslign[ns1] = -1;
	}
/* L170: */
    }

/*     les triangles externes sont marques vides dans le tableau noartr */
/*     ================================================================ */
    *nbtria = 0;
    i__1 = *ndtri0;
    for (nt = 1; nt <= i__1; ++nt) {

	if (letrsu[nt] <= 0) {

/*           triangle nt externe */
	    if (noartr[nt * noartr_dim1 + 1] != 0) {
/*              la premiere arete est annulee */
		noartr[nt * noartr_dim1 + 1] = 0;
/*              le triangle nt est considere comme etant vide */
		noartr[nt * noartr_dim1 + 2] = *n1artr;
		*n1artr = nt;
	    }

	} else {

/*           triangle nt interne */
	    ++(*nbtria);
	    letrsu[nt] = *nbtria;

/*           marquage des 3 sommets du triangle nt */
	    for (i__ = 1; i__ <= 3; ++i__) {
/*              le numero nosoar de l'arete i du triangle nt */
		noar = (i__2 = noartr[i__ + nt * noartr_dim1], abs(i__2));
/*              le numero des 2 sommets */
		ns1 = nosoar[noar * nosoar_dim1 + 1];
		ns2 = nosoar[noar * nosoar_dim1 + 2];
/*              mise a jour du numero d'une arete des 2 sommets de l'arete */
		noarst[ns1] = noar;
		noarst[ns2] = noar;
/*              ns1 et ns2 sont des sommets de la triangulation du domaine */
		if (nslign[ns1] < 0) {
		    nslign[ns1] = 0;
		}
		if (nslign[ns2] < 0) {
		    nslign[ns2] = 0;
		}
/* L190: */
	    }

	}

/* L200: */
    }
/*     ici tout sommet externe ns verifie nslign(ns)=-1 */

/*     les triangles externes sont mis a zero dans nosoar */
/*     ================================================== */
    i__1 = *mxsoar;
    for (noar = 1; noar <= i__1; ++noar) {

	if (nosoar[noar * nosoar_dim1 + 1] > 0) {

/*           le second triangle de l'arete noar */
	    nt = nosoar[noar * nosoar_dim1 + 5];
	    if (nt > 0) {
/*              si le triangle nt est externe */
/*              alors il est supprime pour l'arete noar */
		if (letrsu[nt] <= 0) {
		    nosoar[noar * nosoar_dim1 + 5] = 0;
		}
	    }

/*           le premier triangle de l'arete noar */
	    nt = nosoar[noar * nosoar_dim1 + 4];
	    if (nt > 0) {
		if (letrsu[nt] <= 0) {
/*                 si le triangle nt est externe */
/*                 alors il est supprime pour l'arete noar */
/*                 et l'eventuel triangle oppose prend sa place */
/*                 en position 4 de nosoar */
		    if (nosoar[noar * nosoar_dim1 + 5] > 0) {
			nosoar[noar * nosoar_dim1 + 4] = nosoar[noar * 
				nosoar_dim1 + 5];
			nosoar[noar * nosoar_dim1 + 5] = 0;
		    } else {
			nosoar[noar * nosoar_dim1 + 4] = 0;
		    }
		}
	    }
	}

/* L300: */
    }

/*     remise en etat pour eviter les modifications de ladefi */
L9990:
    i__1 = *nblftr;
    for (nl = 1; nl <= i__1; ++nl) {
	if (nulftr[nl] < 0) {
	    nulftr[nl] = -nulftr[nl];
	}
/* L9991: */
    }
    return 0;
} /* tesuex_ */

/* Subroutine */ int trp1st_(integer *ns, integer *noarst, integer *mosoar, 
	integer *nosoar, integer *moartr, integer *mxartr, integer *noartr, 
	integer *mxpile, integer *lhpile, integer *lapile)
{
    /* Format strings */
    static char fmt_19990[] = "(5(\002 triangle\002,i9))";

    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset, i__1;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void), s_wsfe(cilist *), do_fio(integer *, char *, ftnlen),
	   //  e_wsfe(void);

    /* Local variables */
    static integer j, ii, nt0, nt1, nar, nta, noar, nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___202 = { 0, 0, 0, 0, 0 };
    static cilist io___207 = { 0, 0, 0, 0, 0 };
    static cilist io___208 = { 0, 0, 0, 0, 0 };
    static cilist io___209 = { 0, 0, 0, 0, 0 };
    static cilist io___210 = { 0, 0, 0, 0, 0 };
    static cilist io___212 = { 0, 0, 0, 0, 0 };
    static cilist io___213 = { 0, 0, 0, 0, 0 };
    static cilist io___214 = { 0, 0, 0, fmt_19990, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   recherche des triangles de noartr partageant le sommet ns */
/* ----- */
/*         limite: un camembert de centre ns entame 2 fois */
/*                 ne donne que l'une des parties */

/* entrees: */
/* -------- */
/* ns     : numero du sommet */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre de triangles declares dans noartr */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/* mxpile : nombre maximal de triangles empilables */

/* sorties : */
/* --------- */
/* lhpile : >0 nombre de triangles empiles */
/*          =0       si impossible de tourner autour du point */
/*                   ou zero triangle contenant le sommet ns */
/*          =-lhpile si apres butee sur la frontiere il y a a nouveau */
/*          butee sur la frontiere . a ce stade on ne peut dire si tous */
/*          les triangles ayant ce sommet ont ete recenses */
/*          ce cas arrive seulement si le sommet est sur la frontiere */
/*          par un balayage de tous les triangles, lhpile donne le */
/*          nombre de triangles de sommet ns */
/*          remarque: si la pile est saturee recherche de tous les */
/*          triangles de sommet ns par balayage de tous les triangles */
/* lapile : numero dans noartr des triangles de sommet ns */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur: alain perronnet analyse numerique paris upmc         mars 1997 */
/* modifs: alain perronnet Laboratoire J-L. Lions UPMC Paris octobre 2006 */
/* ....................................................................012 */

    /* Parameter adjustments */
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --lapile;

    /* Function Body */
    *lhpile = 0;

/*     la premiere arete de sommet ns */
    nar = noarst[*ns];
    if (nar <= 0) {
/* cc         write(imprim,*) 'trp1st: sommet',ns,' sans arete' */
	goto L100;
    }

/*     l'arete nar est elle active? */
    if (nosoar[nar * nosoar_dim1 + 1] <= 0) {
/* cc         write(imprim,*) 'trp1st: arete vide',nar, */
/* cc     %                  ' st1:', nosoar(1,nar),' st2:',nosoar(2,nar) */
	goto L100;
    }

/*     le premier triangle de sommet ns */
 //   nt0 = (i__1 = nosoar[nar * nosoar_dim1 + 4], abs(i__1));
    if (nt0 <= 0) {
	//io___202.ciunit = unites_1.imprim;
	//s_wsle(&io___202);
	//do_lio(&c__9, &c__1, "trp1st: sommet", (ftnlen)14);
	//do_lio(&c__3, &c__1, (char *)&(*ns), (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " dans aucun triangle", (ftnlen)20);
	//e_wsle();
	goto L100;
    }

/*     le triangle est il actif? */
    if (noartr[nt0 * noartr_dim1 + 1] == 0) {
	goto L100;
    }

/*     le numero des 3 sommets du triangle nt0 dans le sens direct */
    nusotr_(&nt0, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
	    noartr_offset], nosotr);

/*     reperage du sommet ns dans le triangle nt0 */
    for (nar = 1; nar <= 3; ++nar) {
	if (nosotr[nar - 1] == *ns) {
	    goto L10;
	}
/* L5: */
    }
/*     pas de sommet ns dans le triangle nt0 */
    goto L100;

/*     ns retrouve : le triangle nt0 de sommet ns est empile */
L10:
    *lhpile = 1;
    lapile[1] = nt0;
    nta = nt0;

/*     recherche dans le sens des aiguilles d'une montre */
/*     (sens indirect) du triangle nt1 de l'autre cote de l'arete */
/*     nar du triangle et en tournant autour du sommet ns */
/*     ========================================================== */
    noar = (i__1 = noartr[nar + nt0 * noartr_dim1], abs(i__1));
/*     le triangle nt1 oppose du triangle nt0 par l'arete noar */
    if (nosoar[noar * nosoar_dim1 + 4] == nt0) {
	nt1 = nosoar[noar * nosoar_dim1 + 5];
    } else if (nosoar[noar * nosoar_dim1 + 5] == nt0) {
	nt1 = nosoar[noar * nosoar_dim1 + 4];
    } else {
	//io___207.ciunit = unites_1.imprim;
	//s_wsle(&io___207);
	//do_lio(&c__9, &c__1, "trp1st: anomalie arete", (ftnlen)22);
	//do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " sans triangle", (ftnlen)14);
	//do_lio(&c__3, &c__1, (char *)&nt0, (ftnlen)sizeof(integer));
	//e_wsle();
	goto L100;
    }

/*     la boucle sur les triangles nt1 de sommet ns dans le sens indirect */
/*     ================================================================== */
    if (nt1 > 0) {

	if (noartr[nt1 * noartr_dim1 + 1] == 0) {
	    goto L30;
	}

/*        le triangle nt1 n'a pas ete detruit. il est actif */
/*        le triangle oppose par l'arete noar existe */
/*        le numero des 3 sommets du triangle nt1 dans le sens direct */
L15:
	nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);

/*        reperage du sommet ns dans nt1 */
	for (nar = 1; nar <= 3; ++nar) {
	    if (nosotr[nar - 1] == *ns) {
		goto L25;
	    }
/* L20: */
	}
/*        pas de sommet ns dans le triangle nt1 */
	goto L100;

/*        nt1 est empile */
L25:
	if (*lhpile >= *mxpile) {
	    goto L100;
	}
	++(*lhpile);
	lapile[*lhpile] = nt1;

/*        le triangle nt1 de l'autre cote de l'arete de sommet ns */
/*        sauvegarde du precedent triangle dans nta */
	nta = nt1;
	noar = (i__1 = noartr[nar + nt1 * noartr_dim1], abs(i__1));
	if (nosoar[noar * nosoar_dim1 + 4] == nt1) {
	    nt1 = nosoar[noar * nosoar_dim1 + 5];
	} else if (nosoar[noar * nosoar_dim1 + 5] == nt1) {
	    nt1 = nosoar[noar * nosoar_dim1 + 4];
	} else {
	    //io___208.ciunit = unites_1.imprim;
	    //s_wsle(&io___208);
	    //do_lio(&c__9, &c__1, "trp1st: Anomalie arete", (ftnlen)22);
	    //do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(integer));
	    //do_lio(&c__9, &c__1, " sans triangle", (ftnlen)14);
	    //do_lio(&c__3, &c__1, (char *)&nt1, (ftnlen)sizeof(integer));
	    //e_wsle();
	    goto L100;
	}

/*        le triangle suivant est il a l'exterieur? */
	if (nt1 <= 0) {
	    goto L30;
	}

/*        non: est il le premier triangle de sommet ns? */
	if (nt1 != nt0) {
	    goto L15;
	}

/*        oui: recherche terminee par arrivee sur nt0 */
/*        les triangles forment un "cercle" de "centre" ns */
/*        lhpile ressort avec le signe + */
	return 0;

    }

/*     pas de triangle voisin a nt1 qui doit etre frontalier */
/*     ===================================================== */
/*     le parcours passe par 1 des triangles exterieurs */
/*     le parcours est inverse par l'arete de gauche */
/*     le triangle nta est le premier triangle empile */
L30:
    *lhpile = 1;
    lapile[*lhpile] = nta;

/*     le numero des 3 sommets du triangle nta dans le sens direct */
    nusotr_(&nta, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
	    noartr_offset], nosotr);
    for (nar = 1; nar <= 3; ++nar) {
	if (nosotr[nar - 1] == *ns) {
	    goto L33;
	}
/* L32: */
    }
    goto L100;

/*     l'arete qui precede (rotation / ns dans le sens direct) */
L33:
    if (nar == 1) {
	nar = 3;
    } else {
	--nar;
    }

/*     le triangle voisin de nta dans le sens direct */
    noar = (i__1 = noartr[nar + nta * noartr_dim1], abs(i__1));
    if (nosoar[noar * nosoar_dim1 + 4] == nta) {
	nt1 = nosoar[noar * nosoar_dim1 + 5];
    } else if (nosoar[noar * nosoar_dim1 + 5] == nta) {
	nt1 = nosoar[noar * nosoar_dim1 + 4];
    } else {
	//io___209.ciunit = unites_1.imprim;
	//s_wsle(&io___209);
	//do_lio(&c__9, &c__1, "trp1st: Anomalie arete", (ftnlen)22);
	//do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " SANS triangle", (ftnlen)14);
	//do_lio(&c__3, &c__1, (char *)&nta, (ftnlen)sizeof(integer));
	//e_wsle();
	goto L100;
    }
    if (nt1 <= 0) {
/*        un seul triangle contient ns */
/*        parcours de tous les triangles pour lever le doute */
	goto L100;
    }

/*     boucle sur les triangles de sommet ns dans le sens direct */
/*     ========================================================== */
L40:
    if (noartr[nt1 * noartr_dim1 + 1] == 0) {
	goto L70;
    }

/*     le triangle nt1 n'a pas ete detruit. il est actif */
/*     le numero des 3 sommets du triangle nt1 dans le sens direct */
    nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
	    noartr_offset], nosotr);

/*     reperage du sommet ns dans nt1 */
    for (nar = 1; nar <= 3; ++nar) {
	if (nosotr[nar - 1] == *ns) {
	    goto L60;
	}
/* L50: */
    }
    goto L100;

/*     nt1 est empile */
L60:
    if (*lhpile >= *mxpile) {
	goto L70;
    }
    ++(*lhpile);
    lapile[*lhpile] = nt1;

/*     l'arete qui precede dans le sens direct */
    if (nar == 1) {
	nar = 3;
    } else {
	--nar;
    }

/*     l'arete de sommet ns dans nosoar */
    noar = (i__1 = noartr[nar + nt1 * noartr_dim1], abs(i__1));

/*     le triangle voisin de nta dans le sens direct */
    nta = nt1;
    if (nosoar[noar * nosoar_dim1 + 4] == nt1) {
	nt1 = nosoar[noar * nosoar_dim1 + 5];
    } else if (nosoar[noar * nosoar_dim1 + 5] == nt1) {
	nt1 = nosoar[noar * nosoar_dim1 + 4];
    } else {
	//io___210.ciunit = unites_1.imprim;
	//s_wsle(&io___210);
	//do_lio(&c__9, &c__1, "trp1st: anomalie arete", (ftnlen)22);
	//do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " SANS triangle", (ftnlen)14);
	//do_lio(&c__3, &c__1, (char *)&nt1, (ftnlen)sizeof(integer));
	//e_wsle();
	goto L100;
    }
    if (nt1 > 0) {
	goto L40;
    }

/*     butee sur le trou => fin des triangles de sommet ns */
/*     ---------------------------------------------------- */
/*     impossible ici de trouver tous les triangles de sommet ns directement */
/*     les triangles de sommet ns ne forment pas une boule de centre ns */
/*     au moins 1, voire 2 triangles frontaliers de sommet ns */
L70:
    *lhpile = -(*lhpile);
    return 0;

/*     Balayage de tous les triangles actifs et de sommet ns */
/*     methode lourde et couteuse mais a priori tres fiable */
/*     ----------------------------------------------------- */
L100:
    *lhpile = 0;
    i__1 = *mxartr;
    for (nt1 = 1; nt1 <= i__1; ++nt1) {
	if (noartr[nt1 * noartr_dim1 + 1] != 0) {
/*           le numero des 3 sommets du triangle i */
	    nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		    noartr_offset], nosotr);
	    for (j = 1; j <= 3; ++j) {
		if (nosotr[j - 1] == *ns) {
/*                 le triangle contient le sommet ns */
		    ++(*lhpile);
		    if (*lhpile > *mxpile) {
			goto L9990;
		    }
		    lapile[*lhpile] = nt1;
		}
/* L110: */
	    }
	}
/* L120: */
    }
/*     il n'est pas sur que ces triangles forment une boule de centre ns */
    *lhpile = -(*lhpile);
    return 0;

/*     saturation de la pile des triangles */
/*     ----------------------------------- */
L9990:
 //   io___212.ciunit = unites_1.imprim;
 //   s_wsle(&io___212);
 //   do_lio(&c__9, &c__1, "trp1st: saturation pile des triangles autour du so"
	//    "mmet", (ftnlen)54);
 //   do_lio(&c__3, &c__1, (char *)&(*ns), (ftnlen)sizeof(integer));
 //   e_wsle();
 //   io___213.ciunit = unites_1.imprim;
 //   s_wsle(&io___213);
 //   do_lio(&c__9, &c__1, "Plus de", (ftnlen)7);
 //   do_lio(&c__3, &c__1, (char *)&(*mxpile), (ftnlen)sizeof(integer));
 //   do_lio(&c__9, &c__1, " triangles de sommet", (ftnlen)20);
 //   do_lio(&c__3, &c__1, (char *)&(*ns), (ftnlen)sizeof(integer));
 //   e_wsle();
 //   io___214.ciunit = unites_1.imprim;
 //   s_wsfe(&io___214);
 //   i__1 = *mxpile;
 //   for (ii = 1; ii <= i__1; ++ii) {
	//do_fio(&c__1, (char *)&ii, (ftnlen)sizeof(integer));
	//do_fio(&c__1, (char *)&lapile[ii], (ftnlen)sizeof(integer));
 //   }
 //   e_wsfe();

/* L9999: */
    *lhpile = 0;
    return 0;
} /* trp1st_ */

/* Subroutine */ int nusotr_(integer *nt, integer *mosoar, integer *nosoar, 
	integer *moartr, integer *noartr, integer *nosotr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;

    /* Local variables */
    static integer na;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    calcul du numero des 3 sommets du triangle nt de noartr */
/* -----    dans le sens direct (aire>0 si non degenere) */

/* entrees: */
/* -------- */
/* nt     : numero du triangle dans le tableau noartr */
/* mosoar : nombre maximal d'entiers par arete */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages en + */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1=0 si triangle vide => arete2=triangle vide suivant */

/* sorties: */
/* -------- */
/* nosotr : numero (dans le tableau pxyd) des 3 sommets du triangle */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*     les 2 sommets de l'arete 1 du triangle nt dans le sens direct */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --nosotr;

    /* Function Body */
    na = noartr[*nt * noartr_dim1 + 1];
    if (na > 0) {
	nosotr[1] = 1;
	nosotr[2] = 2;
    } else {
	nosotr[1] = 2;
	nosotr[2] = 1;
	na = -na;
    }
    nosotr[1] = nosoar[nosotr[1] + na * nosoar_dim1];
    nosotr[2] = nosoar[nosotr[2] + na * nosoar_dim1];

/*     l'arete suivante */
    na = (i__1 = noartr[*nt * noartr_dim1 + 2], abs(i__1));

/*     le sommet nosotr(3 du triangle 123 */
    nosotr[3] = nosoar[na * nosoar_dim1 + 1];
    if (nosotr[3] == nosotr[1] || nosotr[3] == nosotr[2]) {
	nosotr[3] = nosoar[na * nosoar_dim1 + 2];
    }
    return 0;
} /* nusotr_ */

/* Subroutine */ int tesusp_(doublereal *quamal, integer *nbarpi, doublereal *
	pxyd, integer *noarst, integer *mosoar, integer *mxsoar, integer *
	n1soar, integer *nosoar, integer *moartr, integer *mxartr, integer *
	n1artr, integer *noartr, integer *mxarcf, integer *n1arcf, integer *
	noarcf, integer *larmin, integer *notrcf, integer *liarcf, integer *
	ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1, 
	    i__2;
    doublereal d__1, d__2;
    static integer equiv_2[3];

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	//    e_wsle(void);

    /* Local variables */
    static real d__;
    static integer i__, j;
    static real d0;
    static integer ns, nt;
#define ns1 (equiv_2)
#define ns2 (equiv_2 + 1)
#define ns3 (equiv_2 + 2)
    static integer nst, nste;
    extern /* Subroutine */ int te1stm_(integer *, integer *, doublereal *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *), qutr2d_(doublereal *,
	     doublereal *, doublereal *, doublereal *), trp1st_(integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer narete, nbtrcf;
    static doublereal quamin, qualit;
    static integer ntqmin, nbsuns;
    static doublereal quaopt;
    static integer nbstsu;
#define nosotr (equiv_2)
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___226 = { 0, 0, 0, 0, 0 };
    static cilist io___238 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   supprimer de la triangulation les sommets de te trop proches */
/* -----   soit d'un sommet frontalier ou point interne impose */
/*         soit d'une arete frontaliere si la qualite minimale des triangles */
/*         est inferieure a quamal */

/*         attention: le chainage lchain de nosoar devient celui des cf */

/* entrees: */
/* -------- */
/* quamal : qualite des triangles au dessous de laquelle supprimer des sommets */
/* nbarpi : numero du dernier point interne impose par l'utilisateur */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxarcf : nombre de variables des tableaux n1arcf, noarcf, larmin, notrcf */

/* modifies: */
/* --------- */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */


/* auxiliaires : */
/* ------------- */
/* n1arcf : tableau (0:mxarcf) auxiliaire d'entiers */
/* noarcf : tableau (3,mxarcf) auxiliaire d'entiers */
/* larmin : tableau ( mxarcf ) auxiliaire d'entiers */
/* notrcf : tableau ( mxarcf ) auxiliaire d'entiers */
/* liarcf : tableau ( mxarcf ) auxiliaire d'entiers */

/* sortie : */
/* -------- */
/* ierr   : =0 si pas d'erreur */
/*          >0 si une erreur est survenue */
/*          11 algorithme defaillant */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */


/*     le nombre de sommets de te supprimes */
    /* Parameter adjustments */
    pxyd -= 4;
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --liarcf;
    --notrcf;
    --larmin;
    noarcf -= 4;

    /* Function Body */
    nbstsu = 0;
    *ierr = 0;

/*     initialisation du chainage des aretes des cf => 0 arete de cf */
    i__1 = *mxsoar;
    for (narete = 1; narete <= i__1; ++narete) {
	nosoar[narete * nosoar_dim1 + 6] = -1;
/* L10: */
    }

/*     boucle sur l'ensemble des sommets frontaliers ou points internes */
/*     ================================================================ */
    i__1 = *nbarpi;
    for (ns = 1; ns <= i__1; ++ns) {

/*        le nombre de sommets supprimes pour ce sommet ns */
	nbsuns = 0;
/*        la qualite minimale au dessous de laquelle le point proche */
/*        interne est supprime */
	quaopt = *quamal;

/*        une arete de sommet ns */
L15:
	narete = noarst[ns];
	if (narete <= 0) {
/*           erreur: le point appartient a aucune arete */
	    //io___226.ciunit = unites_2.imprim;
	    //s_wsle(&io___226);
	    //do_lio(&c__9, &c__1, "sommet ", (ftnlen)7);
	    //do_lio(&c__3, &c__1, (char *)&ns, (ftnlen)sizeof(integer));
	    //do_lio(&c__9, &c__1, " dans aucune arete", (ftnlen)18);
	    //e_wsle();
	    *ierr = 11;
	    return 0;
	}

/*        recherche des triangles de sommet ns */
	trp1st_(&ns, &noarst[1], mosoar, &nosoar[nosoar_offset], moartr, 
		mxartr, &noartr[noartr_offset], mxarcf, &nbtrcf, &notrcf[1]);
	if (nbtrcf == 0) {
	    goto L100;
	}
	if (nbtrcf < 0) {
/*           impossible de trouver tous les triangles de sommet ns */
/*           seule une partie est a priori retrouvee ce qui est normal */
/*           si ns est un sommet frontalier */
	    nbtrcf = -nbtrcf;
	}

/*        boucle sur les triangles de l'etoile du sommet ns */
/*        recherche du triangle de sommet ns ayant la plus basse qualite */
	quamin = 2.;
	i__2 = nbtrcf;
	for (i__ = 1; i__ <= i__2; ++i__) {
/*           le numero des 3 sommets du triangle nt */
	    nt = notrcf[i__];
	    nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		    noartr_offset], nosotr);
/*           nosotr(1:3) est en equivalence avec ns1, ns2, ns3 */
/*           la qualite du triangle ns1 ns2 ns3 */
	    qutr2d_(&pxyd[*ns1 * 3 + 1], &pxyd[*ns2 * 3 + 1], &pxyd[*ns3 * 3 
		    + 1], &qualit);
	    if (qualit < quamin) {
		quamin = qualit;
		ntqmin = nt;
	    }
/* L20: */
	}

/*        bilan sur la qualite des triangles de sommet ns */
	if (quamin < quaopt) {

/*           recherche du sommet de ntqmin le plus proche et non frontalier */
/*           ============================================================== */
/*           le numero des 3 sommets du triangle ntqmin */
	    nusotr_(&ntqmin, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		    noartr_offset], nosotr);
	    nste = 0;
	    d0 = 1e28f;
	    for (j = 1; j <= 3; ++j) {
		nst = nosotr[j - 1];
		if (nst != ns && nst > *nbarpi) {
/* Computing 2nd power */
		    d__1 = pxyd[nst * 3 + 1] - pxyd[ns * 3 + 1];
/* Computing 2nd power */
		    d__2 = pxyd[nst * 3 + 2] - pxyd[ns * 3 + 2];
		    d__ = d__1 * d__1 + d__2 * d__2;
		    if (d__ < d0) {
			d0 = d__;
			nste = j;
		    }
		}
/* L30: */
	    }

	    if (nste > 0) {

/*              nste est le sommet le plus proche de ns de ce */
/*              triangle de mauvaise qualite et sommet non encore traite */
		nste = nosotr[nste - 1];

/*              nste est un sommet de triangle equilateral */
/*              => le sommet nste va etre supprime */
/*              ========================================== */
		te1stm_(&nste, nbarpi, &pxyd[4], &noarst[1], mosoar, mxsoar, 
			n1soar, &nosoar[nosoar_offset], moartr, mxartr, 
			n1artr, &noartr[noartr_offset], mxarcf, n1arcf, &
			noarcf[4], &larmin[1], &notrcf[1], &liarcf[1], ierr);
		if (*ierr == 0) {
/*                 un sommet de te supprime de plus */
		    ++nbstsu;

/*                 boucle jusqu'a obtenir une qualite suffisante */
/*                 si triangulation tres irreguliere => */
/*                 destruction de beaucoup de points internes */
/*                 les 2 variables suivantes brident ces destructions massives */
		    ++nbsuns;
		    quaopt *= .8f;
		    if (nbsuns < 5) {
			goto L15;
		    }
		} else {
		    if (*ierr < 0) {
/*                    le sommet nste est externe donc non supprime */
/*                    ou bien le sommet nste est le centre d'un cf dont toutes */
/*                    les aretes simples sont frontalieres */
/*                    dans les 2 cas le sommet n'est pas supprime */
			*ierr = 0;
			goto L100;
		    } else {
/*                    erreur motivant un arret de la triangulation */
			return 0;
		    }
		}
	    }
	}

L100:
	;
    }

    //io___238.ciunit = unites_2.imprim;
    //s_wsle(&io___238);
    //do_lio(&c__9, &c__1, "tesusp: suppression de", (ftnlen)22);
    //do_lio(&c__3, &c__1, (char *)&nbstsu, (ftnlen)sizeof(integer));
    //do_lio(&c__9, &c__1, " sommets de te trop proches de la frontiere", (
	   // ftnlen)43);
    //e_wsle();
    return 0;
} /* tesusp_ */

#undef nosotr
#undef ns3
#undef ns2
#undef ns1


/* Subroutine */ int teamqa_(integer *nutysu, doublereal *airemx, integer *
	noarst, integer *mosoar, integer *mxsoar, integer *n1soar, integer *
	nosoar, integer *moartr, integer *mxartr, integer *n1artr, integer *
	noartr, integer *mxtrcf, integer *notrcf, integer *nostbo, integer *
	n1arcf, integer *noarcf, integer *larmin, integer *nbarpi, integer *
	nbsomm, integer *mxsomm, doublereal *pxyd, integer *nslign, integer *
	ierr)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset, i__1, 
	    i__2, i__3, i__4, i__5;
    doublereal d__1, d__2;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);
    double sqrt(doublereal);

    /* Local variables */
    static doublereal d__;
    static integer i__, j;
    static doublereal x, y, s0, s1;
    static integer na, ns, nt, ns1, ns2, ns3, ier;
    static doublereal dns;
    static integer nst;
    static doublereal xns, yns;
    static integer nbs1, nbs2, nbs3;
    static doublereal dmin__, dmax__, xbar, ybar;
    static integer imax, noar, iter;
    static doublereal dmoy;
    static integer noar0;
    static doublereal ponde1, xyzns[3];
    extern doublereal surtd2_(doublereal *, doublereal *, doublereal *);
    extern /* Subroutine */ int trp1st_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *), tedela_(doublereal *, integer *, integer *, integer *,
	     integer *, integer *, integer *, integer *, integer *, integer *,
	     integer *, integer *), tr3str_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *), tetaid_(integer *, 
	    doublereal *, doublereal *, doublereal *, integer *);
    static integer nbitaq, nbtrcf;
    static doublereal airetm;
    static integer modifs;
    static doublereal ponder;
    static integer nbstbo, nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___260 = { 0, 0, 0, 0, 0 };
    static cilist io___262 = { 0, 0, 0, 0, 0 };
    static cilist io___274 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but:    Boucles sur les aretes actives de la triangulation actuelle */
/* ----    si la taille de l'arete moyenne est >ampli*taille souhaitee */
/*         alors ajout d'un sommet barycentre du plus grand triangle */
/*               de sommet ns */
/*         si la taille de l'arete moyenne est <ampli/2*taille souhaitee */
/*         alors suppression du sommet ns */
/*         sinon le sommet ns devient le barycentre pondere de ses voisins */

/*         remarque: ampli est defini dans $mefisto/mail/tehote.f */
/*         et doit avoir la meme valeur pour eviter trop de modifications */

/* entrees: */
/* -------- */
/* nutysu : numero de traitement de areteideale() selon le type de surface */
/*          0 pas d'emploi de la fonction areteideale() => aretmx active */
/*          1 il existe une fonction areteideale() */
/*            dont seules les 2 premieres composantes de uv sont actives */
/*          autres options a definir... */
/* airemx : aire maximale d'un triangle */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes frontalieres declarables */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles declarables dans noartr */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/* mxtrcf : nombre maximal de triangles empilables */
/* nbarpi : numero du dernier sommet frontalier ou interne impose */
/* nslign : tableau du numero de sommet dans sa ligne pour chaque */
/*          sommet frontalier */
/*          numero du point dans le lexique point si interne impose */
/*          0 si le point est interne non impose par l'utilisateur */
/*         -1 si le sommet est externe au domaine */

/* modifies : */
/* ---------- */
/* nbsomm : nombre actuel de sommets de la triangulation */
/*          (certains sommets internes ont ete desactives ou ajoutes) */
/* pxyd   : tableau des coordonnees 2d des points */

/* auxiliaires: */
/* ------------ */
/* notrcf : tableau ( mxtrcf ) auxiliaire d'entiers */
/*          numero dans noartr des triangles de sommet ns */
/* nostbo : tableau ( mxtrcf ) auxiliaire d'entiers */
/*          numero dans pxyd des sommets des aretes simples de la boule */
/* n1arcf : tableau (0:mxtrcf) auxiliaire d'entiers */
/* noarcf : tableau (3,mxtrcf) auxiliaire d'entiers */
/* larmin : tableau ( mxtrcf ) auxiliaire d'entiers */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       juin 1997 */
/* ....................................................................012 */

/*     initialisation du chainage des aretes des cf => 0 arete de cf */
    /* Parameter adjustments */
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --larmin;
    noarcf -= 4;
    --notrcf;
    --nostbo;
    pxyd -= 4;
    --nslign;

    /* Function Body */
    i__1 = *mxsoar;
    for (noar = 1; noar <= i__1; ++noar) {
	nosoar[noar * nosoar_dim1 + 6] = -1;
/* L1: */
    }
    noar0 = 0;

/*     le nombre d'iterations pour ameliorer la qualite */
    nbitaq = 5;
    ier = 0;

/*     initialisation du parcours */
    nbs1 = *nbsomm;
    nbs2 = *nbarpi + 1;
    nbs3 = -1;

    i__1 = nbitaq;
    for (iter = 1; iter <= i__1; ++iter) {

/* ccc        le nombre de barycentres ajoutes */
/* cc         nbbaaj = 0 */

/*        coefficient de ponderation croissant avec les iterations */
	ponder = iter * .5 / nbitaq + .1;
/* cc 9 octobre 2006 ponder = min( 1d0, 0.1d0 + iter * 0.9d0 / nbitaq ) */
/* cc 9 mars    2006 ponder = min( 1d0, ( 50 + (50*iter)/nbitaq ) * 0.01d0 ) */
	ponde1 = 1. - ponder;

/*        l'ordre du parcours dans le sens croissant ou decroissant */
/*        alternance du parcours */
	nt = nbs1;
	nbs1 = nbs2;
	nbs2 = nt;
	nbs3 = -nbs3;

	i__2 = nbs2;
	i__3 = nbs3;
	for (ns = nbs1; i__3 < 0 ? ns >= i__2 : ns <= i__2; ns += i__3) {

/*           le sommet est il interne au domaine? */
	    if (nslign[ns] != 0) {
		goto L1000;
	    }

/*           existe-t-il une arete de sommet ns ? */
	    noar = noarst[ns];
	    if (noar <= 0) {
		goto L1000;
	    }
	    if (nosoar[noar * nosoar_dim1 + 1] <= 0) {
		goto L1000;
	    }

/*           le 1-er triangle de l'arete noar */
	    nt = nosoar[noar * nosoar_dim1 + 4];
	    if (nt <= 0) {
		goto L1000;
	    }

/*           recherche des triangles de sommet ns */
/*           ils doivent former un contour ferme de type etoile */
	    trp1st_(&ns, &noarst[1], mosoar, &nosoar[nosoar_offset], moartr, 
		    mxartr, &noartr[noartr_offset], mxtrcf, &nbtrcf, &notrcf[
		    1]);
	    if (nbtrcf <= 0) {
		goto L1000;
	    }

/*           mise a jour de la distance souhaitee autour de ns */
	    xns = pxyd[ns * 3 + 1];
	    yns = pxyd[ns * 3 + 2];
	    if (*nutysu > 0) {
/*              la fonction taille_ideale(x,y,z) existe */
		tetaid_(nutysu, &xns, &yns, &pxyd[ns * 3 + 3], &ier);
	    }

/*           boucle sur les triangles qui forment une etoile autour du sommet ns */
/*           chainage des aretes simples de l'etoile formee par ces triangles */

/*           remise a zero du lien nosoar des aretes a rendre Delaunay */
L19:
	    if (noar0 > 0) {
		noar = nosoar[noar0 * nosoar_dim1 + 6];
		nosoar[noar0 * nosoar_dim1 + 6] = -1;
		noar0 = noar;
		goto L19;
	    }

	    noar0 = 0;
	    nbstbo = 0;
	    airetm = 0.;
	    i__4 = nbtrcf;
	    for (i__ = 1; i__ <= i__4; ++i__) {
/*              recherche du triangle de plus grande aire */
		nt = notrcf[i__];
		nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		d__ = surtd2_(&pxyd[nosotr[0] * 3 + 1], &pxyd[nosotr[1] * 3 + 
			1], &pxyd[nosotr[2] * 3 + 1]);
		if (d__ > airetm) {
		    airetm = d__;
		    imax = i__;
		} else if (d__ <= 0.) {
		    //io___260.ciunit = unites_1.imprim;
		    //s_wsle(&io___260);
		    //do_lio(&c__9, &c__1, "teamqa: triangle notrcf(", (ftnlen)
			   // 24);
		    //do_lio(&c__3, &c__1, (char *)&i__, (ftnlen)sizeof(integer)
			   // );
		    //do_lio(&c__9, &c__1, ")=", (ftnlen)2);
		    //do_lio(&c__3, &c__1, (char *)&notrcf[i__], (ftnlen)sizeof(
			   // integer));
		    //do_lio(&c__9, &c__1, " st", (ftnlen)3);
		    //do_lio(&c__3, &c__3, (char *)&nosotr[0], (ftnlen)sizeof(
			   // integer));
		    //do_lio(&c__9, &c__1, " AIRE=", (ftnlen)6);
		    //do_lio(&c__5, &c__1, (char *)&d__, (ftnlen)sizeof(
			   // doublereal));
		    //do_lio(&c__9, &c__1, "<=0", (ftnlen)3);
		    //e_wsle();
		    goto L1000;
		}

/*              le no de l'arete du triangle nt ne contenant pas le sommet ns */
		for (na = 1; na <= 3; ++na) {
/*                 le numero de l'arete na dans le tableau nosoar */
		    noar = (i__5 = noartr[na + nt * noartr_dim1], abs(i__5));
		    if (nosoar[noar * nosoar_dim1 + 1] != ns && nosoar[noar * 
			    nosoar_dim1 + 2] != ns) {
			goto L25;
		    }
/* L20: */
		}
		//io___262.ciunit = unites_1.imprim;
		//s_wsle(&io___262);
		//do_lio(&c__9, &c__1, "teamqa: ERREUR triangle", (ftnlen)23);
		//do_lio(&c__3, &c__1, (char *)&nt, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " SANS sommet", (ftnlen)12);
		//do_lio(&c__3, &c__1, (char *)&ns, (ftnlen)sizeof(integer));
		//e_wsle();

/*              construction de la liste des sommets des aretes simples */
/*              de la boule des triangles de sommet ns */
/*              ------------------------------------------------------- */
L25:
		for (na = 1; na <= 2; ++na) {
		    ns1 = nosoar[na + noar * nosoar_dim1];
		    for (j = nbstbo; j >= 1; --j) {
			if (ns1 == nostbo[j]) {
			    goto L35;
			}
/* L30: */
		    }
/*                 ns1 est un nouveau sommet a ajouter a l'etoile */
		    ++nbstbo;
		    nostbo[nbstbo] = ns1;
L35:
		    ;
		}

/*              noar est une arete potentielle a rendre Delaunay */
		if (nosoar[noar * nosoar_dim1 + 3] == 0) {
/*                 arete non frontaliere */
		    nosoar[noar * nosoar_dim1 + 6] = noar0;
		    noar0 = noar;
		}

/* L40: */
	    }

/*           calcul des 2 coordonnees du barycentre de la boule du sommet ns */
/*           calcul de la longueur moyenne des aretes issues du sommet ns */
/*           --------------------------------------------------------------- */
	    xbar = 0.;
	    ybar = 0.;
	    dmoy = 0.;
	    dmax__ = 0.;
	    dmin__ = 1e124;
	    dns = 0.;
	    i__4 = nbstbo;
	    for (i__ = 1; i__ <= i__4; ++i__) {
		nst = nostbo[i__];
		x = pxyd[nst * 3 + 1];
		y = pxyd[nst * 3 + 2];
		xbar += x;
		ybar += y;
/* Computing 2nd power */
		d__1 = x - xns;
/* Computing 2nd power */
		d__2 = y - yns;
		d__ = sqrt(d__1 * d__1 + d__2 * d__2);
		dmoy += d__;
		dmax__ = max(dmax__,d__);
		dmin__ = min(dmin__,d__);
		dns += pxyd[nst * 3 + 3];
/* L50: */
	    }
	    xbar /= nbstbo;
	    ybar /= nbstbo;
	    dmoy /= nbstbo;
	    dns /= nbstbo;

/*           pas de modification de la topologie lors de la derniere iteration */
/*           ================================================================= */
	    if (iter == nbitaq) {
		goto L200;
	    }

/*           si la taille de l'arete maximale est >ampli*taille souhaitee */
/*           alors ajout d'un sommet barycentre du plus grand triangle */
/*                 de sommet ns */
/*           ============================================================ */
	    if (airetm > *airemx || dmax__ > dns * 1.34) {

/*              ajout du barycentre du triangle notrcf(imax) */
		nt = notrcf[imax];
		nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		if (*nbsomm >= *mxsomm) {
		    //io___274.ciunit = unites_1.imprim;
		    //s_wsle(&io___274);
		    //do_lio(&c__9, &c__1, "saturation du tableau pxyd", (
			   // ftnlen)26);
		    //e_wsle();
/*                 abandon de l'amelioration du sommet ns */
		    goto L9999;
		}
		++(*nbsomm);
		for (i__ = 1; i__ <= 3; ++i__) {
		    pxyd[i__ + *nbsomm * 3] = (pxyd[i__ + nosotr[0] * 3] + 
			    pxyd[i__ + nosotr[1] * 3] + pxyd[i__ + nosotr[2] *
			     3]) / 3.;
/* L160: */
		}
		if (*nutysu > 0) {
/*                 la fonction taille_ideale(x,y,z) existe */
		    tetaid_(nutysu, &pxyd[*nbsomm * 3 + 1], &pxyd[*nbsomm * 3 
			    + 2], &pxyd[*nbsomm * 3 + 3], &ier);
		}

/*              sommet interne a la triangulation */
		nslign[*nbsomm] = 0;

/*              les 3 aretes du triangle nt sont a rendre delaunay */
		for (i__ = 1; i__ <= 3; ++i__) {
		    noar = (i__4 = noartr[i__ + nt * noartr_dim1], abs(i__4));
		    if (nosoar[noar * nosoar_dim1 + 3] == 0) {
/*                    arete non frontaliere */
			if (nosoar[noar * nosoar_dim1 + 6] < 0) {
/*                       arete non encore chainee */
			    nosoar[noar * nosoar_dim1 + 6] = noar0;
			    noar0 = noar;
			}
		    }
/* L170: */
		}

/*              triangulation du triangle de barycentre nbsomm */
/*              protection a ne pas modifier sinon erreur! */
		tr3str_(nbsomm, &nt, mosoar, mxsoar, n1soar, &nosoar[
			nosoar_offset], moartr, mxartr, n1artr, &noartr[
			noartr_offset], &noarst[1], nosotr, ierr);
		if (*ierr != 0) {
		    goto L9999;
		}

/* ccc              un barycentre ajoute de plus */
/* cc               nbbaaj = nbbaaj + 1 */

/*              les aretes chainees de la boule sont rendues delaunay */
		goto L900;

	    }

/*           les 2 coordonnees du barycentre des sommets des aretes */
/*           simples de la boule du sommet ns */
/*           ====================================================== */
/* DEBUT AJOUT 10 octobre 2006 */
/*           PONDERATION POUR EVITER LES DEGENERESCENSES AVEC PROTECTION */
/*           SI UN TRIANGLE DE SOMMET NS A UNE AIRE NEGATIVE APRES BARYCENTRAGE */
/*           ALORS LE SOMMET NS N'EST PAS BOUGE */

/*           protection des XY du point initial */
L200:
	    xyzns[0] = pxyd[ns * 3 + 1];
	    xyzns[1] = pxyd[ns * 3 + 2];
	    xyzns[2] = pxyd[ns * 3 + 3];

/*           ponderation pour eviter les degenerescenses */
	    pxyd[ns * 3 + 1] = ponde1 * pxyd[ns * 3 + 1] + ponder * xbar;
	    pxyd[ns * 3 + 2] = ponde1 * pxyd[ns * 3 + 2] + ponder * ybar;
	    if (*nutysu > 0) {
/*              la fonction taille_ideale(x,y,z) existe */
		tetaid_(nutysu, &pxyd[ns * 3 + 1], &pxyd[ns * 3 + 2], &pxyd[
			ns * 3 + 3], &ier);
	    }

/*           calcul des surfaces avant et apres deplacement de ns */
	    s0 = 0.;
	    s1 = 0.;
	    i__4 = nbtrcf;
	    for (i__ = 1; i__ <= i__4; ++i__) {
/*              le numero de l'arete du triangle nt ne contenant pas le sommet ns */
		nt = notrcf[i__];
		for (na = 1; na <= 3; ++na) {
/*                 le numero de l'arete na dans le tableau nosoar */
		    noar = (i__5 = noartr[na + nt * noartr_dim1], abs(i__5));
		    if (nosoar[noar * nosoar_dim1 + 1] != ns && nosoar[noar * 
			    nosoar_dim1 + 2] != ns) {
			ns2 = nosoar[noar * nosoar_dim1 + 1];
			ns3 = nosoar[noar * nosoar_dim1 + 2];
			goto L206;
		    }
/* L204: */
		}
/*              aire signee des 2 triangles */
L206:
		s0 += (d__1 = surtd2_(xyzns, &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 
			3 + 1]), abs(d__1));
		s1 += (d__1 = surtd2_(&pxyd[ns * 3 + 1], &pxyd[ns2 * 3 + 1], &
			pxyd[ns3 * 3 + 1]), abs(d__1));
/* L210: */
	    }
	    if ((d__1 = s0 - s1, abs(d__1)) > abs(s0) * 1e-10) {
/*              retour a la position initiale */
/*              car le point est passe au dela d'une arete de son etoile */
		pxyd[ns * 3 + 1] = xyzns[0];
		pxyd[ns * 3 + 2] = xyzns[1];
		pxyd[ns * 3 + 3] = xyzns[2];
/*              la ponderation est reduite  10 octobre 2006 */
/* Computing MAX */
		d__1 = .1, d__2 = ponder * .5;
		ponder = max(d__1,d__2);
		ponde1 = 1. - ponder;
		goto L1000;
	    }

/*           les aretes chainees de la boule sont rendues delaunay */
L900:
	    tedela_(&pxyd[4], &noarst[1], mosoar, mxsoar, n1soar, &nosoar[
		    nosoar_offset], &noar0, moartr, mxartr, n1artr, &noartr[
		    noartr_offset], &modifs);

L1000:
	    ;
	}

/* cc         write(imprim,11000) iter, nbbaaj */
/* cc11000 format('teamqa: iteration',i3,' =>',i6,' barycentres ajoutes') */

/*        mise a jour pour ne pas oublier les nouveaux sommets */
	if (nbs1 > nbs2) {
	    nbs1 = *nbsomm;
	} else {
	    nbs2 = *nbsomm;
	}

/* L5000: */
    }

L9999:
    return 0;
} /* teamqa_ */

/* Subroutine */ int teamqt_(integer *nutysu, doublereal *aretmx, doublereal *
	airemx, integer *noarst, integer *mosoar, integer *mxsoar, integer *
	n1soar, integer *nosoar, integer *moartr, integer *mxartr, integer *
	n1artr, integer *noartr, integer *mxarcf, integer *notrcf, integer *
	nostbo, integer *n1arcf, integer *noarcf, integer *larmin, integer *
	nbarpi, integer *nbsomm, integer *mxsomm, doublereal *pxyd, integer *
	nslign, integer *ierr)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset;

    /* Local variables */
    extern /* Subroutine */ int qualitetrte_(doublereal *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *, doublereal *), teamqa_(integer *, doublereal *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *, integer *, integer *);
    static integer nbtria;
    static doublereal quamin, quamoy;
    extern /* Subroutine */ int tesuqm_(doublereal *, integer *, doublereal *,
	     integer *, integer *, integer *, integer *, integer *, integer *,
	     integer *, integer *, integer *, integer *, integer *, integer *,
	     integer *, integer *, integer *, doublereal *), tesusp_(
	    doublereal *, integer *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    amelioration de la qualite de la triangulation */
/* ----- */

/* entrees: */
/* -------- */
/* nutysu : numero de traitement de areteideale() selon le type de surface */
/*          0 pas d'emploi de la fonction areteideale() => aretmx active */
/*          1 il existe une fonction areteideale() */
/*            dont seules les 2 premieres composantes de uv sont actives */
/*          autres options a definir... */
/* aretmx : longueur maximale des aretes de la future triangulation */
/* airemx : aire maximale souhaitee des triangles */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes frontalieres declarables */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles declarables dans noartr */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/* mxarcf : nombre maximal de triangles empilables */
/* nbarpi : numero du dernier sommet frontalier ou interne impose */
/* nslign : tableau du numero de sommet dans sa ligne pour chaque */
/*          sommet frontalier */
/*          numero du point dans le lexique point si interne impose */
/*          0 si le point est interne non impose par l'utilisateur */
/*         -1 si le sommet est externe au domaine */

/* modifies : */
/* ---------- */
/* nbsomm : nombre actuel de sommets de la triangulation */
/*          (certains sommets internes ont ete desactives ou ajoutes) */
/* pxyd   : tableau des coordonnees 2d des points */

/* auxiliaires: */
/* ------------ */
/* notrcf : tableau ( mxarcf ) auxiliaire d'entiers */
/*          numero dans noartr des triangles de sommet ns */
/* nostbo : tableau ( mxarcf ) auxiliaire d'entiers */
/*          numero dans pxyd des sommets des aretes simples de la boule */
/* n1arcf : tableau (0:mxarcf) auxiliaire d'entiers */
/* noarcf : tableau (3,mxarcf) auxiliaire d'entiers */
/* larmin : tableau ( mxarcf ) auxiliaire d'entiers */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       juin 1997 */
/* ....................................................................012 */
/*     parameter       ( quamal=0.3d0 ) => ok */
/*     parameter       ( quamal=0.4d0 ) => pb pour le test ocean */
/*     parameter       ( quamal=0.5d0 ) => pb pour le test ocean */
/*     quamal=0.1d0 est choisi pour ne pas trop detruire de sommets */


    /* Parameter adjustments */
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --larmin;
    noarcf -= 4;
    --nostbo;
    --notrcf;
    pxyd -= 4;
    --nslign;

    /* Function Body */
    *ierr = 0;

/*     supprimer de la triangulation les triangles de qualite */
/*     inferieure a quamal */
/*     ====================================================== */
    tesuqm_(&c_b357, nbarpi, &pxyd[4], &noarst[1], mosoar, mxsoar, n1soar, &
	    nosoar[nosoar_offset], moartr, mxartr, n1artr, &noartr[
	    noartr_offset], mxarcf, n1arcf, &noarcf[4], &larmin[1], &notrcf[1]
	    , &nostbo[1], &quamin);
    qualitetrte_(&pxyd[4], mosoar, mxsoar, &nosoar[nosoar_offset], moartr, 
	    mxartr, &noartr[noartr_offset], &nbtria, &quamoy, &quamin);

/*     suppression des sommets de triangles equilateraux trop proches */
/*     d'un sommet frontalier ou d'un point interne impose par */
/*     triangulation frontale de l'etoile et mise en delaunay */
/*     ============================================================== */
    if (quamin <= .1) {
	tesusp_(&c_b357, nbarpi, &pxyd[4], &noarst[1], mosoar, mxsoar, n1soar,
		 &nosoar[nosoar_offset], moartr, mxartr, n1artr, &noartr[
		noartr_offset], mxarcf, n1arcf, &noarcf[4], &larmin[1], &
		notrcf[1], &nostbo[1], ierr);
	if (*ierr != 0) {
	    goto L9999;
	}
    }

/*     ajustage des tailles moyennes des aretes avec ampli=1.34d0 entre */
/*     ampli/2 x taille_souhaitee et ampli x taille_souhaitee */
/*     + barycentrage des sommets et mise en triangulation delaunay */
/*     ================================================================ */
    teamqa_(nutysu, airemx, &noarst[1], mosoar, mxsoar, n1soar, &nosoar[
	    nosoar_offset], moartr, mxartr, n1artr, &noartr[noartr_offset], 
	    mxarcf, &notrcf[1], &nostbo[1], n1arcf, &noarcf[4], &larmin[1], 
	    nbarpi, nbsomm, mxsomm, &pxyd[4], &nslign[1], ierr);
    qualitetrte_(&pxyd[4], mosoar, mxsoar, &nosoar[nosoar_offset], moartr, 
	    mxartr, &noartr[noartr_offset], &nbtria, &quamoy, &quamin);
    if (*ierr != 0) {
	goto L9999;
    }

L9999:
    return 0;
} /* teamqt_ */

/* Subroutine */ int trfrcf_(integer *nscent, integer *mosoar, integer *
	nosoar, integer *moartr, integer *noartr, integer *nbtrcf, integer *
	notrcf, integer *nbarfr)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset, i__1, 
	    i__2;

    /* Local variables */
    static integer i__, j, n, ns, nt, noar;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    calculer le nombre d'aretes simples du contour ferme des */
/* -----    nbtrcf triangles de numeros stockes dans le tableau notrcf */
/*          ayant tous le sommet nscent */

/* entrees: */
/* -------- */
/* nscent : numero du sommet appartenant a tous les triangles notrcf */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/* nbtrcf : >0 nombre de triangles empiles */
/*          =0       si impossible de tourner autour du point */
/*          =-nbtrcf si apres butee sur la frontiere il y a a nouveau */
/*          butee sur la frontiere . a ce stade on ne peut dire si tous */
/*          les triangles ayant ce sommet ont ete recenses */
/*          ce cas arrive seulement si le sommet est sur la frontiere */
/* notrcf : numero dans noartr des triangles de sommet ns */

/* sortie : */
/* -------- */
/* nbarfr : nombre d'aretes simples frontalieres */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       juin 1997 */
/* ....................................................................012 */

    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --notrcf;

    /* Function Body */
    *nbarfr = 0;
    i__1 = *nbtrcf;
    for (n = 1; n <= i__1; ++n) {
/*        le numero du triangle n dans le tableau noartr */
	nt = notrcf[n];
/*        parcours des 3 aretes du triangle nt */
	for (i__ = 1; i__ <= 3; ++i__) {
/*           le numero de l'arete i dans le tableau nosoar */
	    noar = (i__2 = noartr[i__ + nt * noartr_dim1], abs(i__2));
	    for (j = 1; j <= 2; ++j) {
/*              le numero du sommet j de l'arete noar */
		ns = nosoar[j + noar * nosoar_dim1];
		if (ns == *nscent) {
		    goto L40;
		}
/* L30: */
	    }
/*           l'arete noar (sans sommet nscent) est elle frontaliere? */
	    if (nosoar[noar * nosoar_dim1 + 5] <= 0) {
/*              l'arete appartient au plus a un triangle */
/*              une arete simple frontaliere de plus */
		++(*nbarfr);
	    }
/*           le triangle a au plus une arete sans sommet nscent */
	    goto L50;
L40:
	    ;
	}
L50:
	;
    }
    return 0;
} /* trfrcf_ */

/* Subroutine */ int int2ar_(doublereal *p1, doublereal *p2, doublereal *p3, 
	doublereal *p4, logical *oui)
{
    /* System generated locals */
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal d__, x, y, d21, d43, x21, y21, x43, y43, xx;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    les 2 aretes de r**2 p1-p2  p3-p4 s'intersectent elles */
/* -----    entre leurs sommets? */

/* entrees: */
/* -------- */
/* p1,p2,p3,p4 : les 2 coordonnees reelles des sommets des 2 aretes */

/* sortie : */
/* -------- */
/* oui    : .true. si intersection, .false. sinon */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    octobre 1991 */
/* 2345x7..............................................................012 */

/*     longueur des aretes */
    /* Parameter adjustments */
    --p4;
    --p3;
    --p2;
    --p1;

    /* Function Body */
    x21 = p2[1] - p1[1];
    y21 = p2[2] - p1[2];
/* Computing 2nd power */
    d__1 = x21;
/* Computing 2nd power */
    d__2 = y21;
    d21 = d__1 * d__1 + d__2 * d__2;

    x43 = p4[1] - p3[1];
    y43 = p4[2] - p3[2];
/* Computing 2nd power */
    d__1 = x43;
/* Computing 2nd power */
    d__2 = y43;
    d43 = d__1 * d__1 + d__2 * d__2;

/*     les 2 aretes sont-elles jugees paralleles ? */
    d__ = x43 * y21 - y43 * x21;
    if (abs(d__) <= sqrt(d21 * d43) * .001f) {
/*        aretes paralleles . pas d'intersection */
	*oui = FALSE_;
	return 0;
    }

/*     les 2 coordonnees du point d'intersection */
    x = (p1[1] * x43 * y21 - p3[1] * x21 * y43 - (p1[2] - p3[2]) * x21 * x43) 
	    / d__;
    y = -(p1[2] * y43 * x21 - p3[2] * y21 * x43 - (p1[1] - p3[1]) * y21 * y43)
	     / d__;

/*     coordonnees de x,y dans le repere ns1-ns2 */
    xx = (x - p1[1]) * x21 + (y - p1[2]) * y21;
/*     le point est il entre p1 et p2 ? */
    *oui = d21 * -1e-5 <= xx && xx <= d21 * 1.00001;

/*     coordonnees de x,y dans le repere ns3-ns4 */
    xx = (x - p3[1]) * x43 + (y - p3[2]) * y43;
/*     le point est il entre p3 et p4 ? */
    *oui = *oui && d43 * -1e-5 <= xx && xx <= d43 * 1.00001;
    return 0;
} /* int2ar_ */

/* Subroutine */ int trchtd_(doublereal *pxyd, integer *nar00, integer *nar0, 
	integer *noarcf, integer *namin0, integer *namin, integer *larmin)
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1, d__2;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j;
    static doublereal q, dd;
    static integer na0, na1, np1, np2, ns1, ns2, ns3, ns4, na00, ier, nar;
    static logical oui;
    static integer nar1, nar2, nar3, nbar;
    static doublereal dmima;
    static integer nbmin;
    static real qmima;
    static doublereal rayon;
    extern /* Subroutine */ int int2ar_(doublereal *, doublereal *, 
	    doublereal *, doublereal *, logical *), qutr2d_(doublereal *, 
	    doublereal *, doublereal *, doublereal *), cenced_(doublereal *, 
	    doublereal *, doublereal *, doublereal *, integer *);
    extern doublereal surtd2_(doublereal *, doublereal *, doublereal *);
    static doublereal centre[3], unpeps;

    /* Fortran I/O blocks */
    static cilist io___310 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    recherche dans le contour ferme du sommet qui joint a la plus */
/* -----    courte arete nar00 donne le triangle sans intersection */
/*          avec le contour ferme de meilleure qualite */

/* entrees: */
/* -------- */
/* pxyd   : tableau des coordonnees des sommets et distance_souhaitee */

/* entrees et sorties: */
/* ------------------- */
/* nar00  : numero dans noarcf de l'arete avant nar0 */
/* nar0   : numero dans noarcf de la plus petite arete du contour ferme */
/*          a joindre a noarcf(1,namin) pour former le triangle ideal */
/* noarcf : numero du sommet , numero de l'arete suivante */
/*          numero du triangle exterieur a l'etoile */

/* sortie : */
/* -------- */
/* namin0 : numero dans noarcf de l'arete avant namin */
/* namin  : numero dans noarcf du sommet choisi */
/*          0 si contour ferme reduit a moins de 3 aretes */
/* larmin : tableau auxiliaire pour stocker la liste des numeros des */
/*          aretes de meilleure qualite pour faire le choix final */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */
/*     ATTENTION:variables a ajuster selon la machine! */
/*     ATTENTION:dmaxim : le plus grand reel machine */
/*     ATTENTION:sur dec-alpha la precision est de 10**-14 seulement */

/*     initialisations */
/*     dmaxim : le plus grand reel machine */
    /* Parameter adjustments */
    --larmin;
    noarcf -= 4;
    pxyd -= 4;

    /* Function Body */
    unpeps = 1.00000000000001;

/*     recherche de la plus courte arete du contour ferme */
    nbmin = 0;
    na00 = *nar00;
    dmima = 1.7e308;
    nbar = 0;

L2:
    na0 = noarcf[na00 * 3 + 2];
    na1 = noarcf[na0 * 3 + 2];
    ++nbar;
/*     les 2 sommets de l'arete na0 du cf */
    ns1 = noarcf[na0 * 3 + 1];
    ns2 = noarcf[na1 * 3 + 1];
/* Computing 2nd power */
    d__1 = pxyd[ns2 * 3 + 1] - pxyd[ns1 * 3 + 1];
/* Computing 2nd power */
    d__2 = pxyd[ns2 * 3 + 2] - pxyd[ns1 * 3 + 2];
    dd = d__1 * d__1 + d__2 * d__2;
    if (dd < dmima) {
	dmima = dd;
	larmin[1] = na00;
    }
    na00 = na0;
    if (na00 != *nar00) {
/*        derniere arete non atteinte */
	goto L2;
    }

    if (nbar == 3) {

/*        contour ferme reduit a un triangle */
/*        ---------------------------------- */
	*namin = *nar00;
	*nar0 = noarcf[*nar00 * 3 + 2];
	*namin0 = noarcf[*nar0 * 3 + 2];
	return 0;

    } else if (nbar <= 2) {
	//io___310.ciunit = unites_1.imprim;
	//s_wsle(&io___310);
	//do_lio(&c__9, &c__1, "erreur trchtd: cf<3 aretes", (ftnlen)26);
	//e_wsle();
	*namin = 0;
	*namin0 = 0;
	return 0;
    }

/*     cf non reduit a un triangle */
/*     la plus petite arete est nar0 dans noarcf */
    *nar00 = larmin[1];
    *nar0 = noarcf[*nar00 * 3 + 2];
    nar = noarcf[*nar0 * 3 + 2];

    ns1 = noarcf[*nar0 * 3 + 1];
    ns2 = noarcf[nar * 3 + 1];

/*     recherche dans cette etoile du sommet offrant la meilleure qualite */
/*     du triangle ns1-ns2 ns3 sans intersection avec le contour ferme */
/*     ================================================================== */
    nar3 = nar;
    qmima = -1.f;

/*     parcours des sommets possibles ns3 */
L10:
    nar3 = noarcf[nar3 * 3 + 2];
    if (nar3 != *nar0) {

/*        il existe un sommet ns3 different de ns1 et ns2 */
	ns3 = noarcf[nar3 * 3 + 1];

/*        les aretes ns1-ns3 et ns2-ns3 intersectent-elles une arete */
/*        du contour ferme ? */
/*        ---------------------------------------------------------- */
/*        intersection de l'arete ns2-ns3 et des aretes du cf */
/*        jusqu'au sommet ns3 */
	nar1 = noarcf[nar * 3 + 2];

L15:
	if (nar1 != nar3 && noarcf[nar1 * 3 + 2] != nar3) {
/*           l'arete suivante */
	    nar2 = noarcf[nar1 * 3 + 2];
/*           le numero des 2 sommets de l'arete */
	    np1 = noarcf[nar1 * 3 + 1];
	    np2 = noarcf[nar2 * 3 + 1];
	    int2ar_(&pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 1], &pxyd[np1 * 3 + 1]
		    , &pxyd[np2 * 3 + 1], &oui);
	    if (oui) {
		goto L10;
	    }
/*           les 2 aretes ne s'intersectent pas entre leurs sommets */
	    nar1 = nar2;
	    goto L15;
	}

/*        intersection de l'arete ns3-ns1 et des aretes du cf */
/*        jusqu'au sommet de l'arete nar0 */
	nar1 = noarcf[nar3 * 3 + 2];

L18:
	if (nar1 != *nar0 && noarcf[nar1 * 3 + 2] != *nar0) {
/*           l'arete suivante */
	    nar2 = noarcf[nar1 * 3 + 2];
/*           le numero des 2 sommets de l'arete */
	    np1 = noarcf[nar1 * 3 + 1];
	    np2 = noarcf[nar2 * 3 + 1];
	    int2ar_(&pxyd[ns1 * 3 + 1], &pxyd[ns3 * 3 + 1], &pxyd[np1 * 3 + 1]
		    , &pxyd[np2 * 3 + 1], &oui);
	    if (oui) {
		goto L10;
	    }
/*           les 2 aretes ne s'intersectent pas entre leurs sommets */
	    nar1 = nar2;
	    goto L18;
	}

/*        le triangle ns1-ns2-ns3 n'intersecte pas une arete du contour ferme */
/*        le calcul de la surface du triangle */
	dd = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 
		1]);
	if (dd <= 0.) {
/*           surface negative => triangle a rejeter */
	    q = 0.;
	} else {
/*           calcul de la qualite du  triangle  ns1-ns2-ns3 */
	    qutr2d_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 1]
		    , &q);
	}

	if (q >= qmima * 1.00001f) {
/*           q est un vrai maximum de la qualite */
	    qmima = q;
	    nbmin = 1;
	    larmin[1] = nar3;
	} else if (q >= qmima * .999998f) {
/*           q est voisin de qmima */
/*           il est empile */
	    ++nbmin;
	    larmin[nbmin] = nar3;
	}
	goto L10;
    }

/*     bilan : existe t il plusieurs sommets de meme qualite? */
/*     ====================================================== */
    if (nbmin > 1) {

/*        oui:recherche de ceux de cercle ne contenant pas d'autres sommets */
	i__1 = nbmin;
	for (i__ = 1; i__ <= i__1; ++i__) {
/*           le sommet */
	    nar = larmin[i__];
	    if (nar <= 0) {
		goto L80;
	    }
	    ns3 = noarcf[nar * 3 + 1];
/*           les coordonnees du centre du cercle circonscrit */
/*           et son rayon */
	    ier = -1;
	    cenced_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 1]
		    , centre, &ier);
	    if (ier != 0) {
/*              le sommet ns3 ne convient pas */
		larmin[i__] = 0;
		goto L80;
	    }
	    rayon = centre[2] * unpeps;
	    i__2 = nbmin;
	    for (j = 1; j <= i__2; ++j) {
		if (j != i__) {
/*                 l'autre sommet */
		    nar1 = larmin[j];
		    if (nar1 <= 0) {
			goto L70;
		    }
		    ns4 = noarcf[nar1 * 3 + 1];
/*                 appartient t il au cercle ns1 ns2 ns3 ? */
/* Computing 2nd power */
		    d__1 = centre[0] - pxyd[ns4 * 3 + 1];
/* Computing 2nd power */
		    d__2 = centre[1] - pxyd[ns4 * 3 + 2];
		    dd = d__1 * d__1 + d__2 * d__2;
		    if (dd <= rayon) {
/*                    ns4 est dans le cercle circonscrit  ns1 ns2 ns3 */
/*                    le sommet ns3 ne convient pas */
			larmin[i__] = 0;
			goto L80;
		    }
		}
L70:
		;
	    }
L80:
	    ;
	}

/*        existe t il plusieurs sommets ? */
	j = 0;
	i__1 = nbmin;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    if (larmin[i__] > 0) {
/*              compactage des min */
		++j;
		larmin[j] = larmin[i__];
	    }
/* L90: */
	}

	if (j > 1) {
/*           oui : choix du plus petit rayon de cercle circonscrit */
	    dmima = 1.7e308;
	    i__1 = nbmin;
	    for (i__ = 1; i__ <= i__1; ++i__) {
		ns3 = noarcf[larmin[i__] * 3 + 1];

/*              les coordonnees du centre de cercle circonscrit */
/*              au triangle nt et son rayon */
		ier = -1;
		cenced_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 
			+ 1], centre, &ier);
		if (ier != 0) {
/*                 le sommet ns3 ne convient pas */
		    goto L120;
		}
		rayon = sqrt(centre[2]);
		if (rayon < dmima) {
		    dmima = rayon;
		    larmin[1] = larmin[i__];
		}
L120:
		;
	    }
	}
    }

/*     le choix final */
/*     ============== */
    *namin = larmin[1];

/*     recherche de l'arete avant namin ( nar0 <> namin ) */
/*     ================================================== */
    nar1 = *nar0;
L200:
    if (nar1 != *namin) {
	*namin0 = nar1;
	nar1 = noarcf[nar1 * 3 + 2];
	goto L200;
    }
    return 0;
} /* trchtd_ */

/* Subroutine */ int trcf0a_(integer *nbcf, integer *na01, integer *na1, 
	integer *na2, integer *na3, integer *noar1, integer *noar2, integer *
	noar3, integer *mosoar, integer *mxsoar, integer *n1soar, integer *
	nosoar, integer *moartr, integer *n1artr, integer *noartr, integer *
	noarst, integer *mxarcf, integer *n1arcf, integer *noarcf, integer *
	nt)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer nav, na2s, na3s, nav1, ierr;
    extern /* Subroutine */ int trcf3a_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer noar2s, noar3s;
    extern /* Subroutine */ int fasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___334 = { 0, 0, 0, 0, 0 };
    static cilist io___335 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    modification de la triangulation du contour ferme nbcf */
/* -----    par ajout d'un triangle ayant 0 arete sur le contour */
/*          creation des 3 aretes dans le tableau nosoar */
/*          modification du contour par ajout de la 3-eme arete */
/*          creation d'un contour ferme a partir de la seconde arete */

/* entrees: */
/* -------- */
/* nbcf    : numero dans n1arcf du cf traite ici */
/* na01    : numero noarcf de l'arete precedent l'arete na1 de noarcf */
/* na1     : numero noarcf du 1-er sommet du triangle */
/*           implicitement l'arete na1 n'est pas une arete du triangle */
/* na2     : numero noarcf du 2-eme sommet du triangle */
/*           implicitement l'arete na1 n'est pas une arete du triangle */
/* na3     : numero noarcf du 3-eme sommet du triangle */
/*           implicitement l'arete na1 n'est pas une arete du triangle */

/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */

/* entrees et sorties : */
/* -------------------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1arcf : numero d'une arete de chaque contour */
/* noarcf : numero des aretes de la ligne du contour ferme */
/*          attention : chainage circulaire des aretes */

/* sortie : */
/* -------- */
/* noar1  : numero dans le tableau nosoar de l'arete 1 du triangle */
/* noar2  : numero dans le tableau nosoar de l'arete 2 du triangle */
/* noar3  : numero dans le tableau nosoar de l'arete 3 du triangle */
/* nt     : numero du triangle ajoute dans noartr */
/*          0 si saturation du tableau noartr ou noarcf ou n1arcf */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    noarcf -= 4;

    /* Function Body */
    ierr = 0;

/*     2 contours fermes peuvent ils etre ajoutes ? */
    if (*nbcf + 2 > *mxarcf) {
	goto L9100;
    }

/*     creation des 3 aretes du triangle dans le tableau nosoar */
/*     ======================================================== */
/*     la formation de l'arete sommet1-sommet2 dans le tableau nosoar */
    fasoar_(&noarcf[*na1 * 3 + 1], &noarcf[*na2 * 3 + 1], &c_n1, &c_n1, &c__0,
	     mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], 
	    noar1, &ierr);
    if (ierr != 0) {
	goto L9900;
    }

/*     la formation de l'arete sommet2-sommet3 dans le tableau nosoar */
    fasoar_(&noarcf[*na2 * 3 + 1], &noarcf[*na3 * 3 + 1], &c_n1, &c_n1, &c__0,
	     mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], 
	    noar2, &ierr);
    if (ierr != 0) {
	goto L9900;
    }

/*     la formation de l'arete sommet3-sommet1 dans le tableau nosoar */
    fasoar_(&noarcf[*na3 * 3 + 1], &noarcf[*na1 * 3 + 1], &c_n1, &c_n1, &c__0,
	     mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], 
	    noar3, &ierr);
    if (ierr != 0) {
	goto L9900;
    }

/*     ajout dans noartr de ce triangle nt */
/*     =================================== */
    trcf3a_(&noarcf[*na1 * 3 + 1], &noarcf[*na2 * 3 + 1], &noarcf[*na3 * 3 + 
	    1], noar1, noar2, noar3, mosoar, &nosoar[nosoar_offset], moartr, 
	    n1artr, &noartr[noartr_offset], nt);
    if (*nt <= 0) {
	return 0;
    }

/*     modification du contour nbcf existant */
/*     chainage de l'arete na2 vers l'arete na1 */
/*     ======================================== */
/*     modification du cf en pointant na2 sur na1 */
    na2s = noarcf[*na2 * 3 + 2];
    noarcf[*na2 * 3 + 2] = *na1;
/*     le numero de l'arete dans le tableau nosoar */
    noar2s = noarcf[*na2 * 3 + 3];
/*     le numero de l'arete dans le tableau nosoar */
    noarcf[*na2 * 3 + 3] = *noar1;
/*     debut du cf */
    n1arcf[*nbcf] = *na2;

/*     creation d'un nouveau contour ferme na2 - na3 */
/*     ============================================= */
    ++(*nbcf);
/*     recherche d'une arete de cf vide */
    nav = n1arcf[0];
    if (nav <= 0) {
	goto L9100;
    }
/*     la 1-ere arete vide est mise a jour */
    n1arcf[0] = noarcf[nav * 3 + 2];

/*     ajout de l'arete nav pointant sur na2s */
/*     le numero du sommet */
    noarcf[nav * 3 + 1] = noarcf[*na2 * 3 + 1];
/*     l'arete suivante */
    noarcf[nav * 3 + 2] = na2s;
/*     le numero nosoar de cette arete */
    noarcf[nav * 3 + 3] = noar2s;

/*     l'arete na3 se referme sur nav */
    na3s = noarcf[*na3 * 3 + 2];
    noarcf[*na3 * 3 + 2] = nav;
/*     le numero de l'arete dans le tableau nosoar */
    noar3s = noarcf[*na3 * 3 + 3];
    noarcf[*na3 * 3 + 3] = *noar2;
/*     debut du cf+1 */
    n1arcf[*nbcf] = *na3;

/*     creation d'un nouveau contour ferme na3 - na1 */
/*     ============================================= */
    ++(*nbcf);
/*     recherche d'une arete de cf vide */
    nav = n1arcf[0];
    if (nav <= 0) {
	goto L9100;
    }
/*     la 1-ere arete vide est mise a jour */
    n1arcf[0] = noarcf[nav * 3 + 2];

/*     ajout de l'arete nav pointant sur na3s */
/*     le numero du sommet */
    noarcf[nav * 3 + 1] = noarcf[*na3 * 3 + 1];
/*     l'arete suivante */
    noarcf[nav * 3 + 2] = na3s;
/*     le numero de l'arete dans le tableau nosoar */
    noarcf[nav * 3 + 3] = noar3s;

/*     recherche d'une arete de cf vide */
    nav1 = n1arcf[0];
    if (nav1 <= 0) {
	goto L9100;
    }
/*     la 1-ere arete vide est mise a jour */
    n1arcf[0] = noarcf[nav1 * 3 + 2];

/*     l'arete precedente na01 de na1 pointe sur la nouvelle nav1 */
    noarcf[*na01 * 3 + 2] = nav1;

/*     ajout de l'arete nav1 pointant sur nav */
/*     le numero du sommet */
    noarcf[nav1 * 3 + 1] = noarcf[*na1 * 3 + 1];
/*     l'arete suivante */
    noarcf[nav1 * 3 + 2] = nav;
/*     le numero de l'arete dans le tableau nosoar */
    noarcf[nav1 * 3 + 3] = *noar3;

/*     debut du cf+2 */
    n1arcf[*nbcf] = nav1;
    return 0;

/*     erreur */
L9100:
    //io___334.ciunit = unites_1.imprim;
    //s_wsle(&io___334);
    //do_lio(&c__9, &c__1, "saturation du tableau mxarcf", (ftnlen)28);
    //e_wsle();
    *nt = 0;
    return 0;

/*     erreur tableau nosoar sature */
L9900:
    //io___335.ciunit = unites_1.imprim;
    //s_wsle(&io___335);
    //do_lio(&c__9, &c__1, "saturation du tableau nosoar", (ftnlen)28);
    //e_wsle();
    *nt = 0;
    return 0;
} /* trcf0a_ */

/* Subroutine */ int trcf1a_(integer *nbcf, integer *na01, integer *na1, 
	integer *na2, integer *noar1, integer *noar3, integer *mosoar, 
	integer *mxsoar, integer *n1soar, integer *nosoar, integer *moartr, 
	integer *n1artr, integer *noartr, integer *noarst, integer *mxarcf, 
	integer *n1arcf, integer *noarcf, integer *nt)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer na3, nav, ierr;
    extern /* Subroutine */ int trcf3a_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *), fasoar_(integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___336 = { 0, 0, 0, 0, 0 };
    static cilist io___340 = { 0, 0, 0, 0, 0 };
    static cilist io___341 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    modification de la triangulation du contour ferme nbcf */
/* -----    par ajout d'un triangle ayant 1 arete sur le contour */
/*          modification du contour par ajout de la 3-eme arete */
/*          creation d'un contour ferme a partir de la seconde arete */

/* entrees: */
/* -------- */
/* nbcf    : numero dans n1arcf du cf traite ici */
/* na01    : numero noarcf de l'arete precedant l'arete na1 de noarcf */
/* na1     : numero noarcf du 1-er sommet du triangle */
/*           implicitement l'arete na1 n'est pas une arete du triangle */
/* na2     : numero noarcf du 2-eme sommet du triangle */
/*           cette arete est l'arete 2 du triangle a ajouter */
/*           son arete suivante dans noarcf n'est pas sur le contour */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */

/* entrees et sorties : */
/* -------------------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1arcf : numero d'une arete de chaque contour */
/* noarcf : numero des aretes de la ligne du contour ferme */
/*          attention : chainage circulaire des aretes */

/* sortie : */
/* -------- */
/* noar1  : numero nosoar de l'arete 1 du triangle cree */
/* noar3  : numero nosoar de l'arete 3 du triangle cree */
/* nt     : numero du triangle ajoute dans notria */
/*          0 si saturation du tableau notria ou noarcf ou n1arcf */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*     un cf supplementaire peut il etre ajoute ? */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    noarcf -= 4;

    /* Function Body */
    if (*nbcf >= *mxarcf) {
	//io___336.ciunit = unites_1.imprim;
	//s_wsle(&io___336);
	//do_lio(&c__9, &c__1, "saturation du tableau noarcf", (ftnlen)28);
	//e_wsle();
	*nt = 0;
	return 0;
    }

    ierr = 0;

/*     l' arete suivante du triangle non sur le cf */
    na3 = noarcf[*na2 * 3 + 2];

/*     creation des 2 nouvelles aretes du triangle dans le tableau nosoar */
/*     ================================================================== */
/*     la formation de l'arete sommet1-sommet2 dans le tableau nosoar */
    fasoar_(&noarcf[*na1 * 3 + 1], &noarcf[*na2 * 3 + 1], &c_n1, &c_n1, &c__0,
	     mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], 
	    noar1, &ierr);
    if (ierr != 0) {
	goto L9900;
    }

/*     la formation de l'arete sommet1-sommet3 dans le tableau nosoar */
    fasoar_(&noarcf[na3 * 3 + 1], &noarcf[*na1 * 3 + 1], &c_n1, &c_n1, &c__0, 
	    mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], noar3,
	     &ierr);
    if (ierr != 0) {
	goto L9900;
    }

/*     le triangle nt de noartr a l'arete 2 comme arete du contour na2 */
/*     =============================================================== */
    trcf3a_(&noarcf[*na1 * 3 + 1], &noarcf[*na2 * 3 + 1], &noarcf[na3 * 3 + 1]
	    , noar1, &noarcf[*na2 * 3 + 3], noar3, mosoar, &nosoar[
	    nosoar_offset], moartr, n1artr, &noartr[noartr_offset], nt);
    if (*nt <= 0) {
	return 0;
    }

/*     modification du contour ferme existant */
/*     suppression de l'arete na2 du cf */
/*     ====================================== */
/*     modification du cf en pointant na2 sur na1 */
    noarcf[*na2 * 3 + 2] = *na1;
    noarcf[*na2 * 3 + 3] = *noar1;
/*     debut du cf */
    n1arcf[*nbcf] = *na2;

/*     creation d'un nouveau contour ferme na3 - na1 */
/*     ============================================= */
    ++(*nbcf);

/*     recherche d'une arete de cf vide */
    nav = n1arcf[0];
    if (nav <= 0) {
	//io___340.ciunit = unites_1.imprim;
	//s_wsle(&io___340);
	//do_lio(&c__9, &c__1, "saturation du tableau noarcf", (ftnlen)28);
	//e_wsle();
	*nt = 0;
	return 0;
    }

/*     la 1-ere arete vide est mise a jour */
    n1arcf[0] = noarcf[nav * 3 + 2];

/*     ajout de l'arete nav pointant sur na3 */
/*     le numero du sommet */
    noarcf[nav * 3 + 1] = noarcf[*na1 * 3 + 1];
/*     l'arete suivante */
    noarcf[nav * 3 + 2] = na3;
/*     le numero de l'arete dans le tableau nosoar */
    noarcf[nav * 3 + 3] = *noar3;

/*     l'arete precedente na01 de na1 pointe sur la nouvelle nav */
    noarcf[*na01 * 3 + 2] = nav;

/*     debut du cf */
    n1arcf[*nbcf] = nav;
    return 0;

/*     erreur tableau nosoar sature */
L9900:
    //io___341.ciunit = unites_1.imprim;
    //s_wsle(&io___341);
    //do_lio(&c__9, &c__1, "saturation du tableau nosoar", (ftnlen)28);
    //e_wsle();
    *nt = 0;
    return 0;
} /* trcf1a_ */

/* Subroutine */ int trcf2a_(integer *nbcf, integer *na1, integer *noar3, 
	integer *mosoar, integer *mxsoar, integer *n1soar, integer *nosoar, 
	integer *moartr, integer *n1artr, integer *noartr, integer *noarst, 
	integer *n1arcf, integer *noarcf, integer *nt)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer na2, na3, ierr;
    extern /* Subroutine */ int trcf3a_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *), fasoar_(integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___345 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    modification de la triangulation du contour ferme nbcf */
/* -----    par ajout d'un triangle ayant 2 aretes sur le contour */
/*          creation d'une arete dans nosoar (sommet3-sommet1) */
/*          et modification du contour par ajout de la 3-eme arete */

/* entrees: */
/* -------- */
/* nbcf   : numero dans n1arcf du cf traite ici */
/* na1    : numero noarcf de la premiere arete sur le contour */
/*          implicitement sa suivante est sur le contour */
/*          la suivante de la suivante n'est pas sur le contour */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */

/* entrees et sorties : */
/* -------------------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1arcf : numero d'une arete de chaque contour */
/* noarcf : numero des aretes de la ligne du contour ferme */
/*          attention : chainage circulaire des aretes */

/* sortie : */
/* -------- */
/* noar3  : numero de l'arete 3 dans le tableau nosoar */
/* nt     : numero du triangle ajoute dans noartr */
/*          0 si saturation du tableau noartr ou nosoar */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    noarcf -= 4;

    /* Function Body */
    ierr = 0;

/*     l'arete suivante de l'arete na1 dans noarcf */
    na2 = noarcf[*na1 * 3 + 2];
/*     l'arete suivante de l'arete na2 dans noarcf */
    na3 = noarcf[na2 * 3 + 2];

/*     la formation de l'arete sommet3-sommet1 dans le tableau nosoar */
    fasoar_(&noarcf[na3 * 3 + 1], &noarcf[*na1 * 3 + 1], &c_n1, &c_n1, &c__0, 
	    mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], noar3,
	     &ierr);
    if (ierr != 0) {
	if (ierr == 1) {
	    //io___345.ciunit = unites_1.imprim;
	    //s_wsle(&io___345);
	    //do_lio(&c__9, &c__1, "saturation des aretes (tableau nosoar)", (
		   // ftnlen)38);
	    //e_wsle();
	}
	*nt = 0;
	return 0;
    }

/*     le triangle a ses 2 aretes na1 na2 sur le contour ferme */
/*     ajout dans noartr de ce triangle nt */
    trcf3a_(&noarcf[*na1 * 3 + 1], &noarcf[na2 * 3 + 1], &noarcf[na3 * 3 + 1],
	     &noarcf[*na1 * 3 + 3], &noarcf[na2 * 3 + 3], noar3, mosoar, &
	    nosoar[nosoar_offset], moartr, n1artr, &noartr[noartr_offset], nt)
	    ;
    if (*nt <= 0) {
	return 0;
    }

/*     suppression des 2 aretes (na1 na2) du cf */
/*     ces 2 aretes se suivent dans le chainage du cf */
/*     ajout de la 3-eme arete  (noar3) dans le cf */
/*     l'arete suivante de na1 devient la suivante de na2 */
    noarcf[*na1 * 3 + 2] = na3;
    noarcf[*na1 * 3 + 3] = *noar3;

/*     l'arete na2 devient vide dans noarcf */
    noarcf[na2 * 3 + 2] = n1arcf[0];
    n1arcf[0] = na2;

/*     la premiere pointee dans noarcf est na1 */
/*     chainage circulaire => ce peut etre n'importe laquelle */
    n1arcf[*nbcf] = *na1;
    return 0;
} /* trcf2a_ */

/* Subroutine */ int trcf3a_(integer *ns1, integer *ns2, integer *ns3, 
	integer *noar1, integer *noar2, integer *noar3, integer *mosoar, 
	integer *nosoar, integer *moartr, integer *n1artr, integer *noartr, 
	integer *nt)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer n;

    /* Fortran I/O blocks */
    static cilist io___346 = { 0, 0, 0, 0, 0 };


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    ajouter dans le tableau noartr le triangle */
/* -----    de sommets ns1   ns2   ns3 */
/*          d'aretes   noar1 noar2 noar3 deja existantes */
/*                     dans le tableau nosoar des aretes */

/* entrees: */
/* -------- */
/* ns1,  ns2,  ns3   : le numero dans pxyd   des 3 sommets du triangle */
/* noar1,noar2,noar3 : le numero dans nosoar des 3 aretes  du triangle */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies : */
/* ---------- */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* sorties: */
/* -------- */
/* nt     : numero dans noartr du triangle ajoute */
/*          =0 si le tableau noartr est sature */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

/*     recherche d'un triangle libre dans le tableau noartr */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;

    /* Function Body */
    if (*n1artr <= 0) {
	//io___346.ciunit = unites_1.imprim;
	//s_wsle(&io___346);
	//do_lio(&c__9, &c__1, "saturation du tableau noartr des aretes", (
	//	ftnlen)39);
	//e_wsle();
	*nt = 0;
	return 0;
    }

/*     le numero dans noartr du nouveau triangle */
    *nt = *n1artr;

/*     le nouveau premier triangle vide dans le tableau noartr */
    *n1artr = noartr[*n1artr * noartr_dim1 + 2];

/*     arete 1 du triangle nt */
/*     ====================== */
/*     orientation des 3 aretes du triangle pour qu'il soit direct */
    if (*ns1 == nosoar[*noar1 * nosoar_dim1 + 1]) {
	n = 1;
    } else {
	n = -1;
    }
/*     le numero de l'arete 1 du triangle nt */
    noartr[*nt * noartr_dim1 + 1] = n * *noar1;

/*     le numero du triangle nt pour l'arete */
    if (nosoar[*noar1 * nosoar_dim1 + 4] <= 0) {
	n = 4;
    } else {
	n = 5;
    }
    nosoar[n + *noar1 * nosoar_dim1] = *nt;

/*     arete 2 du triangle nt */
/*     ====================== */
/*     orientation des 3 aretes du triangle pour qu'il soit direct */
    if (*ns2 == nosoar[*noar2 * nosoar_dim1 + 1]) {
	n = 1;
    } else {
	n = -1;
    }
/*     le numero de l'arete 2 du triangle nt */
    noartr[*nt * noartr_dim1 + 2] = n * *noar2;

/*     le numero du triangle nt pour l'arete */
    if (nosoar[*noar2 * nosoar_dim1 + 4] <= 0) {
	n = 4;
    } else {
	n = 5;
    }
    nosoar[n + *noar2 * nosoar_dim1] = *nt;

/*     arete 3 du triangle nt */
/*     ====================== */
/*     orientation des 3 aretes du triangle pour qu'il soit direct */
    if (*ns3 == nosoar[*noar3 * nosoar_dim1 + 1]) {
	n = 1;
    } else {
	n = -1;
    }
/*     le numero de l'arete 3 du triangle nt */
    noartr[*nt * noartr_dim1 + 3] = n * *noar3;

/*     le numero du triangle nt pour l'arete */
    if (nosoar[*noar3 * nosoar_dim1 + 4] <= 0) {
	n = 4;
    } else {
	n = 5;
    }
    nosoar[n + *noar3 * nosoar_dim1] = *nt;
    return 0;
} /* trcf3a_ */

/* Subroutine */ int trcf3s_(integer *nbcf, integer *na01, integer *na1, 
	integer *na02, integer *na2, integer *na03, integer *na3, integer *
	mosoar, integer *mxsoar, integer *n1soar, integer *nosoar, integer *
	moartr, integer *n1artr, integer *noartr, integer *noarst, integer *
	mxarcf, integer *n1arcf, integer *noarcf, integer *nt)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Local variables */
    static integer naa1, naa2, naa01, na1cf, na2cf, na3cf, naor1, naor3;
    extern /* Subroutine */ int trcf0a_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *), trcf1a_(
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *),
	     trcf2a_(integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *), trcf3a_(integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);
    static integer nbascf;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :     ajout d'un triangle d'aretes na1 2 3 du tableau noarcf */
/* -----     a la triangulation d'un contour ferme (cf) */

/* entrees: */
/* -------- */
/* nbcf    : numero dans n1arcf du cf traite ici */
/*           mais aussi nombre actuel de cf avant ajout du triangle */
/* na01    : numero noarcf de l'arete precedent l'arete na1 de noarcf */
/* na1     : numero noarcf du 1-er sommet du triangle */
/* na02    : numero noarcf de l'arete precedent l'arete na2 de noarcf */
/* na2     : numero noarcf du 2-eme sommet du triangle */
/* na03    : numero noarcf de l'arete precedent l'arete na3 de noarcf */
/* na3     : numero noarcf du 3-eme sommet du triangle */

/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxarcf : nombre maximal d'aretes declarables dans noarcf, n1arcf */

/* modifies: */
/* --------- */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */

/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero d'une arete de sommet i */

/* n1arcf : numero d'une arete de chaque contour ferme */
/* noarcf : numero du sommet , numero de l'arete suivante */
/*          numero de l'arete dans le tableau nosoar */
/*          attention : chainage circulaire des aretes */

/* sortie : */
/* -------- */
/* nbcf   : nombre actuel de cf apres ajout du triangle */
/* nt     : numero du triangle ajoute dans noartr */
/*          0 si saturation du tableau nosoar ou noartr ou noarcf ou n1arcf */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*     combien y a t il d'aretes nbascf sur le cf ? */
/*     ============================================ */
/*     la premiere arete est elle sur le cf? */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    noarcf -= 4;

    /* Function Body */
    if (noarcf[*na1 * 3 + 2] == *na2) {
/*        la 1-ere arete est sur le cf */
	na1cf = 1;
    } else {
/*        la 1-ere arete n'est pas sur le cf */
	na1cf = 0;
    }

/*     la seconde arete est elle sur le cf? */
    if (noarcf[*na2 * 3 + 2] == *na3) {
/*        la 2-eme arete est sur le cf */
	na2cf = 1;
    } else {
	na2cf = 0;
    }

/*     la troisieme arete est elle sur le cf? */
    if (noarcf[*na3 * 3 + 2] == *na1) {
/*        la 3-eme arete est sur le cf */
	na3cf = 1;
    } else {
	na3cf = 0;
    }

/*     le nombre d'aretes sur le cf */
    nbascf = na1cf + na2cf + na3cf;

/*     traitement selon le nombre d'aretes sur le cf */
/*     ============================================= */
    if (nbascf == 3) {

/*        le contour ferme se reduit a un triangle avec 3 aretes sur le cf */
/*        ---------------------------------------------------------------- */
/*        ajout dans noartr de ce nouveau triangle */
	trcf3a_(&noarcf[*na1 * 3 + 1], &noarcf[*na2 * 3 + 1], &noarcf[*na3 * 
		3 + 1], &noarcf[*na1 * 3 + 3], &noarcf[*na2 * 3 + 3], &noarcf[
		*na3 * 3 + 3], mosoar, &nosoar[nosoar_offset], moartr, n1artr,
		 &noartr[noartr_offset], nt);
	if (*nt <= 0) {
	    return 0;
	}

/*        le cf est supprime et chaine vide */
	noarcf[*na3 * 3 + 2] = n1arcf[0];
	n1arcf[0] = *na1;

/*        ce cf a ete traite => un cf de moins a traiter */
	--(*nbcf);

    } else if (nbascf == 2) {

/*        le triangle a 2 aretes sur le contour */
/*        ------------------------------------- */
/*        les 2 aretes sont la 1-ere et 2-eme du triangle */
	if (na1cf == 0) {
/*           l'arete 1 n'est pas sur le cf */
	    naa1 = *na2;
	} else if (na2cf == 0) {
/*           l'arete 2 n'est pas sur le cf */
	    naa1 = *na3;
	} else {
/*           l'arete 3 n'est pas sur le cf */
	    naa1 = *na1;
	}
/*        le triangle oppose a l'arete 3 est inconnu */
/*        modification du contour apres integration du */
/*        triangle ayant ses 2-eres aretes sur le cf */
	trcf2a_(nbcf, &naa1, &naor3, mosoar, mxsoar, n1soar, &nosoar[
		nosoar_offset], moartr, n1artr, &noartr[noartr_offset], &
		noarst[1], n1arcf, &noarcf[4], nt);

    } else if (nbascf == 1) {

/*        le triangle a 1 arete sur le contour */
/*        ------------------------------------ */
/*        cette arete est la seconde du triangle */
	if (na3cf != 0) {
/*           l'arete 3 est sur le cf */
	    naa01 = *na02;
	    naa1 = *na2;
	    naa2 = *na3;
	} else if (na1cf != 0) {
/*           l'arete 1 est sur le cf */
	    naa01 = *na03;
	    naa1 = *na3;
	    naa2 = *na1;
	} else {
/*           l'arete 2 est sur le cf */
	    naa01 = *na01;
	    naa1 = *na1;
	    naa2 = *na2;
	}
/*        le triangle oppose a l'arete 1 et 3 est inconnu */
/*        modification du contour apres integration du */
/*        triangle ayant 1 arete sur le cf avec creation */
/*        d'un nouveau contour ferme */
	trcf1a_(nbcf, &naa01, &naa1, &naa2, &naor1, &naor3, mosoar, mxsoar, 
		n1soar, &nosoar[nosoar_offset], moartr, n1artr, &noartr[
		noartr_offset], &noarst[1], mxarcf, n1arcf, &noarcf[4], nt);

    } else {

/*        le triangle a 0 arete sur le contour */
/*        ------------------------------------ */
/*        modification du contour apres integration du */
/*        triangle ayant 0 arete sur le cf avec creation */
/*        de 2 nouveaux contours fermes */
	trcf0a_(nbcf, na01, na1, na2, na3, &naa1, &naa2, &naa01, mosoar, 
		mxsoar, n1soar, &nosoar[nosoar_offset], moartr, n1artr, &
		noartr[noartr_offset], &noarst[1], mxarcf, n1arcf, &noarcf[4],
		 nt);
    }
    return 0;
} /* trcf3s_ */

/* Subroutine */ int tridcf_(integer *nbcf0, integer *nbstpe, integer *nostpe,
	 doublereal *pxyd, integer *noarst, integer *mosoar, integer *mxsoar, 
	integer *n1soar, integer *nosoar, integer *moartr, integer *n1artr, 
	integer *noartr, integer *mxarcf, integer *n1arcf, integer *noarcf, 
	integer *larmin, integer *nbtrcf, integer *notrcf, integer *ierr)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset, i__1, 
	    i__2;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static doublereal d__;
    static integer i__, k;
    static doublereal s;
    static integer nt, na0, na1, na2, na3, ns1, ns2, ns3, nt0, nt1, nt2, na01,
	     na02, na12, na03, ncf, ntp0, nbcf;
    static doublereal dmin__;
    static integer imin, noar, noar1, noar2, noar3, nbstp;
    extern /* Subroutine */ int trcf3a_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *), trcf3s_(integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);
    extern doublereal surtd2_(doublereal *, doublereal *, doublereal *);
    extern /* Subroutine */ int fasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    extern doublereal diptdr_(doublereal *, doublereal *, doublereal *);
    extern /* Subroutine */ int trchtd_(doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);
    static integer nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___375 = { 0, 0, 0, 0, 0 };
    static cilist io___376 = { 0, 0, 0, 0, 0 };
    static cilist io___377 = { 0, 0, 0, 0, 0 };
    static cilist io___379 = { 0, 0, 0, 0, 0 };
    static cilist io___380 = { 0, 0, 0, 0, 0 };
    static cilist io___381 = { 0, 0, 0, 0, 0 };
    static cilist io___387 = { 0, 0, 0, 0, 0 };
    static cilist io___388 = { 0, 0, 0, 0, 0 };
    static cilist io___389 = { 0, 0, 0, 0, 0 };
    static cilist io___395 = { 0, 0, 0, 0, 0 };
    static cilist io___396 = { 0, 0, 0, 0, 0 };
    static cilist io___398 = { 0, 0, 0, 0, 0 };
    static cilist io___399 = { 0, 0, 0, 0, 0 };
    static cilist io___400 = { 0, 0, 0, 0, 0 };
    static cilist io___401 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    triangulation directe de nbcf0 contours fermes (cf) */
/* -----    definis par la liste circulaire de leurs aretes peripheriques */
/*          avec integration de nbstpe sommets isoles a l'un des cf initiaux */

/* entrees: */
/* -------- */
/* nbcf0  : nombre initial de cf a trianguler */
/* nbstpe : nombre de sommets isoles a l'interieur des cf et */
/*          a devenir sommets de la triangulation */
/* nostpe : numero dans pxyd des nbstpe sommets isoles */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxarcf  : nombre maximal d'aretes declarables dans noarcf, n1arcf, larmin, not */

/* modifies: */
/* --------- */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */

/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* n1arcf : numero de la premiere arete de chacun des nbcf0 cf */
/*          n1arcf(0)   no de la premiere arete vide du tableau noarcf */
/*          noarcf(2,i) no de l'arete suivante */
/* noarcf : numero du sommet , numero de l'arete suivante du cf */
/*          numero de l'arete dans le tableau nosoar */

/* auxiliaires : */
/* ------------- */
/* larmin : tableau (mxarcf)   auxiliaire */
/*          stocker la liste des numeros des meilleures aretes */
/*          lors de la selection du meilleur sommet du cf a trianguler */
/*          cf le sp trchtd */

/* sortie : */
/* -------- */
/* nbtrcf : nombre de  triangles des nbcf0 cf */
/* notrcf : numero des triangles des nbcf0 cf dans le tableau noartr */
/* ierr   : 0 si pas d'erreur */
/*          2 saturation de l'un des des tableaux nosoar, noartr, ... */
/*          3 si contour ferme reduit a moins de 3 aretes */
/*          4 saturation du tableau notrcf */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    mars    1997 */
/* modifs : alain perronnet laboratoire jl lions upmc paris  octobre 2006 */
/* ....................................................................012 */


/*     depart avec nbcf0 cf a trianguler */
    /* Parameter adjustments */
    --nostpe;
    pxyd -= 4;
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --notrcf;
    --larmin;
    noarcf -= 4;

    /* Function Body */
    nbcf = *nbcf0;

/*     le nombre de triangles formes dans l'ensemble des cf */
    *nbtrcf = 0;

/*     le nombre restant de sommets isoles a integrer au cf */
    nbstp = *nbstpe;

L1:
    if (nbstp <= 0) {
	goto L10;
    }

/*     il existe au moins un sommet isole */
/*     recherche d'un cf dont la premiere arete forme un triangle */
/*     d'aire>0 avec un sommet isole et recherche du sommet isole */
/*     le plus proche de cette arete */
/*     ========================================================== */
    imin = 0;
    dmin__ = 1e123;
    i__1 = nbcf;
    for (ncf = 1; ncf <= i__1; ++ncf) {
/*        le cf en haut de pile a pour arete avant la premiere arete */
	na1 = n1arcf[ncf];
	na2 = na1;
/*        recherche de l'arete qui precede la premiere arete */
L2:
	if (noarcf[na2 * 3 + 2] != na1) {
	    na2 = noarcf[na2 * 3 + 2];
	    goto L2;
	}
/*        l'arete na0 dans noarcf qui precede n1arcf( ncf ) */
	na0 = na2;
/*        la premiere arete du cf */
	na1 = noarcf[na0 * 3 + 2];
/*        son numero dans nosoar */
	noar1 = noarcf[na1 * 3 + 3];
/*        l'arete suivante */
	na2 = noarcf[na1 * 3 + 2];
/*        le no pxyd des 2 sommets de l'arete na1 */
	ns1 = noarcf[na1 * 3 + 1];
	ns2 = noarcf[na2 * 3 + 1];
	i__2 = *nbstpe;
	for (i__ = 1; i__ <= i__2; ++i__) {
/*           le sommet isole ns3 */
	    ns3 = nostpe[i__];
	    if (ns3 <= 0) {
		goto L3;
	    }
/*           aire du triangle arete na1 et sommet ns3 */
	    d__ = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 
		    3 + 1]);
	    if (d__ > 0.) {
/*              distance de ce sommet ns3 a l'arete na1 */
		d__ = diptdr_(&pxyd[ns3 * 3 + 1], &pxyd[ns1 * 3 + 1], &pxyd[
			ns2 * 3 + 1]);
		if (d__ < dmin__) {
		    dmin__ = d__;
		    imin = i__;
		}
	    }
L3:
	    ;
	}
	if (imin > 0) {
/*           le sommet imin de nostpe est a distance minimale de */
/*           la premiere arete du cf de numero ncf */
/*           la formation de l'arete ns2-ns3 dans le tableau nosoar */
	    fasoar_(&ns2, &ns3, &c_n1, &c_n1, &c__0, mosoar, mxsoar, n1soar, &
		    nosoar[nosoar_offset], &noarst[1], &noar2, ierr);
	    if (*ierr != 0) {
		goto L9900;
	    }
/*           la formation de l'arete ns3-ns1 dans le tableau nosoar */
	    fasoar_(&ns3, &ns1, &c_n1, &c_n1, &c__0, mosoar, mxsoar, n1soar, &
		    nosoar[nosoar_offset], &noarst[1], &noar3, ierr);
	    if (*ierr != 0) {
		goto L9900;
	    }

/*           ajout dans noartr du triangle de sommets ns1 ns2 ns3 */
/*           et d'aretes na1, noar2, noar3 dans nosoar */
	    trcf3a_(&ns1, &ns2, &ns3, &noar1, &noar2, &noar3, mosoar, &nosoar[
		    nosoar_offset], moartr, n1artr, &noartr[noartr_offset], &
		    nt);
	    s = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 
		    + 1]);
	    if (s <= 0.) {
		//io___375.ciunit = unites_1.imprim;
		//s_wsle(&io___375);
		//do_lio(&c__9, &c__1, "tridcf: trcf3a produit tr", (ftnlen)25);
		//do_lio(&c__3, &c__1, (char *)&nt, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " st", (ftnlen)3);
		//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
		//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
		//do_lio(&c__3, &c__1, (char *)&ns3, (ftnlen)sizeof(integer));
		//e_wsle();
		//io___376.ciunit = unites_1.imprim;
		//s_wsle(&io___376);
		//do_lio(&c__9, &c__1, "tridcf: triangle AIRE<0", (ftnlen)23);
		//e_wsle();
	    }
	    if (nt <= 0) {
		*ierr = 7;
		return 0;
	    }
	    if (*nbtrcf >= *mxarcf) {
		//io___377.ciunit = unites_1.imprim;
		//s_wsle(&io___377);
		//do_lio(&c__9, &c__1, "saturation du tableau notrcf", (ftnlen)
		//	28);
		//e_wsle();
		*ierr = 8;
		return 0;
	    }
	    ++(*nbtrcf);
	    notrcf[*nbtrcf] = nt;

/*           modification du cf. creation d'une arete dans noarcf */
	    na12 = n1arcf[0];
	    if (na12 <= 0) {
		//io___379.ciunit = unites_1.imprim;
		//s_wsle(&io___379);
		//do_lio(&c__9, &c__1, "saturation du tableau noarcf", (ftnlen)
		//	28);
		//e_wsle();
		*ierr = 10;
		return 0;
	    }
/*           la 1-ere arete vide de noarcf est mise a jour */
	    n1arcf[0] = noarcf[na12 * 3 + 2];

/*           l'arete suivante de na0 */
	    noarcf[na1 * 3 + 1] = ns1;
	    noarcf[na1 * 3 + 2] = na12;
	    noarcf[na1 * 3 + 3] = noar3;
/*           l'arete suivante de na1 */
	    noarcf[na12 * 3 + 1] = ns3;
	    noarcf[na12 * 3 + 2] = na2;
	    noarcf[na12 * 3 + 3] = noar2;

/*           un sommet isole traite */
	    --nbstp;
	    nostpe[imin] = -nostpe[imin];
	    goto L1;
	}

/* L6: */
    }

    if (imin == 0) {
	//io___380.ciunit = unites_1.imprim;
	//s_wsle(&io___380);
	//do_lio(&c__9, &c__1, "tridcf: il reste", (ftnlen)16);
	//do_lio(&c__3, &c__1, (char *)&nbstp, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " sommets isoles non triangules", (ftnlen)30);
	//e_wsle();
	//io___381.ciunit = unites_1.imprim;
	//s_wsle(&io___381);
	//do_lio(&c__9, &c__1, "ameliorer l'algorithme", (ftnlen)22);
	//e_wsle();
/* cc         pause */
	*ierr = 9;
	return 0;
    }

/*     tant qu'il existe un cf a trianguler faire */
/*     la triangulation directe du cf */
/*     ========================================== */
L10:
    if (nbcf > 0) {

/*        le cf en haut de pile a pour premiere arete */
	na01 = n1arcf[nbcf];
	na1 = noarcf[na01 * 3 + 2];

/*        choix du sommet du cf a relier a l'arete na1 */
/*        -------------------------------------------- */
	trchtd_(&pxyd[4], &na01, &na1, &noarcf[4], &na03, &na3, &larmin[1]);
	if (na3 == 0) {
	    *ierr = 3;
	    return 0;
	}

/*        l'arete suivante de na1 */
	na02 = na1;
	na2 = noarcf[na1 * 3 + 2];

/*        formation du triangle arete na1 - sommet noarcf(1,na3) */
/*        ------------------------------------------------------ */
	trcf3s_(&nbcf, &na01, &na1, &na02, &na2, &na03, &na3, mosoar, mxsoar, 
		n1soar, &nosoar[nosoar_offset], moartr, n1artr, &noartr[
		noartr_offset], &noarst[1], mxarcf, n1arcf, &noarcf[4], &nt);
	if (nt <= 0) {
/*           saturation du tableau noartr ou noarcf ou n1arcf */
	    *ierr = 2;
	    return 0;
	}
	nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);
	s = surtd2_(&pxyd[nosotr[0] * 3 + 1], &pxyd[nosotr[1] * 3 + 1], &pxyd[
		nosotr[2] * 3 + 1]);
	if (s <= 0.) {
	    //io___387.ciunit = unites_1.imprim;
	    //s_wsle(&io___387);
	    //do_lio(&c__9, &c__1, "tridcf: trcf3s produit tr", (ftnlen)25);
	    //do_lio(&c__3, &c__1, (char *)&nt, (ftnlen)sizeof(integer));
	    //do_lio(&c__9, &c__1, " st", (ftnlen)3);
	    //do_lio(&c__3, &c__3, (char *)&nosotr[0], (ftnlen)sizeof(integer));
	    //e_wsle();
	    //io___388.ciunit = unites_1.imprim;
	    //s_wsle(&io___388);
	    //do_lio(&c__9, &c__1, "tridcf: triangle AIRE<0", (ftnlen)23);
	    //e_wsle();
	}

/*        ajout du triangle cree a sa pile */
	if (*nbtrcf >= *mxarcf) {
	    //io___389.ciunit = unites_1.imprim;
	    //s_wsle(&io___389);
	    //do_lio(&c__9, &c__1, "saturation du tableau notrcf", (ftnlen)28);
	    //e_wsle();
	    *ierr = 4;
	    return 0;
	}
	++(*nbtrcf);
	notrcf[*nbtrcf] = nt;
	goto L10;
    }

/*     mise a jour du chainage des triangles des aretes */
/*     ================================================ */
    i__1 = *nbtrcf;
    for (ntp0 = 1; ntp0 <= i__1; ++ntp0) {

/*        le numero du triangle ajoute dans le tableau noartr */
	nt0 = notrcf[ntp0];

/*        boucle sur les 3 aretes du triangle nt0 */
	for (i__ = 1; i__ <= 3; ++i__) {

/*           le numero de l'arete i du triangle dans le tableau nosoar */
	    noar = (i__2 = noartr[i__ + nt0 * noartr_dim1], abs(i__2));

/*           ce triangle est il deja chaine dans cette arete? */
	    nt1 = nosoar[noar * nosoar_dim1 + 4];
	    nt2 = nosoar[noar * nosoar_dim1 + 5];
	    if (nt1 == nt0 || nt2 == nt0) {
		goto L20;
	    }

/*           ajout de ce triangle nt0 a l'arete noar */
	    if (nt1 <= 0) {
/*               le triangle est ajoute a l'arete */
		nosoar[noar * nosoar_dim1 + 4] = nt0;
	    } else if (nt2 <= 0) {
/*               le triangle est ajoute a l'arete */
		nosoar[noar * nosoar_dim1 + 5] = nt0;
	    } else {
/*              l'arete appartient a 2 triangles differents de nt0 */
/*              anomalie. chainage des triangles des aretes defectueux */
/*              a corriger */
		//io___395.ciunit = unites_1.imprim;
		//s_wsle(&io___395);
		//do_lio(&c__9, &c__1, "tridcf: erreur 1 arete dans 3 triangles"
		//	, (ftnlen)39);
		//e_wsle();
		//io___396.ciunit = unites_1.imprim;
		//s_wsle(&io___396);
		//do_lio(&c__9, &c__1, "tridcf: arete nosoar(", (ftnlen)21);
		//do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, ")=", (ftnlen)2);
		//i__2 = *mosoar;
		//for (k = 1; k <= i__2; ++k) {
		//    do_lio(&c__3, &c__1, (char *)&nosoar[k + noar * 
		//	    nosoar_dim1], (ftnlen)sizeof(integer));
		//}
		//e_wsle();
		nusotr_(&nt0, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		//io___398.ciunit = unites_1.imprim;
		//s_wsle(&io___398);
		//do_lio(&c__9, &c__1, "tridcf: triangle nt0=", (ftnlen)21);
		//do_lio(&c__3, &c__1, (char *)&nt0, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " st:", (ftnlen)4);
		//for (k = 1; k <= 3; ++k) {
		//    do_lio(&c__3, &c__1, (char *)&nosotr[k - 1], (ftnlen)
		//	    sizeof(integer));
		//}
		//e_wsle();
		nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		//io___399.ciunit = unites_1.imprim;
		//s_wsle(&io___399);
		//do_lio(&c__9, &c__1, "tridcf: triangle nt1=", (ftnlen)21);
		//do_lio(&c__3, &c__1, (char *)&nt1, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " st:", (ftnlen)4);
		//for (k = 1; k <= 3; ++k) {
		//    do_lio(&c__3, &c__1, (char *)&nosotr[k - 1], (ftnlen)
		//	    sizeof(integer));
		//}
		//e_wsle();
		nusotr_(&nt2, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		//io___400.ciunit = unites_1.imprim;
		//s_wsle(&io___400);
		//do_lio(&c__9, &c__1, "tridcf: triangle nt2=", (ftnlen)21);
		//do_lio(&c__3, &c__1, (char *)&nt2, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " st:", (ftnlen)4);
		//for (k = 1; k <= 3; ++k) {
		//    do_lio(&c__3, &c__1, (char *)&nosotr[k - 1], (ftnlen)
		//	    sizeof(integer));
		//}
		//e_wsle();
/* cc               pause */
		*ierr = 5;
		return 0;
	    }

L20:
	    ;
	}

/* L30: */
    }
    return 0;

/*     erreur tableau nosoar sature */
L9900:
    //io___401.ciunit = unites_1.imprim;
    //s_wsle(&io___401);
    //do_lio(&c__9, &c__1, "saturation du tableau nosoar", (ftnlen)28);
    //e_wsle();
    *ierr = 6;
    return 0;
} /* tridcf_ */

/* Subroutine */ int te1stm_(integer *nsasup, integer *nbarpi, doublereal *
	pxyd, integer *noarst, integer *mosoar, integer *mxsoar, integer *
	n1soar, integer *nosoar, integer *moartr, integer *mxartr, integer *
	n1artr, integer *noartr, integer *mxarcf, integer *n1arcf, integer *
	noarcf, integer *larmin, integer *notrcf, integer *liarcf, integer *
	ierr)
{
    /* Format strings */
    static char fmt_10055[] = "(\002aire0=\002,d25.16,\002 aire1=\002,d25.16)"
	    ;

    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;
    doublereal d__1;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void), s_wsfe(cilist *), do_fio(integer *, char *, ftnlen),
	   //  e_wsfe(void);

    /* Local variables */
    static integer i__;
    static doublereal s, s0, s1;
    static integer nt, nbcf, noar, noar0;
    extern doublereal surtd2_(doublereal *, doublereal *, doublereal *);
    static integer nbarcf;
    extern /* Subroutine */ int trp1st_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *), tedela_(doublereal *, integer *, integer *, integer *,
	     integer *, integer *, integer *, integer *, integer *, integer *,
	     integer *, integer *);
    static integer mmarcf, nbarfr;
    extern /* Subroutine */ int tridcf_(integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *);
    static integer nbtrcf;
    extern /* Subroutine */ int focftr_(integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *);
    static integer modifs;
    extern /* Subroutine */ int trfrcf_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);
    static integer nbstpe, nostpe[512], nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___403 = { 0, 0, 0, 0, 0 };
    static cilist io___416 = { 0, 0, 0, 0, 0 };
    static cilist io___417 = { 0, 0, 0, 0, 0 };
    static cilist io___418 = { 0, 0, 0, 0, 0 };
    static cilist io___419 = { 0, 0, 0, fmt_10055, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    supprimer de la triangulation le sommet nsasup qui doit */
/* -----    etre un sommet interne ("centre" d'une boule de triangles) */

/*          attention: le chainage lchain de nosoar devient celui des cf */

/* entrees: */
/* -------- */
/* nsasup : numero dans le tableau pxyd du sommet a supprimer */
/* nbarpi : numero du dernier sommet frontalier ou interne impose */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxarcf : nombre de variables des tableaux n1arcf, noarcf, larmin, notrcf */

/* modifies: */
/* --------- */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */


/* auxiliaires : */
/* ------------- */
/* n1arcf : tableau (0:mxarcf) auxiliaire d'entiers */
/* noarcf : tableau (3,mxarcf) auxiliaire d'entiers */
/* larmin : tableau ( mxarcf ) auxiliaire d'entiers */
/* notrcf : tableau ( mxarcf ) auxiliaire d'entiers */
/* liarcf : tableau ( mxarcf ) auxiliaire d'entiers */

/* sortie : */
/* -------- */
/* ierr   : =0 si pas d'erreur */
/*          -1 le sommet a supprimer n'est pas le centre d'une boule */
/*             de triangles. il est suppose externe */
/*             ou bien le sommet est centre d'un cf dont toutes les */
/*             aretes sont frontalieres */
/*             dans les 2 cas => retour sans modifs */
/*          >0 si une erreur est survenue */
/*          =11 algorithme defaillant */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

    /* Parameter adjustments */
    pxyd -= 4;
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --liarcf;
    --notrcf;
    --larmin;
    noarcf -= 4;

    /* Function Body */
    if (*nsasup <= *nbarpi) {
/*        sommet frontalier non destructible */
	*ierr = -1;
	return 0;
    }
    *ierr = 0;

/*     nsasup est il un sommet interne, "centre" d'une boule de triangles? */
/*     => le sommet nsasup peut etre supprime */
/*     =================================================================== */
/*     formation du cf de ''centre'' le sommet nsasup */
    trp1st_(nsasup, &noarst[1], mosoar, &nosoar[nosoar_offset], moartr, 
	    mxartr, &noartr[noartr_offset], mxarcf, &nbtrcf, &notrcf[1]);

    if (nbtrcf <= 2) {
/*        erreur: impossible de trouver tous les triangles de sommet nsasup */
/*        ou pas assez de triangles de sommet nsasup */
/*        le sommet nsasup n'est pas supprime de la triangulation */
	*ierr = -1;
	return 0;
    }

    if (nbtrcf * 3 > *mxarcf) {
	//io___403.ciunit = unites_2.imprim;
	//s_wsle(&io___403);
	//do_lio(&c__9, &c__1, "saturation du tableau noarcf", (ftnlen)28);
	//e_wsle();
	*ierr = 10;
	return 0;
    }

/*     si toutes les aretes du cf sont frontalieres, alors il est */
/*     interdit de detruire le sommet "centre" du cf */
/*     calcul du nombre nbarfr des aretes simples des nbtrcf triangles */
    trfrcf_(nsasup, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
	    noartr_offset], &nbtrcf, &notrcf[1], &nbarfr);
    if (nbarfr >= nbtrcf) {
/*        toutes les aretes simples sont frontalieres */
/*        le sommet nsasup ("centre" de la cavite) n'est pas supprime */
	*ierr = -1;
	return 0;
    }

/*     calcul des surfaces avant suppression du point */
    s0 = 0.;
    i__1 = nbtrcf;
    for (i__ = 1; i__ <= i__1; ++i__) {
	nt = notrcf[i__];
/*        les numeros des 3 sommets du triangle nt */
	nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);
	s = surtd2_(&pxyd[nosotr[0] * 3 + 1], &pxyd[nosotr[1] * 3 + 1], &pxyd[
		nosotr[2] * 3 + 1]);
	s0 += abs(s);
/* L10: */
    }

/*     formation du contour ferme (liste chainee des aretes simples) */
/*     forme a partir des aretes des triangles de l'etoile du sommet nsasup */
/*     les aretes doubles sont detruites */
/*     les triangles du cf sont detruits */
    focftr_(&nbtrcf, &notrcf[1], nbarpi, &pxyd[4], &noarst[1], mosoar, mxsoar,
	     n1soar, &nosoar[nosoar_offset], moartr, n1artr, &noartr[
	    noartr_offset], &nbarcf, n1arcf, &noarcf[4], &nbstpe, nostpe, 
	    ierr);
    if (*ierr != 0) {
/*        modification de ierr pour continuer le calcul */
	*ierr = -543;
	return 0;
    }

/*     ici le sommet nsasup n'appartient plus a aucune arete */
    noarst[*nsasup] = 0;

/*     chainage des aretes vides dans le tableau noarcf */
    n1arcf[0] = nbarcf + 1;
/* Computing MIN */
    i__1 = nbarcf << 3;
    mmarcf = min(i__1,*mxarcf);
    i__1 = mmarcf;
    for (i__ = nbarcf + 1; i__ <= i__1; ++i__) {
	noarcf[i__ * 3 + 2] = i__ + 1;
/* L40: */
    }
    noarcf[mmarcf * 3 + 2] = 0;

/*     sauvegarde du chainage des aretes peripheriques */
/*     pour la mise en delaunay du maillage */
    nbcf = n1arcf[1];
    i__1 = nbarcf;
    for (i__ = 1; i__ <= i__1; ++i__) {
/*        le numero de l'arete dans le tableau nosoar */
	liarcf[i__] = noarcf[nbcf * 3 + 3];
/*        l'arete suivante dans le cf */
	nbcf = noarcf[nbcf * 3 + 2];
/* L50: */
    }

/*     triangulation directe du contour ferme sans le sommet nsasup */
/*     ============================================================ */
    nbcf = 1;
    tridcf_(&nbcf, &nbstpe, nostpe, &pxyd[4], &noarst[1], mosoar, mxsoar, 
	    n1soar, &nosoar[nosoar_offset], moartr, n1artr, &noartr[
	    noartr_offset], mxarcf, n1arcf, &noarcf[4], &larmin[1], &nbtrcf, &
	    notrcf[1], ierr);
    if (*ierr != 0) {
	return 0;
    }
/*     calcul des surfaces apres suppression du point */
    s1 = 0.;
    i__1 = nbtrcf;
    for (i__ = 1; i__ <= i__1; ++i__) {
	nt = notrcf[i__];
/*        les numeros des 3 sommets du triangle nt */
	nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);
	s = surtd2_(&pxyd[nosotr[0] * 3 + 1], &pxyd[nosotr[1] * 3 + 1], &pxyd[
		nosotr[2] * 3 + 1]);
	if (s <= 0.) {
	    //io___416.ciunit = unites_2.imprim;
	    //s_wsle(&io___416);
	    //do_lio(&c__9, &c__1, "te1stm: apres tridcf le triangle", (ftnlen)
		   // 32);
	    //do_lio(&c__3, &c__1, (char *)&nt, (ftnlen)sizeof(integer));
	    //do_lio(&c__9, &c__1, " st", (ftnlen)3);
	    //do_lio(&c__3, &c__3, (char *)&nosotr[0], (ftnlen)sizeof(integer));
	    //do_lio(&c__9, &c__1, " AIRE<0", (ftnlen)7);
	    //e_wsle();
	}
	s1 += abs(s);
/* L55: */
    }

    if ((d__1 = s0 - s1, abs(d__1)) > s0 * 1e-10) {
	//io___417.ciunit = unites_2.imprim;
	//s_wsle(&io___417);
	//e_wsle();
	//io___418.ciunit = unites_2.imprim;
	//s_wsle(&io___418);
	//do_lio(&c__9, &c__1, "te1stm: difference des aires lors suppression "
	//	"st", (ftnlen)48);
	//do_lio(&c__3, &c__1, (char *)&(*nsasup), (ftnlen)sizeof(integer));
	//e_wsle();
	//io___419.ciunit = unites_2.imprim;
	//s_wsfe(&io___419);
	//do_fio(&c__1, (char *)&s0, (ftnlen)sizeof(doublereal));
	//do_fio(&c__1, (char *)&s1, (ftnlen)sizeof(doublereal));
	//e_wsfe();
    }

/*     transformation des triangles du cf en triangles delaunay */
/*     ======================================================== */
/*     construction du chainage lchain dans nosoar */
/*     des aretes peripheriques du cf a partir de la sauvegarde liarcf */
    noar0 = liarcf[1];
    i__1 = nbarcf;
    for (i__ = 2; i__ <= i__1; ++i__) {
/*        le numero de l'arete peripherique du cf dans nosoar */
	noar = liarcf[i__];
	if (nosoar[noar * nosoar_dim1 + 3] <= 0) {
/*           arete interne => elle est chainee a partir de la precedente */
	    nosoar[noar0 * nosoar_dim1 + 6] = noar;
	    noar0 = noar;
	}
/* L60: */
    }
/*     la derniere arete peripherique n'a pas de suivante */
    nosoar[noar0 * nosoar_dim1 + 6] = 0;

/*     mise en delaunay des aretes chainees */
    tedela_(&pxyd[4], &noarst[1], mosoar, mxsoar, n1soar, &nosoar[
	    nosoar_offset], &liarcf[1], moartr, mxartr, n1artr, &noartr[
	    noartr_offset], &modifs);
    return 0;
} /* te1stm_ */

/* Subroutine */ int tr3str_(integer *np, integer *nt, integer *mosoar, 
	integer *mxsoar, integer *n1soar, integer *nosoar, integer *moartr, 
	integer *mxartr, integer *n1artr, integer *noartr, integer *noarst, 
	integer *nutr, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Local variables */
    static integer i__, i1, nt0, nti, noar, nu2sar[2];
    extern /* Subroutine */ int hasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer nuarco[3], nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former les 3 sous-triangles du triangle nt a partir */
/* -----    du point interne np */

/* entrees: */
/* -------- */
/* np     : numero dans le tableau pxyd du point */
/* nt     : numero dans le tableau noartr du triangle a trianguler */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/*          hachage des aretes = (nosoar(1)+nosoar(2)) modulo mxsoar */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero d'une arete de sommet i */

/* sorties: */
/* -------- */
/* nutr   : le numero des 3 sous-triangles du triangle nt */
/* nt     : en sortie le triangle initial n'est plus actif dans noartr */
/*          c'est en fait le premier triangle vide de noartr */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */


/*     reservation des 3 nouveaux triangles dans le tableau noartr */
/*     =========================================================== */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --nutr;

    /* Function Body */
    for (i__ = 1; i__ <= 3; ++i__) {
/*        le numero du sous-triangle i dans le tableau noartr */
	if (*n1artr <= 0) {
/*           tableau noartr sature */
	    *ierr = 2;
	    return 0;
	}
	nutr[i__] = *n1artr;
/*        le nouveau premier triangle libre dans noartr */
	*n1artr = noartr[*n1artr * noartr_dim1 + 2];
/* L10: */
    }

/*     les numeros des 3 sommets du triangle nt */
    nusotr_(nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[noartr_offset]
	    , nosotr);

/*     formation des 3 aretes nosotr(i)-np dans le tableau nosoar */
/*     ========================================================== */
    nt0 = nutr[3];
    for (i__ = 1; i__ <= 3; ++i__) {

/*        le triangle a creer */
	nti = nutr[i__];

/*        les 2 sommets du cote i du triangle nosotr */
	nu2sar[0] = nosotr[i__ - 1];
	nu2sar[1] = *np;
	hasoar_(mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], nu2sar, &noar)
		;
/*        en sortie: noar>0 => no arete retrouvee */
/*                       <0 => no arete ajoutee */
/*                       =0 => saturation du tableau nosoar */

	if (noar == 0) {
/*           saturation du tableau nosoar */
	    *ierr = 1;
	    return 0;
	} else if (noar < 0) {
/*           l'arete a ete ajoutee. initialisation des autres informations */
	    noar = -noar;
/*           le numero des 2 sommets a ete initialise par hasoar */
/*           et (nosoar(1,noar)<nosoar(2,noar)) */
/*           le numero de la ligne de l'arete: ici arete interne */
	    nosoar[noar * nosoar_dim1 + 3] = 0;
/*        else */
/*           l'arete a ete retrouvee */
/*           le numero des 2 sommets a ete retrouve par hasoar */
/*           et (nosoar(1,noar)<nosoar(2,noar)) */
/*           le numero de ligne reste inchange */
	}

/*        le triangle 1 de l'arete noar => le triangle nt0 */
	nosoar[noar * nosoar_dim1 + 4] = nt0;
/*        le triangle 2 de l'arete noar => le triangle nti */
	nosoar[noar * nosoar_dim1 + 5] = nti;

/*        le sommet nosotr(i) appartient a l'arete noar */
	noarst[nosotr[i__ - 1]] = noar;

/*        le numero d'arete nosotr(i)-np */
	nuarco[i__ - 1] = noar;

/*        le triangle qui precede le suivant */
	nt0 = nti;
/* L20: */
    }

/*     le numero d'une arete du point np */
    noarst[*np] = noar;

/*     les 3 sous-triangles du triangle nt sont formes dans le tableau noartr */
/*     ====================================================================== */
    for (i__ = 1; i__ <= 3; ++i__) {

/*        le numero suivant i => i mod 3 + 1 */
	if (i__ != 3) {
	    i1 = i__ + 1;
	} else {
	    i1 = 1;
	}

/*        le numero dans noartr du sous-triangle a ajouter */
	nti = nutr[i__];

/*        le numero de l'arete i du triangle initial nt */
/*        est l'arete 1 du sous-triangle i */
	noar = noartr[i__ + *nt * noartr_dim1];
	noartr[nti * noartr_dim1 + 1] = noar;

/*        mise a jour du numero de triangle de cette arete */
	noar = abs(noar);
	if (nosoar[noar * nosoar_dim1 + 4] == *nt) {
/*           le sous-triangle nti remplace le triangle nt */
	    nosoar[noar * nosoar_dim1 + 4] = nti;
	} else {
/*           le sous-triangle nti remplace le triangle nt */
	    nosoar[noar * nosoar_dim1 + 5] = nti;
	}

/*        l'arete 2 du sous-triangle i est l'arete i1 ajoutee */
	if (nosotr[i1 - 1] == nosoar[nuarco[i1 - 1] * nosoar_dim1 + 1]) {
/*           l'arete ns i1-np dans nosoar est dans le sens direct */
	    noartr[nti * noartr_dim1 + 2] = nuarco[i1 - 1];
	} else {
/*           l'arete ns i1-np dans nosoar est dans le sens indirect */
	    noartr[nti * noartr_dim1 + 2] = -nuarco[i1 - 1];
	}

/*        l'arete 3 du sous-triangle i est l'arete i ajoutee */
	if (nosotr[i__ - 1] == nosoar[nuarco[i__ - 1] * nosoar_dim1 + 1]) {
/*           l'arete ns i1-np dans nosoar est dans le sens indirect */
	    noartr[nti * noartr_dim1 + 3] = -nuarco[i__ - 1];
	} else {
/*           l'arete ns i1-np dans nosoar est dans le sens direct */
	    noartr[nti * noartr_dim1 + 3] = nuarco[i__ - 1];
	}
/* L30: */
    }

/*     le triangle nt est rendu libre */
/*     ============================== */
/*     il devient n1artr le premier triangle libre */
    noartr[*nt * noartr_dim1 + 1] = 0;
    noartr[*nt * noartr_dim1 + 2] = *n1artr;
    *n1artr = *nt;
    return 0;
} /* tr3str_ */

/* Subroutine */ int mt4sqa_(integer *na, integer *moartr, integer *noartr, 
	integer *mosoar, integer *nosoar, integer *ns1, integer *ns2, integer 
	*ns3, integer *ns4)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset, i__1;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, nt, naa;

    /* Fortran I/O blocks */
    static cilist io___431 = { 0, 0, 0, 0, 0 };
    static cilist io___432 = { 0, 0, 0, 0, 0 };
    static cilist io___434 = { 0, 0, 0, 0, 0 };
    static cilist io___436 = { 0, 0, 0, 0, 0 };
    static cilist io___438 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    calcul du numero des 4 sommets de l'arete na de nosoar */
/* -----    formant un quadrangle */

/* entrees: */
/* -------- */
/* na     : numero de l'arete dans nosoar a traiter */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1=0 si triangle vide => arete2=triangle vide suivant */
/* mosoar : nombre maximal d'entiers par arete */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages en + */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */

/* sorties: */
/* -------- */
/* ns1,ns2,ns3 : les 3 numeros des sommets du triangle t1 en sens direct */
/* ns1,ns4,ns2 : les 3 numeros des sommets du triangle t2 en sens direct */

/* si erreur rencontree => ns4 = 0 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*     le numero de triangle est il correct  ? */
/*     a supprimer apres mise au point */
    /* Parameter adjustments */
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;

    /* Function Body */
    if (*na <= 0) {
/*         nblgrc(nrerr) = 1 */
/*         write(kerr(mxlger)(1:6),'(i6)') na */
/*         kerr(1) = kerr(mxlger)(1:6) // */
/*     %           ' no incorrect arete dans nosoar' */
/*         call lereur */
	//io___431.ciunit = unites_1.imprim;
	//s_wsle(&io___431);
	//do_lio(&c__3, &c__1, (char *)&(*na), (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " no incorrect arete dans nosoar", (ftnlen)31);
	//e_wsle();
	*ns4 = 0;
	return 0;
    }

    if (nosoar[*na * nosoar_dim1 + 1] <= 0) {
/*         nblgrc(nrerr) = 1 */
/*         write(kerr(mxlger)(1:6),'(i6)') na */
/*         kerr(1) = kerr(mxlger)(1:6) // */
/*     %           ' arete non active dans nosoar' */
/*         call lereur */
	//io___432.ciunit = unites_1.imprim;
	//s_wsle(&io___432);
	//do_lio(&c__3, &c__1, (char *)&(*na), (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " arete non active dans nosoar", (ftnlen)29);
	//e_wsle();
	*ns4 = 0;
	return 0;
    }

/*     recherche de l'arete na dans le premier triangle */
    nt = nosoar[*na * nosoar_dim1 + 4];
    if (nt <= 0) {
/*         nblgrc(nrerr) = 1 */
/*         write(kerr(mxlger)(1:6),'(i6)') na */
/*         kerr(1) =  'triangle 1 incorrect pour l''arete ' // */
/*     %               kerr(mxlger)(1:6) */
/*         call lereur */
	//io___434.ciunit = unites_1.imprim;
	//s_wsle(&io___434);
	//do_lio(&c__9, &c__1, "triangle 1 incorrect pour l'arete ", (ftnlen)34)
	//	;
	//do_lio(&c__3, &c__1, (char *)&(*na), (ftnlen)sizeof(integer));
	//e_wsle();
	*ns4 = 0;
	return 0;
    }

    for (i__ = 1; i__ <= 3; ++i__) {
	if ((i__1 = noartr[i__ + nt * noartr_dim1], abs(i__1)) == *na) {
	    goto L8;
	}
/* L5: */
    }
/*     si arrivee ici => bogue avant */
 /*   io___436.ciunit = unites_1.imprim;
    s_wsle(&io___436);
    do_lio(&c__9, &c__1, "mt4sqa: arete", (ftnlen)13);
    do_lio(&c__3, &c__1, (char *)&(*na), (ftnlen)sizeof(integer));
    do_lio(&c__9, &c__1, " non dans le triangle", (ftnlen)21);
    do_lio(&c__3, &c__1, (char *)&nt, (ftnlen)sizeof(integer));
    e_wsle();*/
    *ns4 = 0;
    return 0;

/*     les 2 sommets de l'arete na */
L8:
    if (noartr[i__ + nt * noartr_dim1] > 0) {
	*ns1 = 1;
	*ns2 = 2;
    } else {
	*ns1 = 2;
	*ns2 = 1;
    }
    *ns1 = nosoar[*ns1 + *na * nosoar_dim1];
    *ns2 = nosoar[*ns2 + *na * nosoar_dim1];

/*     l'arete suivante */
    if (i__ < 3) {
	++i__;
    } else {
	i__ = 1;
    }
    naa = (i__1 = noartr[i__ + nt * noartr_dim1], abs(i__1));

/*     le sommet ns3 du triangle 123 */
    *ns3 = nosoar[naa * nosoar_dim1 + 1];
    if (*ns3 == *ns1 || *ns3 == *ns2) {
	*ns3 = nosoar[naa * nosoar_dim1 + 2];
    }

/*     le triangle de l'autre cote de l'arete na */
/*     ========================================= */
    nt = nosoar[*na * nosoar_dim1 + 5];
    if (nt <= 0) {
/*         nblgrc(nrerr) = 1 */
/*         write(kerr(mxlger)(1:6),'(i6)') na */
/*         kerr(1) =  'triangle 2 incorrect pour l''arete ' // */
/*     %               kerr(mxlger)(1:6) */
/*         call lereur */
	//io___438.ciunit = unites_1.imprim;
	//s_wsle(&io___438);
	//do_lio(&c__9, &c__1, "triangle 2 incorrect pour l'arete ", (ftnlen)34)
	//	;
	//do_lio(&c__3, &c__1, (char *)&(*na), (ftnlen)sizeof(integer));
	//e_wsle();
	*ns4 = 0;
	return 0;
    }

/*     le numero de l'arete naa du triangle nt */
    naa = (i__1 = noartr[nt * noartr_dim1 + 1], abs(i__1));
    if (naa == *na) {
	naa = (i__1 = noartr[nt * noartr_dim1 + 2], abs(i__1));
    }
    *ns4 = nosoar[naa * nosoar_dim1 + 1];
    if (*ns4 == *ns1 || *ns4 == *ns2) {
	*ns4 = nosoar[naa * nosoar_dim1 + 2];
    }
    return 0;
} /* mt4sqa_ */

/* Subroutine */ int te2t2t_(integer *noaret, integer *mosoar, integer *
	n1soar, integer *nosoar, integer *noarst, integer *moartr, integer *
	noartr, integer *noar34)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer n1, n2, n3, ns1, ns2, ns3, ns4, nt1, nt2, na31, na23, na14,
	     na42, ierr;
    extern /* Subroutine */ int mt4sqa_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *),
	     fasoar_(integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *), sasoar_(integer *, integer *, integer *, integer *, 
	    integer *, integer *);
    static integer mxsoar;

    /* Fortran I/O blocks */
    static cilist io___445 = { 0, 0, 0, 0, 0 };
    static cilist io___451 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    echanger la diagonale des 2 triangles ayant en commun */
/* -----    l'arete noaret du tableau nosoar si c'est possible */

/* entrees: */
/* -------- */
/* noaret : numero de l'arete a echanger entre les 2 triangles */
/* mosoar : nombre maximal d'entiers par arete */
/* moartr : nombre maximal d'entiers par triangle */

/* modifies : */
/* ---------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages en + */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* sortie : */
/* -------- */
/* noar34 : numero nosoar de la nouvelle arete diagonale */
/*          0 si pas d'echange des aretes diagonales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc      avril 1997 */
/* ....................................................................012 */

/*     une arete frontaliere ne peut etre echangee */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    --noarst;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;

    /* Function Body */
    *noar34 = 0;
    if (nosoar[*noaret * nosoar_dim1 + 3] > 0) {
	return 0;
    }

/*     les 4 sommets des 2 triangles ayant l'arete noaret en commun */
    mt4sqa_(noaret, moartr, &noartr[noartr_offset], mosoar, &nosoar[
	    nosoar_offset], &ns1, &ns2, &ns3, &ns4);
/*     ns1,ns2,ns3 : les 3 numeros des sommets du triangle nt1 en sens direct */
/*     ns1,ns4,ns2 : les 3 numeros des sommets du triangle nt2 en sens direct */

/*     recherche du numero de l'arete noaret dans le triangle nt1 */
    nt1 = nosoar[*noaret * nosoar_dim1 + 4];
    for (n1 = 1; n1 <= 3; ++n1) {
	if ((i__1 = noartr[n1 + nt1 * noartr_dim1], abs(i__1)) == *noaret) {
	    goto L15;
	}
/* L10: */
    }
/*     impossible d'arriver ici sans bogue! */
    //io___445.ciunit = unites_2.imprim;
    //s_wsle(&io___445);
    //do_lio(&c__9, &c__1, "anomalie dans te2t2t 1", (ftnlen)22);
    //e_wsle();

/*     l'arete de sommets 2 et 3 */
L15:
    if (n1 < 3) {
	n2 = n1 + 1;
    } else {
	n2 = 1;
    }
    na23 = noartr[n2 + nt1 * noartr_dim1];

/*     l'arete de sommets 3 et 1 */
    if (n2 < 3) {
	n3 = n2 + 1;
    } else {
	n3 = 1;
    }
    na31 = noartr[n3 + nt1 * noartr_dim1];

/*     recherche du numero de l'arete noaret dans le triangle nt2 */
    nt2 = nosoar[*noaret * nosoar_dim1 + 5];
    for (n1 = 1; n1 <= 3; ++n1) {
	if ((i__1 = noartr[n1 + nt2 * noartr_dim1], abs(i__1)) == *noaret) {
	    goto L25;
	}
/* L20: */
    }
/*     impossible d'arriver ici sans bogue! */
    //io___451.ciunit = unites_2.imprim;
    //s_wsle(&io___451);
    //do_lio(&c__9, &c__1, "Anomalie dans te2t2t 2", (ftnlen)22);
    //e_wsle();

/*     l'arete de sommets 1 et 4 */
L25:
    if (n1 < 3) {
	n2 = n1 + 1;
    } else {
	n2 = 1;
    }
    na14 = noartr[n2 + nt2 * noartr_dim1];

/*     l'arete de sommets 4 et 2 */
    if (n2 < 3) {
	n3 = n2 + 1;
    } else {
	n3 = 1;
    }
    na42 = noartr[n3 + nt2 * noartr_dim1];

/*     les triangles 123 142 deviennent 143 234 */
/*     ======================================== */
/*     ajout de l'arete ns3-ns4 */
/*     on evite l'affichage de l'erreur */
    ierr = -1;
    fasoar_(&ns3, &ns4, &nt1, &nt2, &c__0, mosoar, &mxsoar, n1soar, &nosoar[
	    nosoar_offset], &noarst[1], noar34, &ierr);
    if (ierr > 0) {
/*        ierr=1 si le tableau nosoar est sature */
/*            =2 si arete a creer et appartenant a 2 triangles distincts */
/*               des triangles nt1 et nt2 */
/*            =3 si arete appartenant a 2 triangles distincts */
/*               differents des triangles nt1 et nt2 */
/*            =4 si arete appartenant a 2 triangles distincts */
/*               dont le second n'est pas le triangle nt2 */
/*        => pas d'echange */
	*noar34 = 0;
	return 0;
    }

/*     suppression de l'arete noaret */
    sasoar_(noaret, mosoar, &mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[
	    1]);

/*     nt1 = triangle 143 */
    noartr[nt1 * noartr_dim1 + 1] = na14;
/*     sens de stockage de l'arete ns3-ns4 dans nosoar? */
    if (nosoar[*noar34 * nosoar_dim1 + 1] == ns3) {
	n1 = -1;
    } else {
	n1 = 1;
    }
    noartr[nt1 * noartr_dim1 + 2] = *noar34 * n1;
    noartr[nt1 * noartr_dim1 + 3] = na31;

/*     nt2 = triangle 234 */
    noartr[nt2 * noartr_dim1 + 1] = na23;
    noartr[nt2 * noartr_dim1 + 2] = -(*noar34) * n1;
    noartr[nt2 * noartr_dim1 + 3] = na42;

/*     echange nt1 -> nt2 pour l'arete na23 */
    na23 = abs(na23);
    if (nosoar[na23 * nosoar_dim1 + 4] == nt1) {
	n1 = 4;
    } else {
	n1 = 5;
    }
    nosoar[n1 + na23 * nosoar_dim1] = nt2;

/*     echange nt2 -> nt1 pour l'arete na14 */
    na14 = abs(na14);
    if (nosoar[na14 * nosoar_dim1 + 4] == nt2) {
	n1 = 4;
    } else {
	n1 = 5;
    }
    nosoar[n1 + na14 * nosoar_dim1] = nt1;

/*     numero d'une arete de chacun des 4 sommets */
    noarst[ns1] = na14;
    noarst[ns2] = na23;
    noarst[ns3] = *noar34;
    noarst[ns4] = *noar34;
    return 0;
} /* te2t2t_ */

/* Subroutine */ int f0trte_(integer *letree, doublereal *pxyd, integer *
	mosoar, integer *mxsoar, integer *n1soar, integer *nosoar, integer *
	moartr, integer *mxartr, integer *n1artr, integer *noartr, integer *
	noarst, integer *nbtr, integer *nutr, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, i1, nt;
    extern /* Subroutine */ int fasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer lesign, nuarco[3];
    extern /* Subroutine */ int trpite_(integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___456 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former le ou les triangles du triangle equilateral letree */
/* -----    les points internes au te deviennent des sommets des */
/*          sous-triangles du te */

/* entrees: */
/* -------- */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          si letree(0)>0 alors */
/*             letree(0:3) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3) :-no pxyd des 1 a 4 points internes au triangle j */
/*                           0  si pas de point */
/*                         ( le te est une feuille de l'arbre ) */
/*          letree(4) : no letree du sur-triangle du triangle j */
/*          letree(5) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8) : no pxyd des 3 sommets du triangle j */
/* pxyd   : tableau des x  y  distance_souhaitee de chaque sommet */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = (nosoar(1)+nosoar(2)) modulo mxsoar */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero d'une arete de sommet i */

/* sorties: */
/* -------- */
/* nbtr   : nombre de sous-triangles du te, triangulation du te */
/* nutr   : numero des nbtr sous-triangles du te dans le tableau noartr */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/*          =3 si aucun des triangles ne contient l'un des points internes au te */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */

/*     le numero nt du triangle dans le tableau noartr */
    /* Parameter adjustments */
    pxyd -= 4;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --nutr;

    /* Function Body */
    if (*n1artr <= 0) {
/*        tableau noartr sature */
	//io___456.ciunit = unites_1.imprim;
	//s_wsle(&io___456);
	//do_lio(&c__9, &c__1, "f0trte: tableau noartr sature", (ftnlen)29);
	//e_wsle();
	*ierr = 2;
	return 0;
    }
    nt = *n1artr;
/*     le numero du nouveau premier triangle libre dans noartr */
    *n1artr = noartr[*n1artr * noartr_dim1 + 2];

/*     formation du triangle = le triangle equilateral letree */
    for (i__ = 1; i__ <= 3; ++i__) {
	if (i__ != 3) {
	    i1 = i__ + 1;
	} else {
	    i1 = 1;
	}
/*        ajout eventuel de l'arete si si+1 dans le tableau nosoar */
	fasoar_(&letree[i__ + 5], &letree[i1 + 5], &nt, &c_n1, &c__0, mosoar, 
		mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], &nuarco[
		i__ - 1], ierr);
	if (*ierr != 0) {
	    return 0;
	}
/* L10: */
    }

/*     le triangle nt est forme dans le tableau noartr */
    for (i__ = 1; i__ <= 3; ++i__) {
/*        letree(5+i) est le numero du sommet 1 de l'arete i du te */
	if (letree[i__ + 5] == nosoar[nuarco[i__ - 1] * nosoar_dim1 + 1]) {
	    lesign = 1;
	} else {
	    lesign = -1;
	}
/*        l'arete ns1-ns2 dans nosoar est celle du cote du te */
	noartr[i__ + nt * noartr_dim1] = lesign * nuarco[i__ - 1];
/* L20: */
    }

/*     triangulation du te=triangle nt par ajout des points internes du te */
    *nbtr = 1;
    nutr[1] = nt;
    trpite_(letree, &pxyd[4], mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], 
	    moartr, mxartr, n1artr, &noartr[noartr_offset], &noarst[1], nbtr, 
	    &nutr[1], ierr);
    return 0;
} /* f0trte_ */

/* Subroutine */ int f1trte_(integer *letree, doublereal *pxyd, integer *
	milieu, integer *mosoar, integer *mxsoar, integer *n1soar, integer *
	nosoar, integer *moartr, integer *mxartr, integer *n1artr, integer *
	noartr, integer *noarst, integer *nbtr, integer *nutr, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Local variables */
    static integer i__, i1, nm;
    extern /* Subroutine */ int fasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer lesign, nuarco[5];
    extern /* Subroutine */ int trpite_(integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);
    static integer nosotr[3];

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former les triangles du triangle equilateral letree */
/* -----    a partir de l'un des 3 milieux des cotes du te */
/*          et des points internes au te */
/*          ils deviennent tous des sommets des sous-triangles du te */

/* entrees: */
/* -------- */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          si letree(0)>0 alors */
/*             letree(0:3) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3) :-no pxyd des 1 a 4 points internes au triangle j */
/*                           0  si pas de point */
/*                         ( le te est une feuille de l'arbre ) */
/*          letree(4) : no letree du sur-triangle du triangle j */
/*          letree(5) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8) : no pxyd des 3 sommets du triangle j */
/* pxyd   : tableau des x  y  distance_souhaitee de chaque sommet */
/* milieu : milieu(i) numero dans pxyd du milieu de l'arete i du te */
/*                    0 si pas de milieu du cote i a ajouter */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = (nosoar(1)+nosoar(2)) modulo mxsoar */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(np) numero d'une arete du sommet np */

/* sorties: */
/* -------- */
/* nbtr   : nombre de sous-triangles du te, triangulation du te */
/* nutr   : numero des nbtr sous-triangles du te dans le tableau noartr */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/*          =3 si aucun des triangles ne contient l'un des points internes au te */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */


/*     le numero des 2 triangles (=2 demi te) a creer dans le tableau noartr */
    /* Parameter adjustments */
    pxyd -= 4;
    --milieu;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --nutr;

    /* Function Body */
    for (*nbtr = 1; *nbtr <= 2; ++(*nbtr)) {
	if (*n1artr <= 0) {
/*           tableau noartr sature */
	    *ierr = 2;
	    return 0;
	}
	nutr[*nbtr] = *n1artr;
/*        le nouveau premier triangle libre dans noartr */
	*n1artr = noartr[*n1artr * noartr_dim1 + 2];
/* L5: */
    }
    *nbtr = 2;

/*     recherche du milieu a creer */
    for (i__ = 1; i__ <= 3; ++i__) {
	if (milieu[i__] != 0) {
	    goto L9;
	}
/* L7: */
    }
/*     le numero pxyd du point milieu du cote i */
L9:
    nm = milieu[i__];

/*     on se ramene au seul cas i=3 c-a-d le milieu est sur le cote 3 */
    if (i__ == 1) {
/*        milieu sur le cote 1 */
	nosotr[0] = letree[7];
	nosotr[1] = letree[8];
	nosotr[2] = letree[6];
    } else if (i__ == 2) {
/*        milieu sur le cote 2 */
	nosotr[0] = letree[8];
	nosotr[1] = letree[6];
	nosotr[2] = letree[7];
    } else {
/*        milieu sur le cote 3 */
	nosotr[0] = letree[6];
	nosotr[1] = letree[7];
	nosotr[2] = letree[8];
    }

/*     formation des 2 aretes s1 s2 et s2 s3 */
    for (i__ = 1; i__ <= 2; ++i__) {
	if (i__ != 3) {
	    i1 = i__ + 1;
	} else {
	    i1 = 1;
	}
/*        ajout eventuel de l'arete dans nosoar */
	fasoar_(&nosotr[i__ - 1], &nosotr[i1 - 1], &nutr[i__], &c_n1, &c__0, 
		mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], &
		nuarco[i__ - 1], ierr);
	if (*ierr != 0) {
	    return 0;
	}
/* L10: */
    }

/*     ajout eventuel de l'arete s3 milieu dans nosoar */
    fasoar_(&nosotr[2], &nm, &nutr[2], &c_n1, &c__0, mosoar, mxsoar, n1soar, &
	    nosoar[nosoar_offset], &noarst[1], &nuarco[2], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete milieu s1 dans nosoar */
    fasoar_(nosotr, &nm, &nutr[1], &c_n1, &c__0, mosoar, mxsoar, n1soar, &
	    nosoar[nosoar_offset], &noarst[1], &nuarco[3], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete milieu s2 dans nosoar */
    fasoar_(&nosotr[1], &nm, &nutr[1], &nutr[2], &c__0, mosoar, mxsoar, 
	    n1soar, &nosoar[nosoar_offset], &noarst[1], &nuarco[4], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     les aretes s1 s2 et s2 s3 dans le tableau noartr */
    for (i__ = 1; i__ <= 2; ++i__) {
/*        nosotr(i) est le numero du sommet 1 de l'arete i du te */
	if (nosotr[i__ - 1] == nosoar[nuarco[i__ - 1] * nosoar_dim1 + 1]) {
	    lesign = 1;
	} else {
	    lesign = -1;
	}
/*        l'arete ns1-ns2 dans nosoar est celle du cote du te */
	noartr[nutr[i__] * noartr_dim1 + 1] = lesign * nuarco[i__ - 1];
/* L20: */
    }

/*     l'arete mediane s2 milieu */
    if (nm == nosoar[nuarco[4] * nosoar_dim1 + 1]) {
	lesign = -1;
    } else {
	lesign = 1;
    }
    noartr[nutr[1] * noartr_dim1 + 2] = lesign * nuarco[4];
    noartr[nutr[2] * noartr_dim1 + 3] = -lesign * nuarco[4];

/*     l'arete s1 milieu */
    if (nm == nosoar[nuarco[3] * nosoar_dim1 + 1]) {
	lesign = 1;
    } else {
	lesign = -1;
    }
    noartr[nutr[1] * noartr_dim1 + 3] = lesign * nuarco[3];

/*     l'arete s3 milieu */
    if (nm == nosoar[nuarco[2] * nosoar_dim1 + 1]) {
	lesign = -1;
    } else {
	lesign = 1;
    }
    noartr[nutr[2] * noartr_dim1 + 2] = lesign * nuarco[2];

/*     triangulation des 2 demi te par ajout des points internes du te */
    trpite_(letree, &pxyd[4], mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], 
	    moartr, mxartr, n1artr, &noartr[noartr_offset], &noarst[1], nbtr, 
	    &nutr[1], ierr);
    return 0;
} /* f1trte_ */

/* Subroutine */ int f2trte_(integer *letree, doublereal *pxyd, integer *
	milieu, integer *mosoar, integer *mxsoar, integer *n1soar, integer *
	nosoar, integer *moartr, integer *mxartr, integer *n1artr, integer *
	noartr, integer *noarst, integer *nbtr, integer *nutr, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Local variables */
    static integer i__, nm2, nm3;
    extern /* Subroutine */ int fasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer lesign, nuarco[7];
    extern /* Subroutine */ int trpite_(integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);
    static integer nosotr[3];

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former les triangles du triangle equilateral letree */
/* -----    a partir de 2 milieux des cotes du te */
/*          et des points internes au te */
/*          ils deviennent tous des sommets des sous-triangles du te */

/* entrees: */
/* -------- */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          si letree(0)>0 alors */
/*             letree(0:3) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3) :-no pxyd des 1 a 4 points internes au triangle j */
/*                           0  si pas de point */
/*                         ( le te est une feuille de l'arbre ) */
/*          letree(4) : no letree du sur-triangle du triangle j */
/*          letree(5) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8) : no pxyd des 3 sommets du triangle j */
/* pxyd   : tableau des x  y  distance_souhaitee de chaque sommet */
/* milieu : milieu(i) numero dans pxyd du milieu de l'arete i du te */
/*                    0 si pas de milieu du cote i a ajouter */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = (nosoar(1)+nosoar(2)) modulo mxsoar */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(np) numero d'une arete du sommet np */

/* sorties: */
/* -------- */
/* nbtr   : nombre de sous-triangles du te, triangulation du te */
/* nutr   : numero des nbtr sous-triangles du te dans le tableau noartr */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/*          =3 si aucun des triangles ne contient l'un des points internes au te */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */


/*     le numero des 3 triangles a creer dans le tableau noartr */
    /* Parameter adjustments */
    pxyd -= 4;
    --milieu;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --nutr;

    /* Function Body */
    for (*nbtr = 1; *nbtr <= 3; ++(*nbtr)) {
	if (*n1artr <= 0) {
/*           tableau noartr sature */
	    *ierr = 2;
	    return 0;
	}
	nutr[*nbtr] = *n1artr;
/*        le nouveau premier triangle libre dans noartr */
	*n1artr = noartr[*n1artr * noartr_dim1 + 2];
/* L5: */
    }
    *nbtr = 3;

/*     recherche du premier milieu a creer */
    for (i__ = 1; i__ <= 3; ++i__) {
	if (milieu[i__] != 0) {
	    goto L9;
	}
/* L7: */
    }

/*     on se ramene au seul cas i=2 c-a-d le cote 1 n'a pas de milieu */
L9:
    if (i__ == 2) {
/*        pas de milieu sur le cote 1 */
	nosotr[0] = letree[6];
	nosotr[1] = letree[7];
	nosotr[2] = letree[8];
/*        le numero pxyd du milieu du cote 2 */
	nm2 = milieu[2];
/*        le numero pxyd du milieu du cote 3 */
	nm3 = milieu[3];
    } else if (milieu[2] != 0) {
/*        pas de milieu sur le cote 3 */
	nosotr[0] = letree[8];
	nosotr[1] = letree[6];
	nosotr[2] = letree[7];
/*        le numero pxyd du milieu du cote 2 */
	nm2 = milieu[1];
/*        le numero pxyd du milieu du cote 3 */
	nm3 = milieu[2];
    } else {
/*        pas de milieu sur le cote 2 */
	nosotr[0] = letree[7];
	nosotr[1] = letree[8];
	nosotr[2] = letree[6];
/*        le numero pxyd du milieu du cote 2 */
	nm2 = milieu[3];
/*        le numero pxyd du milieu du cote 3 */
	nm3 = milieu[1];
    }

/*     ici seul le cote 1 n'a pas de milieu */
/*     nm2 est le milieu du cote 2 */
/*     nm3 est le milieu du cote 3 */

/*     ajout eventuel de l'arete s1 s2 dans nosoar */
    fasoar_(nosotr, &nosotr[1], &nutr[1], &c_n1, &c__0, mosoar, mxsoar, 
	    n1soar, &nosoar[nosoar_offset], &noarst[1], nuarco, ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete s1 s2 dans nosoar */
    fasoar_(&nosotr[1], &nm2, &nutr[1], &c_n1, &c__0, mosoar, mxsoar, n1soar, 
	    &nosoar[nosoar_offset], &noarst[1], &nuarco[1], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete s1 nm2 dans nosoar */
    fasoar_(nosotr, &nm2, &nutr[1], &nutr[2], &c__0, mosoar, mxsoar, n1soar, &
	    nosoar[nosoar_offset], &noarst[1], &nuarco[2], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete nm2 nm3 dans nosoar */
    fasoar_(&nm3, &nm2, &nutr[2], &nutr[3], &c__0, mosoar, mxsoar, n1soar, &
	    nosoar[nosoar_offset], &noarst[1], &nuarco[3], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete s1 nm3 dans nosoar */
    fasoar_(nosotr, &nm3, &nutr[2], &c_n1, &c__0, mosoar, mxsoar, n1soar, &
	    nosoar[nosoar_offset], &noarst[1], &nuarco[4], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     ajout eventuel de l'arete nm2 s3 dans nosoar */
    fasoar_(&nm2, &nosotr[2], &nutr[3], &c_n1, &c__0, mosoar, mxsoar, n1soar, 
	    &nosoar[nosoar_offset], &noarst[1], &nuarco[5], ierr);

/*     ajout eventuel de l'arete nm3 s3 dans nosoar */
    fasoar_(&nosotr[2], &nm3, &nutr[3], &c_n1, &c__0, mosoar, mxsoar, n1soar, 
	    &nosoar[nosoar_offset], &noarst[1], &nuarco[6], ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     le triangle s1 s2 nm2  ou arete1 arete2 arete3 */
    for (i__ = 1; i__ <= 2; ++i__) {
/*        nosotr(i) est le numero du sommet 1 de l'arete i du te */
	if (nosotr[i__ - 1] == nosoar[nuarco[i__ - 1] * nosoar_dim1 + 1]) {
	    lesign = 1;
	} else {
	    lesign = -1;
	}
/*        l'arete ns1-ns2 dans nosoar est celle du cote du te */
	noartr[i__ + nutr[1] * noartr_dim1] = lesign * nuarco[i__ - 1];
/* L20: */
    }
    if (nm2 == nosoar[nuarco[2] * nosoar_dim1 + 1]) {
	lesign = 1;
    } else {
	lesign = -1;
    }
    noartr[nutr[1] * noartr_dim1 + 3] = lesign * nuarco[2];

/*     le triangle s1 nm2 nm3 */
    noartr[nutr[2] * noartr_dim1 + 1] = -lesign * nuarco[2];
    if (nm2 == nosoar[nuarco[3] * nosoar_dim1 + 1]) {
	lesign = 1;
    } else {
	lesign = -1;
    }
    noartr[nutr[2] * noartr_dim1 + 2] = lesign * nuarco[3];
    noartr[nutr[3] * noartr_dim1 + 1] = -lesign * nuarco[3];
    if (nm3 == nosoar[nuarco[4] * nosoar_dim1 + 1]) {
	lesign = 1;
    } else {
	lesign = -1;
    }
    noartr[nutr[2] * noartr_dim1 + 3] = lesign * nuarco[4];

/*     le triangle nm2 nm3 s3 */
    if (nm2 == nosoar[nuarco[5] * nosoar_dim1 + 1]) {
	lesign = 1;
    } else {
	lesign = -1;
    }
    noartr[nutr[3] * noartr_dim1 + 2] = lesign * nuarco[5];
    if (nm3 == nosoar[nuarco[6] * nosoar_dim1 + 1]) {
	lesign = -1;
    } else {
	lesign = 1;
    }
    noartr[nutr[3] * noartr_dim1 + 3] = lesign * nuarco[6];

/*     triangulation des 3 sous-te par ajout des points internes du te */
    trpite_(letree, &pxyd[4], mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], 
	    moartr, mxartr, n1artr, &noartr[noartr_offset], &noarst[1], nbtr, 
	    &nutr[1], ierr);
    return 0;
} /* f2trte_ */

/* Subroutine */ int f3trte_(integer *letree, doublereal *pxyd, integer *
	milieu, integer *mosoar, integer *mxsoar, integer *n1soar, integer *
	nosoar, integer *moartr, integer *mxartr, integer *n1artr, integer *
	noartr, integer *noarst, integer *nbtr, integer *nutr, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset;

    /* Local variables */
    static integer i__, i0, i1, i3;
    extern /* Subroutine */ int fasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *);
    static integer lesign, nuarco[9];
    extern /* Subroutine */ int trpite_(integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former les triangles du triangle equilateral letree */
/* -----    a partir de 3 milieux des cotes du te */
/*          et des points internes au te */
/*          ils deviennent tous des sommets des sous-triangles du te */

/* entrees: */
/* -------- */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          si letree(0)>0 alors */
/*             letree(0:3) : no (>0) letree des 4 sous-triangles du triangle j */
/*          sinon */
/*             letree(0:3) :-no pxyd des 1 a 4 points internes au triangle j */
/*                           0  si pas de point */
/*                         ( le te est une feuille de l'arbre ) */
/*          letree(4) : no letree du sur-triangle du triangle j */
/*          letree(5) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8) : no pxyd des 3 sommets du triangle j */
/* pxyd   : tableau des x  y  distance_souhaitee de chaque sommet */
/* milieu : milieu(i) numero dans pxyd du milieu de l'arete i du te */
/*                    0 si pas de milieu du cote i a ajouter */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = (nosoar(1)+nosoar(2)) modulo mxsoar */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(np) numero d'une arete du sommet np */

/* sorties: */
/* -------- */
/* nbtr   : nombre de sous-triangles du te, triangulation du te */
/* nutr   : numero des nbtr sous-triangles du te dans le tableau noartr */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/*          =3 si aucun des triangles ne contient l'un des points internes au te */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */


/*     le numero des 4 triangles a creer dans le tableau noartr */
    /* Parameter adjustments */
    pxyd -= 4;
    --milieu;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --nutr;

    /* Function Body */
    for (*nbtr = 1; *nbtr <= 4; ++(*nbtr)) {
	if (*n1artr <= 0) {
/*           tableau noartr sature */
	    *ierr = 2;
	    return 0;
	}
	nutr[*nbtr] = *n1artr;
/*        le nouveau premier triangle libre dans noartr */
	*n1artr = noartr[*n1artr * noartr_dim1 + 2];
/* L5: */
    }
    *nbtr = 4;

    for (i__ = 1; i__ <= 3; ++i__) {
/*        le sommet suivant */
	if (i__ != 3) {
	    i1 = i__ + 1;
	} else {
	    i1 = 1;
	}
/*        le sommet precedant */
	if (i__ != 1) {
	    i0 = i__ - 1;
	} else {
	    i0 = 3;
	}
	i3 = i__ * 3;

/*        ajout eventuel de l'arete si mi dans nosoar */
	fasoar_(&letree[i__ + 5], &milieu[i__], &nutr[i__], &c_n1, &c__0, 
		mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], &
		nuarco[i3 - 3], ierr);
	if (*ierr != 0) {
	    return 0;
	}

/*        ajout eventuel de l'arete mi mi-1 dans nosoar */
	fasoar_(&milieu[i__], &milieu[i0], &nutr[i__], &nutr[4], &c__0, 
		mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], &
		nuarco[i3 - 2], ierr);
	if (*ierr != 0) {
	    return 0;
	}

/*        ajout eventuel de l'arete m i-1  si dans nosoar */
	fasoar_(&milieu[i0], &letree[i__ + 5], &nutr[i__], &c_n1, &c__0, 
		mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[1], &
		nuarco[i3 - 1], ierr);
	if (*ierr != 0) {
	    return 0;
	}

/* L10: */
    }

/*     les 3 sous-triangles pres des sommets */
    for (i__ = 1; i__ <= 3; ++i__) {
/*        le sommet suivant */
	if (i__ != 3) {
	    i1 = i__ + 1;
	} else {
	    i1 = 1;
	}
/*        le sommet precedant */
	if (i__ != 1) {
	    i0 = i__ - 1;
	} else {
	    i0 = 3;
	}
	i3 = i__ * 3;

/*        ajout du triangle  arete3i-2 arete3i-1 arete3i */
	if (letree[i__ + 5] == nosoar[nuarco[i3 - 3] * nosoar_dim1 + 1]) {
	    lesign = 1;
	} else {
	    lesign = -1;
	}
	noartr[nutr[i__] * noartr_dim1 + 1] = lesign * nuarco[i3 - 3];

	if (milieu[i__] == nosoar[nuarco[i3 - 2] * nosoar_dim1 + 1]) {
	    lesign = 1;
	} else {
	    lesign = -1;
	}
	noartr[nutr[i__] * noartr_dim1 + 2] = lesign * nuarco[i3 - 2];

	if (milieu[i0] == nosoar[nuarco[i3 - 1] * nosoar_dim1 + 1]) {
	    lesign = 1;
	} else {
	    lesign = -1;
	}
	noartr[nutr[i__] * noartr_dim1 + 3] = lesign * nuarco[i3 - 1];

/* L20: */
    }

/*     le sous triangle central */
    i3 = -1;
    for (i__ = 1; i__ <= 3; ++i__) {
	i3 += 3;
	if (milieu[i__] == nosoar[nuarco[i3 - 1] * nosoar_dim1 + 1]) {
	    lesign = -1;
	} else {
	    lesign = 1;
	}
	noartr[i__ + nutr[4] * noartr_dim1] = lesign * nuarco[i3 - 1];
/* L30: */
    }

/*     triangulation des 3 sous-te par ajout des points internes du te */
    trpite_(letree, &pxyd[4], mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], 
	    moartr, mxartr, n1artr, &noartr[noartr_offset], &noarst[1], nbtr, 
	    &nutr[1], ierr);
    return 0;
} /* f3trte_ */

/* Subroutine */ int hasoar_(integer *mosoar, integer *mxsoar, integer *
	n1soar, integer *nosoar, integer *nu2sar, integer *noar)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset;

    /* Local variables */
    static integer i__;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    rechercher le numero des 2 sommets d'une arete parmi */
/* -----    les numeros des 2 sommets des aretes du tableau nosoar */
/*          s ils n y sont pas stockes les y ajouter */
/*          dans tous les cas retourner le numero de l'arete dans nosoar */

/*          la methode employee ici est celle du hachage */
/*          avec pour fonction d'adressage h(ns1,ns2)=min(ns1,ns2) */

/*          remarque: h(ns1,ns2)=ns1 + 2*ns2 */
/*                    ne marche pas si des aretes sont detruites */
/*                    et ajoutees aux aretes vides */
/*                    le chainage est commun a plusieurs hachages! */
/*                    d'ou ce choix du minimum pour le hachage */

/* entrees: */
/* -------- */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/*          chainage des aretes vides amont et aval */
/*          l'arete vide qui precede=nosoar(4,i) */
/*          l'arete vide qui suit   =nosoar(5,i) */
/* nosoar : numero des 2 sommets, no ligne, 2 triangles de l'arete, */
/*          chainage momentan'e d'aretes, chainage du hachage des aretes */
/*          hachage des aretes = min( nosoar(1), nosoar(2) ) */
/* nu2sar : en entree les 2 numeros des sommets de l'arete */
/*          en sortie nu2sar(1)<nu2sar(2) numeros des 2 sommets de l'arete */

/* sorties: */
/* -------- */
/* noar   : numero dans nosoar de l'arete apres hachage */
/*          =0 si saturation du tableau nosoar */
/*          >0 si le tableau nu2sar est l'arete noar retrouvee */
/*             dans le tableau nosoar */
/*          <0 si le tableau nu2sar a ete ajoute et forme l'arete */
/*             -noar du tableau nosoar avec nosoar(1,noar)<nosoar(2,noar) */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique upmc paris       mars 1997 */
/* ...................................................................012 */

    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    --nu2sar;

    /* Function Body */
    if (nu2sar[1] > nu2sar[2]) {

/*        permutation des numeros des 2 sommets pour */
/*        amener le plus petit dans nu2sar(1) */
	i__ = nu2sar[1];
	nu2sar[1] = nu2sar[2];
	nu2sar[2] = i__;
    }

/*     la fonction d'adressage du hachage des aretes : h(ns1,ns2)=min(ns1,ns2) */
/*     =============================================== */
    *noar = nu2sar[1];

/*     la recherche de l'arete dans le chainage du hachage */
/*     --------------------------------------------------- */
L10:
    if (nu2sar[1] == nosoar[*noar * nosoar_dim1 + 1]) {
	if (nu2sar[2] == nosoar[*noar * nosoar_dim1 + 2]) {

/*           l'arete est retrouvee */
/*           ..................... */
	    return 0;
	}
    }

/*     l'arete suivante parmi celles ayant meme fonction d'adressage */
    i__ = nosoar[*mosoar + *noar * nosoar_dim1];
    if (i__ > 0) {
	*noar = i__;
	goto L10;
    }

/*     noar est ici la derniere arete (sans suivante) du chainage */
/*     a partir de l'adressage du hachage */

/*     l'arete non retrouvee doit etre ajoutee */
/*     ....................................... */
    if (nosoar[nu2sar[1] * nosoar_dim1 + 1] == 0) {

/*        l'adresse de hachage est libre => elle devient la nouvelle arete */
/*        retouche des chainages de cette arete noar qui ne sera plus vide */
	*noar = nu2sar[1];
/*        l'eventuel chainage du hachage n'est pas modifie */

    } else {

/*        la premiere arete dans l'adressage du hachage n'est pas libre */
/*        => choix quelconque d'une arete vide pour ajouter cette arete */
	if (*n1soar <= 0) {

/*           le tableau nosoar est sature avec pour temoin d'erreur */
	    *noar = 0;
	    return 0;

	} else {

/*           l'arete n1soar est vide => c'est la nouvelle arete */
/*           mise a jour du chainage de la derniere arete noar du chainage */
/*           sa suivante est la nouvelle arete n1soar */
	    nosoar[*mosoar + *noar * nosoar_dim1] = *n1soar;

/*           l'arete ajoutee est n1soar */
	    *noar = *n1soar;

/*           la nouvelle premiere arete vide */
	    *n1soar = nosoar[*n1soar * nosoar_dim1 + 5];

/*           la premiere arete vide n1soar n'a pas d'arete vide precedente */
	    nosoar[*n1soar * nosoar_dim1 + 4] = 0;

/*           noar la nouvelle arete est la derniere du chainage du hachage */
	    nosoar[*mosoar + *noar * nosoar_dim1] = 0;

	}

    }

/*     les 2 sommets de la nouvelle arete noar */
    nosoar[*noar * nosoar_dim1 + 1] = nu2sar[1];
    nosoar[*noar * nosoar_dim1 + 2] = nu2sar[2];

/*     le tableau nu2sar a ete ajoute avec l'indice -noar */
    *noar = -(*noar);
    return 0;
} /* hasoar_ */

/* Subroutine */ int mt3str_(integer *nt, integer *moartr, integer *noartr, 
	integer *mosoar, integer *nosoar, integer *ns1, integer *ns2, integer 
	*ns3)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer na;

    /* Fortran I/O blocks */
    static cilist io___481 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but : calcul du numero des 3 sommets du triangle nt du tableau noartr */
/* ----- */

/* entrees: */
/* -------- */
/* nt     : numero du triangle de noartr a traiter */
/* moartr : nombre maximal d'entiers par triangle */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1=0 si triangle vide => arete2=triangle vide suivant */
/* mosoar : nombre maximal d'entiers par arete */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles, chainages en + */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */

/* sorties: */
/* -------- */
/* ns1,ns2,ns3 : les 3 numeros des sommets du triangle en sens direct */

/* si erreur rencontree => ns1 = 0 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    juillet 1995 */
/* 2345x7..............................................................012 */

/*     le numero de triangle est il correct  ? */
/*     a supprimer apres mise au point */
    /* Parameter adjustments */
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;

    /* Function Body */
    if (*nt <= 0) {
/*         nblgrc(nrerr) = 1 */
/*         write(kerr(mxlger)(1:6),'(i6)') nt */
/*         kerr(1) = kerr(mxlger)(1:6) // */
/*     %           ' no triangle dans noartr incorrect' */
/*         call lereur */
	//io___481.ciunit = unites_1.imprim;
	//s_wsle(&io___481);
	//do_lio(&c__3, &c__1, (char *)&(*nt), (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " no triangle dans noartr incorrect", (ftnlen)34)
	//	;
	//e_wsle();
	*ns1 = 0;
	return 0;
    }

    na = noartr[*nt * noartr_dim1 + 1];
    if (na > 0) {
/*        arete dans le sens direct */
	*ns1 = nosoar[na * nosoar_dim1 + 1];
	*ns2 = nosoar[na * nosoar_dim1 + 2];
    } else {
/*        arete dans le sens indirect */
	*ns1 = nosoar[-na * nosoar_dim1 + 2];
	*ns2 = nosoar[-na * nosoar_dim1 + 1];
    }

    na = noartr[*nt * noartr_dim1 + 2];
    if (na > 0) {
/*        arete dans le sens direct => ns3 est le second sommet de l'arete */
	*ns3 = nosoar[na * nosoar_dim1 + 2];
    } else {
/*        arete dans le sens indirect => ns3 est le premier sommet de l'arete */
	*ns3 = nosoar[-na * nosoar_dim1 + 1];
    }
    return 0;
} /* mt3str_ */

/* Subroutine */ int trpite_(integer *letree, doublereal *pxyd, integer *
	mosoar, integer *mxsoar, integer *n1soar, integer *nosoar, integer *
	moartr, integer *mxartr, integer *n1artr, integer *noartr, integer *
	noarst, integer *nbtr, integer *nutr, integer *ierr)
{
    /* Format strings */
    static char fmt_10010[] = "(\002 erreur trpite: pas de triangle contenan"
	    "t le point\002,i7)";

    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static integer k, n, np, nt;
    extern /* Subroutine */ int tr3str_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);
    static integer nsigne;
    extern /* Subroutine */ int ptdatr_(doublereal *, doublereal *, integer *,
	     integer *);
    static integer nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___489 = { 0, 0, 0, fmt_10010, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former le ou les sous-triangles des nbtr triangles nutr */
/* -----    qui forment le triangle equilateral letree par ajout */
/*          des points internes au te qui deviennent des sommets des */
/*          sous-triangles des nbtr triangles */

/* entrees: */
/* -------- */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*          letree(0:3):-no pxyd des 1 a 4 points internes au triangle j */
/*                       0  si pas de point */
/*                     ( le te est ici une feuille de l'arbre ) */
/*          letree(4) : no letree du sur-triangle du triangle j */
/*          letree(5) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*          letree(6:8) : no pxyd des 3 sommets du triangle j */
/* pxyd   : tableau des x  y  distance_souhaitee de chaque sommet */
/* mosoar : nombre maximal d'entiers par arete du tableau nosoar */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = (nosoar(1)+nosoar(2)) modulo mxsoar */
/*          sommet 1 = 0 si arete vide => sommet 2 = arete vide suivante */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero d'une arete de sommet i */

/* sorties: */
/* -------- */
/* nbtr   : nombre de sous-triangles du te */
/* nutr   : numero des nbtr sous-triangles du te dans le tableau noartr */
/* ierr   : =0 si pas d'erreur */
/*          =1 si le tableau nosoar est sature */
/*          =2 si le tableau noartr est sature */
/*          =3 si aucun des triangles ne contient l'un des points internes au te */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* ....................................................................012 */


    /* Parameter adjustments */
    pxyd -= 4;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --nutr;

    /* Function Body */
    *ierr = 0;

/*     si pas de point interne alors retour */
    if (letree[0] == 0) {
	goto L150;
    }

/*     il existe au moins un point interne a trianguler */
/*     dans les nbtr triangles */
    for (k = 0; k <= 3; ++k) {

/*        le numero du point */
	np = -letree[k];
	if (np == 0) {
	    goto L150;
	}

/*        le point np dans pxyd est a traiter */
	i__1 = *nbtr;
	for (n = 1; n <= i__1; ++n) {

/*           les numeros des 3 sommets du triangle nt=nutr(n) */
	    nt = nutr[n];
	    nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		    noartr_offset], nosotr);

/*           le triangle nt contient il le point np? */
	    ptdatr_(&pxyd[np * 3 + 1], &pxyd[4], nosotr, &nsigne);
/*           nsigne>0 si le point est dans le triangle ou sur une des 3 aretes */
/*                 =0 si triangle degenere ou indirect ou ne contient pas le poin */

	    if (nsigne > 0) {

/*              le triangle nt est triangule en 3 sous-triangles */
		tr3str_(&np, &nt, mosoar, mxsoar, n1soar, &nosoar[
			nosoar_offset], moartr, mxartr, n1artr, &noartr[
			noartr_offset], &noarst[1], &nutr[*nbtr + 1], ierr);
		if (*ierr != 0) {
		    return 0;
		}

/*              reamenagement des 3 triangles crees dans nutr */
/*              en supprimant le triangle nt */
		nutr[n] = nutr[*nbtr + 3];
		*nbtr += 2;
/*              le point np est triangule */
		goto L100;

	    }
/* L10: */
	}

/*        erreur: le point np n'est pas dans l'un des nbtr triangles */
	//io___489.ciunit = unites_1.imprim;
	//s_wsfe(&io___489);
	//do_fio(&c__1, (char *)&np, (ftnlen)sizeof(integer));
	//e_wsfe();
	*ierr = 3;
	return 0;

L100:
	;
    }

L150:
    return 0;
} /* trpite_ */

/* Subroutine */ int sasoar_(integer *noar, integer *mosoar, integer *mxsoar, 
	integer *n1soar, integer *nosoar, integer *noarst)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, i__1;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, k, ns[2], noar0, noar1;

    /* Fortran I/O blocks */
    static cilist io___495 = { 0, 0, 0, 0, 0 };
    static cilist io___496 = { 0, 0, 0, 0, 0 };
    static cilist io___497 = { 0, 0, 0, 0, 0 };


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    supprimer l'arete noar du tableau nosoar */
/* -----    si celle ci n'est pas une arete des lignes de la fontiere */

/*          la methode employee ici est celle du hachage */
/*          avec pour fonction d'adressage h = min( nu2sar(1), nu2sar(2) ) */

/*          attention: il faut mettre a jour le no d'arete des 2 sommets */
/*                     de l'arete supprimee dans le tableau noarst! */

/* entrees: */
/* -------- */
/* noar   : numero de l'arete de nosoar a supprimer */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage h */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */

/* modifies: */
/* --------- */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(4,arete vide)=l'arete vide qui precede */
/*          nosoar(5,arete vide)=l'arete vide qui suit */
/* noarst : numero d'une arete de nosoar pour chaque sommet */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique    upmc paris  mars    1997 */
/* modifs : alain perronnet laboratoire jl lions upmc paris  octobre 2006 */
/* ...................................................................012 */

/*     13/10/2006 */
/*     mise a jour de noarst pour les 2 sommets de l'arete a supprimer */
/*     necessaire uniquement pour les sommets frontaliers et internes imposes */
/*     le numero des 2 sommets de l'arete noar a supprimer */
    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    --noarst;

    /* Function Body */
    ns[0] = nosoar[*noar * nosoar_dim1 + 1];
    ns[1] = nosoar[*noar * nosoar_dim1 + 2];
    for (k = 1; k <= 2; ++k) {
	if (noarst[ns[k - 1]] == *noar) {
/*           il faut remettre a jour le pointeur sur une arete */
	    if (nosoar[ns[k - 1] * nosoar_dim1 + 1] == ns[k - 1] && nosoar[ns[
		    k - 1] * nosoar_dim1 + 2] > 0 && nosoar[ns[k - 1] * 
		    nosoar_dim1 + 4] > 0) {
/*              arete active de sommet ns(k) */
		noarst[ns[k - 1]] = ns[k - 1];
	    } else {
		i__1 = *mxsoar;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    if (nosoar[i__ * nosoar_dim1 + 1] > 0 && nosoar[i__ * 
			    nosoar_dim1 + 4] > 0) {
/*                    arete non vide */
			if  (nosoar[i__ * nosoar_dim1 + 2] == ns[k - 1] || 
				(nosoar[i__ * nosoar_dim1 + 1] == ns[k - 1] && 
				 nosoar[i__ * nosoar_dim1 + 2] > 0)) {
/*                       arete active de sommet ns(k) */
			    noarst[ns[k - 1]] = i__;
			    goto L8;
			}
		    }
/* L5: */
		}
	    }
	}
L8:
	;
    }
/*     13/10/2006 */

    if (nosoar[*noar * nosoar_dim1 + 3] <= 0) {

/*        l'arete n'est pas frontaliere => elle devient une arete vide */

/*        recherche de l'arete qui precede dans le chainage du hachage */
	noar1 = nosoar[*noar * nosoar_dim1 + 1];

/*        parcours du chainage du hachage jusqu'a retrouver l'arete noar */
L10:
	if (noar1 != *noar) {

/*           l'arete suivante parmi celles ayant meme fonction d'adressage */
	    noar0 = noar1;
	    noar1 = nosoar[*mosoar + noar1 * nosoar_dim1];
	    if (noar1 > 0) {
		goto L10;
	    }

/*           l'arete noar n'a pas ete retrouvee dans le chainage => erreur */
	 //   io___495.ciunit = unites_1.imprim;
	 //   s_wsle(&io___495);
	 //   do_lio(&c__9, &c__1, "erreur sasoar:arete non dans le chainage ", 
		//    (ftnlen)41);
	 //   do_lio(&c__3, &c__1, (char *)&(*noar), (ftnlen)sizeof(integer));
	 //   e_wsle();
	 //   io___496.ciunit = unites_1.imprim;
	 //   s_wsle(&io___496);
	 //   do_lio(&c__9, &c__1, "arete de st1=", (ftnlen)13);
	 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 1], (
		//    ftnlen)sizeof(integer));
	 //   do_lio(&c__9, &c__1, " st2=", (ftnlen)5);
	 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 2], (
		//    ftnlen)sizeof(integer));
	 //   do_lio(&c__9, &c__1, " ligne=", (ftnlen)7);
	 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 3], (
		//    ftnlen)sizeof(integer));
	 //   do_lio(&c__9, &c__1, " tr1=", (ftnlen)5);
	 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 4], (
		//    ftnlen)sizeof(integer));
	 //   do_lio(&c__9, &c__1, " tr2=", (ftnlen)5);
	 //   do_lio(&c__3, &c__1, (char *)&nosoar[*noar * nosoar_dim1 + 5], (
		//    ftnlen)sizeof(integer));
	 //   e_wsle();
	 //   io___497.ciunit = unites_1.imprim;
	 //   s_wsle(&io___497);
	 //   do_lio(&c__9, &c__1, "chainages=", (ftnlen)10);
	 //   i__1 = *mosoar;
	 //   for (i__ = 6; i__ <= i__1; ++i__) {
		//do_lio(&c__3, &c__1, (char *)&nosoar[i__ + *noar * 
		//	nosoar_dim1], (ftnlen)sizeof(integer));
	 //   }
	 //   e_wsle();
/* cc            pause */
/*           l'arete n'est pas detruite */
	    return 0;

	}

	if (*noar != nosoar[*noar * nosoar_dim1 + 1]) {

/*           saut de l'arete noar dans le chainage du hachage */
/*           noar0 initialisee est ici l'arete qui precede noar dans ce chainage */
	    nosoar[*mosoar + noar0 * nosoar_dim1] = nosoar[*mosoar + *noar * 
		    nosoar_dim1];

/*           le chainage du hachage n'existe plus pour noar */
/*           pas utile car mise a zero faite dans le sp hasoar */
/* cc         nosoar( mosoar, noar ) = 0 */

/*           noar devient la nouvelle premiere arete du chainage des vides */
	    nosoar[*noar * nosoar_dim1 + 4] = 0;
	    nosoar[*noar * nosoar_dim1 + 5] = *n1soar;
/*           la nouvelle precede l'ancienne premiere */
	    nosoar[*n1soar * nosoar_dim1 + 4] = *noar;
	    *n1soar = *noar;

/* cc      else */

/*           noar est la premiere arete du chainage du hachage h */
/*           cette arete ne peut etre consideree dans le chainage des vides */
/*           car le chainage du hachage doit etre conserve (sinon perte...) */

	}

/*        le temoin d'arete vide */
	nosoar[*noar * nosoar_dim1 + 1] = 0;
    }
    return 0;
} /* sasoar_ */

/* Subroutine */ int caetoi_(integer *noar, integer *mosoar, integer *mxsoar, 
	integer *n1soar, integer *nosoar, integer *noarst, integer *n1aeoc, 
	integer *nbtrar)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer na, na0, nbpass;
    extern /* Subroutine */ int sasoar_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___501 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    ajouter (ou retirer) l'arete noar de nosoar de l'etoile */
/* -----    des aretes simples chainees en position lchain de nosoar */
/*          detruire du tableau nosoar les aretes doubles */

/*          attention: le chainage lchain de nosoar devient celui des cf */

/* entree : */
/* -------- */
/* noar   : numero dans le tableau nosoar de l'arete a traiter */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */

/* entrees et sorties: */
/* ------------------- */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/* n1aeoc : numero dans nosoar de la premiere arete simple de l'etoile */

/* sortie : */
/* -------- */
/* nbtrar : 1 si arete ajoutee, 2 si arete double supprimee, 0 si erreur */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc       mars 1997 */
/* 2345x7..............................................................012 */

/*     si    l'arete n'appartient pas aux aretes de l'etoile naetoi */
/*     alors elle est ajoutee a l'etoile dans naetoi */
/*     sinon elle est empilee dans npile pour etre detruite ensuite */
/*           elle est supprimee de l'etoile naetoi */

    /* Parameter adjustments */
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    --noarst;

    /* Function Body */
    if (nosoar[*noar * nosoar_dim1 + 6] < 0) {

/*        arete de l'etoile vue pour la premiere fois */
/*        elle est ajoutee au chainage */
	nosoar[*noar * nosoar_dim1 + 6] = *n1aeoc;
/*        elle devient la premiere du chainage */
	*n1aeoc = *noar;
/*        arete simple */
	*nbtrar = 1;

    } else {

/*        arete double de l'etoile. elle est supprimee du chainage */
	na0 = 0;
	na = *n1aeoc;
	nbpass = 0;
/*        parcours des aretes chainees jusqu'a trouver l'arete noar */
L10:
	if (na != *noar) {
/*           passage a la suivante */
	    na0 = na;
	    na = nosoar[na * nosoar_dim1 + 6];
	    if (na <= 0) {
		*nbtrar = 0;
		return 0;
	    }
	    ++nbpass;
	    if (nbpass > 512) {
		//io___501.ciunit = unites_1.imprim;
		//s_wsle(&io___501);
		//do_lio(&c__9, &c__1, "Pb dans caetoi: boucle infinie evitee", 
		//	(ftnlen)37);
		//e_wsle();
		*nbtrar = 0;
		return 0;
	    }
	    goto L10;
	}

/*        suppression de noar du chainage des aretes simples de l'etoile */
	if (na0 > 0) {
/*           il existe une arete qui precede */
	    nosoar[na0 * nosoar_dim1 + 6] = nosoar[*noar * nosoar_dim1 + 6];
	} else {
/*           noar est en fait n1aeoc la premiere du chainage */
	    *n1aeoc = nosoar[*noar * nosoar_dim1 + 6];
	}
/*        noar n'est plus une arete simple de l'etoile */
	nosoar[*noar * nosoar_dim1 + 6] = -1;

/*        destruction du tableau nosoar de l'arete double noar */
	sasoar_(noar, mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &noarst[
		1]);

/*        arete double */
	*nbtrar = 2;
    }
    return 0;
} /* caetoi_ */

/* Subroutine */ int focftr_(integer *nbtrcf, integer *notrcf, integer *
	nbarpi, doublereal *pxyd, integer *noarst, integer *mosoar, integer *
	mxsoar, integer *n1soar, integer *nosoar, integer *moartr, integer *
	n1artr, integer *noartr, integer *nbarcf, integer *n1arcf, integer *
	noarcf, integer *nbstpe, integer *nostpe, integer *ierr)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1, 
	    i__2;

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, j, k, n, kk, nt, na0, na1, ns0, ns1, ns2, nt0, noar, 
	    nbst, n1ae00, n1aeoc;
    extern /* Subroutine */ int caetoi_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *);
    static integer nbtrar, nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___511 = { 0, 0, 0, 0, 0 };
    static cilist io___512 = { 0, 0, 0, 0, 0 };
    static cilist io___513 = { 0, 0, 0, 0, 0 };
    static cilist io___519 = { 0, 0, 0, 0, 0 };
    static cilist io___521 = { 0, 0, 0, 0, 0 };
    static cilist io___523 = { 0, 0, 0, 0, 0 };
    static cilist io___524 = { 0, 0, 0, 0, 0 };
    static cilist io___525 = { 0, 0, 0, 0, 0 };
    static cilist io___526 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    former un contour ferme (cf) avec les aretes simples des */
/* -----    nbtrcf triangles du tableau notrcf */
/*          destruction des nbtrcf triangles du tableau noartr */
/*          destruction des aretes doubles   du tableau nosoar */

/*          attention: le chainage lchain de nosoar devient celui des cf */

/* entrees: */
/* -------- */
/* nbtrcf : nombre de  triangles du cf a former */
/* notrcf : numero des triangles dans le tableau noartr */
/* nbarpi : numero du dernier sommet frontalier ou interne impose */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */

/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */

/* entrees et sorties : */
/* -------------------- */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1soar : numero de la premiere arete vide dans le tableau nosoar */
/*          une arete i de nosoar est vide  <=>  nosoar(1,i)=0 */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* sorties: */
/* -------- */
/* nbarcf : nombre d'aretes du cf */
/* n1arcf : numero d'une arete de chaque contour */
/* noarcf : numero des aretes de la ligne du contour ferme */
/* attention: chainage circulaire des aretes */
/*            les aretes vides pointes par n1arcf(0) ne sont pas chainees */
/* nbstpe : nombre de  sommets perdus dans la suppression des triangles */
/* nostpe : numero des sommets perdus dans la suppression des triangles */
/* ierr   :  0 si pas d'erreur */
/*          14 si les lignes fermees se coupent => donnees a revoir */
/*          15 si une seule arete simple frontaliere */
/*          16 si boucle infinie car toutes les aretes simples */
/*                de la boule sont frontalieres! */
/*          17 si boucle infinie dans caetoi */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique    upmc paris  mars    1997 */
/* modifs : alain perronnet laboratoire jl lions upmc paris  octobre 2006 */
/* ....................................................................012 */

/*     formation des aretes simples du cf autour de l'arete ns1-ns2 */
/*     attention: le chainage lchain du tableau nosoar devient actif */
/*     ============================================================ */
/*     ici toutes les aretes du tableau nosoar verifient nosoar(lchain,i) = -1 */
/*     ce qui equivaut a dire que l'etoile des aretes simples est vide */
/*     (initialisation dans le sp insoar puis remise a -1 dans la suite!) */
    /* Parameter adjustments */
    --notrcf;
    pxyd -= 4;
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    noarcf -= 4;
    --nostpe;

    /* Function Body */
    n1aeoc = 0;
    *ierr = 0;

/*     13/10/2006 */
/*     nombre de sommets des triangles a supprimer sans repetition */
    nbst = 0;
/*     13/10/2006 */

/*     ajout a l'etoile des aretes simples des 3 aretes des triangles a supprimer */
/*     suppression des triangles de l'etoile pour les aretes simples de l'etoile */
    i__1 = *nbtrcf;
    for (i__ = 1; i__ <= i__1; ++i__) {

/*        ajout ou retrait des 3 aretes du triangle notrcf(i) de l'etoile */
	nt = notrcf[i__];

/*        13/10/2006  ............................................... */
	nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);

/*        ajout des numeros de sommets non encore vus dans l'etoile */
	for (k = 1; k <= 3; ++k) {
	    i__2 = nbst;
	    for (j = 1; j <= i__2; ++j) {
		if (nosotr[k - 1] == nostpe[j]) {
		    goto L3;
		}
/* L2: */
	    }
/*           ajout du sommet */
	    ++nbst;
	    nostpe[nbst] = nosotr[k - 1];
L3:
	    ;
	}
/*        13/10/2006 ................................................ */

	for (j = 1; j <= 3; ++j) {
/*           l'arete de nosoar a traiter */
	    noar = (i__2 = noartr[j + nt * noartr_dim1], abs(i__2));
	    caetoi_(&noar, mosoar, mxsoar, n1soar, &nosoar[nosoar_offset], &
		    noarst[1], &n1aeoc, &nbtrar);
	    if (nbtrar <= 0) {
		//io___511.ciunit = unites_1.imprim;
		//s_wsle(&io___511);
		//do_lio(&c__9, &c__1, "focftr: erreur dans caetoi noar=", (
		//	ftnlen)32);
		//do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(integer));
		//e_wsle();
		*ierr = 17;
		return 0;
	    }
/*           si arete simple alors suppression du numero de triangle */
/*           pour cette arete */
	    if (nbtrar == 1) {
		if (nosoar[noar * nosoar_dim1 + 4] == nt) {
		    nosoar[noar * nosoar_dim1 + 4] = nosoar[noar * 
			    nosoar_dim1 + 5];
		} else if (nosoar[noar * nosoar_dim1 + 5] == nt) {
		    nosoar[noar * nosoar_dim1 + 5] = -1;
		} else {
		 //   io___512.ciunit = unites_1.imprim;
		 //   s_wsle(&io___512);
		 //   do_lio(&c__9, &c__1, "focftr: anomalie arete", (ftnlen)22)
			//    ;
		 //   do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(
			//    integer));
		 //   do_lio(&c__9, &c__1, " sans triangle", (ftnlen)14);
		 //   do_lio(&c__3, &c__1, (char *)&nt, (ftnlen)sizeof(integer))
			//    ;
		 //   e_wsle();
		 //   io___513.ciunit = unites_1.imprim;
		 //   s_wsle(&io___513);
		 //   do_lio(&c__9, &c__1, "focftr: nosoar(", (ftnlen)15);
		 //   do_lio(&c__3, &c__1, (char *)&noar, (ftnlen)sizeof(
			//    integer));
		 //   do_lio(&c__9, &c__1, ")=", (ftnlen)2);
		 //   i__2 = *mosoar;
		 //   for (kk = 1; kk <= i__2; ++kk) {
			//do_lio(&c__3, &c__1, (char *)&nosoar[kk + noar * 
			//	nosoar_dim1], (ftnlen)sizeof(integer));
		 //   }
		 //   e_wsle();
		    nosoar[noar * nosoar_dim1 + 5] = -1;
		}
/*           else */
/*              l'arete appartient a aucun triangle => elle est vide */
/*              les positions 4 et 5 servent maintenant aux chainages des vides */
	    }
/* L5: */
	}
/* L10: */
    }

/*     les aretes simples de l'etoile sont reordonnees pour former une */
/*     ligne fermee = un contour ferme peripherique de l'etoile encore dit 1 cf */
/*     ======================================================================== */
    n1ae00 = n1aeoc;
L12:
    na1 = n1aeoc;
/*     la premiere arete du contour ferme */
    ns0 = nosoar[na1 * nosoar_dim1 + 1];
    ns1 = nosoar[na1 * nosoar_dim1 + 2];

/*     l'arete est-elle dans le sens direct? */
/*     recherche de l'arete du triangle exterieur nt d'arete na1 */
    nt = nosoar[na1 * nosoar_dim1 + 4];
    if (nt <= 0) {
	nt = nosoar[na1 * nosoar_dim1 + 5];
    }

/*     attention au cas de l'arete initiale frontaliere de no de triangles 0 et - */
    if (nt <= 0) {
/*        permutation circulaire des aretes simples chainees */
/*        la premiere arete doit devenir la derniere du chainage, */
/*        la 2=>1, la 3=>2, ... , la derniere=>l'avant derniere, 1=>derniere */
	n1aeoc = nosoar[n1aeoc * nosoar_dim1 + 6];
	if (n1aeoc == n1ae00) {
/*           attention: boucle infinie si toutes les aretes simples */
/*           de la boule sont frontalieres!... arretee par ce test */
	    *ierr = 16;
	    //io___519.ciunit = unites_1.imprim;
	    //s_wsle(&io___519);
	    //do_lio(&c__9, &c__1, "focftr: boucle dans les aretes de l etoile",
		   //  (ftnlen)42);
	    //e_wsle();
	    return 0;
	}
	noar = n1aeoc;
	na0 = 0;
L14:
	if (noar > 0) {
/*           la sauvegarde de l'arete et l'arete suivante */
	    na0 = noar;
	    noar = nosoar[noar * nosoar_dim1 + 6];
	    goto L14;
	}
	if (na0 <= 0) {
/*           une seule arete simple frontaliere */
	    *ierr = 15;
	    //io___521.ciunit = unites_1.imprim;
	    //s_wsle(&io___521);
	    //do_lio(&c__9, &c__1, "focftr: 1 arete seule pour l etoile", (
		   // ftnlen)35);
	    //e_wsle();
	    return 0;
	}
/*        le suivant de l'ancien dernier est l'ancien premier */
	nosoar[na0 * nosoar_dim1 + 6] = na1;
/*        le nouveau dernier est l'ancien premier */
	nosoar[na1 * nosoar_dim1 + 6] = 0;
	goto L12;
    }

/*     ici l'arete na1 est l'une des aretes du triangle nt */
    for (i__ = 1; i__ <= 3; ++i__) {
	if ((i__1 = noartr[i__ + nt * noartr_dim1], abs(i__1)) == na1) {
/*           c'est l'arete */
	    if (noartr[i__ + nt * noartr_dim1] > 0) {
/*              elle est parcourue dans le sens indirect de l'etoile */
/*             (car c'est en fait le triangle exterieur a la boule) */
		ns0 = nosoar[na1 * nosoar_dim1 + 2];
		ns1 = nosoar[na1 * nosoar_dim1 + 1];
	    }
	    goto L17;
	}
/* L15: */
    }

/*     le 1-er sommet ou arete du contour ferme */
L17:
    n1arcf[1] = 1;
/*     le nombre de sommets du contour ferme de l'etoile */
    *nbarcf = 1;
/*     le premier sommet de l'etoile */
    noarcf[*nbarcf * 3 + 1] = ns0;
/*     l'arete suivante du cf */
    noarcf[*nbarcf * 3 + 2] = *nbarcf + 1;
/*     le numero de cette arete dans le tableau nosoar */
    noarcf[*nbarcf * 3 + 3] = na1;
/*     mise a jour du numero d'arete du sommet ns0 */
    noarst[ns0] = na1;

/*     l'arete suivante a chainer */
    n1aeoc = nosoar[na1 * nosoar_dim1 + 6];
/*     l'arete na1 n'est plus dans l'etoile */
    nosoar[na1 * nosoar_dim1 + 6] = -1;

/*     boucle sur les aretes simples de l'etoile */
L20:
    if (n1aeoc > 0) {

/*        recherche de l'arete de 1-er sommet ns1 */
	na0 = -1;
	na1 = n1aeoc;
L25:
	if (na1 > 0) {

/*           le numero du dernier sommet de l'arete precedente */
/*           est il l'un des 2 sommets de l'arete na1? */
	    if (ns1 == nosoar[na1 * nosoar_dim1 + 1]) {
/*               l'autre sommet de l'arete na1 */
		ns2 = nosoar[na1 * nosoar_dim1 + 2];
	    } else if (ns1 == nosoar[na1 * nosoar_dim1 + 2]) {
/*               l'autre sommet de l'arete na1 */
		ns2 = nosoar[na1 * nosoar_dim1 + 1];
	    } else {
/*              non: passage a l'arete suivante */
		na0 = na1;
		na1 = nosoar[na1 * nosoar_dim1 + 6];
		goto L25;
	    }

/*           oui: na1 est l'arete peripherique suivante */
/*                na0 est sa precedente dans le chainage */
/*           une arete de plus dans le contour ferme (cf) */
	    ++(*nbarcf);
/*           le premier sommet de l'arete nbarcf peripherique */
	    noarcf[*nbarcf * 3 + 1] = ns1;
/*           l'arete suivante du cf */
	    noarcf[*nbarcf * 3 + 2] = *nbarcf + 1;
/*           le numero de cette arete dans le tableau nosoar */
	    noarcf[*nbarcf * 3 + 3] = na1;
/*           mise a jour du numero d'arete du sommet ns1 */
	    noarst[ns1] = na1;

/*           suppression de l'arete des aretes simples de l'etoile */
	    if (n1aeoc == na1) {
		n1aeoc = nosoar[na1 * nosoar_dim1 + 6];
	    } else {
		nosoar[na0 * nosoar_dim1 + 6] = nosoar[na1 * nosoar_dim1 + 6];
	    }
/*           l'arete n'est plus une arete simple de l'etoile */
	    nosoar[na1 * nosoar_dim1 + 6] = -1;

/*           le sommet final de l'arete a rechercher ensuite */
	    ns1 = ns2;
	    goto L20;
	}
    }

/*     verification */
    if (ns1 != ns0) {
/*        arete non retrouvee : l'etoile ne se referme pas */
	//io___523.ciunit = unites_1.imprim;
	//s_wsle(&io___523);
	//do_lio(&c__9, &c__1, "focftr: revoyez vos donnees du bord", (ftnlen)
	//	35);
	//e_wsle();
	//io___524.ciunit = unites_1.imprim;
	//s_wsle(&io___524);
	//do_lio(&c__9, &c__1, "les lignes fermees doivent etre disjointes", (
	//	ftnlen)42);
	//e_wsle();
	//io___525.ciunit = unites_1.imprim;
	//s_wsle(&io___525);
	//do_lio(&c__9, &c__1, "verifiez si elles ne se coupent pas", (ftnlen)
	//	35);
	//e_wsle();
	*ierr = 14;
	return 0;
    }

/*     l'arete suivant la derniere arete du cf est la premiere du cf */
/*     => realisation d'un chainage circulaire des aretes du cf */
    noarcf[*nbarcf * 3 + 2] = 1;

/*     13/10/2006 */
/*     existe t il des sommets perdus? */
/*     ------------------------------- */
    if (nbst > 512) {
	//io___526.ciunit = unites_1.imprim;
	//s_wsle(&io___526);
	//do_lio(&c__9, &c__1, "focftr: tableau nostfe(", (ftnlen)23);
	//do_lio(&c__3, &c__1, (char *)&c__512, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, ") a augmenter", (ftnlen)13);
	//e_wsle();
	//*ierr = 15;
	return 0;
    }
/*     le nombre de sommets perdus */
    *nbstpe = nbst - *nbarcf;
    if (*nbstpe > 0) {
/*        oui: stockage dans nostpe des sommets perdus */
/*        tout sommet des aretes de l'etoile est supprime */
/*        de la liste des sommets */
	i__1 = *nbarcf;
	for (i__ = 1; i__ <= i__1; ++i__) {
/*           le numero du sommet de l'arete du cf */
	    ns1 = noarcf[i__ * 3 + 1];
	    i__2 = nbst;
	    for (j = 1; j <= i__2; ++j) {
		if (ns1 == nostpe[j]) {
/*                 le sommet peripherique est supprime */
/*                 de la liste des sommets perdus */
		    nostpe[j] = 0;
		    goto L40;
		}
/* L30: */
	    }
L40:
	    ;
	}

/*        compression */
	n = 0;
	i__1 = nbst;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    if (nostpe[i__] == 0 || nostpe[i__] > *nbarpi) {
/*              un sommet de l'etoile ou perdu mais supprimable */
/*              ce qui apporte plus de qualites aux triangles a former */
		++n;
	    } else {
/*              un sommet perdu */
		nostpe[i__ - n] = nostpe[i__];
	    }
/* L45: */
	}
	*nbstpe = nbst - n;
/* cc      write(imprim,*)'focftr:',nbstpe,' sommets isoles:',(nostpe(k),k=1,nbstpe) */
    }
/*     13/10/2006 */

/*     destruction des triangles de l'etoile du tableau noartr */
/*     ------------------------------------------------------- */
    i__1 = *nbtrcf;
    for (n = 1; n <= i__1; ++n) {
/*        le numero du triangle dans noartr */
	nt0 = notrcf[n];
/*        l'arete 1 de nt0 devient nulle */
	noartr[nt0 * noartr_dim1 + 1] = 0;
/*        chainage de nt0 en tete du chainage des triangles vides de noartr */
	noartr[nt0 * noartr_dim1 + 2] = *n1artr;
	*n1artr = nt0;
/* L60: */
    }
    return 0;
} /* focftr_ */

/* Subroutine */ int int1sd_(integer *ns1, integer *ns2, integer *ns3, 
	integer *ns4, doublereal *pxyd, integer *linter, doublereal *x0, 
	doublereal *y0)
{
    /* System generated locals */
    doublereal d__1, d__2;

    /* Local variables */
    static doublereal d__, x, y, x1, y1, d21, d43, p21, p43, x21, y21, x43, 
	    y43;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    existence ou non  d'une intersection a l'interieur */
/* -----    des 2 aretes ns1-ns2 et ns3-ns4 */
/*          attention les intersections au sommet sont comptees */

/* entrees: */
/* -------- */
/* ns1,...ns4 : numero pxyd des 4 sommets */
/* pxyd   : les coordonnees des sommets */

/* sortie : */
/* -------- */
/* linter : -1 si ns3-ns4 parallele a ns1 ns2 */
/*           0 si ns3-ns4 n'intersecte pas ns1-ns2 entre les aretes */
/*           1 si ns3-ns4   intersecte     ns1-ns2 entre les aretes */
/*           2 si le point d'intersection est ns1  entre ns3-ns4 */
/*           3 si le point d'intersection est ns3  entre ns1-ns2 */
/*           4 si le point d'intersection est ns4  entre ns1-ns2 */
/* x0,y0  :  2 coordonnees du point d'intersection s'il existe(linter>=1) */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    fevrier 1992 */
/* 2345x7..............................................................012 */

    /* Parameter adjustments */
    pxyd -= 4;

    /* Function Body */
    x1 = pxyd[*ns1 * 3 + 1];
    y1 = pxyd[*ns1 * 3 + 2];
    x21 = pxyd[*ns2 * 3 + 1] - x1;
    y21 = pxyd[*ns2 * 3 + 2] - y1;
/* Computing 2nd power */
    d__1 = x21;
/* Computing 2nd power */
    d__2 = y21;
    d21 = d__1 * d__1 + d__2 * d__2;

    x43 = pxyd[*ns4 * 3 + 1] - pxyd[*ns3 * 3 + 1];
    y43 = pxyd[*ns4 * 3 + 2] - pxyd[*ns3 * 3 + 2];
/* Computing 2nd power */
    d__1 = x43;
/* Computing 2nd power */
    d__2 = y43;
    d43 = d__1 * d__1 + d__2 * d__2;

/*     les 2 aretes sont-elles jugees paralleles ? */
    d__ = x43 * y21 - y43 * x21;
    if (d__ * d__ <= d21 * 1e-6 * d43) {
/*        cote i parallele a ns1-ns2 */
	*linter = -1;
	return 0;
    }

/*     les 2 coordonnees du point d'intersection */
    x = (x1 * x43 * y21 - pxyd[*ns3 * 3 + 1] * x21 * y43 - (y1 - pxyd[*ns3 * 
	    3 + 2]) * x21 * x43) / d__;
    y = (-y1 * y43 * x21 + pxyd[*ns3 * 3 + 2] * y21 * x43 + (x1 - pxyd[*ns3 * 
	    3 + 1]) * y21 * y43) / d__;

/*     coordonnee barycentrique de x,y dans le repere ns1-ns2 */
    p21 = ((x - x1) * x21 + (y - y1) * y21) / d21;
/*     coordonnee barycentrique de x,y dans le repere ns3-ns4 */
    p43 = ((x - pxyd[*ns3 * 3 + 1]) * x43 + (y - pxyd[*ns3 * 3 + 2]) * y43) / 
	    d43;


    if (-1e-6f <= p21 && p21 <= 1.000001f) {
/*        x,y est entre ns1-ns2 */
	if (p21 <= .001f && (-1e-6f <= p43 && p43 <= 1.000001f)) {
/*           le point x,y est proche de ns1 et interne a ns3-ns4 */
	    *linter = 2;
	    *x0 = pxyd[*ns1 * 3 + 1];
	    *y0 = pxyd[*ns1 * 3 + 2];
	    return 0;
	} else if (-1e-6f <= p43 && p43 <= .001f) {
/*           le point x,y est proche de ns3 et entre ns1-ns2 */
	    *linter = 3;
	    *x0 = pxyd[*ns3 * 3 + 1];
	    *y0 = pxyd[*ns3 * 3 + 2];
	    return 0;
	} else if (.999f <= p43 && p43 <= 1.000001f) {
/*           le point x,y est proche de ns4 et entre ns1-ns2 */
	    *linter = 4;
	    *x0 = pxyd[*ns4 * 3 + 1];
	    *y0 = pxyd[*ns4 * 3 + 2];
	    return 0;
	} else if (.001f <= p43 && p43 <= .999f) {
/*           le point x,y est entre ns3-ns4 */
	    *linter = 1;
	    *x0 = x;
	    *y0 = y;
	    return 0;
	}
    }

/*     pas d'intersection a l'interieur des aretes */
    *linter = 0;
    return 0;
} /* int1sd_ */

/* Subroutine */ int tefoar_(integer *narete, integer *nbarpi, doublereal *
	pxyd, integer *mosoar, integer *mxsoar, integer *n1soar, integer *
	nosoar, integer *moartr, integer *mxartr, integer *n1artr, integer *
	noartr, integer *noarst, integer *mxarcf, integer *n1arcf, integer *
	noarcf, integer *larmin, integer *notrcf, integer *ierr)
{
    /* System generated locals */
    integer noartr_dim1, noartr_offset, nosoar_dim1, nosoar_offset, i__1;
    doublereal d__1, d__2;
    static integer equiv_1[2];

    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void), s_rsle(cilist *), e_rsle(void);
    double sqrt(doublereal);

    /* Local variables */
    static doublereal d__;
    static integer i__, j, k;
    static doublereal x, y, d3, d4;
    static integer i1, n1, n2;
    static doublereal x1, y1, x2, y2, d12;
    static integer na, ns[2], nt, na0, na1, na2, ns1, ns2, nt0, ns3, ns4, nt1,
	     na00, na01, nbt, nsp;
#define nacf (equiv_1)
    static integer nbcf;
    static doublereal dmin__;
    static integer ipas, noar;
#define nacf1 (equiv_1)
#define nacf2 (equiv_1 + 1)
    static integer nsens, nbtrc0;
    extern /* Subroutine */ int int1sd_(integer *, integer *, integer *, 
	    integer *, doublereal *, integer *, doublereal *, doublereal *);
    extern integer nopre3_(integer *), nosui3_(integer *);
    static integer nbarcf;
    extern /* Subroutine */ int trp1st_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *);
    static integer mmarcf;
    extern /* Subroutine */ int tridcf_(integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *);
    static integer nbtrcf;
    extern /* Subroutine */ int focftr_(integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *);
    static integer lapitr[32], linter, nbstpe, nostpe[512], nosotr[3];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___548 = { 0, 0, 0, 0, 0 };
    static cilist io___549 = { 0, 0, 0, 0, 0 };
    static cilist io___558 = { 0, 0, 0, 0, 0 };
    static cilist io___560 = { 0, 0, 0, 0, 0 };
    static cilist io___563 = { 0, 0, 0, 0, 0 };
    static cilist io___564 = { 0, 0, 0, 0, 0 };
    static cilist io___571 = { 0, 0, 0, 0, 0 };
    static cilist io___572 = { 0, 0, 0, 0, 0 };
    static cilist io___576 = { 0, 0, 0, 0, 0 };
    static cilist io___577 = { 0, 0, 0, 0, 0 };
    static cilist io___587 = { 0, 0, 0, 0, 0 };
    static cilist io___588 = { 0, 0, 0, 0, 0 };
    static cilist io___589 = { 0, 0, 0, 0, 0 };
    static cilist io___590 = { 0, 0, 0, 0, 0 };
    static cilist io___591 = { 0, 0, 0, 0, 0 };
    static cilist io___592 = { 0, 0, 0, 0, 0 };
    static cilist io___593 = { 0, 0, 0, 0, 0 };
    static cilist io___594 = { 0, 0, 0, 0, 0 };
    static cilist io___595 = { 0, 0, 0, 0, 0 };
    static cilist io___596 = { 0, 0, 0, 0, 0 };
    static cilist io___599 = { 0, 0, 0, 0, 0 };
    static cilist io___604 = { 0, 0, 0, 0, 0 };
    static cilist io___605 = { 0, 0, 0, 0, 0 };
    static cilist io___606 = { 0, 0, 0, 0, 0 };
    static cilist io___607 = { 0, 0, 0, 0, 0 };
    static cilist io___608 = { 0, 0, 0, 0, 0 };
    static cilist io___609 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :   forcer l'arete narete de nosoar dans la triangulation actuelle */
/* -----   triangulation frontale pour la reobtenir */

/*         attention: le chainage lchain(=6) de nosoar devient actif */
/*                    durant la formation des contours fermes (cf) */

/* entrees: */
/* -------- */
/* narete : numero nosoar de l'arete frontaliere a forcer */
/* nbarpi : numero du dernier point interne impose par l'utilisateur */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */

/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */
/* mxartr : nombre maximal de triangles stockables dans le tableau noartr */

/* modifies: */
/* --------- */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */
/* noarst : noarst(i) numero d'une arete de sommet i */

/* mxarcf : nombre de variables des tableaux n1arcf, noarcf, larmin, notrcf */

/* tableaux auxiliaires : */
/* ---------------------- */
/* n1arcf : tableau (0:mxarcf) auxiliaire */
/* noarcf : tableau (3,mxarcf) auxiliaire */
/* larmin : tableau (mxarcf)   auxiliaire */
/* notrcf : tableau (1:mxarcf) auxiliaire */

/* sortie : */
/* -------- */
/* ierr   : 0 si pas d'erreur */
/*          1 saturation des sommets */
/*          2 ns1 dans aucun triangle */
/*          9 tableau nosoar de taille insuffisante car trop d'aretes */
/*            a probleme */
/*          10 un des tableaux n1arcf, noarcf notrcf est sature */
/*             augmenter a l'appel mxarcf */
/*         >11 algorithme defaillant */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet analyse numerique paris upmc     mars    1997 */
/* modifs : alain perronnet laboratoire jl lions upmc paris  octobre 2006 */
/* ....................................................................012 */


    /* Parameter adjustments */
    pxyd -= 4;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --noarst;
    --notrcf;
    --larmin;
    noarcf -= 4;

    /* Function Body */
    *ierr = 0;

/*     traitement de cette arete perdue */
    ns1 = nosoar[*narete * nosoar_dim1 + 1];
    ns2 = nosoar[*narete * nosoar_dim1 + 2];

/* cc      write(imprim,*) */
/* cc      write(imprim,*) 'tefoar reconstruction de l''arete ',ns1,' ', ns2 */
/* cc      write(imprim,*) 'sommet',ns1,' x=',pxyd(1,ns1),' y=',pxyd(2,ns1) */
/* cc      write(imprim,*) 'sommet',ns2,' x=',pxyd(1,ns2),' y=',pxyd(2,ns2) */

/*     le sommet ns2 est il correct? */
    na = noarst[ns2];
    if (na <= 0) {
	//io___548.ciunit = unites_2.imprim;
	//s_wsle(&io___548);
	//do_lio(&c__9, &c__1, "tefoar: erreur sommet ", (ftnlen)22);
	//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " sans arete", (ftnlen)11);
	//e_wsle();
	*ierr = 8;
/* cc         pause */
	return 0;
    }
    if (nosoar[na * nosoar_dim1 + 4] <= 0) {
	//io___549.ciunit = unites_2.imprim;
	//s_wsle(&io___549);
	//do_lio(&c__9, &c__1, "tefoar: erreur sommet ", (ftnlen)22);
	//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " dans aucun triangle", (ftnlen)20);
	//e_wsle();
	*ierr = 8;
/* cc         pause */
	return 0;
    }

/*     le premier passage: recherche dans le sens ns1->ns2 */
    ipas = 0;

/*     recherche des triangles intersectes par le segment ns1-ns2 */
/*     ========================================================== */
L3:
    x1 = pxyd[ns1 * 3 + 1];
    y1 = pxyd[ns1 * 3 + 2];
    x2 = pxyd[ns2 * 3 + 1];
    y2 = pxyd[ns2 * 3 + 2];
/* Computing 2nd power */
    d__1 = x2 - x1;
/* Computing 2nd power */
    d__2 = y2 - y1;
    d12 = d__1 * d__1 + d__2 * d__2;

/*     recherche du triangle voisin dans le sens indirect de rotation */
    nsens = -1;

/*     recherche du no local du sommet ns1 dans l'un de ses triangles */
L10:
    na01 = noarst[ns1];
    if (na01 <= 0) {
	//io___558.ciunit = unites_2.imprim;
	//s_wsle(&io___558);
	//do_lio(&c__9, &c__1, "tefoar: sommet ", (ftnlen)15);
	//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " sans arete", (ftnlen)11);
	//e_wsle();
	*ierr = 8;
/* cc         pause */
	return 0;
    }
    nt0 = nosoar[na01 * nosoar_dim1 + 4];
    if (nt0 <= 0) {
	//io___560.ciunit = unites_2.imprim;
	//s_wsle(&io___560);
	//do_lio(&c__9, &c__1, "tefoar: sommet ", (ftnlen)15);
	//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " dans aucun triangle", (ftnlen)20);
	//e_wsle();
	*ierr = 8;
/* cc         pause */
	return 0;
    }

/*     le numero des 3 sommets du triangle nt0 dans le sens direct */
L20:
    nusotr_(&nt0, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
	    noartr_offset], nosotr);
    for (na00 = 1; na00 <= 3; ++na00) {
	if (nosotr[na00 - 1] == ns1) {
	    goto L26;
	}
/* L22: */
    }

L25:
    if (ipas == 0) {
/*        le second passage: recherche dans le sens ns2->ns1 */
/*        tentative d'inversion des 2 sommets extremites de l'arete a forcer */
	na00 = ns1;
	ns1 = ns2;
	ns2 = na00;
	ipas = 1;
	goto L3;
    } else {
/*        les sens ns1->ns2 et ns2->ns1 ne donne pas de solution! */
	//io___563.ciunit = unites_2.imprim;
	//s_wsle(&io___563);
	//do_lio(&c__9, &c__1, "tefoar:arete ", (ftnlen)13);
	//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " - ", (ftnlen)3);
	//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " a imposer", (ftnlen)10);
	//e_wsle();
	//io___564.ciunit = unites_2.imprim;
	//s_wsle(&io___564);
	//do_lio(&c__9, &c__1, "tefoar:anomalie sommet ", (ftnlen)23);
	//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, "non dans le triangle de sommets ", (ftnlen)32);
	//for (i__ = 1; i__ <= 3; ++i__) {
	//    do_lio(&c__3, &c__1, (char *)&nosotr[i__ - 1], (ftnlen)sizeof(
	//	    integer));
	//}
	//e_wsle();
	*ierr = 11;
/* cc         pause */
	return 0;
    }

/*     le numero des aretes suivante et precedente */
L26:
    na0 = nosui3_(&na00);
    na1 = nopre3_(&na00);
    ns3 = nosotr[na0 - 1];
    ns4 = nosotr[na1 - 1];

/*     point d'intersection du segment ns1-ns2 avec l'arete ns3-ns4 */
/*     ------------------------------------------------------------ */
    int1sd_(&ns1, &ns2, &ns3, &ns4, &pxyd[4], &linter, &x1, &y1);
    if (linter <= 0) {

/*        pas d'intersection: rotation autour du point ns1 */
/*        pour trouver le triangle de l'autre cote de l'arete na01 */
	if (nsens < 0) {
/*           sens indirect de rotation: l'arete de sommet ns1 */
	    na01 = (i__1 = noartr[na00 + nt0 * noartr_dim1], abs(i__1));
	} else {
/*           sens direct de rotation: l'arete de sommet ns1 qui precede */
	    na01 = (i__1 = noartr[na1 + nt0 * noartr_dim1], abs(i__1));
	}
/*        le triangle de l'autre cote de l'arete na01 */
	if (nosoar[na01 * nosoar_dim1 + 4] == nt0) {
	    nt0 = nosoar[na01 * nosoar_dim1 + 5];
	} else {
	    nt0 = nosoar[na01 * nosoar_dim1 + 4];
	}
	if (nt0 > 0) {
	    goto L20;
	}

/*        le parcours sort du domaine */
/*        il faut tourner dans l'autre sens autour de ns1 */
	if (nsens < 0) {
	    nsens = 1;
	    goto L10;
	}

/*        dans les 2 sens, pas d'intersection => impossible */
/*        essai avec l'arete inversee ns1 <-> ns2 */
	if (ipas == 0) {
	    goto L25;
	}
	//io___571.ciunit = unites_2.imprim;
	//s_wsle(&io___571);
	//do_lio(&c__9, &c__1, "tefoar: arete ", (ftnlen)14);
	//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " ", (ftnlen)1);
	//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
	//do_lio(&c__9, &c__1, " sans intersection avec les triangles actuels", 
	//	(ftnlen)45);
	//e_wsle();
	//io___572.ciunit = unites_2.imprim;
	//s_wsle(&io___572);
	//do_lio(&c__9, &c__1, "revoyez les lignes du contour", (ftnlen)29);
	//e_wsle();
	*ierr = 12;
/* cc         pause */
	return 0;
    }

/*     il existe une intersection avec l'arete opposee au sommet ns1 */
/*     ============================================================= */
/*     nbtrcf : nombre de triangles du cf */
    nbtrcf = 1;
    notrcf[1] = nt0;

/*     le triangle oppose a l'arete na0 de nt0 */
L30:
    noar = (i__1 = noartr[na0 + nt0 * noartr_dim1], abs(i__1));
    if (nosoar[noar * nosoar_dim1 + 4] == nt0) {
	nt1 = nosoar[noar * nosoar_dim1 + 5];
    } else {
	nt1 = nosoar[noar * nosoar_dim1 + 4];
    }
    if (nt1 <= 0) {
	//io___576.ciunit = unites_2.imprim;
	//s_wsle(&io___576);
	//do_lio(&c__9, &c__1, "erreur dans tefoar nt1=", (ftnlen)23);
	//do_lio(&c__3, &c__1, (char *)&nt1, (ftnlen)sizeof(integer));
	//e_wsle();
	//io___577.ciunit = unites_2.lecteu;
	//s_rsle(&io___577);
	//do_lio(&c__3, &c__1, (char *)&j, (ftnlen)sizeof(integer));
	//e_rsle();
    }

/*     le numero des 3 sommets du triangle nt1 dans le sens direct */
    nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
	    noartr_offset], nosotr);

/*     le triangle nt1 contient il ns2 ? */
    for (j = 1; j <= 3; ++j) {
	if (nosotr[j - 1] == ns2) {
	    goto L70;
	}
/* L32: */
    }

/*     recherche de l'arete noar, na1 dans nt1 qui est l'arete na0 de nt0 */
    for (na1 = 1; na1 <= 3; ++na1) {
	if ((i__1 = noartr[na1 + nt1 * noartr_dim1], abs(i__1)) == noar) {
	    goto L35;
	}
/* L34: */
    }

/*     recherche de l'intersection de ns1-ns2 avec les 2 autres aretes de nt1 */
/*     ====================================================================== */
L35:
    na2 = na1;
    for (i1 = 1; i1 <= 2; ++i1) {
/*        l'arete suivante */
	na2 = nosui3_(&na2);

/*        les 2 sommets de l'arete na2 de nt1 */
	noar = (i__1 = noartr[na2 + nt1 * noartr_dim1], abs(i__1));
	ns3 = nosoar[noar * nosoar_dim1 + 1];
	ns4 = nosoar[noar * nosoar_dim1 + 2];

/*        point d'intersection du segment ns1-ns2 avec l'arete ns3-ns4 */
/*        ------------------------------------------------------------ */
	int1sd_(&ns1, &ns2, &ns3, &ns4, &pxyd[4], &linter, &x, &y);
	if (linter > 0) {

/*           les 2 aretes s'intersectent en (x,y) */
/*           distance de (x,y) a ns3 et ns4 */
/* Computing 2nd power */
	    d__1 = pxyd[ns3 * 3 + 1] - x;
/* Computing 2nd power */
	    d__2 = pxyd[ns3 * 3 + 2] - y;
	    d3 = d__1 * d__1 + d__2 * d__2;
/* Computing 2nd power */
	    d__1 = pxyd[ns4 * 3 + 1] - x;
/* Computing 2nd power */
	    d__2 = pxyd[ns4 * 3 + 2] - y;
	    d4 = d__1 * d__1 + d__2 * d__2;
/*           nsp est le point le plus proche de (x,y) */
	    if (d3 < d4) {
		nsp = ns3;
		d__ = d3;
	    } else {
		nsp = ns4;
		d__ = d4;
	    }
	    if (d__ > d12 * 1e-5) {
		goto L60;
	    }

/*           ici le sommet nsp est trop proche de l'arete perdue ns1-ns2 */
	    if (nsp <= *nbarpi) {
/*              point utilisateur ou frontalier donc non supprimable */
		//io___587.ciunit = unites_2.imprim;
		//s_wsle(&io___587);
		//do_lio(&c__9, &c__1, "tefoar: sommet nsp=", (ftnlen)19);
		//do_lio(&c__3, &c__1, (char *)&nsp, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " frontalier trop proche de l'arete per"
		//	"due ns1=", (ftnlen)46);
		//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, "-ns2=", (ftnlen)5);
		//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
		//e_wsle();
		//io___588.ciunit = unites_2.imprim;
		//s_wsle(&io___588);
		//do_lio(&c__9, &c__1, "s", (ftnlen)1);
		//do_lio(&c__3, &c__1, (char *)&nsp, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, ": x=", (ftnlen)4);
		//do_lio(&c__5, &c__1, (char *)&pxyd[nsp * 3 + 1], (ftnlen)
		//	sizeof(doublereal));
		//do_lio(&c__9, &c__1, " y=", (ftnlen)3);
		//do_lio(&c__5, &c__1, (char *)&pxyd[nsp * 3 + 2], (ftnlen)
		//	sizeof(doublereal));
		//e_wsle();
		//io___589.ciunit = unites_2.imprim;
		//s_wsle(&io___589);
		//do_lio(&c__9, &c__1, "s", (ftnlen)1);
		//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, ": x=", (ftnlen)4);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns1 * 3 + 1], (ftnlen)
		//	sizeof(doublereal));
		//do_lio(&c__9, &c__1, " y=", (ftnlen)3);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns1 * 3 + 2], (ftnlen)
		//	sizeof(doublereal));
		//e_wsle();
		//io___590.ciunit = unites_2.imprim;
		//s_wsle(&io___590);
		//do_lio(&c__9, &c__1, "s", (ftnlen)1);
		//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, ": x=", (ftnlen)4);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns2 * 3 + 1], (ftnlen)
		//	sizeof(doublereal));
		//do_lio(&c__9, &c__1, " y=", (ftnlen)3);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns2 * 3 + 2], (ftnlen)
		//	sizeof(doublereal));
		//e_wsle();
		//io___591.ciunit = unites_2.imprim;
		//s_wsle(&io___591);
		//do_lio(&c__9, &c__1, "arete s", (ftnlen)7);
		//do_lio(&c__3, &c__1, (char *)&ns1, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, "-s", (ftnlen)2);
		//do_lio(&c__3, &c__1, (char *)&ns2, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " coupe arete s", (ftnlen)14);
		//do_lio(&c__3, &c__1, (char *)&ns3, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, "-s", (ftnlen)2);
		//do_lio(&c__3, &c__1, (char *)&ns4, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " en (x,y)", (ftnlen)9);
		//e_wsle();
		//io___592.ciunit = unites_2.imprim;
		//s_wsle(&io___592);
		//do_lio(&c__9, &c__1, "s", (ftnlen)1);
		//do_lio(&c__3, &c__1, (char *)&ns3, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, ": x=", (ftnlen)4);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns3 * 3 + 1], (ftnlen)
		//	sizeof(doublereal));
		//do_lio(&c__9, &c__1, " y=", (ftnlen)3);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns3 * 3 + 2], (ftnlen)
		//	sizeof(doublereal));
		//e_wsle();
		//io___593.ciunit = unites_2.imprim;
		//s_wsle(&io___593);
		//do_lio(&c__9, &c__1, "s", (ftnlen)1);
		//do_lio(&c__3, &c__1, (char *)&ns4, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, ": x=", (ftnlen)4);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns4 * 3 + 1], (ftnlen)
		//	sizeof(doublereal));
		//do_lio(&c__9, &c__1, " y=", (ftnlen)3);
		//do_lio(&c__5, &c__1, (char *)&pxyd[ns4 * 3 + 2], (ftnlen)
		//	sizeof(doublereal));
		//e_wsle();
		//io___594.ciunit = unites_2.imprim;
		//s_wsle(&io___594);
		//do_lio(&c__9, &c__1, "intersection en: x=", (ftnlen)19);
		//do_lio(&c__5, &c__1, (char *)&x, (ftnlen)sizeof(doublereal));
		//do_lio(&c__9, &c__1, " y=", (ftnlen)3);
		//do_lio(&c__5, &c__1, (char *)&y, (ftnlen)sizeof(doublereal));
		//e_wsle();
		//io___595.ciunit = unites_2.imprim;
		//s_wsle(&io___595);
		//do_lio(&c__9, &c__1, "distance ns1-ns2=", (ftnlen)17);
		//d__1 = sqrt(d12);
		//do_lio(&c__5, &c__1, (char *)&d__1, (ftnlen)sizeof(doublereal)
		//	);
		//e_wsle();
		//io___596.ciunit = unites_2.imprim;
		//s_wsle(&io___596);
		//do_lio(&c__9, &c__1, "distance (x,y) au plus proche", (ftnlen)
		//	29);
		//do_lio(&c__3, &c__1, (char *)&ns3, (ftnlen)sizeof(integer));
		//do_lio(&c__3, &c__1, (char *)&ns4, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, "=", (ftnlen)1);
		//d__1 = sqrt(d__);
		//do_lio(&c__5, &c__1, (char *)&d__1, (ftnlen)sizeof(doublereal)
		//	);
		//e_wsle();
		*ierr = 13;
/* cc               pause */
		return 0;
	    }

/*           le sommet interne nsp est supprime en mettant tous les triangles */
/*           l'ayant comme sommet dans la pile notrcf des triangles a supprimer */
/*           ------------------------------------------------------------------ */
/* cc            write(imprim,*) 'tefoar: le sommet ',nsp,' est supprime' */
/*           construction de la liste des triangles de sommet nsp */
	    trp1st_(&nsp, &noarst[1], mosoar, &nosoar[nosoar_offset], moartr, 
		    mxartr, &noartr[noartr_offset], &c__32, &nbt, lapitr);
	    if (nbt <= 0) {
/*              les triangles de sommet nsp ne forme pas une "boule" */
/*              avec ce sommet nsp pour "centre" */
		//io___599.ciunit = unites_2.imprim;
		//s_wsle(&io___599);
		//do_lio(&c__9, &c__1, "tefoar: les triangles autour du sommet "
		//	, (ftnlen)39);
		//do_lio(&c__3, &c__1, (char *)&nsp, (ftnlen)sizeof(integer));
		//do_lio(&c__9, &c__1, " ne forme pas une etoile", (ftnlen)24);
		//e_wsle();
		nbt = -nbt;
	    }

/*           ajout des triangles de sommet nsp a notrcf */
	    nbtrc0 = nbtrcf;
	    i__1 = nbt;
	    for (j = 1; j <= i__1; ++j) {
		nt = lapitr[j - 1];
		for (k = nbtrcf; k >= 1; --k) {
		    if (nt == notrcf[k]) {
			goto L38;
		    }
/* L37: */
		}
/*              triangle ajoute */
		++nbtrcf;
		notrcf[nbtrcf] = nt;
L38:
		;
	    }

/*           ce sommet supprime n'appartient plus a aucun triangle */
	    noarst[nsp] = 0;

/*           ns2 est-il un sommet des triangles empiles? */
/*           ------------------------------------------- */
	    i__1 = nbtrcf;
	    for (nt = nbtrc0 + 1; nt <= i__1; ++nt) {
/*              le triangle a supprimer nt */
		nt1 = notrcf[nt];
/*              le numero des 3 sommets du triangle nt1 dans le sens direct */
		nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		for (k = 1; k <= 3; ++k) {
/*                 le sommet k de nt1 */
		    if (nosotr[k - 1] == ns2) {
/*                    but atteint */
			goto L80;
		    }
/* L39: */
		}
/* L40: */
	    }

/*           recherche du plus proche point d'intersection de ns1-ns2 */
/*           par rapport a ns2 avec les aretes des triangles ajoutes */
	    nt0 = 0;
	    dmin__ = d12 * 10000;
	    i__1 = nbtrcf;
	    for (nt = nbtrc0 + 1; nt <= i__1; ++nt) {
		nt1 = notrcf[nt];
/*              le numero des 3 sommets du triangle nt1 dans le sens direct */
		nusotr_(&nt1, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
			noartr_offset], nosotr);
		for (k = 1; k <= 3; ++k) {
/*                 les 2 sommets de l'arete k de nt */
		    ns3 = nosotr[k - 1];
		    ns4 = nosotr[nosui3_(&k) - 1];

/*                 point d'intersection du segment ns1-ns2 avec l'arete ns3-ns4 */
/*                 ------------------------------------------------------------ */
		    int1sd_(&ns1, &ns2, &ns3, &ns4, &pxyd[4], &linter, &x, &y)
			    ;
		    if (linter > 0) {
/*                    les 2 aretes s'intersectent en (x,y) */
/* Computing 2nd power */
			d__1 = x - x2;
/* Computing 2nd power */
			d__2 = y - y2;
			d__ = d__1 * d__1 + d__2 * d__2;
			if (d__ < dmin__) {
			    nt0 = nt1;
			    na0 = k;
			    dmin__ = d__;
			}
		    }
/* L45: */
		}
/* L48: */
	    }

/*           redemarrage avec le triangle nt0 et l'arete na0 */
	    if (nt0 > 0) {
		goto L30;
	    }

	    //io___604.ciunit = unites_2.imprim;
	    //s_wsle(&io___604);
	    //do_lio(&c__9, &c__1, "tefoar: algorithme defaillant", (ftnlen)29);
	    //e_wsle();
	    *ierr = 14;
/* cc            pause */
	    return 0;
	}
/* L50: */
    }

/*     pas d'intersection differente de l'initiale => sommet sur ns1-ns2 */
/*     tentative d'inversion des sommets de l'arete ns1-ns2 */
    if (ipas == 0) {
	goto L25;
    }
    //io___605.ciunit = unites_2.imprim;
    //s_wsle(&io___605);
    //e_wsle();
    //io___606.ciunit = unites_2.imprim;
    //s_wsle(&io___606);
    //do_lio(&c__9, &c__1, "tefoar 50: revoyez vos donnees", (ftnlen)30);
    //e_wsle();
    //io___607.ciunit = unites_2.imprim;
    //s_wsle(&io___607);
    //do_lio(&c__9, &c__1, "les lignes fermees doivent etre disjointes", (
	   // ftnlen)42);
    //e_wsle();
    //io___608.ciunit = unites_2.imprim;
    //s_wsle(&io___608);
    //do_lio(&c__9, &c__1, "verifiez si elles ne se coupent pas", (ftnlen)35);
    //e_wsle();
    *ierr = 15;
/* cc      pause */
    return 0;

/*     cas sans probleme : intersection differente de celle initiale */
/*     =================   ========================================= */
L60:
    ++nbtrcf;
    notrcf[nbtrcf] = nt1;
/*     passage au triangle suivant */
    na0 = na2;
    nt0 = nt1;
    goto L30;

/*     ---------------------------------------------------------- */
/*     ici toutes les intersections de ns1-ns2 ont ete parcourues */
/*     tous les triangles intersectes ou etendus forment les */
/*     nbtrcf triangles du tableau notrcf */
/*     ---------------------------------------------------------- */
L70:
    ++nbtrcf;
    notrcf[nbtrcf] = nt1;

/*     formation du cf des aretes simples des triangles de notrcf */
/*     et destruction des nbtrcf triangles du tableau noartr */
/*     attention: le chainage lchain du tableau nosoar devient actif */
/*     ============================================================= */
L80:
    if (nbtrcf * 3 > *mxarcf) {
	//io___609.ciunit = unites_2.imprim;
	//s_wsle(&io___609);
	//do_lio(&c__9, &c__1, "saturation du tableau noarcf", (ftnlen)28);
	//e_wsle();
	*ierr = 10;
/* cc         pause */
	return 0;
    }

    focftr_(&nbtrcf, &notrcf[1], nbarpi, &pxyd[4], &noarst[1], mosoar, mxsoar,
	     n1soar, &nosoar[nosoar_offset], moartr, n1artr, &noartr[
	    noartr_offset], &nbarcf, n1arcf, &noarcf[4], &nbstpe, nostpe, 
	    ierr);
    if (*ierr != 0) {
	return 0;
    }

/*     chainage des aretes vides dans le tableau noarcf */
/*     ------------------------------------------------ */
/*     decalage de 2 aretes car 2 aretes sont necessaires ensuite pour */
/*     integrer 2 fois l'arete perdue et former ainsi 2 cf */
/*     comme nbtrcf*3 minore mxarcf il existe au moins 2 places vides */
/*     derriere => pas de test de debordement */
    n1arcf[0] = nbarcf + 3;
/* Computing MIN */
    i__1 = nbarcf << 3;
    mmarcf = min(i__1,*mxarcf);
    i__1 = mmarcf;
    for (i__ = nbarcf + 3; i__ <= i__1; ++i__) {
	noarcf[i__ * 3 + 2] = i__ + 1;
/* L90: */
    }
    noarcf[mmarcf * 3 + 2] = 0;

/*     reperage des sommets ns1 ns2 de l'arete perdue dans le cf */
/*     --------------------------------------------------------- */
    ns1 = nosoar[*narete * nosoar_dim1 + 1];
    ns2 = nosoar[*narete * nosoar_dim1 + 2];
    ns[0] = ns1;
    ns[1] = ns2;
    for (i__ = 1; i__ <= 2; ++i__) {
/*        la premiere arete dans noarcf du cf */
	na0 = n1arcf[1];
L110:
	if (noarcf[na0 * 3 + 1] != ns[i__ - 1]) {
/*           passage a l'arete suivante */
	    na0 = noarcf[na0 * 3 + 2];
	    goto L110;
	}
/*        position dans noarcf du sommet i de l'arete perdue */
	nacf[i__ - 1] = na0;
/* L120: */
    }

/*     formation des 2 cf chacun contenant l'arete ns1-ns2 */
/*     --------------------------------------------------- */
/*     sauvegarde de l'arete suivante de celle de sommet ns1 */
    na0 = noarcf[*nacf1 * 3 + 2];
    nt1 = noarcf[*nacf1 * 3 + 3];

/*     le premier cf */
    n1arcf[1] = *nacf1;
/*     l'arete suivante dans le premier cf */
    noarcf[*nacf1 * 3 + 2] = *nacf2;
/*     cette arete est celle perdue */
    noarcf[*nacf1 * 3 + 3] = *narete;

/*     le second cf */
/*     l'arete doublee */
    n1 = nbarcf + 1;
    n2 = nbarcf + 2;
/*     le premier sommet de la premiere arete du second cf */
    noarcf[n1 * 3 + 1] = ns2;
/*     l'arete suivante dans le second cf */
    noarcf[n1 * 3 + 2] = n2;
/*     cette arete est celle perdue */
    noarcf[n1 * 3 + 3] = *narete;
/*     la seconde arete du second cf */
    noarcf[n2 * 3 + 1] = ns1;
    noarcf[n2 * 3 + 2] = na0;
    noarcf[n2 * 3 + 3] = nt1;
    n1arcf[2] = n1;

/*     recherche du precedent de nacf2 */
L130:
    na1 = noarcf[na0 * 3 + 2];
    if (na1 != *nacf2) {
/*        passage a l'arete suivante */
	na0 = na1;
	goto L130;
    }
/*     na0 precede nacf2 => il precede n1 */
    noarcf[na0 * 3 + 2] = n1;

/*     depart avec 2 cf */
    nbcf = 2;

/*     triangulation directe des 2 contours fermes */
/*     l'arete ns1-ns2 devient une arete de la triangulation des 2 cf */
/*     ============================================================== */
    tridcf_(&nbcf, &nbstpe, nostpe, &pxyd[4], &noarst[1], mosoar, mxsoar, 
	    n1soar, &nosoar[nosoar_offset], moartr, n1artr, &noartr[
	    noartr_offset], mxarcf, n1arcf, &noarcf[4], &larmin[1], &nbtrcf, &
	    notrcf[1], ierr);

    return 0;
} /* tefoar_ */

#undef nacf2
#undef nacf1
#undef nacf


/* Subroutine */ int te4ste_(integer *nbsomm, integer *mxsomm, doublereal *
	pxyd, integer *ntrp, integer *letree, integer *ierr)
{
    /* Builtin functions */
    //integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	   // e_wsle(void);

    /* Local variables */
    static integer i__, i1, i2, np[4], ns1, ns2, nsot;
    extern integer nopre3_(integer *);
    extern /* Subroutine */ int n1trva_(integer *, integer *, integer *, 
	    integer *, integer *);
    extern integer nosui3_(integer *);
    static integer milieu[3], niveau, noteva;
    extern integer notrpt_(doublereal *, doublereal *, integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___625 = { 0, 0, 0, 0, 0 };
    static cilist io___626 = { 0, 0, 0, 0, 0 };
    static cilist io___629 = { 0, 0, 0, 0, 0 };
    static cilist io___630 = { 0, 0, 0, 0, 0 };


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    decouper un te ntrp de letree en 4 sous-triangles */
/* -----    eliminer les sommets de te trop proches des points */

/* entrees: */
/* -------- */
/* mxsomm : nombre maximal de points declarables dans pxyd */
/* ntrp   : numero letree du triangle a decouper en 4 sous-triangles */

/* modifies : */
/* ---------- */
/* nbsomm : nombre actuel de points dans pxyd */
/* pxyd   : tableau des coordonnees des points */
/*          par point : x  y  distance_souhaitee */
/* letree : arbre-4 des triangles equilateraux (te) fond de la triangulation */
/*      letree(0,0) :  no du 1-er te vide dans letree */
/*      letree(0,1) : maximum du 1-er indice de letree (ici 8) */
/*      letree(0,2) : maximum declare du 2-eme indice de letree (ici mxtree) */
/*      letree(0:8,1) : racine de l'arbre  (triangle sans sur triangle) */
/*      si letree(0,.)>0 alors */
/*         letree(0:3,j) : no (>0) letree des 4 sous-triangles du triangle j */
/*      sinon */
/*         letree(0:3,j) :-no pxyd des 1 a 4 points internes au triangle j */
/*                         0  si pas de point */
/*                        ( j est alors une feuille de l'arbre ) */
/*      letree(4,j) : no letree du sur-triangle du triangle j */
/*      letree(5,j) : 0 1 2 3 no du sous-triangle j pour son sur-triangle */
/*      letree(6:8,j) : no pxyd des 3 sommets du triangle j */

/* sorties : */
/* --------- */
/* ierr    : 0 si pas d'erreur, 51 saturation letree, 52 saturation pxyd */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet  analyse numerique paris upmc    juillet 1994 */
/* 2345x7..............................................................012 */

/*     debut par l'arete 2 du triangle ntrp */
    /* Parameter adjustments */
    pxyd -= 4;

    /* Function Body */
    *ierr = 0;
    i1 = 2;
    i2 = 3;
    for (i__ = 1; i__ <= 3; ++i__) {

/*        le milieu de l'arete i1 existe t il deja ? */
	n1trva_(ntrp, &i1, letree, &noteva, &niveau);
	if (noteva > 0) {
/*           il existe un te voisin */
/*           s'il existe 4 sous-triangles le milieu existe deja */
	    if (letree[noteva * 9] > 0) {
/*              le milieu existe */
		nsot = letree[noteva * 9];
		milieu[i__ - 1] = letree[nopre3_(&i1) + 5 + nsot * 9];
		goto L25;
	    }
	}

/*        le milieu n'existe pas. il est cree */
	++(*nbsomm);
	if (*nbsomm > *mxsomm) {
/*           plus assez de place dans pxyd */
	    //io___625.ciunit = unites_1.imprim;
	    //s_wsle(&io___625);
	    //do_lio(&c__9, &c__1, "te4ste: saturation pxyd", (ftnlen)23);
	    //e_wsle();
	    //io___626.ciunit = unites_1.imprim;
	    //s_wsle(&io___626);
	    //e_wsle();
	    *ierr = 52;
	    return 0;
	}
/*        le milieu de l'arete i */
	milieu[i__ - 1] = *nbsomm;

/*        ntrp est le triangle de milieux d'arete ces 3 sommets */
	ns1 = letree[i1 + 5 + *ntrp * 9];
	ns2 = letree[i2 + 5 + *ntrp * 9];
	pxyd[*nbsomm * 3 + 1] = (pxyd[ns1 * 3 + 1] + pxyd[ns2 * 3 + 1]) * .5f;
	pxyd[*nbsomm * 3 + 2] = (pxyd[ns1 * 3 + 2] + pxyd[ns2 * 3 + 2]) * .5f;

/*        l'arete et milieu suivant */
L25:
	i1 = i2;
	i2 = nosui3_(&i2);
/* L30: */
    }

    for (i__ = 0; i__ <= 3; ++i__) {

/*        le premier triangle vide */
	nsot = letree[0];
	if (nsot <= 0) {
/*           manque de place. saturation letree */
	    *ierr = 51;
	    //io___629.ciunit = unites_1.imprim;
	    //s_wsle(&io___629);
	    //do_lio(&c__9, &c__1, "te4ste: saturation letree", (ftnlen)25);
	    //e_wsle();
	    //io___630.ciunit = unites_1.imprim;
	    //s_wsle(&io___630);
	    //e_wsle();
	    return 0;
	}

/*        mise a jour du premier te libre */
	letree[0] = letree[nsot * 9];

/*        nsot est le i-eme sous triangle */
	letree[nsot * 9] = 0;
	letree[nsot * 9 + 1] = 0;
	letree[nsot * 9 + 2] = 0;
	letree[nsot * 9 + 3] = 0;

/*        le numero des points et sous triangles dans ntrp */
	np[i__] = -letree[i__ + *ntrp * 9];
	letree[i__ + *ntrp * 9] = nsot;

/*        le sommet commun avec le triangle ntrp */
	letree[i__ + 5 + nsot * 9] = letree[i__ + 5 + *ntrp * 9];

/*        le sur-triangle et numero de sous-triangle de nsot */
/*        a laisser ici car incorrect sinon pour i=0 */
	letree[nsot * 9 + 4] = *ntrp;
	letree[nsot * 9 + 5] = i__;

/*        le sous-triangle du triangle */
	letree[i__ + *ntrp * 9] = nsot;
/* L50: */
    }

/*     le numero des nouveaux sommets milieux */
    nsot = letree[*ntrp * 9];
    letree[nsot * 9 + 6] = milieu[0];
    letree[nsot * 9 + 7] = milieu[1];
    letree[nsot * 9 + 8] = milieu[2];

    nsot = letree[*ntrp * 9 + 1];
    letree[nsot * 9 + 7] = milieu[2];
    letree[nsot * 9 + 8] = milieu[1];

    nsot = letree[*ntrp * 9 + 2];
    letree[nsot * 9 + 6] = milieu[2];
    letree[nsot * 9 + 8] = milieu[0];

    nsot = letree[*ntrp * 9 + 3];
    letree[nsot * 9 + 6] = milieu[1];
    letree[nsot * 9 + 7] = milieu[0];

/*     repartition des eventuels 4 points np dans ces 4 sous-triangles */
/*     il y a obligatoirement suffisamment de place */
    for (i__ = 0; i__ <= 3; ++i__) {
	if (np[i__] > 0) {
	    nsot = notrpt_(&pxyd[np[i__] * 3 + 1], &pxyd[4], ntrp, letree);
/*           ajout du point */
	    for (i1 = 0; i1 <= 3; ++i1) {
		if (letree[i1 + nsot * 9] == 0) {
/*                 place libre a occuper */
		    letree[i1 + nsot * 9] = -np[i__];
		    goto L110;
		}
/* L100: */
	    }
	}
L110:
	;
    }
    return 0;
} /* te4ste_ */

/* Subroutine */ int tesuqm_(doublereal *quamal, integer *nbarpi, doublereal *
	pxyd, integer *noarst, integer *mosoar, integer *mxsoar, integer *
	n1soar, integer *nosoar, integer *moartr, integer *mxartr, integer *
	n1artr, integer *noartr, integer *mxarcf, integer *n1arcf, integer *
	noarcf, integer *larmin, integer *notrcf, integer *liarcf, doublereal 
	*quamin)
{
    /* System generated locals */
    integer nosoar_dim1, nosoar_offset, noartr_dim1, noartr_offset, i__1, 
	    i__2;
    doublereal d__1, d__2;

    /* Local variables */
    static integer j, n;
    static doublereal a12, s12, s34;
    static integer nt;
    static doublereal s123, s142, s143, s234;
    static integer ns1, ns2, ns3, ns4, nbt, naop, noar, ierr, noar34;
    extern /* Subroutine */ int te2t2t_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *), mt4sqa_(
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *), te1stm_(integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *), 
	    qutr2d_(doublereal *, doublereal *, doublereal *, doublereal *);
    extern doublereal surtd2_(doublereal *, doublereal *, doublereal *);
    static integer narete, notraj[3];
    static doublereal qualit;
    static integer nbtrqm;
    extern /* Subroutine */ int tritas_(integer *, doublereal *, integer *);
    static integer ntqmin, notrqm[1024], nosotr[3];
    static doublereal qutrqm[1024];
    extern /* Subroutine */ int nusotr_(integer *, integer *, integer *, 
	    integer *, integer *, integer *);

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    supprimer de la triangulation les triangles de qualite */
/* -----    inferieure a quamal */

/* entrees: */
/* -------- */
/* quamal : qualite des triangles au dessous de laquelle supprimer des sommets */
/* nbarpi : numero du dernier point interne impose par l'utilisateur */
/* pxyd   : tableau des coordonnees 2d des points */
/*          par point : x  y  distance_souhaitee */
/* mosoar : nombre maximal d'entiers par arete et */
/*          indice dans nosoar de l'arete suivante dans le hachage */
/* mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar */
/*          attention: mxsoar>3*mxsomm obligatoire! */
/* moartr : nombre maximal d'entiers par arete du tableau noartr */

/* modifies: */
/* --------- */
/* noarst : noarst(i) numero d'une arete de sommet i */
/* n1soar : no de l'eventuelle premiere arete libre dans le tableau nosoar */
/*          chainage des vides suivant en 3 et precedant en 2 de nosoar */
/* nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete, */
/*          chainage des aretes frontalieres, chainage du hachage des aretes */
/*          hachage des aretes = nosoar(1)+nosoar(2)*2 */
/*          avec mxsoar>=3*mxsomm */
/*          une arete i de nosoar est vide <=> nosoar(1,i)=0 et */
/*          nosoar(2,arete vide)=l'arete vide qui precede */
/*          nosoar(3,arete vide)=l'arete vide qui suit */
/* n1artr : numero du premier triangle vide dans le tableau noartr */
/*          le chainage des triangles vides se fait sur noartr(2,.) */
/* noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3 */
/*          arete1 = 0 si triangle vide => arete2 = triangle vide suivant */

/* auxiliaires : */
/* ------------- */
/* n1arcf : tableau (0:mxarcf) auxiliaire d'entiers */
/* noarcf : tableau (3,mxarcf) auxiliaire d'entiers */
/* larmin : tableau (mxarcf)   auxiliaire d'entiers */
/* notrcf : tableau (mxarcf)   auxiliaire d'entiers */
/* liarcf : tableau (mxarcf)   auxiliaire d'entiers */

/* sortie : */
/* -------- */
/* quamin : qualite minimale des triangles */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : alain perronnet Laboratoire JL Lions UPMC Paris  Octobre 2006 */
/* ....................................................................012 */

    /* Parameter adjustments */
    pxyd -= 4;
    --noarst;
    nosoar_dim1 = *mosoar;
    nosoar_offset = 1 + nosoar_dim1;
    nosoar -= nosoar_offset;
    noartr_dim1 = *moartr;
    noartr_offset = 1 + noartr_dim1;
    noartr -= noartr_offset;
    --liarcf;
    --notrcf;
    --larmin;
    noarcf -= 4;

    /* Function Body */
    ierr = 0;

/*     initialisation du chainage des aretes des cf => 0 arete de cf */
    i__1 = *mxsoar;
    for (narete = 1; narete <= i__1; ++narete) {
	nosoar[narete * nosoar_dim1 + 6] = -1;
/* L5: */
    }

/*     recherche des triangles de plus basse qualite */
    *quamin = 2.f;
    nbtrqm = 0;
    i__1 = *mxartr;
    for (nt = 1; nt <= i__1; ++nt) {
	if (noartr[nt * noartr_dim1 + 1] == 0) {
	    goto L10;
	}
/*        le numero des 3 sommets du triangle nt */
	nusotr_(&nt, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);
/*        la qualite du triangle ns1 ns2 ns3 */
	qutr2d_(&pxyd[nosotr[0] * 3 + 1], &pxyd[nosotr[1] * 3 + 1], &pxyd[
		nosotr[2] * 3 + 1], &qualit);
	if (qualit < *quamal) {
	    if (nbtrqm >= 1024) {
		goto L10;
	    }
	    ++nbtrqm;
	    notrqm[nbtrqm - 1] = nt;
	    qutrqm[nbtrqm - 1] = qualit;
	}
L10:
	;
    }

/*     tri croissant des qualites minimales des triangles */
    tritas_(&nbtrqm, qutrqm, notrqm);

/*     le plus mauvais triangle */
    ntqmin = notrqm[0];
    *quamin = qutrqm[0];

    i__1 = nbtrqm;
    for (n = 1; n <= i__1; ++n) {

/*        no du triangle de mauvaise qualite */
	ntqmin = notrqm[n - 1];

/*        le triangle a t il ete traite? */
	if (noartr[ntqmin * noartr_dim1 + 1] == 0) {
	    goto L100;
	}

/* cc         print * */
/* cc         print *,'tesuqm: triangle',ntqmin,' qualite=',qutrqm(n) */
/* cc         print *,'tesuqm: noartr(',ntqmin,')=', */
/* cc     %           (noartr(j,ntqmin),j=1,moartr) */
/* ccc */
/* cc         do 12 j=1,3 */
/* cc            noar = noartr(j,ntqmin) */
/* cc         print*,'arete',noar,' nosoar=',(nosoar(i,abs(noar)),i=1,mosoar) */
/* cc 12      continue */

/*        le numero des 3 sommets du triangle ntqmin */
	nusotr_(&ntqmin, mosoar, &nosoar[nosoar_offset], moartr, &noartr[
		noartr_offset], nosotr);

/* cc         do 15 j=1,3 */
/* cc            nbt = nosotr(j) */
/* cc            print *,'sommet',nbt,':  x=',pxyd(1,nbt),'  y=',pxyd(2,nbt) */
/* cc 15      continue */

/*        recherche des triangles adjacents par les aretes de ntqmin */
	nbt = 0;
	for (j = 1; j <= 3; ++j) {
/*           le no de l'arete j dans nosoar */
	    noar = (i__2 = noartr[j + ntqmin * noartr_dim1], abs(i__2));
/*           le triangle adjacent a l'arete j de ntqmin */
	    if (nosoar[noar * nosoar_dim1 + 4] == ntqmin) {
		notraj[j - 1] = nosoar[noar * nosoar_dim1 + 5];
	    } else {
		notraj[j - 1] = nosoar[noar * nosoar_dim1 + 4];
	    }
	    if (notraj[j - 1] > 0) {
/*              1 triangle adjacent de plus */
		naop = j;
		++nbt;
	    } else {
/*              pas de triangle adjacent */
		notraj[j - 1] = 0;
	    }
/* L20: */
	}

	if (nbt == 1) {

/*           ntqmin a un seul triangle oppose par l'arete naop */
/*           le triangle a 2 aretes frontalieres est plat */
/*           l'arete commune aux 2 triangles est rendue Delaunay */
/*           --------------------------------------------------- */
	    noar = (i__2 = noartr[naop + ntqmin * noartr_dim1], abs(i__2));
	    if (nosoar[noar * nosoar_dim1 + 3] != 0) {
/*              arete frontaliere */
		goto L100;
	    }

/*           l'arete appartient a deux triangles actifs */
/*           le numero des 4 sommets du quadrangle des 2 triangles */
	    mt4sqa_(&noar, moartr, &noartr[noartr_offset], mosoar, &nosoar[
		    nosoar_offset], &ns1, &ns2, &ns3, &ns4);
	    if (ns4 == 0) {
		goto L100;
	    }

/*           carre de la longueur de l'arete ns1 ns2 */
/* Computing 2nd power */
	    d__1 = pxyd[ns2 * 3 + 1] - pxyd[ns1 * 3 + 1];
/* Computing 2nd power */
	    d__2 = pxyd[ns2 * 3 + 2] - pxyd[ns1 * 3 + 2];
	    a12 = d__1 * d__1 + d__2 * d__2;

/*           comparaison de la somme des aires des 2 triangles */
/*           ------------------------------------------------- */
/*           calcul des surfaces des triangles 123 et 142 de cette arete */
	    s123 = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns2 * 3 + 1], &pxyd[ns3 *
		     3 + 1]);
	    s142 = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns4 * 3 + 1], &pxyd[ns2 *
		     3 + 1]);
/* cc            print *,'tesuqm: ns4=',ns4,' x=',pxyd(1,ns4), */
/* cc     %                                 ' y=',pxyd(2,ns4) */
/* cc            print *,'tesuqm: s123=',s123,'  s142=',s142 */
	    s12 = abs(s123) + abs(s142);
	    if (s12 <= a12 * .001f) {
		goto L100;
	    }

/*           calcul des surfaces des triangles 143 et 234 de cette arete */
	    s143 = surtd2_(&pxyd[ns1 * 3 + 1], &pxyd[ns4 * 3 + 1], &pxyd[ns3 *
		     3 + 1]);
	    s234 = surtd2_(&pxyd[ns2 * 3 + 1], &pxyd[ns3 * 3 + 1], &pxyd[ns4 *
		     3 + 1]);
/* cc            print *,'tesuqm: s143=',s143,'  s234=',s234 */
	    s34 = abs(s234) + abs(s143);
/* cc            print *,'tesuqm: s12=',s12,'  s34=',s34 */

	    if ((d__1 = s34 - s12, abs(d__1)) > s34 * 1e-14) {
		goto L100;
	    }

/*           quadrangle convexe */
/*           echange de la diagonale 12 par 34 des 2 triangles */
/*           ------------------------------------------------- */
	    te2t2t_(&noar, mosoar, n1soar, &nosoar[nosoar_offset], &noarst[1],
		     moartr, &noartr[noartr_offset], &noar34);
/* cc            print *,'tesuqm: sortie te2t2t avec noar34=',noar34 */


	} else if (nbt == 2) {

/*           ntqmin a 2 triangles opposes par l'arete naop */
/*           essai de supprimer le sommet non frontalier */
/*           --------------------------------------------- */
	    for (j = 1; j <= 3; ++j) {
		if (notraj[j - 1] == 0) {
		    goto L33;
		}
/* L30: */
	    }

/*           arete sans triangle adjacent */
L33:
	    noar = (i__2 = noartr[j + ntqmin * noartr_dim1], abs(i__2));
/* cc            print *,'tesuqm: nosoar(',noar,')=', */
/* cc     %              (nosoar(j,noar),j=1,mosoar) */
	    if (noar <= 0) {
		goto L100;
	    }

/*           ses 2 sommets */
	    ns1 = nosoar[noar * nosoar_dim1 + 1];
	    ns2 = nosoar[noar * nosoar_dim1 + 2];

/*           ns3 l'autre sommet non frontalier */
	    for (j = 1; j <= 3; ++j) {
		ns3 = nosotr[j - 1];
		if (ns3 != ns1 && ns3 != ns2) {
		    goto L40;
		}
/* L36: */
	    }

L40:
	    if (ns3 > *nbarpi) {

/*              le sommet ns3 non frontalier va etre supprime */
/* cc               print*,'tesuqm: ntqmin=',ntqmin, */
/* cc     %                ' demande la suppression ns3=',ns3 */
		te1stm_(&ns3, nbarpi, &pxyd[4], &noarst[1], mosoar, mxsoar, 
			n1soar, &nosoar[nosoar_offset], moartr, mxartr, 
			n1artr, &noartr[noartr_offset], mxarcf, n1arcf, &
			noarcf[4], &larmin[1], &notrcf[1], &liarcf[1], &ierr);
/* cc               if( ierr .eq. 0 ) then */
/* cc                  print *,'tesuqm: st supprime ns3=',ns3 */
/* cc               else */
/* cc                print *,'tesuqm: ST NON SUPPRIME ns3=',ns3,' ierr=',ierr */
/* cc               endif */
	    }

	}

L100:
	;
    }

    return 0;
} /* tesuqm_ */

/* Subroutine */ int tritas_(integer *nb, doublereal *a, integer *noanc)
{
    static integer fil, fin, per;
    static doublereal aux;
    static integer pere, naux, fils1, fils2;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* but :    tri croissant du tableau a de nb reels par la methode du tas */
/* -----    methode due a williams et floyd     o(n log n ) */
/*          version avec un pointeur sur un tableau dont est extrait a */
/* entrees: */
/* -------- */
/* nb     : nombre de termes du tableau a */
/* a      : les nb reels double precision a trier dans a */
/* noanc  : numero ancien position de l'information (souvent noanc(i)=i) */

/* sorties: */
/* -------- */
/* a      : les nb reels croissants dans a */
/* noanc  : numero ancien position de l'information */
/*          noanc(1)=no position pointeur sur a(1), ... */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* auteur : perronnet alain analyse numerique upmc paris     fevrier 1991 */
/* ...................................................................012 */

/*     formation du tas sous forme d'un arbre binaire */
    /* Parameter adjustments */
    --noanc;
    --a;

    /* Function Body */
    fin = *nb + 1;

    for (pere = *nb / 2; pere >= 1; --pere) {

/*        descendre pere jusqu'a n dans a  de facon  a respecter */
/*        a(pere)>a(j) pour j fils ou petit fils de pere */
/*        c-a-d pour tout j tel que pere <= e(j/2)<j<nb+1 */
/*                                          a(j/2) >= a(j) */
/*                                                 >= a(j+1) */

/*        protection du pere */
	per = pere;

/*        le fils 1 du pere */
L10:
	fils1 = per << 1;
	if (fils1 < fin) {
/*           il existe un fils1 */
	    fil = fils1;
	    fils2 = fils1 + 1;
	    if (fils2 < fin) {
/*              il existe 2 fils . selection du plus grand */
		if (a[fils2] > a[fils1]) {
		    fil = fils2;
		}
	    }

/*           ici fil est le plus grand des fils */
	    if (a[per] < a[fil]) {
/*              permutation de per et fil */
		aux = a[per];
		a[per] = a[fil];
		a[fil] = aux;
/*              le pointeur est aussi permute */
		naux = noanc[per];
		noanc[per] = noanc[fil];
		noanc[fil] = naux;
/*              le nouveau pere est le fils permute */
		per = fil;
		goto L10;
	    }
	}
/* L20: */
    }

/*     a chaque iteration la racine (plus grande valeur actuelle de a) */
/*     est mise a sa place (fin actuelle du tableau) et permutee avec */
/*     la valeur qui occupe cette place, puis descente de cette nouvelle */
/*     racine pour respecter le fait que tout pere est plus grand que tous */
/*     ses fils */
/*     c-a-d pour tout j tel que pere <= e(j/2)<j<nb+1 */
/*                                          a(j/2) >= a(j) */
/*                                                 >= a(j+1) */
    for (fin = *nb; fin >= 2; --fin) {
/*        la permutation premier dernier */
	aux = a[fin];
	a[fin] = a[1];
	a[1] = aux;
/*        le pointeur est aussi permute */
	naux = noanc[fin];
	noanc[fin] = noanc[1];
	noanc[1] = naux;

/*        descendre a(1) entre 1 et fin */
	per = 1;

/*        le fils 1 du pere */
L30:
	fils1 = per << 1;
	if (fils1 < fin) {
/*           il existe un fils1 */
	    fil = fils1;
	    fils2 = fils1 + 1;
	    if (fils2 < fin) {
/*              il existe 2 fils . selection du plus grand */
		if (a[fils2] > a[fils1]) {
		    fil = fils2;
		}
	    }

/*           ici fil est le plus grand des fils */
	    if (a[per] < a[fil]) {
/*              permutation de per et fil */
		aux = a[per];
		a[per] = a[fil];
		a[fil] = aux;
/*              le pointeur est aussi permute */
		naux = noanc[per];
		noanc[per] = noanc[fil];
		noanc[fil] = naux;
/*              le nouveau pere est le fils permute */
		per = fil;
		goto L30;
	    }
	}
/* L50: */
    }
    return 0;
} /* tritas_ */

