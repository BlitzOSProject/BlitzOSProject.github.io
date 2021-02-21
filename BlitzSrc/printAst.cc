// printAst.cc  --  Routines to print the Abstract Syntax Tree
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
// This file contains a routine "printAst()" which can be called to print
// out an Abstract Syntax Tree (AST).  It can be invoked as follows:
//
//     AstNode *p = ... ;
//     ...
//     printAst (6, p);
//

#include "main.h"



#define TABAMT 2



// printAst (indent, p)
//
// This routine prints the abstract syntax tree pointed to by p.
// The tree is printed with "indent" characters of indentation.
// Initially, it should be called with an "indent" of about 6 to
// allow enough space to print the #999: labels.
//
void printAst (int indent, AstNode *p) {

    Header * header;
    Code * code;
    Uses * uses;
    Renaming * renaming;
    Interface * interface;
    ClassDef * cl;
    Behavior * behavior;
    TypeDef * typeDef;
    ConstDecl * constDecl;
    ErrorDecl * errorDecl;
    FunctionProto * functionProto;
    Function * fun;
    MethodProto * methodProto;
    Method * meth;
    TypeParm * typeParm;
    TypeArg * typeArg;
    CharType * charType;
    IntType * intType;
    DoubleType * doubleType;
    BoolType * boolType;
    VoidType * voidType;
    TypeOfNullType * typeOfNullType;
    AnyType * anyType;
    PtrType * pType;
    ArrayType * aType;
    RecordType * rType;
    FunctionType * fType;
    NamedType * nType;
    IfStmt * ifStmt;
    AssignStmt * assignStmt;
    CallStmt * callStmt;
    SendStmt * sendStmt;
    WhileStmt * whileStmt;
    DoStmt * doStmt;
    BreakStmt * breakStmt;
    ContinueStmt * continueStmt;
    ReturnStmt * returnStmt;
    ForStmt * forStmt;
    SwitchStmt * switchStmt;
    TryStmt * tryStmt;
    ThrowStmt * throwStmt;
    FreeStmt * freeStmt;
    DebugStmt * debugStmt;
    Case * cas;
    Catch * cat;
    Global * global;
    Local * local;
    Parameter * parm;
    ClassField * classField;
    RecordField * recordField;
    IntConst * intConst;
    DoubleConst * doubleConst;
    CharConst * charConst;
    StringConst * stringConst;
    BoolConst * boolConst;
    NullConst * nullConst;
    CallExpr * callExpr;
    SendExpr * sendExpr;
    SelfExpr * selfExpr;
    SuperExpr * superExpr;
    FieldAccess * fieldAccess;
    ArrayAccess * arrayAccess;
    Constructor * constructor;
    ClosureExpr * closureExpr;
    VariableExpr * var;
    AsPtrToExpr * asPtrToExpr;
    AsIntegerExpr * asIntegerExpr;
    ArraySizeExpr * arraySizeExpr;
    IsInstanceOfExpr * isInstanceOfExpr;
    IsKindOfExpr * isKindOfExpr;
    SizeOfExpr * sizeOfExpr;
    DynamicCheck * dynamicCheck;
    Argument * arg;
    CountValue * countValue;
    FieldInit * fieldInit;


  if (p==NULL) {
    printLine (indent, "NULL");
    return;
  }

  printHeader (indent, symbolName (p->op), p);

  switch (p->op) {

    case HEADER:
      header = (Header *) p;
      printStringField (indent, "packageName:", header->packageName);
      printItem (indent, "uses:", header->uses);
      printItem (indent, "consts:", header->consts);
      printItem (indent, "errors:", header->errors);
      printItem (indent, "globals:", header->globals);
      printItem (indent, "typeDefs:", header->typeDefs);
      printItem (indent, "functionProtos:", header->functionProtos);
      printItem (indent, "functions:", header->functions);
      printItem (indent, "closures:", header->closures);
      printItem (indent, "interfaces:", header->interfaces);
      printItem (indent, "classes:", header->classes);
      printIntField (indent, "hashVal:", header->hashVal);
      printIndent (indent);
      printf ("packageMapping:\n");
      if (header->packageMapping != NULL) {
        header->packageMapping->print (indent+2);
      } else {
        printf ("NULL\n");
      }
      printIndent (indent);
      printf ("errorMapping:\n");
      if (header->errorMapping != NULL) {
        header->errorMapping->print (indent+2);
      } else {
        printf ("NULL\n");
      }
      printFooter (indent);
      if (header->next) {
        printAst (indent, header->next);
      }
      break;
    case CODE:
      code = (Code *) p;
      printStringField (indent, "packageName:", code->packageName);
      printItem (indent, "consts:", code->consts);
      printItem (indent, "errors:", code->errors);
      printItem (indent, "globals:", code->globals);
      printItem (indent, "typeDefs:", code->typeDefs);
      printItem (indent, "functions:", code->functions);
      printItem (indent, "interfaces:", code->interfaces);
      printItem (indent, "classes:", code->classes);
      printItem (indent, "behaviors:", code->behaviors);
      printIntField (indent, "hashVal:", code->hashVal);
      printFooter (indent);
      break;
    case USES:
      uses = (Uses *) p;
      printId (indent, uses->id);
      printPtrField (indent, "myDef:", uses-> myDef);
      printItem (indent, "renamings:", uses->renamings);
      printFooter (indent);
      if (uses->next) {
        printAst (indent, uses->next);
      }
      break;
    case RENAMING:
      renaming = (Renaming *) p;
      printStringField (indent, "from:", renaming->from);
      printStringField (indent, "to:", renaming->to);
      printFooter (indent);
      if (renaming->next) {
        printAst (indent, renaming->next);
      }
      break;
    case INTERFACE:
      interface = (Interface *) p;
      printId (indent, interface->id);
      printCharPtrField (indent, "newName:", interface->newName);
      printBoolField (indent, "isPrivate:", interface->isPrivate);
      printItem (indent, "typeParms:", interface->typeParms);
      printItem (indent, "extends:", interface->extends);
      printItem (indent, "methodProtos:", interface->methodProtos);
      printIntField (indent, "mark:", interface->mark);
      printPtrField (indent, "myHeader:", interface->myHeader);
      printPtrField (indent, "tempNext:", interface->tempNext);
      printIndent (indent+2);
      printf ("selectorMapping: \n");
      if (interface->selectorMapping) {
        interface->selectorMapping->print (indent+4);
      } else {
        printf ("NULL\n");
      }
      printItem (indent, "inheritedMethodProtos:", interface->inheritedMethodProtos);
      printFooter (indent);
      if (interface->next) {
        printAst (indent, interface->next);
      }
      break;
    case CLASS_DEF:
      cl = (ClassDef *) p;
      printId (indent, cl->id);
      printCharPtrField (indent, "newName:", cl->newName);
      printBoolField (indent, "isPrivate:", cl->isPrivate);
      printIntField (indent, "mark:", cl->mark);
      printIntField (indent, "sizeInBytes:", cl->sizeInBytes);
      printPtrField (indent, "myHeader:", cl->myHeader);
      printPtrField (indent, "tempNext:", cl->tempNext);
      printPtrField (indent, "superclassDef:", cl->superclassDef);
      printItem (indent, "typeParms:", cl->typeParms);
      printItem (indent, "typeOfSelf:", cl->typeOfSelf);
      printItem (indent, "implements:", cl->implements);
      printItem (indent, "superclass:", cl->superclass);
      printItem (indent, "fields:", cl->fields);
      printItem (indent, "methodProtos:", cl->methodProtos);
      printItem (indent, "methods:", cl->methods);
/***
      printIndent (indent+2);
      printf ("superclassMapping: \n");
      if (cl->superclassMapping) {
        cl->superclassMapping->print (indent+4);
      } else {
        printf ("NULL\n");
      }
      printIndent (indent+2);
      printf ("classMapping: \n");
      if (cl->classMapping) {
        cl->classMapping->print (indent+4);
      } else {
        printf ("NULL\n");
      }
***/
      printIndent (indent+2);
      printf ("localMethodMapping: \n");
      if (cl-> localMethodMapping) {
        cl-> localMethodMapping->print (indent+4);
      } else {
        printf ("NULL\n");
      }
      printIndent (indent+2);
      printf ("selectorMapping: \n");
      if (cl->selectorMapping) {
        cl-> selectorMapping->print (indent+4);
      } else {
        printf ("NULL\n");
      }
/***
      printIndent (indent+2);
      printf ("knownSubAbstracts: \n");
      if (cl->knownSubAbstracts) {
        cl->knownSubAbstracts->print (indent+4);
      } else {
        printf ("NULL\n");
      }

      printIndent (indent+2);
      printf ("superAbstractsInThisPackage: \n");
      if (cl->superAbstractsInThisPackage) {
        cl->superAbstractsInThisPackage->print (indent+4);
      } else {
        printf ("NULL\n");
      }

      printIndent (indent+2);
      printf ("supersAbstractsInOtherPackages: \n");
      if (cl->supersAbstractsInOtherPackages) {
        cl->supersAbstractsInOtherPackages->print (indent+4);
      } else {
        printf ("NULL\n");
      }

***/
      printFooter (indent);
      if (cl->next) {
        printAst (indent, cl->next);
      }
      break;
    case BEHAVIOR:
      behavior = (Behavior *) p;
      printId (indent, behavior->id);
      printItem (indent, "methods:", behavior->methods);
      printFooter (indent);
      if (behavior->next) {
        printAst (indent, behavior->next);
      }
      break;
    case TYPE_DEF:
      typeDef = (TypeDef *) p;
      printId (indent, typeDef->id);
      printIntField (indent, "mark:", typeDef->mark);
      printItem (indent, "type:", typeDef->type);
      printFooter (indent);
      if (typeDef->next) {
        printAst (indent, typeDef->next);
      }
      break;
    case CONST_DECL:
      constDecl = (ConstDecl *) p;
      printId (indent, constDecl->id);
      printItem (indent, "expr:", constDecl->expr);
      printBoolField (indent, "isPrivate:", constDecl->isPrivate);
      printFooter (indent);
      if (constDecl->next) {
        printAst (indent, constDecl->next);
      }
      break;
    case ERROR_DECL:
      errorDecl = (ErrorDecl *) p;
      printId (indent, errorDecl->id);
      printCharPtrField (indent, "newName:", errorDecl->newName);
      printIntField (indent, "totalParmSize:", errorDecl->totalParmSize);
      printItem (indent, "parmList:", errorDecl->parmList);
      // printBoolField (indent, "isPrivate:", errorDecl->isPrivate);
      printFooter (indent);
      if (errorDecl->next) {
        printAst (indent, errorDecl->next);
      }
      break;
    case FUNCTION_PROTO:
      functionProto = (FunctionProto *) p;
      printCharPtrField (indent, "newName:", functionProto->newName);
      printBoolField (indent, "isExternal:", functionProto->isExternal);
      printBoolField (indent, "isPrivate:", functionProto->isPrivate);
      printPtrField (indent, "myFunction:", functionProto->myFunction);
      printPtrField (indent, "myHeader:", functionProto->myHeader);
      printId (indent, functionProto->id);
      printItem (indent, "parmList:", functionProto->parmList);
      printItem (indent, "retType:", functionProto->retType);
      printIntField (indent, "totalParmSize:", functionProto->totalParmSize);
      printIntField (indent, "retSize:", functionProto->retSize);
      printFooter (indent);
      if (functionProto->next) {
        printAst (indent, functionProto->next);
      }
      break;
    case METHOD_PROTO:
      methodProto = (MethodProto *) p;
      printSymbolField (indent, "kind:", methodProto->kind);
      printStringField (indent, "selector:", methodProto-> selector);
      printPtrField (indent, "myMethod:", methodProto->myMethod);
      printItem (indent, "parmList:", methodProto->parmList);
      printItem (indent, "retType:", methodProto->retType);
      printIntField (indent, "totalParmSize:", methodProto->totalParmSize);
      printIntField (indent, "retSize:", methodProto->retSize);
      printIntField (indent, "offset:", methodProto->offset);
      printFooter (indent);
      if (methodProto->next) {
        printAst (indent, methodProto->next);
      }
      break;
    case FUNCTION:
      fun = (Function *) p;
      printId (indent, fun->id);
      printCharPtrField (indent, "newName:", fun->newName);
      printPtrField (indent, "myProto:", fun->myProto);
      printPtrField (indent, "catchList:", fun->catchList);
      printIntField (indent, "frameSize:", fun->frameSize);
      printIntField (indent, "maxArgBytes:", fun->maxArgBytes);
      printIntField (indent, "totalParmSize:", fun->totalParmSize);
      printBoolField (indent, "containsTry:", fun->containsTry);
      printItem (indent, "catchStackSave:", fun->catchStackSave);
      printItem (indent, "parmList:", fun->parmList);
      printItem (indent, "retType:", fun->retType);
      printItem (indent, "locals:", fun->locals);
      printItem (indent, "stmts:", fun->stmts);
      printFooter (indent);
      if (fun->next) {
        printAst (indent, fun->next);
      }
      break;
    case METHOD:
      meth = (Method *) p;
      printSymbolField (indent, "kind:", meth->kind);
      printStringField (indent, "selector:", meth->selector);
      printCharPtrField (indent, "newName:", meth->newName);
      printPtrField (indent, "myMethodProto:", meth->myMethodProto);
      printPtrField (indent, "myClass:", meth->myClass);
      printPtrField (indent, "catchList:", meth->catchList);
      printIntField (indent, "frameSize:", meth->frameSize);
      printIntField (indent, "maxArgBytes:", meth->maxArgBytes);
      printIntField (indent, "totalParmSize:", meth->totalParmSize);
      printBoolField (indent, "containsTry:", meth->containsTry);
      printItem (indent, "catchStackSave:", meth->catchStackSave);
      printItem (indent, "parmList:", meth->parmList);
      printItem (indent, "retType:", meth->retType);
      printItem (indent, "locals:", meth->locals);
      printItem (indent, "stmts:", meth->stmts);
      printFooter (indent);
      if (meth->next) {
        printAst (indent, meth->next);
      }
      break;
    case TYPE_PARM:
      typeParm = (TypeParm *) p;
      printId (indent, typeParm->id);
      printBoolField (indent, "fourByteRestricted:", typeParm->fourByteRestricted);
      printItem (indent, "type:", typeParm->type);
      printFooter (indent);
      if (typeParm->next) {
        printAst (indent, typeParm->next);
      }
      break;
    case TYPE_ARG:
      typeArg = (TypeArg *) p;
      printIntField (indent, "offset:", typeArg->offset);
      printIntField (indent, "sizeInBytes:", typeArg->sizeInBytes);
      printItem (indent, "type:", typeArg->type);
      printFooter (indent);
      if (typeArg->next) {
        printAst (indent, typeArg->next);
      }
      break;
    case CHAR_TYPE:
      charType = (CharType *) p;
      printFooter (indent);
      break;
    case INT_TYPE:
      intType = (IntType *) p;
      printFooter (indent);
      break;
    case DOUBLE_TYPE:
      doubleType = (DoubleType *) p;
      printFooter (indent);
      break;
    case BOOL_TYPE:
      boolType = (BoolType *) p;
      printFooter (indent);
      break;
    case VOID_TYPE:
      voidType = (VoidType *) p;
      printFooter (indent);
      break;
    case TYPE_OF_NULL_TYPE:
      typeOfNullType = (TypeOfNullType *) p;
      printFooter (indent);
      break;
    case ANY_TYPE:
      anyType = (AnyType *) p;
      printFooter (indent);
      break;
    case PTR_TYPE:
      pType = (PtrType *) p;
      printItem (indent, "baseType:", pType->baseType);
      printFooter (indent);
      break;
    case ARRAY_TYPE:
      aType = (ArrayType *) p;
      printIntField (indent, "sizeInBytes:", aType->sizeInBytes);
      printIntField (indent, "sizeOfElements:", aType->sizeOfElements);
      printItem (indent, "sizeExpr:", aType->sizeExpr);
      printItem (indent, "baseType:", aType->baseType);
      printFooter (indent);
      break;
    case RECORD_TYPE:
      rType = (RecordType *) p;
      printIntField (indent, "sizeInBytes:", rType->sizeInBytes);
      printItem (indent, "fields:", rType->fields);
      printIndent (indent+2);
      printf ("fieldMapping: \n");
      if (rType-> fieldMapping) {
        rType-> fieldMapping->print (indent+4);
      } else {
        printf ("NULL\n");
      }
      printFooter (indent);
      break;
    case FUNCTION_TYPE:
      fType = (FunctionType *) p;
      printItem (indent, "parmTypes:", fType-> parmTypes);
      printItem (indent, "retType:", fType-> retType);
      printIntField (indent, "totalParmSize:", fType->totalParmSize);
      printIntField (indent, "retSize:", fType->retSize);
      printFooter (indent);
      break;
    case NAMED_TYPE:
      nType = (NamedType *) p;
      printId (indent, nType->id);
      printItem (indent, "typeArgs:", nType-> typeArgs);
      printPtrField (indent, "myDef:", nType->myDef);
      printIndent (indent+2);
      printf ("subst: ");
      if (nType->subst) {
        printf ("...\n");
        // printf ("\n");
        // nType->subst->print (indent+4);
      } else {
        printf ("NULL\n");
      }
      printFooter (indent);
      break;
    case IF_STMT:
      ifStmt = (IfStmt *) p;
      printItem (indent, "expr:", ifStmt->expr);
      printItem (indent, "thenStmts:", ifStmt->thenStmts);
      printItem (indent, "elseStmts:", ifStmt->elseStmts);
      printFooter (indent);
      if (ifStmt->next) {
        printAst (indent, ifStmt->next);
      }
      break;
    case ASSIGN_STMT:
      assignStmt = (AssignStmt *) p;
      printIntField (indent, "sizeInBytes:", assignStmt->sizeInBytes);
      printIntField (indent, "dynamicCheck:", assignStmt->dynamicCheck);
      printPtrField (indent, "classDef:", assignStmt->classDef);
      printIntField (indent, "arraySize:", assignStmt->arraySize);
      printItem (indent, "lvalue:", assignStmt->lvalue);
      printItem (indent, "expr:", assignStmt->expr);
      printFooter (indent);
      if (assignStmt->next) {
        printAst (indent, assignStmt->next);
      }
      break;
    case CALL_STMT:
      callStmt = (CallStmt *) p;
      printItem (indent, "expr:", callStmt->expr);
      printFooter (indent);
      if (callStmt->next) {
        printAst (indent, callStmt->next);
      }
      break;
    case SEND_STMT:
      sendStmt = (SendStmt *) p;
      printItem (indent, "expr:", sendStmt->expr);
      printFooter (indent);
      if (sendStmt->next) {
        printAst (indent, sendStmt->next);
      }
      break;
    case WHILE_STMT:
      whileStmt = (WhileStmt *) p;
      printCharPtrField (indent, "topLabel:", whileStmt->topLabel);
      printCharPtrField (indent, "exitLabel:", whileStmt->exitLabel);
      printItem (indent, "expr:", whileStmt->expr);
      printItem (indent, "stmts:", whileStmt->stmts);
      printBoolField (indent, "containsAnyBreaks:", whileStmt->containsAnyBreaks);
      printBoolField (indent, "containsAnyContinues:", whileStmt->containsAnyContinues);
      printFooter (indent);
      if (whileStmt->next) {
        printAst (indent, whileStmt->next);
      }
      break;
    case DO_STMT:
      doStmt = (DoStmt *) p;
      printCharPtrField (indent, "exitLabel:", doStmt->exitLabel);
      printCharPtrField (indent, "testLabel:", doStmt->testLabel);
      printItem (indent, "stmts:", doStmt->stmts);
      printItem (indent, "expr:", doStmt->expr);
      printBoolField (indent, "containsAnyBreaks:", doStmt->containsAnyBreaks);
      printBoolField (indent, "containsAnyContinues:", doStmt->containsAnyContinues);
      printFooter (indent);
      if (doStmt->next) {
        printAst (indent, doStmt->next);
      }
      break;
    case BREAK_STMT:
      breakStmt = (BreakStmt *) p;
      printPtrField (indent, "enclosingStmt:", breakStmt->enclosingStmt);
      printFooter (indent);
      if (breakStmt->next) {
        printAst (indent, breakStmt->next);
      }
      break;
    case CONTINUE_STMT:
      continueStmt = (ContinueStmt *) p;
      printPtrField (indent, "enclosingStmt:", continueStmt->enclosingStmt);
      printFooter (indent);
      if (continueStmt->next) {
        printAst (indent, continueStmt->next);
      }
      break;
    case RETURN_STMT:
      returnStmt = (ReturnStmt *) p;
      printItem (indent, "expr:", returnStmt->expr);
      printPtrField (indent, "enclosingMethOrFunction:",
                             returnStmt-> enclosingMethOrFunction);
      printIntField (indent, "retSize:", returnStmt->retSize);
      printFooter (indent);
      if (returnStmt->next) {
        printAst (indent, returnStmt->next);
      }
      break;
    case FOR_STMT:
      forStmt = (ForStmt *) p;
      printCharPtrField (indent, "incrLabel:", forStmt->incrLabel);
      printCharPtrField (indent, "exitLabel:", forStmt->exitLabel);
      printItem (indent, "lvalue:", forStmt->lvalue);
      printItem (indent, "expr1:", forStmt->expr1);
      printItem (indent, "expr2:", forStmt->expr2);
      printItem (indent, "expr3:", forStmt->expr3);
      printItem (indent, "stmts:", forStmt->stmts);
      printBoolField (indent, "containsAnyBreaks:", forStmt->containsAnyBreaks);
      printBoolField (indent, "containsAnyContinues:", forStmt->containsAnyContinues);
      printFooter (indent);
      if (forStmt->next) {
        printAst (indent, forStmt->next);
      }
      break;
    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) p;
      printBoolField (indent, "containsAnyBreaks:", switchStmt->containsAnyBreaks);
      printBoolField (indent, "defaultIncluded:", switchStmt->defaultIncluded);
      printIntField (indent, "lowValue:", switchStmt->lowValue);
      printIntField (indent, "highValue:", switchStmt->highValue);
      printCharPtrField (indent, "exitLabel:", switchStmt->exitLabel);
      printItem (indent, "expr:", switchStmt->expr);
      printItem (indent, "caseList:", switchStmt-> caseList);
      printItem (indent, "defaultStmts:", switchStmt-> defaultStmts);
      printFooter (indent);
      if (switchStmt->next) {
        printAst (indent, switchStmt->next);
      }
      break;
    case TRY_STMT:
      tryStmt = (TryStmt *) p;
      printItem (indent, "stmts:", tryStmt->stmts);
      printItem (indent, "catchList:", tryStmt-> catchList);
      printFooter (indent);
      if (tryStmt->next) {
        printAst (indent, tryStmt->next);
      }
      break;
    case THROW_STMT:
      throwStmt = (ThrowStmt *) p;
      printId (indent, throwStmt->id);
      printPtrField (indent, "myDef:", throwStmt->myDef);
      printItem (indent, "argList:", throwStmt->argList);
      printFooter (indent);
      if (throwStmt->next) {
        printAst (indent, throwStmt->next);
      }
      break;
    case FREE_STMT:
      freeStmt = (FreeStmt *) p;
      printItem (indent, "expr:", freeStmt->expr);
      printFooter (indent);
      if (freeStmt->next) {
        printAst (indent, freeStmt->next);
      }
      break;
    case DEBUG_STMT:
      debugStmt = (DebugStmt *) p;
      printFooter (indent);
      if (debugStmt->next) {
        printAst (indent, debugStmt->next);
      }
      break;
    case CASE:
      cas = (Case *) p;
      printIntField (indent, "ivalue:", cas->ivalue);
      printCharPtrField (indent, "label:", cas->label);
      printItem (indent, "expr:", cas->expr);
      printItem (indent, "stmts:", cas-> stmts);
      printFooter (indent);
      if (cas->next) {
        printAst (indent, cas->next);
      }
      break;
    case CATCH:
      cat = (Catch *) p;
      printId (indent, cat->id);
      printCharPtrField (indent, "label:", cat->label);
      printPtrField (indent, "nextInMethOrFunction:", cat->nextInMethOrFunction);
      printPtrField (indent, "myDef:", cat->myDef);
      printPtrField (indent, "enclosingMethOrFunction:", cat->enclosingMethOrFunction);
      printItem (indent, "parmList:", cat->parmList);
      printItem (indent, "stmts:", cat-> stmts);
      printFooter (indent);
      if (cat->next) {
        printAst (indent, cat->next);
      }
      break;
    case GLOBAL:
      global = (Global *) p;
      printId (indent, global->id);
      printIntField (indent, "sizeInBytes:", global->sizeInBytes);
      printBoolField (indent, "isPrivate:", global->isPrivate);
      printItem (indent, "type:", global->type);
      printItem (indent, "initExpr:", global->initExpr);
      printFooter (indent);
      if (global->next) {
        printAst (indent, global->next);
      }
      break;
    case LOCAL:
      local = (Local *) p;
      printId (indent, local->id);
      printIntField (indent, "offset:", local->offset);
      printIntField (indent, "sizeInBytes:", local->sizeInBytes);
      printCharPtrField (indent, "varDescLabel:", local->varDescLabel);
      printItem (indent, "type:", local->type);
      printItem (indent, "initExpr:", local->initExpr);
      printBoolField (indent, "wasUsed:", local->wasUsed);
      printFooter (indent);
      if (local->next) {
        printAst (indent, local->next);
      }
      break;
    case PARAMETER:
      parm = (Parameter *) p;
      printId (indent, parm->id);
      printIntField (indent, "offset:", parm->offset);
      printIntField (indent, "throwSideOffset:", parm->throwSideOffset);
      printIntField (indent, "sizeInBytes:", parm->sizeInBytes);
      printCharPtrField (indent, "varDescLabel:", parm->varDescLabel);
      printItem (indent, "type:", parm->type);
      printFooter (indent);
      if (parm->next) {
        printAst (indent, parm->next);
      }
      break;
    case CLASS_FIELD:
      classField = (ClassField *) p;
      printId (indent, classField->id);
      printIntField (indent, "offset:", classField->offset);
      printIntField (indent, "sizeInBytes:", classField->sizeInBytes);
      printCharPtrField (indent, "varDescLabel:", classField->varDescLabel);
      printItem (indent, "type:", classField->type);
      printFooter (indent);
      if (classField->next) {
        printAst (indent, classField->next);
      }
      break;
    case RECORD_FIELD:
      recordField = (RecordField *) p;
      printId (indent, recordField->id);
      printIntField (indent, "offset:", recordField->offset);
      printIntField (indent, "sizeInBytes:", recordField->sizeInBytes);
      printCharPtrField (indent, "varDescLabel:", recordField->varDescLabel);
      printItem (indent, "type:", recordField->type);
      printFooter (indent);
      if (recordField->next) {
        printAst (indent, recordField->next);
      }
      break;
    case INT_CONST:
      intConst = (IntConst *) p;
      printIntField (indent, "ivalue:", intConst->ivalue);
      printFooter (indent);
      break;
    case DOUBLE_CONST:
      doubleConst = (DoubleConst *) p;
      printFieldName (indent, "rvalue=");
      printf ("%.17g\n", doubleConst->rvalue);
      printCharPtrField (indent, "nameOfConstant:", doubleConst->nameOfConstant);
      printPtrField (indent, "next=", doubleConst->next);
      printFooter (indent);
      break;
    case CHAR_CONST:
      charConst = (CharConst *) p;
      printFieldName (indent, "ivalue=");
      printf ("%d", charConst->ivalue);
      printf ("\n");
      printFooter (indent);
      break;
    case STRING_CONST:
      stringConst = (StringConst *) p;
      printStringField (indent, "svalue:", stringConst->svalue);
      printPtrField (indent, "next=", stringConst->next);
      printFooter (indent);
      break;
    case BOOL_CONST:
      boolConst = (BoolConst *) p;
      printBoolField (indent, "ivalue:", boolConst->ivalue);
      printFooter (indent);
      break;
    case NULL_CONST:
      nullConst = (NullConst *) p;
      printFooter (indent);
      break;
    case CALL_EXPR:
      callExpr = (CallExpr *) p;
      printId (indent, callExpr->id);
      printSymbolField (indent, "primitiveSymbol:", callExpr->primitiveSymbol);
      printIntField (indent, "retSize:", callExpr->retSize);
      printItem (indent, "argList:", callExpr->argList);
      printPtrField (indent, "myDef:", callExpr->myDef);
      printFooter (indent);
      break;
    case SEND_EXPR:
      sendExpr = (SendExpr *) p;
      printStringField (indent, "selector:", sendExpr->selector);
      printSymbolField (indent, "kind:", sendExpr->kind);
      printSymbolField (indent, "primitiveSymbol:", sendExpr->primitiveSymbol);
      printPtrField (indent, "myProto:", sendExpr->myProto);
      printItem (indent, "receiver:", sendExpr->receiver);
      printItem (indent, "argList:", sendExpr->argList);
      printFooter (indent);
      break;
    case SELF_EXPR:
      selfExpr = (SelfExpr *) p;
      printFooter (indent);
      break;
    case SUPER_EXPR:
      superExpr = (SuperExpr *) p;
      printFooter (indent);
      break;
    case FIELD_ACCESS:
      fieldAccess = (FieldAccess *) p;
      printId (indent, fieldAccess->id);
      printIntField (indent, "offset:", fieldAccess->offset);
      printItem (indent, "expr:", fieldAccess->expr);
      printFooter (indent);
      break;
    case ARRAY_ACCESS:
      arrayAccess = (ArrayAccess *) p;
      printIntField (indent, "sizeOfElements:", arrayAccess->sizeOfElements);
      printItem (indent, "arrayExpr:", arrayAccess->arrayExpr);
      printItem (indent, "indexExpr:", arrayAccess->indexExpr);
      printFooter (indent);
      break;
    case CONSTRUCTOR:
      constructor = (Constructor *) p;
      printSymbolField (indent, "kind:", constructor->kind);
      printSymbolField (indent, "allocKind:", constructor->allocKind);
      printIntField (indent, "sizeInBytes:", constructor->sizeInBytes);
      printPtrField (indent, "myClass:", constructor->myClass);
      printItem (indent, "type:", constructor->type);
      printItem (indent, "countValueList:", constructor->countValueList);
      printItem (indent, "fieldInits:", constructor->fieldInits);
      printFooter (indent);
      break;
    case CLOSURE_EXPR:
      closureExpr = (ClosureExpr *) p;
      printItem (indent, "function:", closureExpr->function);
      printFooter (indent);
      break;
    case VARIABLE_EXPR:
      var = (VariableExpr *) p;
      printStringField (indent, "id:", var->id);
      printPtrField (indent, "myDef:", var->myDef);
      break;
    case AS_PTR_TO_EXPR:
      asPtrToExpr = (AsPtrToExpr *) p;
      printItem (indent, "expr:", asPtrToExpr->expr);
      printItem (indent, "type:", asPtrToExpr->type);
      printFooter (indent);
      break;
    case AS_INTEGER_EXPR:
      asIntegerExpr = (AsIntegerExpr *) p;
      printItem (indent, "expr:", asIntegerExpr->expr);
      printFooter (indent);
      break;
    case ARRAY_SIZE_EXPR:
      arraySizeExpr = (ArraySizeExpr *) p;
      printItem (indent, "expr:", arraySizeExpr->expr);
      printFooter (indent);
      break;
    case IS_INSTANCE_OF_EXPR:
      isInstanceOfExpr = (IsInstanceOfExpr *) p;
      printPtrField (indent, "classDef:", isInstanceOfExpr->classDef);
      printItem (indent, "expr:", isInstanceOfExpr->expr);
      printItem (indent, "type:", isInstanceOfExpr->type);
      printFooter (indent);
      break;
    case IS_KIND_OF_EXPR:
      isKindOfExpr = (IsKindOfExpr *) p;
      printPtrField (indent, "classOrInterface:", isKindOfExpr->classOrInterface);
      printItem (indent, "expr:", isKindOfExpr->expr);
      printItem (indent, "type:", isKindOfExpr->type);
      printFooter (indent);
      break;
    case SIZE_OF_EXPR:
      sizeOfExpr = (SizeOfExpr *) p;
      printItem (indent, "type:", sizeOfExpr->type);
      printFooter (indent);
      break;
    case DYNAMIC_CHECK:
      dynamicCheck = (DynamicCheck *) p;
      printIntField (indent, "kind:", dynamicCheck->kind);
      printIntField (indent, "expectedArraySize:", dynamicCheck->expectedArraySize);
      printIntField (indent, "arraySizeInBytes:", dynamicCheck->arraySizeInBytes);
      printPtrField (indent, "expectedClassDef:", dynamicCheck->expectedClassDef);
      printItem (indent, "expr:", dynamicCheck->expr);
      printFooter (indent);
      break;
    case ARGUMENT:
      arg = (Argument *) p;
      printItem (indent, "expr:", arg->expr);
      printItem (indent, "tempName:", arg->tempName);
      printFooter (indent);
      if (arg->next) {
        printAst (indent, arg->next);
      }
      break;
    case COUNT_VALUE:
      countValue = (CountValue *) p;
      printItem (indent, "count:", countValue->count);
      printItem (indent, "value:", countValue->value);
      printItem (indent, "countTemp:", countValue->countTemp);
      printItem (indent, "valueTemp:", countValue->valueTemp);
      printFooter (indent);
      if (countValue->next) {
        printAst (indent, countValue->next);
      }
      break;
    case FIELD_INIT:
      fieldInit = (FieldInit *) p;
      printId (indent, fieldInit->id);
      printIntField (indent, "offset:", fieldInit->offset);
      printIntField (indent, "sizeInBytes:", fieldInit->sizeInBytes);
      printItem (indent, "expr:", fieldInit->expr);
      printFooter (indent);
      if (fieldInit->next) {
        printAst (indent, fieldInit->next);
      }
      break;

    default:
      printLine (indent, "(********** op is garbage! **********)");
      errorsDetected++;
  }
}



// printHeader (indent, str, p)
//
// This routine indents, then prints a label, then prints str, and then prints
// the p->pos. Finally, a newline is printed.
//
void printHeader (int indent, char * str, AstNode * p) {
  int i = printPtr (p);
  printf (": ");
  i = indent - i - TABAMT;
  printIndent (i);
  printf ("-----%s-----\n", str);
  printFieldName (indent, "token:");
  printf("%s", symbolName (p->tokn.type));
  switch (p->tokn.type) {
    case ID:
      printf(" ");
      printString (stdout, p->tokn.value.svalue);
      break;
    case STRING_CONST:
      printf(" \"");
      printString (stdout, p->tokn.value.svalue);
      printf("\"");
      break;
    case CHAR_CONST:
      printf(" \'");
      printChar (stdout, p->tokn.value.ivalue);
      printf("\'");
      break;
    case INT_CONST:
      printf(" %d", p->tokn.value.ivalue);
      break;
    case DOUBLE_CONST:
      printf(" %.17g", p->tokn.value.rvalue);
      break;
  }
  printf(" (%s: %d)\n", extractFilename (p->tokn), extractLineNumber (p->tokn));

}



// printFooter (indent)
//
// This routine indents, then the closing bracketting symbol.
//
void printFooter (int indent) {
  printIndent (indent);
  printf ("--------------------\n");
}



// printIndent (indent)
//
// This routine prints "indent" spaces.
//
void printIndent (int indent) {
  int i;
  for (i=indent; i>0; i--)
    printf (" ");
}



// printLine (indent, str)
//
// This routine indents, then prints the given string, then prints newline.
//
void printLine (int indent, char * str) {
  printIndent (indent);
  printf ("%s\n", str);
}



// printPtrField (indent, str, p)
//
// This routine indents by "indent" + TABAMT, then prints str, then prints
// the p argument as a pointer, then prints newline.
//
void printPtrField (int indent, char * str, AstNode * p) {
  printIndent (indent+TABAMT);
  printf ("%s ", str);
  printPtr (p);
  printf ("\n");
}



// printIntField (indent, str, i)
//
// This routine indents by "indent" + TABAMT, then prints str, then prints
// the integer i, then prints newline.
//
void printIntField (int indent, char * str, int i) {
  printIndent (indent+TABAMT);
  printf ("%s %d\n", str, i);
}



// printBoolField (indent, str, i)
//
// This routine indents by "indent" + TABAMT, then prints str, then prints
// the integer i (as TRUE or FALSE), then prints newline.
//
void printBoolField (int indent, char * str, int i) {
  printIndent (indent+TABAMT);
  if (i == 0) {
    printf ("%s FALSE\n", str);
  } else if (i == 1) {
    printf ("%s TRUE\n", str);
  } else {
    printf ("%s %d  **********  ERROR: Not 0 or 1  **********\n", str, i);
    errorsDetected++;
  }
}



// printStringField (indent, str1, str2)
//
// This routine indents by "indent" + TABAMT, then prints str1, then prints
// str2, then prints newline.
//
void printStringField (int indent, char * str1, String * str2) {
  printIndent (indent+TABAMT);
  if (str2 == NULL) {
    printf ("%s NULL\n", str1);
  } else {
    printf ("%s ", str1);
    printString (stdout, str2);
    printf ("\n");
  }
}



// printSymbolField (indent, str, sym)
//
// This routine indents by "indent" + TABAMT, then prints str, then prints
// sym, then prints newline.
//
void printSymbolField (int indent, char * str, int sym) {
  printIndent (indent+TABAMT);
  if (sym == 0) {
    printf ("%s NULL\n", str);
  } else {
    printf ("%s %s\n", str, symbolName (sym));
  }
}



// printCharPtrField (indent, str, charPtr)
//
// This routine indents by "indent" + TABAMT, then prints str, then prints
// charPtr, then prints newline.
//
void printCharPtrField (int indent, char * str, char * charPtr) {
  printIndent (indent+TABAMT);
  if (charPtr == NULL) {
    printf ("%s NULL\n", str);
  } else {
    printf ("%s %s\n", str, charPtr);
  }
}



// printId (indent, id)
//
// This routine indents by "indent" + TABAMT, then prints "id:", then prints
// the id argument, then prints newline.
//
void printId (int indent, String * id) {
  printStringField (indent, "id:", id);
}



// printFieldName (indent, str)
//
// This routine indents by "indent" + TABAMT, then prints the given string.  It
// prints no newLine.
//
void printFieldName (int indent, char * str) {
  printIndent (indent+TABAMT);
  printf ("%s ", str);
}



// printItem (indent, str, t)
//
// This routine prints the given string on one line (indented by TABAMT more
// than "indent") and then calls printAst() to print the tree "t" (indented by
// 2*TABAMT more spaces than "indent").
//
void printItem (int indent, char * str, AstNode * t) {
  printIndent (indent+TABAMT);
  printf ("%s", str);
  if (t == NULL) {
    printf (" NULL\n");
  } else {
    printf ("\n");
    printAst (indent+TABAMT+TABAMT, t);
  }
}



// printOperator (indent, op)
//
// This routine is passed a token type in "op".  It prints it out in the form:
//       op= '+'
///
void printOperator (int in, int op) {
  printIndent (in+TABAMT);
  switch (op) {
/*****
    case LEQ:
      printf ("op= LEQ\n");
      return;
    case GEQ:
      printf ("op= GEQ\n");
      return;
    case NEQ:
      printf ("op= NEQ\n");
      return;
    case AND:
      printf ("op= AND\n");
      return;
    case DIV:
      printf ("op= DIV\n");
      return;
    case MOD:
      printf ("op= MOD\n");
      return;
    case NOT:
      printf ("op= NOT\n");
      return;
    case OR:
      printf ("op= OR\n");
      return;
    case '<':
    case '>':
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
      printf ("op= '%c'\n", op);
      return;
*****/
    default:
      printf ("op=***** ERROR: op IS GARBAGE *****\n");
      errorsDetected++;
  }
}



// printPtr (p)
//
// This routine is passed a pointer, possibly NULL.  It prints the pointer.
// The actual address is not printed, since these may vary from run to run.
// Instead, this routine assigns 'labels' to each address and prints the
// same label each time.  It saves the mapping between label and address
// in a static array.
//
int printPtr (AstNode * p) {

#define MAX_LABELS 10000

  static int a [MAX_LABELS];
  static int nextLabel = 1;
  int p1 = (int) p;
  int i;
  if (p == NULL) {
    printf ("NULL");
    return 4;
  }
  for (i=1; i<nextLabel && i<MAX_LABELS; i++) {
    if (a [i] == p1) {
      printf ("#%d", i);
      if (i<10) return 2;
      if (i<100) return 3;
      if (i<1000) return 4;
      if (i<10000) return 5;
      return 6;
    }
  }
  if (nextLabel == MAX_LABELS) {
    printf ("**********  Overflow in printPtr!  **********");
    errorsDetected++;
    return 30;
  } else {
    a [nextLabel] = p1;
    i = nextLabel++;
    printf ("#%d", i);
    if (i<10) return 2;
    if (i<100) return 3;
    if (i<1000) return 4;
    if (i<10000) return 5;
    return 6;
  }
}



// fpretty (p)
//
// This routine is passed a type node.  It prints it on stderr, not followed
// by a return.  It will deal with a NULL argument, but it should be passed
// a legal type otherwise.
//
void fpretty (Type * p) {
  fpretty2 (p, 1);
}



// fpretty2 (p)
//
// This routine does the work.
//
// Normally, we print defined types out like:
//    MyType = record f: int endRecord
// The "wantRecursion" parameter is to prevent recursion when printing types like:
//    type MyRec = record
//                   next: ptr to MyRec
//                 endRecord
// We want to print just the NamedType, like this:
//    MyRec = record next: ptr to MyRec endRecord
//
void fpretty2 (Type * p, int wantRecursion) {

    PtrType * pType;
    ArrayType * aType;
    RecordType * rType;
    FunctionType * fType;
    NamedType * nType;

    RecordField * field;
    TypeParm * typeParm;
    TypeArg * typeArg;

  if (p==NULL) {
    fprintf (stderr, "***** TYPE IS MISSING *****");
    return;
  }

  switch (p->op) {

    case CHAR_TYPE:
      fprintf (stderr, "char");
      return;

    case INT_TYPE:
      fprintf (stderr, "int");
      return;

    case DOUBLE_TYPE:
      fprintf (stderr, "double");
      return;

    case BOOL_TYPE:
      fprintf (stderr, "bool");
      return;

    case VOID_TYPE:
      fprintf (stderr, "void");
      return;

    case TYPE_OF_NULL_TYPE:
      fprintf (stderr, "typeOfNull");
      return;

    case ANY_TYPE:
      fprintf (stderr, "anyType");
      return;

    case PTR_TYPE:
      pType = (PtrType *) p;
      fprintf (stderr, "ptr to ");
      fpretty2 (pType->baseType, 0);  // wantRecursion = false
      return;

    case ARRAY_TYPE:
      aType = (ArrayType *) p;
      if (aType->sizeExpr) {
        if (aType->sizeExpr->op == INT_CONST) {
          fprintf (stderr, "array [%d] of ", ((IntConst *) (aType->sizeExpr))->ivalue);
        } else {
          fprintf (stderr, "array [...] of ");
        }
      } else {
        fprintf (stderr, "array [*] of ");
      }
      fpretty2 (aType->baseType, 0);  // wantRecursion = false
      return;

    case RECORD_TYPE:
      rType = (RecordType *) p;
      fprintf (stderr, "record ");
      for (field = rType->fields; field != NULL; field = (RecordField *) field->next) {
        printString (stderr, field->id);
        fprintf (stderr, ": ");
        fpretty2 (field->type, 0);  // wantRecursion = false
        if (field->next) {
          fprintf (stderr, ", ");
        }
      }
      fprintf (stderr, " endRecord");
      return;

    case FUNCTION_TYPE:
      fType = (FunctionType *) p;
      fprintf (stderr, "function (");
      for (typeArg = fType->parmTypes; typeArg != NULL; typeArg = typeArg->next) {
        fpretty2 (typeArg->type, 0);  // wantRecursion = false
        if (typeArg->next) {
          fprintf (stderr, ", ");
        }
      }
      fprintf (stderr, ") returns ");
      fpretty2 (fType->retType, 0);  // wantRecursion = false
      return;

    case TYPE_PARM:
      typeParm = (TypeParm *) p;
      printString (stderr, typeParm->id);
      fprintf (stderr, ": ");
      fpretty2 (typeParm->type, 0);  // wantRecursion = false
      return;

    case NAMED_TYPE:
      nType = (NamedType *) p;
      printString (stderr, nType->id);
      if (nType->typeArgs) {
        fprintf (stderr, " [");
        for (typeArg = nType->typeArgs; typeArg != NULL; typeArg = typeArg->next) {
          fpretty2 (typeArg->type, 0);  // wantRecursion = false
          if (typeArg->next) {
            fprintf (stderr, ", ");
          }
        }
        fprintf (stderr, "]");
      }
      if ((nType->myDef) &&
          (nType->myDef->op == TYPE_PARM)) {
        fprintf (stderr, ": ");
        fpretty2 (((TypeParm *) nType->myDef)->type, 0);  // wantRecursion = false
      } else if ((nType->myDef) &&
          (nType->myDef->op == TYPE_DEF)) {
        if (wantRecursion) {
          fprintf (stderr, " = ");
          fpretty2 (((TypeDef *) nType->myDef)->type, 0);  // wantRecursion = false
        }
      }
      return;

    default:
      printf("\np->op = %s\n", symbolName (p->op));
      programLogicError ("Unexpected OP in fpretty");
      errorsDetected++;
  }
}
