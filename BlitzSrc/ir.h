// ir.h  --  Classes of the Intermediate Representation
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
//   05/24/03 - Harry H. Porter III
//
// Modifcations by:
//   03/15/06 - Harry H. Porter III
//


#define OPComment  1
#define OPGoto     2
#define OPLabel    3
#define OPImport   4
#define OPExport   5
#define OPData     6
#define OPText     7
#define OPAlign    8
#define OPSkip     9
#define OPByte     10
#define OPWord     11
#define OPWord2    12
#define OPLoadAddr 13
#define OPDouble   14
#define OPCall     15
#define OPDebug    16
#define OPHalt     17
//   #define OPZeroLocal          18
#define OPSetLineNumber      19
#define OPFunctionEntry      20
#define OPFunctionReturn     21
#define OPCheckVersion       22
#define OPCallCheckVersion   23
#define OPEndCheckVersion    24
#define OPStartCheckVersion  25
#define OPVarDesc1           26
#define OPVarDesc2           27
#define OPFrameSize          28
#define OPAssign1            29
#define OPAssign4            30
#define OPAssign8            31
#define OPAscii              32
#define OPAscii2             33
#define OPIAdd               34
#define OPISub               35
#define OPIMul               36
#define OPIDiv               37
#define OPIRem               38
#define OPSll                39
#define OPSra                40
#define OPSrl                41
#define OPAnd                42
#define OPOr                 43
#define OPXor                44
#define OPNot                45
#define OPINeg               46
#define OPFAdd               47
#define OPFSub               48
#define OPFMul               49
#define OPFDiv               50
#define OPFNeg               51
#define OPItoF               52
#define OPFtoI               53
#define OPItoC               54
#define OPCtoI               55
#define OPPosInf             56
#define OPNegInf             57
#define OPNegZero            58
#define OPIntLTGoto          59
#define OPIntLEGoto          60
#define OPIntGTGoto          61
#define OPIntGEGoto          62
#define OPIntEQGoto          63
#define OPIntNEGoto          64
#define OPFloatLTGoto        65
#define OPFloatLEGoto        66
#define OPFloatGTGoto        67
#define OPFloatGEGoto        68
#define OPFloatEQGoto        69
#define OPFloatNEGoto        70
#define OPBoolTest           71
#define OPBXor               72
#define OPBEq                73
#define OPBNot               74
#define OPIntEqZero          75
#define OPPrepareArg         76
#define OPRetrieveResult     77
#define OPReturnResult       78
#define OPComment2           79
#define OPGoto2              80
#define OPMethodEntry        81
#define OPMethodReturn       82
#define OPSet                83
#define OPSend               84
#define OPLoadAddr2          85
#define OPBoolTest2          86
#define OPLoadSelfPtr        87
#define OPMove               88
#define OPBoolEqZeroIndirect 89
#define OPCallIndirect       90
#define OPLoadAddrWithIncr   91
#define OPCheckDPT           92
#define OPCheckDPT2          93
#define OPWord3              94
#define OPDynamicObjectMove  95
#define OPCopyArrays         96
#define OPCheckArraySizeInt  97
#define OPCheckArraySizeInt2 98
#define OPArrayIndex         99
#define OPTestObjEq         100
#define OPForTest           101
#define OPForTest2          102
#define OPIncrVarDirect     104
#define OPIncrVarIndirect   103
#define OPSwitchReg1        105
#define OPSwitchTestReg1    106
#define OPComment3          107
#define OPSwitchDirect      108
#define OPSwitchHashJump    109
#define OPMultiplyVarImmed  110
#define OPIntNeZero         111
#define OPAlloc             112
#define OPFree              114
#define OPIntLeZero         115
#define OPLoadAddrIndirect  116
#define OPLoadAddrWithIncr2 117
#define OPSaveCatchStack    118
#define OPRestoreCatchStack 119
#define OPPushCatchRecord   120
#define OPThrow             121
#define OPCopyCatchParm     122
#define OPResetStack        123
#define OPIsKindOf          124
#define OPIsInstanceOf      125
#define OPZeroMemory        126
#define OPAscii0            127


class IRLabel;


//----------  IR  ----------

class IR {
  public:
    IR * next;
    int op;

    IR (int oper) {
      op = oper;
      next = NULL;
    }
    ~IR () {}
    virtual void print ();


};



//----------  Comment  ----------

void IRComment (char * str);

class Comment : public IR {
  public:
    char * str;

    Comment (char * s) : IR (OPComment) {
      str = s;
    }
    ~Comment () {}
    virtual void print ();
};



//----------  Comment3  ----------

void IRComment3 (char * str, int i);

class Comment3 : public IR {
  public:
    char * str;
    int ivalue;

    Comment3 (char * s, int i) : IR (OPComment3) {
      str = s;
      ivalue = i;
    }
    ~Comment3 () {}
    virtual void print ();
};



//----------  Goto  ----------

void IRGoto (char * lab);

class Goto : public IR {
  public:
    char * label;

    Goto (char * lab) : IR (OPGoto) {
      label = lab;
    }
    ~Goto () {}
    virtual void print ();
};



//----------  Goto2  ----------

void IRGoto2 (char * lab, int offset, char * selector);

class Goto2 : public IR {
  public:
    char * label;
    int    offset;
    char * selector;

    Goto2 (char * lab, int i, char * sel) : IR (OPGoto2) {
      label = lab;
      offset = i;
      selector = sel;
    }
    ~Goto2 () {}
    virtual void print ();
};



//----------  Label  ----------

void IRLabel (char * lab);

class Label : public IR {
  public:
    char * label;

    Label (char * lab) : IR (OPLabel) {
      label = lab;
    }
    ~Label () {}
    virtual void print ();
};



//----------  Import  ----------

void IRImport (char * nam);

class Import : public IR {
  public:
    char * name;

    Import (char * nam) : IR (OPImport) {
      name = nam;
    }
    ~Import () {}
    virtual void print ();
};



//----------  Export  ----------

void IRExport (char * nam);

class Export : public IR {
  public:
    char * name;

    Export (char * nam) : IR (OPExport) {
      name = nam;
    }
    ~Export () {}
    virtual void print ();
};



//----------  Data  ----------

void IRData ();

class Data : public IR {
  public:

    Data () : IR (OPData) {
    }
    ~Data () {}
    virtual void print ();
};



//----------  Text  ----------

void IRText ();

class Text : public IR {
  public:

    Text () : IR (OPText) {
    }
    ~Text () {}
    virtual void print ();
};



//----------  Align  ----------

void IRAlign ();

class Align : public IR {
  public:

    Align () : IR (OPAlign) {
    }
    ~Align () {}
    virtual void print ();
};



//----------  Skip  ----------

void IRSkip (int i);

class Skip : public IR {
  public:
    int byteCount;

    Skip (int i) : IR (OPSkip) {
      byteCount = i;
    }
    ~Skip () {}
    virtual void print ();
};



//----------  Byte  ----------

void IRByte (int i);

class Byte : public IR {
  public:
    int byteValue;

    Byte (int i) : IR (OPByte) {
      byteValue = i;
    }
    ~Byte () {}
    virtual void print ();
};



//----------  Word  ----------

void IRWord (int i);

class Word : public IR {
  public:
    int wordValue;

    Word (int i) : IR (OPWord) {
      wordValue = i;
    }
    ~Word () {}
    virtual void print ();
};



//----------  Word2  ----------

void IRWord2 (char * s);

class Word2 : public IR {
  public:
    char * symbol;

    Word2 (char * s) : IR (OPWord2) {
      symbol = s;
    }
    ~Word2 () {}
    virtual void print ();
};



//----------  Word3  ----------

void IRWord3 (int i, char * s);

class Word3 : public IR {
  public:
    int    wordValue;
    char * comment;

    Word3 (int i, char * s) : IR (OPWord3) {
      wordValue = i;
      comment = s;
    }
    ~Word3 () {}
    virtual void print ();
};



//----------  LoadAddr  ----------

void IRLoadAddr (AstNode * dest, char * stringName);

class LoadAddr : public IR {
  public:
    AstNode * dest;         // Either local, global, or parameter
    char * stringName;      // A string Constant

    LoadAddr (AstNode * d, char * s) : IR (OPLoadAddr) {
      dest = d;
      stringName = s;
    }
    ~LoadAddr () {}
    virtual void print ();
};



//----------  LoadAddrIndirect  ----------

void IRLoadAddrIndirect (AstNode * dest, char * stringName);

class LoadAddrIndirect : public IR {
  public:
    AstNode * dest;         // Either local, global, or parameter
    char * stringName;      // A string Constant

    LoadAddrIndirect (AstNode * d, char * s) : IR (OPLoadAddrIndirect) {
      dest = d;
      stringName = s;
    }
    ~LoadAddrIndirect () {}
    virtual void print ();
};



//----------  LoadAddr2  ----------

void IRLoadAddr2 (VarDecl * dest, AstNode * src);

class LoadAddr2 : public IR {
  public:
    VarDecl * dest;         // Either local, global, or parameter
    AstNode * src;          // Either local, global, or parameter

    LoadAddr2 (VarDecl * d, AstNode * s) : IR (OPLoadAddr2) {
      dest = d;
      src = s;
    }
    ~LoadAddr2 () {}
    virtual void print ();
};



//----------  LoadAddrWithIncr  ----------

void IRLoadAddrWithIncr (VarDecl * dest, AstNode * src, FieldInit * f);

class LoadAddrWithIncr : public IR {
  public:
    VarDecl * dest;         // Either local, global, or parameter
    AstNode * src;          // Either local, global, or parameter
    FieldInit * fieldInit;

    LoadAddrWithIncr (VarDecl * d, AstNode * s, FieldInit * f) : IR (OPLoadAddrWithIncr) {
      dest = d;
      src = s;
      fieldInit = f;
    }
    ~LoadAddrWithIncr () {}
    virtual void print ();
};



//----------  LoadAddrWithIncr2  ----------

void IRLoadAddrWithIncr2 (VarDecl * dest, AstNode * src, FieldInit * f);

class LoadAddrWithIncr2 : public IR {
  public:
    VarDecl * dest;         // Either local, global, or parameter
    AstNode * src;          // Either local, global, or parameter
    FieldInit * fieldInit;

    LoadAddrWithIncr2 (VarDecl * d, AstNode * s, FieldInit * f) : IR (OPLoadAddrWithIncr2) {
      dest = d;
      src = s;
      fieldInit = f;
    }
    ~LoadAddrWithIncr2 () {}
    virtual void print ();
};



//----------  Double  ----------

void IRDouble (double d);

class Double : public IR {
  public:
    double doubleValue;

    Double (double d) : IR (OPDouble) {
      doubleValue = d;
    }
    ~Double () {}
    virtual void print ();
};



//----------  Call  ----------

void IRCall (char * n);

class Call : public IR {
  public:
    char * name;

    Call (char * n) : IR (OPCall) {
      name = n;
    }
    ~Call () {}
    virtual void print ();
};



//----------  CallIndirect  ----------

void IRCallIndirect (VarDecl * varDesc);

class CallIndirect : public IR {
  public:
    VarDecl * varDesc;

    CallIndirect (VarDecl * vd) : IR (OPCallIndirect) {
      varDesc = vd;
    }
    ~CallIndirect () {}
    virtual void print ();
};



//----------  Debug  ----------

void IRDebug ();

class Debug : public IR {
  public:

    Debug () : IR (OPDebug) {
    }
    ~Debug () {}
    virtual void print ();
};



//----------  Halt  ----------

void IRHalt ();

class Halt : public IR {
  public:

    Halt () : IR (OPHalt) {
    }
    ~Halt () {}
    virtual void print ();
};



//----------  SetLineNumber  ----------

void IRSetLineNumber (int i, char * s);

class SetLineNumber : public IR {
  public:
    int lineNumber;
    char * stmtCode;

    SetLineNumber (int i, char * s) : IR (OPSetLineNumber) {
      lineNumber = i;
      stmtCode = s;
    }
    ~SetLineNumber () {}
    virtual void print ();
};



//----------  FunctionEntry  ----------

void IRFunctionEntry (Function * f);

class FunctionEntry : public IR {
  public:
    Function * fun;

    FunctionEntry (Function * f) : IR (OPFunctionEntry) {
      fun = f;
    }
    ~FunctionEntry () {}
    virtual void print ();
};



//----------  FunctionReturn  ----------

void IRFunctionReturn (Function * f);

class FunctionReturn : public IR {
  public:
    Function * fun;

    FunctionReturn (Function * f) : IR (OPFunctionReturn) {
      fun = f;
    }
    ~FunctionReturn () {}
    virtual void print ();
};



//----------  MethodEntry  ----------

void IRMethodEntry (Method * m);

class MethodEntry : public IR {
  public:
    Method * meth;

    MethodEntry (Method * m) : IR (OPMethodEntry) {
      meth = m;
    }
    ~MethodEntry () {}
    virtual void print ();
};



//----------  MethodReturn  ----------

void IRMethodReturn (Method * m);

class MethodReturn : public IR {
  public:
    Method * meth;

    MethodReturn (Method * m) : IR (OPMethodReturn) {
      meth = m;
    }
    ~MethodReturn () {}
    virtual void print ();
};



//----------  CheckVersion  ----------

void IRCheckVersion (char * str, int i, char * lab1);

class CheckVersion : public IR {
  public:
    char * mySaniName;
    int myHashVal;
    char * label1;

    CheckVersion (char * str, int i, char * lab1) : IR (OPCheckVersion) {
      myHashVal = i;
      mySaniName = str;
      label1 = lab1;
    }
    ~CheckVersion () {}
    virtual void print ();
};



//----------  CallCheckVersion  ----------

void IRCallCheckVersion (char * str, int i, char * lab2);

class CallCheckVersion : public IR {
  public:
    char * theirSaniName;
    int theirHashVal;
    char * label2;

    CallCheckVersion (char * str, int i, char * lab2) : IR (OPCallCheckVersion) {
      theirHashVal = i;
      theirSaniName = str;
      label2 = lab2;
    }
    ~CallCheckVersion () {}
    virtual void print ();
};



//----------  EndCheckVersion  ----------

void IREndCheckVersion (char * lab2);

class EndCheckVersion : public IR {
  public:
    char * label2;

    EndCheckVersion (char * lab2) : IR (OPEndCheckVersion) {
      label2 = lab2;
    }
    ~EndCheckVersion () {}
    virtual void print ();
};



//----------  StartCheckVersion  ----------

void IRStartCheckVersion (char * str, int i, char * lab);

class StartCheckVersion : public IR {
  public:
    char * mySaniName;
    int myHashVal;
    char * continueLab;

    StartCheckVersion (char * str, int i, char * lab) : IR (OPStartCheckVersion) {
      myHashVal = i;
      mySaniName = str;
      continueLab = lab;
    }
    ~StartCheckVersion () {}
    virtual void print ();
};



//----------  VarDesc1  ----------

void IRVarDesc1 (char * lab, VarDecl * vd, int sz);

class VarDesc1 : public IR {
  public:
    char * label;
    VarDecl * varDecl;
    int sizeInBytes;

    VarDesc1 (char * lab, VarDecl * vd, int sz) : IR (OPVarDesc1) {
      label = lab;
      varDecl = vd;
      sizeInBytes = sz;
    }
    ~VarDesc1 () {}
    virtual void print ();
};



//----------  VarDesc2  ----------

void IRVarDesc2 (char * lab, char k, char * n);

class VarDesc2 : public IR {
  public:
    char * label;
    char kind;
    char * name;

    VarDesc2 (char * lab, char k, char * n) : IR (OPVarDesc2) {
      label = lab;
      kind = k;
      name = n;
    }
    ~VarDesc2 () {}
    virtual void print ();
};



//----------  FrameSize  ----------

void IRFrameSize (AstNode * n);

class FrameSize : public IR {
  public:
    AstNode * funOrMeth;

    FrameSize (AstNode * n) : IR (OPFrameSize) {
      funOrMeth = n;
    }
    ~FrameSize () {}
    virtual void print ();
};



//----------  Assign1  ----------

void IRAssign1 (AstNode * dest, AstNode * src);

class Assign1 : public IR {
  public:
    AstNode * dest;  // Either local, global, or parameter
    AstNode * src;   // Either local, global, parameter, char or bool

    Assign1 (AstNode * d, AstNode * s) : IR (OPAssign1) {
      dest = d;
      src = s;
    }
    ~Assign1 () {}
    virtual void print ();
};



//----------  Assign4  ----------

void IRAssign4 (AstNode * dest, AstNode * src);

class Assign4 : public IR {
  public:
    AstNode * dest;  // Either local, global, or parameter
    AstNode * src;   // Either local, global, parameter, int

    Assign4 (AstNode * d, AstNode * s) : IR (OPAssign4) {
      dest = d;
      src = s;
    }
    ~Assign4 () {}
    virtual void print ();
};



//----------  Assign8  ----------

void IRAssign8 (AstNode * dest, AstNode * src);

class Assign8 : public IR {
  public:
    AstNode * dest;  // Either local, global, or parameter
    AstNode * src;   // Either local, global, parameter, double

    Assign8 (AstNode * d, AstNode * s) : IR (OPAssign8) {
      dest = d;
      src = s;
    }
    ~Assign8 () {}
    virtual void print ();
};



//----------  Ascii  ----------

void IRAscii (char * s);

class Ascii : public IR {
  public:
    char * str;

    Ascii (char * s) : IR (OPAscii) {
      str = s;
    }
    ~Ascii () {}
    virtual void print ();
};



//----------  Ascii0  ----------

void IRAscii0 (char * s);

class Ascii0 : public IR {
  public:
    char * str;

    Ascii0 (char * s) : IR (OPAscii0) {
      str = s;
    }
    ~Ascii0 () {}
    virtual void print ();
};



//----------  Ascii2  ----------

void IRAscii2 (String * s);

class Ascii2 : public IR {
  public:
    String * str;

    Ascii2 (String * s) : IR (OPAscii2) {
      str = s;
    }
    ~Ascii2 () {}
    virtual void print ();
};



//----------  IAdd  ----------

void IRIAdd (AstNode * d, AstNode * a1, AstNode * a2);

class IAdd : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    IAdd (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPIAdd) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~IAdd () {}
    virtual void print ();
};



//----------  ISub  ----------

void IRISub (AstNode * d, AstNode * a1, AstNode * a2);

class ISub : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    ISub (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPISub) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~ISub () {}
    virtual void print ();
};



//----------  IMul  ----------

void IRIMul (AstNode * d, AstNode * a1, AstNode * a2);

class IMul : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    IMul (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPIMul) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~IMul () {}
    virtual void print ();
};



//----------  IDiv  ----------

void IRIDiv (AstNode * d, AstNode * a1, AstNode * a2);

class IDiv : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    IDiv (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPIDiv) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~IDiv () {}
    virtual void print ();
};



//----------  IRem  ----------

void IRIRem (AstNode * d, AstNode * a1, AstNode * a2);

class IRem : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    IRem (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPIRem) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~IRem () {}
    virtual void print ();
};



//----------  Sll  ----------

void IRSll (AstNode * d, AstNode * a1, AstNode * a2);

class Sll : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    Sll (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPSll) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~Sll () {}
    virtual void print ();
};



//----------  Sra  ----------

void IRSra (AstNode * d, AstNode * a1, AstNode * a2);

class Sra : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    Sra (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPSra) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~Sra () {}
    virtual void print ();
};



//----------  Srl  ----------

void IRSrl (AstNode * d, AstNode * a1, AstNode * a2);

class Srl : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    Srl (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPSrl) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~Srl () {}
    virtual void print ();
};



//----------  And  ----------

void IRAnd (AstNode * d, AstNode * a1, AstNode * a2);

class And : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    And (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPAnd) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~And () {}
    virtual void print ();
};



//----------  Or  ----------

void IROr (AstNode * d, AstNode * a1, AstNode * a2);

class Or : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    Or (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPOr) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~Or () {}
    virtual void print ();
};



//----------  Xor  ----------

void IRXor (AstNode * d, AstNode * a1, AstNode * a2);

class Xor : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst

    Xor (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPXor) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~Xor () {}
    virtual void print ();
};



//----------  INeg  ----------

void IRINeg (AstNode * d, AstNode * a);

class INeg : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, IntConst

    INeg (AstNode * d, AstNode * a) : IR (OPINeg) {
      dest = d;
      arg = a;
    }
    ~INeg () {}
    virtual void print ();
};



//----------  Not  ----------

void IRNot (AstNode * d, AstNode * a);

class Not : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, IntConst

    Not (AstNode * d, AstNode * a) : IR (OPNot) {
      dest = d;
      arg = a;
    }
    ~Not () {}
    virtual void print ();
};



//----------  FAdd  ----------

void IRFAdd (AstNode * d, AstNode * a1, AstNode * a2);

class FAdd : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst

    FAdd (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPFAdd) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~FAdd () {}
    virtual void print ();
};



//----------  FSub  ----------

void IRFSub (AstNode * d, AstNode * a1, AstNode * a2);

class FSub : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst

    FSub (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPFSub) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~FSub () {}
    virtual void print ();
};



//----------  FMul  ----------

void IRFMul (AstNode * d, AstNode * a1, AstNode * a2);

class FMul : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst

    FMul (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPFMul) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~FMul () {}
    virtual void print ();
};



//----------  FDiv  ----------

void IRFDiv (AstNode * d, AstNode * a1, AstNode * a2);

class FDiv : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst

    FDiv (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPFDiv) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~FDiv () {}
    virtual void print ();
};



//----------  FNeg  ----------

void IRFNeg (AstNode * d, AstNode * a);

class FNeg : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, DoubleConst

    FNeg (AstNode * d, AstNode * a) : IR (OPFNeg) {
      dest = d;
      arg = a;
    }
    ~FNeg () {}
    virtual void print ();
};



//----------  ItoF  ----------

void IRItoF (AstNode * d, AstNode * a);

class ItoF : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, IntConst

    ItoF (AstNode * d, AstNode * a) : IR (OPItoF) {
      dest = d;
      arg = a;
    }
    ~ItoF () {}
    virtual void print ();
};



//----------  FtoI  ----------

void IRFtoI (AstNode * d, AstNode * a);

class FtoI : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, DoubleConst

    FtoI (AstNode * d, AstNode * a) : IR (OPFtoI) {
      dest = d;
      arg = a;
    }
    ~FtoI () {}
    virtual void print ();
};



//----------  ItoC  ----------

void IRItoC (AstNode * d, AstNode * a);

class ItoC : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, IntConst

    ItoC (AstNode * d, AstNode * a) : IR (OPItoC) {
      dest = d;
      arg = a;
    }
    ~ItoC () {}
    virtual void print ();
};



//----------  CtoI  ----------

void IRCtoI (AstNode * d, AstNode * a);

class CtoI : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;            // Local, Global, Parm, CharConst, BoolConst

    CtoI (AstNode * d, AstNode * a) : IR (OPCtoI) {
      dest = d;
      arg = a;
    }
    ~CtoI () {}
    virtual void print ();
};



//----------  PosInf  ----------

void IRPosInf (AstNode * d);

class PosInf : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm

    PosInf (AstNode * d) : IR (OPPosInf) {
      dest = d;
    }
    ~PosInf () {}
    virtual void print ();
};



//----------  NegInf  ----------

void IRNegInf (AstNode * d);

class NegInf : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm

    NegInf (AstNode * d) : IR (OPNegInf) {
      dest = d;
    }
    ~NegInf () {}
    virtual void print ();
};



//----------  NegZero  ----------

void IRNegZero (AstNode * d);

class NegZero : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm

    NegZero (AstNode * d) : IR (OPNegZero) {
      dest = d;
    }
    ~NegZero () {}
    virtual void print ();
};



//----------  IntLTGoto  ----------

void IRIntLTGoto (AstNode * a1, AstNode * a2, char * lab);

class IntLTGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst
    char * label;

    IntLTGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPIntLTGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~IntLTGoto () {}
    virtual void print ();
};



//----------  IntLEGoto  ----------

void IRIntLEGoto (AstNode * a1, AstNode * a2, char * lab);

class IntLEGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst
    char * label;

    IntLEGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPIntLEGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~IntLEGoto () {}
    virtual void print ();
};



//----------  IntGTGoto  ----------

void IRIntGTGoto (AstNode * a1, AstNode * a2, char * lab);

class IntGTGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst
    char * label;

    IntGTGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPIntGTGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~IntGTGoto () {}
    virtual void print ();
};



//----------  IntGEGoto  ----------

void IRIntGEGoto (AstNode * a1, AstNode * a2, char * lab);

class IntGEGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst
    char * label;

    IntGEGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPIntGEGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~IntGEGoto () {}
    virtual void print ();
};



//----------  IntEQGoto  ----------

void IRIntEQGoto (AstNode * a1, AstNode * a2, char * lab);

class IntEQGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst
    char * label;

    IntEQGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPIntEQGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~IntEQGoto () {}
    virtual void print ();
};



//----------  IntNEGoto  ----------

void IRIntNEGoto (AstNode * a1, AstNode * a2, char * lab);

class IntNEGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, IntConst
    AstNode * arg2;           // Local, Global, Parm, IntConst
    char * label;

    IntNEGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPIntNEGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~IntNEGoto () {}
    virtual void print ();
};



//----------  FloatLTGoto  ----------

void IRFloatLTGoto (AstNode * a1, AstNode * a2, char * lab);

class FloatLTGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst
    char * label;

    FloatLTGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPFloatLTGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~FloatLTGoto () {}
    virtual void print ();
};



//----------  FloatLEGoto  ----------

void IRFloatLEGoto (AstNode * a1, AstNode * a2, char * lab);

class FloatLEGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst
    char * label;

    FloatLEGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPFloatLEGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~FloatLEGoto () {}
    virtual void print ();
};



//----------  FloatGTGoto  ----------

void IRFloatGTGoto (AstNode * a1, AstNode * a2, char * lab);

class FloatGTGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst
    char * label;

    FloatGTGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPFloatGTGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~FloatGTGoto () {}
    virtual void print ();
};



//----------  FloatGEGoto  ----------

void IRFloatGEGoto (AstNode * a1, AstNode * a2, char * lab);

class FloatGEGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst
    char * label;

    FloatGEGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPFloatGEGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~FloatGEGoto () {}
    virtual void print ();
};



//----------  FloatEQGoto  ----------

void IRFloatEQGoto (AstNode * a1, AstNode * a2, char * lab);

class FloatEQGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst
    char * label;

    FloatEQGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPFloatEQGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~FloatEQGoto () {}
    virtual void print ();
};



//----------  FloatNEGoto  ----------

void IRFloatNEGoto (AstNode * a1, AstNode * a2, char * lab);

class FloatNEGoto : public IR {
  public:
    AstNode * arg1;           // Local, Global, Parm, DoubleConst
    AstNode * arg2;           // Local, Global, Parm, DoubleConst
    char * label;

    FloatNEGoto (AstNode * a1, AstNode * a2, char * lab) : IR (OPFloatNEGoto) {
      arg1 = a1;
      arg2 = a2;
      label = lab;
    }
    ~FloatNEGoto () {}
    virtual void print ();
};



//----------  BoolTest  ----------

void IRBoolTest (AstNode * a, char * lab1, char * lab2);

class BoolTest : public IR {
  public:
    AstNode * arg;           // Local, Global, Parm, BoolConst
    char * trueLabel;
    char * falseLabel;

    BoolTest (AstNode * a, char * lab1, char * lab2) : IR (OPBoolTest) {
      arg = a;
      trueLabel = lab1;
      falseLabel = lab2;
    }
    ~BoolTest () {}
    virtual void print ();
};



//----------  BoolTest2  ----------

void IRBoolTest2 (char * lab1, char * lab2);

class BoolTest2 : public IR {
  public:
    char * trueLabel;
    char * falseLabel;

    BoolTest2 (char * lab1, char * lab2) : IR (OPBoolTest2) {
      trueLabel = lab1;
      falseLabel = lab2;
    }
    ~BoolTest2 () {}
    virtual void print ();
};



//----------  BXor  ----------

void IRBXor (AstNode * d, AstNode * a1, AstNode * a2);

class BXor : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, BoolConst
    AstNode * arg2;           // Local, Global, Parm, BoolConst

    BXor (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPBXor) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~BXor () {}
    virtual void print ();
};



//----------  BEq  ----------

void IRBEq (AstNode * d, AstNode * a1, AstNode * a2);

class BEq : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg1;           // Local, Global, Parm, BoolConst
    AstNode * arg2;           // Local, Global, Parm, BoolConst

    BEq (AstNode * d, AstNode * a1, AstNode * a2) : IR (OPBEq) {
      dest = d;
      arg1 = a1;
      arg2 = a2;
    }
    ~BEq () {}
    virtual void print ();
};



//----------  BNot  ----------

void IRBNot (AstNode * d, AstNode * a);

class BNot : public IR {
  public:
    AstNode * dest;           // Local, Global, Parm
    AstNode * arg;           // Local, Global, Parm, BoolConst

    BNot (AstNode * d, AstNode * a) : IR (OPBNot) {
      dest = d;
      arg = a;
    }
    ~BNot () {}
    virtual void print ();
};



//----------  IntEqZero  ----------

void IRIntEqZero (AstNode * a, char * lab);

class IntEqZero : public IR {
  public:
    AstNode * arg;           // Local, Global, Parm, ClassField, IntConst
    char * label;

    IntEqZero (AstNode * a, char * lab) : IR (OPIntEqZero) {
      arg = a;
      label = lab;
    }
    ~IntEqZero () {}
    virtual void print ();
};



//----------  IntNeZero  ----------

void IRIntNeZero (AstNode * a, char * lab);

class IntNeZero : public IR {
  public:
    AstNode * arg;           // Local, Global, Parm, ClassField, IntConst
    char * label;

    IntNeZero (AstNode * a, char * lab) : IR (OPIntNeZero) {
      arg = a;
      label = lab;
    }
    ~IntNeZero () {}
    virtual void print ();
};



//----------  IntLeZero  ----------

void IRIntLeZero (AstNode * a, char * lab);

class IntLeZero : public IR {
  public:
    AstNode * arg;           // Local, Global, Parm, ClassField, IntConst
    char * label;

    IntLeZero (AstNode * a, char * lab) : IR (OPIntLeZero) {
      arg = a;
      label = lab;
    }
    ~IntLeZero () {}
    virtual void print ();
};



//----------  BoolEqZeroIndirect  ----------

void IRBoolEqZeroIndirect (VarDecl * a, char * lab);

class BoolEqZeroIndirect : public IR {
  public:
    VarDecl * arg;           // Local, Global, Parm, ClassField -- this will be a ptr
    char * label;

    BoolEqZeroIndirect (VarDecl * a, char * lab) : IR (OPBoolEqZeroIndirect) {
      arg = a;
      label = lab;
    }
    ~BoolEqZeroIndirect () {}
    virtual void print ();
};



//----------  PrepareArg  ----------

void IRPrepareArg (int off, AstNode * tname, int size);

class PrepareArg : public IR {
  public:
    int offset;
    AstNode * tempName;
    int sizeInBytes;

    PrepareArg (int off, AstNode * tname, int size) : IR (OPPrepareArg) {
      offset = off;
      tempName = tname;
      sizeInBytes = size;
    }
    ~PrepareArg () {}
    virtual void print ();
};



//----------  RetrieveResult  ----------

void IRRetrieveResult (VarDecl * tname, int size);

class RetrieveResult : public IR {
  public:
    VarDecl * targetName;
    int sizeInBytes;

    RetrieveResult (VarDecl * tname, int size) : IR (OPRetrieveResult) {
      targetName = tname;
      sizeInBytes = size;
    }
    ~RetrieveResult () {}
    virtual void print ();
};



//----------  ReturnResult  ----------

void IRReturnResult (AstNode * tname, int size);

class ReturnResult : public IR {
  public:
    AstNode * tempName;
    int sizeInBytes;

    ReturnResult (AstNode * tname, int size) : IR (OPReturnResult) {
      tempName = tname;
      sizeInBytes = size;
    }
    ~ReturnResult () {}
    virtual void print ();
};



//----------  Comment2  ----------

void IRComment2 (char * str1, char * str2, char * str3);

class Comment2 : public IR {
  public:
    char * str1;
    char * str2;
    char * str3;

    Comment2 (char * s1, char * s2, char * s3) : IR (OPComment2) {
      str1 = s1;
      str2 = s2;
      str3 = s3;
    }
    ~Comment2 () {}
    virtual void print ();
};



//----------  Set  ----------

void IRSet (VarDecl * v, int i);

class Set : public IR {
  public:
    VarDecl * varDecl;
    int initValue;

    Set (VarDecl * v, int i) : IR (OPSet) {
      varDecl = v;
      initValue = i;
    }
    ~Set () {}
    virtual void print ();
};



//----------  Send  ----------

void IRSend (VarDecl * r, MethodProto * m);

class Send : public IR {
  public:
    VarDecl *     recvr;
    MethodProto * methProto;

    Send (VarDecl * r, MethodProto * m) : IR (OPSend) {
      recvr = r;
      methProto = m;
    }
    ~Send () {}
    virtual void print ();
};



//----------  LoadSelfPtr  ----------

void IRLoadSelfPtr (VarDecl * tname);

class LoadSelfPtr : public IR {
  public:
    VarDecl * targetName;

    LoadSelfPtr (VarDecl * tname) : IR (OPLoadSelfPtr) {
      targetName = tname;
    }
    ~LoadSelfPtr () {}
    virtual void print ();
};



//----------  Move  ----------

void IRMove (VarDecl * tarV, VarDecl * tarP, AstNode * srcV, VarDecl * srcP, int sz);

class Move : public IR {
  public:
    VarDecl * targetVar;
    VarDecl * targetPtr;
    AstNode * srcVar;  // INT_, DOUBLE_, BOOL_, CHAR_, LOCAL, PARM, GLOBAL, CLASS_FIELD
    VarDecl * srcPtr;
    int sizeInBytes;

    Move (VarDecl * tarV, VarDecl * tarP, AstNode * srcV, VarDecl * srcP, int sz) : IR (OPMove) {
      targetVar = tarV;
      targetPtr = tarP;
      srcVar = srcV;
      srcPtr = srcP;
      sizeInBytes = sz;
    }
    ~Move () {}
    virtual void print ();
};



//----------  DynamicObjectMove  ----------

void IRDynamicObjectMove (VarDecl * tar, VarDecl * src);

class DynamicObjectMove : public IR {
  public:
    VarDecl * targetPtr;
    VarDecl * srcPtr;

    DynamicObjectMove (VarDecl * tar, VarDecl * src) : IR (OPDynamicObjectMove) {
      targetPtr = tar;
      srcPtr = src;
    }
    ~DynamicObjectMove () {}
    virtual void print ();
};



//   //----------  ZeroLocal  ----------
//   
//   void IRZeroLocal (Local * loc);
//   
//   class ZeroLocal : public IR {
//     public:
//       Local * local;
//   
//       ZeroLocal (Local * loc) : IR (OPZeroLocal) {
//         local = loc;
//       }
//       ~ZeroLocal () {}
//       virtual void print ();
//   };



//----------  IncrVarDirect  ----------

void IRIncrVarDirect (VarDecl * d, VarDecl * s, AstNode * i, int i2, int want);

class IncrVarDirect : public IR {
  public:
    VarDecl * dest;
    VarDecl * src;
    AstNode * incr;     // or NULL; local,parm,global,classfield, or intConst
    int       incrInt;  // This is a 16-bit number if incr is NULL
    int       wantOverflowTest;

    IncrVarDirect (VarDecl * d, VarDecl * s, AstNode * i, int i2, int want) : IR (OPIncrVarDirect) {
      dest = d;
      src = s;
      incr = i;
      incrInt = i2;
      wantOverflowTest = want;
    }
    ~IncrVarDirect () {}
    virtual void print ();
};



//----------  IncrVarIndirect  ----------

void IRIncrVarIndirect (VarDecl * p, VarDecl * i, int i2);

class IncrVarIndirect : public IR {
  public:
    VarDecl * ptr;
    VarDecl * incr;
    int       incrInt;

    IncrVarIndirect (VarDecl * p, VarDecl * i, int i2) : IR (OPIncrVarIndirect) {
      ptr = p;
      incr = i;
      incrInt = i2;
    }
    ~IncrVarIndirect () {}
    virtual void print ();
};



//----------  MultiplyVarImmed  ----------

void IRMultiplyVarImmed (VarDecl * d, VarDecl * s, int i);

class MultiplyVarImmed : public IR {
  public:
    VarDecl * dest;
    VarDecl * src;
    int       ivalue;

    MultiplyVarImmed (VarDecl * d, VarDecl * s, int i) : IR (OPMultiplyVarImmed) {
      dest = d;
      src = s;
      ivalue = i;
    }
    ~MultiplyVarImmed () {}
    virtual void print ();
};



//----------  CheckDPT  ----------

void IRCheckDPT (VarDecl * v, ClassDef * cl);

class CheckDPT : public IR {
  public:
    VarDecl * var;
    ClassDef * classDef;

    CheckDPT (VarDecl * v, ClassDef * cl) : IR (OPCheckDPT) {
      var = v;
      classDef = cl;
    }
    ~CheckDPT () {}
    virtual void print ();
};



//----------  CheckDPT2  ----------

void IRCheckDPT2 (VarDecl * v, ClassDef * cl);

class CheckDPT2 : public IR {
  public:
    VarDecl * var;
    ClassDef * classDef;

    CheckDPT2 (VarDecl * v, ClassDef * cl) : IR (OPCheckDPT2) {
      var = v;
      classDef = cl;
    }
    ~CheckDPT2 () {}
    virtual void print ();
};



//----------  CopyArrays  ----------

void IRCopyArrays (VarDecl * tar, VarDecl * src, int i);

class CopyArrays : public IR {
  public:
    VarDecl * targetPtr;
    VarDecl * srcPtr;
    int       elementSize;

    CopyArrays (VarDecl * tar, VarDecl * src, int i) : IR (OPCopyArrays) {
      targetPtr = tar;
      srcPtr = src;
      elementSize = i;
    }
    ~CopyArrays () {}
    virtual void print ();
};



//----------  CheckArraySizeInt  ----------

void IRCheckArraySizeInt (VarDecl * p, int i);

class CheckArraySizeInt : public IR {
  public:
    VarDecl * ptr;
    int       numberOfElements;

    CheckArraySizeInt (VarDecl * p, int i) : IR (OPCheckArraySizeInt) {
      ptr = p;
      numberOfElements = i;
    }
    ~CheckArraySizeInt () {}
    virtual void print ();
};



//----------  CheckArraySizeInt2  ----------

void IRCheckArraySizeInt2 (VarDecl * p, int i);

class CheckArraySizeInt2 : public IR {
  public:
    VarDecl * ptr;
    int       numberOfElements;

    CheckArraySizeInt2 (VarDecl * p, int i) : IR (OPCheckArraySizeInt2) {
      ptr = p;
      numberOfElements = i;
    }
    ~CheckArraySizeInt2 () {}
    virtual void print ();
};



//----------  ArrayIndex  ----------

void IRArrayIndex (VarDecl * b, AstNode * i, VarDecl * r, int sz);

class ArrayIndex : public IR {
  public:
    VarDecl * baseAddr;
    AstNode * indexVal;
    VarDecl * result;
    int       elementSize;

    ArrayIndex (VarDecl * b, AstNode * i, VarDecl * r, int sz) : IR (OPArrayIndex) {
      baseAddr = b;
      indexVal = i;
      result = r;
      elementSize = sz;
    }
    ~ArrayIndex () {}
    virtual void print ();
};



//----------  TestObjEq  ----------

void IRTestObjEq (VarDecl * p1, VarDecl * p2, char * tlab, char * flab);

class TestObjEq : public IR {
  public:
    VarDecl * ptr1;
    VarDecl * ptr2;
    char *    trueLabel;
    char *    falseLabel;

    TestObjEq (VarDecl * p1, VarDecl * p2, char * tlab, char * flab) : IR (OPTestObjEq) {
    ptr1 = p1;
    ptr2 = p2;
    trueLabel = tlab;
    falseLabel = flab;
    }
    ~TestObjEq () {}
    virtual void print ();
};



//----------  ForTest  ----------

void IRForTest (VarDecl * pt, AstNode * st, char * lab);

class ForTest : public IR {
  public:
    VarDecl * ptr;           // Local, Global, Parm, ClassField
    AstNode * stopVal;       // Local, Global, Parm, ClassField, IntConst
    char * exitLabel;

    ForTest (VarDecl * pt, AstNode * st, char * lab) : IR (OPForTest) {
      ptr = pt;
      stopVal = st;
      exitLabel = lab;
    }
    ~ForTest () {}
    virtual void print ();
};



//----------  ForTest2  ----------

void IRForTest2 (VarDecl * v, AstNode * st, char * lab);

class ForTest2 : public IR {
  public:
    VarDecl * var;           // Local or Parm
    AstNode * stopVal;       // Local, Global, Parm, ClassField, IntConst
    char * exitLabel;

    ForTest2 (VarDecl * v, AstNode * st, char * lab) : IR (OPForTest2) {
      var = v;
      stopVal = st;
      exitLabel = lab;
    }
    ~ForTest2 () {}
    virtual void print ();
};



//----------  SwitchReg1  ----------

void IRSwitchReg1 (AstNode * e);

class SwitchReg1 : public IR {
  public:
    AstNode * expr;

    SwitchReg1 (AstNode * e) : IR (OPSwitchReg1) {
      expr = e;
    }
    ~SwitchReg1 () {}
    virtual void print ();
};



//----------  SwitchTestReg1  ----------

void IRSwitchTestReg1 (int i, char * l);

class SwitchTestReg1 : public IR {
  public:
    int ivalue;
    char * label;

    SwitchTestReg1 (int i, char * l) : IR (OPSwitchTestReg1) {
      ivalue = i;
      label = l;
    }
    ~SwitchTestReg1 () {}
    virtual void print ();
};



//----------  SwitchDirect  ----------

void IRSwitchDirect (AstNode * e, char * tab, char * def, int low, int high);

class SwitchDirect : public IR {
  public:
    AstNode * expr;
    char * directTable;
    char * defaultLabel;
    int lowValue;
    int highValue;

    SwitchDirect (AstNode * e, char * tab, char * def, int low, int high) : IR (OPSwitchDirect) {
      expr = e;
      directTable = tab;
      defaultLabel = def;
      lowValue = low;
      highValue = high;
    }
    ~SwitchDirect () {}
    virtual void print ();
};




//----------  SwitchHashJump  ----------

void IRSwitchHashJump (AstNode * e, char * tab, char * def, int sz);

class SwitchHashJump : public IR {
  public:
    AstNode * expr;
    char * tableName;
    char * defaultLabel;
    int tableSize;

    SwitchHashJump (AstNode * e, char * tab, char * def, int sz) : IR (OPSwitchHashJump) {
      expr = e;
      tableName = tab;
      defaultLabel = def;
      tableSize = sz;
    }
    ~SwitchHashJump () {}
    virtual void print ();
};



//----------  Alloc  ----------

void IRAlloc (VarDecl * d, VarDecl * b);

class Alloc : public IR {
  public:
    VarDecl * dest;
    VarDecl * byteCount;

    Alloc (VarDecl * d, VarDecl * b) : IR (OPAlloc) {
      dest = d;
      byteCount = b;
    }
    ~Alloc () {}
    virtual void print ();
};



//----------  Free  ----------

void IRFree (AstNode * p);

class Free : public IR {
  public:
    AstNode * ptr;

    Free (AstNode * p) : IR (OPFree) {
      ptr = p;
    }
    ~Free () {}
    virtual void print ();
};



//----------  SaveCatchStack  ----------

void IRSaveCatchStack (VarDecl * t);

class SaveCatchStack : public IR {
  public:
    VarDecl * temp;

    SaveCatchStack (VarDecl * t) : IR (OPSaveCatchStack) {
      temp = t;
    }
    ~SaveCatchStack () {}
    virtual void print ();
};



//----------  RestoreCatchStack  ----------

void IRRestoreCatchStack (VarDecl * t);

class RestoreCatchStack : public IR {
  public:
    VarDecl * temp;

    RestoreCatchStack (VarDecl * t) : IR (OPRestoreCatchStack) {
      temp = t;
    }
    ~RestoreCatchStack () {}
    virtual void print ();
};



//----------  PushCatchRecord  ----------

void IRPushCatchRecord (Catch * cat);

class PushCatchRecord : public IR {
  public:
    Catch * cat;

    PushCatchRecord (Catch * c) : IR (OPPushCatchRecord) {
      cat = c;
    }
    ~PushCatchRecord () {}
    virtual void print ();
};



//----------  Throw  ----------

void IRThrow (ErrorDecl * errorDecl);

class Throw : public IR {
  public:
    ErrorDecl * errorDecl;

    Throw (ErrorDecl * e) : IR (OPThrow) {
      errorDecl = e;
    }
    ~Throw () {}
    virtual void print ();
};



//----------  CopyCatchParm  ----------

void IRCopyCatchParm (Parameter * parm);

class CopyCatchParm : public IR {
  public:
    Parameter * parm;

    CopyCatchParm (Parameter * p) : IR (OPCopyCatchParm) {
      parm = p;
    }
    ~CopyCatchParm () {}
    virtual void print ();
};



//----------  ResetStack  ----------

void IRResetStack ();

class ResetStack : public IR {
  public:

    ResetStack () : IR (OPResetStack) {
    }
    ~ResetStack () {}
    virtual void print ();
};



//----------  IsKindOf  ----------

void IRIsKindOf (VarDecl * target, VarDecl * temp, char * descLab, char * falseLabel);

class IsKindOf : public IR {
  public:
    VarDecl * target;
    VarDecl * temp;
    char * falseLabel;
    char * descLab;

    IsKindOf (VarDecl * tar, VarDecl * tmp, char * dslab, char * lab) : IR (OPIsKindOf) {
      target = tar;
      temp = tmp;
      descLab = dslab;
      falseLabel = lab;
    }
    ~IsKindOf () {}
    virtual void print ();
};



//----------  IsInstanceOf  ----------

void IRIsInstanceOf (VarDecl * target, VarDecl * temp, char * descLab, char * falseLabel);

class IsInstanceOf : public IR {
  public:
    VarDecl * target;
    VarDecl * temp;
    char * falseLabel;
    char * descLab;

    IsInstanceOf (VarDecl * tar, VarDecl * tmp, char * dslab, char * lab) : IR (OPIsInstanceOf) {
      target = tar;
      temp = tmp;
      descLab = dslab;
      falseLabel = lab;
    }
    ~IsInstanceOf () {}
    virtual void print ();
};



//----------  ZeroMemory  ----------

void IRZeroMemory (VarDecl * tarV, VarDecl * tarP, int sz);

class ZeroMemory : public IR {
  public:
    VarDecl * targetVar;
    VarDecl * targetPtr;
    int sizeInBytes;

    ZeroMemory (VarDecl * tarV, VarDecl * tarP, int sz) : IR (OPZeroMemory) {
      targetVar = tarV;
      targetPtr = tarP;
      sizeInBytes = sz;
    }
    ~ZeroMemory () {}
    virtual void print ();
};
