// ir.cc  --  Methods for IR classes
//
// KPL Compiler
//
// Copyright 2002-2007, Harry H. Porter III
//
// This file may be freely copied, modified and compiled, on the sole
// condition that if you modify it...
//   (1) Your name and the date of modification is added to this comment
//       under "Modifications by", and
//   (2) Your name and the date of modification is added to the printHelp()
//       routine in file "main.cc" under "Modifications by".
//
// Original Author:
//    05/24/03 - Harry H. Porter III
//
// Modifcations by:
//   03/15/06 - Harry H. Porter III
//
//

#include "main.h"



// within16Bits (int) --> bool
//
// This routine returns true if the argument can be represented as a 2's
// complement signed, 16-bit integer, and can therefore be put into the
// immediate field of instructions like:
//        mul     r1,xxxxx,r2
//        mov     xxxxx, r3
//        load    [r14+xxxxx],r5
//
int within16Bits (int i) {
  if ((i <= 32767) && (i >= -32768)) {
    return 1;
  } else {
    return 0;
  }
}



// getIntoReg4 (src, reg)
//
// This routine generates the code necessary to get the "src" into "reg".
// It moves the 4 bytes of data into "reg".
//
// "src" should be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//      INT_CONST
//
void getIntoReg4 (AstNode * src, char * reg) {
  int i;
  if ((src->op == LOCAL) || (src->op == PARAMETER)) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+%d],%s\n", i, reg);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE1: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg);
      fprintf (outputFile, "\tload\t[r14+%s],%s\n", reg, reg);
    }
  } else if (src->op == CLASS_FIELD) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg);
      fprintf (outputFile, "\tload\t[%s+%d],%s\n", reg, i, reg);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE2: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tload\t[r14+8],r11\n");
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg);
      fprintf (outputFile, "\tload\t[r11+%s],%s\n", reg, reg);
    }
  } else if (src->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", ((VarDecl *) src)->id->chars, reg);
    fprintf (outputFile, "\tload\t[%s],%s\n", reg, reg);
  } else if (src->op == INT_CONST) {
    i = ((IntConst *) src)->ivalue;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tmov\t%d,%s\n", i, reg);
    } else if (i == 0x80000000) {
      fprintf (outputFile, "\tset\t0x80000000,%s\n", reg);
    } else {
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg);
    }
  } else {
    printf ("\nsrc->op = %s\n", symbolName (src->op));
    programLogicError ("Unknown node in getIntoReg4");
  }
}



// getIntoReg1 (src, reg)
//
// This routine generates the code necessary to get the "src" into "reg".
// It moves the 1 byte of data into the LSB of "reg1" and clears the hi-order
// 24 bits.
//
// "src" should be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//      CHAR_CONST
//      BOOL_CONST
//
void getIntoReg1 (AstNode * src, char * reg) {
  int i;
  if ((src->op == LOCAL) || (src->op == PARAMETER)) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tloadb\t[r14+%d],%s\n", i, reg);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE3: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg);
      fprintf (outputFile, "\tloadb\t[r14+%s],%s\n", reg, reg);
    }
  } else if (src->op == CLASS_FIELD) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg);
      fprintf (outputFile, "\tloadb\t[%s+%d],%s\n", reg, i, reg);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE4: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tload\t[r14+8],r11\n");
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg);
      fprintf (outputFile, "\tloadb\t[r11+%s],%s\n", reg, reg);
    }
  } else if (src->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", ((VarDecl *) src)->id->chars, reg);
    fprintf (outputFile, "\tloadb\t[%s],%s\n", reg, reg);
  } else if (src->op == CHAR_CONST) {
    i = ((IntConst *) src)->ivalue;
    fprintf (outputFile, "\tmov\t%d,%s\n", i, reg);
  } else if (src->op == BOOL_CONST) {
    i = ((BoolConst *) src)->ivalue;
    fprintf (outputFile, "\tmov\t%d,%s\n", i, reg);
  } else {
    printf ("\nsrc->op = %s\n", symbolName (src->op));
    programLogicError ("Unknown node in getIntoReg1");
  }
}



// getIntoReg8 (src, freg1, reg2)
//
// This routine generates the code necessary to get the "src" into "freg1", using
// "reg2" if necessary.  It moves the 8 byte of data into the floating reg "freg1".
//
// "src" should be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//      DOUBLE_CONST
//
void getIntoReg8 (AstNode * src, char * freg1, char * reg2) {
  int i;
  DoubleConst * doubleConst;
  if ((src->op == LOCAL) || (src->op == PARAMETER)) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tfload\t[r14+%d],%s\n", i, freg1);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE5: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tfload\t[r14+%s],%s\n", reg2, freg1);
    }
  } else if (src->op == CLASS_FIELD) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg2);
      fprintf (outputFile, "\tfload\t[%s+%d],%s\n", reg2, i, freg1);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE6: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tload\t[r14+8],r11\n");
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tfload\t[r11+%s],%s\n", reg2, freg1);
    }
  } else if (src->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", ((VarDecl *) src)->id->chars, reg2);
    fprintf (outputFile, "\tfload\t[%s],%s\n", reg2, freg1);
  } else if (src->op == DOUBLE_CONST) {
    doubleConst = (DoubleConst *) src;
    fprintf (outputFile, "\tset\t%s,%s\n", doubleConst->nameOfConstant, reg2);
    fprintf (outputFile, "\tfload\t[%s],%s\n", reg2, freg1);
  } else {
    printf ("\nsrc->op = %s\n", symbolName (src->op));
    programLogicError ("Unknown node in getIntoReg8");
  }
}



// getAddrOfVarIntoReg (src, reg1)
//
// This routine generates the code necessary to get the address of "src" into
// "reg1".  The "src" may be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//
void getAddrOfVarIntoReg (AstNode * src, char * reg1) {
  int i;
  if ((src->op == LOCAL) || (src->op == PARAMETER)) {
    i = ((VarDecl *) src)->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tadd\tr14,%d,%s\n", i, reg1);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE7: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg1);
      fprintf (outputFile, "\tadd\tr14,%s,%s\n", reg1, reg1);
    }
  } else if (src->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", ((VarDecl *) src)->id->chars, reg1);
  } else if (src->op == CLASS_FIELD) {
    i = ((VarDecl *) src)->offset;
    // printf ("ClassField = %s   offset = %d\n", ((VarDecl *) src)->id->chars, i);
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg1);
      fprintf (outputFile, "\tadd\t%s,%d,%s\n", reg1, i, reg1);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE8: %s, offset = %d\n", ((VarDecl *) src)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,r11\n", i);
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg1);
      fprintf (outputFile, "\tadd\t%s,r11,%s\n", reg1, reg1);
    }
  } else {
    printf ("\nsrc->op = %s\n", symbolName (src->op));
    programLogicError ("Unknown node in getAddrOfVarIntoReg");
  }
}



// storeFromReg4 (dest, reg1, reg2)
//
// This routine generates the code necessary to move a word from "reg1" to
// the "dest" location, using "reg2" if necessary.
//
// "dest" should be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//
void storeFromReg4 (VarDecl * dest, char * reg1, char * reg2) {
  int i;
  if ((dest->op == LOCAL) || (dest->op == PARAMETER)) {
    i = dest->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tstore\t%s,[r14+%d]\n", reg1, i);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE9: %s, offset = %d\n", ((VarDecl *) dest)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tstore\t%s,[r14+%s]\n", reg1, reg2);
    }
  } else if (dest->op == CLASS_FIELD) {
    i = dest->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg2);
      fprintf (outputFile, "\tstore\t%s,[%s+%d]\n", reg1, reg2, i);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE10: %s, offset = %d\n", ((VarDecl *) dest)->id->chars, i);
      fprintf (outputFile, "\tload\t[r14+8],r11\n");
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tstore\t%s,[r11+%s]\n", reg1, reg2);
    }
  } else if (dest->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", dest->id->chars, reg2);
    fprintf (outputFile, "\tstore\t%s,[%s]\n", reg1, reg2);
  } else {
    printf ("\ndest->op = %s\n", symbolName (dest->op));
    programLogicError ("Unknown node in storeFromReg4");
  }
}



// storeFromReg1 (dest, reg1, reg2)
//
// This routine generates the code necessary to move 1 byte from "reg1" to
// the "dest" location, using "reg2" if necessary.
//
// "dest" should be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//
void storeFromReg1 (VarDecl * dest, char * reg1, char * reg2) {
  int i;
  if ((dest->op == LOCAL) || (dest->op == PARAMETER)) {
    i = dest->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tstoreb\t%s,[r14+%d]\n", reg1, i);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE11: %s, offset = %d\n", ((VarDecl *) dest)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tstoreb\t%s,[r14+%s]\n", reg1, reg2);
    }
  } else if (dest->op == CLASS_FIELD) {
    i = dest->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg2);
      fprintf (outputFile, "\tstoreb\t%s,[%s+%d]\n", reg1, reg2, i);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE12: %s, offset = %d\n", ((VarDecl *) dest)->id->chars, i);
      fprintf (outputFile, "\tload\t[r14+8],r11\n");
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tstoreb\t%s,[r11+%s]\n", reg1, reg2);
    }
  } else if (dest->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", dest->id->chars, reg2);
    fprintf (outputFile, "\tstoreb\t%s,[%s]\n", reg1, reg2);
  } else {
    printf ("\ndest->op = %s\n", symbolName (dest->op));
    programLogicError ("Unknown node in storeFromReg1");
  }
}



// storeFromReg8 (dest, reg1, reg2)
//
// This routine generates the code necessary to move 8 bytes from "freg1" to
// the "dest" location, using "reg2" if necessary.
//
// "dest" should be:
//      LOCAL
//      PARAMETER
//      GLOBAL
//      CLASS_FIELD
//
void storeFromReg8 (VarDecl * dest, char * freg1, char * reg2) {
  int i;
  if ((dest->op == LOCAL) || (dest->op == PARAMETER)) {
    i = dest->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tfstore\t%s,[r14+%d]\n", freg1, i);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE13: %s, offset = %d\n", ((VarDecl *) dest)->id->chars, i);
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tfstore\t%s,[r14+%s]\n", freg1, reg2);
    }
  } else if (dest->op == CLASS_FIELD) {
    i = dest->offset;
    if (within16Bits (i)) {
      fprintf (outputFile, "\tload\t[r14+8],%s\n", reg2);
      fprintf (outputFile, "\tfstore\t%s,[%s+%d]\n", freg1, reg2, i);
    } else {
      // printf ("16-BIT OVERFLOW IN VARIABLE14: %s, offset = %d\n", ((VarDecl *) dest)->id->chars, i);
      fprintf (outputFile, "\tload\t[r14+8],r11\n");
      fprintf (outputFile, "\tset\t%d,%s\n", i, reg2);
      fprintf (outputFile, "\tfstore\t%s,[r11+%s]\n", freg1, reg2);
    }
  } else if (dest->op == GLOBAL) {
    fprintf (outputFile, "\tset\t%s,%s\n", dest->id->chars, reg2);
    fprintf (outputFile, "\tfstore\t%s,[%s]\n", freg1, reg2);
  } else {
    printf ("\ndest->op = %s\n", symbolName (dest->op));
    programLogicError ("Unknown node in storeFromReg8");
  }
}



// overflowTest ()
//
// This routine generates:
//       bvs   _runtimeErrorOverflow
//
void overflowTest () {
    fprintf (outputFile, "\tbvs\t_runtimeErrorOverflow\n");
}



//
// printIR
//
// This routine runs through the IR statements and prints them.
//
void printIR () {
    IR * inst;
    for (inst = firstInstruction; inst; inst = inst->next) {
      inst->print ();
      // fflush (outputFile);      // Useful during debugging...
    }
}



//
// printANode (node)
//
// This routine is passed a pointer to:
//    Local
//    Global
//    Parameter
//    ClassField
//    IntConst
//    DoubleConst
// It prints the node into the output file.
//
void printANode (AstNode * node) {
    double r, rval;
    r = 0.0;

  if (node==NULL) {
    fprintf (outputFile, "*****  NULL  *****");
    // For debugging, keep going.  Later, perhaps we should just abort...
    //     programLogicError ("printANode called with node == NULL");
  }

  switch (node->op) {

    case INT_CONST:
      fprintf (outputFile, "%d", ((IntConst *) node)->ivalue);
      return;

    case DOUBLE_CONST:
      rval = ((DoubleConst *) node)->rvalue;
      if (rval == (-1.0) / r) {
        fprintf (outputFile, "<NegativeInfinity>");
      } else if (rval == (+1.0) / r) {
        fprintf (outputFile, "<PositiveInfinity>");
      } else if (isnan (rval)) {
        fprintf (outputFile, "<Not-A-Number>");
      } else if (rval == 0.0 && 1.0/rval < 0.0) {
        fprintf (outputFile, "<NegativeZero>");
      } else {
        fprintf (outputFile, "%.16gD", rval);
      }
      return;

    case CHAR_CONST:
      fprintf (outputFile, "%d", ((CharConst *) node)->ivalue);
      return;

    case BOOL_CONST:
      fprintf (outputFile, "%d", ((BoolConst *) node)->ivalue);
      return;

    case GLOBAL:
      printString (outputFile, ((Global *) node)->id);
      return;

    case LOCAL:
      printString (outputFile, ((Local *) node)->id);
      return;

    case PARAMETER:
      printString (outputFile, ((Parameter *) node)->id);
      return;

    case CLASS_FIELD:
      printString (outputFile, ((ClassField *) node)->id);
      return;

    default:
      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in printANode");

  }

  programLogicError ("All cases should have returned in printANode");
  return;

}



//----------  IR  ----------

void IR::print () {
  programLogicError ("Method 'print' should have been overridden (IR)");
}

void linkIR (IR * inst) {
  // if (inst->next != NULL) {
  //   programLogicError ("Problems in linkIR (1)");
  // }
  if (firstInstruction == NULL) {
    firstInstruction = inst;
    // if (lastInstruction != NULL) {
    //   programLogicError ("Problems in linkIR (2)");
    // }
  }
  if (lastInstruction != NULL) {
    lastInstruction->next = inst;
  }
  lastInstruction = inst;
}



//----------  Comment  ----------

void Comment::print () {
  fprintf (outputFile, "! %s\n", str);
}

void IRComment (char * str) {
  linkIR (new Comment (str));
}



//----------  Comment3  ----------

void Comment3::print () {
  fprintf (outputFile, "! %s%d...\n", str, ivalue);
}

void IRComment3 (char * str, int i) {
  linkIR (new Comment3 (str, i));
}



//----------  Goto  ----------
void Goto::print () {
  Label * lab;
  // If we happen to be jumping to the very next IR instruction...
  if (next && next->op == OPLabel) {
    lab = (Label *) this->next;
    if (lab->label == label) {
      fprintf (outputFile, "!", label);   // Comment the jump out
    }
  }
  // Else generate a "jmp" instruction...
  fprintf (outputFile, "\tjmp\t%s\n", label);
}

void IRGoto (char * label) {
  linkIR (new Goto (label));
}



//----------  Goto2  ----------
void Goto2::print () {
  fprintf (outputFile, "\tjmp\t%s\t! %d:\t%s\n", label, offset, selector);
}

void IRGoto2 (char * label, int offset, char * selector) {
  linkIR (new Goto2 (label, offset, selector));
}



//----------  Label  ----------

void Label::print () {
  fprintf (outputFile, "%s:\n", label);
}

void IRLabel (char * label) {
  linkIR (new Label (label));
}



//----------  Import  ----------

void Import::print () {
  fprintf (outputFile, "\t.import\t%s\n", name);
}

void IRImport (char * nam) {
  linkIR (new Import (nam));
}



//----------  Export  ----------

void Export::print () {
  fprintf (outputFile, "\t.export\t%s\n", name);
}

void IRExport (char * nam) {
  linkIR (new Export (nam));
}



//----------  Data  ----------

void Data::print () {
  fprintf (outputFile, "\t.data\n");
}

void IRData () {
  linkIR (new Data ());
}



//----------  Text  ----------

void Text::print () {
  fprintf (outputFile, "\t.text\n");
}

void IRText () {
  linkIR (new Text ());
}



//----------  Align  ----------

void Align::print () {
  fprintf (outputFile, "\t.align\n");
}

void IRAlign () {
  linkIR (new Align ());
}



//----------  Skip  ----------

void Skip::print () {
  fprintf (outputFile, "\t.skip\t%d\n", byteCount);
}

void IRSkip (int i) {
  linkIR (new Skip (i));
}



//----------  Byte  ----------

void Byte::print () {
  fprintf (outputFile, "\t.byte\t%d", byteValue);
  if (byteValue >= 32 && byteValue <= 126) {
    fprintf (outputFile, "\t\t\t! '");
    printChar (outputFile, byteValue);
    fprintf (outputFile, "'\n");
  } else if (byteValue <= 9) {
    fprintf (outputFile, "\n");
  } else {
    int i = intToHexChar ((byteValue >>4) & 0x0000000f);
    int j = intToHexChar (byteValue & 0x0000000f);
    fprintf (outputFile, "\t\t\t! (%c%c in hex)\n", i, j);
  }
}

void IRByte (int i) {
  linkIR (new Byte (i));
}



//----------  Word  ----------

void Word::print () {
  if (wordValue == 0) {
    fprintf (outputFile, "\t.word\t0\n");
  } else {
    fprintf (outputFile, "\t.word\t0x%08x\t\t! decimal value = %d\n",
                         wordValue, wordValue);
  }
}

void IRWord (int i) {
  linkIR (new Word (i));
}



//----------  Word2  ----------

void Word2::print () {
  fprintf (outputFile, "\t.word\t%s\n", symbol);
}

void IRWord2 (char * s) {
  linkIR (new Word2 (s));
}



//----------  Word3  ----------

void Word3::print () {
  fprintf (outputFile, "\t.word\t%d\t\t! %s\n", wordValue, comment);
}

void IRWord3 (int i, char * s) {
  linkIR (new Word3 (i, s));
}



//----------  LoadAddr  ----------

void LoadAddr::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = %s\n", stringName);
  fprintf (outputFile, "\tset\t%s,r1\n", stringName);
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRLoadAddr (AstNode * d, char * s) {
  linkIR (new LoadAddr (d, s));
}



//----------  LoadAddrIndirect  ----------

void LoadAddrIndirect::print () {
  fprintf (outputFile, "!   *");
  printANode (dest);
  fprintf (outputFile, " = %s\n", stringName);
  fprintf (outputFile, "\tset\t%s,r1\n", stringName);
  getIntoReg4 (dest, "r2");
  fprintf (outputFile, "\tstore\tr1,[r2]\n");


}

void IRLoadAddrIndirect (AstNode * d, char * s) {
  linkIR (new LoadAddrIndirect (d, s));
}



//----------  LoadAddr2  ----------

void LoadAddr2::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = &");
  printANode (src);
  fprintf (outputFile, "\n");
  getAddrOfVarIntoReg (src, "r1");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRLoadAddr2 (VarDecl * d, AstNode * s) {
  linkIR (new LoadAddr2 (d, s));
}



//----------  LoadAddrWithIncr  ----------

void LoadAddrWithIncr::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (src);
  fprintf (outputFile, ".%s\n", fieldInit->id->chars);
  getAddrOfVarIntoReg (src, "r1");
  if (within16Bits (fieldInit->offset)) {
    fprintf (outputFile, "\tadd\tr1,%d,r1\n", fieldInit->offset);
  } else {
    fprintf (outputFile, "\tset\t0x%08x,r1\n", fieldInit->offset);
    fprintf (outputFile, "\tadd\tr1,r2,r1\n");
  }
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRLoadAddrWithIncr (VarDecl * dest, AstNode * src, FieldInit * fieldInit) {
  linkIR (new LoadAddrWithIncr (dest, src, fieldInit));
}



//----------  LoadAddrWithIncr2  ----------

void LoadAddrWithIncr2::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = (*");
  printANode (src);
  fprintf (outputFile, ") . %s\n", fieldInit->id->chars);
  getIntoReg4 (src, "r1");
  if (within16Bits (fieldInit->offset)) {
    fprintf (outputFile, "\tadd\tr1,%d,r1\n", fieldInit->offset);
  } else {
    fprintf (outputFile, "\tset\t0x%08x,r1\n", fieldInit->offset);
    fprintf (outputFile, "\tadd\tr1,r2,r1\n");
  }
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRLoadAddrWithIncr2 (VarDecl * d, AstNode * s, FieldInit * f) {
  linkIR (new LoadAddrWithIncr2 (d, s, f));
}



//----------  Double  ----------

void Double::print () {
  int * p1, * p2;
  double r = 0.0;
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  p2 = (int *) &doubleValue;
  p1 = p2+1;
#else
  p1 = (int *) &doubleValue;
  p2 = p1+1;
#endif
  fprintf (outputFile, "\t.word\t0x%08x\t\t! double value = ", *p1);
  if (doubleValue == (-1.0) / r) {
    fprintf (outputFile, "NegInf\n");
  } else if (doubleValue == (+1.0) / r) {
    fprintf (outputFile, "PosInf\n");
  } else if (isnan (doubleValue)) {
    fprintf (outputFile, "NaN\n");
  } else if (doubleValue == 0.0 && 1.0/doubleValue < 0.0) {
    fprintf (outputFile, "NegZero\n");
  } else {
    fprintf (outputFile, "%.15g\n", doubleValue);
  }
  fprintf (outputFile, "\t.word\t0x%08x\t\t! .\n", *p2);
}

void IRDouble (double r) {
  linkIR (new Double (r));
}



//----------  Call  ----------

void Call::print () {
  fprintf (outputFile, "\tcall\t%s\n", name);
}

void IRCall (char * n) {
  linkIR (new Call (n));
}



//----------  CallIndirect  ----------

void CallIndirect::print () {
  getIntoReg4 (varDesc, "r1");
  fprintf (outputFile, "\tcmp\tr1,0\n");
  fprintf (outputFile, "\tbe\t_runtimeErrorNullPointerDuringCall\n");
  fprintf (outputFile, "\tcall\tr1\n");
}

void IRCallIndirect (VarDecl * vd) {
  linkIR (new CallIndirect (vd));
}



//----------  Debug  ----------

void Debug::print () {
  fprintf (outputFile, "\tdebug\n");
}

void IRDebug () {
  linkIR (new Debug ());
}



//----------  Halt  ----------

void Halt::print () {
  fprintf (outputFile, "\thalt\n");
}

void IRHalt () {
  linkIR (new Halt ());
}



//----------  SetLineNumber  ----------

void SetLineNumber::print () {
  if (within16Bits (lineNumber)) {
    fprintf (outputFile, "\tmov\t%d,r13\t\t! source line %d\n",
             lineNumber, lineNumber);
  } else {
    fprintf (outputFile, "\tset\t%d,r13\t\t! source line %d\n",
             lineNumber, lineNumber);
  }
  fprintf (outputFile, "\tmov\t\"\\0\\0%s\",r10\n", stmtCode);

}

void IRSetLineNumber (int i, char * s) {
  linkIR (new SetLineNumber (i, s));
}



//----------  FunctionEntry  ----------

void FunctionEntry::print () {
  int i;
  char * label;

  fprintf (outputFile, "\tpush\tr14\n");
  fprintf (outputFile, "\tmov\tr15,r14\n");
  fprintf (outputFile, "\tpush\tr13\n");
  fprintf (outputFile, "\tset\t%s,r1\n",
           appendStrings ("_RoutineDescriptor_",fun->newName,""));
  fprintf (outputFile, "\tpush\tr1\n");
  if (fun->frameSize % 4 != 0) {
    programLogicError ("fun->frameSize should be a multiple of 4");
  }

  // Generate code to zero-out the var & arg portion of this frame...
  i = fun->frameSize / 4;
  if (i > 0) {
    label = newLabel ();
    if (within16Bits (i)) {
      fprintf (outputFile, "\tmov\t%d,r1\n", i);
    } else {
      // printf ("16-bit OVERFLOW IN FUNCTION: %s,  i = %d\n", fun->id->chars, i);
      fprintf (outputFile, "\tset\t%d,r1\n", i);
    }
    fprintf (outputFile, "%s:\n", label);
    fprintf (outputFile, "\tpush\tr0\n");
    fprintf (outputFile, "\tsub\tr1,1,r1\n");
    fprintf (outputFile, "\tbne\t%s\n", label);
  }

}

void IRFunctionEntry (Function * fun) {
  linkIR (new FunctionEntry (fun));
}



//----------  FunctionReturn  ----------

void FunctionReturn::print () {
  int i = fun->frameSize+4;
  if (within16Bits (i)) {
    fprintf (outputFile, "\tadd\tr15,%d,r15\n", i);
  } else {
    // printf ("16-bit OVERFLOW IN FUNCTION RETURN: %s   i=%d\n", fun->id->chars, i);
    fprintf (outputFile, "\tset\t%d,r1\n", i);
    fprintf (outputFile, "\tadd\tr15,r1,r15\n");
  }
  fprintf (outputFile, "\tpop\tr13\n");
  fprintf (outputFile, "\tpop\tr14\n");
  fprintf (outputFile, "\tret\n");
}

void IRFunctionReturn (Function * fun) {
  linkIR (new FunctionReturn (fun));
}



//----------  MethodEntry  ----------

void MethodEntry::print () {
  int i;
  char * label;

  fprintf (outputFile, "\tpush\tr14\n");
  fprintf (outputFile, "\tmov\tr15,r14\n");
  fprintf (outputFile, "\tpush\tr13\n");
  fprintf (outputFile, "\tset\t%s,r1\n",
           appendStrings ("_RoutineDescriptor_",meth->newName,""));
  fprintf (outputFile, "\tpush\tr1\n");
  if (meth->frameSize % 4 != 0) {
    programLogicError ("meth->frameSize should be a multiple of 4");
  }

  // Generate code to zero-out the var & arg portion of this frame...
  i = meth->frameSize / 4;
  if (i > 0) {
    label = newLabel ();
    if (within16Bits (i)) {
      fprintf (outputFile, "\tmov\t%d,r1\n", i);
    } else {
      // printf ("16-bit OVERFLOW IN METHOD: %s,  i = %d\n", meth->selector->chars, i);
      fprintf (outputFile, "\tset\t%d,r1\n", i);
    }
    fprintf (outputFile, "%s:\n", label);
    fprintf (outputFile, "\tpush\tr0\n");
    fprintf (outputFile, "\tsub\tr1,1,r1\n");
    fprintf (outputFile, "\tbne\t%s\n", label);
  }

}

void IRMethodEntry (Method * meth) {
  linkIR (new MethodEntry (meth));
}



//----------  MethodReturn  ----------

void MethodReturn::print () {
  int i = meth->frameSize+4;
  if (within16Bits (i)) {
    fprintf (outputFile, "\tadd\tr15,%d,r15\n", i);
  } else {
    // printf ("16-bit OVERFLOW IN METHOD RETURN: %s   i=%d\n", meth->selector->chars, i);
    fprintf (outputFile, "\tset\t%d,r1\n", i);
    fprintf (outputFile, "\tadd\tr15,r1,r15\n");
  }
  fprintf (outputFile, "\tpop\tr13\n");
  fprintf (outputFile, "\tpop\tr14\n");
  fprintf (outputFile, "\tret\n");
}

void IRMethodReturn (Method * meth) {
  linkIR (new MethodReturn (meth));
}



//----------  CheckVersion  ----------

void CheckVersion::print () {
  fprintf (outputFile, "!\n");
  fprintf (outputFile, "! CheckVersion\n");
  fprintf (outputFile, "!\n");
  fprintf (outputFile, "!     This routine is passed:\n");
  fprintf (outputFile, "!       r2 = ptr to the name of the 'using' package\n");
  fprintf (outputFile, "!       r3 = the expected hashVal for 'used' package (myPackage)\n");
  fprintf (outputFile, "!     It prints an error message if the expected hashVal is not correct\n");
  fprintf (outputFile, "!     It then checks all the packages that 'myPackage' uses.\n");
  fprintf (outputFile, "!\n");
  fprintf (outputFile, "!     This routine returns:\n");
  fprintf (outputFile, "!       r1:  0=No problems, 1=Problems\n");
  fprintf (outputFile, "!\n");
  fprintf (outputFile, "!     Registers modified: r1-r4\n");
  fprintf (outputFile, "!\n");

  fprintf (outputFile, "_CheckVersion%s:\n", mySaniName);
  fprintf (outputFile, "\t.export\t_CheckVersion%s\n", mySaniName);

//  fprintf (outputFile, "\tset\t_CVMess7,r1\n");
//  fprintf (outputFile, "\tcall\t_putString\n");

//  fprintf (outputFile, "\tset\t_packageName,r1\t\t! print 'Checking this package name'\n");
//  fprintf (outputFile, "\tcall\t_putString\n");

//  fprintf (outputFile, "\tset\t_CVMess8,r1\n");
//  fprintf (outputFile, "\tcall\t_putString\n");
//  fprintf (outputFile, "\tcall\t_flush\n");

  fprintf (outputFile, "\tset\t0x%08x,r4\t\t! myHashVal = %d\n", myHashVal, myHashVal);
  fprintf (outputFile, "\tcmp\tr3,r4\n");
  fprintf (outputFile, "\tbe\t%s\n", label1);

  fprintf (outputFile, "\tset\t_CVMess1,r1\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tmov\tr2,r1\t\t\t! print using package\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_CVMess2,r1\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_packageName,r1\t\t! print myPackage\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_CVMess3,r1\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_packageName,r1\t\t! print myPackage\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_CVMess4,r1\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tmov\tr2,r1\t\t\t! print using package\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_CVMess5,r1\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_packageName,r1\t\t! print myPackage\n");
  fprintf (outputFile, "\tcall\t_putString\n");

  fprintf (outputFile, "\tset\t_CVMess6,r1\n");
  fprintf (outputFile, "\tcall\t_putString\n");
  // fprintf (outputFile, "\tcall\t_flush\n");

  fprintf (outputFile, "\tmov\t1,r1\n");
  fprintf (outputFile, "\tret\t\n");

  fprintf (outputFile, "%s:\n", label1);
  fprintf (outputFile, "\tmov\t0,r1\n");
}

void IRCheckVersion (char * mySaniName, int myHashVal, char * label1) {
  linkIR (new CheckVersion (mySaniName, myHashVal, label1));
}



//----------  CallCheckVersion  ----------

void CallCheckVersion::print () {

  fprintf (outputFile, "! Make sure %s has hash value 0x%08x (decimal %d)\n",
                        theirSaniName, theirHashVal, theirHashVal);
  fprintf (outputFile, "\tset\t_packageName,r2\n");
  fprintf (outputFile, "\tset\t0x%08x,r3\n", theirHashVal);
  fprintf (outputFile, "\tcall\t_CheckVersion%s\n", theirSaniName);
  fprintf (outputFile, "\t.import\t_CheckVersion%s\n", theirSaniName);
  fprintf (outputFile, "\tcmp\tr1,0\n");
  fprintf (outputFile, "\tbne\t%s\n", label2);
}

void IRCallCheckVersion (char * theirSaniName, int theirHashVal, char * label2) {
  linkIR (new CallCheckVersion (theirSaniName, theirHashVal, label2));
}



//----------  EndCheckVersion  ----------

void EndCheckVersion::print () {
  fprintf (outputFile, "%s:\n", label2);
  fprintf (outputFile, "\tret\n");
  fprintf (outputFile, "_CVMess1:\t.ascii\t\"\\nVERSION CONSISTENCY ERROR: Package '\\0\"\n");
  fprintf (outputFile, "_CVMess2:\t.ascii\t\"' uses package '\\0\"\n");
  fprintf (outputFile, "_CVMess3:\t.ascii\t\"'.  Whenever a header file is modified, all packages that use that package (directly or indirectly) must be recompiled.  The header file for '\\0\"\n");
  fprintf (outputFile, "_CVMess4:\t.ascii\t\"' has been changed since '\\0\"\n");
  fprintf (outputFile, "_CVMess5:\t.ascii\t\"' was compiled last.  Please recompile all packages that depend on '\\0\"\n");
  fprintf (outputFile, "_CVMess6:\t.ascii\t\"'.\\n\\n\\0\"\n");
//  fprintf (outputFile, "_CVMess7:\t.ascii\t\"Checking \\0\"\n");
//  fprintf (outputFile, "_CVMess8:\t.ascii\t\"...\\n\\0\"\n");
  fprintf (outputFile, "\t.align\n");
}

void IREndCheckVersion (char * label2) {
  linkIR (new EndCheckVersion (label2));
}



//----------  StartCheckVersion  ----------

void StartCheckVersion::print () {

  fprintf (outputFile, "\tset\t_packageName,r2\t\t! Get CheckVersion started\n");
  fprintf (outputFile, "\tset\t0x%08x,r3\t\t! .  hashVal = %d\n",
                              myHashVal, myHashVal);
  fprintf (outputFile, "\tcall\t_CheckVersion%s\t! .\n", mySaniName);
  fprintf (outputFile, "\tcmp\tr1,0\t\t\t! .\n");
  fprintf (outputFile, "\tbe\t%s\t\t! .\n", continueLab);
  fprintf (outputFile, "\tret\t\t\t\t! .\n");
  fprintf (outputFile, "%s:\t\t\t\t! .\n", continueLab);
}

void IRStartCheckVersion (char * mySaniName, int myHashVal, char * continueLab) {
  linkIR (new StartCheckVersion (mySaniName, myHashVal, continueLab));
}



//----------  VarDesc1  ----------

void VarDesc1::print () {
  fprintf (outputFile, "\t.word\t%s\n", label);
  fprintf (outputFile, "\t.word\t%d\n", varDecl->offset);
  fprintf (outputFile, "\t.word\t%d\n", sizeInBytes);
}

void IRVarDesc1 (char * lab, VarDecl * vd, int sz) {
  linkIR (new VarDesc1 (lab, vd, sz));
}



//----------  VarDesc2  ----------

void VarDesc2::print () {
  fprintf (outputFile, "%s:\n", label);
  fprintf (outputFile, "\t.byte\t'%c'\n", kind);
  fprintf (outputFile, "\t.ascii\t\"%s\\0\"\n", name);
  fprintf (outputFile, "\t.align\n");
}

void IRVarDesc2 (char * lab, char k, char * n) {
  linkIR (new VarDesc2 (lab, k, n));
}



//----------  FrameSize  ----------

void FrameSize::print () {
  int i;
  if (funOrMeth->op == FUNCTION) {
    i = ((Function *) funOrMeth)->frameSize;
  } else if (funOrMeth->op == METHOD) {
    i = ((Method *) funOrMeth)->frameSize;
  } else {
    programLogicError ("Expecting fun or meth in IRFrameSize");
  }
  fprintf (outputFile, "\t.word\t%d\t\t! frame size = %d\n", i, i);
}

void IRFrameSize (AstNode * funOrMeth) {
  linkIR (new FrameSize (funOrMeth));
}



//----------  Assign1  ----------

void Assign1::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (src);
  fprintf (outputFile, "\t\t(1 byte)\n");
  getIntoReg1 (src, "r1");
  storeFromReg1 ((VarDecl *) dest, "r1", "r2");
}

void IRAssign1 (AstNode * d, AstNode * s) {
  linkIR (new Assign1 (d, s));
}



//----------  Assign4  ----------

void Assign4::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (src);
  fprintf (outputFile, "\t\t(4 bytes)\n");
  getIntoReg4 (src, "r1");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRAssign4 (AstNode * d, AstNode * s) {
  linkIR (new Assign4 (d, s));
}



//----------  Assign8  ----------

void Assign8::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (src);
  fprintf (outputFile, "\t\t(8 bytes)\n");

  getIntoReg8 (src, "f0", "r1");
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRAssign8 (AstNode * d, AstNode * s) {
  linkIR (new Assign8 (d, s));
}



//----------  Ascii  ----------

void Ascii::print () {
  fprintf (outputFile, "\t.ascii\t\"%s\"\n", str);
}

void IRAscii (char * s) {
  linkIR (new Ascii (s));
}



//----------  Ascii0  ----------

void Ascii0::print () {
  fprintf (outputFile, "\t.ascii\t\"%s\\0\"\n", str);
}

void IRAscii0 (char * s) {
  linkIR (new Ascii0 (s));
}



//----------  Ascii2  ----------

void Ascii2::print () {
  fprintf (outputFile, "\t.word\t%d\t\t\t! length\n", str->length);
  fprintf (outputFile, "\t.ascii\t\"");
  printString (outputFile, str);
  fprintf (outputFile, "\"\n");
}

void IRAscii2 (String * s) {
  linkIR (new Ascii2 (s));
}



//----------  IAdd  ----------

void IAdd::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " + ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tadd\tr1,r2,r1\n");
  overflowTest ();
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRIAdd (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new IAdd (d, a1, a2));
}



//----------  ISub  ----------

void ISub::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " - ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tsub\tr1,r2,r1\n");
  overflowTest ();
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRISub (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new ISub (d, a1, a2));
}



//----------  IMul  ----------

void IMul::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " * ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tmul\tr1,r2,r1\n");
  overflowTest ();
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRIMul (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new IMul (d, a1, a2));
}



//----------  IDiv  ----------

void IDiv::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " div ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr2,0\n");
  fprintf (outputFile, "\tbe\t_runtimeErrorZeroDivide\n");
  fprintf (outputFile, "\tdiv\tr1,r2,r1\n");
  overflowTest ();
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRIDiv (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new IDiv (d, a1, a2));
}



//----------  IRem  ----------

void IRem::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " rem ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr2,0\n");
  fprintf (outputFile, "\tbe\t_runtimeErrorZeroDivide\n");
  fprintf (outputFile, "\trem\tr1,r2,r1\n");
  overflowTest ();
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRIRem (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new IRem (d, a1, a2));
}



//----------  Sll  ----------

void Sll::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " sll ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tsll\tr1,r2,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRSll (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new Sll (d, a1, a2));
}



//----------  Sra  ----------

void Sra::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " sra ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tsra\tr1,r2,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRSra (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new Sra (d, a1, a2));
}



//----------  Srl  ----------

void Srl::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " srl ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tsrl\tr1,r2,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRSrl (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new Srl (d, a1, a2));
}



//----------  And  ----------

void And::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " AND ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tand\tr1,r2,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRAnd (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new And (d, a1, a2));
}



//----------  Or  ----------

void Or::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " OR ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tor\tr1,r2,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IROr (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new Or (d, a1, a2));
}



//----------  Xor  ----------

void Xor::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " XOR ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\txor\tr1,r2,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRXor (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new Xor (d, a1, a2));
}



//----------  INeg  ----------

void INeg::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = -");
  printANode (arg);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\tneg\tr1\n");
  overflowTest ();
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRINeg (AstNode * d, AstNode * a) {
  linkIR (new INeg (d, a));
}



//----------  Not  ----------

void Not::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = NOT ");
  printANode (arg);
  fprintf (outputFile, "\t\t(int)\n");
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\tnot\tr1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRNot (AstNode * d, AstNode * a) {
  linkIR (new Not (d, a));
}



//----------  FAdd  ----------

void FAdd::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " + ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(float)\n");
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r2");
  fprintf (outputFile, "\tfadd\tf0,f1,f0\n");
  // CCR is not affected: no overflow test possible
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRFAdd (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new FAdd (d, a1, a2));
}



//----------  FSub  ----------

void FSub::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " + ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(float)\n");
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r2");
  fprintf (outputFile, "\tfsub\tf0,f1,f0\n");
  // CCR is not affected: no overflow test possible
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRFSub (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new FSub (d, a1, a2));
}



//----------  FMul  ----------

void FMul::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " + ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(float)\n");
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r2");
  fprintf (outputFile, "\tfmul\tf0,f1,f0\n");
  // CCR is not affected: no overflow test possible
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRFMul (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new FMul (d, a1, a2));
}



//----------  FDiv  ----------

void FDiv::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " + ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(float)\n");
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r2");
  fprintf (outputFile, "\tfdiv\tf0,f1,f0\n");
  // CCR is not affected: no overflow test possible
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRFDiv (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new FDiv (d, a1, a2));
}



//----------  FNeg  ----------

void FNeg::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = -");
  printANode (arg);
  fprintf (outputFile, "\t\t(float)\n");
  getIntoReg8 (arg, "f0", "r1");
  fprintf (outputFile, "\tfneg\tf0,f0\n");
  // CCR is not affected: no overflow test possible
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRFNeg (AstNode * d, AstNode * a) {
  linkIR (new FNeg (d, a));
}



//----------  ItoF  ----------

void ItoF::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = intToFloat (");
  printANode (arg);
  fprintf (outputFile, ")\n");
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\titof\tr1,f0\n");
  // CCR is not affected: no overflow test possible
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRItoF (AstNode * d, AstNode * a) {
  linkIR (new ItoF (d, a));
}



//----------  FtoI  ----------

void FtoI::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = floatToInt (");
  printANode (arg);
  fprintf (outputFile, ")\n");
  getIntoReg8 (arg, "f0", "r1");
  fprintf (outputFile, "\tftoi\tf0,r1\n");
  // CCR is not affected: no overflow test possible
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRFtoI (AstNode * d, AstNode * a) {
  linkIR (new FtoI (d, a));
}



//----------  ItoC  ----------

void ItoC::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = intToChar (");
  printANode (arg);
  fprintf (outputFile, ")\n");
  getIntoReg4 (arg, "r1");
  // CCR is not affected: no overflow test possible
  storeFromReg1 ((VarDecl *) dest, "r1", "r2");
}

void IRItoC (AstNode * d, AstNode * a) {
  linkIR (new ItoC (d, a));
}



//----------  CtoI  ----------

void CtoI::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = charToInt (");
  printANode (arg);
  fprintf (outputFile, ")\n");
  getIntoReg1 (arg, "r1");
  fprintf (outputFile, "\tsll\tr1,24,r1\n");
  fprintf (outputFile, "\tsra\tr1,24,r1\n");
  storeFromReg4 ((VarDecl *) dest, "r1", "r2");
}

void IRCtoI (AstNode * d, AstNode * a) {
  linkIR (new CtoI (d, a));
}



//----------  PosInf  ----------

void PosInf::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = PosInf\n");
  fprintf (outputFile, "\tset\t_PosInf,r1\n");
  fprintf (outputFile, "\tfload\t[r1],f0\n");
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRPosInf (AstNode * d) {
  linkIR (new PosInf (d));
}



//----------  NegInf  ----------

void NegInf::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = NegInf\n");
  fprintf (outputFile, "\tset\t_NegInf,r1\n");
  fprintf (outputFile, "\tfload\t[r1],f0\n");
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRNegInf (AstNode * d) {
  linkIR (new NegInf (d));
}



//----------  NegZero  ----------

void NegZero::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = NegZero\n");
  fprintf (outputFile, "\tset\t_NegZero,r1\n");
  fprintf (outputFile, "\tfload\t[r1],f0\n");
  storeFromReg8 ((VarDecl *) dest, "f0", "r2");
}

void IRNegZero (AstNode * d) {
  linkIR (new NegZero (d));
}



//----------  IntLTGoto  ----------

void IntLTGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " < ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbl\t%s\n", label);
}

void IRIntLTGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new IntLTGoto (a1, a2, lab));
}



//----------  IntLEGoto  ----------

void IntLEGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " <= ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tble\t%s\n", label);
}

void IRIntLEGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new IntLEGoto (a1, a2, lab));
}



//----------  IntGTGoto  ----------

void IntGTGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " > ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbg\t%s\n", label);
}

void IRIntGTGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new IntGTGoto (a1, a2, lab));
}



//----------  IntGEGoto  ----------

void IntGEGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " >= ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbge\t%s\n", label);
}

void IRIntGEGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new IntGEGoto (a1, a2, lab));
}



//----------  IntEQGoto  ----------

void IntEQGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " == ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  // overflowTest ();             // The Z flag will be ok, even if overflow
  fprintf (outputFile, "\tbe\t%s\n", label);
}

void IRIntEQGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new IntEQGoto (a1, a2, lab));
}



//----------  IntNEGoto  ----------

void IntNEGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " != ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg1, "r1");
  getIntoReg4 (arg2, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  // overflowTest ();             // The Z flag will be ok, even if overflow
  fprintf (outputFile, "\tbne\t%s\n", label);
}

void IRIntNEGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new IntNEGoto (a1, a2, lab));
}



//----------  FloatLTGoto  ----------

void FloatLTGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " < ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(float)\n", label);
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r1");
  fprintf (outputFile, "\tfcmp\tf0,f1\n");
  overflowTest ();             // Can occur if either is Nan
  fprintf (outputFile, "\tbl\t%s\n", label);
}

void IRFloatLTGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new FloatLTGoto (a1, a2, lab));
}



//----------  FloatLEGoto  ----------

void FloatLEGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " <= ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(float)\n", label);
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r1");
  fprintf (outputFile, "\tfcmp\tf0,f1\n");
  overflowTest ();             // Can occur if either is Nan
  fprintf (outputFile, "\tble\t%s\n", label);
}

void IRFloatLEGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new FloatLEGoto (a1, a2, lab));
}



//----------  FloatGTGoto  ----------

void FloatGTGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " > ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(float)\n", label);
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r1");
  fprintf (outputFile, "\tfcmp\tf0,f1\n");
  overflowTest ();             // Can occur if either is Nan
  fprintf (outputFile, "\tbg\t%s\n", label);
}

void IRFloatGTGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new FloatGTGoto (a1, a2, lab));
}



//----------  FloatGEGoto  ----------

void FloatGEGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " >= ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(float)\n", label);
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r1");
  fprintf (outputFile, "\tfcmp\tf0,f1\n");
  overflowTest ();             // Can occur if either is Nan
  fprintf (outputFile, "\tbge\t%s\n", label);
}

void IRFloatGEGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new FloatGEGoto (a1, a2, lab));
}



//----------  FloatEQGoto  ----------

void FloatEQGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " == ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(float)\n", label);
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r1");
  fprintf (outputFile, "\tfcmp\tf0,f1\n");
  overflowTest ();             // Can occur if either is Nan
  fprintf (outputFile, "\tbe\t%s\n", label);
}

void IRFloatEQGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new FloatEQGoto (a1, a2, lab));
}



//----------  FloatNEGoto  ----------

void FloatNEGoto::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg1);
  fprintf (outputFile, " != ");
  printANode (arg2);
  fprintf (outputFile, " then goto %s\t\t(float)\n", label);
  getIntoReg8 (arg1, "f0", "r1");
  getIntoReg8 (arg2, "f1", "r1");
  fprintf (outputFile, "\tfcmp\tf0,f1\n");
  overflowTest ();             // Can occur if either is Nan
  fprintf (outputFile, "\tbne\t%s\n", label);
}

void IRFloatNEGoto (AstNode * a1, AstNode * a2, char * lab) {
  linkIR (new FloatNEGoto (a1, a2, lab));
}



//----------  BoolTest  ----------

void BoolTest::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg);
  fprintf (outputFile, " then goto %s else goto %s\n", trueLabel, falseLabel);
  getIntoReg1 (arg, "r1");
  fprintf (outputFile, "\tcmp\tr1,0\n");
  fprintf (outputFile, "\tbe\t%s\n", falseLabel);
  fprintf (outputFile, "\tjmp\t%s\n", trueLabel);
}

void IRBoolTest (AstNode * a, char * lab1, char * lab2) {
  linkIR (new BoolTest (a, lab1, lab2));
}



//----------  BoolTest2  ----------

void BoolTest2::print () {
  fprintf (outputFile, "!   if result==true then goto %s else goto %s\n", trueLabel, falseLabel);
  fprintf (outputFile, "\tloadb\t[r15],r1\n");
  fprintf (outputFile, "\tcmp\tr1,0\n");
  fprintf (outputFile, "\tbe\t%s\n", falseLabel);
  fprintf (outputFile, "\tjmp\t%s\n", trueLabel);
}

void IRBoolTest2 (char * lab1, char * lab2) {
  linkIR (new BoolTest2 (lab1, lab2));
}



//----------  BXor  ----------

void BXor::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " XOR ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(bool)\n");
  getIntoReg1 (arg1, "r1");
  getIntoReg1 (arg2, "r2");
  fprintf (outputFile, "\txor\tr1,r2,r1\n");
  storeFromReg1 ((VarDecl *) dest, "r1", "r2");
}

void IRBXor (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new BXor (d, a1, a2));
}



//----------  BEq  ----------

void BEq::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (arg1);
  fprintf (outputFile, " XOR ");
  printANode (arg2);
  fprintf (outputFile, "\t\t(bool)\n");
  getIntoReg1 (arg1, "r1");
  getIntoReg1 (arg2, "r2");
  fprintf (outputFile, "\txor\tr1,r2,r1\n");
  fprintf (outputFile, "\tbtog\t1,r1\n");
  storeFromReg1 ((VarDecl *) dest, "r1", "r2");
}

void IRBEq (AstNode * d, AstNode * a1, AstNode * a2) {
  linkIR (new BEq (d, a1, a2));
}



//----------  BNot  ----------

void BNot::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  fprintf (outputFile, " NOT ");
  printANode (arg);
  fprintf (outputFile, "\t\t(bool)\n");
  getIntoReg1 (arg, "r1");
  fprintf (outputFile, "\tbtog\t1,r1\n");
  storeFromReg1 ((VarDecl *) dest, "r1", "r2");
}

void IRBNot (AstNode * d, AstNode * a) {
  linkIR (new BNot (d, a));
}



//----------  IntEqZero  ----------

void IntEqZero::print () {
  fprintf (outputFile, "!   if intIsZero (");
  printANode (arg);
  fprintf (outputFile, ") then goto %s\n", label);
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\tcmp\tr1,r0\n");
  fprintf (outputFile, "\tbe\t%s\n", label);
}

void IRIntEqZero (AstNode * a, char * lab) {
  linkIR (new IntEqZero (a, lab));
}



//----------  IntNeZero  ----------

void IntNeZero::print () {
  fprintf (outputFile, "!   if intNotZero (");
  printANode (arg);
  fprintf (outputFile, ") then goto %s\n", label);
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\tcmp\tr1,r0\n");
  fprintf (outputFile, "\tbne\t%s\n", label);
}

void IRIntNeZero (AstNode * a, char * lab) {
  linkIR (new IntNeZero (a, lab));
}



//----------  IntLeZero  ----------

void IntLeZero::print () {
  fprintf (outputFile, "!   if ");
  printANode (arg);
  fprintf (outputFile, " <= 0 then goto %s\n", label);
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\tcmp\tr1,r0\n");
  fprintf (outputFile, "\tble\t%s\n", label);
}

void IRIntLeZero (AstNode * a, char * lab) {
  linkIR (new IntLeZero (a, lab));
}



//----------  BoolEqZeroIndirect  ----------

void BoolEqZeroIndirect::print () {
  fprintf (outputFile, "!   if boolIsZero (");
  printANode (arg);
  fprintf (outputFile, " ) then goto %s\t\t(int)\n", label);
  getIntoReg4 (arg, "r1");
  fprintf (outputFile, "\tloadb\t[r1],r1\n");
  fprintf (outputFile, "\tcmp\tr1,r0\n");
  fprintf (outputFile, "\tbe\t%s\n", label);
}

void IRBoolEqZeroIndirect (VarDecl * a, char * lab) {
  linkIR (new BoolEqZeroIndirect (a, lab));
}



//----------  PrepareArg  ----------

void PrepareArg::print () {
  char * label;
  int i;
  fprintf (outputFile, "!   Prepare Argument: offset=%d  value=", offset);
  printANode (tempName);
  fprintf (outputFile, "  sizeInBytes=%d\n", sizeInBytes);
  if (sizeInBytes == 1) {
    getIntoReg1 (tempName, "r1");
    if (within16Bits (offset-8)) {
      fprintf (outputFile, "\tstoreb\tr1,[r15+%d]\n", offset-8);
    } else {
      fprintf (outputFile, "\tset\t%d,r2\n", offset-8);
      fprintf (outputFile, "\tstoreb\tr1,[r15+r2]\n");
    }
  } else if (sizeInBytes == 4) {
    getIntoReg4 (tempName, "r1");
    if (within16Bits (offset-8)) {
      fprintf (outputFile, "\tstore\tr1,[r15+%d]\n", offset-8);
    } else {
      fprintf (outputFile, "\tset\t%d,r2\n", offset-8);
      fprintf (outputFile, "\tstore\tr1,[r15+r2]\n");
    }
  } else if (sizeInBytes == 8) {
    getIntoReg8 (tempName, "f0", "r1");
    if (within16Bits (offset-8)) {
      fprintf (outputFile, "\tfstore\tf0,[r15+%d]\n", offset-8);
    } else {
      fprintf (outputFile, "\tset\t%d,r2\n", offset-8);
      fprintf (outputFile, "\tfstore\tf0,[r15+r2]\n");
    }
  } else if ((sizeInBytes < 0) || (sizeInBytes%4 != 0)) {
    printf ("sizeInBytes = %d\n", sizeInBytes);
    programLogicError ("In PrepareArg, problems with sizeInBytes");
  } else {
    // Generate code to move N bytes into the arg position...
    //     r3 = counter of words
    //     r4 = ptr to dest
    //     r5 = ptr to source
    //     r1 = word being moved
    label = newLabel ();
    if (within16Bits (sizeInBytes / 4)) {
      fprintf (outputFile, "\tmov\t%d,r3\n", sizeInBytes / 4);
    } else {
      fprintf (outputFile, "\tset\t%d,r3\n", sizeInBytes / 4);
    }
    if (within16Bits (offset-8)) {
      fprintf (outputFile, "\tadd\tr15,%d,r4\n", offset-8);
    } else {
      fprintf (outputFile, "\tset\t%d,r4\n", offset-8);
      fprintf (outputFile, "\tadd\tr15,r4,r4\n");
    }
    getAddrOfVarIntoReg (tempName, "r5");
    fprintf (outputFile, "%s:\n", label);
    fprintf (outputFile, "\tload\t[r5],r1\n");
    fprintf (outputFile, "\tadd\tr5,4,r5\n");
    fprintf (outputFile, "\tstore\tr1,[r4]\n");
    fprintf (outputFile, "\tadd\tr4,4,r4\n");
    fprintf (outputFile, "\tsub\tr3,1,r3\n");
    fprintf (outputFile, "\tbne\t%s\n", label);
  }
}

void IRPrepareArg (int off, AstNode * tname, int size) {
  linkIR (new PrepareArg (off, tname, size));
}



//----------  RetrieveResult  ----------

void RetrieveResult::print () {
  char * label;
  int i;

  fprintf (outputFile, "!   Retrieve Result: targetName=");
  printANode (targetName);
  fprintf (outputFile, "  sizeInBytes=%d\n", sizeInBytes);
  if (sizeInBytes == 1) {
    fprintf (outputFile, "\tloadb\t[r15],r1\n");
    storeFromReg1 (targetName, "r1", "r2");
  } else if (sizeInBytes == 4) {
    fprintf (outputFile, "\tload\t[r15],r1\n");
    storeFromReg4 (targetName, "r1", "r2");
  } else if (sizeInBytes == 8) {
    fprintf (outputFile, "\tfload\t[r15],f0\n");
    storeFromReg8 (targetName, "f0", "r2");
  } else if ((sizeInBytes < 0) || (sizeInBytes%4 != 0)) {
    printf ("sizeInBytes = %d\n", sizeInBytes);
    programLogicError ("In RetrieveResult, problems with sizeInBytes");
  } else {
    // Generate code to move N bytes from result to target...
    //     r3 = counter of words
    //     r4 = ptr to dest
    //     r5 = ptr to source
    //     r1 = word being moved
    label = newLabel ();
    if (within16Bits (sizeInBytes / 4)) {
      fprintf (outputFile, "\tmov\t%d,r3\n", sizeInBytes / 4);
    } else {
      fprintf (outputFile, "\tset\t%d,r3\n", sizeInBytes / 4);
    }
    fprintf (outputFile, "\tmov\tr15,r5\n");
    getAddrOfVarIntoReg (targetName, "r4");
    fprintf (outputFile, "%s:\n", label);
    fprintf (outputFile, "\tload\t[r5],r1\n");
    fprintf (outputFile, "\tadd\tr5,4,r5\n");
    fprintf (outputFile, "\tstore\tr1,[r4]\n");
    fprintf (outputFile, "\tadd\tr4,4,r4\n");
    fprintf (outputFile, "\tsub\tr3,1,r3\n");
    fprintf (outputFile, "\tbne\t%s\n", label);
  }
}

void IRRetrieveResult (VarDecl * tname, int size) {
  linkIR (new RetrieveResult (tname, size));
}



//----------  ReturnResult  ----------

void ReturnResult::print () {
  char * label;
  int i;

  fprintf (outputFile, "!   ReturnResult: ");
  printANode (tempName);
  fprintf (outputFile, "  (sizeInBytes=%d)\n", sizeInBytes);
  if (sizeInBytes == 1) {
    getIntoReg1 (tempName, "r1");
    fprintf (outputFile, "\tstoreb\tr1,[r14+8]\n");
  } else if (sizeInBytes == 4) {
    getIntoReg4 (tempName, "r1");
    fprintf (outputFile, "\tstore\tr1,[r14+8]\n");
  } else if (sizeInBytes == 8) {
    getIntoReg8 (tempName, "f0", "r1");
    fprintf (outputFile, "\tfstore\tf0,[r14+8]\n");
  } else if ((sizeInBytes < 0) || (sizeInBytes%4 != 0)) {
    printf ("sizeInBytes = %d\n", sizeInBytes);
    programLogicError ("In ReturnResult, problems with sizeInBytes");
  } else {
    // Generate code to move N bytes into the arg position...
    //     r3 = counter of words
    //     r4 = ptr to dest (r14+8)
    //     r5 = ptr to source (tempName)
    //     r1 = word being moved
    label = newLabel ();
    if (within16Bits (sizeInBytes / 4)) {
      fprintf (outputFile, "\tmov\t%d,r3\n", sizeInBytes / 4);
    } else {
      fprintf (outputFile, "\tset\t%d,r3\n", sizeInBytes / 4);
    }
    fprintf (outputFile, "\tadd\tr14,8,r4\n");
    getAddrOfVarIntoReg (tempName, "r5");
    fprintf (outputFile, "%s:\n", label);
    fprintf (outputFile, "\tload\t[r5],r1\n");
    fprintf (outputFile, "\tadd\tr5,4,r5\n");
    fprintf (outputFile, "\tstore\tr1,[r4]\n");
    fprintf (outputFile, "\tadd\tr4,4,r4\n");
    fprintf (outputFile, "\tsub\tr3,1,r3\n");
    fprintf (outputFile, "\tbne\t%s\n", label);
  }
}

void IRReturnResult (AstNode * tname, int size) {
  linkIR (new ReturnResult (tname, size));
}



//----------  Comment2  ----------

void Comment2::print () {
  fprintf (outputFile, "! %s%s%s\n", str1, str2, str3);
}

void IRComment2 (char * str1, char * str2, char * str3) {
  linkIR (new Comment2 (str1, str2, str3));
}



//----------  Set  ----------

void Set::print () {
  if (within16Bits (initValue)) {
    fprintf (outputFile, "\tmov\t%d,r1\n", initValue);
  } else if (initValue == 0x80000000) {
    fprintf (outputFile, "\tset\t0x80000000,r1\n");
  } else {
    fprintf (outputFile, "\tset\t%d,r1\n", initValue);
  }
  storeFromReg4 (varDecl, "r1", "r2");
}

void IRSet (VarDecl * v, int i) {
  linkIR (new Set (v, i));
}



//----------  Send  ----------

void Send::print () {
  fprintf (outputFile, "!   Send message %s\n", methProto->selector->chars);
  getIntoReg4 (recvr, "r1");
//  The check for null pointer occurs elsewhere, when the ptr is first encountered.
//     fprintf (outputFile, "\tcmp\tr1,0\n");
//     fprintf (outputFile, "\tbe\t_runtimeErrorNullPointer\n");
  fprintf (outputFile, "\tload\t[r1],r2\n");
  fprintf (outputFile, "\tcmp\tr2,0\n");
  fprintf (outputFile, "\tbe\t _runtimeErrorUninitializedObject\n");
  fprintf (outputFile, "\tstore\tr1,[r15]\n");
  if (within16Bits (methProto->offset)) {
    fprintf (outputFile, "\tadd\tr2,%d,r2\n", methProto->offset);
  } else {
    fprintf (outputFile, "\tset\t%d,r11\n", methProto->offset);
    fprintf (outputFile, "\tadd\tr2,r11,r2\n");
    // printf ("16-bit overflow in IRSend   Method=%s  offset=%d\n",
    //         methProto->selector->chars, methProto->offset);
  }
  fprintf (outputFile, "\tcall\tr2\n");
}

void IRSend (VarDecl * r, MethodProto * m) {
  linkIR (new Send (r, m));
}



//----------  LoadSelfPtr  ----------

void LoadSelfPtr::print () {
    fprintf (outputFile, "\tload\t[r14+8],r1\n");
    storeFromReg4 (targetName, "r1", "r2");
}

void IRLoadSelfPtr (VarDecl * tname) {
  linkIR (new LoadSelfPtr (tname));
}



//----------  Move  ----------

void Move::print () {
  char * label;
  int i;
  fprintf (outputFile, "!   Data Move: ");
  if (targetVar) {
    printANode (targetVar);
  }
  if (targetPtr) {
    fprintf (outputFile, "*");
    printANode (targetPtr);
  }
  fprintf (outputFile, " = ");
  if (srcVar) {
    printANode (srcVar);
  }
  if (srcPtr) {
    fprintf (outputFile, "*");
    printANode (srcPtr);
  }
  fprintf (outputFile, "  (sizeInBytes=%d)\n", sizeInBytes);
  if ((targetVar==NULL) == (targetPtr==NULL)) {
    programLogicError ("In Move, targetVar and targetPtr are incorrect");
  }
  if ((srcVar==NULL) == (srcPtr==NULL)) {
    programLogicError ("In Move, srcVar and srcPtr are incorrect");
  }
  if (sizeInBytes == 1) {
    if (srcVar) {
      // untested...
      getIntoReg1 (srcVar, "r1");
    } else if (srcPtr) {
      getIntoReg4 (srcPtr, "r1");
      fprintf (outputFile, "\tloadb\t[r1],r1\n");
    }
    if (targetVar) {
      storeFromReg1 (targetVar, "r1", "r2");
    } else if (targetPtr) {
      // untested...
      getIntoReg4 (targetPtr, "r2");
      fprintf (outputFile, "\tstoreb\tr1,[r2]\n");
    }
  } else if (sizeInBytes == 4) {
    if (srcVar) {
      getIntoReg4 (srcVar, "r1");
    } else if (srcPtr) {
      getIntoReg4 (srcPtr, "r1");
      fprintf (outputFile, "\tload\t[r1],r1\n");
    }
    if (targetVar) {
      storeFromReg4 (targetVar, "r1", "r2");
    } else if (targetPtr) {
      // untested...
      getIntoReg4 (targetPtr, "r2");
      fprintf (outputFile, "\tstore\tr1,[r2]\n");
    }
  } else if (sizeInBytes == 8) {
    if (srcVar) {
      // untested...
      getIntoReg8 (srcVar, "f1", "r1");
    } else if (srcPtr) {
      getIntoReg4 (srcPtr, "r1");
      fprintf (outputFile, "\tfload\t[r1],f1\n");
    }
    if (targetVar) {
      storeFromReg8 (targetVar, "f1", "r2");
    } else if (targetPtr) {
      // untested...
      getIntoReg4 (targetPtr, "r2");
      fprintf (outputFile, "\tfstore\tf1,[r2]\n");
    }
  } else if ((sizeInBytes < 0) || (sizeInBytes%4 != 0)) {
    printf ("sizeInBytes = %d\n", sizeInBytes);
    programLogicError ("In Move, problems with sizeInBytes");
  } else {

    if (srcVar) {
      // untested...
      getAddrOfVarIntoReg (srcVar, "r5");
    } else if (srcPtr) {
      getIntoReg4 (srcPtr, "r5");
    }
    if (targetVar) {
      // untested...
      getAddrOfVarIntoReg (targetVar, "r4");
    } else if (targetPtr) {
      getIntoReg4 (targetPtr, "r4");
    }

    if (sizeInBytes == 12) {
      fprintf (outputFile, "\tload\t[r5],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4]\n");
      fprintf (outputFile, "\tload\t[r5+4],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+4]\n");
      fprintf (outputFile, "\tload\t[r5+8],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+8]\n");
    } else if (sizeInBytes == 16) {
      fprintf (outputFile, "\tload\t[r5],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4]\n");
      fprintf (outputFile, "\tload\t[r5+4],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+4]\n");
      fprintf (outputFile, "\tload\t[r5+8],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+8]\n");
      fprintf (outputFile, "\tload\t[r5+12],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+12]\n");
    } else if (sizeInBytes == 20) {
      fprintf (outputFile, "\tload\t[r5],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4]\n");
      fprintf (outputFile, "\tload\t[r5+4],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+4]\n");
      fprintf (outputFile, "\tload\t[r5+8],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+8]\n");
      fprintf (outputFile, "\tload\t[r5+12],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+12]\n");
      fprintf (outputFile, "\tload\t[r5+16],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+16]\n");
    } else if (sizeInBytes == 24) {
      fprintf (outputFile, "\tload\t[r5],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4]\n");
      fprintf (outputFile, "\tload\t[r5+4],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+4]\n");
      fprintf (outputFile, "\tload\t[r5+8],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+8]\n");
      fprintf (outputFile, "\tload\t[r5+12],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+12]\n");
      fprintf (outputFile, "\tload\t[r5+16],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+16]\n");
      fprintf (outputFile, "\tload\t[r5+20],r1\n");
      fprintf (outputFile, "\tstore\tr1,[r4+20]\n");
    } else {
      // Generate code to move N bytes...
      //     r3 = counter of words
      //     r4 = ptr to dest
      //     r5 = ptr to source
      //     r1 = word being moved
      label = newLabel ();
      if (within16Bits (sizeInBytes / 4)) {
        fprintf (outputFile, "\tmov\t%d,r3\n", sizeInBytes / 4);
      } else {
        fprintf (outputFile, "\tset\t0x%08x,r3\t\t\t! decimal = %d\n",
                 sizeInBytes / 4, sizeInBytes / 4);
      }
      fprintf (outputFile, "%s:\n", label);
      fprintf (outputFile, "\tload\t[r5],r1\n");
      fprintf (outputFile, "\tadd\tr5,4,r5\n");
      fprintf (outputFile, "\tstore\tr1,[r4]\n");
      fprintf (outputFile, "\tadd\tr4,4,r4\n");
      fprintf (outputFile, "\tsub\tr3,1,r3\n");
      fprintf (outputFile, "\tbne\t%s\n", label);
    }
  }
}

void IRMove (VarDecl * tarV, VarDecl * tarP, AstNode * srcV, VarDecl * srcP, int sz) {
  linkIR (new Move (tarV, tarP, srcV, srcP, sz));
}



//----------  DynamicObjectMove  ----------

void DynamicObjectMove::print () {
  char * label;
  int i;
  fprintf (outputFile, "!   Dynamic Object Move: *");
  printANode (targetPtr);
  fprintf (outputFile, " = *");
  printANode (srcPtr);
  fprintf (outputFile, "\n");
  // Generate code to get pointers to the objects into r4 and r5...
  // fprintf (outputFile, "! get ptr to target into r4 and ptr to src into r5...\n");
  getIntoReg4 (targetPtr, "r4");
  getIntoReg4 (srcPtr, "r5");
  // Generate code to make sure the dispatch table ptrs are the same...
  // fprintf (outputFile, "! make sure the dispatch table ptrs are the same...\n");
  fprintf (outputFile, "\tload\t[r4],r1\n");
  fprintf (outputFile, "\tload\t[r5],r3\n");
  fprintf (outputFile, "\tcmp\tr1,r3\n");
  fprintf (outputFile, "\tbne\t_runtimeErrorWrongObject3\n");
  // Generate code to get the size in words into r3...
  // fprintf (outputFile, "! get size in words into r3...\n");
  fprintf (outputFile, "\tload\t[r3],r3\n");
  fprintf (outputFile, "\tload\t[r3+16],r3\n");
  fprintf (outputFile, "\tcmp\tr3,4\n");
  fprintf (outputFile, "\tbl\t_runtimeErrorBadObjectSize\n");
  fprintf (outputFile, "\tsrl\tr3,2,r3\n");
  // Generate code to move N bytes...
  //     r3 = counter of words
  //     r4 = ptr to dest
  //     r5 = ptr to source
  //     r1 = word being moved
  // fprintf (outputFile, "! do the move...\n");
  label = newLabel ();
  fprintf (outputFile, "%s:\n", label);
  fprintf (outputFile, "\tload\t[r5],r1\n");
  fprintf (outputFile, "\tadd\tr5,4,r5\n");
  fprintf (outputFile, "\tstore\tr1,[r4]\n");
  fprintf (outputFile, "\tadd\tr4,4,r4\n");
  fprintf (outputFile, "\tsub\tr3,1,r3\n");
  fprintf (outputFile, "\tbne\t%s\n", label);
  fprintf (outputFile, "! done with move\n");
}

void IRDynamicObjectMove (VarDecl * tar, VarDecl * src) {
  linkIR (new DynamicObjectMove (tar, src));
}



//   //----------  ZeroLocal  ----------
//   
//   void ZeroLocal::print () {
//     int i;
//     if (local->sizeInBytes == 1) {
//       fprintf (outputFile, "\tstoreb\tr0,[r14+%d]\n", local->offset);
//     } else {
//       if (local->sizeInBytes <= 20) {
//         for (i = 0; i < local->sizeInBytes; i = i + 4) {
//           fprintf (outputFile, "\tstore\tr0,[r14+%d]\n", local->offset + i);
//         }
//       } else {
//         printf ("UNIMPLEMENTED / UNTESTED: IRZeroLocal with large value\n");
//       }
//     }
//   }
//   
//   void IRZeroLocal (Local * loc) {
//     linkIR (new ZeroLocal (loc));
//   }



//----------  CheckDPT  ----------

void CheckDPT::print () {
  getIntoReg4 (var, "r1");
  fprintf (outputFile, "\tload\t[r1],r1\n");
  fprintf (outputFile, "\tset\t%s,r2\n", classDef->newName);
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  fprintf (outputFile, "\tbne\t_runtimeErrorWrongObject\n");
}

void IRCheckDPT (VarDecl * v, ClassDef * cl) {
  linkIR (new CheckDPT (v, cl));
}



//----------  CheckDPT2  ----------

void CheckDPT2::print () {
  getIntoReg4 (var, "r1");
  fprintf (outputFile, "\tload\t[r1],r1\n");
  fprintf (outputFile, "\tset\t%s,r2\n", classDef->newName);
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  fprintf (outputFile, "\tbne\t_runtimeErrorWrongObject2\n");
}

void IRCheckDPT2 (VarDecl * v, ClassDef * cl) {
  linkIR (new CheckDPT2 (v, cl));
}



//----------  CopyArrays  ----------

void CopyArrays::print () {
  char * label;
  int i;
  fprintf (outputFile, "!   Dynamic Array Copy: *");
  printANode (targetPtr);
  fprintf (outputFile, " = *");
  printANode (srcPtr);
  fprintf (outputFile, "\n");
  // Generate code to get pointers to the arrays into r4 and r5...
  fprintf (outputFile, "!     get ptr to target into r4 and ptr to src into r5...\n");
  getIntoReg4 (targetPtr, "r4");
  getIntoReg4 (srcPtr, "r5");
  // Generate code to make sure the sizes are the same...
  fprintf (outputFile, "!     make sure the sizes are the same...\n");
  fprintf (outputFile, "\tload\t[r4],r1\n");
  fprintf (outputFile, "\tload\t[r5],r3\n");
  fprintf (outputFile, "\tcmp\tr1,r3\n");
  fprintf (outputFile, "\tbne\t_runtimeErrorDifferentArraySizes\n");
  // Generate code to get the size in words into r3...
  fprintf (outputFile, "!     get size in words into r3...\n");
  if (within16Bits (elementSize)) {
    fprintf (outputFile, "\tmul\tr3,%d,r3\t\t! multiply by size of elements\n", elementSize);
  } else {
    // printf ("16-BIT OVERFLOW;  DEST = %s, SOURCE = %s, ELEMENT SIZE = %d\n",
    //         ((VarDecl *) targetPtr)->id->chars, ((VarDecl *) srcPtr)->id->chars,
    //         elementSize);
    fprintf (outputFile, "\tset\t%d,r11\t\t! multiply by size of elements\n", elementSize);
    fprintf (outputFile, "\tmul\tr3,r11,r3\n");
  }
  overflowTest ();
  fprintf (outputFile, "\tadd\tr3,7,r3\t\t! add 4 and divide by 4, rounding up\n");
  overflowTest ();
  fprintf (outputFile, "\tsrl\tr3,2,r3\n");
  // Generate code to move N bytes...
  //     r3 = counter of words
  //     r4 = ptr to dest
  //     r5 = ptr to source
  //     r1 = word being moved
  fprintf (outputFile, "!     do the move...\n");
  label = newLabel ();
  fprintf (outputFile, "%s:\n", label);
  fprintf (outputFile, "\tload\t[r5],r1\n");
  fprintf (outputFile, "\tadd\tr5,4,r5\n");
  fprintf (outputFile, "\tstore\tr1,[r4]\n");
  fprintf (outputFile, "\tadd\tr4,4,r4\n");
  fprintf (outputFile, "\tsub\tr3,1,r3\n");
  fprintf (outputFile, "\tbne\t%s\n", label);
  fprintf (outputFile, "! done with move\n");
}

void IRCopyArrays (VarDecl * tar, VarDecl * src, int i) {
  linkIR (new CopyArrays (tar, src, i));
}



//----------  CheckArraySizeInt  ----------

void CheckArraySizeInt::print () {
  // This instruction is passed a variable "ptr", which contains the address
  // of an array, and "numberOfElements", which is the expected size of the
  // the array.  It signals an error if the array does not have that size.
  fprintf (outputFile, "!   make sure array has size %d\n", numberOfElements);
  getIntoReg4 (ptr, "r1");
  fprintf (outputFile, "\tload\t[r1],r1\n");
  fprintf (outputFile, "\tset\t%d, r2\n", numberOfElements);
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbne\t_runtimeErrorWrongArraySize\n");
}

void IRCheckArraySizeInt (VarDecl * p, int i) {
  linkIR (new CheckArraySizeInt (p, i));
}



//----------  CheckArraySizeInt2  ----------

void CheckArraySizeInt2::print () {
  // This instruction is passed a variable "ptr", which contains the address
  // of an array, and "numberOfElements", which is the expected size of the
  // the array.  It signals an error if the array does not have that size.
  // HOWEVER: If the array size if zero (i.e., the array is uninitialized), it
  // ignores the previous size.
  char * label;
  fprintf (outputFile, "!   make sure array has size %d\n", numberOfElements);
  getIntoReg4 (ptr, "r1");
  fprintf (outputFile, "\tload\t[r1],r1\n");
  fprintf (outputFile, "\tset\t%d, r2\n", numberOfElements);
  fprintf (outputFile, "\tcmp\tr1,0\n");
  label = newLabel ();
  fprintf (outputFile, "\tbe\t%s\n", label);
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbne\t_runtimeErrorWrongArraySize\n");
  fprintf (outputFile, "%s:\n", label);
}

void IRCheckArraySizeInt2 (VarDecl * p, int i) {
  linkIR (new CheckArraySizeInt2 (p, i));
}



//----------  ArrayIndex  ----------

void ArrayIndex::print () {
  fprintf (outputFile, "!   Move address of ");
  printANode (baseAddr);
  fprintf (outputFile, " [");
  printANode (indexVal);
  fprintf (outputFile, " ] into ");
  printANode (result);
  fprintf (outputFile, "\n");
  fprintf (outputFile, "!     make sure index expr is >= 0\n");
  getIntoReg4 (indexVal, "r2");
  fprintf (outputFile, "\tcmp\tr2,0\n");
  fprintf (outputFile, "\tbl\t_runtimeErrorBadArrayIndex\n");
  fprintf (outputFile, "!     make sure index expr is < array size\n");
  getIntoReg4 (baseAddr, "r1");
  fprintf (outputFile, "\tload\t[r1],r3\n");
  fprintf (outputFile, "\tcmp\tr3,0\n");
  fprintf (outputFile, "\tble\t_runtimeErrorUninitializedArray\n");
  fprintf (outputFile, "\tcmp\tr2,r3\n");
  overflowTest ();
  fprintf (outputFile, "\tbge\t_runtimeErrorBadArrayIndex\n");
  fprintf (outputFile, "!     compute address of array element\n");
  fprintf (outputFile, "\tset\t%d,r3\n", elementSize);
  fprintf (outputFile, "\tmul\tr2,r3,r2\n");
  fprintf (outputFile, "\tadd\tr2,4,r2\n");
  fprintf (outputFile, "\tadd\tr2,r1,r2\n");
  storeFromReg4 (result, "r2", "r1");
}

void IRArrayIndex (VarDecl * b, AstNode * i, VarDecl * r, int sz) {
  linkIR (new ArrayIndex (b, i, r, sz));
}



//----------  TestObjEq  ----------

void TestObjEq::print () {
  char * label;
  fprintf (outputFile, "!   TEST OBJECT EQUALITY: if * ");
  printANode (ptr1);
  fprintf (outputFile, " == * ");
  printANode (ptr2);
  fprintf (outputFile, " goto %s else goto %s\n", trueLabel, falseLabel);

  // Generate code to get pointers to the objects into r4 and r5...
  fprintf (outputFile, "!    get ptr1 into r4 and ptr2 into r5...\n");
  getIntoReg4 (ptr1, "r4");
  getIntoReg4 (ptr2, "r5");

  // Generate code to make sure the dispatch table ptrs are the same...
  fprintf (outputFile, "!    make sure the dispatch table ptrs are the same...\n");
  fprintf (outputFile, "\tload\t[r4],r1\n");
  fprintf (outputFile, "\tcmp\tr1,0\n");
  fprintf (outputFile, "\tbe\t_runtimeErrorUninitializedObject\n");
  fprintf (outputFile, "\tload\t[r5],r3\n");
  fprintf (outputFile, "\tcmp\tr3,0\n");
  fprintf (outputFile, "\tbe\t_runtimeErrorUninitializedObject\n");
  fprintf (outputFile, "\tcmp\tr1,r3\n");
  fprintf (outputFile, "\tbne\t%s\n", falseLabel);
  // Generate code to get the size in words into r3...
  fprintf (outputFile, "!    get size in words into r3...\n");
  fprintf (outputFile, "\tload\t[r3],r3\n");
  fprintf (outputFile, "\tload\t[r3+16],r3\n");
  fprintf (outputFile, "\tcmp\tr3,4\n");
  fprintf (outputFile, "\tbl\t_runtimeErrorBadObjectSize\n");
  fprintf (outputFile, "\tsrl\tr3,2,r3\n");
  // Generate code to test N bytes...
  //     r3 = counter of words
  //     r4 = ptr to dest -- ptr1
  //     r5 = ptr to source -- ptr2
  //     r1 = word from *ptr1
  //     r2 = word from *ptr2
  fprintf (outputFile, "!    do the testing in a loop...\n");
  label = newLabel ();
  fprintf (outputFile, "%s:\n", label);
  fprintf (outputFile, "\tload\t[r4],r1\n");
  fprintf (outputFile, "\tadd\tr4,4,r4\n");
  fprintf (outputFile, "\tload\t[r5],r2\n");
  fprintf (outputFile, "\tadd\tr5,4,r5\n");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  fprintf (outputFile, "\tbne\t%s\n", falseLabel);
  fprintf (outputFile, "\tsub\tr3,1,r3\n");
  fprintf (outputFile, "\tbne\t%s\n", label);
  fprintf (outputFile, "\tjmp\t%s\n", trueLabel);
  fprintf (outputFile, "!    done with test\n");
}

void IRTestObjEq (VarDecl * ptr1, VarDecl * ptr2, char * trueLabel, char * falseLabel) {
  linkIR (new TestObjEq (ptr1, ptr2, trueLabel, falseLabel));
}



//----------  ForTest  ----------

void ForTest::print () {
  fprintf (outputFile, "!   Perform the FOR-LOOP termination test\n");
  fprintf (outputFile, "!   if * ");
  printANode (ptr);
  fprintf (outputFile, " > ");
  printANode (stopVal);
  fprintf (outputFile, " then goto %s\t\t\n", exitLabel);
  getIntoReg4 (ptr, "r1");
  fprintf (outputFile, "\tload\t[r1],r1\n");
  getIntoReg4 (stopVal, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbg\t%s\n", exitLabel);
}

void IRForTest (VarDecl * pt, AstNode * st, char * lab) {
  linkIR (new ForTest (pt, st, lab));
}



//----------  ForTest2  ----------

void ForTest2::print () {
  fprintf (outputFile, "!   Perform the FOR-LOOP termination test\n");
  fprintf (outputFile, "!   if ");
  printANode (var);
  fprintf (outputFile, " > ");
  printANode (stopVal);
  fprintf (outputFile, " then goto %s\t\t\n", exitLabel);
  getIntoReg4 (var, "r1");
  getIntoReg4 (stopVal, "r2");
  fprintf (outputFile, "\tcmp\tr1,r2\n");
  overflowTest ();
  fprintf (outputFile, "\tbg\t%s\n", exitLabel);
}

void IRForTest2 (VarDecl * v, AstNode * st, char * lab) {
  linkIR (new ForTest2 (v, st, lab));
}



//----------  IncrVarDirect  ----------
//
// IRIncrVarDirect (VarDecl * dest, VarDecl * src, AstNode * incr, int incrInt, int wantOverflowTest)
//
// 
void IncrVarDirect::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (src);
  fprintf (outputFile, " + ");
  if (incr == NULL) {
    fprintf (outputFile, "%d\n", incrInt);
  } else {
    printANode (incr);
    fprintf (outputFile, "\n");
  }

  getIntoReg4 (src, "r1");
  if (incr == NULL) {
    if (within16Bits (incrInt)) {
      fprintf (outputFile, "\tadd\tr1,%d,r1\n", incrInt);
    } else {
      fprintf (outputFile, "\tset\t0x%08x,r2\t\t\t! decimal = %d\n", incrInt, incrInt);
      fprintf (outputFile, "\tadd\tr1,r2,r1\n");
    }
  } else {
    getIntoReg4 (incr, "r2");
    fprintf (outputFile, "\tadd\tr1,r2,r1\n");
  }
  if (wantOverflowTest) {
    overflowTest ();
  }
  storeFromReg4 (dest, "r1", "r2");
}

void IRIncrVarDirect (VarDecl * dest, VarDecl * src,
                      AstNode * incr, int incrInt, int wantOverflowTest) {
  linkIR (new IncrVarDirect (dest, src, incr, incrInt, wantOverflowTest));
}



//----------  IncrVarIndirect  ----------

void IncrVarIndirect::print () {
  getIntoReg4 (ptr, "r1");
  fprintf (outputFile, "\tload\t[r1],r3\n");
  if (incr == NULL) {
    if (within16Bits (incrInt)) {
      fprintf (outputFile, "\tadd\tr3,%d,r3\n", incrInt);
    } else {
      fprintf (outputFile, "\tset\t0x%08x,r2\t\t\t! decimal = %d\n", incrInt, incrInt);
      fprintf (outputFile, "\tadd\tr3,r2,r3\n");
    }
    overflowTest ();
  } else {
    getIntoReg4 (incr, "r2");
    fprintf (outputFile, "\tadd\tr3,r2,r3\n");
    overflowTest ();
  }
  fprintf (outputFile, "\tstore\tr3,[r1]\n");
}

void IRIncrVarIndirect (VarDecl * p, VarDecl * i, int i2) {
  linkIR (new IncrVarIndirect (p, i, i2));
}



//----------  MultiplyVarImmed  ----------

void MultiplyVarImmed::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = ");
  printANode (src);
  fprintf (outputFile, " * %d\n", ivalue);

  getIntoReg4 (src, "r1");
  if (within16Bits (ivalue)) {
    fprintf (outputFile, "\tmul\tr1,%d,r1\n", ivalue);
    overflowTest ();
  } else {
    fprintf (outputFile, "\tset\t0x%08x,r2\t\t\t! decimal = %d\n", ivalue, ivalue);
    fprintf (outputFile, "\tmul\tr1,r2,r1\n");
    overflowTest ();
  }
  storeFromReg4 (dest, "r1", "r2");
}

void IRMultiplyVarImmed (VarDecl * dest, VarDecl * src, int ivalue) {
  linkIR (new MultiplyVarImmed (dest, src, ivalue));
}



//----------  SwitchReg1  ----------

void SwitchReg1::print () {
  getIntoReg4 (expr, "r1");
}

void IRSwitchReg1 (AstNode * e) {
  linkIR (new SwitchReg1 (e));
}



//----------  SwitchTestReg1  ----------

void SwitchTestReg1::print () {
  if (within16Bits (ivalue)) {
    fprintf (outputFile, "\tcmp\tr1,%d\n", ivalue);
  } else {
    fprintf (outputFile, "\tset\t%d,r2\n", ivalue);
    fprintf (outputFile, "\tcmp\tr1,r2\n");
  }
  fprintf (outputFile, "\tbe\t%s\n", label);
}

void IRSwitchTestReg1 (int i, char * l) {
  linkIR (new SwitchTestReg1 (i, l));
}



//----------  SwitchDirect  ----------

void SwitchDirect::print () {
  char * okLabel = newLabel ();
  getIntoReg4 (expr, "r1");
  // Make sure the expr is not negative...
  fprintf (outputFile, "!   If ");
  printANode (expr);
  fprintf (outputFile, " is not within 16-bits goto default code\n");
  fprintf (outputFile, "\tsrl\tr1,15,r2\n");
  fprintf (outputFile, "\tcmp\tr2,0\n");
  fprintf (outputFile, "\tbe\t%s\n", okLabel);
  fprintf (outputFile, "\tset\t0x1ffff,r3\n");
  fprintf (outputFile, "\tcmp\tr2,r3\n");
  fprintf (outputFile, "\tbne\t%s\n", defaultLabel);
  fprintf (outputFile, "%s:\n", okLabel);
  // Make sure the expr is not < lowValue...
  fprintf (outputFile, "!   If ");
  printANode (expr);
  fprintf (outputFile, " is < %d (==smallestCaseValue) goto default code\n", lowValue, highValue);
  fprintf (outputFile, "\tcmp\tr1,%d\n", lowValue);
  // (Overflow cannot occur since r1 and lowValue are both within -32768..32767.)
  fprintf (outputFile, "\tbl\t%s\n", defaultLabel);
  // Make sure the expr is not > highValue...
  // (Overflow cannot occur since r1 and highValue are both within -32768..32767.)
  fprintf (outputFile, "!   If ");
  printANode (expr);
  fprintf (outputFile, " is > %d (==greatestCaseValue) goto default code\n", highValue);
  fprintf (outputFile, "\tcmp\tr1,%d\n", highValue);
  fprintf (outputFile, "\tbg\t%s\n", defaultLabel);
  fprintf (outputFile, "!   r1 = (r1-lowValue) * 4 + jumpTableAddr\n");
  fprintf (outputFile, "\tsub\tr1,%d,r1\n", lowValue);
  fprintf (outputFile, "\tsll\tr1,2,r1\n");
  fprintf (outputFile, "\tset\t%s,r2\n", directTable);
  fprintf (outputFile, "\tadd\tr1,r2,r1\n");
  fprintf (outputFile, "!   Jump indirect through the jump table\n");
  fprintf (outputFile, "\tjmp\tr1\n");
}

void IRSwitchDirect (AstNode * e, char * tab, char * def, int low, int high) {
  linkIR (new SwitchDirect (e, tab, def, low, high));
}



//----------  SwitchHashJump  ----------

void SwitchHashJump::print () {
  char * loopLabel = newLabel ();
  char * incrLabel = newLabel ();
  getIntoReg4 (expr, "r1");
  fprintf (outputFile, "!   r2 = hashValue (r1)\n");
  fprintf (outputFile, "\trem\tr1,%d,r2\n", tableSize);
  fprintf (outputFile, "!   LOOP: r5 = addr of hashTable [r2 * 8 + 4]\n");
  fprintf (outputFile, "%s:\n", loopLabel);
  fprintf (outputFile, "\tsll\tr2,3,r5\n");
  fprintf (outputFile, "\tadd\tr5,%s+4,r5\n", tableName);
  fprintf (outputFile, "!   r3 = the label for this table entry\n");
  fprintf (outputFile, "\tload\t[r5],r3\n");
  fprintf (outputFile, "!   If r3 == 0 goto default code\n");
  fprintf (outputFile, "\tcmp\tr3,0\n");
  fprintf (outputFile, "\tbe\t%s\n", defaultLabel);
  fprintf (outputFile, "!   r4 = the value for this table entry\n");
  fprintf (outputFile, "\tload\t[r5+-4],r4\n");
  fprintf (outputFile, "!   If r1 != r4 goto %s\n", incrLabel);
  fprintf (outputFile, "\tcmp\tr1,r4\n");
  fprintf (outputFile, "\tbne\t%s\n", incrLabel);
  fprintf (outputFile, "!   Jump indirect through r3\n");
  fprintf (outputFile, "\tjmp\tr3\n");
  fprintf (outputFile, "%s:\n", incrLabel);
  fprintf (outputFile, "!   r2 = r2+1\n");
  fprintf (outputFile, "\tadd\tr2,1,r2\n");
  fprintf (outputFile, "!   If r2 != tableSize goto LOOP\n");
  fprintf (outputFile, "\tcmp\tr2,%d\n", tableSize);
  fprintf (outputFile, "\tbne\t%s\n", loopLabel);
  fprintf (outputFile, "!   r2 = 0\n");
  fprintf (outputFile, "\tmov\t0,r2\n");
  fprintf (outputFile, "!   Goto LOOP\n");
  fprintf (outputFile, "\tjmp\t%s\n", loopLabel);
}

void IRSwitchHashJump (AstNode * e, char * tab, char * def, int sz) {
  linkIR (new SwitchHashJump (e, tab, def, sz));
}



//----------  Alloc  ----------

void Alloc::print () {
  fprintf (outputFile, "!   ");
  printANode (dest);
  fprintf (outputFile, " = alloc (");
  printANode (byteCount);
  fprintf (outputFile, ")\n");

  getIntoReg4 (byteCount, "r1");
  fprintf (outputFile, "\tcall\t_heapAlloc\n");  // Will not return NULL
  storeFromReg4 (dest, "r1", "r2");
}

void IRAlloc (VarDecl * dest, VarDecl * byteCount) {
  linkIR (new Alloc (dest, byteCount));
}



//----------  Free  ----------

void Free::print () {
  fprintf (outputFile, "!   Free (");
  printANode (ptr);
  fprintf (outputFile, ")\n");

  getIntoReg4 (ptr, "r1");
  fprintf (outputFile, "\tcall\t_heapFree\n");
}

void IRFree (AstNode * ptr) {
  linkIR (new Free (ptr));
}



//----------  SaveCatchStack  ----------

void SaveCatchStack::print () {
  fprintf (outputFile, "!   Save Catch Stack to ");
  printANode (temp);
  fprintf (outputFile, "\n");
  storeFromReg4 (temp, "r12", "r2");
}

void IRSaveCatchStack (VarDecl * temp) {
  linkIR (new SaveCatchStack (temp));
}



//----------  RestoreCatchStack  ----------

void RestoreCatchStack::print () {
  // This instruction does the following.  Since the code sequence is
  // is lengthy, most of the work is done in a routine in "runtime.s".
  //        load   <temp>, r4
  //        r1 := r12 (CatchStack top ptr)
  //        r12 := r4
  //    loop:
  //        if r1 == r4 goto done
  //        if r1 == NULL goto _runtimeErrorRestoreCatchStackError
  //        load   [r1], r2
  //        push   r2
  //        push   r4
  //        free   (r1)
  //        pop    r4
  //        pop    r1
  //        goto   loop
  //    done:

  fprintf (outputFile, "!   Restore Catch Stack from ");
  printANode (temp);
  fprintf (outputFile, "\n");
  getIntoReg4 (temp, "r4");
  fprintf (outputFile, "\tcall\t_RestoreCatchStack\n");    
}

void IRRestoreCatchStack (VarDecl * temp) {
  linkIR (new RestoreCatchStack (temp));
}



//----------  PushCatchRecord  ----------

void PushCatchRecord::print () {
  // Generate this code...
  //     r1 = alloc (28)
  //     r1->0 = r12 (catchStack top ptr)
  //     r1->4 = errorID
  //     r1->8 = catchCodeAddr
  //     r1->12 = r14
  //     r1->16 = r15
  //     r1->20 = ptr to source file name
  //     r1->24 = source line number
  //     r12 = r1
  //
  fprintf (outputFile, "!   Push Catch Record\n");
  fprintf (outputFile, "\tmov\t28,r1\n");
  fprintf (outputFile, "\tcall\t_heapAlloc\n");
  fprintf (outputFile, "\tstore\tr12,[r1]\n");
  fprintf (outputFile, "\tset\t%s,r2\n", cat->myDef->newName);
  fprintf (outputFile, "\tstore\tr2,[r1+4]\n");
  fprintf (outputFile, "\tset\t%s,r2\n", cat->label);
  fprintf (outputFile, "\tstore\tr2,[r1+8]\n");
  fprintf (outputFile, "\tstore\tr14,[r1+12]\n");
  fprintf (outputFile, "\tstore\tr15,[r1+16]\n");
  fprintf (outputFile, "\tset\t_sourceFileName,r2\n");
  fprintf (outputFile, "\tstore\tr2,[r1+20]\n");
  fprintf (outputFile, "\tset\t%d,r2\n", extractLineNumber (cat->tokn));
  fprintf (outputFile, "\tstore\tr2,[r1+24]\n");
  fprintf (outputFile, "\tmov\tr1,r12\n");
  // fprintf (outputFile, "\tdebug\n");
}

void IRPushCatchRecord (Catch * cat) {
  linkIR (new PushCatchRecord (cat));
}



//----------  Throw  ----------

void Throw::print () {
  fprintf (outputFile, "\tset\t%s,r4\n", errorDecl->newName);
  fprintf (outputFile, "\tjmp\t_PerformThrow\n");
}

void IRThrow (ErrorDecl * errorDecl) {
  linkIR (new Throw (errorDecl));
}



//----------  CopyCatchParm  ----------

void CopyCatchParm::print () {
  char * label;
  int sizeInBytes = parm->sizeInBytes;

  // fprintf (outputFile, "debug\n");

  fprintf (outputFile, "!   Copy catch arg to parm %s, offset=%d, throwSideOffset=%d, sizeInBytes=%d\n",
         parm->id->chars,
         parm->offset,
         parm->throwSideOffset,
         sizeInBytes);
  // printf ("Parm=%s     Offset=%d    ThrowSideOffset=%d    SizeInBytes=%d\n",
  //        parm->id->chars,
  //        parm->offset,
  //        parm->throwSideOffset,
  //        sizeInBytes);

  // At this point, r15 is the old SP, and will be used to acess the src args

  if (sizeInBytes == 1 || sizeInBytes == 4 || sizeInBytes == 8) {
    if (within16Bits (parm->throwSideOffset)) {
      if (sizeInBytes == 1) {
        fprintf (outputFile, "\tloadb\t[r15+%d], r1\n", parm->throwSideOffset);
      } else if (sizeInBytes == 4) {
        fprintf (outputFile, "\tload\t[r15+%d], r1\n", parm->throwSideOffset);
      } else if (sizeInBytes == 8) {
        fprintf (outputFile, "\tfload\t[r15+%d], f0\n", parm->throwSideOffset);
      }
    } else {
      // printf ("          throwSideOffset exceeds 16-bit size limitation: %s, %d\n",
      //         parm->id->chars, parm->throwSideOffset);
      if (sizeInBytes == 1) {
        fprintf (outputFile, "\tset\t%d, r11\n", parm->throwSideOffset);
        fprintf (outputFile, "\tloadb\t[r15+r11], r1\n");
      } else if (sizeInBytes == 4) {
        fprintf (outputFile, "\tset\t%d, r11\n", parm->throwSideOffset);
        fprintf (outputFile, "\tload\t[r15+r11], r1\n");
      } else if (sizeInBytes == 8) {
        fprintf (outputFile, "\tset\t%d, r11\n", parm->throwSideOffset);
        fprintf (outputFile, "\tfload\t[r15+r11], f0\n");
      }
    }
    if (within16Bits (parm->offset)) {
      if (sizeInBytes == 1) {
        fprintf (outputFile, "\tstoreb\tr1,[r14+%d]\n", parm->offset);
      } else if (sizeInBytes == 4) {
        fprintf (outputFile, "\tstore\tr1,[r14+%d]\n", parm->offset);
      } else if (sizeInBytes == 8) {
        fprintf (outputFile, "\tfstore\tf0,[r14+%d]\n", parm->offset);
      }
    } else {
      // printf ("          offset exceeds 16-bit size limitation: %s, %d\n",
      //         parm->id->chars, parm->offset);
      if (sizeInBytes == 1) {
        fprintf (outputFile, "\tset\t%d, r11\n", parm->offset);
        fprintf (outputFile, "\tstoreb\tr1,[r14+r11]\n");
      } else if (sizeInBytes == 4) {
        fprintf (outputFile, "\tset\t%d, r11\n", parm->offset);
        fprintf (outputFile, "\tstore\tr1,[r14+r11]\n");
      } else if (sizeInBytes == 8) {
        fprintf (outputFile, "\tset\t%d, r11\n", parm->offset);
        fprintf (outputFile, "\tfstore\tf0,[r14+r11]\n");
      }
    }
  } else if ((sizeInBytes < 0) || (sizeInBytes%4 != 0)) {
    printf ("sizeInBytes = %d\n", sizeInBytes);
    programLogicError ("In CopyCatchParm, problems with sizeInBytes");
  } else {
    // Generate code to move N bytes into the arg position...
    //     r3 = counter of words
    //     r4 = ptr to dest
    //     r5 = ptr to source
    //     r1 = word being moved
    label = newLabel ();
    if (within16Bits (sizeInBytes / 4)) {
      fprintf (outputFile, "\tmov\t%d,r3\n", sizeInBytes / 4);
    } else {
      fprintf (outputFile, "\tset\t%d,r3\n", sizeInBytes / 4);
      // printf ("In IRCopyCatchParm, sizeInBytes / 4 exceeds 16-bit size limitation: %s\n",
      //         parm->id->chars);
    }
    if (within16Bits (parm->offset)) {
      fprintf (outputFile, "\tadd\tr14,%d,r4\n", parm->offset);
    } else {
      fprintf (outputFile, "\tset\t%d,r11\n", parm->offset);
      fprintf (outputFile, "\tadd\tr14,r11,r4\n");
      // printf ("In IRCopyCatchParm, offset exceeds 16-bit size limitation: %s\n",
      //         parm->id->chars);
    }
    if (within16Bits (parm->throwSideOffset)) {
      fprintf (outputFile, "\tadd\tr15,%d,r5\n", parm->throwSideOffset);
    } else {
      fprintf (outputFile, "\tset\t%d,r11\n", parm->throwSideOffset);
      fprintf (outputFile, "\tadd\tr15,r11,r5\n");
      // printf ("In IRCopyCatchParm, throwSideOffset exceeds 16-bit size limitation: %s\n",
      //         parm->id->chars);
    }
    fprintf (outputFile, "%s:\n", label);
    fprintf (outputFile, "\tload\t[r5],r1\n");
    fprintf (outputFile, "\tadd\tr5,4,r5\n");
    fprintf (outputFile, "\tstore\tr1,[r4]\n");
    fprintf (outputFile, "\tadd\tr4,4,r4\n");
    fprintf (outputFile, "\tsub\tr3,1,r3\n");
    fprintf (outputFile, "\tbne\t%s\n", label);
  }



}

void IRCopyCatchParm (Parameter * parm) {
  linkIR (new CopyCatchParm (parm));
}



//----------  ResetStack  ----------

void ResetStack::print () {
  fprintf (outputFile, "\tmov\tr6,r15\t\t! Done copying args so reset stack\n");
}

void IRResetStack () {
  linkIR (new ResetStack ());
}



//----------  IsKindOf  ----------

void IsKindOf::print () {
  if (target) {
    fprintf (outputFile, "!   ");
    printANode (target);
    fprintf (outputFile, " = IsKindOf (");
    printANode (temp);
    fprintf (outputFile, ", %s)\n", descLab);
  } else {
    fprintf (outputFile, "!   if not isKindOf (");
    printANode (temp);
    fprintf (outputFile, ", %s) then goto %s\n", descLab, falseLabel);
  }
  getIntoReg4 (temp, "r1");
  fprintf (outputFile, "\tset\t%s,r2\n", descLab);
  fprintf (outputFile, "\tcall\t_IsKindOf\n");
  // If we have a target, move r1 into it...
  if (target) {
    storeFromReg1 (target, "r1", "r2");
  // Else, generate a jump if false, and fall thru otherwise...
  } else {
    fprintf (outputFile, "\tcmp\tr1,0\n");
    fprintf (outputFile, "\tbe\t%s\n", falseLabel);
  }
}

void IRIsKindOf (VarDecl * target, VarDecl * temp, char * descLab, char * falseLabel) {
  linkIR (new IsKindOf (target, temp, descLab, falseLabel));
}



//----------  IsInstanceOf  ----------

void IsInstanceOf::print () {
  char * retLabel, * return0;
  if (target) {
    fprintf (outputFile, "!   ");
    printANode (target);
    fprintf (outputFile, " = IsInstanceOf (");
    printANode (temp);
    fprintf (outputFile, ", %s)\n", descLab);
  } else {
    fprintf (outputFile, "!   if not IsInstanceOf (");
    printANode (temp);
    fprintf (outputFile, ", %s) then goto %s\n", descLab, falseLabel);
  }
  getIntoReg4 (temp, "r1");
  fprintf (outputFile, "\tcmp\tr1,0\n");
  fprintf (outputFile, "\tbe\t_runtimeErrorNullPointer\n");
  fprintf (outputFile, "\tset\t%s,r2\n", descLab);
  fprintf (outputFile, "\tload\t[r1],r1\n");
  fprintf (outputFile, "\tcmp\tr1,0\n");
  // If we have a target, generate code to move 0 or 1 into it...
  if (target) {
    return0 = newLabel ();
    retLabel = newLabel ();
    fprintf (outputFile, "\tbe\t%s\n", return0);
    fprintf (outputFile, "\tcmp\tr1,r2\n");
    fprintf (outputFile, "\tbne\t%s\n", return0);
    fprintf (outputFile, "\tmov\t1,r1\n");
    fprintf (outputFile, "\tjmp\t%s\n", retLabel);
    fprintf (outputFile, "%s:\n", return0);
    fprintf (outputFile, "\tmov\t0,r1\n");
    fprintf (outputFile, "%s:\n", retLabel);
    storeFromReg1 (target, "r1", "r2");
  // Else, generate a jump if false, and fall thru otherwise...
  } else {
    fprintf (outputFile, "\tbe\t%s\n", falseLabel);
    fprintf (outputFile, "\tcmp\tr1,r2\n");
    fprintf (outputFile, "\tbne\t%s\n", falseLabel);
  }
  
}

void IRIsInstanceOf (VarDecl * target, VarDecl * temp, char * descLab, char * falseLabel) {
  linkIR (new IsInstanceOf (target, temp, descLab, falseLabel));
}



//----------  ZeroMemory  ----------

void ZeroMemory::print () {
  char * label;
  int i;
  fprintf (outputFile, "!   ZeroMemory: ");
  if (targetVar) {
    printANode (targetVar);
  }
  if (targetPtr) {
    fprintf (outputFile, "*");
    printANode (targetPtr);
  }
  fprintf (outputFile, " = zeros  (sizeInBytes=%d)\n", sizeInBytes);
  if ((targetVar==NULL) == (targetPtr==NULL)) {
    programLogicError ("In ZeroMemory, targetVar and targetPtr are incorrect");
  }
  if (sizeInBytes == 1) {
    programLogicError ("In ZeroMemory, untested code - 1");
    if (targetVar) {
      storeFromReg1 (targetVar, "r0", "r1");
    } else if (targetPtr) {
      getIntoReg4 (targetPtr, "r1");
      fprintf (outputFile, "\tstoreb\tr0,[r1]\n");
    }
  } else if (sizeInBytes == 4) {
    programLogicError ("In ZeroMemory, untested code - 2");
    if (targetVar) {
      storeFromReg4 (targetVar, "r0", "r1");
    } else if (targetPtr) {
      getIntoReg4 (targetPtr, "r1");
      fprintf (outputFile, "\tstore\tr0,[r1]\n");
    }
  } else if ((sizeInBytes < 0) || (sizeInBytes%4 != 0)) {
    printf ("sizeInBytes = %d\n", sizeInBytes);
    programLogicError ("In ZeroMemory, problems with sizeInBytes");
  } else {

    if (targetVar) {
      getAddrOfVarIntoReg (targetVar, "r4");
    } else if (targetPtr) {
      getIntoReg4 (targetPtr, "r4");
    }

    if (sizeInBytes == 8) {
      // tested...
      fprintf (outputFile, "\tstore\tr0,[r4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+4]\n");
    } else if (sizeInBytes == 12) {
      // tested...
      fprintf (outputFile, "\tstore\tr0,[r4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+8]\n");
    } else if (sizeInBytes == 16) {
      // tested...
      fprintf (outputFile, "\tstore\tr0,[r4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+8]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+12]\n");
    } else if (sizeInBytes == 20) {
      // tested...
      fprintf (outputFile, "\tstore\tr0,[r4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+8]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+12]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+16]\n");
    } else if (sizeInBytes == 24) {
      // tested...
      fprintf (outputFile, "\tstore\tr0,[r4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+4]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+8]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+12]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+16]\n");
      fprintf (outputFile, "\tstore\tr0,[r4+20]\n");
    } else {
      // tested...
      // Generate code to move N bytes...
      //     r3 = counter of words
      //     r4 = ptr to dest
      label = newLabel ();
      if (within16Bits (sizeInBytes / 4)) {
        fprintf (outputFile, "\tmov\t%d,r3\n", sizeInBytes / 4);
      } else {
        fprintf (outputFile, "\tset\t0x%08x,r3\t\t\t! decimal = %d\n",
                 sizeInBytes / 4, sizeInBytes / 4);
      }
      fprintf (outputFile, "%s:\n", label);
      fprintf (outputFile, "\tstore\tr0,[r4]\n");
      fprintf (outputFile, "\tadd\tr4,4,r4\n");
      fprintf (outputFile, "\tsub\tr3,1,r3\n");
      fprintf (outputFile, "\tbne\t%s\n", label);
    }
  }
}

void IRZeroMemory (VarDecl * tarV, VarDecl * tarP, int sz) {
  linkIR (new ZeroMemory (tarV, tarP, sz));
}
