/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TYPEVAR = 258,
     ABSTYPE = 259,
     DATA = 260,
     TYPESYM = 261,
     DEC = 262,
     INFIX = 263,
     INFIXR = 264,
     USES = 265,
     PRIVATE = 266,
     DISPLAY = 267,
     SAVE = 268,
     WRITE = 269,
     TO = 270,
     EXIT = 271,
     EDIT = 272,
     DEFEQ = 273,
     OR = 274,
     VALOF = 275,
     IS = 276,
     GIVES = 277,
     THEN = 278,
     FORALL = 279,
     MODSYM = 280,
     PUBCONST = 281,
     PUBFUN = 282,
     PUBTYPE = 283,
     END = 284,
     MU = 285,
     IN = 286,
     WHEREREC = 287,
     WHERE = 288,
     ELSE = 289,
     BIN_BASE = 290,
     LBINARY1 = 291,
     RBINARY1 = 292,
     LBINARY2 = 293,
     RBINARY2 = 294,
     LBINARY3 = 295,
     RBINARY3 = 296,
     LBINARY4 = 297,
     RBINARY4 = 298,
     LBINARY5 = 299,
     RBINARY5 = 300,
     LBINARY6 = 301,
     RBINARY6 = 302,
     LBINARY7 = 303,
     RBINARY7 = 304,
     LBINARY8 = 305,
     RBINARY8 = 306,
     LBINARY9 = 307,
     RBINARY9 = 308,
     NONOP = 309,
     LAMBDA = 310,
     IF = 311,
     LETREC = 312,
     LET = 313,
     CHAR = 314,
     LITERAL = 315,
     NUMBER = 316,
     IDENT = 317,
     APPLY = 318,
     ALWAYS_REDUCE = 319
   };
#endif
/* Tokens.  */
#define TYPEVAR 258
#define ABSTYPE 259
#define DATA 260
#define TYPESYM 261
#define DEC 262
#define INFIX 263
#define INFIXR 264
#define USES 265
#define PRIVATE 266
#define DISPLAY 267
#define SAVE 268
#define WRITE 269
#define TO 270
#define EXIT 271
#define EDIT 272
#define DEFEQ 273
#define OR 274
#define VALOF 275
#define IS 276
#define GIVES 277
#define THEN 278
#define FORALL 279
#define MODSYM 280
#define PUBCONST 281
#define PUBFUN 282
#define PUBTYPE 283
#define END 284
#define MU 285
#define IN 286
#define WHEREREC 287
#define WHERE 288
#define ELSE 289
#define BIN_BASE 290
#define LBINARY1 291
#define RBINARY1 292
#define LBINARY2 293
#define RBINARY2 294
#define LBINARY3 295
#define RBINARY3 296
#define LBINARY4 297
#define RBINARY4 298
#define LBINARY5 299
#define RBINARY5 300
#define LBINARY6 301
#define RBINARY6 302
#define LBINARY7 303
#define RBINARY7 304
#define LBINARY8 305
#define RBINARY8 306
#define LBINARY9 307
#define RBINARY9 308
#define NONOP 309
#define LAMBDA 310
#define IF 311
#define LETREC 312
#define LET 313
#define CHAR 314
#define LITERAL 315
#define NUMBER 316
#define IDENT 317
#define APPLY 318
#define ALWAYS_REDUCE 319




/* Copy the first part of user declarations.  */

#include "defs.h"
#include "memory.h"
#include "typevar.h"
#include "op.h"
#include "newstring.h"
#include "module.h"
#include "expr.h"
#include "deftype.h"
#include "cons.h"
#include "eval.h"
#include "error.h"
#include "text.h"


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
	Num	numval;
	int	intval;
	Text	*textval;
	String	strval;
	Natural	charval;
	Type	*type;
	TypeList *typelist;
	DefType	*deftype;
	QType	*qtype;
	Expr	*expr;
	Branch	*branch;
	Cons	*cons;
}
/* Line 193 of yacc.c.  */
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */

/* Traditional yacc provides a global variable yyerrflag, which is
   non-zero when the parser is attempting to recover from an error.
   We use this for simple error recovery for interactive sessions
   in yylex().
 */
extern	int	yyerrflag;

global Bool
recovering(void)
{
	return yyerrflag != 0;
}

#ifdef	YYBISON
/* Bison defines a corresponding variable yyerrstatus local to
   yyparse().  To kludge around this, the Makefile comments out
   the local definition, and we supply a global one.
 */
int	yyerrflag;
#define yyerrstatus yyerrflag
#endif


/* Line 216 of yacc.c.  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1751

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  73
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  202
/* YYNRULES -- Number of states.  */
#define YYNSTATES  401

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      56,    71,     2,     2,    31,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    70,    69,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    57,     2,    72,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    32,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     7,    10,    13,    16,    19,    22,
      25,    30,    35,    37,    40,    42,    47,    51,    55,    57,
      60,    65,    67,    70,    73,    75,    78,    80,    83,    86,
      89,    92,    94,    95,    99,   101,   103,   107,   109,   113,
     117,   121,   125,   127,   129,   133,   135,   139,   141,   143,
     146,   151,   156,   160,   164,   168,   172,   176,   180,   184,
     188,   192,   196,   200,   204,   208,   212,   216,   220,   224,
     228,   229,   232,   234,   236,   240,   242,   244,   248,   251,
     256,   260,   264,   268,   272,   276,   280,   284,   288,   292,
     296,   300,   304,   308,   312,   316,   320,   324,   328,   331,
     336,   340,   344,   348,   352,   356,   360,   364,   368,   372,
     376,   380,   384,   388,   392,   396,   400,   404,   408,   413,
     417,   418,   421,   423,   427,   429,   431,   435,   437,   439,
     443,   447,   450,   451,   453,   457,   461,   463,   465,   467,
     469,   474,   479,   483,   487,   490,   493,   497,   501,   505,
     509,   513,   517,   521,   525,   529,   533,   537,   541,   545,
     549,   553,   557,   561,   565,   569,   576,   583,   590,   596,
     602,   607,   609,   613,   615,   619,   623,   629,   631,   632,
     634,   636,   638,   640,   644,   647,   649,   651,   653,   655,
     657,   659,   661,   663,   665,   667,   669,   671,   673,   675,
     677,   679,   681
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      74,     0,    -1,    74,    75,    -1,    -1,    76,    69,    -1,
       1,    69,    -1,     3,    78,    -1,     8,    80,    -1,     9,
      81,    -1,     4,    85,    -1,     6,    87,    18,    94,    -1,
       5,    87,    18,    92,    -1,    11,    -1,     7,   100,    -1,
     101,    -1,    20,   105,    21,   106,    -1,   105,    21,   106,
      -1,   105,    18,   106,    -1,   106,    -1,    14,   105,    -1,
      14,   105,    15,    64,    -1,    12,    -1,    10,    83,    -1,
      13,   112,    -1,    17,    -1,    17,   112,    -1,    16,    -1,
      25,   112,    -1,    26,    77,    -1,    27,    77,    -1,    28,
      77,    -1,    29,    -1,    -1,    77,    31,   112,    -1,   112,
      -1,    79,    -1,    78,    31,    79,    -1,   112,    -1,    66,
      70,    82,    -1,    66,    31,    80,    -1,    66,    70,    82,
      -1,    66,    31,    81,    -1,    65,    -1,    84,    -1,    83,
      31,    84,    -1,   112,    -1,    85,    31,    86,    -1,    86,
      -1,    87,    -1,   112,    88,    -1,   112,    56,    91,    71,
      -1,   112,    56,    90,    71,    -1,    91,    38,    91,    -1,
      91,    39,    91,    -1,    91,    40,    91,    -1,    91,    41,
      91,    -1,    91,    42,    91,    -1,    91,    43,    91,    -1,
      91,    44,    91,    -1,    91,    45,    91,    -1,    91,    46,
      91,    -1,    91,    47,    91,    -1,    91,    48,    91,    -1,
      91,    49,    91,    -1,    91,    50,    91,    -1,    91,    51,
      91,    -1,    91,    52,    91,    -1,    91,    53,    91,    -1,
      91,    54,    91,    -1,    91,    55,    91,    -1,    -1,    91,
      88,    -1,    91,    -1,    90,    -1,    91,    31,    89,    -1,
     112,    -1,    93,    -1,    93,    19,    92,    -1,   112,    95,
      -1,   112,    56,    98,    71,    -1,    94,    38,    94,    -1,
      94,    39,    94,    -1,    94,    40,    94,    -1,    94,    41,
      94,    -1,    94,    42,    94,    -1,    94,    43,    94,    -1,
      94,    44,    94,    -1,    94,    45,    94,    -1,    94,    46,
      94,    -1,    94,    47,    94,    -1,    94,    48,    94,    -1,
      94,    49,    94,    -1,    94,    50,    94,    -1,    94,    51,
      94,    -1,    94,    52,    94,    -1,    94,    53,    94,    -1,
      94,    54,    94,    -1,    94,    55,    94,    -1,   112,    95,
      -1,   112,    56,    98,    71,    -1,    94,    38,    94,    -1,
      94,    39,    94,    -1,    94,    40,    94,    -1,    94,    41,
      94,    -1,    94,    42,    94,    -1,    94,    43,    94,    -1,
      94,    44,    94,    -1,    94,    45,    94,    -1,    94,    46,
      94,    -1,    94,    47,    94,    -1,    94,    48,    94,    -1,
      94,    49,    94,    -1,    94,    50,    94,    -1,    94,    51,
      94,    -1,    94,    52,    94,    -1,    94,    53,    94,    -1,
      94,    54,    94,    -1,    94,    55,    94,    -1,    30,    99,
      22,    94,    -1,    56,    94,    71,    -1,    -1,    96,    95,
      -1,   112,    -1,    56,    94,    71,    -1,    94,    -1,    98,
      -1,    94,    31,    97,    -1,   112,    -1,   101,    -1,   111,
      31,   100,    -1,   111,    70,   102,    -1,   103,    94,    -1,
      -1,   112,    -1,   104,    31,   104,    -1,    56,   104,    71,
      -1,   112,    -1,    65,    -1,    64,    -1,    63,    -1,    56,
     105,   113,    71,    -1,    56,   113,   105,    71,    -1,    56,
     106,    71,    -1,    57,   107,    72,    -1,    57,    72,    -1,
     105,   105,    -1,   105,    38,   105,    -1,   105,    39,   105,
      -1,   105,    40,   105,    -1,   105,    41,   105,    -1,   105,
      42,   105,    -1,   105,    43,   105,    -1,   105,    44,   105,
      -1,   105,    45,   105,    -1,   105,    46,   105,    -1,   105,
      47,   105,    -1,   105,    48,   105,    -1,   105,    49,   105,
      -1,   105,    50,   105,    -1,   105,    51,   105,    -1,   105,
      52,   105,    -1,   105,    53,   105,    -1,   105,    54,   105,
      -1,   105,    55,   105,    -1,    59,   108,   110,    -1,    60,
     105,    23,   106,    36,   105,    -1,    62,   106,    18,   106,
      33,   105,    -1,    61,   104,    18,   106,    33,   105,    -1,
     105,    35,   106,    18,   105,    -1,   105,    34,   104,    18,
     105,    -1,    30,   104,    22,   105,    -1,   105,    -1,   105,
      31,   106,    -1,   105,    -1,   105,    31,   107,    -1,   109,
      22,   105,    -1,   109,    22,   105,    32,   108,    -1,   106,
      -1,    -1,    29,    -1,   112,    -1,   113,    -1,    66,    -1,
      56,   113,    71,    -1,    58,   113,    -1,    38,    -1,    39,
      -1,    40,    -1,    41,    -1,    42,    -1,    43,    -1,    44,
      -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,    49,
      -1,    50,    -1,    51,    -1,    52,    -1,    53,    -1,    54,
      -1,    55,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   177,   177,   178,   181,   182,   185,   186,   187,   189,
     190,   192,   194,   195,   196,   197,   199,   201,   203,   204,
     205,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   221,   222,   225,   226,   229,   232,   234,
     238,   240,   244,   247,   248,   251,   254,   255,   258,   261,
     262,   265,   267,   271,   275,   279,   283,   287,   291,   295,
     299,   303,   307,   311,   315,   319,   323,   327,   331,   335,
     341,   342,   345,   346,   349,   352,   355,   356,   364,   366,
     368,   373,   378,   383,   388,   393,   398,   403,   408,   413,
     418,   423,   428,   433,   438,   443,   448,   453,   461,   463,
     465,   470,   475,   480,   485,   490,   495,   500,   505,   510,
     515,   520,   525,   530,   535,   540,   545,   550,   555,   557,
     560,   561,   565,   567,   570,   571,   574,   578,   581,   582,
     585,   588,   591,   594,   595,   596,   599,   600,   601,   602,
     603,   605,   607,   609,   611,   612,   614,   618,   622,   626,
     630,   634,   638,   642,   646,   650,   654,   658,   662,   666,
     670,   674,   678,   682,   686,   688,   690,   692,   694,   696,
     698,   702,   703,   707,   710,   714,   716,   729,   740,   741,
     744,   745,   748,   749,   750,   753,   754,   755,   756,   757,
     758,   759,   760,   761,   762,   763,   764,   765,   766,   767,
     768,   769,   770
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TYPEVAR", "ABSTYPE", "DATA", "TYPESYM",
  "DEC", "INFIX", "INFIXR", "USES", "PRIVATE", "DISPLAY", "SAVE", "WRITE",
  "TO", "EXIT", "EDIT", "DEFEQ", "OR", "VALOF", "IS", "GIVES", "THEN",
  "FORALL", "MODSYM", "PUBCONST", "PUBFUN", "PUBTYPE", "END", "MU", "','",
  "'|'", "IN", "WHEREREC", "WHERE", "ELSE", "BIN_BASE", "LBINARY1",
  "RBINARY1", "LBINARY2", "RBINARY2", "LBINARY3", "RBINARY3", "LBINARY4",
  "RBINARY4", "LBINARY5", "RBINARY5", "LBINARY6", "RBINARY6", "LBINARY7",
  "RBINARY7", "LBINARY8", "RBINARY8", "LBINARY9", "RBINARY9", "'('", "'['",
  "NONOP", "LAMBDA", "IF", "LETREC", "LET", "CHAR", "LITERAL", "NUMBER",
  "IDENT", "APPLY", "ALWAYS_REDUCE", "';'", "':'", "')'", "']'", "$accept",
  "lines", "line", "cmd", "idlist", "newtvlist", "newtv", "infixlist",
  "infixrlist", "precedence", "uselist", "use", "abstypelist", "abstype",
  "newtype", "tvargs", "tvlist", "tvpair", "tv", "constypelist",
  "constype", "type", "typeargs", "typearg", "typelist", "typepair",
  "mu_tv", "decl", "simple_decl", "q_type", "start_dec", "tuple", "expr",
  "exprbody", "exprlist", "rulelist", "formals", "optend", "name", "ident",
  "binop", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,    44,   124,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,    40,    91,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,   319,    59,
      58,    41,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    73,    74,    74,    75,    75,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    77,    77,    78,    78,    79,    80,    80,
      81,    81,    82,    83,    83,    84,    85,    85,    86,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      88,    88,    89,    89,    90,    91,    92,    92,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      95,    95,    96,    96,    97,    97,    98,    99,   100,   100,
     101,   102,   103,   104,   104,   104,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   106,   106,   107,   107,   108,   108,   109,   110,   110,
     111,   111,   112,   112,   112,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     0,     2,     2,     2,     2,     2,     2,
       4,     4,     1,     2,     1,     4,     3,     3,     1,     2,
       4,     1,     2,     2,     1,     2,     1,     2,     2,     2,
       2,     1,     0,     3,     1,     1,     3,     1,     3,     3,
       3,     3,     1,     1,     3,     1,     3,     1,     1,     2,
       4,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       0,     2,     1,     1,     3,     1,     1,     3,     2,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     3,
       0,     2,     1,     3,     1,     1,     3,     1,     1,     3,
       3,     2,     0,     1,     3,     3,     1,     1,     1,     1,
       4,     4,     3,     3,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     6,     6,     6,     5,     5,
       4,     1,     3,     1,     3,     3,     5,     1,     0,     1,
       1,     1,     1,     3,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,    21,     0,     0,    26,    24,     0,     0,
       0,     0,     0,    31,     0,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,     0,     0,     0,     0,     0,     0,     0,
     139,   138,   137,   182,     2,     0,    14,   171,    18,     0,
     136,   181,     5,     0,     6,    35,    37,     9,    47,    48,
       0,    75,     0,     0,    13,   128,     0,   180,     0,     7,
       0,     8,    22,    43,    45,    23,    19,   136,    25,     0,
      27,    28,    34,    29,    30,     0,     0,   133,   171,     0,
       0,   144,   173,     0,   184,   171,   177,   178,     0,     0,
       0,     0,     4,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   145,   132,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    70,    75,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,     0,   142,   183,     0,     0,
     143,   179,   164,     0,     0,     0,     0,    17,    16,   172,
       0,     0,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     130,     0,    36,    46,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,     0,     0,    71,     0,     0,    11,    76,     0,
     120,    10,   120,   129,    39,    42,    38,    41,    40,    44,
      20,    15,    33,   135,   170,   134,   140,   141,   174,   175,
       0,     0,     0,     0,     0,   131,    51,     0,    50,     0,
     127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    98,   120,   122,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    98,     0,     0,     0,     0,   169,
     168,    74,    73,    72,     0,   119,    77,    80,    81,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,     0,     0,     0,   121,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,     0,   176,   165,
     167,   166,   118,     0,   123,    99,     0,    99,   124,   126,
     125
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    54,    55,    91,    64,    65,    79,    81,   266,
      82,    83,    67,    68,    69,   160,   341,   252,    70,   257,
     258,   259,   312,   313,   399,   366,   289,    74,    75,   230,
     231,    96,   136,   106,   103,   107,   108,   202,    76,    87,
     138
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -302
static const yytype_int16 yypact[] =
{
    -302,   646,  -302,   -40,   -28,   -28,   -28,   -28,  1369,   -29,
     -24,   -28,  -302,  -302,   -28,  1243,  -302,   -28,  1243,   -28,
     -28,   -28,   -28,  -302,   -25,  -302,  -302,  -302,  -302,  -302,
    -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,
    -302,  -302,  -302,  1179,   922,  1547,  1243,  1243,   -25,  1243,
    -302,  -302,  -302,  -302,  -302,   -21,  -302,   795,  -302,   -18,
     -15,  -302,  -302,  1547,    29,  -302,  -302,    65,  -302,  -302,
    1565,    -7,    51,    96,  -302,  -302,    -9,  -302,    -6,  -302,
      35,  -302,    98,  -302,  -302,  -302,   698,  -302,  -302,   841,
    -302,    99,  -302,    99,    99,  1398,   143,  -302,  1068,    60,
     998,  -302,  1105,    62,  -302,  1142,  -302,   103,   113,   885,
       8,   119,  -302,  1243,  1243,  1243,   -25,  1243,  1243,  1243,
    1243,  1243,  1243,  1243,  1243,  1243,  1243,  1243,  1243,  1243,
    1243,  1243,  1243,  1243,  1243,  1243,  -302,  -302,    90,   -28,
     -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,
     -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,  1369,
    -302,   -28,  -302,    67,    67,  1369,   -29,   102,   -24,   102,
     -28,   104,  1243,   -28,   -17,  1243,   -25,  1243,  1243,  1243,
    1243,  1243,  1243,  1243,  1243,  1243,  1243,  1243,  1243,  1243,
    1243,  1243,  1243,  1243,  1243,   105,  -302,  -302,   961,  1243,
    -302,  -302,  -302,  1243,  1243,  1243,  1243,  -302,  -302,  -302,
       9,   155,  1426,  1426,  1452,  1452,  1476,  1476,  1498,  1498,
    1518,  1518,  1572,  1572,   724,   724,  1230,  1230,   661,   661,
    -302,    67,  -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,
    -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,  -302,
    -302,  -302,   107,    27,  -302,   -28,  1216,  -302,   158,  1601,
      50,  1619,    61,  -302,  -302,  -302,  -302,  -302,  -302,  -302,
    -302,  -302,  -302,  -302,  1311,   149,  -302,  -302,  -302,  1278,
     171,   175,   181,  1243,  1243,  1619,  -302,   -28,  -302,   194,
    -302,   918,    67,    67,    67,    67,    67,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
      67,  1216,    -4,    70,  -302,    67,    67,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
      67,    67,    67,  1216,  -302,  1243,  1243,  1243,  1243,  1340,
    1340,  -302,  -302,   187,    67,  -302,  -302,  1637,  1655,   140,
     191,   350,   574,   151,   746,   421,   755,   372,   429,    49,
     196,   203,   209,    -2,    38,  1034,   148,  1216,  -302,  1672,
    1672,   310,   310,  1685,  1685,  1696,  1696,   287,   287,   230,
     230,   172,   172,   -19,   -19,   166,   166,   160,  -302,  1340,
    1311,  1311,  1619,    67,  -302,    26,   995,  -302,   556,  -302,
    -302
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -302,  -302,  -302,  -302,    88,  -302,   114,    63,    84,    97,
    -302,   100,  -302,   127,   106,   108,  -302,   -16,   -70,    -3,
    -302,   246,  -250,  -302,  -302,  -301,  -302,   109,   267,  -302,
    -302,    -5,   332,     7,    74,   -60,  -302,  -302,   275,    -1,
       1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -181
static const yytype_int16 yytable[] =
{
      60,   161,    61,    66,    71,    71,    71,    77,    58,    61,
      84,   -70,   334,    85,   176,   -78,    88,   -96,    90,    92,
      92,    92,   165,    97,   -70,   166,   205,   283,    63,    62,
      45,    95,   387,    45,   330,   331,   332,    78,    53,   176,
     176,    53,    80,   110,   100,   -79,   104,    97,   112,   159,
      99,    45,   137,   332,   273,  -180,   111,   -97,   287,    53,
     139,   137,   -70,   368,   167,   -78,   168,   -96,   -92,   163,
     162,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   253,
     174,   161,   400,   332,    97,   -79,   140,   255,   288,   195,
     328,   329,   330,   331,   332,   169,   311,   -97,    45,    93,
      94,   210,    72,    73,   164,    97,    53,   333,   -92,    45,
     207,   208,   209,   256,   211,    45,   367,    53,    45,   170,
     173,   196,   201,    53,   200,   203,    53,   206,    66,    71,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   -82,
     162,   197,   260,   262,    77,   175,    61,   265,   270,    84,
     -86,   275,   272,   284,   176,    97,   276,   292,   286,   271,
     176,   318,   319,   320,   321,   322,   323,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   336,   337,   -82,
     -83,   280,   281,   282,   338,   -93,   344,   343,   287,   395,
     -86,   332,   -94,   328,   329,   330,   331,   332,   -95,   264,
     262,   397,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   328,   329,   330,
     331,   332,   267,   232,   290,   262,   330,   331,   332,   314,
     -83,   314,   330,   331,   332,   -93,   268,   233,    56,   254,
     269,   342,   -94,   278,   263,   388,    59,     0,   -95,   326,
     327,   328,   329,   330,   331,   332,   162,     0,     0,   346,
       0,   260,   262,   262,   262,   262,   262,   262,   262,   262,
     262,   262,   262,   262,   262,   262,   262,   262,   262,   262,
     262,     0,   314,     0,   262,   262,   262,   262,   262,   262,
     262,   262,   262,   262,   262,   262,   262,   262,   262,   262,
     262,   262,   262,    57,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   262,     0,     0,     0,    86,     0,     0,
      89,   318,   319,   320,   321,   322,   323,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   262,     0,     0,   -84,
       0,     0,     0,     0,     0,    98,   102,     0,   105,   109,
       0,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   -90,   262,   320,   321,   322,   323,   324,   325,   326,
     327,   328,   329,   330,   331,   332,     0,     0,     0,     0,
     261,     0,     0,     0,     0,     0,     0,     0,     0,   -84,
       0,   326,   327,   328,   329,   330,   331,   332,     0,     0,
       0,     0,   198,     0,     0,     0,     0,     0,     0,     0,
     -88,   -90,     0,     0,     0,   105,   105,   105,   -91,   105,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   285,   326,   327,
     328,   329,   330,   331,   332,     0,     0,     0,     0,     0,
     -88,     0,     0,     0,     0,     0,     0,     0,   -91,     0,
       0,     0,   291,     0,   105,     0,     0,   274,     0,   212,
     213,   214,   215,   216,   217,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,   229,     0,     0,     0,
       0,   102,     0,     0,     0,   279,   105,   105,   105,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,     0,     0,
       0,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   365,
       0,     0,     0,     0,     0,     0,     0,   393,     0,     0,
     392,     0,     0,   -85,   315,   316,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,     0,   396,     0,   339,   340,   320,   321,   322,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   332,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   398,
       0,     0,     0,   -85,     0,     0,     2,     3,     0,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,     0,    16,    17,     0,     0,    18,   105,   389,   390,
     391,    19,    20,    21,    22,    23,    24,     0,     0,     0,
       0,     0,     0,     0,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,   171,     0,   -32,   135,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    24,     0,
       0,     0,   116,   117,     0,     0,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,   -87,     0,     0,     0,     0,
       0,     0,     0,     0,   -89,   131,   132,   133,   134,   135,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   324,   325,   326,   327,   328,   329,   330,   331,
     332,     0,     0,   113,     0,   -87,   114,     0,     0,     0,
       0,     0,     0,     0,   -89,    24,   115,     0,     0,   116,
     117,     0,     0,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,   172,     0,     0,     0,     0,     0,     0,     0,
       0,    24,     0,     0,     0,   116,   117,     0,     0,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   204,     0,
       0,     0,     0,     0,     0,    24,     0,     0,     0,   116,
     117,     0,     0,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    24,     0,     0,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,     0,     0,     0,     0,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,   345,
       0,    24,     0,     0,   101,   116,   117,     0,     0,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    24,     0,
       0,     0,   277,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,     0,     0,     0,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,   393,   394,     0,     0,   197,
       0,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   332,
       0,     0,     0,     0,     0,     0,     0,     0,    24,   115,
       0,     0,   116,   117,     0,   394,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    24,   199,     0,     0,   116,
     117,     0,     0,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    24,   115,     0,     0,   116,   117,     0,     0,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    24,
       0,     0,     0,     0,     0,     0,     0,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   255,     0,     0,     0,
       0,     0,     0,     0,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,   256,    24,    45,     0,     0,     0,     0,     0,
       0,     0,    53,   133,   134,   135,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,     0,     0,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
     335,     0,   116,   117,     0,     0,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,   116,   117,     0,     0,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    63,     0,    45,     0,     0,
       0,     0,     0,     0,     0,    53,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    95,     0,    45,     0,     0,     0,
       0,     0,     0,     0,    53,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,   127,   128,   129,   130,   131,
     132,   133,   134,   135,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   129,   130,   131,   132,   133,   134,   135,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   315,   316,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,  -100,   316,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,  -101,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332
};

static const yytype_int16 yycheck[] =
{
       1,    71,     1,     4,     5,     6,     7,     8,     1,     8,
      11,    18,   262,    14,    31,    19,    17,    19,    19,    20,
      21,    22,    31,    24,    31,    31,    18,    18,    56,    69,
      58,    56,   333,    58,    53,    54,    55,    66,    66,    31,
      31,    66,    66,    48,    43,    19,    45,    48,    69,    56,
      43,    58,    70,    55,    71,    70,    49,    19,    31,    66,
      31,    70,    69,   313,    70,    69,    31,    69,    19,    18,
      71,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
      95,   161,   393,    55,    95,    69,    31,    30,    71,    98,
      51,    52,    53,    54,    55,    70,    56,    69,    58,    21,
      22,   116,     6,     7,    18,   116,    66,    56,    69,    58,
     113,   114,   115,    56,   117,    58,    56,    66,    58,    31,
      31,    71,    29,    66,    72,    22,    66,    18,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,    19,
     161,    71,   163,   164,   165,    22,   165,    65,    64,   170,
      19,   176,   173,    18,    31,   176,    71,    19,    71,   172,
      31,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    36,    33,    69,
      19,   204,   205,   206,    33,    19,    22,   287,    31,    71,
      69,    55,    19,    51,    52,    53,    54,    55,    19,   166,
     231,    71,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    51,    52,    53,
      54,    55,   168,   139,   255,   256,    53,    54,    55,   260,
      69,   262,    53,    54,    55,    69,   169,   140,     1,   161,
     170,   287,    69,   199,   165,   335,     1,    -1,    69,    49,
      50,    51,    52,    53,    54,    55,   287,    -1,    -1,   292,
      -1,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,    -1,   313,    -1,   315,   316,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,     1,    47,    48,    49,    50,    51,    52,
      53,    54,    55,   344,    -1,    -1,    -1,    15,    -1,    -1,
      18,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,   367,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,   393,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    -1,    -1,    -1,    -1,
     164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      -1,    49,    50,    51,    52,    53,    54,    55,    -1,    -1,
      -1,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    69,    -1,    -1,    -1,   113,   114,   115,    19,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,    47,    48,
      49,    50,    51,    52,    53,    54,    55,   231,    49,    50,
      51,    52,    53,    54,    55,    -1,    -1,    -1,    -1,    -1,
      69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,
      -1,    -1,   256,    -1,   172,    -1,    -1,   175,    -1,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,    -1,    -1,    -1,
      -1,   199,    -1,    -1,    -1,   203,   204,   205,   206,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,    -1,    -1,
      -1,   315,   316,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
     344,    -1,    -1,    19,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    -1,   367,    -1,   283,   284,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   393,
      -1,    -1,    -1,    69,    -1,    -1,     0,     1,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    -1,    16,    17,    -1,    -1,    20,   335,   336,   337,
     338,    25,    26,    27,    28,    29,    30,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    15,    -1,    69,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    30,    -1,
      -1,    -1,    34,    35,    -1,    -1,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    19,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    -1,    -1,    18,    -1,    69,    21,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    30,    31,    -1,    -1,    34,
      35,    -1,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    21,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    30,    -1,    -1,    -1,    34,    35,    -1,    -1,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    -1,    -1,    -1,    34,
      35,    -1,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    30,    -1,    -1,    -1,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    -1,    -1,    -1,    -1,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    71,
      -1,    30,    -1,    -1,    72,    34,    35,    -1,    -1,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    30,    -1,
      -1,    -1,    71,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    -1,    -1,    -1,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    31,    71,    -1,    -1,    71,
      -1,    -1,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,    31,
      -1,    -1,    34,    35,    -1,    71,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    30,    31,    -1,    -1,    34,
      35,    -1,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    30,    31,    -1,    -1,    34,    35,    -1,    -1,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    30,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    30,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    30,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    66,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    -1,    -1,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      32,    -1,    34,    35,    -1,    -1,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    34,    35,    -1,    -1,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    58,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    -1,    58,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    66,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    74,     0,     1,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    16,    17,    20,    25,
      26,    27,    28,    29,    30,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    75,    76,   101,   105,   106,   111,
     112,   113,    69,    56,    78,    79,   112,    85,    86,    87,
      91,   112,    87,    87,   100,   101,   111,   112,    66,    80,
      66,    81,    83,    84,   112,   112,   105,   112,   112,   105,
     112,    77,   112,    77,    77,    56,   104,   112,   105,   106,
     113,    72,   105,   107,   113,   105,   106,   108,   109,   105,
     104,   106,    69,    18,    21,    31,    34,    35,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,   105,    70,   113,    31,
      31,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      88,    91,   112,    18,    18,    31,    31,    70,    31,    70,
      31,    15,    21,    31,   104,    22,    31,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,   113,    71,    71,   105,    31,
      72,    29,   110,    22,    23,    18,    18,   106,   106,   106,
     104,   106,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     102,   103,    79,    86,    91,    91,    91,    91,    91,    91,
      91,    91,    91,    91,    91,    91,    91,    91,    91,    91,
      91,    91,    90,    91,    88,    30,    56,    92,    93,    94,
     112,    94,   112,   100,    80,    65,    82,    81,    82,    84,
      64,   106,   112,    71,   105,   104,    71,    71,   107,   105,
     106,   106,   106,    18,    18,    94,    71,    31,    71,    99,
     112,    94,    19,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    95,    96,   112,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    95,    32,    36,    33,    33,   105,
     105,    89,    90,    91,    22,    71,    92,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    98,    56,    95,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    98,   108,   105,
     105,   105,    94,    31,    71,    71,    94,    71,    94,    97,
      98
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
/*   int yyerrstatus; */
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
    { erroneous = FALSE; }
    break;

  case 3:
    { erroneous = FALSE; }
    break;

  case 4:
    { clean_slate(); mod_fetch(); }
    break;

  case 5:
    { clean_slate(); yyerrok; }
    break;

  case 7:
    { preserve(); }
    break;

  case 8:
    { preserve(); }
    break;

  case 10:
    { type_syn((yyvsp[(2) - (4)].deftype), (yyvsp[(4) - (4)].type)); }
    break;

  case 11:
    { decl_type((yyvsp[(2) - (4)].deftype), (yyvsp[(4) - (4)].cons)); }
    break;

  case 12:
    { mod_private(); }
    break;

  case 14:
    {;}
    break;

  case 15:
    { def_value((yyvsp[(2) - (4)].expr), (yyvsp[(4) - (4)].expr)); }
    break;

  case 16:
    { def_value((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 17:
    { def_value((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 18:
    { eval_expr((yyvsp[(1) - (1)].expr)); }
    break;

  case 19:
    { wr_expr((yyvsp[(2) - (2)].expr), (const char *)0); }
    break;

  case 20:
    { wr_expr((yyvsp[(2) - (4)].expr), (const char *)((yyvsp[(4) - (4)].textval)->t_start)); }
    break;

  case 21:
    { display(); }
    break;

  case 22:
    { preserve(); }
    break;

  case 23:
    { mod_save((yyvsp[(2) - (2)].strval)); }
    break;

  case 24:
    { edit((String)0); }
    break;

  case 25:
    { edit((yyvsp[(2) - (2)].strval)); }
    break;

  case 26:
    { YYACCEPT; }
    break;

  case 34:
    {;}
    break;

  case 37:
    { tv_declare((yyvsp[(1) - (1)].strval)); }
    break;

  case 38:
    { op_declare((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].intval), ASSOC_LEFT); (yyval.intval) = (yyvsp[(3) - (3)].intval); }
    break;

  case 39:
    { op_declare((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].intval), ASSOC_LEFT); (yyval.intval) = (yyvsp[(3) - (3)].intval); }
    break;

  case 40:
    { op_declare((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].intval), ASSOC_RIGHT); (yyval.intval) = (yyvsp[(3) - (3)].intval); }
    break;

  case 41:
    { op_declare((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].intval), ASSOC_RIGHT); (yyval.intval) = (yyvsp[(3) - (3)].intval); }
    break;

  case 42:
    { (yyval.intval) = (int)(yyvsp[(1) - (1)].numval); }
    break;

  case 45:
    { mod_use((yyvsp[(1) - (1)].strval)); }
    break;

  case 48:
    { abstype((yyvsp[(1) - (1)].deftype)); }
    break;

  case 49:
    { (yyval.deftype) = new_deftype((yyvsp[(1) - (2)].strval), FALSE, (yyvsp[(2) - (2)].typelist)); }
    break;

  case 50:
    { (yyval.deftype) = new_deftype((yyvsp[(1) - (4)].strval), FALSE,
						cons_type((yyvsp[(3) - (4)].type), NULL)); }
    break;

  case 51:
    { (yyval.deftype) = new_deftype((yyvsp[(1) - (4)].strval), TRUE, (yyvsp[(3) - (4)].typelist)); }
    break;

  case 52:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 53:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 54:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 55:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 56:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 57:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 58:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 59:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 60:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 61:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 62:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 63:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 64:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 65:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 66:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 67:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 68:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 69:
    { (yyval.deftype) = new_deftype((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
					    cons_type((yyvsp[(3) - (3)].type),
						(TypeList *)NULL))); }
    break;

  case 70:
    { (yyval.typelist) = NULL; }
    break;

  case 71:
    { (yyval.typelist) = cons_type((yyvsp[(1) - (2)].type), (yyvsp[(2) - (2)].typelist)); }
    break;

  case 72:
    { (yyval.typelist) = cons_type((yyvsp[(1) - (1)].type), (TypeList *)NULL); }
    break;

  case 73:
    { (yyval.typelist) = (yyvsp[(1) - (1)].typelist); }
    break;

  case 74:
    { (yyval.typelist) = cons_type((yyvsp[(1) - (3)].type), (yyvsp[(3) - (3)].typelist)); }
    break;

  case 75:
    { (yyval.type) = new_tv((yyvsp[(1) - (1)].strval)); }
    break;

  case 76:
    { (yyval.cons) = alt_cons((yyvsp[(1) - (1)].cons), (Cons *)NULL); }
    break;

  case 77:
    { (yyval.cons) = alt_cons((yyvsp[(1) - (3)].cons), (yyvsp[(3) - (3)].cons)); }
    break;

  case 78:
    { (yyval.cons) = constructor((yyvsp[(1) - (2)].strval), FALSE, (yyvsp[(2) - (2)].typelist)); }
    break;

  case 79:
    { (yyval.cons) = constructor((yyvsp[(1) - (4)].strval), TRUE, (yyvsp[(3) - (4)].typelist)); }
    break;

  case 80:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 81:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 82:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 83:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 84:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 85:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 86:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 87:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 88:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 89:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 90:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 91:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 92:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 93:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 94:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 95:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 96:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 97:
    { (yyval.cons) = constructor((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 98:
    { (yyval.type) = new_type((yyvsp[(1) - (2)].strval), FALSE, (yyvsp[(2) - (2)].typelist)); }
    break;

  case 99:
    { (yyval.type) = new_type((yyvsp[(1) - (4)].strval), TRUE, (yyvsp[(3) - (4)].typelist)); }
    break;

  case 100:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 101:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 102:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 103:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 104:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 105:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 106:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 107:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 108:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 109:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 110:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 111:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 112:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 113:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 114:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 115:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 116:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 117:
    { (yyval.type) = new_type((yyvsp[(2) - (3)].strval), TRUE,
					cons_type((yyvsp[(1) - (3)].type),
						cons_type((yyvsp[(3) - (3)].type),
							(TypeList *)NULL))); }
    break;

  case 118:
    { (yyval.type) = mu_type((yyvsp[(4) - (4)].type)); }
    break;

  case 119:
    { (yyval.type) = (yyvsp[(2) - (3)].type); }
    break;

  case 120:
    { (yyval.typelist) = (TypeList *)NULL; }
    break;

  case 121:
    { (yyval.typelist) = cons_type((yyvsp[(1) - (2)].type), (yyvsp[(2) - (2)].typelist)); }
    break;

  case 122:
    { (yyval.type) = new_type((yyvsp[(1) - (1)].strval), FALSE,
						(TypeList *)NULL); }
    break;

  case 123:
    { (yyval.type) = (yyvsp[(2) - (3)].type); }
    break;

  case 124:
    { (yyval.typelist) = cons_type((yyvsp[(1) - (1)].type), (TypeList *)NULL); }
    break;

  case 125:
    { (yyval.typelist) = (yyvsp[(1) - (1)].typelist); }
    break;

  case 126:
    { (yyval.typelist) = cons_type((yyvsp[(1) - (3)].type), (yyvsp[(3) - (3)].typelist)); }
    break;

  case 127:
    { enter_mu_tv((yyvsp[(1) - (1)].strval)); }
    break;

  case 128:
    { (yyval.qtype) = (yyvsp[(1) - (1)].qtype); }
    break;

  case 129:
    { decl_value((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].qtype)); (yyval.qtype) = (yyvsp[(3) - (3)].qtype); }
    break;

  case 130:
    { decl_value((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].qtype)); (yyval.qtype) = (yyvsp[(3) - (3)].qtype); }
    break;

  case 131:
    { (yyval.qtype) = qualified_type((yyvsp[(2) - (2)].type)); }
    break;

  case 132:
    { start_dec_type(); }
    break;

  case 133:
    { (yyval.expr) = id_expr((yyvsp[(1) - (1)].strval)); }
    break;

  case 134:
    { (yyval.expr) = pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 135:
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 136:
    { (yyval.expr) = id_expr((yyvsp[(1) - (1)].strval)); }
    break;

  case 137:
    { (yyval.expr) = num_expr((yyvsp[(1) - (1)].numval)); }
    break;

  case 138:
    { (yyval.expr) = text_expr((yyvsp[(1) - (1)].textval)->t_start, (yyvsp[(1) - (1)].textval)->t_length); }
    break;

  case 139:
    { (yyval.expr) = char_expr((yyvsp[(1) - (1)].charval)); }
    break;

  case 140:
    { (yyval.expr) = presection((yyvsp[(3) - (4)].strval), (yyvsp[(2) - (4)].expr)); }
    break;

  case 141:
    { (yyval.expr) = postsection((yyvsp[(2) - (4)].strval), (yyvsp[(3) - (4)].expr)); }
    break;

  case 142:
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 143:
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 144:
    { (yyval.expr) = e_nil; }
    break;

  case 145:
    { (yyval.expr) = apply_expr((yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].expr)); }
    break;

  case 146:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 147:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 148:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 149:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 150:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 151:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 152:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 153:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 154:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 155:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 156:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 157:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 158:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 159:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 160:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 161:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 162:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 163:
    { (yyval.expr) = apply_expr(id_expr((yyvsp[(2) - (3)].strval)),
						pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)));
				}
    break;

  case 164:
    { (yyval.expr) = func_expr((yyvsp[(2) - (3)].branch)); }
    break;

  case 165:
    { (yyval.expr) = ite_expr((yyvsp[(2) - (6)].expr), (yyvsp[(4) - (6)].expr), (yyvsp[(6) - (6)].expr)); }
    break;

  case 166:
    { (yyval.expr) = let_expr((yyvsp[(2) - (6)].expr), (yyvsp[(4) - (6)].expr), (yyvsp[(6) - (6)].expr), FALSE); }
    break;

  case 167:
    { (yyval.expr) = let_expr((yyvsp[(2) - (6)].expr), (yyvsp[(4) - (6)].expr), (yyvsp[(6) - (6)].expr), TRUE); }
    break;

  case 168:
    { (yyval.expr) = where_expr((yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr), FALSE); }
    break;

  case 169:
    { (yyval.expr) = where_expr((yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr), TRUE); }
    break;

  case 170:
    { (yyval.expr) = mu_expr((yyvsp[(2) - (4)].expr), (yyvsp[(4) - (4)].expr)); }
    break;

  case 171:
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 172:
    { (yyval.expr) = pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 173:
    { (yyval.expr) = apply_expr(e_cons,
						  pair_expr((yyvsp[(1) - (1)].expr), e_nil));
				}
    break;

  case 174:
    { (yyval.expr) = apply_expr(e_cons, pair_expr((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr))); }
    break;

  case 175:
    { (yyval.branch) = new_branch((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (Branch *)0); }
    break;

  case 176:
    { (yyval.branch) = new_branch((yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].branch)); }
    break;

  case 177:
    { (yyval.expr) = apply_expr((Expr *)0, (yyvsp[(1) - (1)].expr)); }
    break;

  case 180:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 181:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 182:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 183:
    { (yyval.strval) = (yyvsp[(2) - (3)].strval); }
    break;

  case 184:
    { (yyval.strval) = (yyvsp[(2) - (2)].strval); }
    break;

  case 185:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 186:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 187:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 188:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 189:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 190:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 191:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 192:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 193:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 194:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 195:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 196:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 197:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 198:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 199:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 200:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 201:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 202:
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;


/* Line 1267 of yacc.c.  */
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}




