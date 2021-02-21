// ast.h  --  Classes of the Abstract Syntax Tree (AST)
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
//   06/15/02 - Harry H. Porter III
//
// Modifcations by:
//   03/15/06 - Harry H. Porter III
//



//class AstNode;              // ...

  class Header;                // HEADER
  class Code;                  // CODE
  class Uses;                  // USES
  class Renaming;              // RENAMING
  class Abstract;              // ...
    class Interface;              // INTERFACE
    class ClassDef;               // CLASS_DEF
  class Behavior;              // BEHAVIOR
  class TypeDef;               // TYPE_DEF
  class ConstDecl;             // CONST_DECL
  class ErrorDecl;             // ERROR_DECL
  class FunctionProto;         // FUNCTION_PROTO
  class MethodProto;           // METHOD_PROTO
  class MethOrFunction;        // ...
    class Function;               // FUNCTION
    class Method;                 // METHOD
  class TypeParm;              // TYPE_PARM
  class TypeArg;               // TYPE_ARG

  class Type;                  // ...
    class CharType;               // CHAR_TYPE
    class IntType;                // INT_TYPE
    class DoubleType;             // DOUBLE_TYPE
    class BoolType;               // BOOL_TYPE
    class VoidType;               // VOID_TYPE
    class TypeOfNullType;         // TYPE_OF_NULL_TYPE
    class AnyType;                // ANY_TYPE
    class PtrType;                // PTR_TYPE
    class ArrayType;              // ARRAY_TYPE
    class RecordType;             // RECORD_TYPE
    class FunctionType;           // FUNCTION_TYPE
    class NamedType;              // NAMED_TYPE

  class Statement;             // ...
    class IfStmt;                 // IF_STMT
    class AssignStmt;             // ASSIGN_STMT
    class CallStmt;               // CALL_STMT
    class SendStmt;               // SEND_STMT
    class WhileStmt;              // WHILE_STMT
    class DoStmt;                 // DO_STMT
    class BreakStmt;              // BREAK_STMT
    class ContinueStmt;           // CONTINUE_STMT
    class ReturnStmt;             // RETURN_STMT
    class ForStmt;                // FOR_STMT
    class SwitchStmt;             // SWITCH_STMT
    class TryStmt;                // TRY_STMT
    class ThrowStmt;              // THROW_STMT
    class FreeStmt;               // FREE_STMT
    class DebugStmt;              // DEBUG_STMT

  class Case;                  // CASE
  class Catch;                 // CATCH

  class VarDecl;               // ...
    class Global;                 // GLOBAL
    class Local;                  // LOCAL
    class Parameter;              // PARAMETER
    class ClassField;             // CLASS_FIELD
    class RecordField;            // RECORD_FIELD

  class Expression;            // ...

    class Constant;               // ...
      class IntConst;                // INT_CONST
      class DoubleConst;             // DOUBLE_CONST
      class CharConst;               // CHAR_CONST
      class StringConst;             // STRING_CONST
      class BoolConst;               // BOOL_CONST
      class NullConst;               // NULL_CONST

    class CallExpr;               // CALL_EXPR
    class SendExpr;               // SEND_EXPR
    class SelfExpr;               // SELF_EXPR
    class SuperExpr;              // SUPER_EXPR
    class FieldAccess;            // FIELD_ACCESS
    class ArrayAccess;            // ARRAY_ACCESS
    class Constructor;            // CONSTRUCTOR
    class ClosureExpr;            // CLOSURE_EXPR
    class VariableExpr;           // VARIABLE_EXPR
    class AsPtrToExpr;            // AS_PTR_TO_EXPR
    class AsIntegerExpr;          // AS_INTEGER_EXPR
    class ArraySizeExpr;          // ARRAY_SIZE_EXPR
    class IsInstanceOfExpr;       // IS_INSTANCE_OF_EXPR
    class IsKindOfExpr;           // IS_KIND_OF_EXPR
    class SizeOfExpr;             // SIZE_OF_EXPR
    class DynamicCheck;           // DYNAMIC_CHECK

  class Argument;              // ARGUMENT
  class CountValue;            // COUNT_VALUE
  class FieldInit;             // FIELD_INIT






//----------  AstNode  ----------

class AstNode {
  public:
    int   op;                      // e.g., HEADER, IF_STMT, SEND_EXPR, etc.
    Token tokn;

    AstNode (int oper) {
      op = oper;
      tokn = token;
    }
    ~AstNode () {}
    virtual void prettyPrint (int indent);
    void positionAt (AstNode * node) {
      tokn = node->tokn;
    }
    void positionAtToken (Token tok) {
      tokn = tok;
    }
};


//----------  Header  ----------

class Header : public AstNode {
  public:
    Header *        next;
    String *        packageName;     // e.g., "MyPack"
    Uses *          uses;            // Linked list, may be NULL
    ConstDecl *     consts;          // Linked list, may be NULL
    ErrorDecl *     errors;          // Linked list, may be NULL
    Global *        globals;         // Linked list, may be NULL
    TypeDef *       typeDefs;        // Linked list, may be NULL
    FunctionProto * functionProtos;  // Linked list, may be NULL
    Interface *     interfaces;      // Linked list, may be NULL
    ClassDef *      classes;         // Linked list, may be NULL
    int             hashVal;         // The pseudo-random code for this token sequence
    int             mark;            // 0=unseen, 1=processing, 2=done
    Header *        tempNext;        // Used in topoProcessAllPackages
    Mapping<String,AstNode> *        // String --> ConstDecl,Global,TypeDef,
                    packageMapping;  //            FunctionProto, Interface, ClassDef
    Mapping<String,ErrorDecl> *      // String --> ErrorDecl
                    errorMapping;    //
    Function *      functions;       // For mainHeader; same as in code file
    Function *      closures;        // All closures we have encountered;

    Header () : AstNode (HEADER) {
      next = NULL;
      packageName = NULL;
      uses = NULL;
      consts = NULL;
      errors = NULL;
      globals = NULL;
      typeDefs = NULL;
      functionProtos = NULL;
      interfaces = NULL;
      classes = NULL;
      hashVal = 0;
      mark = 0;
      tempNext = NULL;
      packageMapping = NULL;
      errorMapping = NULL;
      functions = NULL;
      closures = NULL;
    }
    ~Header () {}
    virtual void prettyPrint (int indent);
};



//----------  Code  ----------

class Code : public AstNode {
  public:
    String *    packageName;         // e.g., "MyPack"
    ConstDecl * consts;              // Linked list, may be NULL
    ErrorDecl * errors;              // Linked list, may be NULL
    Global *    globals;             // Linked list, may be NULL
    TypeDef *   typeDefs;            // Linked list, may be NULL
    Function *  functions;           // Linked list, may be NULL
    Interface * interfaces;          // Linked list, may be NULL
    ClassDef *  classes;             // Linked list, may be NULL
    Behavior *  behaviors;           // Linked list, may be NULL
    int         hashVal;             // The pseudo-random code for this token sequence

    Code () : AstNode (CODE) {
      packageName = NULL;
      consts = NULL;
      errors = NULL;
      globals = NULL;
      typeDefs = NULL;
      functions = NULL;
      interfaces = NULL;
      classes = NULL;
      behaviors = NULL;
      hashVal = 0;
    }
    ~ Code () {}
    virtual void prettyPrint (int indent);
};



//----------  Uses  ----------

class Uses : public AstNode {
  public:
    Uses *     next;
    String *   id;                // name of package (e.g., "SysPack")
    Renaming * renamings;         // Linked list, may be NULL
    Header *    myDef;            // Null if errors in topoProcessing Headers

    Uses () : AstNode (USES) {
      next = NULL;
      id = NULL;
      renamings = NULL;
      myDef = NULL;
    }
    ~ Uses () {}
    virtual void prettyPrint (int indent);
};



//----------  Renaming  ----------

class Renaming : public AstNode {
  public:
    Renaming * next;
    String *   from;               // e.g., "foo" or "++"
    String *   to;                 // e.g., "superFoo" or "^++"

    Renaming () : AstNode (RENAMING) {
      next = NULL;
      from = NULL;
      to = NULL;
    }
    ~ Renaming () {}
    virtual void prettyPrint (int indent);
};



//----------  Abstract  ----------

class Abstract : public AstNode {
  public:
    String *    id;                            // E.g., "MyClass" or "MyInterface"
    Header *    myHeader;                      // Never NULL (set in
                                               // .  topoProcessClasses / Interfaces)
    Abstract *  nextAbstract;                  // A list of all classes & interfaces
    Mapping<Abstract,Abstract> *               // All sub-Abstracts in this package
                knownSubAbstracts;             // .  (including self)
    Mapping<Abstract,Abstract> *               // All super-Abstracts in this package
                superAbstractsInThisPackage;   // .  (including self)
    Mapping<Abstract,Abstract> *               // Minimal super-Abstracts that are
                supersAbstractsInOtherPackages;// .   not in this package
    Mapping<String,MethodProto> *       // String --> MethodProto
                selectorMapping;        // For Classes: This includes copies of
                                        //   MethodProtos inherited from our superclass
                                        // For Interfaces: Initially this
                                        //   includes only the local MethodProtos
                                        //   from this interface; after "checkExtends"
                                        //   it includes inherited MethodProtos, too
    Mapping<Offset,String> *            // This maps offsets to selectors
               offsetToSelector;        // .
    Mapping<String,Offset> *            // This maps selectors to offsets
               selectorToOffset;        // .
    Mapping<Offset,String> *            // Indicates that an offset has been reserved
               unavailableOffsets;      // .  in this Abstract (and for which message);
                                        // .  (The message may not be in this Abstract,
                                        // .  but the offset in nonetheless occupied.)
    char *        newName;              // ClassDef: label on dispatch table
                                        // Interface: label on Interface Descriptor

    Abstract (int op) : AstNode (op) {
      id = NULL;
      myHeader = NULL;
      nextAbstract = NULL;
      knownSubAbstracts = NULL;
      superAbstractsInThisPackage = NULL;
      supersAbstractsInOtherPackages = NULL;
      selectorMapping = NULL;
      offsetToSelector = NULL;
      selectorToOffset = NULL;
      unavailableOffsets = NULL;
      newName = NULL;
    }
    ~ Abstract () {}
    virtual void prettyPrint (int indent);
};



//----------  Interface  ----------

class Interface : public Abstract {
  public:
    Interface *   next;
    TypeParm *    typeParms;               // Linked list, may be NULL
    TypeArg *     extends;                 // Linked list of NamedTypes, may be NULL
    MethodProto * methodProtos;            // Linked list, may be NULL (locals only)
    int           mark;                    // 0=unseen, 1=processing, 2=done
    Interface *   tempNext;                // temp, used in topo-sorting
    int           isPrivate;               // 1=from code file, 0=from header file
    MethodProto *                          // A list of (copies of) inherited
                  inheritedMethodProtos;   //   messages

    Interface () : Abstract (INTERFACE) {
      next = NULL;
      typeParms = NULL;
      extends = NULL;
      methodProtos = NULL;
      mark = 0;
      tempNext = NULL;
      isPrivate = 0;
      inheritedMethodProtos = NULL;
    }
    ~ Interface () {}
    virtual void prettyPrint (int indent);
};



//----------  ClassDef  ----------

class ClassDef : public Abstract {
  public:
    ClassDef *    next;
    TypeParm *    typeParms;            // Linked list, may be NULL
    TypeArg *     implements;           // Linked list, may be NULL
    NamedType *   superclass;           // NULL means class = "Object"
    ClassField *  fields;               // Linked list, may be NULL
    MethodProto * methodProtos;         // Linked list, may be NULL, in order
                                        //   (copies of inherited protos added at front)
    int mark;                           // 0=unseen, 1=processing, 2=done
    ClassDef *    tempNext;             // temp var, used in topo-sorting
    int           isPrivate;            // 1=from code file, 0=from header file
    Method *      methods;              // From Behavior, may be NULL;
                                        //   Does not include inherited methods.
    ClassDef *    superclassDef;        // To our superclass, NULL if errors or none
    Mapping<TypeParm,Type> *            // This mapping maps TypeParms from the
                  superclassMapping;    //   superclass's ClassDef to types in our
                                        //   'superclass' clause
    Mapping<String,AstNode> *           // String --> ClassField; parent=packageMapping
                  classMapping;         //   Includes copies of inherited ClassFields
    Mapping<String,Method> *            // String --> Method; includes only local methods
                  localMethodMapping;   //   (not inherited methods)
    int           sizeInBytes;          // Size, including class ptr; -1 = problems
    Type *        typeOfSelf;           // e.g., "Person" or Person[T]"

    ClassDef () : Abstract (CLASS_DEF) {
      next = NULL;
      typeParms = NULL;
      implements = NULL;
      superclass = NULL;
      fields = NULL;
      methodProtos = NULL;
      mark = 0;
      tempNext = NULL;
      isPrivate = 0;
      methods = NULL;
      superclassDef = NULL;
      superclassMapping = NULL;
      classMapping = NULL;
      localMethodMapping = NULL;
      selectorMapping = NULL;
      sizeInBytes = -1;
      typeOfSelf = NULL;
    }
    ~ ClassDef () {}
    virtual void prettyPrint (int indent);
};



//----------  Behavior  ----------

class Behavior : public AstNode {
  public:
    Behavior * next;
    String *   id;
    Method *   methods;            // Linked list, may be NULL

    Behavior () : AstNode (BEHAVIOR) {
      next = NULL;
      id = NULL;
      methods = NULL;
    }
    ~ Behavior () {}
    virtual void prettyPrint (int indent);
};



//----------  TypeDef  ----------

class TypeDef : public AstNode {
  public:
    TypeDef * next;
    String *  id;
    Type *    type;
    int       mark;

    TypeDef () : AstNode (TYPE_DEF) {
      next = NULL;
      id = NULL;
      type = NULL;
      mark = 0;
    }
    ~ TypeDef () {}
    virtual void prettyPrint (int indent);
};



//----------  ConstDecl  ----------

class ConstDecl : public AstNode {
  public:
    ConstDecl * next;
    String * id;
    Expression * expr;
    int isPrivate;                  // 1=in code file, 0=in header file

    ConstDecl () : AstNode (CONST_DECL) {
      next = NULL;
      id = NULL;
      expr = NULL;
      isPrivate = 0;
    }
    ~ ConstDecl () {}
    virtual void prettyPrint (int indent);
};



//----------  ErrorDecl  ----------

class ErrorDecl : public AstNode {
  public:
    ErrorDecl *     next;
    String *       id;
    Parameter *    parmList;         // Linked list, may be NULL
//  int            isPrivate;        // 1=in code file, 0=in header file
    char *         newName;          // E.g., "_Error_P_MyPackageName_OriginalName"
    int            totalParmSize;

    ErrorDecl () : AstNode (ERROR_DECL) {
      next = NULL;
      id = NULL;
      parmList = NULL;
      // isPrivate = 0;
      newName = NULL;
      totalParmSize = 0;
    }
    ~ ErrorDecl () {}
    virtual void prettyPrint (int indent);
};



//----------  FunctionProto  ----------

class FunctionProto : public AstNode {
  public:
    FunctionProto * next;
    String *        id;
    Parameter *     parmList;       // Linked list, may be NULL
    Type *          retType;        // Never NULL
    int             isExternal;     // 1=external
    int             isPrivate;      // 0=not in header file, 1=in header file
    Header *        myHeader;       // The package containing this function
    Function *      myFunction;     // NULL if none or if error
    char *          newName;        // E.g., "bar", "_function_123_foo", "_closure_321"
    int             totalParmSize;  // Size of all parms, including padding
    int             retSize;        // Size of return result (-1 = void, could be 1)

    FunctionProto () : AstNode (FUNCTION_PROTO) {
      next = NULL;
      id = NULL;
      parmList = NULL;
      retType = NULL;
      isExternal = 0;
      isPrivate = 0;
      myHeader = NULL;
      myFunction = NULL;
      totalParmSize = 0;
      retSize = 0;
    }
    ~ FunctionProto () {}
    virtual void prettyPrint (int indent);
};



//----------  MethodProto  ----------

class MethodProto : public AstNode {
  public:
    MethodProto * next;
    int           kind;             // INFIX, PREFIX, KEYWORD, NORMAL
    String *      selector;         // e.g., "foo", "+", "at:put:"
    Parameter *   parmList;         // Linked list, may be NULL
    Type *        retType;          // Never NULL
    Method *      myMethod;         // For Classes: Only NULL when errors; may
                                    //   point to Methods in a superclass
                                    // For interfaces: Always NULL
    int           totalParmSize;    // Size of all parms, including padding
    int           retSize;          // Size of return result (-1 = void, could be 1)
    int           offset;           // The offset in the dispatch table

    MethodProto () : AstNode (METHOD_PROTO) {
      next = NULL;
      selector = NULL;
      parmList = NULL;
      retType = NULL;
      kind = EOF;
      myMethod = NULL;
      totalParmSize = 0;
      retSize = 0;
      offset = -1;
    }
    ~ MethodProto () {}
    virtual void prettyPrint (int indent);
};



//----------  MethOrFunction  ----------

class MethOrFunction : public AstNode {
  public:
    Parameter *     parmList;       // Linked list, may be NULL
    Type *          retType;        // Never NULL
    Local *         locals;         // Linked list, may be NULL
    Statement *     stmts;          // Linked list, may be NULL
    int             frameSize;      // Frame size in bytes
    char *          newName;        // Methods: e.g., "_Method_P_MyPackageName_47"
                                    // Funs: "bar", "_function_123_foo", "_closure_321"
    int             maxArgBytes;    // Minimum is zero if we call no funs or meths
    int             totalParmSize;  // Size of all parms, including padding
    int             containsTry;    // 1=there is a TRY stmt in this function
    Local *         catchStackSave; // The temp where the catch stack will be saved
    Catch *         catchList;      // List of all catches in this function

    MethOrFunction (int op) : AstNode (op) {
      parmList = NULL;
      retType = NULL;
      locals = NULL;
      stmts = NULL;
      frameSize = -1;
      newName = NULL;
      maxArgBytes = -1;
      totalParmSize = 0;
      containsTry = 0;
      catchStackSave = NULL;
      catchList = NULL;
    }
    ~ MethOrFunction () {}
    virtual void prettyPrint (int indent);
};



//----------  Function  ----------

class Function : public MethOrFunction {
  public:
    Function *      next;
    String *        id;             // NULL = Closure
    FunctionProto * myProto;        // Null if error or closure

    Function () : MethOrFunction (FUNCTION) {
      next = NULL;
      id = NULL;
      myProto = NULL;
    }
    ~ Function () {}
    virtual void prettyPrint (int indent);
};



//----------  Method  ----------

class Method : public MethOrFunction {
  public:
    Method *      next;
    int           kind;             // INFIX, PREFIX, KEYWORD, NORMAL
    String *      selector;         // e.g., "foo", "+", "at:put:"
    MethodProto * myMethodProto;    // Only NULL when errors
    ClassDef *    myClass;          // Class in which this code appeared

    Method () : MethOrFunction (METHOD) {
      next = NULL;
      kind = 0;
      selector = NULL;
      myMethodProto = NULL;
      myClass = NULL;
    }
    ~ Method () {}
    virtual void prettyPrint (int indent);
};



//----------  TypeParm  ----------

class TypeParm : public AstNode {
  //
  // This represents something like [T1: Object, T2: String]
  //
  public:
    TypeParm * next;
    String * id;
    Type * type;
    int fourByteRestricted;               // 1=used in field, local, or parm decl

    TypeParm () : AstNode (TYPE_PARM) {
      next = NULL;
      id = NULL;
      type = NULL;
      fourByteRestricted = 0;
    }
    ~ TypeParm () {}
    virtual void prettyPrint (int indent);
};



//----------  TypeArg  ----------

class TypeArg : public AstNode {
  //
  // This represents something like "int, char, List[String]"
  //
  public:
    TypeArg * next;
    Type *    type;
    int       offset;          // Only for typeArgs in FunctionTypes
    int       sizeInBytes;     // Only for typeArgs in FunctionTypes

    TypeArg () : AstNode (TYPE_ARG) {
      next        = NULL;
      type        = NULL;
      offset       = -1;
      sizeInBytes = -1;
    }
    ~ TypeArg () {}
    virtual void prettyPrint (int indent);
};



//----------  Type  ----------

class Type : public AstNode {
  public:

    Type (int op) : AstNode (op) { }
    ~ Type () {}
    virtual void prettyPrint (int indent);
};



//----------  CharType  ----------

class CharType : public Type {
  public:

    CharType () : Type (CHAR_TYPE) { }
    ~ CharType () {}
    virtual void prettyPrint (int indent);
};



//----------  IntType  ----------

class IntType : public Type {
  public:

    IntType () : Type (INT_TYPE) { }
    ~ IntType () {}
    virtual void prettyPrint (int indent);
};



//----------  DoubleType  ----------

class DoubleType : public Type {
  public:

    DoubleType () : Type (DOUBLE_TYPE) { }
    ~ DoubleType () {}
    virtual void prettyPrint (int indent);
};



//----------  BoolType  ----------

class BoolType : public Type {
  public:

    BoolType () : Type (BOOL_TYPE) { }
    ~ BoolType () {}
    virtual void prettyPrint (int indent);
};



//----------  VoidType  ----------

class VoidType : public Type {
  public:

    VoidType () : Type (VOID_TYPE) { }
    ~ VoidType () {}
    virtual void prettyPrint (int indent);
};



//----------  TypeOfNullType  ----------

class TypeOfNullType : public Type {
  public:

    TypeOfNullType () : Type (TYPE_OF_NULL_TYPE) { }
    ~ TypeOfNullType () {}
    virtual void prettyPrint (int indent);
};



//----------  AnyType  ----------

class AnyType : public Type {
  public:

    AnyType () : Type (ANY_TYPE) { }
    ~ AnyType () {}
    virtual void prettyPrint (int indent);
};



//----------  PtrType  ----------

class PtrType : public Type {
  public:
    Type * baseType;

    PtrType () : Type (PTR_TYPE) {
      baseType = NULL;
    }
    ~ PtrType () {}
    virtual void prettyPrint (int indent);
};



//----------  ArrayType  ----------

class ArrayType : public Type {
  public:
    Expression * sizeExpr;              // May be NULL
    Type * baseType;
    int sizeInBytes;                    // -1 = error or "array [*] of ..."
    int sizeOfElements;                 // size of each element

    ArrayType () : Type (ARRAY_TYPE) {
      sizeExpr = NULL;
      baseType = NULL;
      sizeInBytes = -1;
      sizeOfElements = -1;
    }
    ~ ArrayType () {}
    virtual void prettyPrint (int indent);
};



//----------  RecordType  ----------

class RecordType : public Type {
  public:
    RecordField * fields;               // Null only if syntax error
    int sizeInBytes;                    // Always a mulitple of 4
    Mapping<String, RecordField> *      // String --> RecordField
              fieldMapping;             //

    RecordType () : Type (RECORD_TYPE) {
      fields = NULL;
      sizeInBytes = -1;
      fieldMapping = NULL;
    }
    ~ RecordType () {}
    virtual void prettyPrint (int indent);
};



//----------  FunctionType  ----------

class FunctionType : public Type {
  public:
    TypeArg * parmTypes;          // may be NULL
    Type * retType;               // never NULL
    int totalParmSize;            // Size of all parms, including return value!
    int retSize;                  // Size of return result (-1 = void, could be 1)

    FunctionType () : Type (FUNCTION_TYPE) {
      parmTypes = NULL;
      retType = NULL;
      totalParmSize = -1;
      retSize = 0;
    }
    ~ FunctionType () {}
    virtual void prettyPrint (int indent);
};



//----------  NamedType  ----------

class NamedType : public Type {
  //
  // This represents something like "Person" or "Dictionary [String, int]"
  //
  public:
    String * id;
    TypeArg * typeArgs;                // May be NULL
    AstNode * myDef;                   // Interface, ClassDef, TypeParm,
                                       //   or TypeDef, or NULL if error
    Mapping <TypeParm, Type> * subst;  // Used only for class and interfaces;
                                       //   may be NULL

    NamedType () : Type (NAMED_TYPE) {
      id = NULL;
      typeArgs = NULL;
      myDef = NULL;
      subst = NULL;
    }
    ~ NamedType () {}
    virtual void prettyPrint (int indent);
};



//----------  Statement  ----------

class Statement : public AstNode {
  public:
    Statement * next;               // Statements are kept on linked lists

    Statement (int op) : AstNode (op) {
      next = NULL;
    }
    ~ Statement () {}
    virtual void prettyPrint (int indent);
};



//----------  IfStmt  ----------

class IfStmt : public Statement {
  public:
    Expression * expr;
    Statement * thenStmts;               // may be NULL
    Statement * elseStmts;               // may be NULL

    IfStmt () : Statement (IF_STMT) {
      expr = NULL;
      thenStmts = NULL;
      elseStmts = NULL;
    }
    ~ IfStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  AssignStmt  ----------

class AssignStmt : public Statement {
  public:
    Expression * lvalue;
    Expression * expr;
    int          sizeInBytes;            // The number of bytes to be copied
    int          dynamicCheck;           // 0 means no special check is needed
    ClassDef *   classDef;               // For dynamicCheck == 2 or 3
    int          arraySize;              // For dynamicCheck == 5, 6, or 7

    AssignStmt () : Statement (ASSIGN_STMT) {
      lvalue = NULL;
      expr = NULL;
      sizeInBytes = -1;
      dynamicCheck = 0;
      classDef = NULL;
      arraySize = -1;
    }
    ~ AssignStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  CallStmt  ----------

class CallStmt : public Statement {
  public:
    CallExpr * expr;         // NULL if error

    CallStmt () : Statement (CALL_STMT) {
      expr = NULL;
    }
    ~ CallStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  SendStmt  ----------

class SendStmt : public Statement {
  public:
    SendExpr * expr;         // NULL if error

    SendStmt () : Statement (SEND_STMT) {
      expr = NULL;
    }
    ~ SendStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  WhileStmt  ----------

class WhileStmt : public Statement {
  public:
    Expression * expr;
    Statement * stmts;
    int containsAnyBreaks;
    int containsAnyContinues;
    char * topLabel;
    char * exitLabel;

    WhileStmt () : Statement (WHILE_STMT) {
      expr = NULL;
      stmts = NULL;
      containsAnyBreaks = 0;
      containsAnyContinues = 0;
      topLabel = NULL;
      exitLabel = NULL;
    }
    ~ WhileStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  DoStmt  ----------

class DoStmt : public Statement {
  public:
    Statement * stmts;
    Expression * expr;
    int containsAnyBreaks;
    int containsAnyContinues;
    char * exitLabel;
    char * testLabel;

    DoStmt () : Statement (DO_STMT) {
      stmts = NULL;
      expr = NULL;
      containsAnyBreaks = 0;
      containsAnyContinues = 0;
      exitLabel = NULL;
      testLabel = NULL;
    }
    ~ DoStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  BreakStmt  ----------

class BreakStmt : public Statement {
  public:
    Statement * enclosingStmt;     // A FOR_STMT, DO_STMT, WHILE_STMT, or SWITCH_STMT

    BreakStmt () : Statement (BREAK_STMT) {
      enclosingStmt = NULL;
    }
    ~ BreakStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  ContinueStmt  ----------

class ContinueStmt : public Statement {
  public:
    Statement * enclosingStmt;       // A FOR_STMT, DO_STMT, or WHILE_STMT

    ContinueStmt () : Statement (CONTINUE_STMT) {
      enclosingStmt = NULL;
    }
    ~ ContinueStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  ReturnStmt  ----------

class ReturnStmt : public Statement {
  public:
    Expression * expr;                   // may be NULL
    MethOrFunction * enclosingMethOrFunction;   // not NULL
    int retSize;                         // Size of return result (-1 = void, could be 1)

    ReturnStmt () : Statement (RETURN_STMT) {
      expr = NULL;
      enclosingMethOrFunction = NULL;
      retSize = 0;
    }
    ~ ReturnStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  ForStmt  ----------

class ForStmt : public Statement {
  public:
    Expression * lvalue;
    Expression * expr1;
    Expression * expr2;
    Expression * expr3;            // may be NULL
    Statement * stmts;
    int containsAnyBreaks;
    int containsAnyContinues;
    char * incrLabel;
    char * exitLabel;

    ForStmt () : Statement (FOR_STMT) {
      lvalue = NULL;
      expr1 = NULL;
      expr2 = NULL;
      expr3 = NULL;
      stmts = NULL;
      containsAnyBreaks = 0;
      containsAnyContinues = 0;
      incrLabel = NULL;
      exitLabel = NULL;
    }
    ~ ForStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  SwitchStmt  ----------

class SwitchStmt : public Statement {
  public:
    Expression * expr;
    Case * caseList;                     // may be NULL
    Statement * defaultStmts;            // may be NULL
    int containsAnyBreaks;
    int defaultIncluded;                 // 1="default:" clause is present
                                         // if absent, must check for runtime error
    int lowValue;                        // The range of values seen
    int highValue;                       // .
    char * exitLabel;                    // Used for break stmts

    SwitchStmt () : Statement (SWITCH_STMT) {
      expr = NULL;
      caseList = NULL;
      defaultStmts = NULL;
      containsAnyBreaks = 0;
      defaultIncluded = 0;
      lowValue = 0x7fffffff;
      highValue = 0x80000000;
      exitLabel = NULL;
    }
    ~ SwitchStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  TryStmt  ----------

class TryStmt : public Statement {
  public:
    Statement * stmts;               // may be NULL
    Catch * catchList;               // NULL only if errors

    TryStmt () : Statement (TRY_STMT) {
      stmts = NULL;
      catchList = NULL;
    }
    ~ TryStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  ThrowStmt  ----------

class ThrowStmt : public Statement {
  public:
    String * id;                  // aaaa
    Argument * argList;           // may be NULL
    ErrorDecl * myDef;            // NULL iff errors

    ThrowStmt () : Statement (THROW_STMT) {
      id = NULL;
      argList = NULL;
      myDef = NULL;
    }
    ~ ThrowStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  FreeStmt  ----------

class FreeStmt : public Statement {
  public:
    Expression * expr;

    FreeStmt () : Statement (FREE_STMT) {
      expr = NULL;
    }
    ~ FreeStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  DebugStmt  ----------

class DebugStmt : public Statement {
  public:

    DebugStmt () : Statement (DEBUG_STMT) { }
    ~ DebugStmt () {}
    virtual void prettyPrint (int indent);
};



//----------  Case  ----------

class Case : public AstNode {
  public:
    Case * next;
    Expression * expr;
    int ivalue;                      // The value of the case expression
    Statement * stmts;               // may be NULL
    char * label;

    Case () : AstNode (CASE) {
      next = NULL;
      expr = NULL;
      ivalue = 0;
      stmts = NULL;
      label = NULL;
    }
    ~ Case () {}
    virtual void prettyPrint (int indent);
};



//----------  Catch  ----------

class Catch : public AstNode {
  public:
    Catch *     next;                    // ... in list for each try stmt
    Catch *     nextInMethOrFunction;    // ... in list for this function/method
    String *    id;
    Parameter * parmList;                // may be NULL
    Statement * stmts;                   // may be NULL
    ErrorDecl * myDef;                   // NULL iff errors
    char *      label;
    MethOrFunction * enclosingMethOrFunction;

    Catch () : AstNode (CATCH) {
      next = NULL;
      nextInMethOrFunction = NULL;
      id = NULL;
      parmList = NULL;
      stmts = NULL;
      myDef = NULL;
      label = NULL;
      enclosingMethOrFunction = NULL;
    }
    ~ Catch () {}
    virtual void prettyPrint (int indent);
};



//----------  VarDecl  ----------

class VarDecl : public AstNode {
  public:
    String *  id;
    VarDecl * next;
    Type *    type;            // NULL for temporaries
    int       offset;          // Only for Local, Parameter, ClassField, RecordField;
                               //   not for Global
    int       sizeInBytes;
    char *    varDescLabel;    // Used in generating VarDesc info

    VarDecl (int op) : AstNode (op) {
      next = NULL;
      id = NULL;
      type = NULL;
      offset = -1;
      sizeInBytes = -1;
      varDescLabel = NULL;
    }
    ~ VarDecl () {}
    virtual void prettyPrint (int indent);
};



//----------  Global  ----------

class Global : public VarDecl {
  public:
//  Global * next;
    Expression * initExpr;               // may be NULL
    int isPrivate;                       // 0=in header file, 1=in code file

    Global () : VarDecl (GLOBAL) {
//    next = NULL;
      initExpr = NULL;
      isPrivate = 0;
    }
    ~ Global () {}
    virtual void prettyPrint (int indent);
};



//----------  Local  ----------

class Local : public VarDecl {
  public:
//  Local * next;
    Expression * initExpr;               // may be NULL
    int wasUsed;

    Local () : VarDecl (LOCAL) {
//    next = NULL;
      initExpr = NULL;
      wasUsed = 0;
    }
    ~ Local () {}
    virtual void prettyPrint (int indent);
};



//----------  Parameter  ----------

class Parameter : public VarDecl {
  public:
//  Parameter * next;
    int throwSideOffset;         // Only for catch parms; offset used during throw.

    Parameter () : VarDecl (PARAMETER) {
//    next = NULL;
      throwSideOffset = -1;
    }
    ~ Parameter () {}
    virtual void prettyPrint (int indent);
};



//----------  ClassField  ----------

class ClassField : public VarDecl {
  public:
//  ClassField * next;

    ClassField () : VarDecl (CLASS_FIELD) {
//    next = NULL;
    }
    ~ ClassField () {}
    virtual void prettyPrint (int indent);
};



//----------  RecordField  ----------

class RecordField : public VarDecl {
  public:
//  RecordField * next;

    RecordField () : VarDecl (RECORD_FIELD) {
//    next = NULL;
    }
    ~ RecordField () {}
    virtual void prettyPrint (int indent);
};



//----------  Expression  ----------

class Expression : public AstNode {
  public:

    Expression (int op) : AstNode (op) {
    }
    ~ Expression () {}
    virtual void prettyPrint (int indent);
};



//----------  Constant  ----------

class Constant : public Expression {
  public:

    Constant (int op) : Expression (op) {
    }
    ~ Constant () {}
    virtual void prettyPrint (int indent);
};



//----------  IntConst  ----------

class IntConst : public Constant {
  public:
    int ivalue;

    IntConst () : Constant (INT_CONST) {
      ivalue = 0;
    }
    ~ IntConst () {}
    virtual void prettyPrint (int indent);
};



//----------  DoubleConst  ----------

class DoubleConst : public Constant {
  public:
    double rvalue;
    DoubleConst * next;
    char * nameOfConstant;

    DoubleConst () : Constant (DOUBLE_CONST) {
      rvalue = 0.0;
      next = NULL;
      nameOfConstant = NULL;
    }
    ~ DoubleConst () {}
    virtual void prettyPrint (int indent);
};



//----------  CharConst  ----------

class CharConst : public Constant {
  public:
    int ivalue;

    CharConst () : Constant (CHAR_CONST) {
      ivalue = '?';
    }
    ~ CharConst () {}
    virtual void prettyPrint (int indent);
};



//----------  StringConst  ----------

class StringConst : public Constant {
  public:
    String * svalue;
    StringConst * next;
    char * nameOfConstant;

    StringConst () : Constant (STRING_CONST) {
      svalue = NULL;
      next = NULL;
      nameOfConstant = NULL;
    }
    ~ StringConst () {}
    virtual void prettyPrint (int indent);
};



//----------  BoolConst  ----------

class BoolConst : public Constant {
  public:
    int ivalue;               // 1=true, 0=false

    BoolConst (int i) : Constant (BOOL_CONST) {
      ivalue = i;
    }
    ~ BoolConst () {}
    virtual void prettyPrint (int indent);
};



//----------  NullConst  ----------

class NullConst : public Constant {
  public:

    NullConst () : Constant (NULL_CONST) { }
    ~ NullConst () {}
    virtual void prettyPrint (int indent);
};



//----------  CallExpr  ----------

class CallExpr : public Expression {
  public:
    String * id;               // e.g., "foo"
    Argument * argList;        // may be NULL
    AstNode * myDef;           // Points to a FunctionProto, Local, Global, Parameter,
                               //   or ClassField; NULL if error or primitive
    int primitiveSymbol;       // 0=not a primitive; e.g., PRIMITIVE_DOUBLE_TO_INT, etc...
    int retSize;               // Size of returned value (0 if void), -1 for primitives

    CallExpr () : Expression (CALL_EXPR) {
      id = NULL;
      argList = NULL;
      myDef = NULL;
      primitiveSymbol = 0;
      retSize = -1;
    }
    ~ CallExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  SendExpr  ----------

class SendExpr : public Expression {
  public:
    String *      selector;            // e.g., "meth", "+", "at:put:"
    Expression *  receiver;
    Argument *    argList;             // may be NULL
    int           kind;                // INFIX, PREFIX, KEYWORD, NORMAL
    int           primitiveSymbol;     // 0=not a primitive; e.g., PRIMITIVE_I_ADD, etc...
    MethodProto * myProto;             // NULL for primitives

    SendExpr () : Expression (SEND_EXPR) {
      selector = NULL;
      receiver = NULL;
      argList = NULL;
      kind = EOF;
      primitiveSymbol = 0;
      myProto = NULL;
    }
    ~ SendExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  SelfExpr  ----------

class SelfExpr : public Expression {
  public:

    SelfExpr () : Expression (SELF_EXPR) {
    }
    ~ SelfExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  SuperExpr  ----------

class SuperExpr : public Expression {
  public:

    SuperExpr () : Expression (SUPER_EXPR) {
    }
    ~ SuperExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  FieldAccess  ----------

class FieldAccess : public Expression {
  public:
    Expression * expr;
    String * id;
    int offset;

    FieldAccess () : Expression (FIELD_ACCESS) {
      expr = NULL;
      id = NULL;
      offset = 0;
    }
    ~ FieldAccess () {}
    virtual void prettyPrint (int indent);
};



//----------  ArrayAccess  ----------

class ArrayAccess : public Expression {
  public:
    Expression * arrayExpr;
    Expression * indexExpr;
    int sizeOfElements;               // This is the multiplication scale factor

    ArrayAccess () : Expression (ARRAY_ACCESS) {
      arrayExpr = NULL;
      indexExpr = NULL;
      sizeOfElements = -1;
    }
    ~ ArrayAccess () {}
    virtual void prettyPrint (int indent);
};



//----------  Constructor  ----------

class Constructor : public Expression {
  public:
    Type * type;                      // should be Array, Class, or Record type
    CountValue * countValueList;      // may be NULL
    FieldInit * fieldInits;           // may be NULL
    int sizeInBytes;                  // Classes, records, array: size of whole
    ClassDef * myClass;               // Null if not a Class constructor
    int kind;                         // CLASS, RECORD, or ARRAY
    int allocKind;                    // NEW or ALLOC

    Constructor () : Expression (CONSTRUCTOR) {
      type = NULL;
      countValueList = NULL;
      fieldInits = NULL;
      sizeInBytes = -1;
      myClass = NULL;
      kind = -1;
    }
    ~ Constructor () {}
    virtual void prettyPrint (int indent);
};



//----------  ClosureExpr  ----------

class ClosureExpr : public Expression {
  public:
    Function * function;

    ClosureExpr () : Expression (CLOSURE_EXPR) {
      function = NULL;
    }
    ~ ClosureExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  VariableExpr  ----------

class VariableExpr : public Expression {
  public:
    String * id;
    AstNode * myDef;

    VariableExpr () : Expression (VARIABLE_EXPR) {
      id = NULL;
      myDef = NULL;
    }
    ~ VariableExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  AsPtrToExpr  ----------

class AsPtrToExpr : public Expression {
  public:
    Expression * expr;
    Type * type;

    AsPtrToExpr () : Expression (AS_PTR_TO_EXPR) {
      expr = NULL;
      type = NULL;
    }
    ~ AsPtrToExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  AsIntegerExpr  ----------

class AsIntegerExpr : public Expression {
  public:
    Expression * expr;

    AsIntegerExpr () : Expression (AS_INTEGER_EXPR) {
      expr = NULL;
    }
    ~ AsIntegerExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  ArraySizeExpr  ----------

class ArraySizeExpr : public Expression {
  public:
    Expression * expr;

    ArraySizeExpr () : Expression (ARRAY_SIZE_EXPR) {
      expr = NULL;
    }
    ~ ArraySizeExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  IsInstanceOfExpr  ----------

class IsInstanceOfExpr : public Expression {
  public:
    Expression * expr;
    Type * type;
    ClassDef * classDef;

    IsInstanceOfExpr () : Expression (IS_INSTANCE_OF_EXPR) {
      expr = NULL;
      type = NULL;
      classDef = NULL;
    }
    ~ IsInstanceOfExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  IsKindOfExpr  ----------

class IsKindOfExpr : public Expression {
  public:
    Expression * expr;
    Type * type;
    Abstract * classOrInterface;

    IsKindOfExpr () : Expression (IS_KIND_OF_EXPR) {
      expr = NULL;
      type = NULL;
      classOrInterface = NULL;
    }
    ~ IsKindOfExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  SizeOfExpr  ----------

class SizeOfExpr : public Expression {
  public:
    Type * type;

    SizeOfExpr () : Expression (SIZE_OF_EXPR) {
      type = NULL;
    }
    ~ SizeOfExpr () {}
    virtual void prettyPrint (int indent);
};



//----------  DynamicCheck  ----------

class DynamicCheck : public Expression {
  public:
    Expression * expr;
    int          kind;      // 8 ==  OBJECT COPY: ... := *objPtr
                            // 9 ==  ARRAY COPY: arr[10] := arr[10]
                            // 10 == ARRAY COPY: arr[10] := arr[*]
                            // 11 == ARRAY COPY: arr[*] := arr[10]  (will not occur)
                            // 12 == ARRAY COPY: arr[*] := arr[*]   (will not occur)
    int          expectedArraySize;   // Cases 9 and 10 only
    int          arraySizeInBytes;    // Cases 9 and 10 only
    ClassDef *   expectedClassDef;    // Case 8 only

    DynamicCheck () : Expression (DYNAMIC_CHECK) {
      expr = NULL;
      kind = -1;
      expectedArraySize = -1;
      arraySizeInBytes = -1;
      expectedClassDef = NULL;
    }
    ~ DynamicCheck () {}
    virtual void prettyPrint (int indent);
};



//----------  Argument  ----------

class Argument : public AstNode {
  public:
    Argument * next;
    Expression * expr;
    AstNode * tempName;             // Used in code gen
    int       offset;
    int       sizeInBytes;
    

    Argument () : AstNode (ARGUMENT) {
      next = NULL;
      expr = NULL;
      tempName = NULL;
      offset = -1;
      sizeInBytes = -1;
    }
    ~ Argument () {}
    virtual void prettyPrint (int indent);
};



//----------  CountValue  ----------

class CountValue : public AstNode {
  public:
    Expression * count;               // may be Null
    Expression * value;               // not NULL
    CountValue * next;
    AstNode *    countTemp;           // may be NULL, which means 1
    AstNode *    valueTemp;           // not NULL

    CountValue () : AstNode (COUNT_VALUE) {
      count = NULL;
      value = NULL;
      next = NULL;
      countTemp = NULL;
      valueTemp = NULL;
    }
    ~ CountValue () {}
    virtual void prettyPrint (int indent);
};



//----------  FieldInit  ----------

class FieldInit : public AstNode {
  public:
    FieldInit * next;
    String * id;
    Expression * expr;
    int offset;
    int sizeInBytes;

    FieldInit () : AstNode (FIELD_INIT) {
      next = NULL;
      id = NULL;
      expr = NULL;
      offset = -1;
      sizeInBytes = -1;
    }
    ~ FieldInit () {}
    virtual void prettyPrint (int indent);
};
