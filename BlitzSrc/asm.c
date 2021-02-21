/* The BLITZ Assembler
**
** Copyright 2000-2007, Harry H. Porter III
**
** This file may be freely copied, modified and compiled, on the sole
** condition that if you modify it...
**   (1) Your name and the date of modification is added to this comment
**       under "Modifications by", and
**   (2) Your name and the date of modification is added to the printHelp()
**       routine under "Modifications by".
**
** Original Author:
**   11/12/00 - Harry H. Porter III
**
** Modifcations by:
**   03/15/06 - Harry H. Porter III
**   04/25/07 - Harry H. Porter III - Support for little endian added
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <math.h>


/* SWAP_BYTES (int)  -->  int
**
** This macro is used to swap the bytes in a 32-bit int from Big Endian order
** to Little Endian order, or vice-versa.
**
** For example:
**     i = SWAP_BYTES (i);
**
** This program was originally written for a Big Endian architecture so swapping
** bytes was never necessary.  When compiled on a Big Endian computer, this macro
** is a no-op; when compiled on a Little Endian machine, it will swap the bytes.
**
*/
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
#define SWAP_BYTES(x) \
    ((int)((((int)(x) & 0xff000000) >> 24) | \
           (((int)(x) & 0x00ff0000) >>  8) | \
           (((int)(x) & 0x0000ff00) <<  8) | \
           (((int)(x) & 0x000000ff) << 24)))
#else
#define SWAP_BYTES(x) (x)
#endif



typedef struct Expression Expression;
typedef struct Instruction Instruction;
typedef struct TableEntry TableEntry;
typedef struct String String;

union tokenValue {
  TableEntry * tableEntry;
  int          ivalue;
  double       rvalue;
} tokenValue;

struct Expression {
  int          op;
  Expression * expr1;
  Expression * expr2;
  TableEntry * tableEntry; /* This field used iff op = ID */
  int          value;      /* The value of this expression */
  double       rvalue;     /* ... possibly a double value */
  TableEntry * relativeTo; /* To entry for ABSOLUTE, TEXT,..., or extrn ID */
};

struct Instruction {
  int           op;
  int           lineNum;
  int           format;
  int           ra;
  int           rb;
  int           rc;
  Expression *  expr;
  TableEntry *  tableEntry;
  Instruction * next;
};

#define MAX_STR_LEN 200          /* Strings are limited to 200 characters. */
#define NUM_KEYWORDS 95



/*****  Symbol Table  *****/

struct String {
  int   length;
  char  string [0];
};

#define SYMBOL_TABLE_HASH_SIZE 2999    /* Size of hash table for sym table */
struct TableEntry {
  int          symbolNum;  /* 1=text,2=data,3=bss,4=absolute,5..N=others */
  TableEntry * next;       /* Link list of TableEntry's */
  int          type;       /* ID, or some keyword like ADD, SUB,... */
  int          offset;     /* The value of this symbol */
  TableEntry * relativeTo; /* To entry for ABSOLUTE, TEXT,..., or extrn ID */
  int          imported;   /* True if this ID appeared in .import */
  int          exported;   /* True if this ID appeared in .export */
  String       string;     /* The length and characters are stored directly */
};
static TableEntry * symbolTableIndex [SYMBOL_TABLE_HASH_SIZE];



/*****  Global variables *****/

char * keywords [NUM_KEYWORDS+1];
int keywordTokens [NUM_KEYWORDS+1];
int tableInitialized = 0;
int currentLine;
int errorsDetected;
int nextToken;
Instruction * instrList;
Instruction * lastInstr;
Expression * absoluteZero;
Expression * absoluteMinusOne;
int currentSegment;              /* 0, TEXT, DATA, BSS_SEG */
int textLC;
int dataLC;
int bssLC;
int finalTextLC;
int finalDataLC;
int finalBssLC;
TableEntry * textTableEntry;
TableEntry * dataTableEntry;
TableEntry * bssTableEntry;
TableEntry * absoluteTableEntry;
TableEntry * entryTableEntry;
int sawEntryLabel;               /* True if this .text seg should be 1st */
int unresolvedEquates;           /* True if we need to reprocess equates */
int anyEquatesResolved;          /* True if anything change this iteration */
int commandOptionS = 0;          /* True: print the symbol table */
int commandOptionD = 0;          /* True: print the instruction list */
int commandOptionL = 0;          /* True: print the listing */
char * commandInFileName = NULL; /* The .s filename, if provided */
char * commandOutFileName = NULL;/* The .o filename */
FILE * inputFile;                /* The .s input file */
FILE * outputFile;               /* The .o output file */



/* Function prototypes */

void processCommandLine (int argc, char ** argv);
void errorExit ();
void printHelp ();
void printError (char * msg);
void printError2 (char * msg, char * msg2);
void scan ();
void mustHave (int token, char* msg);
int getToken (void);
int scanEscape ();
int isEscape (char ch);
int hexCharToInt (char ch);
char intToHexChar (int i);
void initKeywords ();
TableEntry * lookupAndAdd (char * givenStr, int newType);
TableEntry * lookupAndAdd2 (char * givenStr, int length, int newType);
int bytesEqual (char * p, char * q, int length);
void printSymbolTable ();
void printString (TableEntry *);
String * newString (char * p);
void checkAllExportsImports ();
void checkAllExpressions ();
void printSym (int i);
void addInstrToList (Instruction *, int LCIncrAmount);
void findOpCode (Instruction * instr);
void printInstrList (Instruction * listPtr);
void printInstr (Instruction * instrPtr);
void printExpr (Expression * expr);
Instruction * newInstr (int op);
Expression * newExpression (int op);
void getOneInstruction ();
void scanRestOfLine ();
int categoryOf (int tokType);
int isRegister (int tokType);
int isFRegister (int tokType);
int getRegisterA (Instruction * p);
int getRegisterB (Instruction * p);
int getRegisterC (Instruction * p);
int getFRegisterA (Instruction * p);
int getFRegisterB (Instruction * p);
int getFRegisterC (Instruction * p);
int nextIsNot (int tokType, char * message);
Expression * getExpr ();
Expression * parseExpr0 ();
Expression * parseExpr1 ();
Expression * parseExpr2 ();
Expression * parseExpr3 ();
Expression * parseExpr4 ();
Expression * parseExpr5 ();
Expression * parseExpr6 ();
void setCurrentSegmentTo (int type);
int getLC ();
void incrementLCBy (int incr);
int notInDataOrText ();
void checkHostCompatibility ();
void swapBytesInDouble (void *p);
void resolveEquate (Instruction * instr, int printErrors);
void evalExpr (Expression * expr, int printErrors);
void processEquates ();
void passTwo ();
void printOneLine ();
void writeInteger (int i);
int writeSegment (int segType);
void writeSymbols ();
void writeOutSymbol (TableEntry * entryPtr);
void writeRelocationInfo ();
void writeRelocationEntry (int type, int addr, Expression * expr);
void writeLabels ();



/* These are the BLITZ opcodes. */

#define ADD1	96
#define ADD2	128
#define SUB1	97
#define SUB2	129
#define MUL1	98
#define MUL2	130
#define DIV1	99
#define DIV2	131
#define SLL1	100
#define SLL2	132
#define SRL1	101
#define SRL2	133
#define SRA1	102
#define SRA2	134
#define OR1	103
#define OR2	135
#define AND1	104
#define AND2	136
#define ANDN1	105
#define ANDN2	137
#define XOR1	106
#define XOR2	138
#define REM1	115
#define REM2	149
#define LOAD1	107
#define LOAD2	139
#define LOADB1	108
#define LOADB2	140
#define LOADV1	109
#define LOADV2	141
#define LOADBV1	110
#define LOADBV2	142
#define STORE1	111
#define STORE2	143
#define STOREB1	112
#define STOREB2	144
#define STOREV1	113
#define STOREV2	145
#define STOREBV1	114
#define STOREBV2	146
#define CALL1	64
#define CALL2	160
#define JMP1	65
#define JMP2	161
#define BE1	66
#define BE2	162
#define BNE1	67
#define BNE2	163
#define BL1	68
#define BL2	164
#define BLE1	69
#define BLE2	165
#define BG1	70
#define BG2	166
#define BGE1	71
#define BGE2	167
#define BVS1	74
#define BVS2	170
#define BVC1	75
#define BVC2	171
#define BNS1	76
#define BNS2	172
#define BNC1	77
#define BNC2	173
#define BSS1	78
#define BSS2	174
#define BSC1	79
#define BSC2	175
#define BIS1	80
#define BIS2	176
#define BIC1	81
#define BIC2	177
#define BPS1	82
#define BPS2	178
#define BPC1	83
#define BPC2	179
#define PUSH	84
#define POP	85
#define SETHI	192
#define SETLO	193
#define LDADDR	194
#define SYSCALL	195
#define NOP	0
#define WAIT	1
#define DEBUG	2
#define CLEARI	3
#define SETI	4
#define CLEARP	5
#define SETP	6
#define CLEARS	7
#define RETI	8
#define RET     9
#define DEBUG2	10
#define TSET	88
#define READU1	86
#define READU2	147
#define WRITEU1	87
#define WRITEU2	148
#define LDPTBR	32
#define LDPTLR	33
#define FTOI    89
#define ITOF    90
#define FADD    116
#define FSUB    117
#define FMUL    118
#define FDIV    119
#define FCMP    91
#define FSQRT   92
#define FNEG    93
#define FABS    94
#define FLOAD1  120
#define FLOAD2  150
#define FSTORE1 121
#define FSTORE2 151


/* These are the additional token types. */

enum { EOL=256, LABEL, ID, INTEGER, REAL, STRING, ABSOLUTE,
       COMMA, LBRACKET, RBRACKET, PLUS, PLUSPLUS, MINUS, MINUSMINUS,
       COLON, STAR, SLASH, PERCENT, LTLT, GTGT, GTGTGT, LPAREN, RPAREN,
       AMPERSAND, BAR, CARET, TILDE,

/* Pseudo-ops */
       TEXT, DATA, BSS_SEG, ASCII, BYTE, WORD, EXPORT, IMPORT, ALIGN, SKIP,
       EQUAL, DOUBLE,

/* Instructions with multiple formats */
       ADD, SUB, MUL, DIV, SLL, SRL, SRA, OR, AND, ANDN, XOR, REM,
       LOAD, LOADB, LOADV, LOADBV, STORE, STOREB, STOREV, STOREBV,
       CALL, JMP, BE, BNE, BL, BLE, BG, BGE, BVS, BVC,
       BNS, BNC, BSS_OP, BSC, BIS, BIC, BPS, BPC, READU, WRITEU,
       FLOAD, FSTORE,

/* Registers */
       R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15,
       F0, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,

/* Synthetic instructions */
       MOV, CMP, SET, NEG, NOT, CLR, BTST, BSET, BCLR, BTOG,

/* Instruction formats */
       A, B, C, D, E, F, G
     };



/* main()
**
** Initialize, read in the input file, perform processing, the write the .o file.
*/
main (int argc, char ** argv) {
    int i;
    errorsDetected = 0;
    currentLine = 1;
    instrList = NULL;
    lastInstr = NULL;
    currentSegment = 0;
    textLC = 0;
    dataLC = 0;
    bssLC = 0;
    sawEntryLabel = 0;
    checkHostCompatibility ();
    initKeywords ();
    textTableEntry = lookupAndAdd (".text", TEXT);
    textTableEntry->symbolNum = 1;
    dataTableEntry = lookupAndAdd (".data", DATA);
    dataTableEntry->symbolNum = 2;
    bssTableEntry = lookupAndAdd (".bss", BSS_SEG);
    bssTableEntry->symbolNum = 3;
    absoluteTableEntry = lookupAndAdd (".absolute", ABSOLUTE);
    absoluteTableEntry->symbolNum = 4;
    entryTableEntry = lookupAndAdd ("_entry", ID);
    absoluteZero = newExpression (INTEGER);
    absoluteZero->value = 0;
    absoluteZero->relativeTo = absoluteTableEntry;
    absoluteMinusOne = newExpression (INTEGER);
    absoluteMinusOne->value = -1;
    absoluteMinusOne->relativeTo = absoluteTableEntry;
    unresolvedEquates = 0;
    anyEquatesResolved = 0;
    processCommandLine (argc, argv);
    
/*****  Test lexer  *****
    // Call getToken in a loop until EOF, printing out each token as we go.
    while (1) {
        nextToken = getToken ();
        printf("%d\t", currentLine);
        printSym (nextToken);
        switch (nextToken) {
          case ID: 
            printf("\t");
            printString (tokenValue.tableEntry);
            break;
          case STRING:
            printf("\t\"");
            printString (tokenValue.tableEntry);
            printf("\"");
            break;
          case INTEGER:
            printf("\t%d", tokenValue.ivalue);
            break;
          case REAL:
            printf("\t%.17g", tokenValue.rvalue);
            break;
        }
        printf("\n");
        if (nextToken == EOF) {
          break;
        }
    }
    exit (0);
**********/

    /* Pass 1: Read through the source file and build the list of instructions. */
    scan ();
    while (nextToken != EOF) {
      getOneInstruction ();
    }

    if (instrList == NULL) {
      printError ("No legal instructions encountered");
      fprintf (stderr, "%d errors were detected!\n", errorsDetected);
      errorExit ();
    }
    finalTextLC = textLC;
    finalDataLC = dataLC;
    finalBssLC = bssLC;

    /* Process all equates. */
    processEquates ();

    /* Check exports and imports. */
    checkAllExportsImports ();

    /* Evaluate all expressions. */
    checkAllExpressions ();

    /* Print the values of all symbols, if desired. */
    if (commandOptionS) {
      printSymbolTable ();
    }

    /* If debugging, then print the instruction list. */
    if (commandOptionD) {
      printInstrList (instrList);
    }
    if (errorsDetected) {
      fprintf (stderr, "%d errors were detected!\n", errorsDetected);
      errorExit ();
    }

    /* Write the magic numbers and header info to the .o file. */
    writeInteger (0x424C5A6F);   /* "BLZo" as an integer */
    writeInteger (sawEntryLabel);
    writeInteger (textLC);
    writeInteger (dataLC);
    writeInteger (bssLC);

    /* Perform "pass 2", creating the listing if desired. */
    passTwo ();
    if ((finalTextLC != textLC)
         || (finalDataLC != dataLC)
         || (finalBssLC != bssLC)) {
      printError ("Program logic error: the LCs do not match after pass two");
    }

    /* Write the text segment data. */
    i = writeSegment (TEXT);
    if (i != textLC) {
      printError ("Program logic error: .text segment size not correct");
    }
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write the data segment data. */
    i = writeSegment (DATA);
    if (i != dataLC) {
      printError ("Program logic error: .data segment size not correct");
    }
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write the imported and exported symbols out. */
    writeSymbols ();
    writeInteger (0);             /* To mark end of symbols */
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write out all relocation information. */
    writeRelocationInfo ();
    writeInteger (0);             /* To mark end of reloc. info */
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write all the labels out, (used by symbolic debuggers). */
    writeLabels ();
    writeInteger (0);             /* To mark end of labels */
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    if (errorsDetected) {
      fprintf (stderr, "%d errors were detected!\n", errorsDetected);
      errorExit ();
    }
    exit (0);
}



/* processCommandLine (argc, argv)
**
** This routine processes the command line options.
*/
void processCommandLine (int argc, char ** argv) {
  int argCount;
  int badArgs = 0;
  int len;
  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
    argCount = 1;

    /* Scan the -h option */
    if (!strcmp (*argv, "-h")) {
      printHelp ();
      exit (1);

    /* Scan the -s option */
    } else if (!strcmp (*argv, "-s")) {
      commandOptionS = 1;

    /* Scan the -d option */
    } else if (!strcmp (*argv, "-d")) {
      commandOptionD = 1;

    /* Scan the -l option */
    } else if (!strcmp (*argv, "-l")) {
      commandOptionL = 1;

    /* Scan the -o option, which should be followed by a file name */
    } else if (!strcmp (*argv, "-o")) {
      if (argc <= 1) {
        fprintf (stderr,
          "Expecting filename after -o option.  Use -h for help display.\n");
        badArgs = 1;
      } else {
        argCount++;
        if (commandOutFileName == NULL) {
          commandOutFileName = *(argv+1);
        } else {
          fprintf (stderr,
            "Invalid command line.  Multiple output files.  Use -h for help display.\n");
          badArgs = 1;
        }
      }

    /* Scan an input file name */
    } else if ((*argv)[0] != '-') {
      if (commandInFileName == NULL) {
        commandInFileName = *argv;
      } else {
        fprintf (stderr,
          "Invalid command line.  Multiple input files.  Use -h for help display.\n");
        badArgs = 1;
      }
    } else {
      fprintf (stderr,
        "Invalid command line option (%s).  Use -h for help display.\n",
        *argv);
      badArgs = 1;
    }
  }

  /* Open the input (.s) file */
  if (commandInFileName == NULL) {
    inputFile = stdin;
  } else {
    inputFile = fopen (commandInFileName, "r");
    if (inputFile == NULL) {
      fprintf (stderr,
          "Input file \"%s\" could not be opened\n", commandInFileName);
      badArgs = 1;
    }
  }

  /* If command line problems, then abort now. */
  if (badArgs) {
    exit (1);
  }

  /* Figure out the name of the .o file. */
  if (commandOutFileName == NULL) {
    if (commandInFileName == NULL) {
      commandOutFileName = "";
      fprintf (stderr,
       "Must use -o option when input is from stdin\n", commandOutFileName);
      exit (1);
    } else {
      len = strlen (commandInFileName);
      if ((len > 2)
            && (commandInFileName [len-2] == '.')
            && (commandInFileName [len-1] == 's')) {
        commandOutFileName = (char *) calloc (1, strlen(commandInFileName) + 1);
        strcpy (commandOutFileName, commandInFileName);
        commandOutFileName [len-2] = '.';
        commandOutFileName [len-1] = 'o';
      } else {
        commandOutFileName = (char *) calloc (1, strlen(commandInFileName) + 3);
        strcpy (commandOutFileName, commandInFileName);
        commandOutFileName [len] = '.';
        commandOutFileName [len+1] = 'o';
      }
    }
  }

  /* Open the output (.o) file. */
  outputFile = fopen (commandOutFileName, "wa");
  if (outputFile == NULL) {
    fprintf (stderr,
       "Output file \"%s\" could not be opened\n", commandOutFileName);
    exit (1);
  }
}



/* errorExit ()
**
** This routine removes the output (.o) file and calls exit(1).
*/
void errorExit () {
  remove (commandOutFileName);
  exit (1);
}



/* printHelp ()
**
** This routine prints some documentation.  It is invoked whenever
** the -h option is used on the command line.
*/
void printHelp () {
  printf (
"=================================\n"
"=====                       =====\n"
"=====  The BLITZ Assembler  =====\n"
"=====                       =====\n"
"=================================\n"
"\n"
"Copyright 2000-2007, Harry H. Porter III\n"
"========================================\n"
"  Original Author:\n"
"    11/12/00 - Harry H. Porter III\n"
"  Modifcations by:\n"
"    03/15/06 - Harry H. Porter III\n"
"    04/25/07 - Harry H. Porter III - Support for little endian added\n"
"\n"
"Command Line Options\n"
"====================\n"
"  Command line options may be given in any order.\n"
"    filename\n"
"      The input source will come from this file.  (Normally this file\n"
"      will end with \".s\".)  If an input file is not given on the command\n"
"      line, the source must come from stdin.  Only one input source is allowed.\n"
"    -h\n"
"      Print this help info.  All other options are ignored.\n"
"    -l\n"
"      Print a listing on stdout.\n"
"    -s\n"
"      Print the symbol table on stdout.\n"
"    -d\n"
"      Print internal assembler info (for debugging asm.c)\n"
"    -o filename\n"
"      If there are no errors, an object file will be created.  This\n"
"      option can be used to give the object file a specific name.\n"
"      If this option is not used, then the input .s file must be named on\n"
"      the command line (i.e., the source must not come from stdin.)  In this\n"
"      case, the name of the object file will be computed from the name of\n"
"      the input file by removing the \".s\" extension, if any, and appending\n"
"      \".o\".  For example:\n"
"           test.s  -->  test.o\n"
"           foo     -->  foo.o\n"
"  \n"
"Lexical issues:\n"
"===============\n"
"  Identifiers - May contain letters, digits, and underscores.  They must\n"
"    begin with a letter or underscore.  Case is significant.  Identifiers\n"
"    are limited in length to 200 characters.\n"
"  Integers - May be specified in decimal or in hex.\n"
"    Integers must range from 0 to 2147483647.  Hex notation is, for\n"
"    example, 0x1234abcd.  0x1234ABCD is equivalent.  Shorter numbers like\n"
"    0xFFFF are not sign-extended.\n"
"  Strings - Use double quotes.  The following escape sequences are allowed:\n"
"      \\0  \\a  \\b  \\t  \\n  \\v  \\f  \\r  \\\"  \\'  \\\\  \\xHH\n"
"    where HH are any two hex digits.  Strings may not contain newlines directly;\n"
"    in other words, a string  may not span multiple lines.  The source file may\n"
"    not contain unprintable ASCII characters; use the escape sequences if you\n"
"    wish to include unprintable characters in string or character constants.\n"
"    String constants are limited in length to 200 characters.\n"
"  Characters - Use single quotes.  The same escape sequences are allowed.\n"
"  Comments - Begin with the exclamation mark (!) and extend thru end-of-line.\n"
"  Punctuation symbols - The following symbols have special meaning:\n"
"      ,  [  ]  :  .  +  ++  -  --  *  /  %  <<  >>  >>>  &  |  ^  ~  (  )  =\n"
"  Keywords - The following classes of keywords are recognized:\n"
"    BLITZ instruction op-codes (e.g., add, sub, syscall, ...)\n"
"    Synthetic instructions (e.g., mov, set, ...)\n"
"    Assembler pseudo-ops (e.g., .text, .import, .byte, ...)\n"
"    Registers (r0, r1, ... r15)\n"
"  White space - Tabs and space characters may be used between tokens.\n"
"  End-of-line - The EOL (newline) character is treated as a token, not\n"
"    as white space; the EOL is significant in syntax parsing.\n"
"\n"
"Assembler pseudo-ops\n"
"====================\n"
"  .text    The following instructions and data will be placed in the\n"
"           \"text\" segment, which will be read-only during execution.\n"
"  .data    The following instructions and data will be placed in the\n"
"           \"data\" segment, which will be read-write during execution.\n"
"  .bss     The following bytes will be reserved in the \"bss\" segment,\n"
"           which will be initialized to zero at program load time.\n"
"  .ascii   This operand expects a single string operand.  These bytes\n"
"           will be loaded into memory.  Note that no terminating NULL\n"
"           ('\\0') character will be added to the end of the string.\n"
"  .byte    This pseudo-op expects a single expression as an operand.\n"
"           This expression will be evaluated at assembly time, the value\n"
"           will be truncated to 8 bits, and the result used to initialize\n"
"           a single byte of memory.\n"
"  .word    This pseudo-op expects a single expression as an operand.\n"
"           This expression will be evaluated at assembly time to a\n"
"           32 bit value, and the result used to initialize four bytes\n"
"           of memory.  The assembler does not require alignment for .word.\n"
"  .double  This pseudo-op expects a single floating-point constant as an\n"
"           operand.  Examples include 1.2, -3.4E-21, and +4.5e+21.\n"
"  .export  This pseudo-op expects a single symbol as an operand.  This\n"
"           symbol must be given a value in this file.  This symbol with\n"
"           its value will be placed in the object file and made available\n"
"           during segment linking.\n"
"  .import  This pseudo-op expects a single symbol as an operand.  This\n"
"           symbol must not be given a value in this file; instead it will\n"
"           receive its value from another .s file during segment linking.\n"
"           All uses of this symbol in this file will be replaced by that\n"
"           value at segment-link time.\n"
"  .skip    This pseudo-op expects a single expression as an operand.\n"
"           This expression must evaluate to an absolute value.  The\n"
"           indicated number of bytes will be skipped in the current\n"
"           segment.\n"
"  .align   This instruction will insert 0, 1, 2, or 3 bytes into the\n"
"           current segment as necessary to bring the location up to an\n"
"           even multiple of 4.  No operand is used with .align.\n"
"  =        Symbols may be given values with a line of the following\n"
"           format:\n"
"                        symbol   =   expression\n"
"           These are called \"equates\".  Equates will be processed\n"
"           during the first pass, if possible.  If not, they will be\n"
"           processed after the program has been completely read in.\n"
"           The expression may use symbols that are defined later in the\n"
"           file, but this may cause the equate to be given a value\n"
"           slightly later in the assembly.  After the first pass, an\n"
"           attempt will be made to evaluate all the equates.  At this\n"
"           time, errors may be generated.  After the equates have been\n"
"           processed, the machine code can be generated in the final\n"
"           pass.\n"
"\n"
"Segments\n"
"========\n"
"  This assembler is capable of assembling BLITZ instructions and data\n"
"  and placing them in one of three \"segments\":\n"
"    .text\n"
"    .data\n"
"    .bss\n"
"  \n"
"  At run-time, the bytes placed in the .text segment will be read-only.\n"
"  At run-time, the bytes places in the .data segment will be read-write.\n"
"  At run-time, the bytes places in the .bss segment will be read-write.\n"
"  The read-only nature of the bytes in the .text segment may or may not\n"
"  be enforced by the operating system at run-time.\n"
"\n"
"  Instructions and data may be placed in either the .text or .data\n"
"  segment.  No instructions or data may be placed in the .bss segment.\n"
"  The only things that may follow the .bss pseudo-op are the following\n"
"  pseudo-ops:\n"
"    .skip\n"
"    .align\n"
"  The assembler may reserve bytes in the .bss segment but no initial\n"
"  values may be placed in these locations.  Instead, all bytes of the\n"
"  .bss segment will be initialized to zeros at program-load time.  These\n"
"  addresses may be initialized and modified during program execution.\n"
"\n"
"  Segment control is done using the following pseudo-ops:\n"
"    .text\n"
"    .data\n"
"    .bss\n"
"\n"
"  After any one of these pseudo-ops, all following instructions and data\n"
"  will be placed in the named segment.  A \"location counter\" for each of\n"
"  the three segments is maintained by the assembler.  If, for example, a\n"
"  .text pseudo-op has been used to switch to the \".text\" segment, then\n"
"  all subsequent instructions will be placed in the \".text\" segment.\n"
"  Any labels encountered will be be given values relative to the\n"
"  \".text\" segment.  As each instruction is encountered, the location\n"
"  counter for the \".text\" segment will be incremented.  If a .data\n"
"  pseudo-op is the encountered, all subsequent instructions will be placed\n"
"  in the \".data\" segment.  The location counters are not reset;  if a\n"
"  .text pseudo-op is again encountered, subsequent instructions will be\n"
"  placed in the \".text\" segment following the instructions encountered\n"
"  earlier, before the .data pseudo-op was seen.  Thus, we can \"pick up\"\n"
"  in the .text segment where we left off.\n"
"\n"
"Symbols\n"
"=======\n"
"  The assembler builds a symbol table, mapping identifiers to values.\n"
"  Each symbol is given exactly one value: there is no notion of scope\n"
"  or lexical nesting levels, as in high-level languages.  Each symbol\n"
"  is given a value which will be either:\n"
"    absolute\n"
"    relative\n"
"    external\n"
"  An absolute value consists of a 32-bit quantity.  A relative value \n"
"  consists of a 32-bit (signed) offset relative to either a segment\n"
"  or to an external symbol.  An external symbol will have its value\n"
"  assigned in some other assembly file and its value will not be\n"
"  available to the code in this file until segment-linking time.  However,\n"
"  an external symbol may be used in expressions within this file; the\n"
"  actual data will not be filled in until segment-linking time.\n"
"\n"
"  Symbols may be defined internally or externally.  If a symbol is used\n"
"  in this file, but not defined, then it must be \"imported\" using\n"
"  the .import pseudo-op.  If a symbol is defined in this file and used\n"
"  in other files, then it must be \"exported\" using an .export\n"
"  pseudo-op.  If a symbol is not exported, then its value will not be\n"
"  known to the linker; if this same symbol is imported in other files,\n"
"  then an \"undefined symbol\" error will be generated at segment-linking\n"
"  time.\n"
"\n"
"  Symbols may be defined in either of two ways:\n"
"    labels\n"
"    = equates\n"
"  If a symbol is defined by being used as a label, then it is given a\n"
"  value which consists of an offset relative to the beginning of whichever\n"
"  segment is current when the label is encountered.  This is determined by\n"
"  whether a .text, .data, or .bss pseudo-op was seen last, before the label\n"
"  was encountered.  Each label occurs in a segment and names a location in\n"
"  memory.  At segment-link time, the segments are placed in their final\n"
"  positions in memory.  Only at segment-link time does the actual address of\n"
"  the location in memory become known.  At this time, the label is assigned\n"
"  an absolute value.\n"
"\n"
"Expression Evaluation\n"
"=====================\n"
"  Instructions and pseudo-ops may contain expressions in their operands.\n"
"  Expressions have the form given by the following Context-Free Grammar.\n"
"  (In this grammar, the following meta-notation is used: characters\n"
"  enclosed in double quotes are terminals.  The braces { } are used to\n"
"  mean \"zero or more\" occurences.  The vertical bar | is used to mean\n"
"  alternation.  Parentheses are used for grouping.  The start symbol\n"
"  is \"expr\".)\n"
"    expr  ::= expr1 { \"|\" expr1 }\n"
"    expr1 ::= expr2 { \"^\" expr2 }\n"
"    expr2 ::= expr3 { \"&\" expr3 }\n"
"    expr3 ::= expr4 { ( \"<<\" | \">>\" | \">>>\" ) expr4 }\n"
"    expr4 ::= expr5 { ( \"+\" | \"-\" ) expr5 }\n"
"    expr5 ::= expr6 { ( \"*\" | \"/\" | \"%%\" ) expr6 }\n"
"    expr6 ::= \"+\" expr6 | \"-\" expr6 | \"~\" expr6\n"
"              | ID | INTEGER | STRING | \"(\" expr \")\"\n"
"\n"
"  This syntax results in the following precedences and associativities:\n"
"    highest:    unary+  unary-  ~    (right associative)\n"
"                *  /  %%              (left associative)\n"
"                +  -                 (left associative)\n"
"                <<  >>  >>>          (left associative)\n"
"                &                    (left associative)\n"
"                ^                    (left associative)\n"
"    lowest:     |                    (left associative)\n"
"\n"
"  If a string is used in an expression, it must have exactly 4 characters.\n"
"  The string will be interpreted as a 32 bit integer, based on the ASCII\n"
"  values of the 4 characters.  (\"Big Endian\" order is used: the first\n"
"  character will determine the most significant byte.)\n"
"\n"
"  The following operators are recognized in expressions:\n"
"    unary+    nop\n"
"    unary-    32-bit signed arithmetic negation\n"
"    ~         32-bit logical negation (NOT)\n"
"    *         32-bit multiplication\n"
"    /         32-bit integer division with 32-bit integer result\n"
"    %%         32-bit modulo, with 32-bit result\n"
"    binary+   32-bit signed addition\n"
"    binary-   32-bit signed subtraction\n"
"    <<        left shift logical (i.e., zeros shifted in from right)\n"
"    >>        right shift logical (i.e., zeros shifted in from left)\n"
"    >>>       right shift arithmetic (i.e., sign bit shifted in on left)\n"
"    &         32-bit logical AND\n"
"    ^         32-bit logical Exclusive-OR\n"
"    |         32-bit logical OR\n"
"\n"
"  With the shift operators (<<, >>, and >>>) the second operand must\n"
"  evaluate to an integer between 0 and 31.  With the division operators\n"
"  (/ and %%), the first operand must be non-negative and the second\n"
"  operand must be positive, since these operators are implemented with\n"
"  \"C\" operators, which are machine-dependent with negative operands.\n"
"\n"
"  All operators except addition and subtraction require both operands to\n"
"  evaluate to absolute values.  All arithmetic is done with signed 32-bit\n"
"  values.  The addition operator + requires that at least one of the operands\n"
"  evaluates to an absolute value.  If one operand is relative, then the\n"
"  result will be relative to the same location.  The subtraction operator\n"
"  requires that the second operand evaluates to an absolute value.  If the\n"
"  first operand is relative, then the result will be relative to the same\n"
"  location.  Only absolute values can be negated.\n"
"\n"
"  All expressions are evaluated at assembly-time.  An expression may\n"
"  evaluate to either an absolute 32-bit value, or may evaluate to a\n"
"  relative value.  A relative value is a 32-bit offset relative to some\n"
"  some symbol.  The offset will  be relative to the beginning of the .text\n"
"  segment, the .data segment, or the .bss segment, or the offset will be\n"
"  relative to some external symbol.  If the expression evaluates to a\n"
"  relative value, its value will not be determined until segment-link\n"
"  time.  At this time, the absolute locations of the .text, .data, and\n"
"  .bss segments will be determined and the absolute values of external\n"
"  symbols will be determined.  At segment-link time, the final, absolute\n"
"  values of all expressions will be determined by adding the values of the\n"
"  symbols (or locations of the segments) to the offsets.\n"
"\n"
"  Expressions may be used in:\n"
"    .byte\n"
"    .word\n"
"    .skip\n"
"    =\n"
"    various BLITZ instructions\n"
"  The .skip pseudo-op requires the expression evaluates to an absolute value.\n"
"  In the case of an = (equate) pseudo-op, the expression may evaluate to\n"
"  either a relative or absolute value.  In either case, the equated symbol\n"
"  will be given a relative or absolute value (respectively).  At segment-\n"
"  linking time, when the actual value is determined, the value will be\n"
"  filled in in the byte, word, or appropriate field in the instruction.\n"
"\n"
"Instruction Syntax\n"
"==================\n"
"  Each line in the assembly source file has the following general syntax:\n"
"\n"
"    [ label: ]   [ opcode   operands ]    [ \"!\" comment ]   EOL\n"
"\n"
"  The label is optional.  It need not begin in column one.  It must be\n"
"  followed by a colon token.  A label may be on a line by itself.  If\n"
"  so, it will be given an offset from the current value of the location\n"
"  counter, relative to the current segment.\n"
"\n"
"  The opcode must be a legal BLITZ instruction.  The opcode is given in\n"
"  lowercase.  The exact format of the operands depends on the instruction;\n"
"  some BLITZ instructions take no operands while some require several\n"
"  operands. Operands are separated by commas.\n"
"\n"
"  A comment is optional and extends to the end of the line if present.\n"
"\n"
"  Each line is independent.  End-of-line (EOL) is a separate token.  An\n"
"  instruction must be on only one line, although lines may be arbitrarily long.\n"
"\n"
"  Assembler pseudo-ops have the same general syntax.  Some permit labels\n"
"  and others forbid labels.\n"
"\n"
"  The following formatting and spacing conventions are recommended:\n"
"    Labels should begin in column 1.\n"
"    The op-code should be indented by 1 tab stop.\n"
"    The operands, if any, should be indented by 1 additional tab stop.\n"
"    Each BLITZ instruction should be commented.\n"
"    The comment should be indented by 2 additional tab stops.\n"
"    A single space should follow the ! comment character.\n"
"    Block comments should occur before each routine.\n"
"    Comments should be indented with 2 spaces to show logical organization.\n"
"\n"
"  Here is an example of the recommended style for BLITZ assembly code.\n"
"  (The first line shows standard tab stops.)\n"
"       1       t       t       t       t       t       t\n"
"\n"
"       ! main ()\n"
"       !\n"
"       ! This routine does such and such.\n"
"       !\n"
"               .text\n"
"               .export main\n"
"       main:   push    r1              ! Save registers\n"
"               push    r2              ! .\n"
"       loop:                           ! LOOP\n"
"               cmp     r1,10           !   IF r1>10 THEN\n"
"               ble     endif           !   .\n"
"               sub     r2,1,r2         !     r2--\n"
"       endif:                          !   ENDIF\n"
"               sub     r1,r2,r3        !   r3 := r1-r2\n"
"               ...\n"
"\n"
"Labels\n"
"======\n"
"  A label must be followed by a colon token, but the colon is not part of\n"
"  the label.  A label may appear on a line by itself or the label may appear\n"
"  on a line containing a BLITZ instruction or one of the following pseudo-ops:\n"
"    .ascii  .byte  .word  .skip\n"
"  Labels are not allowed on any other assembler pseudo-ops.\n"
"  The label will define a new symbol, and the symbol will be given an\n"
"  offset relative to the beginning of the current segment.  Labels defined\n"
"  in the current file may be exported and labels defined in other files may\n"
"  be imported.  A label will name an address in memory, and as such a label\n"
"  cannot be given a final value until segment-linking time.  During the\n"
"  assembly of the current file, labels in the file are given offsets relative\n"
"  to either the beginning of the .text, .data, or .bss segments.\n"
"\n"
"Operand Syntax\n"
"==============\n"
"  See the BLITZ instruction reference manual for details about what\n"
"  operands each instruction requires.  Operands are separated by\n"
"  commas.  Registers are specified in lowercase (e.g., r4).  A memory\n"
"  reference may be in one of the following forms, although not all forms\n"
"  are allowed in all instructions.  (Here \"R\" stands for any register.)\n"
"    [R]\n"
"    [R+R]\n"
"    [R+expr]\n"
"    [expr]\n"
"    [--R]\n"
"    [R++]\n"
"  Some instructions allow data to be included directly; in such cases\n"
"  the operand will consist of an expression.  The expression may evaluate\n"
"  to an absolute or relative value.  Certain instructions (like jmp, call,\n"
"  and the branch instructions) require the operand to be relative to the\n"
"  segment in which the instruction occurs.\n"
"\n"
"  Here are several sample instructions to illustrate operand syntax:\n"
"                add     r3,r4,r5\n"
"                mul     r7,size,r7\n"
"                sub     r1, ((x*23) << (y+1)), r1\n"
"                call    foo\n"
"                push    r6,[--r14]\n"
"                pop     [r14++],r6\n"
"                load    [r3],r9\n"
"                load    [r3+r4],r9\n"
"                load    [r3+arrayBase],r9\n"
"                load    [x],r9\n"
"                jmp     r3\n"
"                bne     loop\n"
"                set     0x12ab34cd,r8\n"
"                syscall 3\n"
"                reti\n"
"                tset    [r4],r9\n"
"                ldptbr  r5\n"
"  Note that whenever an instruction reads or writes memory, brackets are\n"
"  used.\n");
}

 

/* printError (msg)
**
** This routine is called to print an error message and the current line
** number.  It returns.
*/
void printError (char *msg) {
    fprintf (stderr, "Error on line %d: %s\n", currentLine, msg);
    errorsDetected++;
}

 

/* printError2 (msg, msg2)
**
** This routine is called to print an error message and the current line
** number.  It returns.
*/
void printError2 (char *msg, char * msg2) {
    fprintf (stderr, "Error on line %d: %s%s\n", currentLine, msg, msg2);
    errorsDetected++;
}




/* scan ()
**
** This routine advances one token by calling the lexer.
*/
void scan () {
    if (nextToken != EOF) {
        nextToken = getToken ();
    }
}



/* mustHave (token, msg)
**
** The next token must be 'token'.  If so, scan it.  If not, it is
** a syntax error.
*/
void mustHave (int tok, char * msg) {
    if (nextToken == tok)
        scan ();
    else {
        printError (msg);
    }
}



/* getToken ()
**
** Scan the next token and return it.  Side-effects tokenVal and
** currentLine.  Returns EOF repeatedly after enf-of-file is reached.
*/
int getToken (void) {
  int ch, ch2, containsDot, lengthError, numDigits;
  int intVal, t, intOverflow, realOverflow, sign;
  double realValue, exp, power;
  char lexError2 [] = "Illegal character xxxxxxx in string ignoredxxxxxx";
  char buffer [MAX_STR_LEN+1];          /* buffer for saving a string */
  int next, i;                             /* index into buffer */

  /* If last token was EOL, then increment line number counter. */
  /* Note: if lines end with NL & CR, the line numbering will be doubled. */
  if (nextToken == EOL) {
    currentLine++;
  }
  while (1) {
    ch = getc (inputFile);

    /* Process enf-of-file... */
    if (ch == EOF) {
      ungetc (ch, inputFile);
      return EOF;

    /* Process newline... */
    } else if (ch == '\n') {
      return EOL;

    /* Process CR... */
    } else if (ch == '\r') {
      return EOL;

    /* Process other white space... */
    } else if (ch == ' ' || ch == '\t') {
      /* do nothing */

    /* Process exclamation and comments... */
    } else if (ch == '!') {
      while (1) {
        ch = getc (inputFile);
        if (ch == EOF) {
          printError ("End-of-file encountered within a comment");
          ungetc (ch, inputFile);
          return EOF;
        } else if (ch == '\n') {
          return EOL;
        } else if (ch == '\r') {
          return EOL;
        }
      }

    /* Process strings... */
    } else if (ch == '"') {
      next = 0;
      lengthError = 0;
      while (1) {
        ch2 = getc (inputFile);
        if (ch2 == '"') {
          break;
        } else if (ch2 == '\n') {
          printError ("End-of-line (NL) encountered within a string");
          ungetc (ch2, inputFile);
          break;
        } else if (ch2 == '\r') {
          printError ("End-of-line (CR) encountered within a string");
          ungetc (ch2, inputFile);
          break;
        } else if (ch2 == EOF) {
          printError ("EOF encountered within a string");
          ungetc (ch2, inputFile);
          break;
        } else if ((ch2 < 32) || (ch2 > 126)) {
          sprintf (lexError2,
              "Illegal character 0x%02x in string ignored", ch2);
          printError (lexError2);
        } else {
          if (ch2 == '\\') {
            ch2 = scanEscape ();
          }
          if (next >= MAX_STR_LEN) {
            lengthError = 1;
          } else {
            buffer [next++] = ch2;
          }
        }
      }
      tokenValue.tableEntry = lookupAndAdd2 (buffer, next, ID);
      if (lengthError) {
        printError ("Maximum string length exceeded");
      }
      return STRING;

    /* Process identifiers... */
    } else if (isalpha (ch) || (ch=='.') || (ch=='_')) {
      lengthError = 0;
      if (ch=='.') {
        containsDot = 1;
      } else {
        containsDot = 0;
      }
      next = 0;
      while (isalpha (ch) || isdigit (ch) || (ch=='.') || (ch=='_')) {
        if (ch=='.') {
          containsDot = 1;
        }
        if (next >= MAX_STR_LEN) {
          lengthError = 1;
        } else {
          buffer [next++] = ch;
        }
        ch = getc (inputFile);
      }
      ungetc (ch, inputFile);
      tokenValue.tableEntry = lookupAndAdd2 (buffer, next, ID);
      /* If already there, then its type may be ALIGN, ..., ADD, ..., or ID */
      if (containsDot && (tokenValue.tableEntry->type == ID)) {
        printError ("Unexpected period within identifier");
      }
      if (lengthError) {
        printError ("Maximum string length exceeded");
      }
      return tokenValue.tableEntry->type;

    /* Process character constants... */
    } else if (ch == '\'') {
      ch2 = getc (inputFile);
      if (ch2 == '\\') {
        ch2 = scanEscape ();
      }
      tokenValue.ivalue = ch2;
      ch2 = getc (inputFile);
      if (ch2 != '\'') {
        ungetc (ch2, inputFile);
        printError ("Expecting closing quote in character constant");
      }
      return INTEGER;

    /* Process integers... */
    } else if (isdigit (ch)) {

      /* See if we have 0x...*/
      if (ch == '0') {
        ch2 = getc (inputFile);
        if (ch2 == 'x') {
          numDigits = 0;
          ch = getc (inputFile);
          if (hexCharToInt (ch) < 0) {
            printError ("Must have a hex digit after 0x");
          }
          next = intVal = intOverflow = 0;
          while (hexCharToInt (ch) >= 0) {
            intVal = (intVal << 4) + hexCharToInt(ch);
            numDigits++;
            ch = getc (inputFile);
          }
          ungetc (ch, inputFile);
          if (numDigits > 8) {
            printError ("Hex constants must be 8 or fewer digits");
            intVal = 0;
          }
          tokenValue.ivalue = intVal;
          return INTEGER;
        }
        ungetc (ch2, inputFile);
      }

      /* Otherwise we have a string of decimal numerals. */
      intVal = intOverflow = realOverflow = 0;
      exp = 1.0;
      realValue = 0.0;
      while (isdigit (ch)) {
        t = intVal * 10 + (ch - '0');
        if (t < intVal) {
          intOverflow = 1;
        }
        intVal = t;
        realValue = (realValue * 10.0) + (double) (ch - '0');
        if (realValue > DBL_MAX) {
          realOverflow = 1;
        }
        ch = getc (inputFile);
      }

      /* If we have a real number... */
      if ((ch == '.') || (ch == 'e') || (ch == 'E')) {
        /* Read in the fractional part. */
        if (ch == '.') {
          ch = getc (inputFile);
          if (!isdigit (ch)) {
            printError ("At least one digit is required after decimal");
          }
          while (isdigit (ch)) {
            exp *= 10.0;
            realValue = realValue + ((float) (ch - '0') / exp);
            ch = getc (inputFile);
          }
        }
        intVal = 0;
        sign = 1;
        if ((ch == 'e') || (ch == 'E')) {
          ch = getc (inputFile);
          /* Process the exponent sign, if there is one. */
          if (ch == '+') {
            ch = getc (inputFile);
          } else if (ch == '-') {
            ch = getc (inputFile);
            sign = -1;
          }
          /* Read in the exponent integer into intVal. */
          if (!isdigit (ch)) {
            printError ("Expecting exponent numerals");
          } else {
            intVal = intOverflow = 0;
            while (isdigit (ch)) {
              t = intVal * 10 + (ch - '0');
              if (t < intVal) {
                intOverflow = 1;
              }
              intVal = t;
              ch = getc (inputFile);
            }
            if (intOverflow) {
              printError ("Exponent is out of range");
              intVal = 0;
            }
          }
        }
        ungetc (ch, inputFile);
        tokenValue.rvalue = 999.888;
        power =  pow ((double) 10.0, (double) (sign * intVal));
        realValue = realValue * power;
        if (realValue > DBL_MAX) {
          realOverflow = 1;
        }
        tokenValue.rvalue = realValue;
        if (realOverflow) {
          printError ("Real number is out of range");
          tokenValue.rvalue = 0.0;
        }
        return REAL;
      } else {  /* If we have an integer... */
        ungetc (ch, inputFile);
        if (intOverflow) {
          printError ("Integer out of range (0..2147483647); use 0x80000000 for -2147483648");
          intVal = 0;
        }
        tokenValue.ivalue = intVal;
        return INTEGER;
      }

    /* Check for <<... */
    } else if (ch == '<') {
      ch2 = getc (inputFile);
      if (ch2 == '<') {
        return LTLT;
      } else {
        ungetc (ch2, inputFile);
        printError ("A lone < is not a valid token");
      }

    /* Check for >> and >>> */
    } else if (ch == '>') {
      ch2 = getc (inputFile);
      if (ch2 == '>') {
        ch2 = getc (inputFile);
        if (ch2 == '>') {
          return GTGTGT;
        } else {
          ungetc (ch2, inputFile);
          return GTGT;
        }
      } else {
        ungetc (ch2, inputFile);
        printError ("A lone > is not a valid token");
      }

    /* Check for + and ++ ... */
    } else if (ch == '+') {
      ch2 = getc (inputFile);
      if (ch2 == '+') {
        return PLUSPLUS;
      } else {
        ungetc (ch2, inputFile);
        return PLUS;
      }

    /* Check for - and -- ... */
    } else if (ch == '-') {
      ch2 = getc (inputFile);
      if (ch2 == '-') {
        return MINUSMINUS;
      } else {
        ungetc (ch2, inputFile);
        return MINUS;
      }

    /* Check for one character symbols. */
    } else if (ch == '=') {
      return EQUAL;
    } else if (ch == ',') {
      return COMMA;
    } else if (ch == '[') {
      return LBRACKET;
    } else if (ch == ']') {
      return RBRACKET;
    } else if (ch == ':') {
      return COLON;
    } else if (ch == '*') {
      return STAR;
    } else if (ch == '/') {
      return SLASH;
    } else if (ch == '%') {
      return PERCENT;
    } else if (ch == '&') {
      return AMPERSAND;
    } else if (ch == '|') {
      return BAR;
    } else if (ch == '^') {
      return CARET;
    } else if (ch == '~') {
      return TILDE;
    } else if (ch == '(') {
      return LPAREN;
    } else if (ch == ')') {
      return RPAREN;
 
    /* Otherwise, we have an invalid character; ignore it. */
    } else {
      if ((ch>=' ') && (ch<='~')) {
        sprintf (lexError2, "Illegal character '%c' ignored", ch);
      } else {
        sprintf (lexError2, "Illegal character 0x%02x ignored", ch);
      }
      printError (lexError2);
    }
  }
}



/* scanEscape ()
**
** This routine is called after we have gotten a back-slash.  It
** reads whatever characters follow and returns the character.  If
** problems arise it prints a message and returns '?'.  If EOF is
** encountered, it prints a message, calls ungetc (EOF, inputFile)
** and returns '?'.
*/
int scanEscape () {
  int ch, ch2, i, j;
  ch2 = getc (inputFile);
  if (ch2 == '\n') {
    printError ("End-of-line (NL) encountered after a \\ escape");
    ungetc (ch2, inputFile);
    return '?';
  }
  if (ch2 == '\r') {
    printError ("End-of-line (CR) encountered after a \\ escape");
    ungetc (ch2, inputFile);
    return '?';
  }
  if (ch2 == EOF) {
    printError ("End-of-file encountered after a \\ escape");
    ungetc (ch2, inputFile);
    return '?';
  }
  i = isEscape(ch2);
  if (i != -1) {
    return i;
  } else if (ch2 == 'x') {
    ch = getc (inputFile);  // Get 1st hex digit
    if (ch == '\n') {
      printError ("End-of-line (NL) encountered after a \\x escape");
      return '?';
    }
    if (ch == '\r') {
      printError ("End-of-line (CR) encountered after a \\x escape");
      return '?';
    }
    if (ch == EOF) {
      printError ("End-of-file encountered after a \\x escape");
      ungetc (ch, inputFile);
      return '?';
    }
    i = hexCharToInt (ch);
    if (i < 0) {
      printError ("Must have a hex digit after \\x");
      return '?';
    } else {
      ch2 = getc (inputFile);
      if (ch2 == '\n') {
        printError ("End-of-line (NL) encountered after a \\x escape");
        return '?';
      }
      if (ch2 == '\r') {
        printError ("End-of-line (CR) encountered after a \\x escape");
        return '?';
      }
      if (ch2 == EOF) {
        printError ("End-of-file encountered after a \\x escape");
        ungetc (ch2, inputFile);
        return '?';
      }
      j = hexCharToInt (ch2);
      if (j < 0) {
        printError ("Must have two hex digits after \\x");
        return '?';
      }
      return (i<<4) + j;
    }
  } else {
    printError ("Illegal escape (only \\0, \\a, \\b, \\t, \\n, \\v, \\f, \\r, \\\", \\\', \\\\, and \\xHH allowed)");
    return '?';
  }
}



/* isEscapeChar (char)  -->  ASCII Value
**
** This routine is passed a char, such as 'n'.  If this char is one
** of the escape characters, (e.g., \n), then this routine returns the
** ASCII value (e.g., 10).  Otherwise, it returns -1.
*/
int isEscape (char ch) {
  if (ch == '0') {
    return 0;
  } else if (ch == 'a') {
    return '\a';
  } else if (ch == 'b') {
    return '\b';
  } else if (ch == 't') {
    return '\t';
  } else if (ch == 'n') {
    return '\n';
  } else if (ch == 'v') {
    return '\v';
  } else if (ch == 'f') {
    return '\f';
  } else if (ch == 'r') {
    return '\r';
  } else if (ch == '\"') {
    return '\"';
  } else if (ch == '\'') {
    return '\'';
  } else if (ch == '\\') {
    return '\\';
  } else {
    return -1;
  }
}



/* hexCharToInt (char)  --> int
**
** This routine is passed a character. If it is a hex digit, i.e.,
**    0, 1, 2, ... 9, a, b, ... f, A, B, ... F
** then it returns its value (0..15).  Otherwise, it returns -1.
*/
int hexCharToInt (char ch) {
  if (('0'<=ch) && (ch<='9')) {
    return ch-'0';
  } else if (('a'<=ch) && (ch<='f')) {
    return ch-'a' + 10;
  } else if (('A'<=ch) && (ch<='F')) {
    return ch-'A' + 10;
  } else {
    return -1;
  }
}



/* intToHexChar (int)
**
** This routine is passed an integer 0..15.  It returns a char
** from 0 1 2 3 4 5 6 7 8 9 A B C D E F
*/
char intToHexChar (int i) {
  if (i<10) {
    return '0' + i;
  } else {
    return 'A' + (i-10);
  }
}



/* initKeywords ()
**
** This routine adds each keyword to the symbol table with the corresponding
** type code.
*/
void initKeywords () {
  lookupAndAdd ("add", ADD);
  lookupAndAdd ("sub", SUB);
  lookupAndAdd ("mul", MUL);
  lookupAndAdd ("div", DIV);
  lookupAndAdd ("sll", SLL);
  lookupAndAdd ("srl", SRL);
  lookupAndAdd ("sra", SRA);
  lookupAndAdd ("or", OR);
  lookupAndAdd ("and", AND);
  lookupAndAdd ("andn", ANDN);
  lookupAndAdd ("xor", XOR);
  lookupAndAdd ("rem", REM);
  lookupAndAdd ("load", LOAD);
  lookupAndAdd ("loadb", LOADB);
  lookupAndAdd ("loadv", LOADV);
  lookupAndAdd ("loadbv", LOADBV);
  lookupAndAdd ("store", STORE);
  lookupAndAdd ("storeb", STOREB);
  lookupAndAdd ("storev", STOREV);
  lookupAndAdd ("storebv", STOREBV);
  lookupAndAdd ("call", CALL);
  lookupAndAdd ("jmp", JMP);
  lookupAndAdd ("be", BE);
  lookupAndAdd ("bne", BNE);
  lookupAndAdd ("bl", BL);
  lookupAndAdd ("ble", BLE);
  lookupAndAdd ("bg", BG);
  lookupAndAdd ("bge", BGE);
  lookupAndAdd ("bvs", BVS);
  lookupAndAdd ("bvc", BVC);
  lookupAndAdd ("bns", BNS);
  lookupAndAdd ("bnc", BNC);
  lookupAndAdd ("bss", BSS_OP);
  lookupAndAdd ("bsc", BSC);
  lookupAndAdd ("bis", BIS);
  lookupAndAdd ("bic", BIC);
  lookupAndAdd ("bps", BPS);
  lookupAndAdd ("bpc", BPC);
  lookupAndAdd ("push", PUSH);
  lookupAndAdd ("pop", POP);
  lookupAndAdd ("sethi", SETHI);
  lookupAndAdd ("setlo", SETLO);
  lookupAndAdd ("ldaddr", LDADDR);
  lookupAndAdd ("syscall", SYSCALL);
  lookupAndAdd ("nop", NOP);
  lookupAndAdd ("wait", WAIT);
  lookupAndAdd ("debug", DEBUG);
  lookupAndAdd ("cleari", CLEARI);
  lookupAndAdd ("seti", SETI);
  lookupAndAdd ("clearp", CLEARP);
  lookupAndAdd ("setp", SETP);
  lookupAndAdd ("clears", CLEARS);
  lookupAndAdd ("reti", RETI);
  lookupAndAdd ("ret", RET);
  lookupAndAdd ("debug2", DEBUG2);
  lookupAndAdd ("tset", TSET);
  lookupAndAdd ("readu", READU);
  lookupAndAdd ("writeu", WRITEU);
  lookupAndAdd ("ldptbr", LDPTBR);
  lookupAndAdd ("ldptlr", LDPTLR);
  lookupAndAdd ("ftoi", FTOI);
  lookupAndAdd ("itof", ITOF);
  lookupAndAdd ("fadd", FADD);
  lookupAndAdd ("fsub", FSUB);
  lookupAndAdd ("fmul", FMUL);
  lookupAndAdd ("fdiv", FDIV);
  lookupAndAdd ("fcmp", FCMP);
  lookupAndAdd ("fsqrt", FSQRT);
  lookupAndAdd ("fneg", FNEG);
  lookupAndAdd ("fabs", FABS);
  lookupAndAdd ("fload", FLOAD);
  lookupAndAdd ("fstore", FSTORE);

  lookupAndAdd ("mov", MOV);
  lookupAndAdd ("cmp", CMP);
  lookupAndAdd ("set", SET);
  lookupAndAdd ("neg", NEG);
  lookupAndAdd ("not", NOT);
  lookupAndAdd ("clr", CLR);
  lookupAndAdd ("btst", BTST);
  lookupAndAdd ("bset", BSET);
  lookupAndAdd ("bclr", BCLR);
  lookupAndAdd ("btog", BTOG);

  lookupAndAdd (".text", TEXT);
  lookupAndAdd (".data", DATA);
  lookupAndAdd (".ascii", ASCII);
  lookupAndAdd (".byte", BYTE);
  lookupAndAdd (".word", WORD);
  lookupAndAdd (".double", DOUBLE);
  lookupAndAdd (".export", EXPORT);
  lookupAndAdd (".import", IMPORT);
  lookupAndAdd (".align", ALIGN);
  lookupAndAdd (".skip", SKIP);
  lookupAndAdd (".bss", BSS_SEG);

  lookupAndAdd (".absolute", ABSOLUTE);  /* This is not a keyword */

  lookupAndAdd ("r0", R0);
  lookupAndAdd ("r1", R1);
  lookupAndAdd ("r2", R2);
  lookupAndAdd ("r3", R3);
  lookupAndAdd ("r4", R4);
  lookupAndAdd ("r5", R5);
  lookupAndAdd ("r6", R6);
  lookupAndAdd ("r7", R7);
  lookupAndAdd ("r8", R8);
  lookupAndAdd ("r9", R9);
  lookupAndAdd ("r10", R10);
  lookupAndAdd ("r11", R11);
  lookupAndAdd ("r12", R12);
  lookupAndAdd ("r13", R13);
  lookupAndAdd ("r14", R14);
  lookupAndAdd ("r15", R15);

  lookupAndAdd ("f0", F0);
  lookupAndAdd ("f1", F1);
  lookupAndAdd ("f2", F2);
  lookupAndAdd ("f3", F3);
  lookupAndAdd ("f4", F4);
  lookupAndAdd ("f5", F5);
  lookupAndAdd ("f6", F6);
  lookupAndAdd ("f7", F7);
  lookupAndAdd ("f8", F8);
  lookupAndAdd ("f9", F9);
  lookupAndAdd ("f10", F10);
  lookupAndAdd ("f11", F11);
  lookupAndAdd ("f12", F12);
  lookupAndAdd ("f13", F13);
  lookupAndAdd ("f14", F14);
  lookupAndAdd ("f15", F15);
}



/* lookupAndAdd (givenStr, newType)
**
** This routine is passed a pointer to a string of characters, terminated
** by '\0'.  It looks it up in the table.  If there is already an entry in
** the table, it returns a pointer to the previously stored entry.
** If not found, it allocates a new table entry, copies the new string into
** the table, initializes it's type, and returns a pointer to the new
** table entry.
*/
TableEntry * lookupAndAdd (char * givenStr, int newType) {
  return lookupAndAdd2 (givenStr, strlen(givenStr), newType);
}



/* lookupAndAdd2 (givenStr, length, newType)
**
** This routine is passed a pointer to a sequence of characters (possibly
** containing \0, of length "length".  It looks this string up in the
** symbol table.  If there is already an entry in the table, it returns
** a pointer to the previously stored entry.  If not found, it allocates
** a new table entry, copies the new string into the table, initializes
*  it's type, and returns a pointer to the new table entry.
*/
TableEntry * lookupAndAdd2 (char * givenStr, int length, int newType) {
  unsigned hashVal = 0, g;
  char * p, * q;
  int i;
  TableEntry * entryPtr;

   /* Compute the hash value for the givenStr and set hashVal to it. */
  for ( p = givenStr, i=0;
        i < length;
        p++, i++ ) {
    hashVal = (hashVal << 4) + (*p);
    if (g = hashVal & 0xf0000000) {
      hashVal = hashVal ^ (g >> 24);
      hashVal = hashVal ^ g;
    }
  }
  hashVal %= SYMBOL_TABLE_HASH_SIZE;

  /* Search the linked list and return if we find it. */
  for (entryPtr = symbolTableIndex [hashVal];
                      entryPtr;
                      entryPtr = entryPtr->next) {
    if ((length == entryPtr->string.length) &&
        (bytesEqual (entryPtr->string.string, givenStr, length))) {
      return entryPtr;
    }
  }

  /* Create an entry and initialize it. */
  entryPtr = (TableEntry *)
                calloc (1, sizeof (TableEntry) + length + 1);
  if (entryPtr == 0) {
    fprintf (stderr, "Calloc failed in lookupAndAdd!");
    errorExit ();
  }
  for (p=givenStr, q=entryPtr->string.string, i=length;
       i>0;
       p++, q++, i--) {
    *q = *p;
  } 
  entryPtr->string.length = length;
  entryPtr->type = newType;
  entryPtr->offset = 0;
  entryPtr->relativeTo = NULL;
  entryPtr->imported = 0;
  entryPtr->exported = 0;

  /* Add the new entry to the appropriate linked list and return. */
  entryPtr->next = symbolTableIndex [hashVal];
  symbolTableIndex [hashVal] = entryPtr;
  return entryPtr;
}



/* bytesEqual (p, q, length)
**
** This function is passed two pointers to blocks of characters, and a
** length.  It compares the two sequences of bytes and returns true iff
** they are both equal.
*/
int bytesEqual (char * p, char * q, int length) {
  for (; length>0; length--, p++, q++) {
    if (*p != *q) return 0;
  }
  return 1;
}



/* printSymbolTable ()
**
** This routine runs through the symbol table and prints each entry.
*/
void printSymbolTable () {
  int hashVal;
  TableEntry * entryPtr;
  printf ("STRING\tIMPORT\tEXPORT\tOFFSET\tRELATIVE-TO\n");
  printf ("======\t======\t======\t======\t===========\n");
  for (hashVal = 0; hashVal<SYMBOL_TABLE_HASH_SIZE; hashVal++) {
    for (entryPtr = symbolTableIndex [hashVal];
                       entryPtr;
                       entryPtr = entryPtr->next) {
      if (entryPtr->type == ID) {
        /* printSym (entryPtr->type); */
        printString (entryPtr);
        if (entryPtr->imported) {
          printf ("\timport");
        } else {
          printf ("\t");
        }
        if (entryPtr->exported) {
          printf ("\texport");
        } else {
          printf ("\t");
        }
        printf ("\t%d", entryPtr->offset);
        if (entryPtr->relativeTo != NULL) {
          printf ("\t");
          printString (entryPtr->relativeTo);
        } else {
          printf ("\t");
        }
        printf ("\n");
      }
    }
  }
}



/* printString (tableEntryPtr)
**
** This routine is passed a pointer to a TableEntry.  It prints the
** string, translating non-printable characters into escape sequences.
**
**     \0   \a   \b  \t   \n   \v   \f   \r   \"   \'   \\   \xHH
*/
void printString (TableEntry * tableEntryPtr) {
#define BUF_SIZE 200
  String * str = &tableEntryPtr->string;
  int bufPtr, strPtr;
  char buffer[BUF_SIZE];
  int c;
  strPtr = 0;
  while (1) {
    /* If all characters printed, then exit. */
    if (strPtr >= str->length) return;
    /* Fill up buffer */
    bufPtr = 0;
    while ((bufPtr < BUF_SIZE-4) && (strPtr < str->length)) {  // 4=room for \x12
      c = str->string [strPtr++];
      if (c == '\0') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '0';
      } else if (c == '\a') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'a';
      } else if (c == '\b') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'b';
      } else if (c == '\t') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 't';
      } else if (c == '\n') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'n';
      } else if (c == '\v') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'v';
      } else if (c == '\f') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'f';
      } else if (c == '\r') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'r';
      } else if (c == '\\') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '\\';
      } else if (c == '\"') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '\"';
      } else if (c == '\'') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '\'';
      } else if ((c>=32) && (c<127)) {
        buffer[bufPtr++] = c;
      } else {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'x';
        buffer[bufPtr++] = intToHexChar ((c>>4) & 0x0000000f);
        buffer[bufPtr++] = intToHexChar (c & 0x0000000f);
        /*  printError ("Program logic error: String contains unprintable char");  */
      }
    }
    /* Add \0 to buffer */
    buffer[bufPtr++] = '\0';
    /* Print buffer */
    printf ("%s", buffer);
  }
}



/* newString (char *) -> String *
**
** This routine allocates a String, initializes it from the argument,
** and returns a pointer to the new String.  The argument is assumed
** to be \0 terminated.
*/
String * newString (char * charPtr) {
  String * str;
  str = (String *) calloc (1, strlen (charPtr) + sizeof (String) + 1);
  printf ("size allocated = %d\n", strlen (charPtr) + sizeof (String));
  str->length = strlen (charPtr);
  strcpy (str->string, charPtr);  /* copies all characters plus \0. */
  return str;
}



/* checkAllExportsImports ()
**
** This routine runs through all the instructions, looking at
** .import and .export pseudo-ops.  If the symbol is exported, we
** make sure it has a value.  If the symbol is imported, we make
** sure it is not defined in this file.  This pass must be run after
** we process equates, since the processing of equates may assign
** values to some symbols.
*/
void checkAllExportsImports () {
  Instruction * instrPtr;
  TableEntry * tableEntry;

  /* Run through the instruction list. */
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    currentLine = instrPtr->lineNum;
    if (instrPtr->op == IMPORT) {
      tableEntry = instrPtr->tableEntry;
      if (!tableEntry->imported) {
        printError ("Program logic error: Subject of .import not marked");
      }
      if (tableEntry->relativeTo != NULL) {
        printError ("Attempt to import a symbol which is also defined in this file");
      }
    }
    if (instrPtr->op == EXPORT) {
      tableEntry = instrPtr->tableEntry;
      if (!tableEntry->exported) {
        printError ("Program logic error: Subject of .export not marked");
      }
      if (tableEntry->relativeTo == NULL) {
        printError2 ("Attempt to export a symbol which is not defined in this file: ", tableEntry->string.string);
      }
    }
  }
}



/* checkAllExpressions ()
**
** This routine runs through all the instructions and evaluates any
** expressions found in them, printing errors as they are detected.
** This pass must be run after we process equates, since the processing
** of equates may assign values to some symbols used in expressions.
** We ignore equates, since they have been previously evaluated and doing
** it again will cause duplicate error messages.
*/
void checkAllExpressions () {
  Instruction * instrPtr;
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    currentLine = instrPtr->lineNum;
    if ((instrPtr->expr != NULL) && (instrPtr->op != EQUAL)) {
      evalExpr (instrPtr->expr, 1);
    }
    if ((instrPtr->format == F)
           && (instrPtr->expr->relativeTo == absoluteTableEntry)) {
      printError ("Call, jump, or branch has an absolute value as an operand");
    }
  }
}



/* printSym (s)
**
** This routine prints the symbol given.
*/
void printSym (int i) {
  switch (i) {
    case ADD1:		printf ("ADD1"); break;
    case ADD2:		printf ("ADD2"); break;
    case SUB1:		printf ("SUB1"); break;
    case SUB2:		printf ("SUB2"); break;
    case MUL1:		printf ("MUL1"); break;
    case MUL2:		printf ("MUL2"); break;
    case DIV1:		printf ("DIV1"); break;
    case DIV2:		printf ("DIV2"); break;
    case SLL1:		printf ("SLL1"); break;
    case SLL2:		printf ("SLL2"); break;
    case SRL1:		printf ("SRL1"); break;
    case SRL2:		printf ("SRL2"); break;
    case SRA1:		printf ("SRA1"); break;
    case SRA2:		printf ("SRA2"); break;
    case OR1:		printf ("OR1"); break;
    case OR2:		printf ("OR2"); break;
    case AND1:		printf ("AND1"); break;
    case AND2:		printf ("AND2"); break;
    case ANDN1:		printf ("ANDN1"); break;
    case ANDN2:		printf ("ANDN2"); break;
    case XOR1:		printf ("XOR1"); break;
    case XOR2:		printf ("XOR2"); break;
    case REM1:		printf ("REM1"); break;
    case REM2:		printf ("REM2"); break;
    case LOAD1:		printf ("LOAD1"); break;
    case LOAD2:		printf ("LOAD2"); break;
    case LOADB1:	printf ("LOADB1"); break;
    case LOADB2:	printf ("LOADB2"); break;
    case LOADV1:	printf ("LOADV1"); break;
    case LOADV2:	printf ("LOADV2"); break;
    case LOADBV1:	printf ("LOADBV1"); break;
    case LOADBV2:	printf ("LOADBV2"); break;
    case STORE1:	printf ("STORE1"); break;
    case STORE2:	printf ("STORE2"); break;
    case STOREB1:	printf ("STOREB1"); break;
    case STOREB2:	printf ("STOREB2"); break;
    case STOREV1:	printf ("STOREV1"); break;
    case STOREV2:	printf ("STOREV2"); break;
    case STOREBV1:	printf ("STOREBV1"); break;
    case STOREBV2:	printf ("STOREBV2"); break;
    case SETHI:		printf ("SETHI"); break;
    case SETLO:		printf ("SETLO"); break;
    case CALL1:		printf ("CALL1"); break;
    case CALL2:		printf ("CALL2"); break;
    case JMP1:		printf ("JMP1"); break;
    case JMP2:		printf ("JMP2"); break;
    case BE1:		printf ("BE1"); break;
    case BE2:		printf ("BE2"); break;
    case BNE1:		printf ("BNE1"); break;
    case BNE2:		printf ("BNE2"); break;
    case BL1:		printf ("BL1"); break;
    case BL2:		printf ("BL2"); break;
    case BLE1:		printf ("BLE1"); break;
    case BLE2:		printf ("BLE2"); break;
    case BG1:		printf ("BG1"); break;
    case BG2:		printf ("BG2"); break;
    case BGE1:		printf ("BGE1"); break;
    case BGE2:		printf ("BGE2"); break;
    case BVS1:		printf ("BVS1"); break;
    case BVS2:		printf ("BVS2"); break;
    case BVC1:		printf ("BVC1"); break;
    case BVC2:		printf ("BVC2"); break;
    case BNS1:		printf ("BNS1"); break;
    case BNS2:		printf ("BNS2"); break;
    case BNC1:		printf ("BNC1"); break;
    case BNC2:		printf ("BNC2"); break;
    case BSS1:		printf ("BSS1"); break;
    case BSS2:		printf ("BSS2"); break;
    case BSC1:		printf ("BSC1"); break;
    case BSC2:		printf ("BSC2"); break;
    case BIS1:		printf ("BIS1"); break;
    case BIS2:		printf ("BIS2"); break;
    case BIC1:		printf ("BIC1"); break;
    case BIC2:		printf ("BIC2"); break;
    case BPS1:		printf ("BPS1"); break;
    case BPS2:		printf ("BPS2"); break;
    case BPC1:		printf ("BPC1"); break;
    case BPC2:		printf ("BPC2"); break;
    case PUSH:		printf ("PUSH"); break;
    case POP:		printf ("POP"); break;
    case SYSCALL:	printf ("SYSCALL"); break;
    case NOP:		printf ("NOP"); break;
    case WAIT:		printf ("WAIT"); break;
    case DEBUG:		printf ("DEBUG"); break;
    case CLEARI:	printf ("CLEARI"); break;
    case SETI:		printf ("SETI"); break;
    case CLEARP:	printf ("CLEARP"); break;
    case SETP:		printf ("SETP"); break;
    case CLEARS:	printf ("CLEARS"); break;
    case RETI:		printf ("RETI"); break;
    case RET:		printf ("RET"); break;
    case DEBUG2:	printf ("DEBUG2"); break;
    case READU1:	printf ("READU1"); break;
    case READU2:	printf ("READU2"); break;
    case WRITEU1:	printf ("WRITEU1"); break;
    case WRITEU2:	printf ("WRITEU2"); break;
    case LDPTBR:	printf ("LDPTBR"); break;
    case LDPTLR:	printf ("LDPTLR"); break;
    case ITOF:		printf ("ITOF"); break;
    case FTOI:		printf ("FTOI"); break;
    case FADD:		printf ("FADD"); break;
    case FSUB:		printf ("FSUB"); break;
    case FMUL:		printf ("FMUL"); break;
    case FDIV:		printf ("FDIV"); break;
    case FCMP:		printf ("FCMP"); break;
    case FSQRT:		printf ("FSQRT"); break;
    case FNEG:		printf ("FNEG"); break;
    case FABS:		printf ("FABS"); break;
    case FLOAD1:	printf ("FLOAD1"); break;
    case FLOAD2:	printf ("FLOAD2"); break;
    case FSTORE1:	printf ("FSTORE1"); break;
    case FSTORE2:	printf ("FSTORE2"); break;
    case EOL:		printf ("EOL"); break;
    case LABEL:		printf ("LABEL"); break;
    case ID:		printf ("ID"); break;
    case INTEGER:	printf ("INTEGER"); break;
    case REAL:		printf ("REAL"); break;
    case STRING:	printf ("STRING"); break;
    case ABSOLUTE:	printf ("ABSOLUTE"); break;
    case TEXT:		printf ("TEXT"); break;
    case DATA:		printf ("DATA"); break;
    case BSS_SEG:	printf ("BSS_SEG"); break;
    case ASCII:		printf ("ASCII"); break;
    case BYTE:		printf ("BYTE"); break;
    case WORD:		printf ("WORD"); break;
    case DOUBLE:	printf ("DOUBLE"); break;
    case EXPORT:	printf ("EXPORT"); break;
    case IMPORT:	printf ("IMPORT"); break;
    case ALIGN:		printf ("ALIGN"); break;
    case SKIP:		printf ("SKIP"); break;
    case EQUAL:		printf ("EQUAL"); break;
    case COMMA:		printf ("COMMA"); break;
    case LBRACKET:	printf ("LBRACKET"); break;
    case RBRACKET:	printf ("RBRACKET"); break;
    case PLUS:		printf ("PLUS"); break;
    case PLUSPLUS:	printf ("PLUSPLUS"); break;
    case MINUS:		printf ("MINUS"); break;
    case MINUSMINUS:	printf ("MINUSMINUS"); break;
    case COLON:		printf ("COLON"); break;
    case STAR:		printf ("STAR"); break;
    case SLASH:		printf ("SLASH"); break;
    case PERCENT:	printf ("PERCENT"); break;
    case AMPERSAND:	printf ("AMPERSAND"); break;
    case BAR:		printf ("BAR"); break;
    case CARET:		printf ("CARET"); break;
    case TILDE:		printf ("TILDE"); break;
    case LTLT:		printf ("LTLT"); break;
    case GTGT:		printf ("GTGT"); break;
    case GTGTGT:	printf ("GTGTGT"); break;
    case LPAREN:	printf ("LPAREN"); break;
    case RPAREN:	printf ("RPAREN"); break;
    case ADD:		printf ("ADD"); break;
    case SUB:		printf ("SUB"); break;
    case MUL:		printf ("MUL"); break;
    case DIV:		printf ("DIV"); break;
    case SLL:		printf ("SLL"); break;
    case SRL:		printf ("SRL"); break;
    case SRA:		printf ("SRA"); break;
    case OR:		printf ("OR"); break;
    case AND:		printf ("AND"); break;
    case ANDN:		printf ("ANDN"); break;
    case XOR:		printf ("XOR"); break;
    case REM:		printf ("REM"); break;
    case LOAD:		printf ("LOAD"); break;
    case LOADB:		printf ("LOADB"); break;
    case LOADV:		printf ("LOADV"); break;
    case LOADBV:	printf ("LOADBV"); break;
    case STORE:		printf ("STORE"); break;
    case STOREB:	printf ("STOREB"); break;
    case STOREV:	printf ("STOREV"); break;
    case STOREBV:	printf ("STOREBV"); break;
    case CALL:		printf ("CALL"); break;
    case JMP:		printf ("JMP"); break;
    case BE:		printf ("BE"); break;
    case BNE:		printf ("BNE"); break;
    case BL:		printf ("BL"); break;
    case BLE:		printf ("BLE"); break;
    case BG:		printf ("BG"); break;
    case BGE:		printf ("BGE"); break;
    case BVS:		printf ("BVS"); break;
    case BVC:		printf ("BVC"); break;
    case BNS:		printf ("BNS"); break;
    case BNC:		printf ("BNC"); break;
    case BSS_OP:	printf ("BSS_OP"); break;
    case BSC:		printf ("BSC"); break;
    case BIS:		printf ("BIS"); break;
    case BIC:		printf ("BIC"); break;
    case BPS:		printf ("BPS"); break;
    case BPC:		printf ("BPC"); break;
    case TSET:		printf ("TSET"); break;
    case LDADDR:	printf ("LDADDR"); break;
    case READU:		printf ("READU"); break;
    case WRITEU:	printf ("WRITEU"); break;
    case FLOAD:		printf ("FLOAD"); break;
    case FSTORE:	printf ("FSTORE"); break;
    case R0:		printf ("R0"); break;
    case R1:		printf ("R1"); break;
    case R2:		printf ("R2"); break;
    case R3:		printf ("R3"); break;
    case R4:		printf ("R4"); break;
    case R5:		printf ("R5"); break;
    case R6:		printf ("R6"); break;
    case R7:		printf ("R7"); break;
    case R8:		printf ("R8"); break;
    case R9:		printf ("R9"); break;
    case R10:		printf ("R10"); break;
    case R11:		printf ("R11"); break;
    case R12:		printf ("R12"); break;
    case R13:		printf ("R13"); break;
    case R14:		printf ("R14"); break;
    case R15:		printf ("R15"); break;
    case MOV:		printf ("MOV"); break;
    case CMP:		printf ("CMP"); break;
    case SET:		printf ("SET"); break;
    case NEG:		printf ("NEG"); break;
    case NOT:		printf ("NOT"); break;
    case CLR:		printf ("CLR"); break;
    case BTST:		printf ("BTST"); break;
    case BSET:		printf ("BSET"); break;
    case BCLR:		printf ("BCLR"); break;
    case BTOG:		printf ("BTOG"); break;
    case A:		printf ("A"); break;
    case B:		printf ("B"); break;
    case C:		printf ("C"); break;
    case D:		printf ("D"); break;
    case E:		printf ("E"); break;
    case F:		printf ("F"); break;
    case G:		printf ("G"); break;
    case EOF:		printf ("EOF"); break;
    default:		printf ("*****  unknown symbol  *****"); break;
  }
}



/* addInstrToList (instr)
**
** This routine is passed a pointer to an Instruction; it adds it
** to the end of the instruction list.  If it is an executable
** BLITZ instruction, this routine looks at its opcode and modifies
** that (see findOpCode()).
*/
void addInstrToList (Instruction * instr, int LCIncrAmount) {
  if (instrList==NULL) {
    instrList = instr;
    lastInstr = instr;
  } else {
    lastInstr->next = instr;
    lastInstr = instr;
  }
  findOpCode (instr);
  /*** printInstr (instr); ***/
  incrementLCBy (LCIncrAmount);
}



/* findOpCode (instr)
**
** This routine is passed a pointer to an instruction with its "op"
** and "format" fields filled in.  The "op" will correspond to what
** actually appeared in the source, e.g., ADD.  This routine will modify
** the "op" field to contain the actual op-code, e.g., ADD1 or ADD2, as
** determined by the format code.
*/
void findOpCode (Instruction * instr) {
  switch (instr->op) {
    case ADD:
      if (instr->format == D) {
        instr->op = ADD1;
      } else if (instr->format == E) {
        instr->op = ADD2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case SUB:
      if (instr->format == D) {
        instr->op = SUB1;
      } else if (instr->format == E) {
        instr->op = SUB2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case MUL:
      if (instr->format == D) {
        instr->op = MUL1;
      } else if (instr->format == E) {
        instr->op = MUL2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case DIV:
      if (instr->format == D) {
        instr->op = DIV1;
      } else if (instr->format == E) {
        instr->op = DIV2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case SLL:
      if (instr->format == D) {
        instr->op = SLL1;
      } else if (instr->format == E) {
        instr->op = SLL2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case SRL:
      if (instr->format == D) {
        instr->op = SRL1;
      } else if (instr->format == E) {
        instr->op = SRL2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case SRA:
      if (instr->format == D) {
        instr->op = SRA1;
      } else if (instr->format == E) {
        instr->op = SRA2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case OR:
      if (instr->format == D) {
        instr->op = OR1;
      } else if (instr->format == E) {
        instr->op = OR2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case AND:
      if (instr->format == D) {
        instr->op = AND1;
      } else if (instr->format == E) {
        instr->op = AND2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case ANDN:
      if (instr->format == D) {
        instr->op = ANDN1;
      } else if (instr->format == E) {
        instr->op = ANDN2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case XOR:
      if (instr->format == D) {
        instr->op = XOR1;
      } else if (instr->format == E) {
        instr->op = XOR2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case REM:
      if (instr->format == D) {
        instr->op = REM1;
      } else if (instr->format == E) {
        instr->op = REM2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case LOAD:
      if (instr->format == D) {
        instr->op = LOAD1;
      } else if (instr->format == E) {
        instr->op = LOAD2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case LOADB:
      if (instr->format == D) {
        instr->op = LOADB1;
      } else if (instr->format == E) {
        instr->op = LOADB2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case LOADV:
      if (instr->format == D) {
        instr->op = LOADV1;
      } else if (instr->format == E) {
        instr->op = LOADV2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case LOADBV:
      if (instr->format == D) {
        instr->op = LOADBV1;
      } else if (instr->format == E) {
        instr->op = LOADBV2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case STORE:
      if (instr->format == D) {
        instr->op = STORE1;
      } else if (instr->format == E) {
        instr->op = STORE2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case STOREB:
      if (instr->format == D) {
        instr->op = STOREB1;
      } else if (instr->format == E) {
        instr->op = STOREB2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case STOREV:
      if (instr->format == D) {
        instr->op = STOREV1;
      } else if (instr->format == E) {
        instr->op = STOREV2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case STOREBV:
      if (instr->format == D) {
        instr->op = STOREBV1;
      } else if (instr->format == E) {
        instr->op = STOREBV2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case CALL:
      if (instr->format == C) {
        instr->op = CALL1;
      } else if (instr->format == F) {
        instr->op = CALL2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case JMP:
      if (instr->format == C) {
        instr->op = JMP1;
      } else if (instr->format == F) {
        instr->op = JMP2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BE:
      if (instr->format == C) {
        instr->op = BE1;
      } else if (instr->format == F) {
        instr->op = BE2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BNE:
      if (instr->format == C) {
        instr->op = BNE1;
      } else if (instr->format == F) {
        instr->op = BNE2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BL:
      if (instr->format == C) {
        instr->op = BL1;
      } else if (instr->format == F) {
        instr->op = BL2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BLE:
      if (instr->format == C) {
        instr->op = BLE1;
      } else if (instr->format == F) {
        instr->op = BLE2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BG:
      if (instr->format == C) {
        instr->op = BG1;
      } else if (instr->format == F) {
        instr->op = BG2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BGE:
      if (instr->format == C) {
        instr->op = BGE1;
      } else if (instr->format == F) {
        instr->op = BGE2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BVS:
      if (instr->format == C) {
        instr->op = BVS1;
      } else if (instr->format == F) {
        instr->op = BVS2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BVC:
      if (instr->format == C) {
        instr->op = BVC1;
      } else if (instr->format == F) {
        instr->op = BVC2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BNS:
      if (instr->format == C) {
        instr->op = BNS1;
      } else if (instr->format == F) {
        instr->op = BNS2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BNC:
      if (instr->format == C) {
        instr->op = BNC1;
      } else if (instr->format == F) {
        instr->op = BNC2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BSS_OP:
      if (instr->format == C) {
        instr->op = BSS1;
      } else if (instr->format == F) {
        instr->op = BSS2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BSC:
      if (instr->format == C) {
        instr->op = BSC1;
      } else if (instr->format == F) {
        instr->op = BSC2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BIS:
      if (instr->format == C) {
        instr->op = BIS1;
      } else if (instr->format == F) {
        instr->op = BIS2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BIC:
      if (instr->format == C) {
        instr->op = BIC1;
      } else if (instr->format == F) {
        instr->op = BIC2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BPS:
      if (instr->format == C) {
        instr->op = BPS1;
      } else if (instr->format == F) {
        instr->op = BPS2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case BPC:
      if (instr->format == C) {
        instr->op = BPC1;
      } else if (instr->format == F) {
        instr->op = BPC2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case READU:
      if (instr->format == C) {
        instr->op = READU1;
      } else if (instr->format == E) {
        instr->op = READU2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case WRITEU:
      if (instr->format == C) {
        instr->op = WRITEU1;
      } else if (instr->format == E) {
        instr->op = WRITEU2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case FLOAD:
      if (instr->format == D) {
        instr->op = FLOAD1;
      } else if (instr->format == E) {
        instr->op = FLOAD2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    case FSTORE:
      if (instr->format == D) {
        instr->op = FSTORE1;
      } else if (instr->format == E) {
        instr->op = FSTORE2;
      } else {
        printError ("Program logic error: Invalid format in findOpCode()");
      }
      return;
    default:
      break;
  }
}



/* printInstrList (listPtr)
**
** This routine prints the list of Instructions pointed to by listPtr.
*/
void printInstrList (Instruction * listPtr) {
  Instruction * p = listPtr;
  printf ("line\tformat\topcode\tRa\tRb\tRc\texpr\n");
  printf ("====\t======\t======\t==\t==\t==\t====\n");
  while (p != NULL) {
    printf ("%d\t", p->lineNum);
    printSym (p->format);
    printf ("\t");
    printSym (p->op);
    printf ("\t%d\t%d\t%d\t", p->ra, p->rb, p->rc);
    if (p->tableEntry != NULL) {
      printString (p->tableEntry);
      printf ("\t");
    }
    printExpr (p->expr);
    printf ("\n");
    p = p->next;
  }
}



/* printInstr (instrPtr)
**
** This routine prints the Instruction pointed to by instrPtr.
*/
void printInstr (Instruction * instrPtr) {
  printf ("textLC\tdataLC\tbssLC\tline\tformat\topcode\tRa\tRb\tRc\texpr\n");
  printf ("======\t======\t=====\t====\t======\t======\t==\t==\t==\t====\n");
  printf ("%d\t%d\t%d\t%d\t", textLC, dataLC, bssLC, instrPtr->lineNum);
  printSym (instrPtr->format);
  printf ("\t");
  printSym (instrPtr->op);
  printf ("\t%d\t%d\t%d\t", instrPtr->ra, instrPtr->rb, instrPtr->rc);
  if (instrPtr->tableEntry != NULL) {
    printString (instrPtr->tableEntry);
    printf ("\t");
  }
  printExpr (instrPtr->expr);
  printf ("\n");
}



/* printExpr (p)
**
** This routine is passed a pointer to an expression.  It prints it,
** calling itself recursively as necessary.
*/
void printExpr (Expression * p) {
  if (p == NULL) return;
  switch (p->op) {
    case INTEGER:
      printf ("%d", p->value);
      return;
    case ID:
      printString (p->tableEntry);
      return;
    case STAR:
      printf ("(");
      printExpr (p->expr1);
      printf (" * ");
      printExpr (p->expr2);
      printf (")");
      return;
    case SLASH:
      printf ("(");
      printExpr (p->expr1);
      printf (" / ");
      printExpr (p->expr2);
      printf (")");
      return;
    case PERCENT:
      printf ("(");
      printExpr (p->expr1);
      printf (" %% ");
      printExpr (p->expr2);
      printf (")");
      return;
    case LTLT:
      printf ("(");
      printExpr (p->expr1);
      printf (" << ");
      printExpr (p->expr2);
      printf (")");
      return;
    case GTGT:
      printf ("(");
      printExpr (p->expr1);
      printf (" >> ");
      printExpr (p->expr2);
      printf (")");
      return;
    case GTGTGT:
      printf ("(");
      printExpr (p->expr1);
      printf (" >>> ");
      printExpr (p->expr2);
      printf (")");
      return;
    case AMPERSAND:
      printf ("(");
      printExpr (p->expr1);
      printf (" & ");
      printExpr (p->expr2);
      printf (")");
      return;
    case CARET:
      printf ("(");
      printExpr (p->expr1);
      printf (" ^ ");
      printExpr (p->expr2);
      printf (")");
      return;
    case BAR:
      printf ("(");
      printExpr (p->expr1);
      printf (" | ");
      printExpr (p->expr2);
      printf (")");
      return;
    case TILDE:
      printf ("(~ ");
      printExpr (p->expr1);
      printf (")");
      return;
    case PLUS:
      printf ("(");
      if (p->expr2 != NULL) {
        printExpr (p->expr1);
        printf (" + ");
        printExpr (p->expr2);
      } else {
        printf ("+ ");
        printExpr (p->expr1);
      }
      printf (")");
      return;
    case MINUS:
      printf ("(");
      if (p->expr2 != NULL) {
        printExpr (p->expr1);
        printf (" - ");
        printExpr (p->expr2);
      } else {
        printf ("- ");
        printExpr (p->expr1);
      }
      printf (")");
      return;
    default:
      printf ("***** INVALID OP WITHIN Expression  *****");
      break;
  }
}



/* newInstr (op)
**
** This routine allocates a new Instruction object and returns
** a pointer to it.  It initializes its op field.
*/
Instruction * newInstr (int op) {
  Instruction * p;
  p = (Instruction *) calloc (1, sizeof (Instruction));
  p->op = op;
  p->format = EOF;
  p->expr = NULL;
  p->lineNum = currentLine;
  p->ra = 0;
  p->rb = 0;
  p->rc = 0;
  p->next = NULL;
  return p;
}



/* newExpression (op)
**
** This routine allocates a new Expression object and returns
** a pointer to it.  It initializes its op field.
*/
Expression * newExpression (int op) {
  Expression * p;
  p = (Expression *) calloc (1, sizeof (Expression));
  p->op = op;
  p->value = 0;
  p->rvalue = 0.0;
  p->expr1 = NULL;
  p->expr2 = NULL;
  p->tableEntry = NULL;
  return p;
}



/* getOneInstruction ()
**
** This routine picks up an instruction and adds it to the growing list.
** In parses the following syntax:
**
**    [ id ':' ]  [ keyword operands ] EOL
**
** If problems are encountered, it scans forward to the next EOL and
** scans it.  If nothing is present, it will add nothing to the list.
** It leaves nextToken pointing to the token after the EOL.  If nextToken
** is EOF, then it will return immediately.
*/
void getOneInstruction () {
  Instruction * p, * q;
  Expression * ex;
  TableEntry * tableEntry;
  String * strPtr;
  int i, opCode, category, gotLabel, gotMinus;
  if (nextToken == EOF) {
    return;
  }
  gotLabel = 0;
  if (nextToken == ID) {
    tableEntry = tokenValue.tableEntry;
    scan ();
    if (nextToken == COLON) {
      /* Process a label here */
      gotLabel = 1;
      if (currentSegment == 0) {
        printError ("Not in .text, .data, or .bss segment");
      } else {
        if ((tableEntry->relativeTo != NULL) || (tableEntry->offset != 0)) {
          printError ("This symbol is already defined");
        }
        tableEntry->offset = getLC ();
        switch (currentSegment) {
          case TEXT:
            tableEntry->relativeTo = textTableEntry;
            break;
          case DATA:
            tableEntry->relativeTo = dataTableEntry;
            break;
          case BSS_SEG:
            tableEntry->relativeTo = bssTableEntry;
            break;
        }
        p = newInstr (LABEL);
        p->tableEntry = tableEntry;
        addInstrToList (p, 0);
        if (tableEntry == entryTableEntry) {
          if (currentSegment != TEXT) {
            printError ("\"_entry\" is not in the .text segment");
          } else if (textLC != 0) {
            printError ("\"_entry\" not at beginning of .text segment");
          } else {
            sawEntryLabel = 1;
          }
        }
      }
      scan ();
    } else if (nextToken == EQUAL) {
      p = newInstr (EQUAL);
      scan ();
      ex = getExpr ();
      if (ex == NULL) return;
      p->tableEntry = tableEntry;
      p->expr = ex;
      /* Attempt to evaluate the expression and update the symbol table */
      resolveEquate (p, 0);  /* Try to resolve any equates we can. */
      /* Add this instruction to the list */
      addInstrToList (p, 0);
      if (nextIsNot (EOL, "Unexpected tokens after expression")) return;
      return;
    } else {
      printError ("Invalid op-code or missing colon after label");
      scanRestOfLine ();
      return;
    }
  }
  if (nextToken == EOL) {
    scan ();
    return;
  }
  opCode = nextToken;
  p = newInstr (opCode);
  switch (opCode) {
    case TEXT:
      setCurrentSegmentTo (TEXT);
      if (gotLabel) {
        printError ("A label is not allowed on .text");
      }
      scan ();
      addInstrToList (p, 0);
      if (nextIsNot (EOL, ".text takes no operands")) return;
      return;
    case DATA:
      setCurrentSegmentTo (DATA);
      if (gotLabel) {
        printError ("A label is not allowed on .data");
      }
      scan ();
      addInstrToList (p, 0);
      if (nextIsNot (EOL, ".data takes no operands")) return;
      return;
    case BSS_SEG:
      setCurrentSegmentTo (BSS_SEG);
      if (gotLabel) {
        printError ("A label is not allowed on .bss");
      }
      scan ();
      addInstrToList (p, 0);
      if (nextIsNot (EOL, ".bss takes no operands")) return;
      return;
    case ASCII:
      if (notInDataOrText ()) return;
      scan ();
      if (nextToken != STRING) {
        printError ("Expecting string after .ascii");
        scanRestOfLine ();
        return;
      }
      tableEntry = tokenValue.tableEntry;
      p->tableEntry = tableEntry;
      scan ();
      addInstrToList (p, tableEntry->string.length);
      if (nextIsNot (EOL, "Unexpected tokens after string")) return;
      return;
    case BYTE:
      if (notInDataOrText ()) return;
      scan ();
      ex = getExpr ();
      if (ex == NULL) return;
      p->expr = ex;
      addInstrToList (p, 1);
      if (nextIsNot (EOL, "Unexpected tokens after expression")) return;
      return;
    case WORD:
      if (notInDataOrText ()) return;
      scan ();
      ex = getExpr ();
      if (ex == NULL) return;
      p->expr = ex;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected tokens after expression")) return;
      return;
    case DOUBLE:
      if (notInDataOrText ()) return;
      scan ();
      gotMinus = 0;
      if (nextToken == PLUS) {
        scan ();
      } else if (nextToken == MINUS) {
        gotMinus = 1;
        scan ();
      }
      ex = newExpression (REAL);
      if (nextToken == REAL) {
        ex->rvalue = tokenValue.rvalue;
        scan ();
        if (gotMinus) {
          ex->rvalue = - (ex->rvalue);
        }
      } else {
        printError ("Expecting a floating point constant");
      }
      p->expr = ex;
      addInstrToList (p, 8);
      if (nextIsNot (EOL, "Unexpected tokens after floating constant")) return;
      return;
    case EXPORT:
      if (gotLabel) {
        printError ("A label is not allowed on .export");
      }
      scan ();
      if (nextToken != ID) {
        printError ("Expecting symbol after .export");
        scanRestOfLine ();
        return;
      }
      tableEntry = tokenValue.tableEntry;
      tableEntry->exported = 1;
      scan ();
      p->tableEntry = tableEntry;
      addInstrToList (p, 0);
      if (nextIsNot (EOL, "Unexpected tokens after symbol")) return;
      return;
    case IMPORT:
      if (gotLabel) {
        printError ("A label is not allowed on .import");
      }
      scan ();
      if (nextToken != ID) {
        printError ("Expecting symbol after .import");
        scanRestOfLine ();
        return;
      }
      tableEntry = tokenValue.tableEntry;
      tableEntry->imported = 1;
      scan ();
      p->tableEntry = tableEntry;
      addInstrToList (p, 0);
      if (nextIsNot (EOL, "Unexpected tokens after symbol")) return;
      return;
    case ALIGN:
      if (gotLabel) {
        printError ("A label is not allowed on .align");
      }
      scan ();
      i = getLC() % 4;
      if (i != 0) {
        i = 4 - i;
        p->op = SKIP;   /* Turn this instruction into a SKIP */
        p->ra = i;      /* and save number of skipped bytes in p->ra */
        addInstrToList (p, i);
      }
      if (nextIsNot (EOL, ".align takes no operands")) return;
      return;
    case SKIP:
      scan ();
      i = getLC ();  /* To force an error if not in a segment */
      ex = getExpr ();
      if (ex == NULL) return;
      p->expr = ex;
      evalExpr (ex, 1);
/***
      if (nextToken == INTEGER) {
        i = tokenValue.ivalue;
        scan ();
      } else {
        printError ("Unimplemented: for now .skip operand must be integer");
      }
***/
      if (ex->relativeTo == NULL) {
        /* We already got a message during expression evaluation, but... */
        printError (".skip expression may not use symbols defined after it");
        p->ra = 0;
      } else if (ex->relativeTo != absoluteTableEntry) {
        printError ("The .skip expression must evaluate to an absolute value");
        return;
      } else {
        p->ra = ex->value;        /* Save number of skipped bytes in p->ra */
      }
      addInstrToList (p, ex->value);
      if (nextIsNot (EOL, "Unexpected tokens after operands")) return;
      return;
    default:
      break;
  }
  scan ();
  category = categoryOf (opCode);
  if (category < 0) {
    printError ("Invalid or missing op-code");
    scanRestOfLine ();
    return;
  }
  if (notInDataOrText ()) return;
  if ((getLC() % 4) != 0) {
    fprintf (stderr,
             "Warning on line %d: Instruction not on aligned address\n",
             currentLine);
  }
  switch (category) {
    case 1:     /* add, sub, ... */
      if (getRegisterA (p)) return;
      if (nextIsNot (COMMA, "Expecting comma")) return;
      if (isRegister(nextToken) >= 0) {
        /* Ra, Rb, Rc */
        p->format = D;
        if (getRegisterB (p)) return;
        if (nextIsNot (COMMA, "Expecting comma")) return;
        if (getRegisterC (p)) return;
      } else {
        /* Ra,data16,Rc */
        p->format = E;
        p->expr = getExpr ();
        if (p->expr == NULL) return;
        if (nextIsNot (COMMA, "Expecting comma")) return;
        if (getRegisterC (p)) return;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 2:     /* wait, nop, etc. */
      p->format = A;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after op-code")) return;
      return;
    case 3:     /* load, loadb, loadv, loadbv */
      if (nextIsNot (LBRACKET, "Expecting [ after op-code")) return;
      if (getRegisterA (p)) return;
      if (nextToken == RBRACKET) {
        /* [Ra],Rc */
        scan();
        p->format = D;
      } else {
        if (nextIsNot (PLUS, "Expecting + or ]")) return;
        if (isRegister(nextToken) >= 0) {
          /* [Ra+Rb],Rc */
          p->format = D;
          if (getRegisterB (p)) return;
        } else {
          /* [Ra+data16],Rc */
          p->format = E;
          p->expr = getExpr ();
          if (p->expr == NULL) return;
        }
        if (nextIsNot (RBRACKET, "Expecting ]")) return;
      }
      if (nextIsNot (COMMA, "Expecting comma")) return;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 4:    /* store, storeb, storev, storebv */
      if (getRegisterC (p)) return;
      if (nextIsNot (COMMA, "Expecting comma after reg Rc")) return;
      if (nextIsNot (LBRACKET, "Expecting [ after comma")) return;
      if (getRegisterA (p)) return;
      if (nextToken == RBRACKET) {
        /* Rc,[Ra] */
        scan ();
        p->format = D;
      } else {
        if (nextIsNot (PLUS, "Expecting + after reg Ra")) return;
        if (isRegister(nextToken) >= 0) {
          /* Rc,[Ra+Rb] */
          p->format = D;
          if (getRegisterB (p)) return;
        } else {
          /* Rc,[Ra+data16] */
          p->format = E;
          p->expr = getExpr ();
          if (p->expr == NULL) return;
        }
        if (nextIsNot (RBRACKET, "Expecting ]")) return;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 5:     /* sethi, setlo */
      p->format = G;
      p->expr = getExpr ();
      if (p->expr == NULL) return;
      if ((opCode == SETLO) && ((p->expr->value & 0xffff0000) != 0)) {
        fprintf (stderr,
                 "Warning on line %d: In SETLO, the data exceeds 16 bits in length\n",
                 currentLine);
      }
      if ((opCode == SETHI) && ((p->expr->value & 0x0000ffff) != 0)) {
        fprintf (stderr,
                 "Warning on line %d: In SETHI, the data appears to be in the form 0x1234 instead of 0x12340000 as expected\n",
                 currentLine);
      }
      if (nextIsNot (COMMA, "Expecting comma after expression")) return;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 6:     /* call, jmp, bxx */
      if (isRegister(nextToken) >= 0) {
        p->format = C;
        if (getRegisterA (p)) return;
        if (nextToken == EOL) {
          /* Ra */
        } else {
          /* Ra+Rc */
          if (nextIsNot (PLUS, "Expecting Ra or Ra+Rc")) return;
          if (getRegisterC (p)) return;
        }
      } else {
        /* data24 */
        p->format = F;
        p->expr = getExpr ();
        if (p->expr == NULL) return;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 7:    /* push */
      p->format = C;
      if (getRegisterC (p)) return;
      if (nextToken == COMMA) {
        /* Rc,[--Ra] */
        scan ();
        if (nextIsNot (LBRACKET, "Expecting [ in Rc,[--Ra]")) return;
        if (nextIsNot (MINUSMINUS, "Expecting -- in Rc,[--Ra]")) return;
        if (getRegisterA (p)) return;
        if (nextIsNot (RBRACKET, "Expecting ] in Rc,[--Ra]")) return;
      } else {
        p->ra = 15;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Expecting either Rc or Rc,[--Ra]")) return;
      return;
    case 8:    /* pop */
      p->format = C;
      if (nextToken == LBRACKET) {
        /* [Ra++],Rc */
        scan ();
        if (getRegisterA (p)) return;
        if (nextIsNot (PLUSPLUS, "Expecting ++ in [Ra++],Rc")) return;
        if (nextIsNot (RBRACKET, "Expecting ] in [Ra++],Rc")) return;
        if (nextIsNot (COMMA, "Expecting comma in [Ra++],Rc")) return;
      } else {
        p->ra = 15;
      }
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 9:    /* ldptbr, ldptlr */
      p->format = B;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operand Rc")) return;
      return;
    case 10:   /* tset [Ra],Rc */
      p->format = C;
      if (nextIsNot (LBRACKET, "Expecting [ in [Ra],Rc")) return;
      if (getRegisterA (p)) return;
      if (nextIsNot (RBRACKET, "Expecting ] in [Ra],Rc")) return;
      if (nextIsNot (COMMA, "Expecting comma in [Ra],Rc")) return;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after [Ra],Rc")) return;
      return;
    case 11:    /* readu */
      if (getRegisterC (p)) return;
      if (nextIsNot (COMMA, "Expecting comma in Rc,Ra or Rc,[Ra+data16]")) return;
      if (nextToken == LBRACKET) {
        scan ();
        p->format = E;
        if (getRegisterA (p)) return;
        if (nextToken == PLUS) { /* Rc,[Ra+data16] */
          scan();
          p->expr = getExpr ();
          if (p->expr == NULL) return;
          if (nextIsNot (RBRACKET, "Expecting ] in Rc,[Ra+data16]")) return;
        } else { /* Rc,[Ra] */
          p->expr = absoluteZero;
          if (nextIsNot (RBRACKET, "Expecting ] or + after Rc,[Ra...")) return;
        }
      } else {  /* Rc,Ra */
        p->format = C;
        if (getRegisterA (p)) return;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 12:         /* writeu */
      if (nextToken == LBRACKET) {
        p->format = E;
        scan ();
        if (getRegisterA (p)) return;
        if (nextToken == PLUS) {      /* [Ra+data16],Rc */
          scan ();
          p->expr = getExpr ();
          if (p->expr == NULL) return;
          if (nextIsNot (RBRACKET, "Expecting ] in [Ra+data16],Ra")) return;
        } else {                 /* [Ra],Rc */
          p->expr = absoluteZero;
          if (nextIsNot (RBRACKET, "Expecting ] or + after [Ra...")) return;
        }
      } else {       /* Ra,Rc */
        p->format = C;
        if (getRegisterA (p)) return;
      }
      if (nextIsNot (COMMA, "Expecting comma in Ra,Rc or [Ra+data16],Rc")) return;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 13:    /* syscall */
      p->format = G;
      if (isRegister(nextToken) >= 0) { /* Rc */
        if (getRegisterC (p)) return;
        if (nextToken != EOL) { /* Rc+data16 */
          if (nextIsNot (PLUS, "Expecting Rc or Rc+data16")) return;
          p->expr = getExpr ();
          if (p->expr == NULL) return;
        } else {
          p->expr = absoluteZero;
        }
      } else { /* data16 */
        p->expr = getExpr ();
        if (p->expr == NULL) return;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 14:    /* Unused case*/
      printError ("Program Logic Error: Invalid case in switch statement");
      return;
    case 15:    /* set data32,Rc */
      p->format = G;
      p->op = SETHI;     /* Note, we have not shifted the expression, yet */
      p->expr = getExpr ();
      if (p->expr == NULL) return;
      if (nextIsNot (COMMA, "Expecting comma in data32,Rc")) return;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      /* Make a second instruction and add it to the list also */
      q = newInstr (SETLO);
      q->format = G;
      q->rc = p->rc;
      q->expr = p->expr;    /* Share the expression */
      addInstrToList (q, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 16:    /* mov */
      if (isRegister(nextToken) >= 0) { /* Ra,Rc */
        if (getRegisterA (p)) return;
        if (nextIsNot (COMMA, "Expecting comma in Ra,Rc")) return;
        if (getRegisterC (p)) return;
        p->format = D;
        p->op = OR;
      } else {      /* data16,Rc */
        p->expr = getExpr ();
        if (p->expr == NULL) return;
        if (nextIsNot (COMMA, "Expecting comma in data16,Rc")) return;
        if (getRegisterC (p)) return;
        p->format = E;
        p->op = OR;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 17:    /* cmp */
      if (getRegisterA (p)) return;
      if (nextIsNot (COMMA, "Expecting comma in Ra,Rb or Ra,data16")) return;
      if (isRegister(nextToken) >= 0) { /* Ra,Rb */
        if (getRegisterB (p)) return;
        p->format = D;
        p->op = SUB;
      } else {      /* Ra,data16 */
        p->expr = getExpr ();
        if (p->expr == NULL) return;
        p->format = E;
        p->op = SUB;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 18:    /* neg */
      p->op = SUB;
      p->format = D;
      if (getRegisterC (p)) return;
      p->rb = p->rc;
      if (nextToken == COMMA) {
        /* Rb,Rc */
        scan ();
        if (getRegisterC (p)) return;
      } else {
        /* Rc */
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 19:    /* not */
      p->op = XOR;
      p->format = E;
      p->expr = absoluteMinusOne;
      if (getRegisterC (p)) return;
      p->ra = p->rc;
      p->expr = absoluteMinusOne;
      if (nextToken == COMMA) {
        /* Ra,Rc */
        scan ();
        if (getRegisterC (p)) return;
      } else {
        /* Rc */
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 20:    /* clr */
      p->op = OR;
      p->format = D;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 21:    /* btst */
      if (isRegister(nextToken) >= 0) { /* Ra,Rb */
        if (getRegisterA (p)) return;
        if (nextIsNot (COMMA, "Expecting comma in Ra,Rb")) return;
        if (getRegisterB (p)) return;
        p->format = D;
        p->op = AND;
      } else {      /* data16,Rb */
        p->expr = getExpr ();
        if (p->expr == NULL) return;
        if (nextIsNot (COMMA, "Expecting comma in data16,Rb")) return;
        if (getRegisterA (p)) return;
        p->format = E;
        p->op = AND;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 22:    /* bset, bclr, btog */
      if (isRegister(nextToken) >= 0) { /* Rb,Rc */
        if (getRegisterB (p)) return;
        if (nextIsNot (COMMA, "Expecting comma in Rb,Rc")) return;
        if (getRegisterC (p)) return;
        p->ra = p->rc;
        p->format = D;
      } else {      /* data16,Rc */
        p->expr = getExpr ();
        if (p->expr == NULL) return;
        if (nextIsNot (COMMA, "Expecting comma in data16,Rc")) return;
        if (getRegisterC (p)) return;
        p->ra = p->rc;
        p->format = E;
      }
      if (opCode == BSET) {
        p->op = OR;
      } else if (opCode == BCLR) {
        p->op = ANDN;
      } else if (opCode == BTOG) {
        p->op = XOR;
      } else {
        printError ("Program logic error: opcode not in category");
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 23:     /* ldaddr data16,Rc */
      p->format = G;
      p->expr = getExpr ();
      if (p->expr == NULL) return;
      if (nextIsNot (COMMA, "Expecting comma after expression")) return;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 24:    /* ftoi Fa,Rc */
      if (getFRegisterA (p)) return;
      if (nextIsNot (COMMA, "Expecting comma in Fc,Ra")) return;
      p->format = C;
      if (getRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 25:    /* itof Ra,Fc */
      if (getRegisterA (p)) return;
      if (nextIsNot (COMMA, "Expecting comma in Fc,Ra")) return;
      p->format = C;
      if (getFRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 26:    /* fadd Fa,Fb,Fc */
      if (getFRegisterA (p)) return;
      if (nextIsNot (COMMA, "Expecting comma")) return;
      if (getFRegisterB (p)) return;
      if (nextIsNot (COMMA, "Expecting comma")) return;
      if (getFRegisterC (p)) return;
      p->format = D;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 27:    /* fcmp Fa,Fc */
      if (getFRegisterA (p)) return;
      if (nextIsNot (COMMA, "Expecting comma")) return;
      if (getFRegisterC (p)) return;
      p->format = C;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 28:    /* fload ...,Fc */
      if (nextIsNot (LBRACKET, "Expecting [ after op-code")) return;
      if (getRegisterA (p)) return;
      if (nextToken == RBRACKET) {
        /* [Ra],Fc */
        scan();
        p->format = D;
      } else {
        if (nextIsNot (PLUS, "Expecting + or ]")) return;
        if (isRegister(nextToken) >= 0) {
          /* [Ra+Rb],Fc */
          p->format = D;
          if (getRegisterB (p)) return;
        } else {
          /* [Ra+data16],Fc */
          p->format = E;
          p->expr = getExpr ();
          if (p->expr == NULL) return;
        }
        if (nextIsNot (RBRACKET, "Expecting ]")) return;
      }
      if (nextIsNot (COMMA, "Expecting comma")) return;
      if (getFRegisterC (p)) return;
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    case 29:    /* fstore Fc,... */
      if (getFRegisterC (p)) return;
      if (nextIsNot (COMMA, "Expecting comma after reg Fc")) return;
      if (nextIsNot (LBRACKET, "Expecting [ after comma")) return;
      if (getRegisterA (p)) return;
      if (nextToken == RBRACKET) {
        /* Fc,[Ra] */
        scan ();
        p->format = D;
      } else {
        if (nextIsNot (PLUS, "Expecting + after reg Ra")) return;
        if (isRegister(nextToken) >= 0) {
          /* Fc,[Ra+Rb] */
          p->format = D;
          if (getRegisterB (p)) return;
        } else {
          /* Fc,[Ra+data16] */
          p->format = E;
          p->expr = getExpr ();
          if (p->expr == NULL) return;
        }
        if (nextIsNot (RBRACKET, "Expecting ]")) return;
      }
      addInstrToList (p, 4);
      if (nextIsNot (EOL, "Unexpected material after operands")) return;
      return;
    default:
      printError ("Logic error: this instruction is in no category");
      scanRestOfLine ();
      return;
  }
}



/* scanRestOfLine ()
**
** This routine calls scan() repeatedly until we either get an
** EOL or hit EOF.  The tokens are ignored.
*/
void scanRestOfLine () {
  while ((nextToken != EOL) && (nextToken != EOF)) {
    scan ();
  }
  if (nextToken == EOL) {
    scan ();
  }
}



/* categoryOf (tokType)
**
** This routine if this token is a valid op-code (either a BLITZ instruction,
** a synthetic instruction, or a psuedo-op), then this routine returns its
** category, which will tell what sort of operands it may take.  If it
** is not a valid op-code, we return -1.
*/
int categoryOf (int tokType) {
  switch (tokType) {
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case SLL:
    case SRL:
    case SRA:
    case OR:
    case AND:
    case ANDN:
    case XOR:
    case REM:
      return 1;
    case NOP:
    case WAIT:
    case DEBUG:
    case CLEARI:
    case SETI:
    case CLEARP:
    case SETP:
    case CLEARS:
    case RETI:
    case RET:
    case DEBUG2:
      return 2;
    case LOAD:
    case LOADB:
    case LOADV:
    case LOADBV:
      return 3;
    case STORE:
    case STOREB:
    case STOREV:
    case STOREBV:
      return 4;
    case SETHI:
    case SETLO:
      return 5;
    case CALL:
    case JMP:
    case BE:
    case BNE:
    case BL:
    case BLE:
    case BG:
    case BGE:
    case BVS:
    case BVC:
    case BNS:
    case BNC:
    case BSS_OP:
    case BSC:
    case BIS:
    case BIC:
    case BPS:
    case BPC:
      return 6;
    case PUSH:
      return 7;
    case POP:
      return 8;
    case LDPTBR:
    case LDPTLR:
      return 9;
    case TSET:
      return 10;
    case READU:
      return 11;
    case WRITEU:
      return 12;
    case SYSCALL:
      return 13;

  /* There is no category 14 */

  /* Synthetic instructions */
    case SET:
      return 15;
    case MOV:
      return 16;
    case CMP:
      return 17;
    case NEG:
      return 18;
    case NOT:
      return 19;
    case CLR:
      return 20;
    case BTST:
      return 21;
    case BSET:
    case BCLR:
    case BTOG:
      return 22;
    case LDADDR:
      return 23;
    case FTOI:
      return 24;
    case ITOF:
      return 25;
    case FADD:
    case FSUB:
    case FMUL:
    case FDIV:
      return 26;
    case FCMP:
    case FSQRT:
    case FNEG:
    case FABS:
      return 27;
    case FLOAD:
      return 28;
    case FSTORE:
      return 29;
  /* Pseudo-ops */
    case TEXT:
    case DATA:
    case BSS_SEG:
    case ASCII:
    case BYTE:
    case WORD:
    case DOUBLE:
    case EXPORT:
    case IMPORT:
    case ALIGN:
    case SKIP:
      return 999;
    default:
      return -1;
  }
}



/* isRegister (tokType)
**
** This routine returns an integer in 0..15 if this token is a valid
** integer register keyword and -1 if not.
*/
int isRegister (int tokType) {
  switch (tokType) {
    case R0: return 0;
    case R1: return 1;
    case R2: return 2;
    case R3: return 3;
    case R4: return 4;
    case R5: return 5;
    case R6: return 6;
    case R7: return 7;
    case R8: return 8;
    case R9: return 9;
    case R10: return 10;
    case R11: return 11;
    case R12: return 12;
    case R13: return 13;
    case R14: return 14;
    case R15: return 15;

    default: return -1;
  }
}



/* isFRegister (tokType)
**
** This routine returns an integer in 0..15 if this token is a valid
** floating-point register keyword and -1 if not.
*/
int isFRegister (int tokType) {
  switch (tokType) {
    case F0: return 0;
    case F1: return 1;
    case F2: return 2;
    case F3: return 3;
    case F4: return 4;
    case F5: return 5;
    case F6: return 6;
    case F7: return 7;
    case F8: return 8;
    case F9: return 9;
    case F10: return 10;
    case F11: return 11;
    case F12: return 12;
    case F13: return 13;
    case F14: return 14;
    case F15: return 15;

    default: return -1;
  }
}



/* getRegisterA (p)
**
** This routine scans a register and sets p->ra.  If ok, it returns
** FALSE, otherwise is prints a message and returns TRUE.
*/
int getRegisterA (Instruction * p) {
  int r = isRegister (nextToken);
  if (r>=0) {
    scan ();
    p->ra = r;
    return 0;
  } else {
    printError ("Expecting Register Ra");
    scanRestOfLine ();
    return 1;
  }
}



/* getRegisterB (p)
**
** This routine scans a register and sets p->rb.  If ok, it returns
** FALSE, otherwise is prints a message and returns TRUE.
*/
int getRegisterB (Instruction * p) {
  int r = isRegister (nextToken);
  if (r>=0) {
    scan ();
    p->rb = r;
    return 0;
  } else {
    printError ("Expecting Register Rb");
    scanRestOfLine ();
    return 1;
  }
}



/* getRegisterC (p)
**
** This routine scans a register and sets p->rc.  If ok, it returns
** FALSE, otherwise is prints a message and returns TRUE.
*/
int getRegisterC (Instruction * p) {
  int r = isRegister (nextToken);
  if (r>=0) {
    scan ();
    p->rc = r;
    return 0;
  } else {
    printError ("Expecting Register Rc");
    scanRestOfLine ();
    return 1;
  }
}



/* getFRegisterA (p)
**
** This routine scans a register and sets p->ra.  If ok, it returns
** FALSE, otherwise is prints a message and returns TRUE.
*/
int getFRegisterA (Instruction * p) {
  int r = isFRegister (nextToken);
  if (r>=0) {
    scan ();
    p->ra = r;
    return 0;
  } else {
    printError ("Expecting Register Fa");
    scanRestOfLine ();
    return 1;
  }
}



/* getFRegisterB (p)
**
** This routine scans a register and sets p->rb.  If ok, it returns
** FALSE, otherwise is prints a message and returns TRUE.
*/
int getFRegisterB (Instruction * p) {
  int r = isFRegister (nextToken);
  if (r>=0) {
    scan ();
    p->rb = r;
    return 0;
  } else {
    printError ("Expecting Register Fb");
    scanRestOfLine ();
    return 1;
  }
}



/* getFRegisterC (p)
**
** This routine scans a register and sets p->rc.  If ok, it returns
** FALSE, otherwise is prints a message and returns TRUE.
*/
int getFRegisterC (Instruction * p) {
  int r = isFRegister (nextToken);
  if (r>=0) {
    scan ();
    p->rc = r;
    return 0;
  } else {
    printError ("Expecting Register Fc");
    scanRestOfLine ();
    return 1;
  }
}



/* nextIsNot (tokType, message)
**
** This routine checks to make sure the current token matches tokType
** and then scans to the next token.  If everything is ok, it returns
** FALSE.  If there are problems, it prints an error message, calls
** scanRestOfLine () and returns TRUE.
*/
int nextIsNot (int tokType, char * message) {
  if (nextToken == tokType) {
    scan ();
    return 0;
  } else {
    printError (message);
    scanRestOfLine ();
    return 1;
  }
}



/* getExpr ()
**
** This routine parses an expression and returns a pointer to
** an Expression node.  If there are problems, it will print an
** error message, scan all remaining tokens on this line, and return NULL.
** In some cases, it may find and return a legal expression, but may
** may fail to scan the entire expression.  This will happen when there
** is a legal expression following by an error, as in:
**    3 + 4 : 5
*/
Expression * getExpr () {
  Expression * p;
  p = parseExpr0 ();
  if (p == NULL) {
    scanRestOfLine ();
    return NULL;
  }
  return p;
}



/* parseExpr0 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr0 ::= expr1 { "|" expr1 }
** If successful, it returns a pointer to an Expression, else it returns NULL.
*/
Expression * parseExpr0 () {
  Expression * soFar, * another, * new;
  int op;
  soFar = parseExpr1 ();
  if (soFar == NULL) return NULL;
  while (1) {
    op = nextToken;
    if (nextToken == BAR) {
      scan ();
    } else {
      break;
    }
    another = parseExpr1 ();
    if (another == NULL) return NULL;
    new = newExpression (op);
    new->expr1 = soFar;
    new->expr2 = another;
    soFar = new;
  }
  return soFar;
}



/* parseExpr1 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr1 ::= expr2 { "^" expr2 }
** If successful, it returns a pointer to an Expression, else it returns NULL.
*/
Expression * parseExpr1 () {
  Expression * soFar, * another, * new;
  int op;
  soFar = parseExpr2 ();
  if (soFar == NULL) return NULL;
  while (1) {
    op = nextToken;
    if (nextToken == CARET) {
      scan ();
    } else {
      break;
    }
    another = parseExpr2 ();
    if (another == NULL) return NULL;
    new = newExpression (op);
    new->expr1 = soFar;
    new->expr2 = another;
    soFar = new;
  }
  return soFar;
}



/* parseExpr2 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr2 ::= expr3 { "&" expr3 }
** If successful, it returns a pointer to an Expression, else it returns NULL.
*/
Expression * parseExpr2 () {
  Expression * soFar, * another, * new;
  int op;
  soFar = parseExpr3 ();
  if (soFar == NULL) return NULL;
  while (1) {
    op = nextToken;
    if (nextToken == AMPERSAND) {
      scan ();
    } else {
      break;
    }
    another = parseExpr3 ();
    if (another == NULL) return NULL;
    new = newExpression (op);
    new->expr1 = soFar;
    new->expr2 = another;
    soFar = new;
  }
  return soFar;
}



/* parseExpr3 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr3 ::= expr4 { ("<<" | ">>" | ">>>") expr4 }
** If successful, it returns a pointer to an Expression, else it returns NULL.
*/
Expression * parseExpr3 () {
  Expression * soFar, * another, * new;
  int op;
  soFar = parseExpr4 ();
  if (soFar == NULL) return NULL;
  while (1) {
    op = nextToken;
    if (nextToken == LTLT) {
      scan ();
    } else if (nextToken == GTGT) {
      scan ();
    } else if (nextToken == GTGTGT) {
      scan ();
    } else {
      break;
    }
    another = parseExpr4 ();
    if (another == NULL) return NULL;
    new = newExpression (op);
    new->expr1 = soFar;
    new->expr2 = another;
    soFar = new;
  }
  return soFar;
}



/* parseExpr4 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr4 ::= expr5 { ("+" | "-") expr5 }
** If successful, it returns a pointer to an Expression, else it returns NULL.
*/
Expression * parseExpr4 () {
  Expression * soFar, * another, * new;
  int op;
  soFar = parseExpr5 ();
  if (soFar == NULL) return NULL;
  while (1) {
    op = nextToken;
    if (nextToken == PLUS) {
      scan ();
    } else if (nextToken == MINUS) {
      scan ();
    } else {
      break;
    }
    another = parseExpr5 ();
    if (another == NULL) return NULL;
    new = newExpression (op);
    new->expr1 = soFar;
    new->expr2 = another;
    soFar = new;
  }
  return soFar;
}



/* parseExpr5 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr5 ::= expr6 { ("*" | "/" | "%") expr6 }
** If successful, it returns a pointer to an Expression, else it returns NULL.
*/
Expression * parseExpr5 () {
  Expression * soFar, * another, * new;
  int op;
  soFar = parseExpr6 ();
  if (soFar == NULL) return NULL;
  while (1) {
    op = nextToken;
    if (nextToken == STAR) {
      scan ();
    } else if (nextToken == SLASH) {
      scan ();
    } else if (nextToken == PERCENT) {
      scan ();
    } else {
      return soFar;
    }
    another = parseExpr6 ();
    if (another == NULL) return NULL;
    new = newExpression (op);
    new->expr1 = soFar;
    new->expr2 = another;
    soFar = new;
  }
}



/* parseExpr6 ()
**
** This routine parsesaccording to the following CFG rule.
**     expr6 ::= "+" expr6 | "-" expr6 | "~" expr6
**               | ID | INTEGER | STRING | "(" expr0 ")"
** If successful, it returns a pointer to an Expression, else it returns NULL.
** If a STRING is present, it must have exactly 4 characters.  These
** four characters will be used as an INTEGER.
*/
Expression * parseExpr6 () {
  Expression * new, * another;
  if (nextToken == PLUS) {
    scan ();
    another = parseExpr6 ();
    if (another == NULL) return NULL;
    new = newExpression (PLUS);
    new->expr1 = another;
    return new;
  } else if (nextToken == MINUS) {
    scan ();
    another = parseExpr6 (); if (another == NULL) return NULL;
    new = newExpression (MINUS);
    new->expr1 = another;
    return new;
  } else if (nextToken == TILDE) {
    scan ();
    another = parseExpr6 ();
    if (another == NULL) return NULL;
    new = newExpression (TILDE);
    new->expr1 = another;
    return new;
  } else if (nextToken == ID) {
    new = newExpression (ID);
    new->tableEntry = tokenValue.tableEntry;
    scan ();
    return new;
  } else if (nextToken == STRING) {
    new = newExpression (INTEGER);
    new->value = SWAP_BYTES (* ((int *) tokenValue.tableEntry->string.string));
    new->relativeTo = absoluteTableEntry;
    if (tokenValue.tableEntry->string.length != 4) {
      printError ("When strings are used in places expecting an integer, the string must be exactly 4 chars long");
      return NULL;
    }
    scan ();
    return new;
  } else if (nextToken == INTEGER) {
    new = newExpression (INTEGER);
    new->value = tokenValue.ivalue;
    new->relativeTo = absoluteTableEntry;
    scan ();
    return new;
  } else if (nextToken == LPAREN) {
    scan ();
    new = parseExpr0 ();
    if (new == NULL) return NULL;
    if (nextIsNot (RPAREN, "Expecting ')' in expression")) return NULL;
    return new;
  } else if (nextToken == ID) {
    new = newExpression (ID);
    new->tableEntry = tokenValue.tableEntry;
    scan ();
    return new;
  } else {
    printError ("Expecting expression");
    return NULL;
  }
}



/* setCurrentSegmentTo (type);
**
** This routine changes the current segment to TEXT, DATA, or BSS_SEG.
*/
void setCurrentSegmentTo (int type) {
  switch (type) {
    case TEXT:
    case DATA:
    case BSS_SEG:
      currentSegment = type;
      return;
    default:
      printError ("Program logic error: bad arg to setCurrentSegmentTo()");
      return;
  }
}



/* getLC ()
**
** This routine returns the Location Counter for whichever segment is
** current.  It prints an error if there is no current segment.
*/
int getLC () {
  switch (currentSegment) {
    case TEXT:
      return textLC;
    case DATA:
      return dataLC;
    case BSS_SEG:
      return bssLC;
    default:
      printError ("Not currently in a .text, .data, or .bss segment");
      return;
  }
}



/* incrementLCBy (incr)
**
** This routine increases the current Location Counter by incr.  It
** prints an error if there is no current segment.
*/
void incrementLCBy (int incr) {
  if (incr == 0) return;
  switch (currentSegment) {
    case TEXT:
      textLC += incr;
      return;
    case DATA:
      dataLC += incr;
      return;
    case BSS_SEG:
      bssLC += incr;
      return;
    default:
      printError ("Not currently in a .text, .data, or .bss segment");
  }
}



/* notInDataOrText ()
**
** This routine prints an error message if we are not in .text or
** .data and returns TRUE.  It returns FALSE if all is ok.
*/
int notInDataOrText () {
  if (currentSegment == TEXT) return 0;
  if (currentSegment == DATA) return 0;
  printError ("We are not currently in the .text or .data segment");
  scanRestOfLine ();
  return 1;
}



// checkHostCompatibility ()
//
// This routine checks that the host implementation of C++ meets certain
// requirements.
//
// (1) This routine checks that integers are represented using exactly 4
// bytes.
//
// (2) This routine checks that integers are stored in the expected
// Big or Little Endian order.
//
// (3) This routine checks that integer overflow behavior is as expected
// with two's complement arithmetic.
//
// (4) This routine checks that doubles are implemented using 8-bytes in
// the IEEE standard, with the bytes in correct Big/Little Endian order.
//
// (5) This routine checks that the double->int conversion works as
// expected.  If this is not the case, then truncateToInt() will need to
// be changed.
//
void checkHostCompatibility () {
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
  double d;
  char * p, * q;
  int i, i1, i2, i3;

  // Check that ints are in the expected Big/Little Endian order.
  fourBytes.chars[0] = 0x12;
  fourBytes.chars[1] = 0x34;
  fourBytes.chars[2] = 0x56;
  fourBytes.chars[3] = 0x78;
  if (SWAP_BYTES(fourBytes.i) != 0x12345678) {
    fprintf (stderr, "There is a big/little endian byte ordering problem.\n");
    errorExit ();
  }

  // Check that we have at least 4 bytes of precision.
  i = 0x00000001;
  i <<= 20;
  i <<= 10;
  i >>= 20;
  i >>= 10;
  if (i != 0x00000001) {
    fprintf (stderr, "This program only runs on computers with 4 byte integers - 1\n");
    errorExit ();
  }

  // Check that we have no more than 4 bytes of precision.
  i = 0x00000001;
  i <<= 20;
  i <<= 13;   // Some compilers treat <<33 as a nop!
  i >>= 20;
  i >>= 13;
  if (i != 0x00000000) {
    fprintf (stderr, "This program only runs on computers with 4 byte integers - 2\n");
    errorExit ();
  }

  // Check that we have the expected overflow behavior for ints.
  i = -2147483647;
  i = i - 2;
  if (i != 2147483647) {
    fprintf (stderr, "This program only runs on computers with 4 byte integers - 3\n");
    errorExit ();
  }

  // Check that doubles are represented as we expect.
  d = 123.456e37;
  p = (char *) &d;
  q = p;
  // If doubles are stored in Big Endian byte order....
  if ((*p++ == '\x48') &&
      (*p++ == '\x0d') &&
      (*p++ == '\x06') &&
      (*p++ == '\x3c') &&
      (*p++ == '\xdb') &&
      (*p++ == '\x93') &&
      (*p++ == '\x27') &&
      (*p++ == '\xcf')) {
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
    fprintf (stderr, "There is a big/little endian byte ordering problem with doubles - 1.\n");
    errorExit ();
#endif

  // Else, if doubles are stored in Little Endian byte order...
  } else if ((*q++ == '\xcf') &&
             (*q++ == '\x27') &&
             (*q++ == '\x93') &&
             (*q++ == '\xdb') &&
             (*q++ == '\x3c') &&
             (*q++ == '\x06') &&
             (*q++ == '\x0d') &&
             (*q++ == '\x48')) {
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN

#else
    fprintf (stderr, "There is a big/little endian byte ordering problem with doubles - 2.\n");
    errorExit ();
#endif

  // Else, if doubles are stored in some other way...
  } else {
    fprintf (stderr, "The host implementation of 'double' is not what I expect.\n");
    errorExit ();
  }

  // There is variation in the way different hosts handle double->int conversion
  // when the double is too large to represent as an integer.  When checking
  // we must do the conversion in two steps, since some compilers perform the
  // conversion at compile time, and will do the conversion differently than
  // the host machine.  Truly appalling, isn't it!
  //   On PPC,   (int) 9e99 is 0x7fffffff
  //   On PPC,   (int) d    is 0x7fffffff
  //   On Intel, (int) 9e99 is 0x7fffffff
  //   On Intel, (int) d    is 0x80000000
  //
  i = (int) 9e99;
  // printf ("(int) 9e99 is 0x%08x\n", i);
  d = 9e99;
  i = (int) d;  // Note: ((int) 9e99 == 0 while ((int) d) == 2147483647)!!!
  // printf ("(int) d is 0x%08x\n", i);

  // Check that double->int conversion works as expected.
  d = 4.9;
  i1 = (int) d;
  d = -4.9;
  i2 = (int) d;
  d = -9e99;
  i3 = (int) d;
  if ((i1 !=  4) ||
      (i2 != -4) ||
      (i3 != 0x80000000)) {
    printf ("%d %d %d %d\n", i1, i2, i3);
    fprintf (stderr, "The host implementation of double->int casting is not what I expect.\n");
    errorExit ();
  }

}



/* swapBytesInDouble (void *)
**
** This routine is passed a pointer to 8 bytes.  If the host architecture is
** Big Endian, this routine will do nothing.  If we are running on a Little
** Endian machine, this routine will swap the byte ordering.
**
*/
void swapBytesInDouble (void *p) {
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  union eightBytes {
    char chars [8];
    double d;
  } eightBytes;

  eightBytes.chars[0] = *(((char *) p) + 7);
  eightBytes.chars[1] = *(((char *) p) + 6);
  eightBytes.chars[2] = *(((char *) p) + 5);
  eightBytes.chars[3] = *(((char *) p) + 4);
  eightBytes.chars[4] = *(((char *) p) + 3);
  eightBytes.chars[5] = *(((char *) p) + 2);
  eightBytes.chars[6] = *(((char *) p) + 1);
  eightBytes.chars[7] = *(((char *) p) + 0);
  *((double *) p) = eightBytes.d;

#endif
}



/* resolveEquate (instrPtr, printErrors)
**
** This routine is passed a pointer to an EQUAL instruction.  It attempts
** to evaluate the expression.  If it can be evaluated (i.e., resolved),
** then this routine will set "anyEquatesResolved" to true.  If we are
** unable to resolve this equate, it will set "unresolvedEquates" to
** TRUE.  When an equate is resolved, we will update the entry in the
** symbol table.  If "printErrors" is true, we will print error messages
** when we have problems, otherwise error messages will be suppressed.
*/
void resolveEquate (Instruction * instrPtr, int printErrors) {
  Expression * expr;
  TableEntry * tableEntry;
  if (instrPtr->op != EQUAL) return;
  expr = instrPtr->expr;
  tableEntry = instrPtr->tableEntry;
                    /***
                    printString (tableEntry);
                    printf (" = ");
                    printExpr (expr);
                    printf ("...\n");
                    ***/
  if (tableEntry->relativeTo != NULL) {
                    /***
                    printf ("              Previously resolved\n");
                    ***/
  } else {
    evalExpr (expr, printErrors);
    if (expr->relativeTo != NULL) {
                    /***
                    printf ("              value=%d  relativeTo ", expr->value);
                    printString (expr->relativeTo);
                    printf ("\n");
                    ***/
      anyEquatesResolved = 1;
      tableEntry->relativeTo = expr->relativeTo;     
      tableEntry->offset = expr->value;     
    } else {
                    /***
                    printf ("              No value could be computed\n");
                    ***/
      unresolvedEquates = 1;
    }
  }
}



/* evalExpr (exprPtr)
**
** This routine is passed a pointer to an Expression.  It walks the tree
** and evaluates the expression.  It fills in the "value" and "relativeTo"
** fields.  When "relativeTo" is NULL, it indicates that the Expression
** has not yet been evalutated.  If anything goes wrong it will print
** an error message if printErrors is TRUE; if printErrors is FALSE it
** will not print error messages.  This routine cannot handle a NULL
** argument.  (Such NULL expression pointers can arise from syntax errors.)
** This routine assumes that the expression it is passed is well formed.
*/
void evalExpr (Expression * expr, int printErrors) {
  unsigned a, b, c;
  int i, j;
  /*
    printf ("evalExpr called on...");
    printExpr (expr);
    printf ("\n");
  */
  /* If this expression has already been given a value, we are done. */
  if (expr->relativeTo != NULL) {
    return;
  }
  switch (expr->op) {
    case ID:
      if (expr->tableEntry->relativeTo != NULL) {
        expr->relativeTo = expr->tableEntry->relativeTo;
        expr->value = expr->tableEntry->offset;
      } else if (expr->tableEntry->imported != 0) { 
        expr->relativeTo = expr->tableEntry;
        expr->value = 0;
      } else {
        if (printErrors) {
          printError2 ("Undefined symbol: ", expr->tableEntry->string.string);
        }
      }
      return;
    case INTEGER:
      if (expr->relativeTo != absoluteTableEntry) {
        printError ("Program logic error: Integer in expression is not absolute");
      }
      return;
    case REAL:
      return;
    case PLUS:
      evalExpr (expr->expr1, printErrors);
      if (expr->expr2 == NULL) {  /* if unary operator */
        /* unary + is a nop */
        expr->value = expr->expr1->value;
        expr->relativeTo = expr->expr1->relativeTo;
        return;
      }
      /* We have a binary + operator. */
      evalExpr (expr->expr2, printErrors);
      /* If either operand is unresolved... */
      if ((expr->expr1->relativeTo == NULL) ||
          (expr->expr2->relativeTo == NULL)) {
        return;
      /* If both operands are absolute... */
      } else if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value + expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
      /* If the first operand is relative and the second is absolute... */
      } else if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value + expr->expr2->value;
        expr->relativeTo = expr->expr1->relativeTo;

      /* If the second operand is relative and the first is absolute... */
      } else if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo != absoluteTableEntry)) {
        expr->value = expr->expr1->value + expr->expr2->value;
        expr->relativeTo = expr->expr2->relativeTo;
      /* Else if both are relative... */
      } else {
        if (printErrors) {
          printError ("Both operands to binary + may not be relative");
        }
      }
      return;
    case MINUS:
      evalExpr (expr->expr1, printErrors);
      if (expr->expr2 == NULL) {  /* if unary operator */
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          if (printErrors) {
            printError ("The unary - operator requires operand to be an absolute value");
          }
          return;
        }
        expr->value = - expr->expr1->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      /* We have a binary - operator. */
      evalExpr (expr->expr2, printErrors);
      /* If either operand is unresolved... */
      if ((expr->expr1->relativeTo == NULL) ||
          (expr->expr2->relativeTo == NULL)) {
        return;
      /* If both operands are absolute... */
      } else if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value - expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
      /* If both operands are relative to the same symbol... */
      } else if (expr->expr1->relativeTo ==
                 expr->expr2->relativeTo) {
        expr->value = expr->expr1->value - expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
      /* If the first operand is relative and the second is absolute... */
      } else if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value - expr->expr2->value;
        expr->relativeTo = expr->expr1->relativeTo;
      /* Else second is relative but not to first, or first is absolute... */
      } else {
        if (printErrors) {
          printError ("Operands to binary - are relative to different symbols");
        }
      }
      return;
    case STAR:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value * expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The * operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The * operator requires operands to be absolute values");
        }
      }
      return;
    case SLASH:
      /* The / and % operators are implemented using the "C" /  and %
         operators.  When both are positive, the division is straightforward:
         the fractional part is discared.  For example, 7 / 2 will be 3.  When
         one operand is negative the result is system dependent.  With
         ANSI C, we are only guaranteed that
              (a / b) * b + a % b   ==   a
         will hold.  Because of all this, we simply disallow neg operands. */
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        if ((expr->expr1->value < 0) || (expr->expr2->value <= 0)) {
          if (printErrors) {
            printError ("Operands to / must be positive");
          }
          return;
        }
        expr->value = expr->expr1->value / expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The / operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The / operator requires operands to be absolute values");
        }
      }
      return;
    case PERCENT:
      /* The % operator is implemented using the "C" % operator, which
         is system dependent...  Ugh.  When one operand is negative the
         value and sign may vary across "C" implementations. */
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        if ((expr->expr1->value < 0) || (expr->expr2->value <= 0)) {
          if (printErrors) {
            printError ("Operands to % must be positive");
          }
          return;
        }
        expr->value = expr->expr1->value % expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The % operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The % operator requires operands to be absolute values");
        }
      }
      return;
    case LTLT:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        i = expr->expr2->value;
        if (i<0 || i>31) {
          if (printErrors) {
            printError ("Shift amount must be within 0..31");
          }
          return;
        }
        expr->value = expr->expr1->value << expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The << operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The << operator requires operands to be absolute values");
        }
      }
      return;
    case GTGT:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        i = expr->expr2->value;
        if (i<0 || i>31) {
          if (printErrors) {
            printError ("Shift amount must be within 0..31");
          }
          return;
        }
        a = expr->expr1->value;
        b = expr->expr2->value;
        c = a >> b;  /* With unsigned ints; zeros will be shifted in. */
        expr->value = c;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The >> operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The >> operator requires operands to be absolute values");
        }
      }
      return;
    case GTGTGT:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        i = expr->expr1->value;
        j = expr->expr2->value;
        if (j<0 || j>31) {
          if (printErrors) {
            printError ("Shift amount must be within 0..31");
          }
          return;
        }
        if (j > 32) j = 32;
        for (; j > 0; j--) {
          if (i >= 0) {
            i = (i >> 1) & 0x7fffffff;  /* shift in a 0 */
          } else {
            i = (i >> 1) | 0x80000000;  /* shift in a 1 */
          }
        }
        expr->value = i;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The >>> operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The >>> operator requires operands to be absolute values");
        }
      }
      return;
    case AMPERSAND:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value & expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The & operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The & operator requires operands to be absolute values");
        }
      }
      return;
    case BAR:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value | expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The | operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The | operator requires operands to be absolute values");
        }
      }
      return;
    case CARET:
      evalExpr (expr->expr1, printErrors);
      evalExpr (expr->expr2, printErrors);
      if ((expr->expr1->relativeTo == absoluteTableEntry) &&
          (expr->expr2->relativeTo == absoluteTableEntry)) {
        expr->value = expr->expr1->value ^ expr->expr2->value;
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The ^ operator requires operands to be absolute values");
        }
        if ((expr->expr2->relativeTo != absoluteTableEntry) &&
            (expr->expr2->relativeTo != NULL)) {
          printError ("The ^ operator requires operands to be absolute values");
        }
      }
      return;
    case TILDE:
      evalExpr (expr->expr1, printErrors);
      if (expr->expr1->relativeTo == absoluteTableEntry) {
        expr->value = ~ ( expr->expr1->value );
        expr->relativeTo = absoluteTableEntry;
        return;
      }
      if (printErrors) {
        if ((expr->expr1->relativeTo != absoluteTableEntry) &&
          (expr->expr1->relativeTo != NULL)) {
          printError ("The ~ operator requires its operand to be an absolute value");
        }
      }
      return;
    default:
      printError ("Program logic error: Unknown operator type in expression");
      return;
  }
}



/* processEquates ()
**
** This routine runs over the list of instructions looking for "equates".
** For each one, it attempts to evaluate the expression and assign a
** value to the symbol.  It keeps repeating until it can not do any more.
** Then it runs though them one last time, printing error messages.
*/
void processEquates () {
  Instruction * instrPtr;
  anyEquatesResolved = 1;
  while (anyEquatesResolved && unresolvedEquates) {
    anyEquatesResolved = 0;
    /* Run thru the instruction list */
    for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
      if (instrPtr->op == EQUAL) {
        resolveEquate (instrPtr, 0);  /* Try to resolve any equates we can. */
      }
    }
  }
  /* Run through the equates one last time, printing errors. */
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    if (instrPtr->op == EQUAL) {
      currentLine = instrPtr->lineNum;
      resolveEquate (instrPtr, 1);  /* Try to resolve any equates we can. */
    }
  }
}



/* passTwo ()
**
** Run through the instruction list.  Produce the listing, if one is
** desired.
*/
void passTwo () {
  Instruction * instrPtr;
  int dontPrintLine;
  int i, in;
  int * ip;
  char c0, c1, c2, c3;
  int nextLineFromInput = 1;
  currentSegment = 0;
  textLC = 0;
  dataLC = 0;
  bssLC = 0;
  setCurrentSegmentTo (TEXT);
  if (inputFile != stdin) {
    rewind (inputFile);
  }
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    dontPrintLine = 0;
    currentLine = instrPtr->lineNum;
    if (commandOptionL) {
      while (nextLineFromInput < currentLine) {
        printf ("                ");
        printOneLine ();
        nextLineFromInput++;
      }
    }
    /* process this instr */
    switch (instrPtr->op) {
      case TEXT:
      case DATA:
      case BSS_SEG:
        setCurrentSegmentTo (instrPtr->op);
        if (commandOptionL) {
          printf ("%06x          ", getLC ());
        }
        break;
      case ASCII:
        i = instrPtr->tableEntry->string.length;
        if (commandOptionL) {
          printf ("%06x ", getLC ());
          if (i<=0) {
            printf ("         ");
          } else if (i==1) {
            c0 = instrPtr->tableEntry->string.string[0];
            printf ("%02x       ", c0);
          } else if (i==2) {
            c0 = instrPtr->tableEntry->string.string[0];
            c1 = instrPtr->tableEntry->string.string[1];
            printf ("%02x%02x     ", c0, c1);
          } else if (i==3) {
            c0 = instrPtr->tableEntry->string.string[0];
            c1 = instrPtr->tableEntry->string.string[1];
            c2 = instrPtr->tableEntry->string.string[2];
            printf ("%02x%02x%02x   ", c0, c1, c2);
          } else {
            c0 = instrPtr->tableEntry->string.string[0];
            c1 = instrPtr->tableEntry->string.string[1];
            c2 = instrPtr->tableEntry->string.string[2];
            c3 = instrPtr->tableEntry->string.string[3];
            printf ("%02x%02x%02x%02x ", c0, c1, c2, c3);
          }
        }
        incrementLCBy (i);
        break;
      case BYTE:
        if (commandOptionL) {
          i = instrPtr->expr->value;
          i = i & 0x000000ff;
          printf ("%06x ", getLC ());
          printf ("%02x       ", i);
        }
        incrementLCBy (1);
        break;
      case WORD:
        if (commandOptionL) {
          printf ("%06x ", getLC ());
          printf ("%08x ", instrPtr->expr->value);
        }
        incrementLCBy (4);
        break;
      case DOUBLE:
        if (commandOptionL) {
          printf ("%06x ", getLC ());
          /* Print the first 4 bytes of the 8 byte floating number. */
          ip = (int *) & (instrPtr->expr->rvalue);
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
          printf ("%08x ", *(++ip));
#else
          printf ("%08x ", *ip);
#endif
          /* printf ("  rvalue = %.17g  ", instrPtr->expr->rvalue); */
        }
        incrementLCBy (8);
        break;
      case EQUAL:
        if (commandOptionL) {
          printf ("       %08x ", instrPtr->expr->value);
        }
        break;
      case EXPORT:
        if (commandOptionL) {
          printf ("                ");
        }
        break;
      case IMPORT:
        if (commandOptionL) {
          printf ("                ");
        }
        break;
      case ALIGN:
        printError ("Program logic error: Should have replaced align with skip");
        break;
      case SKIP:
        i = instrPtr->ra;
        if (commandOptionL) {
          printf ("%06x ", getLC ());
          if (i<=0) {
            printf ("         ");
          } else if (i==1) {
            printf ("00       ");
          } else if (i==2) {
            printf ("0000     ");
          } else if (i==3) {
            printf ("000000   ");
          } else {
            printf ("00000000 ");
          }
        }
        incrementLCBy (i);
        break;
      case LABEL:
        if (commandOptionL) {
          if ((instrPtr->next != NULL)
              && (instrPtr->next->lineNum != instrPtr->lineNum)) {
            printf ("%06x          ", getLC ());
          } else {
            dontPrintLine = 1;
          }
        }
        break;
      default:
        if ((instrPtr->op < 0) || (instrPtr->op > 255)) {
          printError ("Program logic error: Invalid opcode in passTwo");
        }
        in = (instrPtr->op << 24)
           | (instrPtr->rc << 20)
           | (instrPtr->ra << 16)
           | (instrPtr->rb << 12);
        if (instrPtr->op == SETHI) {
           in = in | ((instrPtr->expr->value >> 16) & 0x0000ffff);
        } else if (instrPtr->op == SETLO) {
           in = in | (instrPtr->expr->value & 0x0000ffff);
        } else if (instrPtr->op == LDADDR) {
           in = in | 0;
        } else if ((instrPtr->format == E) || (instrPtr->format == G)) {
           i = instrPtr->expr->value;
           if (((i & 0xffff8000) != 0xffff8000) &&
               ((i & 0xffff8000) != 0x00000000) &&
               (instrPtr->expr->relativeTo == absoluteTableEntry)) {
             fprintf (stderr,
                 "Warning on line %d: Immediate value (0x%08x) exceeds 16-bit limit.\n",
                 currentLine, i);
           }
           in = in | (i & 0x0000ffff);
        } else if (instrPtr->format == F) {
           if ((instrPtr->expr->relativeTo == textTableEntry)
                  && (currentSegment == TEXT)) {
             i = instrPtr->expr->value - getLC ();
           } else if ((instrPtr->expr->relativeTo == dataTableEntry)
                  && (currentSegment == DATA)) {
             i = instrPtr->expr->value - getLC ();
           } else {
             i = 0;
           }
           if (((i & 0xff800000) != 0xff800000) &&
               ((i & 0xff800000) != 0x00000000)) {
             fprintf (stderr,
                 "Warning on line %d: Relative branch offset (%08x) exceeds 24-bit limit.\n",
                 currentLine, i);
           }
           in = in | (i & 0x00ffffff);
        }
        if (commandOptionL) {
          printf ("%06x ", getLC ());
          printf ("%08x ", in);
        }
        incrementLCBy (4);
        break;
    }
    /* print the listing info, if a listing is needed. */
    if (commandOptionL && !dontPrintLine) {
      if (nextLineFromInput == currentLine) {
        printOneLine ();
        nextLineFromInput++;
      } else {
        printf ("\n");
      }
    }
  }
  if (commandOptionL) {
    while (1) {
      c0 = getc (inputFile);
      if (c0 == EOF) break;
      printf ("                ");
      while (c0 != EOF) {
        if (c0 == '\r') {
          c0 = '\n';
        }
        putc (c0, stdout);
        if (c0 == '\n') break;
        c0 = getc (inputFile);
      }
    }
  }
}



/* printOneLine ()
**
** This routine will read one line from inputFile and copy it to
** stdout, including the terminating newline.
*/
void printOneLine () {
  int c;
  while (1) {
    c = getc (inputFile);
    if (c == EOF) return;
    if (c == '\r') {
      c = '\n';
    }
    putc (c, stdout);
    if (c == '\n') return;
  }
}



/* writeInteger (i)
**
** This routine writes out 4 bytes to the .o file.
*/
void writeInteger (int i) {
  int j = SWAP_BYTES(i);
  fwrite (&j, 4, 1, outputFile);
}



/* writeSegment (segType)
**
** This routine is passed either TEXT or DATA.  It runs through the
** instruction list and writes the segment data out to the .o file.
** It returns the number of bytes written.
*/
int writeSegment (int segType) {
  Instruction * instrPtr;
  int in, i, len;
  double d;
  char * str;
  char c;
  int zero = 0;
  int numberBytesWritten = 0;
  currentSegment = 0;
  textLC = 0;
  dataLC = 0;
  bssLC = 0;
  setCurrentSegmentTo (TEXT);
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    /* process this instr */
    switch (instrPtr->op) {
      case TEXT:
      case DATA:
      case BSS_SEG:
        currentSegment = instrPtr->op;
        break;
      case ASCII:
        len = instrPtr->tableEntry->string.length;
        if (currentSegment == segType) {
          str = instrPtr->tableEntry->string.string;
          for (i=0; i<len; i++) {
            fwrite (&str[i], 1, 1, outputFile);
          }
          numberBytesWritten += len;
        }
        incrementLCBy (len);
        break;
      case BYTE:
        if (currentSegment == segType) {
          i = instrPtr->expr->value;
          if (instrPtr->expr->relativeTo != absoluteTableEntry) {
            i = 0;
          }
          c = i & 0x000000ff;
          fwrite (&c, 1, 1, outputFile);
          numberBytesWritten++;
        }
        incrementLCBy (1);
        break;
      case WORD:
        if (currentSegment == segType) {
          i = instrPtr->expr->value;
          if (instrPtr->expr->relativeTo != absoluteTableEntry) {
            i = 0;
          }
          writeInteger (i);
          numberBytesWritten += 4;
        }
        incrementLCBy (4);
        break;
      case DOUBLE:
        if (currentSegment == segType) {
          d = instrPtr->expr->rvalue;
          swapBytesInDouble (&d);
          fwrite (&d, 1, 8, outputFile);
          numberBytesWritten += 8;
        }
        incrementLCBy (8);
        break;
      case EQUAL:
        break;
      case EXPORT:
        break;
      case IMPORT:
        break;
      case SKIP:
        len = instrPtr->ra;
        if (currentSegment == segType) {
          for (i=0; i<len; i++) {
            fwrite (&zero, 1, 1, outputFile);
          }
          numberBytesWritten += len;
        }
        incrementLCBy (len);
        break;
      case LABEL:
        break;
      default:
        if ((instrPtr->op < 0) || (instrPtr->op > 255)) {
          printError ("Program logic error: Invalid opcode in writeSeg");
        }
        if (currentSegment == segType) {
          in = (instrPtr->op << 24)
             | (instrPtr->rc << 20)
             | (instrPtr->ra << 16)
             | (instrPtr->rb << 12);
          if (instrPtr->op == SETHI) {
            i = instrPtr->expr->value;
            if (instrPtr->expr->relativeTo == absoluteTableEntry) {
              in = in | ((i>>16) & 0x0000ffff);
            }
          } else if (instrPtr->op == LDADDR) {
            /* Set 16 bits of instruction to zero, which they already are. */
          } else if ((instrPtr->format == E) || (instrPtr->format == G)) {
            i = instrPtr->expr->value;
            if (instrPtr->expr->relativeTo == absoluteTableEntry) {
              in = in | (i & 0x0000ffff);
            }
          } else if (instrPtr->format == F) {
            if ((instrPtr->expr->relativeTo == textTableEntry)
                  && (currentSegment == TEXT)) {
              i = instrPtr->expr->value - getLC ();
            } else if ((instrPtr->expr->relativeTo == dataTableEntry)
                  && (currentSegment == DATA)) {
              i = instrPtr->expr->value - getLC ();
            } else {
              i = 0;
            }
            in = in | (i & 0x00ffffff);
          }
          writeInteger (in);
          numberBytesWritten += 4;
        }
        incrementLCBy (4);
        break;
    }
  }
  return numberBytesWritten;
}



/* writeSymbols ()
**
** This routine writes out imported and exported symbols to the .o file.
*/
void writeSymbols () {
  int nextSymbolNum = 5;
  int hashVal;
  TableEntry * entryPtr;
  /* Run thru all symbols and assign a number to every imported or
     exported symbol. */
  for (hashVal = 0; hashVal<SYMBOL_TABLE_HASH_SIZE; hashVal++) {
    for (entryPtr = symbolTableIndex [hashVal];
                       entryPtr;
                       entryPtr = entryPtr->next) {
      if ((entryPtr == textTableEntry)
          || (entryPtr == dataTableEntry)
          || (entryPtr == bssTableEntry)
          || (entryPtr == absoluteTableEntry)) {
        /* ignore */
      } else if (entryPtr->imported || entryPtr->exported) {
        entryPtr->symbolNum = nextSymbolNum++;
      }
    }
  }
  /* Run thru all symbols and add each to the .o file. */
  writeOutSymbol (textTableEntry);
  writeOutSymbol (dataTableEntry);
  writeOutSymbol (bssTableEntry);
  writeOutSymbol (absoluteTableEntry);
  for (hashVal = 0; hashVal<SYMBOL_TABLE_HASH_SIZE; hashVal++) {
    for (entryPtr = symbolTableIndex [hashVal];
                       entryPtr;
                       entryPtr = entryPtr->next) {
      if ((entryPtr == textTableEntry)
          || (entryPtr == dataTableEntry)
          || (entryPtr == bssTableEntry)
          || (entryPtr == absoluteTableEntry)) {
        /* ignore */
      } else if (entryPtr->imported || entryPtr->exported) {
        writeOutSymbol (entryPtr);
      }
    }
  }
}



/* writeOutSymbol ()
**
** This routine adds a symbol to the .o file.
*/
void writeOutSymbol (TableEntry * entry) {
  int i, len;
  char * str;
  writeInteger (entry->symbolNum);
  writeInteger (entry->offset);
  if (entry->relativeTo != NULL) {
    writeInteger (entry->relativeTo->symbolNum);
  } else {
    writeInteger (0);
  }
  len = entry->string.length;
  writeInteger (len);
  str = entry->string.string;
  for (i=0; i<len; i++) {
    fwrite (&str[i], 1, 1, outputFile);
  }
}



/* writeRelocationInfo ()
**
** This routine runs through the instruction list and for each instruction
** that contains relocatable data, writes out a record saying how the
** linker should modify the data.
*/
void writeRelocationInfo () {
  Instruction * instrPtr;
  int i, in, relTo;
  currentSegment = 0;
  textLC = 0;
  dataLC = 0;
  bssLC = 0;
  setCurrentSegmentTo (TEXT);
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    currentLine = instrPtr->lineNum;
    switch (instrPtr->op) {
      case TEXT:
      case DATA:
      case BSS_SEG:
        setCurrentSegmentTo (instrPtr->op);
        break;
      case ASCII:
        i = instrPtr->tableEntry->string.length;
        incrementLCBy (i);
        break;
      case BYTE:
        writeRelocationEntry (1, getLC(), instrPtr->expr);
/***
        if (instrPtr->expr->relativeTo != absoluteTableEntry) {
          writeInteger (1);
          writeInteger (getLC ());
          if (currentSegment == TEXT) writeInteger (1);
          if (currentSegment == DATA) writeInteger (2);
          writeInteger (instrPtr->expr->value);
          relTo = instrPtr->expr->relativeTo->symbolNum;
          if (relTo == 0) {
            printError ("Program logic error: relativeTo has no symbolNum");
          }
          writeInteger (relTo);
        }
***/
        incrementLCBy (1);
        break;
      case WORD:
        writeRelocationEntry (4, getLC(), instrPtr->expr);
/***
        if (instrPtr->expr->relativeTo != absoluteTableEntry) {
          writeInteger (4);
          writeInteger (getLC ());
          if (currentSegment == TEXT) writeInteger (1);
          if (currentSegment == DATA) writeInteger (2);
          writeInteger (instrPtr->expr->value);
          relTo = instrPtr->expr->relativeTo->symbolNum;
          if (relTo == 0) {
            printError ("Program logic error: relativeTo has no symbolNum");
          }
          writeInteger (relTo);
        }
***/
        incrementLCBy (4);
        break;
      case DOUBLE:
        incrementLCBy (8);
        break;
      case EQUAL:
      case EXPORT:
      case IMPORT:
      case ALIGN:
      case LABEL:
        break;
      case SKIP:
        i = instrPtr->ra;
        incrementLCBy (i);
        break;
      default:
        if ((instrPtr->op < 0) || (instrPtr->op > 255)) {
          printError ("Program logic error: Invalid opcode in passTwo");
        }
        if (instrPtr->op == SETHI) {
          /* 16-bit SETHI update.  Use loc=addr of the 16-bits. */
          writeRelocationEntry (5, getLC () + 2, instrPtr->expr);
        } else if (instrPtr->op == SETLO) {
          /* 16-bit SETLO update.  Use loc=addr of the 16-bits. */
          writeRelocationEntry (6, getLC () + 2, instrPtr->expr);
        } else if (instrPtr->op == LDADDR) {
          /* 16-bit relative.  Use loc=addr of the 16-bits. */
          writeRelocationEntry (7, getLC (), instrPtr->expr);
        } else if ((instrPtr->format == E) || (instrPtr->format == G)) {
          /* 16-bit update.  Use loc=addr of the 16-bits. */
          writeRelocationEntry (2, getLC () + 2, instrPtr->expr);
        } else if (instrPtr->format == F) {
          /* 24-bit update.  Use loc=addr of the instruction. */
          if ((instrPtr->expr->relativeTo == textTableEntry)
                && (currentSegment == TEXT)) {
          } else if ((instrPtr->expr->relativeTo == dataTableEntry)
                && (currentSegment == DATA)) {
          } else {
            writeRelocationEntry (3, getLC () + 0, instrPtr->expr);
          }
        }
        incrementLCBy (4);
        break;
    }
  }
}



/* writeRelocationEntry (type, addr, expr)
**
** This routine writes a single relocation entry to the .o file.
*/
void writeRelocationEntry (int type, int addr, Expression * expr) {
  
  if (expr->relativeTo == NULL) {
    printError ("Program logic error: relativeTo has no symbolNum");
  } else if (expr->relativeTo != absoluteTableEntry) {
    writeInteger (type);
    writeInteger (addr);
    if (currentSegment == TEXT) writeInteger (1);
    if (currentSegment == DATA) writeInteger (2);
    writeInteger (expr->value);
    if (expr->relativeTo->symbolNum == 0) {
      printError ("Program logic error: relativeTo has no symbolNum");
    }
    writeInteger (expr->relativeTo->symbolNum);
    writeInteger (currentLine);
  }
}



/* writeLabels ()
**
** This routine runs through the instruction list and writes
** out each label to the .o output file.
*/
void writeLabels () {
  Instruction * instrPtr;
  TableEntry * relTo;
  int offset, i, len;
  char * str;
  currentSegment = 0;
  for (instrPtr=instrList; instrPtr!=NULL; instrPtr=instrPtr->next) {
    /* process this instr */
    switch (instrPtr->op) {
      case TEXT:
      case DATA:
      case BSS_SEG:
        currentSegment = instrPtr->op;
        break;
      case LABEL:
        relTo = instrPtr->tableEntry->relativeTo;
        if (currentSegment == TEXT) {
          if (relTo != textTableEntry) {
            printError ("Program logic error: problem with label");
          }
          writeInteger (1);
        } else if (currentSegment == DATA) {
          if (relTo != dataTableEntry) {
            printError ("Program logic error: problem with label");
          }
          writeInteger (2);
        } else if (currentSegment == BSS_SEG) {
          if (relTo != bssTableEntry) {
            printError ("Program logic error: problem with label");
          }
          writeInteger (3);
        }
        offset = instrPtr->tableEntry->offset;
        writeInteger (offset);
        len = instrPtr->tableEntry->string.length;
        writeInteger (len);
        str = instrPtr->tableEntry->string.string;
        for (i=0; i<len; i++) {
          fwrite (&str[i], 1, 1, outputFile);
        }
        break;
      default:
        break;
    }
  }
}
