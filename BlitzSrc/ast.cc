// ast.cc  --  Methods for AstNode classes
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
//

#include "main.h"



//----------  AstNode  ----------

void AstNode::prettyPrint (int indent) {
  programLogicError ("Routine prettyPrint should have been overridden (AstNode)");
}



//----------  Header  ----------

void Header::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("header \"");
  printString (stdout, packageName);
  printf ("\"\n\n");
  if (uses) {
    ppIndent (indent+2);
    printf ("uses\n");
    uses->prettyPrint (indent+4);
    printf ("\n");
  }
  printConsts (indent+2, consts);
  if (consts) printf ("\n");
  printErrorDecls (indent+2, errors);
  if (errors) printf ("\n");
  printVarDecls (indent+2, globals);
  if (globals) printf ("\n");
  printTypeDefs (indent+2, typeDefs);
  if (typeDefs) printf ("\n");
  printFunctionProtos (indent+2, functionProtos);
  if (functionProtos) printf ("\n");
  printFunctions (indent+2, functions);
  if (closures) {
    ppIndent (indent+2);
    printf ("closures\n\n");
    printFunctions (indent+4, closures);
  }
  if (interfaces) {
    interfaces->prettyPrint (indent+2);
  }
  if (classes) {
    classes->prettyPrint (indent+2);
  }
  ppIndent (indent);
  printf ("endHeader\n");
}



//----------  Code  ----------

void Code::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("code \"");
  printString (stdout, packageName);
  printf ("\"\n");
  printErrorDecls (indent+2, errors);
  printConsts (indent+2, consts);
  printVarDecls (indent+2, globals);
  printTypeDefs (indent+2, typeDefs);
  printFunctions (indent+2, functions);
  if (interfaces) {
    interfaces->prettyPrint (indent+2);
  }
  if (classes) {
    classes->prettyPrint (indent+2);
  }
  if (behaviors) {
    behaviors->prettyPrint (indent+2);
  }
  ppIndent (indent);
  printf ("endCode\n");
}



//----------  Uses  ----------

void Uses::prettyPrint (int indent) {
  ppIndent (indent);
  printString (stdout, id);
  if (renamings) {
    printf ("\n");
    ppIndent (indent+2);
    printf ("renaming\n");
    renamings->prettyPrint (indent+4);
  }
  if (next) {
    printf (",\n");
    next->prettyPrint (indent);
  } else {
    printf ("\n");
  }
}



//----------  Renaming  ----------

void Renaming::prettyPrint (int indent) {
  // This routine begins with an indent, but does not print the final \n.
  ppIndent (indent);
  printString (stdout, from);
  printf (" to ");
  printString (stdout, to);
  if (next) {
    printf (",\n");
    next->prettyPrint (indent);
  }
}



//----------  Abstract  ----------

void Abstract::prettyPrint (int indent) {
  printf ("prettyPrint (Abstract) should have been overridden\n");
}



//----------  Interface  ----------

void Interface::prettyPrint (int indent) {
  MethodProto * p;

  ppIndent (indent);
  // printf ("[%08x:]", this);
  printf ("interface ");
  printString (stdout, id);
  printTypeParms (indent+8, typeParms);
  printf ("\n");
  if (extends) {
    ppIndent (indent+2);
    printf ("extends ");
    extends->prettyPrint (indent+6);
    printf ("\n");
  }
  if (methodProtos) {
    ppIndent (indent+2);
    printf ("messages\n");
    p = methodProtos;
    while (p) {
      ppIndent (indent+4);
      p->prettyPrint (indent+6);
      printf ("\n");
      p = p->next;
    }
  }
  if (inheritedMethodProtos) {
    ppIndent (indent+2);
    printf ("inheritedMethodProtos\n");
    p = inheritedMethodProtos;
    while (p) {
      ppIndent (indent+4);
      p->prettyPrint (indent+6);
      printf ("\n");
      p = p->next;
    }
  }
  if (offsetToSelector) {
    offsetToSelector->printOffsetToSelector("OFFSET-TO-SELECTOR:");
    // selectorToOffset->printSelectorToOffset();
    // unavailableOffsets->printOffsetToSelector("UNAVAILABLE-OFFSETS:");
  }

  // printIndent (indent+2);
  // printf ("selectorMapping:\n");
  // if (selectorMapping) {
  //   selectorMapping->print (indent+4);
  // } else {
  //   printf ("NULL\n");
  // }

  ppIndent (indent);
  printf ("endInterface\n\n");
  if (next) {
    next->prettyPrint (indent);
  }
}



//----------  ClassDef  ----------

void ClassDef::prettyPrint (int indent) {
  MethodProto * p;
  Method * meth;
  VarDecl * f;

  ppIndent (indent);
  // printf ("[%08x:]", this);
  printf ("class ");
  printString (stdout, id);
  printTypeParms (indent+8, typeParms);
  // printf (" [sizeInBytes: %d]", sizeInBytes);
  printf ("\n");
  if (implements) {
    ppIndent (indent+2);
    printf ("implements ");
    implements->prettyPrint (indent+6);
    printf ("\n");
  }
  if (superclass) {
    ppIndent (indent+2);
    printf ("superclass ");
    superclass->prettyPrint (indent+12);
    printf ("\n");
  }
  ppIndent (indent+2);
  printf ("typeOfSelf = ");
  pretty (typeOfSelf);
  if (fields) {
    ppIndent (indent+2);
    printf ("fields\n");
    f = fields;
    while (f != NULL) {
      f->prettyPrint (indent+4);
      f = f->next;
    }
  }
  if (methodProtos) {
    ppIndent (indent+2);
    printf ("methods\n");
    p = methodProtos;
    while (p) {
      ppIndent (indent+4);
      p->prettyPrint (indent+6);
      printf ("\n");
      p = p->next;
    }
  }
  // Methods come from the behavior, not the syntax.
  if (methods) {
    meth = methods;
    while (meth) {
      meth->prettyPrint (indent+2);
      meth = meth->next;
    }
  }

  // ppIndent (indent);
  // printf ("=====  localMethodMapping  =====\n");
  // localMethodMapping->print (indent+4);

  // ppIndent (indent);
  // printf ("=====  selectorMapping  =====\n");
  // selectorMapping->print (indent+4);

  if (offsetToSelector) {
    offsetToSelector->printOffsetToSelector("OFFSET-TO-SELECTOR:");
    // selectorToOffset->printSelectorToOffset();
    // unavailableOffsets->printOffsetToSelector("UNAVAILABLE-OFFSETS:");
  }

  // printIndent (indent+2);
  // printf ("selectorMapping:\n");
  // if (selectorMapping) {
  //   selectorMapping->print (indent+4);
  // } else {
  //   printf ("NULL\n");
  // }

  ppIndent (indent);
  printf ("endClass\n\n");
  if (next) {
    next->prettyPrint (indent);
  }
}



//----------  Behavior  ----------

void Behavior::prettyPrint (int indent) {
  Method * p;

  printf ("\n");
  ppIndent (indent);
  printf ("behavior ");
  printString (stdout, id);
  printf ("\n");

  if (methods) {
    p = methods;
    while (p) {
      p->prettyPrint (indent+2);
      p = p->next;
    }
  }

  ppIndent (indent);
  printf ("endBehavior\n");
  if (next) {
    next->prettyPrint (indent);
  }
}



//----------  TypeDef  ----------

void TypeDef::prettyPrint (int indent) {
  ppIndent (indent);
  // printf ("[%08x:]", this);
  printString (stdout, id);
  printf (" = ");
  type->prettyPrint (indent+2);
  printf ("\n");
}



//----------  ConstDecl  ----------

void ConstDecl::prettyPrint (int indent) {
  ppIndent (indent);
  printString (stdout, id);
  // printf (" [%08x] ", this);
  printf (" = ");
  expr->prettyPrint (indent+2);
  printf ("\n");
}



//----------  ErrorDecl  ----------

void ErrorDecl::prettyPrint (int indent) {
  ppIndent (indent);
  // printf (" [ %08x: ] ", this);
  printString (stdout, id);
  printParmList (indent+8, parmList);
  printf ("\n");
}



//----------  FunctionProto  ----------

void FunctionProto::prettyPrint (int indent) {
  ppIndent (indent);
  if (isExternal) {
    printf ("external ");
  }
  if (id != NULL) {
    printString (stdout, id);
  }
  // printf (" [%08x] ", this);
  printParmList (indent+8, parmList);
  if (!isVoidType (retType)) {
    printf (" returns ");
    retType->prettyPrint (indent+12);
  }
  // printf (" [myFunction=%08x] ", myFunction);
  printf ("\n");
}



//----------  MethodProto  ----------

void MethodProto::prettyPrint (int indent) {
  if (kind == NORMAL) {
  } else if (kind == KEYWORD) {
    //  This prints like "at:put: (x:int, y:int)", which is adequate...
  } else if (kind == INFIX) {
    printf ("infix ");
  } else if (kind == PREFIX) {
    printf ("prefix ");
  } else {
    printf ("**********  unknown kind in methodProto  ********** ");
  }
  if (selector != NULL) {
    printString (stdout, selector);
  }
  printParmList (indent+8, parmList);
  if (!isVoidType (retType)) {
    printf (" returns ");
    retType->prettyPrint (indent+12);
  }
  // printf ("\t\tthis = %08x ", this);
  // printf ("    myMethod = %08x", myMethod);
  // if (!myMethod) {
  //   printf ("**********  myMethod is NULL  ********** ");
  // }
}



//----------  MethOrFunction  ----------

void MethOrFunction::prettyPrint (int indent) {
  printf ("prettyPrint (MethOrFunction) not implemented\n");
}



//----------  Function  ----------

void Function::prettyPrint (int indent) {
  Catch * cat;
  ppIndent (indent);
  printf ("function");
  if (id != NULL) {
    printf (" ");
    printString (stdout, id);
  }
  // printf (" [%08x] ", this);
  printParmList (indent+8, parmList);
  if (!isVoidType (retType)) {
    printf (" returns ");
    retType->prettyPrint (indent+12);
  }
  // printf (" [maxArgBytes=%08x] ", maxArgBytes);
  // printf (" [myProto=%08x] ", myProto);
  // printf (" [sizeInBytes: %d]", sizeInBytes);
  // printf (" [containsTry: %d]", containsTry);
  printf ("\n");

  for (cat=catchList; cat; cat=cat->nextInMethOrFunction) {
    ppIndent (indent+6);
    printf ("catches %s  label=%s\n", cat->id->chars, cat->label);
  }
  printVarDecls (indent+4, locals);
  printStmtList (indent+4, stmts);
  ppLine (indent+2, "endFunction");
}



//----------  Method  ----------

void Method::prettyPrint (int indent) {
  Catch * cat;
  ppIndent (indent);
  printf ("method ");
  if (kind == NORMAL) {
  } else if (kind == KEYWORD) {
    //  This prints like "at:put: (x:int, y:int)", which is adequate...
  } else if (kind == INFIX) {
    printf ("infix ");
  } else if (kind == PREFIX) {
    printf ("prefix ");
  } else {
    printf ("**********  unknown kind in methodProto  ********** ");
  }
  if (selector != NULL) {
    printString (stdout, selector);
  }
  printParmList (indent+8, parmList);
  if (!isVoidType (retType)) {
    printf (" returns ");
    retType->prettyPrint (indent+8);
  }
  // printf ("\t\tthis = %08x ", this);
  // printf ("   myMethodProto = %08x", myMethodProto);
  // printf (" [containsTry: %d]", containsTry);
  // if (! myMethodProto) {
  //   printf ("**********  myMethodProto is NULL  ********** ");
  // }
  // printf ("   sizeInBytes = %d", sizeInBytes);

  printf ("\n");

  for (cat=catchList; cat; cat=cat->nextInMethOrFunction) {
    ppIndent (indent+6);
    printf ("catches %s  label=%s\n", cat->id->chars, cat->label);
  }
  printVarDecls (indent+4, locals);
  printStmtList (indent+4, stmts);
  ppLine (indent+2, "endMethod");
}



//----------  TypeParm  ----------

void TypeParm::prettyPrint (int indent) {
  // printf ("[%08x:]", this);
  printString (stdout, id);
  printf (": ");
  type->prettyPrint (indent);
  if (fourByteRestricted) {
    printf ("[fourByteRestricted]");
  }
  if (next != NULL) {
    printf (", ");
    next->prettyPrint (indent);
  }
}



//----------  TypeArg  ----------

void TypeArg::prettyPrint (int indent) {
  type->prettyPrint (indent);
  if (next != NULL) {
    printf (", ");
    next->prettyPrint (indent);
  }
}



//----------  Type  ----------

void Type::prettyPrint (int indent) {
  printf ("prettyPrint (Type) should have been overridden\n");
}



//----------  CharType  ----------

void CharType::prettyPrint (int indent) {
  printf ("char");
}



//----------  IntType  ----------

void IntType::prettyPrint (int indent) {
  printf ("int");
}



//----------  DoubleType  ----------

void DoubleType::prettyPrint (int indent) {
  printf ("double");
}



//----------  BoolType  ----------

void BoolType::prettyPrint (int indent) {
  printf ("bool");
}



//----------  VoidType  ----------

void VoidType::prettyPrint (int indent) {
  printf ("void");
}



//----------  TypeOfNullType  ----------

void TypeOfNullType::prettyPrint (int indent) {
  printf ("typeOfNull");
}



//----------  AnyType  ----------

void AnyType::prettyPrint (int indent) {
  printf ("anyType");
}



//----------  PtrType  ----------

void PtrType::prettyPrint (int indent) {
  printf ("ptr to ");
  baseType->prettyPrint (indent);
}



//----------  ArrayType  ----------

void ArrayType::prettyPrint (int indent) {
  printf ("array [");
  if (sizeExpr == NULL) {
    printf ("*");
  } else {
    sizeExpr->prettyPrint (indent+2);
  }
  //printf (" (sizeInBytes=%d)", sizeInBytes);
  //printf (" (sizeOfElements=%d)", sizeOfElements);
  printf ("] of ");
  baseType->prettyPrint (indent);
}



//----------  RecordType  ----------

void RecordType::prettyPrint (int indent) {
  VarDecl * field;
  printf ("record\n");
  for (field = fields; field != NULL; field = field->next) {
    field->prettyPrint (indent+2);
/***
    ppIndent (indent+2);
    printString (stdout, field->id);
    printf (": ");
    field->type->prettyPrint (indent+4);
***/
//  if (field->next) {
      printf ("\n");
//  } else {
//    printf (" ");
//  }
  }

  // ppIndent (indent);
  // printf ("=====  fieldMapping  =====\n");
  // fieldMapping->print (indent+4);

  ppIndent (indent);
  printf ("endRecord");
  // printf (" [sizeInBytes: %d]", sizeInBytes);
}



//----------  FunctionType  ----------

void FunctionType::prettyPrint (int indent) {
  printf ("function (");
  if (parmTypes != NULL) {
    parmTypes->prettyPrint (indent);
  }
  printf (")");
  if (retType == NULL) {
    printf ("**********  retType is NULL!  **********");
  } else if (! isVoidType (retType)) {
    printf (" returns ");
    retType->prettyPrint (indent);
  }
}



//----------  NamedType  ----------

void NamedType::prettyPrint (int indent) {
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  if (typeArgs != NULL) {
    printf (" [");
    typeArgs->prettyPrint (indent);
    printf ("]");
  }
  // printf ("[myDef=%08x]", myDef);
  // if (myDef == NULL) {
  //   printf ("**********  myDef == NULL  **********");
  // }
  // printf ("\n=====  subst  =====\n");
  // subst->print (indent+4);
  // ppIndent (indent);
}



//----------  Statement  ----------

void Statement::prettyPrint (int indent) {
  printf ("prettyPrint (Statement) not implemented\n");
}



//----------  IfStmt  ----------

void IfStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("if ");
  expr->prettyPrint (indent+8);
  printf ("\n");
  printStmtList (indent+2, thenStmts);
  if (elseStmts != NULL) {
    ppLine (indent, "else");
    printStmtList (indent+2, elseStmts);
  }
  ppLine (indent, "endIf");
}



//----------  AssignStmt  ----------

void AssignStmt::prettyPrint (int indent) {
  ppIndent (indent);
  lvalue->prettyPrint (indent+8);
  printf (" = ");
  expr->prettyPrint (indent+8);
  // printf (" [size=%d] ", sizeInBytes);
  // if (dynamicCheck != 0) {
  //   printf ("   -- dynamicCheck = %d", dynamicCheck);
  // }
  printf ("\n");
}



//----------  CallStmt  ----------

void CallStmt::prettyPrint (int indent) {
  ppIndent (indent);
  if (expr) {
    expr->prettyPrint (indent+4);
  } else {
    printf ("**********  callStmt encountered with expr == NULL  **********");
  }
  printf ("\n");
}



//----------  SendStmt  ----------

void SendStmt::prettyPrint (int indent) {
  ppIndent (indent);
  expr->prettyPrint (indent+4);
  printf ("\n");
}



//----------  WhileStmt  ----------

void WhileStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("while ");
  expr->prettyPrint (indent+8);
  printf ("\n");
  printStmtList (indent+2, stmts);
  ppLine (indent, "endWhile");
}



//----------  DoStmt  ----------

void DoStmt::prettyPrint (int indent) {
  ppLine (indent, "do");
  printStmtList (indent+2, stmts);
  ppIndent (indent);
  printf ("until ");
  expr->prettyPrint (indent+8);
  printf ("\n");
}



//----------  BreakStmt  ----------

void BreakStmt::prettyPrint (int indent) {
  ppIndent (indent);
  // printf ("[enclosingStmt=0x%08x] ", enclosingStmt);
  if (enclosingStmt == NULL) {
    printf ("  ***************  enclosingStmt==NULL  *************** ");
  }
  printf ("break\n");
}



//----------  ContinueStmt  ----------

void ContinueStmt::prettyPrint (int indent) {
  ppIndent (indent);
  // printf ("[enclosingStmt=0x%08x] ", enclosingStmt);
  if (enclosingStmt == NULL) {
    printf ("  ***************  enclosingStmt==NULL  *************** ");
  }
  printf ("continue\n");
}



//----------  ReturnStmt  ----------

void ReturnStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("return ");
  if (expr) {
    expr->prettyPrint (indent+8);
  }
  if (enclosingMethOrFunction == NULL) {
    printf ("  ***************  enclosingMethOrFunction==NULL  *************** ");
  }
  // printf (" [enclosingMethOrFunction = 0x%08x]", enclosingMethOrFunction);
  printf ("\n");
}



//----------  ForStmt  ----------

void ForStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("for ");
  lvalue->prettyPrint (indent+8);
  printf (" = ");
  expr1->prettyPrint (indent+8);
  printf (" to ");
  expr2->prettyPrint (indent+8);
  if (expr3 != NULL) {
    printf (" by ");
    expr3->prettyPrint (indent+8);
  }
  printf ("\n");
  printStmtList (indent+2, stmts);
  ppLine (indent, "endFor");
}



//----------  SwitchStmt  ----------

void SwitchStmt::prettyPrint (int indent) {
  Case * cas;
  ppIndent (indent);
  printf ("switch ");
  expr->prettyPrint (indent+8);
  // printf (" [lowValue=%d, highValue=%d]", lowValue, highValue);
  printf ("\n");
  cas = caseList;
  while (cas != NULL) {
    cas->prettyPrint (indent+2);
    cas = cas->next;
  }
  if (defaultIncluded) {
    ppLine (indent+2, "default:");
    printStmtList (indent+4, defaultStmts);
  }
  ppLine (indent, "endSwitch");
}



//----------  TryStmt  ----------

void TryStmt::prettyPrint (int indent) {
  Catch * cat;
  ppLine (indent, "try");
  printStmtList (indent+4, stmts);
  cat = catchList;
  while (cat != NULL) {
    cat->prettyPrint (indent+2);
    cat = cat->next;
  }
  ppLine (indent, "endTry");
}



//----------  ThrowStmt  ----------

void ThrowStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("throw ");
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  printf (" (");
  if (argList) {
    argList->prettyPrint (indent);
  }
  printf (")");
  // if (myDef == NULL) {
  //   printf (" **********  myDef IS NULL  **********");
  // }
  // printf (" [myDef=%08x]", myDef);
  printf ("\n");
}



//----------  FreeStmt  ----------

void FreeStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("free (");
  expr->prettyPrint (indent);
  printf (")\n");
}



//----------  DebugStmt  ----------

void DebugStmt::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("debug\n");
}



//----------  Case  ----------

void Case::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("case ");
  expr->prettyPrint (indent+8);
  // printf (" [ivalue=%d]", ivalue);
  printf (":\n");
  printStmtList (indent+2, stmts);
}



//----------  Catch  ----------

void Catch::prettyPrint (int indent) {
  ppIndent (indent);
  printf ("catch ");
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  printParmList (indent+8, parmList);
  printf (":");
  // if (myDef == NULL) {
  //   printf (" **********  myDef IS NULL  **********");
  // }
  // printf (" [myDef=%08x]", myDef);
  printf ("\n");
  printStmtList (indent+2, stmts);
}



//----------  VarDecl  ----------

void VarDecl::prettyPrint (int indent) {
  printf ("prettyPrint (VarDecl) not implemented\n");
}



//----------  Global  ----------

void Global::prettyPrint (int indent) {
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  // printf (" [%08x] ", this);
  printf (": ");
  type->prettyPrint (indent);
  if (initExpr != NULL) {
  printf (" = ");
    initExpr->prettyPrint (indent);
  }
  if (offset != -1) {
    printf (" **********  offset is not -1  **********");
  }
  // if (sizeInBytes < 0) {
  //   printf (" **********  sizeInBytes is missing  **********");
  // }
  // printf ("   [size=%d]", sizeInBytes);
}



//----------  Local  ----------

void Local::prettyPrint (int indent) {
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  // printf (" [%08x] ", this);
  printf (": ");
  if (type) {
    type->prettyPrint (indent);
  } else {
    printf ("TEMPORARY");
  }
  if (initExpr != NULL) {
  printf (" = ");
    initExpr->prettyPrint (indent);
  }
  // if (offset > -9) {
  //   printf (" **********  offset is > -9  **********");
  // }
  // if (sizeInBytes < 0) {
  //   printf (" **********  sizeInBytes is missing  **********");
  // }
  // printf ("   [offset=%d, size=%d] ", offset, sizeInBytes);
  // printf (" [offset= %d]", offset);
}



//----------  Parameter  ----------

void Parameter::prettyPrint (int indent) {
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  // printf (" [%08x] ", this);
  printf (": ");
  type->prettyPrint (indent);
  // printf (" [offset= %d]", offset);
  // if (offset < 0) {
  //   printf (" **********  offset is missing  **********");
  // }
  // if (sizeInBytes < 0) {
  //   printf (" **********  sizeInBytes is missing  **********");
  // }
  // printf ("   [offset=%d, size=%d] ", offset, sizeInBytes);
  if (next) {
    printf (", ");
    next->prettyPrint (indent);
  }
}



//----------  ClassField  ----------

void ClassField::prettyPrint (int indent) {
  ppIndent (indent);
  printString (stdout, id);
  // printf (" [%08x] ", this);
  printf (": ");
  type->prettyPrint (indent);
  // printf ("   [offset= %d] ", offset);
  // if (offset < 0) {
  //   printf (" **********  offset is missing  **********");
  // }
  // if (sizeInBytes < 0) {
  //   printf (" **********  sizeInBytes is missing  **********");
  // }
  // printf ("   [offset=%d, size=%d] ", offset, sizeInBytes);
  printf ("\n");
}



//----------  RecordField  ----------

void RecordField::prettyPrint (int indent) {
  ppIndent (indent);
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  ID IS NULL  **********");
  }
  printf (": ");
  type->prettyPrint (indent);
  // if (offset < 0) {
  //   printf (" **********  offset is missing  **********");
  // }
  // if (sizeInBytes < 0) {
  //   printf (" **********  sizeInBytes is missing  **********");
  // }
  // printf (" [offset= %d]", offset);
  // printf ("   [offset=%d, size=%d] ", offset, sizeInBytes);
}



//----------  Expression  ----------

void Expression::prettyPrint (int indent) {
  printf ("prettyPrint (Expression) not implemented\n");
}



//----------  Constant  ----------

void Constant::prettyPrint (int indent) {
  printf ("prettyPrint (Constant) not implemented\n");
}



//----------  IntConst  ----------

void IntConst::prettyPrint (int indent) {
  printf ("%d", ivalue);
}



//----------  DoubleConst  ----------

void DoubleConst::prettyPrint (int indent) {
  int * p;
  double r = 0.0;

  if (rvalue == (-1.0) / r) {
    printf ("<NegativeInfinity>");
  } else if (rvalue == (+1.0) / r) {
    printf ("<PositiveInfinity>");
  } else if (isnan (rvalue)) {
    printf ("<Not-A-Number>");
  } else if (rvalue == 0.0 && 1.0/rvalue < 0.0) {
    printf ("<NegativeZero>");
  } else {
    printf ("%.16gD", rvalue);
  }
  // p = (int *) & rvalue;
  // printf (" [0x%08x ", *p);
  // p++;
  // printf ("%08x]", *p);  
}



//----------  CharConst  ----------

void CharConst::prettyPrint (int indent) {
  printf ("\'");
  printChar (stdout, ivalue);
  printf ("\'");
  if ((ivalue & 0xffffff00) != 0) {
    printf ("**********  ivalue out of range = 0x%08x  **********", ivalue);
  }
}



//----------  StringConst  ----------

void StringConst::prettyPrint (int indent) {
  printf ("\"");
  printString (stdout, svalue);
  printf ("\"");
}



//----------  BoolConst  ----------

void BoolConst::prettyPrint (int indent) {
  if (ivalue) {
    printf ("true");
  } else {
    printf ("false");
  }
}



//----------  NullConst  ----------

void NullConst::prettyPrint (int indent) {
  printf ("null");
}



//----------  CallExpr  ----------

void CallExpr::prettyPrint (int indent) {
    if (id) {
      printString (stdout, id);
    } else {
      printf ("**********  id IS NULL  **********");
    }
    printf (" (");
    if (argList) {
      argList->prettyPrint (indent);
    }
    printf (")");
    // printf (" [myDef=%08x] ", myDef);
    // if (primitiveSymbol) {
    //   printf (" [primitiveSymbol=%s] ", symbolName (primitiveSymbol));
    // }
}



//----------  SendExpr  ----------

void SendExpr::prettyPrint (int indent) {
  if (kind == INFIX) {
    printf ("(");
    if (receiver) {
      receiver->prettyPrint (indent);
    } else {
      printf ("*****  recvr is null  *****");
    }
    printf (") ");
    if (selector) {
      printString (stdout, selector);
    } else {
      printf ("**********  selector IS NULL in SendExpr  **********");
    }
    printf (" (");
    if (argList) {
      argList->prettyPrint (indent);
    }
    printf (")");
  } else if (kind == PREFIX) {
    if (selector) {
      printString (stdout, selector);
    } else {
      printf ("**********  selector IS NULL in SendExpr  **********");
    }
    printf (" (");
    if (receiver) {
      receiver->prettyPrint (indent);
    } else {
      printf ("*****  recvr is null  *****");
    }
    printf (")");
  } else if (kind == NORMAL) {
    printf ("(");
    if (receiver) {
      receiver->prettyPrint (indent);
    } else {
      printf ("*****  recvr is null  *****");
    }
    printf (").");
    if (selector) {
      printString (stdout, selector);
    } else {
      printf ("**********  selector IS NULL in SendExpr  **********");
    }
    printf (" (");
    if (argList) {
      argList->prettyPrint (indent);
    }
    printf (")");
  } else if (kind == KEYWORD) {
    printf ("(");
    if (receiver) {
      receiver->prettyPrint (indent);
    } else {
      printf ("*****  recvr is null  *****");
    }
    printf (") ");
    if (selector) {
      printString (stdout, selector);
    } else {
      printf ("**********  selector IS NULL in SendExpr  **********");
    }
    printf (" (");
    if (argList) {
      argList->prettyPrint (indent);
    }
    printf (")");
  } else {
    printf ("**********  Invalid kind in SendExpr  **********");
  }
/***
  if (primitiveSymbol) {
    printf (" [primitiveSymbol=%s] ", symbolName (primitiveSymbol));
  }
***/
  // if (myProto) {
  //   printf (" [myProto=%08x] ", myProto);
  // }
// qqqq  Add this back for security while testing...
  // if ((myProto == NULL) && (primitiveSymbol == 0)) {
  //   printf ("**********  myProto and primitiveSymbol are missing  **********");
  // }
}



//----------  SelfExpr  ----------

void SelfExpr::prettyPrint (int indent) {
  printf ("self");
}



//----------  SuperExpr  ----------

void SuperExpr::prettyPrint (int indent) {
  printf ("super");
}



//----------  FieldAccess  ----------

void FieldAccess::prettyPrint (int indent) {
    printf ("(");
    expr->prettyPrint (indent);
    printf (").");
    if (id) {
      printString (stdout, id);
    } else {
      printf ("**********  id IS NULL  **********");
    }
    // printf ("[offset=%d]", offset);
}



//----------  ArrayAccess  ----------

void ArrayAccess::prettyPrint (int indent) {
    printf ("(");
    arrayExpr->prettyPrint (indent+2);
    printf (" [");
    indexExpr->prettyPrint (indent+4);
    printf ("]");
    // printf ("[sizeOfElements=%d]", sizeOfElements);
    printf (")");
}



//----------  Constructor  ----------

void Constructor::prettyPrint (int indent) {

  if (allocKind == NEW) {
    printf ("new ");
  } else if (allocKind == ALLOC) {
    printf ("alloc ");
  } else {
    programLogicError ("Unexpected allocKind in Constructor");
  }

  if (type) {
    type->prettyPrint (indent+8+12);
  } else {
    printf ("**********  type IS NULL  **********");
  }
  // printf (" [%s]", symbolName (kind));     // ARRAY, RECORD, CLASS; EOF=error
  // printf (" [sizeInBytes=%d]", sizeInBytes);
  // if (myClass) {
  //   printf (" [myClass=%s]", myClass->id->chars);
  // } else {
  //   printf (" [myClass=NULL]");  // Will be null for Arrays & records
  // }
  if (countValueList) {
    printf (" {");
    countValueList->prettyPrint (indent+8+12);
    printf ("}");
  } else if (fieldInits) {
    printf (" {");
    fieldInits->prettyPrint (indent+8+12);
    printf ("}");
  }
}



//----------  ClosureExpr  ----------

void ClosureExpr::prettyPrint (int indent) {
  printf ("\n");
  function->prettyPrint (indent);
}



//----------  VariableExpr  ----------

void VariableExpr::prettyPrint (int indent) {
  printString (stdout, id);
  // printf ("[myDef=%08x]", myDef);
}



//----------  AsPtrToExpr  ----------

void AsPtrToExpr::prettyPrint (int indent) {
  printf ("(");
  expr->prettyPrint (indent);
  printf (") asPtrTo ");
  type->prettyPrint (indent);
}



//----------  AsIntegerExpr  ----------

void AsIntegerExpr::prettyPrint (int indent) {
  printf ("(");
  expr->prettyPrint (indent);
  printf (") asInteger");
}



//----------  ArraySizeExpr  ----------

void ArraySizeExpr::prettyPrint (int indent) {
  printf ("(");
  expr->prettyPrint (indent);
  printf (") arraySize");
}



//----------  IsInstanceOfExpr  ----------

void IsInstanceOfExpr::prettyPrint (int indent) {
  printf ("(");
  expr->prettyPrint (indent);
  printf (") isInstanceOf ");
  type->prettyPrint (indent);
}



//----------  IsKindOfExpr  ----------

void IsKindOfExpr::prettyPrint (int indent) {
  printf ("(");
  expr->prettyPrint (indent);
  printf (") isKindOf ");
  type->prettyPrint (indent);
}



//----------  SizeOfExpr  ----------

void SizeOfExpr::prettyPrint (int indent) {
  printf ("sizeOf (");
  type->prettyPrint (indent);
  printf (")");
}



//----------  DynamicCheck  ----------

void DynamicCheck::prettyPrint (int indent) {
  printf ("DynamicCheck (");
  expr->prettyPrint (indent);
  printf (", kind=%d, expectedArraySize=%d, arraySizeInBytes=%d)",
          kind, expectedArraySize, arraySizeInBytes);
}



//----------  Argument  ----------

void Argument::prettyPrint (int indent) {
  expr->prettyPrint (indent);
  if (next != NULL) {
    printf (", ");
    next->prettyPrint (indent);
  }
}



//----------  CountValue  ----------

void CountValue::prettyPrint (int indent) {
  if (count) {
    count->prettyPrint (indent);
    printf (" of ");
  }
  value->prettyPrint (indent);
  if (next != NULL) {
    printf (", ");
    next->prettyPrint (indent);
  }
}



//----------  FieldInit  ----------

void FieldInit::prettyPrint (int indent) {
  if (id) {
    printString (stdout, id);
  } else {
    printf ("**********  id IS NULL  **********");
  }
  printf (" = ");
  expr->prettyPrint (indent);
  // if (offset < 0) {
  //   printf (" **********  offset is missing  **********");
  // }
  // if (sizeInBytes < 0) {
  //   printf (" **********  sizeInBytes is missing  **********");
  // }
  // printf (" [offset=%d, size=%d]", offset, sizeInBytes);
  if (next != NULL) {
    printf (", ");
    next->prettyPrint (indent);
  }
}



// pretty (ast)
//
// This routine prints the given ast node, followed by a newline.  It is used
// in debugging.
//
void pretty (AstNode * ast) {
  if (ast) {
    ast->prettyPrint (4);
    printf ("\n");
  } else {
    printf ("NULL\n");
  }
}



// pretty2 (ast)
//
// This routine prettyPrints the given ast node.  If passed NULL,
// it simply prints it.  It does not print a newline.
//
void pretty2 (int indent, AstNode * ast) {
  if (ast) {
    ast->prettyPrint (10);
  } else {
    printf ("NULL");
  }
}



// printStmtList (in, stmtList)
//
// Print out the list of statments, indented by "in", each on a separate
// line.
//
void printStmtList (int indent, Statement * p) {
  while (p != NULL) {
/****
    fflush (stdout);
    printf ("<<<");
    printf ("p=%08x ", p);
    printf ("p->op=%d ", p->op);
    fflush (stdout);
    printf ("p->op=%s", symbolName (p->op));
    printf (">>>");
    fflush (stdout);
****/
    p->prettyPrint (indent);
    p = p->next;
  }
}



// printVarDecls (in, list)
//
// Print out the list of VarDecls, indented by "in", each on a separate
// line.
//
void printVarDecls (int indent, VarDecl * p) {
  if (p == NULL) {
    return;
  }
  ppIndent (indent);
  printf ("var ");
  p->prettyPrint (indent+4);
  printf ("\n");
  p = p->next;
  while (p != NULL) {
/****
    fflush (stdout);
    printf ("<<<");
    printf ("p=%08x ", p);
    printf ("p->op=%d ", p->op);
    fflush (stdout);
    printf ("p->op=%s", symbolName (p->op));
    printf (">>>");
    fflush (stdout);
****/
    ppIndent (indent+4);
    p->prettyPrint (indent+4);
    printf ("\n");
    p = p->next;
  }
}



// printConsts (in, list)
//
// Print out the list of ConstDecls, indented by "in", each on a separate line.
//
void printConsts (int indent, ConstDecl * p) {
  if (p == NULL) {
    return;
  }
  ppIndent (indent);
  printf ("const\n");
  while (p != NULL) {
    p->prettyPrint (indent+2);
    p = p->next;
  }
}



// printErrorDecls (in, list)
//
// Print out the list of ErrorDecls, indented by "in", each on a separate line.
//
void printErrorDecls (int indent, ErrorDecl * p) {
  if (p == NULL) {
    return;
  }
  ppIndent (indent);
  printf ("errors\n");
  while (p != NULL) {
    p->prettyPrint (indent+2);
    p = p->next;
  }
}



// printTypeDefs (in, list)
//
// Print out the list of TypeDefs, indented by "in", each on a separate line.
//
void printTypeDefs (int indent, TypeDef * p) {
  if (p == NULL) {
    return;
  }
  ppIndent (indent);
  printf ("type\n");
  while (p != NULL) {
    p->prettyPrint (indent+2);
    p = p->next;
  }
}



// printFunctionProtos (in, list)
//
// Print out the list of FunctionProtos, indented by "in", each on a separate
// line.
//
void printFunctionProtos (int indent, FunctionProto * p) {
  if (p == NULL) {
    return;
  }
  ppIndent (indent);
  printf ("functions\n");
  while (p != NULL) {
    p->prettyPrint (indent+2);
    p = p->next;
  }
}



// printFunctions (in, list)
//
// Print out the list of Functions, indented by "in", each on a separate line.
//
void printFunctions (int indent, Function * p) {
  while (p != NULL) {
    p->prettyPrint (indent);
    printf ("\n");
    p = p->next;
  }
}



// printParmList (in, parmList)
//
// Print out the list of Parameter nodes.  For example:
//   ()
//   (x: int, y: double)
// A leading space is printed.
//
void printParmList (int indent, Parameter * parmList) {
  printf (" (");
  if (parmList != NULL) {
    parmList->prettyPrint (indent);
  }
  printf (")");
}



// printTypeParms (in, parmList)
//
// Print out the list of TypeParm nodes.  For example:
//   [T1: Object, T2:Person]
// A leading space is printed.
//
void printTypeParms (int indent, TypeParm * parmList) {
  if (parmList != NULL) {
    printf (" [");
    parmList->prettyPrint (indent);
    printf ("]");
  }
}



// ppIndent (indent)
//
// This routine prints "indent" space characters.
//
void ppIndent (int indent) {
  int i;
  for (i=indent; i>0; i--) {
    printf (" ");
  }
}



// ppLine (indent, str)
//
// This routine indents, then prints the given string, then prints newline.
//
void ppLine (int indent, char * str) {
  ppIndent (indent);
  printf ("%s\n", str);
}
