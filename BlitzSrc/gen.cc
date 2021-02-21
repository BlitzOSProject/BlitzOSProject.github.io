// gen.cc  --  Routines related to IR code generation
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
//

#include "main.h"


// generateIR ()
//
// This routine walks the AST and generates intermediate (IR) code.
//
void generateIR () {
  Header * hdr;
  ErrorDecl * errorDecl;
  ClassDef * cl;
  Interface * inter;
  Global * glob;
  int i;
  Function * fun;
  FunctionProto * funProto;
  Method * meth;
  DoubleConst * doubleConst;
  StringConst * stringConst;
  char * sanitizedPackName, * s, * lab1, * lab2;
  int sawMain = 0;

  IRComment (appendStrings ("Name of package being compiled: ",
             mainHeader->packageName->chars, ""));
  IRComment ("");

  IRComment ("Symbols from runtime.s");
  IRImport ("_putString");
  // IRImport ("_flush");   No longer used
  IRImport ("_heapInitialize");
  IRImport ("_heapAlloc");
  IRImport ("_heapFree");
  IRImport ("_IsKindOf");
  IRImport ("_RestoreCatchStack");
  IRImport ("_PerformThrow");
  IRImport ("_runtimeErrorOverflow");
  IRImport ("_runtimeErrorZeroDivide");
  IRImport ("_runtimeErrorNullPointer");
  IRImport ("_runtimeErrorUninitializedObject");
  IRImport ("_runtimeErrorWrongObject");
  IRImport ("_runtimeErrorWrongObject2");
  IRImport ("_runtimeErrorWrongObject3");
  IRImport ("_runtimeErrorBadObjectSize");
  IRImport ("_runtimeErrorDifferentArraySizes");
  IRImport ("_runtimeErrorWrongArraySize");
  IRImport ("_runtimeErrorUninitializedArray");
  IRImport ("_runtimeErrorBadArrayIndex");
  IRImport ("_runtimeErrorNullPointerDuringCall");
  IRImport ("_runtimeErrorArrayCountNotPositive");
  IRImport ("_runtimeErrorRestoreCatchStackError");
  //  IRImport ("_runtimeErrorThrowHandlerHasReturned");  // Called from runtime.s only
  //  IRImport ("_runtimeErrorFatalThrowError");          // Called from runtime.s only

  // Go through all ErrorDecls and give them new names...
  IRText ();
  IRComment ("ErrorDecls");
  for (hdr = headerList; hdr; hdr = hdr->next) {
    sanitizedPackName = sanitizedPackageName (hdr->packageName);
    for (errorDecl = hdr->errors; errorDecl; errorDecl = errorDecl->next) {
      errorDecl->newName = appendStrings (
                                  "_Error",
                                  sanitizedPackName,
                                  errorDecl->id->chars);
      if (hdr == mainHeader) {
        IRExport (errorDecl->newName);
        IRLabel (errorDecl->newName);
        IRAscii0 (errorDecl->newName);
      } else {
        IRImport (errorDecl->newName);
      }
    }
  }
  IRAlign ();

  // Go through all function protos in used packages and generate IMPORTs...
  IRComment ("Functions imported from other packages");
  for (hdr = headerList; hdr; hdr = hdr->next) {
    if (hdr == mainHeader) continue;
    sanitizedPackName = sanitizedPackageName (hdr->packageName);
    for (funProto = hdr->functionProtos; funProto; funProto = funProto->next) {
      if (funProto->isExternal) {
        funProto->newName = funProto->id->chars;
      } else {
        funProto->newName = appendStrings (
                                  sanitizedPackName,
                                  funProto->id->chars,
                                  "");
      }
      IRImport (funProto->newName);
    }
  }

  // Go through functions in this package and give them new names and
  // Generate EXPORT statements.  For externals, generate IMPORTs...
  IRComment ("Externally visible functions in this package");
  sanitizedPackName = sanitizedPackageName (mainHeader->packageName);
  for (funProto = mainHeader->functionProtos; funProto; funProto = funProto->next) {
    if ((funProto->myFunction == NULL) != (funProto->isExternal)) {
      programLogicError ("funProto->myFunction should be NULL iff function is external");
    }
    if (funProto->id == stringMain) {
      if (funProto->isExternal) {
        programLogicError ("The main function must not be external");
      }
      funProto->newName = funProto->id->chars;
      // printf ("funProto->newName (MAIN) = %s\n", funProto->newName);
      funProto->myFunction->newName = funProto->newName;
      IRExport ("_mainEntry");
      IRExport (funProto->newName);
      sawMain = 1;
    } else if (funProto->isPrivate) {
      funProto->newName = appendStrings (newName ("function"), "_", funProto->id->chars);
      funProto->myFunction->newName = funProto->newName;
      // printf ("Private function %s    newName = %s\n",
      //         funProto->id->chars,
      //         funProto->newName);
    } else {
      funProto->newName = funProto->id->chars;
      // printf ("funProto->newName = %s\n", funProto->newName);
      if (funProto->isExternal) {
        IRImport (funProto->newName);
      } else {
        funProto->newName = appendStrings (
                                sanitizedPackName,
                                funProto->id->chars,
                                "");
        funProto->myFunction->newName = funProto->newName;
        // printf ("funProto->newName = %s\n", funProto->newName);
        IRExport (funProto->newName);
      }
    }
  }

  // Go through nameless functions and give them new names...
  for (fun = mainHeader->closures; fun; fun = fun->next) {
    fun->newName = newName ("NamelessFunction");
    // printf ("Closure newName = %s\n", fun->newName);
  }

  // Go through all classes in other packages.  Give them new names and
  // generate import statements...
  for (hdr = headerList; hdr; hdr = hdr->next) {
    if (hdr == mainHeader) continue;
    sanitizedPackName = sanitizedPackageName (hdr->packageName);
    for (cl = hdr->classes; cl; cl = cl->next) {
      cl->newName = appendStrings (sanitizedPackName,
                                   cl->id->chars,
                                   "");
      IRComment ("The following class and its methods are from other packages");
      IRImport (cl->newName);
      // Go through all methods.  Give them new names and import them...
      i = 1;
      for (meth = cl->methods; meth; meth = meth->next) {
        meth->newName = newMethodName (cl->newName, i);
        IRImport (meth->newName);
        i++;
      }
    }
  }

  // Go through classes in this package.  Give them new names and
  // generate export statements...
  sanitizedPackName = sanitizedPackageName (mainHeader->packageName);
  for (cl = mainHeader->classes; cl; cl = cl->next) {
    cl->newName = appendStrings (sanitizedPackName,
                                 cl->id->chars,
                                 "");
    IRComment ("The following class and its methods are from this package");
    IRExport (cl->newName);
    // Go through all methods.  Give them new names and export them...
    i = 1;
    for (meth = cl->methods; meth; meth = meth->next) {
      meth->newName = newMethodName (cl->newName, i);
      IRExport (meth->newName);
      i++;
    }
  }

  // Go through all interfaces in other packages.  Give them new names and
  // generate import statements...
  IRComment ("The following interfaces are from other packages");
  for (hdr = headerList; hdr; hdr = hdr->next) {
    if (hdr == mainHeader) continue;
    sanitizedPackName = sanitizedPackageName (hdr->packageName);
    for (inter = hdr->interfaces; inter; inter = inter->next) {
      inter->newName = appendStrings (sanitizedPackName,
                                      inter->id->chars,
                                      "");
      IRImport (inter->newName);
    }
  }

  // Go through interfaces in this package.  Give them new names and
  // generate export statements...
  sanitizedPackName = sanitizedPackageName (mainHeader->packageName);
  IRComment ("The following interfaces are from this package");
  for (inter = mainHeader->interfaces; inter; inter = inter->next) {
    inter->newName = appendStrings (sanitizedPackName,
                                    inter->id->chars,
                                    "");
    IRExport (inter->newName);
  }

  // Go through all headers (except mainHeader) and import globals...
  IRComment ("Globals imported from other packages");
  for (hdr = headerList; hdr; hdr = hdr->next) {
    if (hdr == mainHeader) continue;
    sanitizedPackName = sanitizedPackageName (hdr->packageName);
    for (glob = hdr->globals; glob; glob = (Global *) glob->next) {
      // Rename the global...
      glob->id = lookupAndAdd (appendStrings (sanitizedPackName,
                                              glob->id->chars,
                                              ""),
                               ID);
      IRImport (glob->id->chars);
    }
  }

  // Go through all the globals in this package...
  IRComment ("Global variables in this package");
  IRData ();
  sanitizedPackName = sanitizedPackageName (mainHeader->packageName);
  for (glob = mainHeader->globals; glob; glob = (Global *) glob->next) {
    if (glob->isPrivate) {
      // Rename each private global by pre-pending "_Gloabl_"...
      glob->id = lookupAndAdd (appendStrings ("_Global_",
                                              glob->id->chars,
                                              ""),
                               ID);
    } else {
      // Rename each public global by pre-pending the package name,
      // e.g., "_P_MyPackage_".  Also generate an export...
      glob->id = lookupAndAdd (appendStrings (sanitizedPackName,
                                              glob->id->chars,
                                              ""),
                               ID);
      IRExport (glob->id->chars);
    }
  }

  // Generate the code for all aligned global variables...
  for (glob = mainHeader->globals; glob; glob = (Global *) glob->next) {
    if (glob->sizeInBytes >= 4) {
      IRLabel (glob->id->chars);
      if (glob->initExpr) {
        genStaticData (glob->initExpr);
      } else {
        IRSkip (glob->sizeInBytes);
      }
    }
  }

  // Generate the code for all byte (unaligned) global variables...
  for (glob = mainHeader->globals; glob; glob = (Global *) glob->next) {
    if (glob->sizeInBytes < 4) {
      IRLabel (glob->id->chars);
      if (glob->initExpr) {
        genStaticData (glob->initExpr);
      } else {
        IRSkip (glob->sizeInBytes);
      }
    }
  }
  IRAlign ();

  // Generate all String Constants...
  if (stringList) {
    IRComment ("String constants");
    for (stringConst = stringList; stringConst; stringConst = stringConst->next) {
      IRLabel (stringConst->nameOfConstant);
      IRAscii2 (stringConst->svalue);
      IRAlign ();
    }
  }

  IRText ();

  // Generate all Double Constants...
  if (floatList) {
    IRComment ("Floating point constants");
    for (doubleConst = floatList; doubleConst; doubleConst = doubleConst->next) {
      IRLabel (doubleConst->nameOfConstant);
      IRDouble (doubleConst->rvalue);
    }
  }

  // If we have a main function, generate the "_mainEntry" code...
  if (sawMain) {
    IRComment ("");
    IRComment ("=====  MAIN ENTRY POINT  =====");
    IRComment ("");
    IRLabel ("_mainEntry");
    lab2 = newLabel ();
    IRStartCheckVersion (sanitizedPackageName (mainHeader->packageName),
                         mainHeader->hashVal,
                         lab2);
    IRCall ("_heapInitialize");
    IRGoto ("main");
  }

  IRComment ("");
  IRComment ("Source Filename and Package Name");
  IRComment ("");
  IRLabel ("_sourceFileName");
  s = inputFileNames [0];
  if (s) {
    IRAscii0 (s);
  } else {
    programLogicError ("inputFileNames [0] is NULL");
  }
  IRLabel ("_packageName");
  IRAscii0 (mainHeader->packageName->chars);
  IRAlign ();

  // Generate the CheckVersion routine
  lab1 = newLabel ();
  lab2 = newLabel ();
  IRCheckVersion (sanitizedPackName, mainHeader->hashVal, lab1);
  for (hdr = headerList; hdr; hdr = hdr->next) {
    if (hdr == mainHeader) continue;
    IRCallCheckVersion (sanitizedPackageName (hdr->packageName), hdr->hashVal, lab2);
  }
  IREndCheckVersion (lab2);

  // Go through functions in this package and generate code for them...
  for (fun = mainHeader->functions; fun; fun = fun->next) {
    genMethOrFunction (fun);
  }

  // Go through nameless functions in this package and generate code for them...
  for (fun = mainHeader->closures; fun; fun = fun->next) {
    genMethOrFunction (fun);
  }

  // Go through the interfaces in this package and generate descriptors for each...
  for (inter = mainHeader->interfaces; inter; inter = inter->next) {
    genInterface (inter);
  }

  // Go through the classes in this package and generate code for each...
  for (cl = mainHeader->classes; cl; cl = cl->next) {
    genClass (cl);
  }

}



// genInterface (inter)
//
// This routine generates the "Interface Descriptor" for an interface.
//
void genInterface (Interface * inter) {
  char * interfaceNamePtr;
  Interface * otherInter;
  Mapping<Abstract, Abstract > * setOfSupers
               = new Mapping<Abstract, Abstract > (10, NULL);
  Abstract * super;

  IRComment ("");
  IRComment2 ("===============  INTERFACE ", inter->id->chars ,"  ===============");
  IRComment ("");
  IRComment ("Interface descriptor:");
  IRComment ("");

  IRLabel (inter->newName);

  // Write out magic number, ptr to interface name, ptr to source filename,
  // and line number...
  IRWord3 (0x494e5446, "Magic number 0x494e5446 == 'INTF'");
  interfaceNamePtr = newLabel ();
  IRWord2 (interfaceNamePtr);
  IRWord2 ("_sourceFileName");
  IRWord3 (extractLineNumber (inter->tokn), "line number");

  // Identify all interfaces that this interfaces extends...
  addAllSupers (inter, setOfSupers);

  // For each, generate a pointer to its interface descriptor...
  super = setOfSupers->getFirst ();
  while (super) {
    otherInter = (Interface *) super;
    if (otherInter == NULL || otherInter->op != INTERFACE) {
      programLogicError ("Within genInterface - This was checked in check() - 2");
    }
    IRWord2 (otherInter->newName);
    super = setOfSupers->getNext ();
  }
  IRWord (0);

  IRLabel (interfaceNamePtr);
  IRAscii0 (inter->id->chars);
  IRAlign ();

}



// genClass (cl)
//
// This routine generates the code for a class.
//
void genClass (ClassDef * cl) {
  MethodProto * methProto;
  Method * meth;
  char * lab, * classNamePtr;
  Offset * offset;
  String * sel;
  TypeArg * typeArg;
  NamedType * namedType;
  Interface * otherInter;
  Mapping<Abstract, Abstract > * setOfSupers
               = new Mapping<Abstract, Abstract > (10, NULL);
  Abstract * otherAbs;

  IRComment ("");
  IRComment2 ("===============  CLASS ", cl->id->chars ,"  ===============");
  IRComment ("");
  IRComment ("Dispatch Table:");
  IRComment ("");

  IRLabel (cl->newName);

  // Gen ptr to class descriptor...
  lab = newLabel ();
  IRWord2 (lab);

  // Go through the offsets and write a line for each...
  offset = firstDispatchOffset;
  while (1) {
    sel = cl->offsetToSelector->findInTopScope (offset);
    if (sel == NULL) break;
    methProto = cl->selectorMapping->findInTopScope (sel);
    if (methProto == NULL) {
      programLogicError ("Should find a method proto for every offset");
    }
    meth = methProto->myMethod;
    if (meth == NULL) {
//qqqqq -- This program logic error occurs -- See example in folder "Art-error"
      printf ("class = %s\n", cl->id->chars);
      printf ("methProto selector = %s\n", methProto->selector->chars);
      programLogicError ("Should find a method for every method proto");
    }
    IRGoto2 (meth->newName, offset->ivalue, meth->selector->chars);
    offset = nextDispatchOffset (offset);
  }
  IRWord (0);

  // Generate the class descriptor...
  IRComment ("");
  IRComment ("Class descriptor:");
  IRComment ("");
  IRLabel (lab);
  IRWord3 (0x434c4153, "Magic number 0x434c4153 == 'CLAS'");
  classNamePtr = newLabel ();
  IRWord2 (classNamePtr);
  IRWord2 ("_sourceFileName");
  IRWord3 (extractLineNumber (cl->tokn), "line number");
  IRWord3 (cl->sizeInBytes, "size of instances, in bytes");

  // Generate pointers to super-classes and super-interfaces...
  addAllSupers (cl, setOfSupers);
  otherAbs = setOfSupers->getFirst ();
  while (otherAbs) {
    IRWord2 (otherAbs->newName);
    otherAbs = setOfSupers->getNext ();
  }
  IRWord (0);

  IRLabel (classNamePtr);
  IRAscii0 (cl->id->chars);
  IRAlign ();

  // Go through the methods and generate code for each...
  for (meth = cl->methods; meth; meth = meth->next) {
    genMethOrFunction (meth);
  }

}



// addAllSupers (abs, setOfSupers)
//
// This routine is passed an abstract "abs".  It visits each of its superclasses
// superInterfaces and adds each of them to the set "setOfSupers".
//
void addAllSupers (Abstract * abs, Mapping <Abstract, Abstract> * setOfSupers) {
  ClassDef * cl, * supercl;
  Interface * inter, * superInter;
  TypeArg * typeArg;
  NamedType * nType;

  //    printf ("    %s: CALLED\n", abs->id->chars);

  // Put this abstract into the set if it is not already there...
  if (! setOfSupers->alreadyDefined (abs)) {
    //    printf ("    %s: Adding to setOfSupers\n", abs->id->chars);
    setOfSupers->enter (abs, abs);
  }

  // If this routine is called on a class...
  if (abs->op == CLASS_DEF) {
    cl = (ClassDef *) abs;

    // Identify the superclass, if any...
    //    printf ("    %s:   Looking at the superclass...\n", abs->id->chars);
    nType = cl->superclass;
    if (nType &&
        nType->myDef &&
        nType->myDef->op == CLASS_DEF) {
      supercl = (ClassDef *) nType->myDef;
      // Recursively consider adding it to the set...
      addAllSupers (supercl, setOfSupers);
    }

    // Run through all the interfaces this class implements...
    for (typeArg = cl->implements; typeArg; typeArg = typeArg->next) {
      //    printf ("    %s:   Considering next interface in the implements clause...\n",
      //            abs->id->chars);
      nType = (NamedType *) typeArg->type;
      if (nType &&
          nType->op == NAMED_TYPE &&
          nType->myDef &&
          nType->myDef->op == INTERFACE) {
        superInter = (Interface *) nType->myDef;
        //    printf ("    %s:     ... and calling recursively for interface %s...\n",
        //           abs->id->chars, nType->id->chars);
        // Recursively consider adding it to the set...
        addAllSupers (superInter, setOfSupers);
      }
    }

  // Else, if this routine is called on an interface...
  } else if (abs->op == INTERFACE) {
    inter = (Interface *) abs;

    // Run through all the interfaces this interface extends...
    for (typeArg = inter->extends; typeArg; typeArg = typeArg->next) {
      //    printf ("    %s:   Considering next interface in the extends clause...\n",
      //            abs->id->chars);
      nType = (NamedType *) typeArg->type;
      if (nType &&
          nType->op == NAMED_TYPE &&
          nType->myDef &&
          nType->myDef->op == INTERFACE) {
        superInter = (Interface *) nType->myDef;
        //    printf ("    %s:     ... and calling recursively for %s...\n",
        //            abs->id->chars, nType->id->chars);
        // Recursively consider adding it to the set...
        addAllSupers (superInter, setOfSupers);
      }
    }

  // Else, if this routine is called on something else...
  } else {
    programLogicError ("Unexpected node in addAllSupers");
  }
  //    printf ("    %s: DONE\n", abs->id->chars);
}



// genMethOrFunction (methOrFunction)
//
// This routine is passed a method, function, or closure.
// It generates the code for it.
//
void genMethOrFunction (MethOrFunction * methOrFunction) {
  Local * local;
  AstNode * x;
  char * methOrFunName, * labelForSelf;
  char kind;
  Parameter * parm;
  char * nameOfNamelessFunction;
  Function * fun;
  Method * meth;
  Catch * cat;

  currentFunOrMeth = methOrFunction;   // This is where new temps will be added
  IRComment ("");

  if (methOrFunction->op == FUNCTION) {
    fun = (Function *) methOrFunction;
    if (fun->myProto == NULL) {
      nameOfNamelessFunction = (char *) calloc (1,
                                    strlen (extractFilename (fun->tokn)) + 40);
      if (nameOfNamelessFunction == 0) {
        programLogicError ("Calloc failed in genMethOrFunction; out of memory perhaps???");
      }
      sprintf (nameOfNamelessFunction, "Function (%s:%d)",
                                extractFilename (fun->tokn),
                                extractLineNumber (fun->tokn));
      IRComment2 ("===============  Nameless ", nameOfNamelessFunction,
                                                           "  ===============");
    } else {
      IRComment2 ("===============  FUNCTION ", fun->id->chars, "  ===============");
    }
    IRComment ("");
    IRLabel (fun->newName);
    // printf ("PROCESSING FUNCTION %s\n", fun->newName);
    IRFunctionEntry (fun);
    genLineNumber (methOrFunction, "FU");

  } else {
    meth = (Method *) methOrFunction;
    // printf ("PROCESSING METHOD %s\n", meth->newName);
    IRComment2 ("===============  METHOD ", meth->selector->chars, "  ===============");
    IRComment ("");
    IRLabel (meth->newName);
    IRMethodEntry (meth);
    genLineNumber (methOrFunction, "ME");
  }

  if (methOrFunction->containsTry) {
    methOrFunction->catchStackSave = newTemp (4);
    IRSaveCatchStack (methOrFunction->catchStackSave);
  }

  // Run through the locals...
  IRComment ("VARIABLE INITIALIZATION...");
  for (local = methOrFunction->locals; local; local = (Local *) local->next) {
    //     printf ("LOCAL INIT ");
    //     pretty (local);
    if (local == methOrFunction->catchStackSave) {
      // Do not initialize the temp that we just saved CATCH STACK into.
    } else if (local->initExpr) {
      IRComment (local->id->chars);
      genExprInto (local, local->initExpr, NULL, NULL);
    //  The frame will be initialized to zero, so don't bother with this:
    //      } else {
    //        IRComment (local->id->chars);
    //        IRZeroLocal (local);
    }
  }

  // Run through the stmts and generate code for them...
  genStmts (methOrFunction->stmts);

  // Generate the Routine Descriptor...
  IRComment ("");
  IRComment ("Routine Descriptor");
  IRComment ("");
  IRLabel (appendStrings ("_RoutineDescriptor_",methOrFunction->newName,""));
  IRWord2 ("_sourceFileName");
  methOrFunName = newLabel ();
  IRWord2 (methOrFunName);
  IRWord3 (methOrFunction->totalParmSize, "total size of parameters");
  IRFrameSize (methOrFunction);           // Don't know frameSize until later

  // If this is a method, generate the descriptor for "self"...
  if (methOrFunction->op == METHOD) {
    labelForSelf = newLabel ();
    IRWord2 (labelForSelf);
    IRWord3 (4, "size of self");
    IRWord3 (8, "offset of self");
  }

  // Run through the parameters and generate var descriptors...
  for (parm = methOrFunction->parmList; parm; parm = (Parameter *) parm->next) {
    parm->varDescLabel = newLabel ();
    IRVarDesc1 (parm->varDescLabel, parm, parm->sizeInBytes);
  }

  // Run through the locals and generate var descriptors...
  for (local = methOrFunction->locals; local; local = (Local *) local->next) {
    // if (local->type != NULL) {    // Don't include info for temp variables.
      local->varDescLabel = newLabel ();
      IRVarDesc1 (local->varDescLabel, local, local->sizeInBytes);
    // }
  }

  // Run through the parameters for all catch clauses and generate var descriptors...
  for (cat = methOrFunction->catchList; cat; cat = cat->nextInMethOrFunction) {
    for (parm = cat->parmList; parm; parm = (Parameter *) parm->next) {
      parm->varDescLabel = newLabel ();
      IRVarDesc1 (parm->varDescLabel, parm, parm->sizeInBytes);
    }
  }

  IRWord (0);

  // Generate the ASCII characters for the function or method name...
  IRLabel (methOrFunName);
  if (methOrFunction->op == FUNCTION) {
    if (fun->id) {
      IRAscii0 (fun->id->chars);
    } else {   // if closure...
      IRAscii0 (nameOfNamelessFunction);
    }
  } else {
    IRAscii (meth->myClass->id->chars);
    IRAscii ("::");
    IRAscii0 (meth->selector->chars);
    IRAlign ();
    IRLabel (labelForSelf);
    IRAscii0 ("Pself");
  }
  IRAlign ();

  // Run through the parameters and generate var descriptors...
  for (parm = methOrFunction->parmList; parm; parm = (Parameter *) parm->next) {
    genVarDescriptor (parm);
  }

  // Run through the locals and generate var descriptors...
  for (local = methOrFunction->locals; local; local = (Local *) local->next) {
    // if (local->type != NULL) {    // Don't include info for temp variables.
      genVarDescriptor (local);
    // }
  }

  // Run through the parameters for all catch clauses and generate var descriptors...
  for (cat = methOrFunction->catchList; cat; cat = cat->nextInMethOrFunction) {
    for (parm = cat->parmList; parm; parm = (Parameter *) parm->next) {
      genVarDescriptor (parm);
    }
  }
}



// genVarDescriptor (varDecl)
//
// This routine is passed a local or a catch parameter.  It generates the
// appropriate "IRVarDesc2" instruction.
//
void genVarDescriptor (VarDecl * varDecl) {
  char kind;

  if (varDecl->type == NULL) {
    if (varDecl->sizeInBytes == 1) {
      kind = 'C';
    } else {
      kind = '?';
    }
  } else if (isCharType (varDecl->type)) {
    kind = 'C';
  } else if (isIntType (varDecl->type)) {
    kind = 'I';
  } else if (isBoolType (varDecl->type)) {
    kind = 'B';
  } else if (isDoubleType (varDecl->type)) {
    kind = 'D';
  } else if (isPtrType (varDecl->type) || isTypeOfNullType (varDecl->type)) {
    kind = 'P';
  } else if (isArrayType (varDecl->type)) {
    kind = 'A';
  } else if (isRecordType (varDecl->type)) {
    kind = 'R';
  } else if (isObjectType (varDecl->type)) {
    kind = 'O';
  } else {
    programLogicError ("Unexpected type in genVarDescriptor");
  }

  IRVarDesc2 (varDecl->varDescLabel, kind, varDecl->id->chars);
}



// genStaticData (expr)
//
// This routine generates IR code for a static constant, e.g., for a global var.
// It handles these cases:
//    intConst, doubleConst, charConst, boolConst
//    null ptr
//    & glob
//    function
//    new Object
//    new Record
//    new Array
//
void genStaticData (Expression * expr) {
  SendExpr * sendExpr;
  VariableExpr * var;
  Global * glob;
  FunctionProto * funProto;
  Constructor * constructor;
  CountValue * countValue;
  IntConst * intConst;
  int i, n, nextOff;
  FieldInit * f;

  switch (expr->op) {

    case INT_CONST:                                       // in genStaticData

      IRWord (((IntConst *) expr)->ivalue);
      return;

    case DOUBLE_CONST:                                    // in genStaticData

      IRDouble (((DoubleConst *) expr)->rvalue);
      return;

    case CHAR_CONST:                                      // in genStaticData

      IRByte (((CharConst *) expr)->ivalue);
      return;

    case BOOL_CONST:                                      // in genStaticData

      IRByte (((BoolConst *) expr)->ivalue);
      return;

    case NULL_CONST:                                      // in genStaticData

      IRWord (0);
      return;

    case CLOSURE_EXPR:                                    // in genStaticData

      IRWord2 (((ClosureExpr *) expr)->function->newName);
      return;

    case DYNAMIC_CHECK:                                   // in genStaticData

      genStaticData (((DynamicCheck *) expr)->expr);
      return;

    case CONSTRUCTOR:                                     // in genStaticData

      constructor = (Constructor *) expr;
      if (constructor->allocKind != NEW) {
        programLogicError ("We should have eliminated ALLOC during check");
      }

      // If this is "NEW ARRAY"...
      if (constructor->kind == ARRAY) {
        n = 0;
        for (countValue = constructor->countValueList;
             countValue;
             countValue = countValue->next) {
          if (countValue->count == NULL) {
            i = 1;
          } else {
            intConst = (IntConst *) countValue->count;
            if (intConst->op != INT_CONST) {
              programLogicError ("We should have checked this in checkStaticData (5)");
            }
            i = intConst->ivalue;
          }
          n += i;
        }
        IRComment ("Static array");
        IRWord3 (n, "number of elements");
        for (countValue = constructor->countValueList;
             countValue;
             countValue = countValue->next) {
          if (countValue->count == NULL) {
            i = 1;
          } else {
            intConst = (IntConst *) countValue->count;
            i = intConst->ivalue;
          }
          for (; i > 0; i--) {
            genStaticData (countValue->value);
          }
        }
        IRAlign ();

      // Else, if this is "NEW RECORD" or "NEW CLASS"...
      } else if (constructor->kind == RECORD ||
                 constructor->kind == CLASS) {
        if (constructor->kind == RECORD) {
          IRComment ("Static record");
        } else {
          IRComment ("Static object");
          IRWord2 (constructor->myClass->newName);
        }

        // If this is an object initialization with missing field inits...
        if ((constructor->kind == CLASS) &&
            (constructor->fieldInits == NULL)) {
          for (i = constructor->sizeInBytes; i>4; i = i-4) {
            IRWord (0);
          }
        }

        constructor->fieldInits = sortFieldInits (constructor->fieldInits);
        nextOff = 4;
        for (f = constructor->fieldInits; f; f = f->next) {
          // Add padding zeros if necessary...
          if (f->sizeInBytes >= 4) {
            while (nextOff % 4 != 0) {
              IRByte (0);
              nextOff++;
            }
          }
          genStaticData (f->expr);
          nextOff += f->sizeInBytes;
        }
        // Add padding zeros if necessary...
        while (nextOff % 4 != 0) {
          IRByte (0);
          nextOff++;
        }
      } else {
        programLogicError ("Bad constructor->kind in genStaticData");
      }
      return;

    case VARIABLE_EXPR:                                   // in genStaticData

      var = (VariableExpr *) expr;
      funProto = (FunctionProto *) var->myDef;
      if (funProto->op != FUNCTION_PROTO) {
        programLogicError ("We should have checked this in checkStaticData (1)");
      }
      IRWord2 (funProto->newName);
      return;

    case SEND_EXPR:                                       // in genStaticData

      sendExpr = (SendExpr *) expr;
      if (sendExpr->primitiveSymbol == PRIMITIVE_ADDRESS_OF) {
        var = (VariableExpr *) sendExpr->receiver;
        if (var->op != VARIABLE_EXPR) {
          programLogicError ("We should have checked this in checkStaticData (2)");
        }
        glob = (Global *) var->myDef;
        if (glob->op != GLOBAL) {
          programLogicError ("We should have checked this in checkStaticData (3)");
        }
        IRWord2 (glob->id->chars);
        return;
      } else {
        programLogicError ("We should have checked this in checkStaticData (4)");
      }
      break;

  }

  printf ("node->op = %s\n", symbolName (expr->op));
  pretty (expr);
  programLogicError ("Unexpected node in genStaticData");
}



// sortFieldInits (fieldList) -> fieldList
//
// This routine is passed a list of FieldInit nodes.  It sorts them,
// based on their offsets.  (This uses insertion sort, but this shouldn't
// be too time-consuming since record and class sizes should be small.)
//
FieldInit * sortFieldInits (FieldInit * f) {
  FieldInit * oldList, * newList, * g, * newF;

  // printf ("=====SORTING FIELD INITS=====\n");

  if (f == NULL) return NULL;
  oldList = f->next;
  newList = f;
  f->next = NULL;

  //    printf ("oldList = ");
  //    for (f = oldList; f; f = f->next) {
  //      printf ("%s ", f->id->chars);
  //    }
  //    printf ("\n");

  f = oldList;
  while (f) {
    newF = f->next;
    oldList = f->next;
    f->next = NULL;
    //    printf ("=====  EXAMINING f = %s...  =====\n", f->id->chars);
    if (f->offset < newList->offset) {
      f->next = newList;
      newList = f;
      //    printf ("  inserting at front\n");
    } else {
      //    printf ("  searching...\n");
      for (g = newList; ; g = g->next) {
        //    printf ("    considering g = %s...\n", g->id->chars);
        if (g->next == NULL) break;
        if (f->offset < g->next->offset) break;
      }
      //    printf ("    inserting after g = %s...\n", g->id->chars);
      f->next = g->next;
      g->next = f;
    }

    //    printf ("oldList = ");
    //    for (f = oldList; f; f = f->next) {
    //      printf ("%s ", f->id->chars);
    //    }
    //    printf ("\n");

    //    printf ("newList = ");
    //    for (f = newList; f; f = f->next) {
    //      printf ("%s ", f->id->chars);
    //    }
    //    printf ("\n");

    f = newF;
  }

  //    printf ("========================== DONE: newList = ");
  //    for (f = newList; f; f = f->next) {
  //      printf ("%s ", f->id->chars);
  //    }
  //    printf ("\n");

  return newList;

}



// genStmts (stmtList)
//
// This routine generates IR code for a list of statements.
//
void genStmts (Statement * stmt) {
  while (stmt) {
    // printf ("PROCESSING %s\n", symbolName (stmt->op));
    switch (stmt->op) {
      case IF_STMT:
        IRComment ("IF STATEMENT...");
        genLineNumber (stmt, "IF");
        genIfStmt ((IfStmt *) stmt);
        break;
      case ASSIGN_STMT:
        IRComment ("ASSIGNMENT STATEMENT...");
        genLineNumber (stmt, "AS");
        genAssignStmt ((AssignStmt *) stmt);
        break;
      case CALL_STMT:
        IRComment ("CALL STATEMENT...");
        // genLineNumber (stmt, "CA");
        genCallStmt ((CallStmt *) stmt);
        break;
      case SEND_STMT:
        IRComment ("SEND STATEMENT...");
        // genLineNumber (stmt, "SE");
        genSendStmt ((SendStmt *) stmt);
        break;
      case WHILE_STMT:
        IRComment ("WHILE STATEMENT...");
        // genLineNumber (stmt, "WH");
        genWhileStmt ((WhileStmt *) stmt);
        break;
      case DO_STMT:
        IRComment ("DO STATEMENT...");
        // genLineNumber (stmt, "DO");
        genDoStmt ((DoStmt *) stmt);
        break;
      case BREAK_STMT:
        IRComment ("BREAK STATEMENT...");
        genLineNumber (stmt, "BR");
        genBreakStmt ((BreakStmt *) stmt);
        break;
      case CONTINUE_STMT:
        IRComment ("CONTINUE STATEMENT...");
        genLineNumber (stmt, "CO");
        genContinueStmt ((ContinueStmt *) stmt);
        break;
      case RETURN_STMT:
        IRComment ("RETURN STATEMENT...");
        genLineNumber (stmt, "RE");
        genReturnStmt ((ReturnStmt *) stmt);
        break;
      case FOR_STMT:
        IRComment ("FOR STATEMENT...");
        genLineNumber (stmt, "FO");
        genForStmt ((ForStmt *) stmt);
        break;
      case SWITCH_STMT:
        // IRComment ("SWITCH STATEMENT...");
        // genLineNumber (stmt, "SW");
        genSwitchStmt ((SwitchStmt *) stmt);
        break;
      case TRY_STMT:
        IRComment ("TRY STATEMENT...");
        genLineNumber (stmt, "TR");
        genTryStmt ((TryStmt *) stmt);
        break;
      case THROW_STMT:
        IRComment ("THROW STATEMENT...");
        genLineNumber (stmt, "TH");
        genThrowStmt ((ThrowStmt *) stmt);
        break;
      case FREE_STMT:
        IRComment ("FREE STATEMENT...");
        genLineNumber (stmt, "FR");
        genFreeStmt ((FreeStmt *) stmt);
        break;
      case DEBUG_STMT:
        IRComment ("--------------------  DEBUG  --------------------");
        genLineNumber (stmt, "DE");
        IRDebug ();
        break;
      default:
        printf ("\nstmt->op = %s\n", symbolName (stmt->op));
        programLogicError ("Unknown op in genStmts");
    }
    stmt = stmt->next;
  }

}



//
// genIfStmt (ifStmt)
//
void genIfStmt (IfStmt * ifStmt) {
  char * trueLabel, * falseLabel, * endLabel;
  trueLabel = newLabel ();
  falseLabel = newLabel ();
  genExprInto (NULL, ifStmt->expr, trueLabel, falseLabel);
  IRLabel (trueLabel);
  if (ifStmt->thenStmts) {
    IRComment ("THEN...");
    genLineNumber (ifStmt->thenStmts, "TN");
    genStmts (ifStmt->thenStmts);
  }
  if (ifStmt->elseStmts) {
    endLabel = newLabel ();
    IRGoto (endLabel);
    IRLabel (falseLabel);
    IRComment ("ELSE...");
    genLineNumber (ifStmt->elseStmts, "EL");
    genStmts (ifStmt->elseStmts);
    IRComment ("END IF...");
    IRLabel (endLabel);
  } else {
    IRComment ("END IF...");
    IRLabel (falseLabel);
  }
}



//
// genAssignStmt (assignStmt)
//
void genAssignStmt (AssignStmt * assignStmt) {
  AstNode * temp;
  Expression * lvalue;
  VariableExpr * var;
  VarDecl * varDecl, * varDecl2;
  Expression * arg1, * arg2;

  // Look at the L-Value...
  lvalue = assignStmt->lvalue;

  // printf ("ASSIGNMENT STMT (assignStmt->dynamicCheck = %d):\n    ",
  //         assignStmt->dynamicCheck);
  // pretty (assignStmt);

  // Case 0: NORMAL ASSIGNMENT: x = y...
  if (assignStmt->dynamicCheck == 0) {

    if (assignStmt->sizeInBytes < 1) {
      programLogicError ("Error: sizeInBytes < 1 in genAssignStmt");
    }

    // If it is a LOCAL, GLOBAL, CLASS_FIELD, or PARAMETER...
    if (lvalue->op == VARIABLE_EXPR) {
      var = (VariableExpr *) lvalue;
      varDecl = (VarDecl *) var->myDef;
      if (varDecl == NULL) {
        programLogicError ("In genAssignStmt, lvalue->myDef == NULL");
      }
      if ((varDecl->op == LOCAL) ||
          (varDecl->op == GLOBAL) ||
          (varDecl->op == CLASS_FIELD) ||
          (varDecl->op == PARAMETER)) {
        // Call GenExprInto to do the work...
        genExprInto (varDecl, assignStmt->expr, NULL, NULL);
        return;
      }
    }

    // Else, we have something such as "x.field = y"...
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the data area
    temp = genExpr (assignStmt->expr, assignStmt->sizeInBytes);
    IRMove (NULL,  varDecl, temp, NULL, assignStmt->sizeInBytes);


  // Case 1: OBJECT ASSIGNMENT: *objPtr = *objPtr...
  } else if (assignStmt->dynamicCheck == 1) {
    //   printf ("Case 1: OBJECT ASSIGNMENT: *objPtr = *objPtr:\n    ");
    //   pretty (assignStmt);
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    varDecl2 = genAddressOf (assignStmt->expr);   // this new temp points to the src area
    IRDynamicObjectMove (varDecl, varDecl2);


  // Case 2: OBJECT ASSIGNMENT: *objPtr = x...
  } else if (assignStmt->dynamicCheck == 2) {
    //   printf ("Case 2: OBJECT ASSIGNMENT: *objPtr = x:\n    ");
    //   pretty (assignStmt);
    if (assignStmt->sizeInBytes < 1) {
      programLogicError ("Error: sizeInBytes < 1 in genAssignStmt, case 2");
    }
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    temp = genExpr (assignStmt->expr, assignStmt->sizeInBytes);
    IRCheckDPT (varDecl, assignStmt->classDef);
    IRMove (NULL,  varDecl, temp, NULL, assignStmt->sizeInBytes);


  // Case 3: OBJECT ASSIGNMENT: x = *objPtr...
  } else if (assignStmt->dynamicCheck == 3) {
    //   printf ("Case 3: OBJECT ASSIGNMENT: x = *objPtr:\n    ");
    //   pretty (assignStmt);
    if (assignStmt->sizeInBytes < 1) {
      programLogicError ("Error: sizeInBytes < 1 in genAssignStmt, case 3");
    }
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    varDecl2 = genAddressOf (assignStmt->expr);   // this new temp points to the src area
    IRCheckDPT2 (varDecl2, assignStmt->classDef);
    IRMove (NULL,  varDecl, NULL, varDecl2, assignStmt->sizeInBytes);


  // Case 4: ARRAY ASSIGNMENT: arr[10] = arr[10]...
  } else if (assignStmt->dynamicCheck == 4) {
    //   printf ("Case 4: ARRAY ASSIGNMENT: arr[10] = arr[10]:\n    ");
    //   pretty (assignStmt);
    if (assignStmt->sizeInBytes < 1) {
      programLogicError ("Error: sizeInBytes < 1 in genAssignStmt, case 4");
    }
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    varDecl2 = genAddressOf (assignStmt->expr);   // this new temp points to the src area
    IRCheckArraySizeInt2 (varDecl, assignStmt->arraySize);   // arraySize=number of elts
    IRCheckArraySizeInt (varDecl2, assignStmt->arraySize);  // arraySize=number of elts
    IRMove (NULL,  varDecl, NULL, varDecl2, assignStmt->sizeInBytes);


  // Case 5: ARRAY ASSIGNMENT: arr[10] = arr[*]...
  } else if (assignStmt->dynamicCheck == 5) {
    //   printf ("Case 5: ARRAY ASSIGNMENT: arr[10] = arr[*]:\n    ");
    //   pretty (assignStmt);
    if (assignStmt->sizeInBytes < 1) {
      programLogicError ("Error: sizeInBytes < 1 in genAssignStmt, case 5");
    }
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    varDecl2 = genAddressOf (assignStmt->expr);   // this new temp points to the src area
    IRCheckArraySizeInt2 (varDecl, assignStmt->arraySize);  // arraySize=number of elts
    IRCheckArraySizeInt (varDecl2, assignStmt->arraySize);  // arraySize=number of elts
    IRMove (NULL,  varDecl, NULL, varDecl2, assignStmt->sizeInBytes);


  // Case 6: ARRAY ASSIGNMENT: arr[*] = arr[10]...
  } else if (assignStmt->dynamicCheck == 6) {
    //   printf ("Case 6: ARRAY ASSIGNMENT: arr[*] = arr[10]:\n    ");
    //   pretty (assignStmt);
    if (assignStmt->sizeInBytes < 1) {
      programLogicError ("Error: sizeInBytes < 1 in genAssignStmt, case 6");
    }
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    varDecl2 = genAddressOf (assignStmt->expr);   // this new temp points to the src area
    IRCheckArraySizeInt (varDecl, assignStmt->arraySize);   // arraySize=number of elts
    IRCheckArraySizeInt (varDecl2, assignStmt->arraySize);  // arraySize=number of elts
    IRMove (NULL,  varDecl, NULL, varDecl2, assignStmt->sizeInBytes);


  // Case 7: ARRAY ASSIGNMENT: arr[*] = arr[*]...
  } else if (assignStmt->dynamicCheck == 7) {
    //   printf ("Case 7: ARRAY ASSIGNMENT: arr[*] = arr[*]:\n    ");
    //   pretty (assignStmt);
    if (assignStmt->arraySize < 1) {
      programLogicError ("Error: arraySize < 1 in genAssignStmt, case 7");
    }
    varDecl = genAddressOf (assignStmt->lvalue);  // this new temp points to the dest area
    varDecl2 = genAddressOf (assignStmt->expr);   // this new temp points to the src area
    IRCopyArrays (varDecl, varDecl2, assignStmt->arraySize);  // arraySize=size of elts

  } else {
    programLogicError ("Unknown value for dynamicCheck, in genAssignStmt");
  }
}



//
// genCallStmt (callStmt)
//
void genCallStmt (CallStmt * callStmt) {
  genCallExpr (NULL, callStmt->expr, NULL, NULL);
}



//
// genCallExpr (target, callExpr, trueLabel, falseLabel)
// 
// This routine is called to generate code for a call expr.  It is passed
// a variable ("target") or labels ("trueLabel" and "falseLabel").
// This routine will generate code to perform the function call and move
// the result (if any) into target, or will generate code to branch to the labels.
//
// Case 1: The function returns "void".  In this case, "target", "trueLabel",
// and "falseLabel" will be all NULL.  The generated code will simply fall thru.
//
// Case 2: The function returns a value.  Either "target" will be a variable,
// or "trueLabel" and "falseLabel" will tell where to jump to.  If one label
// is present, both will be present.  If target is non-NULL, the labels will
// be NULL.  If the target is NULL, then the labels will be non-NULL.
//
void genCallExpr (VarDecl * target,
                  CallExpr * callExpr,
                  char * trueLabel,
                  char * falseLabel) {
  FunctionProto * funProto;
  VarDecl * resultVar, * varDecl;
  Expression * arg1, * arg2;
  AstNode * x, * y;
  char * lab1, * lab2;
  Argument * arg;
  Parameter * parm;

  switch (callExpr->primitiveSymbol) {

    case 0:            // I.e., not a primitive...

      break;

    case PRIMITIVE_INT_TO_DOUBLE:                              // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 4);
      IRItoF (target, x);      
      return;

    case PRIMITIVE_DOUBLE_TO_INT:                              // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 8);
      IRFtoI (target, x);      
      return;

    case PRIMITIVE_INT_TO_CHAR:                                // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 4);
      IRItoC (target, x);      
      return;

    case PRIMITIVE_CHAR_TO_INT:                                // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 1);
      IRCtoI (target, x);      
      return;

    case PRIMITIVE_POS_INF:                                    // in genCallExpr

      IRPosInf (target);
      return;

    case PRIMITIVE_NEG_INF:                                    // in genCallExpr

      IRNegInf (target);
      return;

    case PRIMITIVE_NEG_ZERO:                                   // in genCallExpr

      IRNegZero (target);
      return;

    case PRIMITIVE_PTR_TO_BOOL:                                // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 4);
      if (trueLabel) {
        IRIntEQGoto (x, constantIntZero, falseLabel);
        IRGoto (trueLabel);
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        IRIntEQGoto (x, constantIntZero, lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantFalse);
        IRLabel (lab2);
      }
      return;

    case PRIMITIVE_I_IS_ZERO:                                  // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 4);
      if (trueLabel) {
        IRIntEqZero (x, trueLabel);
        IRGoto (falseLabel);
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        IRIntEqZero (x, lab1);
        IRAssign1 (target, constantFalse);
        IRGoto (lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRLabel (lab2);
      }
      return;

    case PRIMITIVE_I_NOT_ZERO:                                 // in genCallExpr

      arg1 = callExpr->argList->expr;
      x = genExpr (arg1, 4);
      if (trueLabel) {
        IRIntEqZero (x, falseLabel);
        IRGoto (trueLabel);
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        IRIntEqZero (x, lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantFalse);
        IRLabel (lab2);
      }
      return;

    default:                                                   // in genCallExpr

      printf ("primitiveSymbol = %s\n", symbolName (callExpr->primitiveSymbol));
      programLogicError ("In genCallExpr, unexpected primitiveSymbol");
  }

  if (callExpr->myDef == NULL) {
    programLogicError ("In genCallExpr, if not PRIMITIVE, myDef should be filled in");
  }

  // If we have a normal call...
  if (callExpr->myDef->op == FUNCTION_PROTO) {
    funProto = (FunctionProto *) callExpr->myDef;

    // Run through the arguments and evaluate each of them...
    parm = funProto->parmList;
    for (arg = callExpr->argList; arg; arg = arg->next) {
      if (parm == NULL) {
        programLogicError ("Should have same number of parms as args");
      }
      arg->tempName = genExpr (arg->expr, arg->sizeInBytes);
      parm = (Parameter *) parm->next;
    }

    // Run through the arguments and move each into its position in the frame...
    parm = funProto->parmList;
    for (arg = callExpr->argList; arg; arg = arg->next) {
      IRPrepareArg (arg->offset, arg->tempName, arg->sizeInBytes);
      parm = (Parameter *) parm->next;
    }

    // Call the function...
    IRComment ("  Call the function");
    if (funProto->isExternal) {
      genLineNumber (callExpr, "CE");
    } else {
      genLineNumber (callExpr, "CA");
    }
    IRCall (funProto->newName);

    // If this function returns a result...
    if (funProto->retSize > 0) {
      if (trueLabel) {
        // Generate branches...
        IRBoolTest2 (trueLabel, falseLabel);
      } else {
        // Move the result into "target"...
        IRRetrieveResult (target, funProto->retSize);
      }
    }
    return;

  }

  // Otherwise, we have something like "f(x)" where "f: ptr to function"...
  //   printf ("In genCallExpr, indirect call: ");
  //   pretty (callExpr);

  if ((callExpr->myDef->op != LOCAL) &&
      (callExpr->myDef->op != GLOBAL) &&
      (callExpr->myDef->op != PARAMETER) &&
      (callExpr->myDef->op != CLASS_FIELD)) {
    programLogicError ("callExpr->myDef->op is wrong");
  }

  varDecl = (VarDecl *) callExpr->myDef;

  // Run through the arguments and evaluate each of them...
  for (arg = callExpr->argList; arg; arg = arg->next) {
    arg->tempName = genExpr (arg->expr, arg->sizeInBytes);
  }

  // Run through the arguments and move each into its position in the frame...
  for (arg = callExpr->argList; arg; arg = arg->next) {
    IRPrepareArg (arg->offset, arg->tempName, arg->sizeInBytes);
  }

  // Call the function...
  IRComment2 ("  call indirectly through variable ", varDecl->id->chars, "");
  genLineNumber (callExpr, "CF");
  IRCallIndirect (varDecl);

  // If this function returns a result...
  if (callExpr->retSize > 0) {
    if (trueLabel) {
      // Generate branches...
      IRBoolTest2 (trueLabel, falseLabel);
    } else {
      // Move the result into "target"...
      IRRetrieveResult (target, callExpr->retSize);
    }
  }

}



//
// genSendStmt (sendStmt)
//
void genSendStmt (SendStmt * sendStmt) {
  genSendExpr (NULL, sendStmt->expr, NULL, NULL);
}



//
// genSendExpr (target, sendExpr, trueLabel, falseLabel)
//
// This routine is called to generate code for a send expr.  It is passed
// a variable ("target") or labels ("trueLabel" and "falseLabel").
// This routine will generate code to perform the method invocation and move
// the result (if any) into target, or will generate code to branch to the labels.
//
// Case 1: The method returns "void".  In this case, "target", "trueLabel",
// and "falseLabel" will be all NULL.  The generated code will simply fall thru.
//
// Case 2: The method returns a value.  Either "target" will be a variable,
// or "trueLabel" and "falseLabel" will tell where to jump to.  If one label
// is present, both will be present.  If target is non-NULL, the labels will
// be NULL.  If the target is NULL, then the labels will be non-NULL.
// 
void genSendExpr (VarDecl * target,
                  SendExpr * sendExpr,
                  char * trueLabel,
                  char * falseLabel) {
  MethodProto * methProto;
  VarDecl * resultVar, * tempVar, * ptr1, * ptr2;
  Expression * arg1, * arg2;
  AstNode * x, * y;
  Local * temp;
  char * lab1, * lab2, * lab3;
  Parameter * parm;
  Argument * arg;

  //   printf ("SEND EXPR = ");
  //   pretty (sendExpr);

  // If we have a non-primitive message-send...
  if (sendExpr->myProto != NULL) {
    genLineNumber (sendExpr, "SE");
    methProto = (MethodProto *) sendExpr->myProto;

    // Run through the arguments and evaluate each of them...
    parm = methProto->parmList;
    for (arg = sendExpr->argList; arg; arg = arg->next) {
      if (parm == NULL) {
        programLogicError ("Should have same number of parms as args");
      }
      arg->tempName = genExpr (arg->expr, arg->sizeInBytes);
      parm = (Parameter *) parm->next;
    }

    // Figure out the address of the receiver object...
    tempVar = genAddressOf (sendExpr->receiver);

    // Run through the arguments and move each into its position in the frame...
    parm = methProto->parmList;
    for (arg = sendExpr->argList; arg; arg = arg->next) {
      IRPrepareArg (arg->offset, arg->tempName, arg->sizeInBytes);
      parm = (Parameter *) parm->next;
    }

    // Do the send...
    IRSend (tempVar, methProto);

    // If this method returns a result...
    if (methProto->retSize > 0) {
      if (trueLabel) {
        IRBoolTest2 (trueLabel, falseLabel);
      } else {
        // Move the result into the target location...
        IRRetrieveResult (target, methProto->retSize);
      }
    }

    return;
  }

  // Otherwise, it must be a primitive.  See which primitive...
  switch (sendExpr->primitiveSymbol) {

    case PRIMITIVE_I_ADD:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRIAdd (target, x, y);      
      return;

    case PRIMITIVE_I_SUB:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRISub (target, x, y);      
      return;

    case PRIMITIVE_I_MUL:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRIMul (target, x, y);      
      return;

    case PRIMITIVE_I_DIV:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRIDiv (target, x, y);      
      return;

    case PRIMITIVE_I_REM:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRIRem (target, x, y);      
      return;

    case PRIMITIVE_I_SLL:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRSll (target, x, y);      
      return;

    case PRIMITIVE_I_SRA:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRSra (target, x, y);      
      return;

    case PRIMITIVE_I_SRL:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRSrl (target, x, y);      
      return;

    case PRIMITIVE_I_AND:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRAnd (target, x, y);      
      return;

    case PRIMITIVE_I_OR:                           // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IROr (target, x, y);      
      return;

    case PRIMITIVE_I_XOR:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 4);
      y = genExpr (arg2, 4);
      IRXor (target, x, y);      
      return;

    case PRIMITIVE_I_NOT:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      x = genExpr (arg1, 4);
      IRNot (target, x);      
      return;

    case PRIMITIVE_I_NEG:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      x = genExpr (arg1, 4);
      IRINeg (target, x);      
      return;

    case PRIMITIVE_D_ADD:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 8);
      y = genExpr (arg2, 8);
      IRFAdd (target, x, y);      
      return;

    case PRIMITIVE_D_SUB:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 8);
      y = genExpr (arg2, 8);
      IRFSub (target, x, y);      
      return;

    case PRIMITIVE_D_MUL:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 8);
      y = genExpr (arg2, 8);
      IRFMul (target, x, y);      
      return;

    case PRIMITIVE_D_DIV:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 8);
      y = genExpr (arg2, 8);
      IRFDiv (target, x, y);      
      return;

    case PRIMITIVE_D_NEG:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      x = genExpr (arg1, 8);
      IRFNeg (target, x);      
      return;

    case PRIMITIVE_I_LT:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 4);
        y = genExpr (arg2, 4);
        IRIntGEGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);              // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_I_LE:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 4);
        y = genExpr (arg2, 4);
        IRIntGTGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);              // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_I_GT:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 4);
        y = genExpr (arg2, 4);
        IRIntLEGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);              // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_I_GE:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 4);
        y = genExpr (arg2, 4);
        IRIntLTGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);              // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_I_EQ:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 4);
        y = genExpr (arg2, 4);
        IRIntNEGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);              // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_I_NE:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 4);
        y = genExpr (arg2, 4);
        IRIntEQGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);              // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_D_LT:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 8);
        y = genExpr (arg2, 8);
        IRFloatGEGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);                // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_D_LE:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 8);
        y = genExpr (arg2, 8);
        IRFloatGTGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);                // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_D_GT:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 8);
        y = genExpr (arg2, 8);
        IRFloatLEGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);                // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_D_GE:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 8);
        y = genExpr (arg2, 8);
        IRFloatLTGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);                // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_D_EQ:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 8);
        y = genExpr (arg2, 8);
        IRFloatNEGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);                // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_D_NE:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        x = genExpr (arg1, 8);
        y = genExpr (arg2, 8);
        IRFloatEQGoto (x, y, falseLabel);  // Reverse test so we can end w/ a
        IRGoto (trueLabel);                // goto that can often be optimized
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_B_AND:                          // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        lab1 = newLabel ();
        genExprInto (NULL, arg1, lab1, falseLabel);
        IRLabel (lab1);
        genExprInto (NULL, arg2, trueLabel, falseLabel);
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_B_OR:                           // in genSendExpr

      if (trueLabel) {
        arg1 = sendExpr->receiver;
        arg2 = sendExpr->argList->expr;
        lab1 = newLabel ();
        genExprInto (NULL, arg1, trueLabel, lab1);
        IRLabel (lab1);
        genExprInto (NULL, arg2, trueLabel, falseLabel);
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        genSendExpr (NULL, sendExpr, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
      }     
      return;

    case PRIMITIVE_B_EQ:                           // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 1);
      y = genExpr (arg2, 1);
      if (trueLabel) {
        temp = newTemp (1);
        IRBXor (temp, x, y);
        IRBoolTest (temp, falseLabel, trueLabel);
      } else {
        IRBEq (target, x, y);
      }     
      return;

    case PRIMITIVE_B_NE:                           // in genSendExpr

      arg1 = sendExpr->receiver;
      arg2 = sendExpr->argList->expr;
      x = genExpr (arg1, 1);
      y = genExpr (arg2, 1);
      if (trueLabel) {
        temp = newTemp (1);
        IRBXor (temp, x, y);
        IRBoolTest (temp, trueLabel, falseLabel);
      } else {
        IRBXor (target, x, y);
      }     
      return;

    case PRIMITIVE_B_NOT:                          // in genSendExpr

      arg1 = sendExpr->receiver;
      x = genExpr (arg1, 1);
      if (trueLabel) {
        IRBoolTest (x, falseLabel, trueLabel);
      } else {
        IRBNot (target, x);
      }     
      return;

    case PRIMITIVE_ADDRESS_OF:                     // in genSendExpr

      //   printf ("Address-of in genSendExpr... sub-expr = ");
      //   pretty (sendExpr->receiver);
      if (target == NULL) {
        programLogicError ("Should never have, e.g., 'if &x ...'");
      }

      // If this is a simple variable, then handle it immediately...
      if (sendExpr->receiver->op == VARIABLE_EXPR) {
        tempVar = (VarDecl *) (((VariableExpr *) (sendExpr->receiver))->myDef);
        if (tempVar->op == LOCAL ||
            tempVar->op == GLOBAL ||
            tempVar->op == PARAMETER ||
            tempVar->op == CLASS_FIELD) {
          IRLoadAddr2 (target, tempVar);
          return;
        }
      }

      // Else, call "genAddressOf" to do the work...
      //   printf ("Calling genAddressOf = ");
      //   pretty (sendExpr->receiver);
      IRAssign4 (target, genAddressOf (sendExpr->receiver));
      return;

    case PRIMITIVE_DEREF:                          // in genSendExpr

      if (trueLabel) {
        tempVar = (VarDecl *) genExpr (sendExpr->receiver, 4);
        if ((tempVar->op != LOCAL) &&
            (tempVar->op != GLOBAL) &&
            (tempVar->op != PARAMETER) &&
            (tempVar->op != INT_CONST) &&
            (tempVar->op != CLASS_FIELD)) {
          printf ("In genSendExpr with deref = ");
          pretty (sendExpr);
          printf ("tempVar = ");
          pretty (tempVar);
          programLogicError ("genExpr should return a 32-bit quantity for xxx in *xxx -- 1");
        }
        IRBoolEqZeroIndirect (tempVar, falseLabel);
        IRGoto (trueLabel);
        return;
      } else {
        //   printf ("In genSendExpr with deref = ");
        //   pretty (sendExpr);
        int sz = target->sizeInBytes;
        if (sz < 1) {
          printf ("size = %d\n", sz);
          programLogicError ("In genSendExpr, the size is missing");
        }
        // We have the target variable and an expression which will yield a ptr.
        tempVar = (VarDecl *) genExpr (sendExpr->receiver, 4);
        if ((tempVar->op != LOCAL) &&
            (tempVar->op != GLOBAL) &&
            (tempVar->op != PARAMETER) &&
            (tempVar->op != INT_CONST) &&
            (tempVar->op != CLASS_FIELD)) {
          printf ("In genSendExpr with deref = ");
          pretty (sendExpr);
          printf ("tempVar = ");
          pretty (tempVar);
          programLogicError ("genExpr should return a 32-bit quantity for xxx in *xxx -- 2");
        }
        IRIntEqZero (tempVar, "_runtimeErrorNullPointer");
        IRMove (target, NULL, NULL, tempVar, sz);   // tempVar is "srcPtr"
        return;
      }

    case PRIMITIVE_OBJECT_EQ:                      // in genSendExpr

      ptr1 = genAddressOf (sendExpr->receiver);
      ptr2 = genAddressOf (sendExpr->argList->expr);
      if (trueLabel) {
        lab1 = newLabel ();
        IRTestObjEq (ptr1, ptr2, trueLabel, falseLabel);
        return;
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        IRTestObjEq (ptr1, ptr2, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantTrue);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantFalse);
        IRLabel (lab3);
        return;
      }

    case PRIMITIVE_OBJECT_NE:                      // in genSendExpr

      ptr1 = genAddressOf (sendExpr->receiver);
      ptr2 = genAddressOf (sendExpr->argList->expr);
      if (trueLabel) {
        lab1 = newLabel ();
        IRTestObjEq (ptr1, ptr2, falseLabel, trueLabel);
        return;
      } else {
        lab1 = newLabel ();
        lab2 = newLabel ();
        lab3 = newLabel ();
        IRTestObjEq (ptr1, ptr2, lab1, lab2);
        IRLabel (lab1);
        IRAssign1 (target, constantFalse);
        IRGoto (lab3);
        IRLabel (lab2);
        IRAssign1 (target, constantTrue);
        IRLabel (lab3);
        return;
      }

    default:                                       // in genSendExpr
      printf ("primitiveSymbol = %s\n", symbolName (sendExpr->primitiveSymbol));
      programLogicError ("In genSendExpr, unexpected primitiveSymbol");
  }

}



//
// genWhileStmt (whileStmt)
//
void genWhileStmt (WhileStmt * whileStmt) {
  char * continueLabel;
  whileStmt->topLabel = newLabel ();
  continueLabel = newLabel ();
  whileStmt->exitLabel = newLabel ();
  genLineNumber (whileStmt, "WH");
  IRLabel (whileStmt->topLabel);
  genExprInto (NULL, whileStmt->expr, continueLabel, whileStmt->exitLabel);
  IRLabel (continueLabel);
  genLineNumber (whileStmt, "WB");
  genStmts (whileStmt->stmts);
  IRComment ("END WHILE...");
  IRGoto (whileStmt->topLabel);
  IRLabel (whileStmt->exitLabel);
}



//
// genDoStmt (doStmt)
//
void genDoStmt (DoStmt * doStmt) {
  char * topLabel;
  topLabel = newLabel ();
  doStmt->testLabel = newLabel ();
  doStmt->exitLabel = newLabel ();
  IRLabel (topLabel);
  genLineNumber (doStmt, "DO");
  genStmts (doStmt->stmts);
  IRComment ("UNTIL...");
  IRLabel (doStmt->testLabel);
  genLineNumber (doStmt->expr, "DU");
  genExprInto (NULL, doStmt->expr, doStmt->exitLabel, topLabel);
  IRLabel (doStmt->exitLabel);
}



//
// genBreakStmt (breakStmt)
//
void genBreakStmt (BreakStmt * breakStmt) {
  ForStmt * forStmt;
  DoStmt * doStmt;
  WhileStmt * whileStmt;
  SwitchStmt * switchStmt;

  switch (breakStmt->enclosingStmt->op) {
    case FOR_STMT:
      forStmt = (ForStmt *) breakStmt->enclosingStmt;
      IRGoto (forStmt->exitLabel);
      return;
    case DO_STMT:
      doStmt = (DoStmt *) breakStmt->enclosingStmt;
      IRGoto (doStmt->exitLabel);
      return;
    case WHILE_STMT:
      whileStmt = (WhileStmt *) breakStmt->enclosingStmt;
      IRGoto (whileStmt->exitLabel);
      return;
    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) breakStmt->enclosingStmt;
      IRGoto (switchStmt->exitLabel);
      return;
    default:
      programLogicError ("Unexected enclosingStmt in BreakStmt");
  }
}



//
// genContinueStmt (continueStmt)
//
void genContinueStmt (ContinueStmt * continueStmt) {
  ForStmt * forStmt;
  DoStmt * doStmt;
  WhileStmt * whileStmt;

  switch (continueStmt->enclosingStmt->op) {
    case FOR_STMT:
      forStmt = (ForStmt *) continueStmt->enclosingStmt;
      IRGoto (forStmt->incrLabel);
      return;
    case DO_STMT:
      doStmt = (DoStmt *) continueStmt->enclosingStmt;
      IRGoto (doStmt->testLabel);
      return;
    case WHILE_STMT:
      whileStmt = (WhileStmt *) continueStmt->enclosingStmt;
      IRGoto (whileStmt->topLabel);
      return;
    default:
      programLogicError ("Unexected enclosingStmt in ContinueStmt");
  }
}



//
// genReturnStmt (returnStmt)
//
void genReturnStmt (ReturnStmt * returnStmt) {
  AstNode * x;
  MethOrFunction * methOrFun;

  // If there is a return expression, generate code to compute and save it.
  if (returnStmt->expr) {
    // printf ("----------In genReturnStmt...  retSize = %d\n", returnStmt->retSize);
    x = genExpr (returnStmt->expr, returnStmt->retSize);
    IRReturnResult (x, returnStmt->retSize);
  }

  // Identify the enclosing method or function...
  methOrFun = returnStmt->enclosingMethOrFunction;
  if (methOrFun == NULL) {
    programLogicError ("returnStmt->enclosingMethOrFunction is NULL");
  }

  // If this method or function contained any "try" statements...
  if (methOrFun->containsTry) {
    if (methOrFun->catchStackSave == NULL) {
      programLogicError ("methOrFun->catchStackSave in NULL in genReturnStmt");
    }
    IRRestoreCatchStack (methOrFun->catchStackSave);
  }

  // Generate the return code...
  if (methOrFun->op == METHOD) {
    IRMethodReturn ((Method *) methOrFun);
  } else {
    IRFunctionReturn ((Function *) methOrFun);
  }

}



//
// genForStmt (forStmt)
//
// For a stmt such as:
//      FOR LValue = InitExpr TO FinalExpr BY StepExpr
//        ...Statements...
//      ENDFOR
// this routine generates the following code:
//
//         tempPtr = & LValue
//         tempStart = InitExpr
//         tempStop = FinalExpr
//         tempIncr = StepExpr
//         * tempPtr = tempStart
//     TOP_LABEL:
//             IF * tempPtr > tempStop GOTO EXIT_LABEL
//     CONTINUE_LABEL:
//             ...Statements...
//     INCR_LABEL:
//             * tempPtr = * tempPtr + tempIncr
//             goto TOP_LABEL
//     EXIT_LABEL:
//
// The "Continue" stmt will branch to INCR_LABEL.
// The "Break" stmt will branch to EXIT_LABEL.
//
// If the LValue is a LOCAL or PARAMETER, then we will not use "tempPtr"
// and will use the variable directly.
//
// Also, if the incr is an integer within 16 bits, we'll avoid "tempIncr".
//
void genForStmt (ForStmt * forStmt) {

  VarDecl * tempPtr, * tempStart, * tempStop, * tempIncr;
  char * topLabel, * continueLabel;
  int incrInteger;
  IntConst * intConst;
  VarDecl * localIndexVar, * def;

  topLabel = newLabel ();
  continueLabel = newLabel ();
  forStmt->incrLabel = newLabel ();
  forStmt->exitLabel = newLabel ();

  localIndexVar = NULL;
  if (forStmt->lvalue->op == VARIABLE_EXPR) {
    def = (VarDecl *) (((VariableExpr *) (forStmt->lvalue)) -> myDef);
    if ((def->op == LOCAL) ||
        (def->op == PARAMETER)) {
      localIndexVar = def;
    }
  }

  if (localIndexVar == NULL) {
    IRComment ("  Calculate and save the address of the FOR-LOOP index variable");
    tempPtr = newTemp (4);
    IRMove (tempPtr, NULL, genAddressOf (forStmt->lvalue), NULL, 4);
  }

  IRComment ("  Calculate and save the FOR-LOOP starting value");
  tempStart = newTemp (4);
  genExprInto (tempStart, forStmt->expr1, NULL, NULL);

  IRComment ("  Calculate and save the FOR-LOOP ending value");
  tempStop = newTemp (4);
  genExprInto (tempStop, forStmt->expr2, NULL, NULL);

  if (forStmt->expr3) {
    intConst = (IntConst *) forStmt->expr3;
    if (intConst->op == INT_CONST) {
      tempIncr = NULL;
      incrInteger = intConst->ivalue;
    } else {
      IRComment ("  Calculate and save the FOR-LOOP increment value");
      tempIncr = newTemp (4);
      genExprInto (tempIncr, forStmt->expr3, NULL, NULL);
    }
  } else {
    tempIncr = NULL;
    incrInteger = 1;
  }

  IRComment ("  Initialize FOR-LOOP index variable");
  if (localIndexVar) {
    IRMove (localIndexVar, NULL, tempStart, NULL, 4);
  } else {
    IRMove (NULL, tempPtr, tempStart, NULL, 4);
  }
  IRLabel (topLabel);

  // Perform the termination test...
  if (localIndexVar) {
    IRForTest2 (localIndexVar, tempStop, forStmt->exitLabel);
  } else {
    IRForTest (tempPtr, tempStop, forStmt->exitLabel);
  }
  IRLabel (continueLabel);

  // Here is the loop body...
  genLineNumber (forStmt, "FB");
  genStmts (forStmt->stmts);

  IRComment ("  Increment the FOR-LOOP index variable and jump back");
  IRLabel (forStmt->incrLabel);
  if (localIndexVar) {
    IRIncrVarDirect (localIndexVar, localIndexVar, tempIncr, incrInteger, 1);
                                                         // wantOverflowTest = 1
  } else {
    IRIncrVarIndirect (tempPtr, tempIncr, incrInteger);
  }
  IRGoto (topLabel);
  IRComment ("END FOR");
  IRLabel (forStmt->exitLabel);
}



//
// genSwitchStmt (switchStmt)
//
void genSwitchStmt (SwitchStmt * switchStmt) {
  Case * cas;
  char * defaultLabel, * tableName;
  int numberOfCases = 0, range;
  AstNode * expr;
  double density;
  char * * a;
  int * valueArray;
  int i, highValue, lowValue, tableSize;

  defaultLabel = newLabel ();
  switchStmt->exitLabel = newLabel ();
  highValue = switchStmt->highValue;
  lowValue = switchStmt->lowValue;

  // Run throught the cases and give each a label.
  for (cas = switchStmt->caseList; cas; cas = cas->next) {
    cas->label = newLabel ();
    numberOfCases++;
  }

  // Determine the range of values and the density a direct table would have...
  if (numberOfCases > 0) {
    range = (highValue - lowValue) + 1;
    if (range < 1) {
      programLogicError ("highValue and lowValue are invalid");
    }
    density = ((double) (numberOfCases)) / ((double) range);
  } else {
    density = 0.0;
  }
  //   printf ("SWITCH STMT: density = %g\n", density);

  // If the number of cases is <= 7, then generate a sequence of tests...
  if (numberOfCases <= 7) {

    IRComment ("SWITCH STATEMENT (using series of tests)...");
    genLineNumber (switchStmt, "SW");
    IRComment ("  Evaluate the switch expression...");
    expr = genExpr (switchStmt->expr, 4);

    // Run throught the cases and generate a test for each.
    IRComment ("  Branch to the right case label");
    IRSwitchReg1 (expr);
    for (cas = switchStmt->caseList; cas; cas = cas->next) {
      IRSwitchTestReg1 (cas->ivalue, cas->label);
    }
    IRGoto (defaultLabel);

  // If the density of cases is > .5 or the table would be fairly small,
  // then generate a direct-jump table...
  } else if (((density > 0.5) || (range <= 300)) &&
             (within16Bits (lowValue)) &&
             (within16Bits (highValue))) {

    IRComment ("SWITCH STATEMENT (using an indirect jump table)...");
    genLineNumber (switchStmt, "SW");
    IRComment ("  Evaluate the switch expression...");
    expr = genExpr (switchStmt->expr, 4);

    // Generate code to test the expr and make a jump into the direct table...
    tableName = newLabel ();
    IRSwitchDirect (expr, tableName, defaultLabel, lowValue, highValue);
    // Create an array of labels of the right size...
    //     printf ("lowValue = %d\n", lowValue);
    //     printf ("highValue = %d\n", highValue);
    //     printf ("range = %d\n", range);
    a = (char * *) calloc (4, range); 
    if (a == NULL) {
      fatalError ("Calloc failed in genSwitchStmt; out of memory perhaps???");
    }
    // Initialize the array so all entries are the default label...
    for (i = 0; i < range; i++) {
      a[i] = defaultLabel;
    }
    // Modify all entries for which we have a case...
    for (cas = switchStmt->caseList; cas; cas = cas->next) {
      a[cas->ivalue-lowValue] = cas->label;
    }
    IRComment ("  Jump table");
    IRLabel (tableName);
    for (i = 0; i < range; i++) {
      if (a[i] == defaultLabel) {
        IRGoto2 (a[i], i+lowValue, "missing - goto default code");
      } else {
        IRGoto2 (a[i], i+lowValue, "");
      }
    }

  // Generate a hash table lookup table and code to search it.
  } else {

    IRComment ("SWITCH STATEMENT (using a hash table)...");
    genLineNumber (switchStmt, "SW");
    IRComment ("  Evaluate the switch expression...");
    expr = genExpr (switchStmt->expr, 4);

    tableSize = roundUpToPrime (numberOfCases * 2);
    if (! within16Bits (tableSize)) {
      programLogicError ("IMPLEMENTATION RESTRICTION: In a Switch statement, there are too many cases (more than about 16000 cannot be handled)");
    }
    //      printf ("table size = %d\n", tableSize);
    tableName = newLabel ();

    // Create an array of labels of the right size...
    a = (char * *) calloc (4, tableSize); 
    if (a == NULL) {
      fatalError ("Calloc failed in genSwitchStmt; out of memory perhaps???");
    }
    // Initialize the array so all entries are NULL...
    for (i = 0; i < tableSize; i++) {
      a[i] = NULL;
    }

    // Create an array of values of the right size...
    valueArray = (int *) calloc (4, tableSize); 
    if (valueArray == NULL) {
      fatalError ("Calloc failed in genSwitchStmt; out of memory perhaps???");
    }
    // Initialize the array so all entries are zero...
    for (i = 0; i < tableSize; i++) {
      valueArray[i] = 0;
    }

    // Go through each case...
    for (cas = switchStmt->caseList; cas; cas = cas->next) {
      i = hashForSwitchStmt (cas->ivalue, tableSize);
      //   printf ("ivalue = %d    hash = %d\n", cas->ivalue, i);
      while (a[i] != NULL) {
        //    printf ("  i = %d already contains value = %d... incrementing...\n",
        //            i, valueArray[i]);
        i++;
        if (i == tableSize) {
          i = 0;
        }
      }
      //    printf ("  Updating i = %d\n", i);
      valueArray [i] = cas->ivalue;
      a [i] = cas->label;
    }

    IRSwitchHashJump (expr, tableName, defaultLabel, tableSize);

    IRComment ("  Hash table");
    IRLabel (tableName);
    for (i = 0; i < tableSize; i++) {
      IRComment3 ("Table entry ", i);
      IRWord (valueArray[i]);
      if (valueArray[i] == 0) {
        IRWord (0);
      } else {
        IRWord2 (a[i]);
      }
    }

  }

  // Run throught the cases and generate code for each.
  for (cas = switchStmt->caseList; cas; cas=cas->next) {
    IRComment3 ("CASE ", cas->ivalue); 
    IRLabel (cas->label);
    genStmts (cas->stmts);
  }

  // Generate code for the default stmt list, if any.
  IRComment ("DEFAULT CASE..."); 
  IRLabel (defaultLabel);
  genStmts (switchStmt->defaultStmts);

  // Generate the exit label, which is used by break stmts.
  IRComment ("END SWITCH..."); 
  IRLabel (switchStmt->exitLabel);

}



// roundUpToPrime (int) --> int
//
// This routine rounds the given number up to a prime number.
// With numbers larger than 200, it simply returns a prime number
// that is as big.  For switch statements with more than 30,000 cases--
// which are presumably machine generated and not expected to occur
// often--this routine will fail with a program logic error.
//
int roundUpToPrime (int i) {
  if (i <= 2) return 2;
  if (i <= 3) return 3;
  if (i <= 5) return 5;
  if (i <= 7) return 7;
  if (i <= 11) return 11;
  if (i <= 13) return 13;
  if (i <= 17) return 17;
  if (i <= 19) return 19;
  if (i <= 23) return 23;
  if (i <= 29) return 29;
  if (i <= 31) return 31;
  if (i <= 37) return 37;
  if (i <= 41) return 41;
  if (i <= 43) return 43;
  if (i <= 47) return 47;
  if (i <= 53) return 53;
  if (i <= 59) return 59;
  if (i <= 61) return 61;
  if (i <= 67) return 67;
  if (i <= 71) return 71;
  if (i <= 73) return 73;
  if (i <= 79) return 79;
  if (i <= 83) return 83;
  if (i <= 89) return 89;
  if (i <= 97) return 97;
  if (i <= 101) return 101;
  if (i <= 103) return 103;
  if (i <= 107) return 107;
  if (i <= 109) return 109;
  if (i <= 113) return 113;
  if (i <= 127) return 127;
  if (i <= 131) return 131;
  if (i <= 137) return 137;
  if (i <= 139) return 139;
  if (i <= 149) return 149;
  if (i <= 151) return 151;
  if (i <= 157) return 157;
  if (i <= 163) return 163;
  if (i <= 167) return 167;
  if (i <= 173) return 173;
  if (i <= 179) return 179;
  if (i <= 181) return 181;
  if (i <= 191) return 191;
  if (i <= 193) return 193;
  if (i <= 197) return 197;
  if (i <= 199) return 199;
  if (i <= 211) return 211;
//...
  if (i <= 307) return 307;
//...
  if (i <= 401) return 401;
//...
  if (i <= 503) return 503;
//...
  if (i <= 601) return 601;
//...
  if (i <= 701) return 701;
//...
  if (i <= 809) return 809;
//...
  if (i <= 907) return 907;
//...
  if (i <= 1009) return 1009;
//...
  if (i <= 2003) return 2003;
//...
  if (i <= 3001) return 3001;
//...
  if (i <= 4001) return 4001;
//...
  if (i <= 5003) return 5003;
//...
  if (i <= 10007) return 10007;
//...
  if (i <= 15013) return 15013;
//...
  if (i <= 20011) return 20011;
//...
  if (i <= 25013) return 25013;
//...
  if (i <= 30011) return 30011;

  programLogicError ("IMPLEMENTATION RESTRICTION: More than 30,000 cases encountered in some switch statement");

}



// hashForSwitchStmt (int, hashMax) --> int
//
// This routine takes a 32-bit integer and returns a hashed value between
// 0 and hashMax-1.   It is very important that this hash function performs
// exactly the same as the compiled code to compute the hash function for a
// switch statement.  This code is generated in IRSwitchHashJump.
// Both pieces of code must deal with overflow in the exact same way, to
// yield the same result.
//
int hashForSwitchStmt (int i, int hashMax) {
  // The following set "quo" and "rem".  It will only fail when
  // i = a = -2147483648 and hashMax = b = -1.  HashMax should be the
  // hash table size, which should be positive, so we are okay.  The
  // "rem" will be a non-negative number, less than the divisor, hashMax.
  if (hashMax <= 0) {
    programLogicError ("In hashForSwitchStmt, hashMax is <= 0");
  }
  divide (i, hashMax);
  return rem;
}



//
// genTryStmt (tryStmt)
//
void genTryStmt (TryStmt * tryStmt) {
  char * tryEnd = newLabel ();
  Catch * cat;
  VarDecl * saveStack = newTemp (4);
  Parameter * parm;

  // Generate code to save the current catch-stack value...
  IRSaveCatchStack (saveStack);

  // Run thru the catch clauses.  Assign them labels and generate
  // code to create and push a new record on the catch-stack...
  for (cat = tryStmt->catchList; cat; cat=cat->next) {
    cat->label = newLabel ();
    IRPushCatchRecord (cat);
  }

  // Generate the body code...
  IRComment ("BODY STMTS...");
  genStmts (tryStmt->stmts);

  // Generate code to restore the catch-stack and jump to end...
  IRRestoreCatchStack (saveStack);
  IRGoto (tryEnd);

  // Run through the catch clauses and generate code for each...
  for (cat = tryStmt->catchList; cat; cat=cat->next) {
    IRComment2 ("CATCH CLAUSE for ", cat->myDef->newName, "...");
    IRLabel (cat->label);
    genLineNumber (cat, "CC");
    for (parm = cat->parmList; parm; parm = (Parameter *) (parm->next)) {
      IRCopyCatchParm (parm);
    }
    IRResetStack ();
    IRRestoreCatchStack (saveStack);
    genStmts (cat->stmts);
    IRGoto (tryEnd);
  }

  // Generate the label at the end...
  IRComment ("END TRY...");
  IRLabel (tryEnd);

}



//
// genThrowStmt (throwStmt)
//
void genThrowStmt (ThrowStmt * throwStmt) {
  Argument * arg;
  ErrorDecl * errorDecl;

    // Run through the arguments and evaluate each of them...
    for (arg = throwStmt->argList; arg; arg = arg->next) {
      arg->tempName = genExpr (arg->expr, arg->sizeInBytes);
    }

    // Run through the arguments and move each into its position in the frame...
    for (arg = throwStmt->argList; arg; arg = arg->next) {
      IRPrepareArg (arg->offset, arg->tempName, arg->sizeInBytes);
    }

    // Perform the throw...
    errorDecl = throwStmt->myDef;
    IRComment2 ("  Throw '", errorDecl->id->chars, "'...");
    // IRDebug ();
    IRThrow (errorDecl);

}



//
// genFreeStmt (freeStmt)
//
void genFreeStmt (FreeStmt * freeStmt) {
  AstNode * temp;
  temp = genExpr (freeStmt->expr, 4);
  IRFree (temp);
}



// genExpr (expr, targetSize) --> node
//
// This routine generates IR code for an expression.  It returns one of:
//    Local
//    Global
//    Parameter
//    ClassField
//    IntConst
//    RealConst
//    BoolConst
//    CharConst
//
// In many cases, it will allocate a temp, generate code to move the value
// into that temp, and return the temp.  This routine is passed "targetSize",
// which is how big to make the temp.  It calls "genExprInto" to do the work.
//
// In other cases (when the expr is a constant or simple variable), it will
// just return it, as is.
//
// In simpler cases (i.e., the expr is a constant value), it simply returns
// the node itself.
//
AstNode * genExpr (Expression * node, int targetSize) {
    StringConst * stringConst;
    NullConst * nullConst;
    CallExpr * callExpr;
    VariableExpr * var;
    VarDecl * varDecl;
    Local * temp;
    FunctionProto * funProto;

  if (node==NULL) {
    programLogicError ("genExpr called with node == NULL");
  }

  //   printf ("=== genExpr: ");
  //   pretty (node);

  switch (node->op) {

    case INT_CONST:                                    // in genExpr
    case DOUBLE_CONST:
    case CHAR_CONST:
    case BOOL_CONST:

      return node;

    case NULL_CONST:                                   // in genExpr

      return constantIntZero;

    case VARIABLE_EXPR:                                // in genExpr

      var = (VariableExpr *) node;
      if (var->myDef == NULL) {
        programLogicError ("var->myDef == NULL in genExpr");
      }
      varDecl = (VarDecl *) var->myDef;
      if ((varDecl->op == LOCAL) ||
          (varDecl->op == GLOBAL) ||
          (varDecl->op == CLASS_FIELD) ||
          (varDecl->op == PARAMETER)) {
        return varDecl;
      } else if (varDecl->op == FUNCTION_PROTO) {
        funProto = (FunctionProto *) varDecl;
        if (targetSize != 4) {
          programLogicError ("targetSize is not 4 in case VARIABLE_EXPR in genExpr");
        }
        temp = newTemp (4);
        IRLoadAddr (temp, funProto->newName);
        return temp;
      } else {
        programLogicError ("varDecl->myDef is wrong in genExpr");
      }

    case STRING_CONST:                                 // in genExpr
    case CALL_EXPR:
    case SEND_EXPR:
    case SELF_EXPR:
    case SUPER_EXPR:
    case FIELD_ACCESS:
    case ARRAY_ACCESS:
    case CONSTRUCTOR:
    case CLOSURE_EXPR:
    case AS_INTEGER_EXPR:
    case ARRAY_SIZE_EXPR:
    case IS_INSTANCE_OF_EXPR:
    case IS_KIND_OF_EXPR:
    case SIZE_OF_EXPR:

      temp = newTemp (targetSize);
      genExprInto (temp, node, NULL, NULL);
      return temp;

    case AS_PTR_TO_EXPR:                                // in genExpr
      return genExpr (((AsPtrToExpr *) node)->expr, targetSize);

    case DYNAMIC_CHECK:                                // in genExpr

      //    printf ("DYNAMIC_CHECK in genExpr, node = ");
      //    pretty (node);
      varDecl = genAddressOf (node);  // This will perform the dynamic check...
      temp = newTemp (targetSize);
      IRMove (temp,  NULL, NULL, varDecl, targetSize);
      return temp;

    default:                                           // in genExpr

      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unknown op in genExpr");
  }

  programLogicError ("All cases should have returned in genExpr");
  return NULL;

}



// genExprInto (target, expr, trueLabel, falseLabel)
//
// This routine generates IR code to evaluate the expression and to move
// a value into the target variable.  The "target" will be either:
//    Local
//    Global
//    Parameter
//    ClassField
//
// If the expr is a boolean expression, the target may be null, in which case
// "trueLabel" and "falseLabel" tell where to jump to and will not be null.
// Either we will have a target OR trueLabel/falseLabel, BUT never both.
// (However, if one label is present, both will be present.)  If the expression
// has type "void", then target and trueLabel/falseLabel will be null.
//
// If trueLabel and falseLabel are present, then this routine will generate code
// to evaluate the expression and jump to the appropriate label.  The boolean value
// will be used in test-and-branch instructions instead of being saved.  For example,
//        if (...expr...)
// will invoke this routine with labels for the then-stmts and the else-stmts, while
//        b = (...expr...)
// will invoke this routine with a target (namely, "b").
//
void genExprInto (VarDecl * target,
                  Expression * node,
                  char * trueLabel,
                  char * falseLabel) {
    BoolConst * boolConst;
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
    FunctionProto * funProto;
    VarDecl * varDecl, * tempVar, * temp;
    char * descriptorLabel;

  if (node==NULL) {
    programLogicError ("genExprInto called with node==NULL");
  }
  if (trueLabel!=NULL && target!=NULL) {
    programLogicError ("genExprInto called with trueLabel!=NULL && target!=NULL");
  }
  if ((trueLabel==NULL) != (falseLabel==NULL)) {
    programLogicError ("genExprInto called with (trueLabel==NULL) != (falseLabel==NULL)");
  }

  //   printf ("=== genExprInto: ");
  //   pretty (node);

  switch (node->op) {

    case INT_CONST:                                      // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case INT_CONST in genExprInto");
      }
      IRAssign4 (target, node);
      return;

    case DOUBLE_CONST:                                   // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case DOUBLE_CONST in genExprInto");
      }
      IRAssign8 (target, node);
      return;

    case CHAR_CONST:                                     // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case CHAR_CONST in genExprInto");
      }
      IRAssign1 (target, node);
      return;

    case STRING_CONST:                                   // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case STRING_CONST in genExprInto");
      }
      IRLoadAddr (target, ((StringConst *) node)->nameOfConstant);
      return;

    case BOOL_CONST:                                     // in genExprInto

      if (trueLabel) {
        boolConst = (BoolConst *) node;
        if (boolConst->ivalue == 0) {
          IRGoto (falseLabel);
        } else {
          IRGoto (trueLabel);
        }
      } else {
        IRAssign1 (target, node);
      }
      return;

    case NULL_CONST:                                     // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case NULL_CONST in genExprInto");
      }
      IRAssign4 (target, constantIntZero);
      return;

    case CALL_EXPR:                                      // in genExprInto

      genCallExpr (target, (CallExpr *) node, trueLabel, falseLabel);
      return;

    case SEND_EXPR:                                      // in genExprInto

      genSendExpr (target, (SendExpr *) node, trueLabel, falseLabel);
      return;

    case SELF_EXPR:                                      // in genExprInto
    case SUPER_EXPR:                                     // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case SELF_EXPR / SUPER_EXPR in genExprInto");
      }
      IRLoadSelfPtr (target);
      return;

    case FIELD_ACCESS:                                   // in genExprInto

      //    printf ("Processing FIELD_ACCESS in genExprInto\n");
      fieldAccess = (FieldAccess *) node;
      tempVar = genAddressOf (fieldAccess->expr);
      temp = newTemp (4);
      IRIncrVarDirect (temp, tempVar, NULL, fieldAccess->offset, 0);
                                           // wantOverflowTest = 0
      if (trueLabel) {
        IRBoolEqZeroIndirect (temp, falseLabel);
        IRGoto (trueLabel);
      } else {
        IRMove (target, NULL, NULL, temp, target->sizeInBytes);
      }
      return;

    case ARRAY_ACCESS:                                   // in genExprInto

      arrayAccess = (ArrayAccess *) node;
      //   printf ("case ARRAY_ACCESS in genExprInto: ");
      //   pretty (arrayAccess);
      tempVar = genAddressOf (arrayAccess);
      if (trueLabel) {
        IRBoolEqZeroIndirect (tempVar, falseLabel);
        IRGoto (trueLabel);
      } else {
        IRMove (target, NULL, NULL, tempVar, target->sizeInBytes);
      }
      return;

    case CONSTRUCTOR:                                    // in genExprInto

      if (target == NULL) {
        programLogicError ("target is NULL, in case CONSTRUCTOR in genExprInto");
      }
      constructor = (Constructor *) node;
      tempVar = genConstructor (constructor, target);
      return;

    case CLOSURE_EXPR:                                   // in genExprInto

      if (target == NULL) {
        programLogicError ("Target is NULL, in case CLOSURE_EXPR in genExprInto");
      }
      closureExpr = (ClosureExpr *) node;
      //   printf ("CLOSURE EXPR in genExprInto: ");
      //   pretty (closureExpr);
      IRLoadAddr (target, closureExpr->function->newName);
      return;

    case VARIABLE_EXPR:                                  // in genExprInto

      var = (VariableExpr *) node;
      if (var->myDef == NULL) {
        programLogicError ("var->myDef == NULL in genExprInto");
      }
      varDecl = (VarDecl *) var->myDef;
      if ((varDecl->op == LOCAL) ||
          (varDecl->op == GLOBAL) ||
          (varDecl->op == CLASS_FIELD) ||
          (varDecl->op == PARAMETER)) {
        if (varDecl->sizeInBytes == 1) {
          if (trueLabel) {
            IRBoolTest (varDecl, trueLabel, falseLabel);
          } else {
            IRAssign1 (target, varDecl);
          }
        } else if (varDecl->sizeInBytes == 4) {
          IRAssign4 (target, varDecl);
        } else if (varDecl->sizeInBytes == 8) {
          IRAssign8 (target, varDecl);
        } else {
          IRMove (target, NULL, varDecl, NULL, varDecl->sizeInBytes);
        }
        return;
      } else if (varDecl->op == FUNCTION_PROTO) {
        funProto = (FunctionProto *) varDecl;
        IRLoadAddr (target, funProto->newName);
        return;
      } else {
        programLogicError ("varDecl->myDef is wrong in genExprInto");
      }

    case AS_PTR_TO_EXPR:                                 // in genExprInto

      asPtrToExpr = (AsPtrToExpr *) node;
      genExprInto (target, asPtrToExpr->expr, trueLabel, falseLabel);
      return;

    case AS_INTEGER_EXPR:                                // in genExprInto

      asIntegerExpr = (AsIntegerExpr *) node;
      genExprInto (target, asIntegerExpr->expr, trueLabel, falseLabel);
      return;

    case ARRAY_SIZE_EXPR:                                // in genExprInto

      arraySizeExpr = (ArraySizeExpr *) node;
      tempVar = genAddressOf (arraySizeExpr->expr);
      if (trueLabel) {
        programLogicError ("Unexpected trueLabel in ARRAY_SIZE_EXPR in genExprInto");
      }
      if (target->sizeInBytes != 4) {
        programLogicError ("sizeInBytes is not 4 in ARRAY_SIZE_EXPR in genExprInto");
      }
      IRMove (target, NULL, NULL, tempVar, 4);
      return;

    case IS_INSTANCE_OF_EXPR:                            // in genExprInto

      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      if (isInstanceOfExpr->classDef == NULL) {
        programLogicError ("classDef is NULL in IS_INSTANCE_OF_EXPR");
      }
      tempVar = genAddressOf (isInstanceOfExpr->expr);
      if (trueLabel) {
        IRIsInstanceOf (NULL, tempVar, isInstanceOfExpr->classDef->newName, falseLabel);
        IRGoto (trueLabel);
      } else {
        IRIsInstanceOf (target, tempVar, isInstanceOfExpr->classDef->newName, NULL);
      }
      return;

    case IS_KIND_OF_EXPR:                                // in genExprInto

      isKindOfExpr = (IsKindOfExpr *) node;
      if (isKindOfExpr->classOrInterface == NULL) {
        programLogicError ("isKindOfExpr->classOrInterface is NULL in IS_KIND_OF_EXPR");
      }
      descriptorLabel = isKindOfExpr->classOrInterface->newName;
      tempVar = genAddressOf (isKindOfExpr->expr);
      if (trueLabel) {
        IRIsKindOf (NULL, tempVar, descriptorLabel, falseLabel);
        IRGoto (trueLabel);
      } else {
        IRIsKindOf (target, tempVar, descriptorLabel, NULL);
      }
      return;

    case SIZE_OF_EXPR:                                   // in genExprInto

      programLogicError ("SIZE_OF_EXPR should have been eliminated in check, in genExprInto");
      return;

    case DYNAMIC_CHECK:                                  // in genExprInto

      dynamicCheck = (DynamicCheck *) node;
      //   printf ("=== In genExprInto, node = ");
      //   pretty (node);
      if (trueLabel) {
        programLogicError ("Objects and arrays are never booleans");
      }

      // NOTE: In the approach taken here, we call "genAddressOf" to do the work.
      // In some cases (e.g., "new" constructors), genAddressOf will create the
      // object, place it in a temporary, and return the address of this temp.
      // Then, we immediately do a "move", copying the object.  This results
      // in an extra temporary and move, which may be significant if the item in
      // question is a large array or large object.  This approach may generate
      // some unnecessary tests.  For example, we always know the size of a "new"
      // at compile time, yet we will generate a dynamic test.
      //
      // Another approach would be to do something like this:
      //      genExprInto (target, dynamicCheck->expr);
      //      gen code to do the check on "target"
      // However, the problem is that "target" might be too small and the code
      // to move in the object might overwrite some stuff before the check can
      // be done.

      //  Case 8: OBJECT COPY: ... := *objPtr
      if (dynamicCheck->kind == 8) {
        varDecl = genAddressOf (node);    // This will perform the dynamic check...
        //   IRCheckDPT2 (varDecl, dynamicCheck->expectedClassDef);
        IRMove (target,  NULL, NULL, varDecl, target->sizeInBytes);
        return;

      //  Case 9: ARRAY COPY: arr[10] := arr[10]
      } else if (dynamicCheck->kind == 9) {
        varDecl = genAddressOf (node);    // This will perform the dynamic check...
        //   IRCheckArraySizeInt (varDecl, dynamicCheck->expectedArraySize);
        IRMove (target,  NULL, NULL, varDecl, dynamicCheck->arraySizeInBytes);
        return;

      //  Case 10: ARRAY COPY: arr[10] := arr[*]
      } else if (dynamicCheck->kind == 10) {
        varDecl = genAddressOf (node);    // This will perform the dynamic check...
        //   IRCheckArraySizeInt (varDecl, dynamicCheck->expectedArraySize);
        IRMove (target,  NULL, NULL, varDecl, dynamicCheck->arraySizeInBytes);
        return;

      } else {
        programLogicError ("Unexpected kind in DYNAMIC_CHECK in genExprInto");
      }
      return;


    default:                                             // in genExprInto

      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unknown op in genExprInto");

  }

  programLogicError ("All cases should have returned in genExprInto");
  return;

}



// genAddressOf (expr) --> VarDecl
//
// This routine generates IR code to compute the address of the data.
// It returns one of:
//    Local
//    Global
//    Parameter
//    ClassField
//
// In most cases, it will allocate a temp, generate code to move the address
// into that temp, and return the temp.
//
// In some cases (e.g., "*p"), it will return a variable directly (e.g., "p").
//
// Note: we will only call this routine on objects or arrays; we should never
// call it on other sorts of values.  We will also call it for the LValue in
// a FOR_STMT, which may be any integer-valued expression of the form:
//       LValue --> Expr [ Expr ]
//              --> Expr . ID
//              --> * Expr           Where the expr has type ptr
//              --> ID               Where id is either a local, global,
//                                     parameter or classField
//
// (It might be nice to pass in a target var, which will eliminate the creation
// of some unnecessary temps...)
//
VarDecl * genAddressOf (Expression * node) {
    CallExpr * callExpr;
    VariableExpr * var;
    VarDecl * varDecl;
    Local * temp;
    FunctionProto * funProto;
    SendExpr * sendExpr;
    AstNode * x;
    FieldAccess * fieldAccess;
    ArrayAccess * arrayAccess;
    DynamicCheck * dynamicCheck;
    Constructor * constructor;
    int i, n;
    CountValue * countValue;
    IntConst * intConst;

  if (node==NULL) {
    programLogicError ("genAddressOf called with node == NULL");
  }

  //   printf ("=== genAddressOf: ");
  //   pretty (node);

  switch (node->op) {

    case INT_CONST:                                 // in genAddressOf
    case DOUBLE_CONST:
    case CHAR_CONST:
    case BOOL_CONST:
    case NULL_CONST:
    case STRING_CONST:
    case IS_INSTANCE_OF_EXPR:
    case IS_KIND_OF_EXPR:
    case CLOSURE_EXPR:
    case AS_PTR_TO_EXPR:
    case AS_INTEGER_EXPR:
    case ARRAY_SIZE_EXPR:

      printf ("\nnode->op = %s\n", symbolName (node->op));
      printf ("node = ");
      pretty (node);
      programLogicError ("In genAddressOf, we should not be asking for the address of this value");

    case SIZE_OF_EXPR:
      programLogicError ("SIZE_OF_EXPR should have been eliminated in check, in genAddressOf");

    case VARIABLE_EXPR:                             // in genAddressOf

      var = (VariableExpr *) node;
      if (var->myDef == NULL) {
        programLogicError ("var->myDef == NULL in genAddressOf");
      }
      varDecl = (VarDecl *) var->myDef;
      if ((varDecl->op == LOCAL) ||
          (varDecl->op == GLOBAL) ||
          (varDecl->op == PARAMETER) ||
          (varDecl->op == CLASS_FIELD)) {
        temp = newTemp (4);
        IRLoadAddr2 (temp, varDecl);
        return temp;
      } else if (varDecl->op == FUNCTION_PROTO) {
        funProto = (FunctionProto *) varDecl;
        programLogicError ("Should have only arrays and objects, in FUNCTION_PROTO in genAddressOf");
      } else {
        programLogicError ("varDecl->myDef is wrong in genAddressOf");
      }

    case SEND_EXPR:                                 // in genAddressOf

      sendExpr = (SendExpr *) node;
      //   printf ("In genAddressOf, processing ");
      //   pretty (sendExpr);

      // If this is an expression of the form "*expr"...
      if (sendExpr->primitiveSymbol == PRIMITIVE_DEREF) {
        x = genExpr (sendExpr->receiver, 4);  // It should be a ptr, so size==4
        if (x->op != LOCAL &&
            x->op != GLOBAL &&
            x->op != PARAMETER &&
            x->op != CLASS_FIELD &&
            x->op != INT_CONST) {
          programLogicError ("Will not be RealConst, BoolConst, or CharConst");
        }
        IRIntEqZero (x, "_runtimeErrorNullPointer");
        return (VarDecl *) x;
      }

      // If this is another primitive
      if (sendExpr->primitiveSymbol != 0) {
        programLogicError ("Unexpected primitive in SEND_EXPR in genAddressOf");
      }

      // Else, we need to evaluate the expression into a temporary and return the
      // address of that temporary.  For example, consider
      //       genAddressOf ("x.foo()")
      // Assume foo returns an object; we'd need its address when we call bar in
      //       (x.foo()) . bar()
      x = genExpr (sendExpr, sendExpr->myProto->retSize);
      if (x->op == LOCAL) {
        temp = newTemp (4);
        IRLoadAddr2 (temp, x);
        return temp;
      } else {
        programLogicError ("GenExpr when applied to non-primitive sendExpr will always return new temp var");
//    } else if ((x->op == INT_CONST) ||
//               (x->op == DOUBLE_CONST) ||
//               (x->op == CHAR_CONST) ||
//               (x->op == BOOL_CONST)) {
//      return genAddressOf ((Expression *) x);
//    } else {
//      programLogicError ("x is unexpected kind in SEND_EXPR in genAddressOf");
      }

    case CALL_EXPR:                                 // in genAddressOf
      callExpr = (CallExpr *) node;
      //     printf ("CALL_EXPR in genAddressOf: ");
      //     pretty (callExpr);
      //     printf ("  retSize = %d\n", callExpr->retSize);
      x = genExpr (callExpr, callExpr->retSize);
      temp = newTemp (4);
      IRLoadAddr2 (temp, x);
      return temp;

    case SELF_EXPR:                                 // in genAddressOf
    case SUPER_EXPR:                                // in genAddressOf
      programLogicError ("SELF_EXPR / SUPER_EXPR is a ptr, not an object, so we should never have it in genAddressOf");

    case FIELD_ACCESS:                              // in genAddressOf
      fieldAccess = (FieldAccess *) node;
      varDecl = genAddressOf (fieldAccess->expr);
      temp = newTemp (4);
      IRIncrVarDirect (temp, varDecl, NULL, fieldAccess->offset, 0);
      return temp;

    case ARRAY_ACCESS:                              // in genAddressOf
      arrayAccess = (ArrayAccess *) node;
      varDecl = genAddressOf (arrayAccess->arrayExpr);
      x = genExpr (arrayAccess->indexExpr, 4);
      temp = newTemp (4);
      if (arrayAccess->sizeOfElements <= 0) {
        // printf ("DEBUGGING....   In genAddressOf>ARRAY_ACCESS: ");
        // pretty (node);
        // printAst (5, node);
        programLogicError ("In genAddressOf>ARRAY_ACCESS, elementSize is zero or less");
      }
      IRArrayIndex (varDecl, x, temp, arrayAccess->sizeOfElements);
      return temp;

    case CONSTRUCTOR:                               // in genAddressOf

      constructor = (Constructor *) node;
      varDecl = genConstructor (constructor, NULL);
      temp = newTemp (4);
      IRLoadAddr2 (temp, varDecl);
      return temp;

    case DYNAMIC_CHECK:                             // in genAddressOf
      //   printf ("DYNAMIC_CHECK in genAddressOf, node = ");
      //   pretty (node);
      dynamicCheck = (DynamicCheck *) node;
      varDecl = genAddressOf (dynamicCheck->expr);

      //  Case 8: OBJECT COPY: ... := *objPtr
      if (dynamicCheck->kind == 8) {
        //   printf ("  Dynamic check 8 in genAddressOf\n");
        IRCheckDPT2 (varDecl, dynamicCheck->expectedClassDef);

      //  Case 9: ARRAY COPY: arr[10] := arr[10]
      } else if (dynamicCheck->kind == 9) {
        //   printf ("  Dynamic check 9 in genAddressOf\n");
        IRCheckArraySizeInt (varDecl, dynamicCheck->expectedArraySize);

      //  Case 10: ARRAY COPY: arr[10] := arr[*]
      } else if (dynamicCheck->kind == 10) {
        //   printf ("  Dynamic check 10 in genAddressOf\n");
        IRCheckArraySizeInt (varDecl, dynamicCheck->expectedArraySize);

      } else {
        programLogicError ("Unexpected kind in DYNAMIC_CHECK in genExprInto");
      }
      return varDecl;

    default:
      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unknown op in genAddressOf");

  }

  programLogicError ("All cases should have returned in genAddressOf");
  return NULL;

}



// genConstructor (expr, target) -> VarDecl
//
// This routine is passed a constructor expression and a "target" variable,
// which will be a LOCAL, GLOBAL, PARM, or CLASS_FIELD or may be null.
// If "target" is NULL, this routine will allocate a temp of the correct size.
// This routine will generate the code to setup a new array, record, or object.
// This routine will return "target". 
//
// In the case of a NEW, this routine will move an initialized array, record,
// or object into "target".
//
// In the case of ALLOC, this routine will allocate the memory,
// initialize the array, record, or object and move a pointer to the new thing
// into "target".
//
VarDecl * genConstructor (Constructor * constructor, VarDecl * target) {
  int n, elementSize, numberOfElements, size;
  CountValue * countValue;
  IntConst * intConst;
  VarDecl * tempPtr, * counter, * byteCount, * loopCounter;
  ArrayType * aType;
  AstNode * value;
  char * label;
  FieldInit * f;

  switch (constructor->allocKind) {

    case NEW:

      switch (constructor->kind) {

        case ARRAY:              // genConstructor: NEW ARRAY

          IRComment ("  NEW ARRAY Constructor...");
          //    printf ("======= NEW ARRAY: ");  pretty (constructor);
          //    printf ("  constructor->sizeInBytes = %d\n", constructor->sizeInBytes);
          aType = (ArrayType *) constructor->type;
          if (aType->op != ARRAY_TYPE) {
            programLogicError ("In genConstructor, expecting array type");
          }
          elementSize = aType->sizeOfElements;
          //    printf ("  elementSize = %d\n", elementSize);
          intConst = (IntConst *) aType->sizeExpr;
          if ((intConst == NULL) || (intConst->op != INT_CONST)) {
            programLogicError ("In genConstructor, arrayType-sizeExpr is not IntConst");
          }
          numberOfElements = intConst->ivalue;
          //    printf ("  numberOfElements = %d\n", numberOfElements);

          if (target == NULL) {
            target = newTemp (constructor->sizeInBytes);
          }
          tempPtr = newTemp (4);
          IRLoadAddr2 (tempPtr, target);
          IRIncrVarDirect (tempPtr, tempPtr, NULL, 4, 0);
                         // wantOverflowTest = 0
          counter = newTemp (4);

          // Run through the count-value pairs and generate code for each...
          n = 0;
          for (countValue = constructor->countValueList;
               countValue;
               countValue = countValue->next) {
            IRComment ("  Next value...");
            value = genExpr (countValue->value, elementSize);
            if (countValue->count == NULL) {
              n++;
              IRMove (NULL, tempPtr, value, NULL, elementSize);
              IRIncrVarDirect (tempPtr, tempPtr, NULL, elementSize, 0);
                         // wantOverflowTest = 0
            } else {
              intConst = (IntConst *) countValue->count;
              if (intConst->op == INT_CONST) {
                // IRComment ("  Fixed count...");
                n += intConst->ivalue;
                // Generate this code:
                //    counter := ivalue
                //    loop:
                //       tempPtr = value
                //       tempPtr := tempPtr + elementSize
                //       counter :- counter - 1
                //       if counter > 0 goto loop
                label = newLabel ();
                IRSet (counter, intConst->ivalue);
                IRLabel (label);
                IRMove (NULL, tempPtr, value, NULL, elementSize);
                IRIncrVarDirect (tempPtr, tempPtr, NULL, elementSize, 0);
                         // wantOverflowTest = 0
                IRIncrVarDirect (counter, counter, NULL, -1, 0);
                         // wantOverflowTest = 0
                IRIntNeZero (counter,label);
//              } else {
//                programLogicError ("For NEW ARRAY, we should have only fixed counts (and they should be within 16 bits limitation)");
//                IRComment ("  Compute count...");
//                countValue->countTemp = genExpr (countValue->count, 4);
              }
            }
          }

          //    printf ("  n = %d\n", n);
          if (n != numberOfElements) {
            programLogicError ("In genConstructor, n != numberOfElements");
          }
          IRComment ("  Initialize the array size...");
          IRSet (target, numberOfElements);
          return target;

        case CLASS:               // genConstructor: NEW CLASS
        case RECORD:              // genConstructor: NEW RECORD

          if (constructor->kind == RECORD) {
            IRComment ("  NEW RECORD Constructor...");
          } else {
            IRComment ("  NEW CLASS Constructor...");
          }
          size = constructor->sizeInBytes;
          //      printf ("new CLASS/RECORD = ");
          //      pretty (constructor);
          //      printf ("size = %d\n", size);
          if (target == NULL) {
            target = newTemp (size);
          }

          // If this is an object initialization with missing field inits...
          if ((constructor->kind == CLASS) &&
              (size > 4) &&
              (constructor->fieldInits == NULL)) {
            IRZeroMemory (target, NULL, size);
          }

          // If this is an object, fill in dispatch table pointer...
          if (constructor->kind == CLASS) {
            IRLoadAddr (target, constructor->myClass->newName);
          }

          // Run thru fieldInits and generate code to initialize each...
          constructor->fieldInits = sortFieldInits (constructor->fieldInits);
          tempPtr = newTemp (4);
          for (f = constructor->fieldInits; f; f = f->next) {
            value = genExpr (f->expr, f->sizeInBytes);
            IRLoadAddrWithIncr (tempPtr, target, f);
            IRMove (NULL, tempPtr, value, NULL, f->sizeInBytes);
          }

          return target;

        default:
          printf ("\nconstructor->kind = %s\n", symbolName (constructor->kind));
          programLogicError ("Unexpected kind in genConstructor");
      }

    case ALLOC:

      switch (constructor->kind) {

        case ARRAY:              // genConstructor: ALLOC ARRAY

          IRComment ("  ALLOC ARRAY Constructor...");
          //     printf ("======= ALLOC ARRAY: ");  pretty (constructor);
          //     printf ("  constructor->sizeInBytes = %d\n", constructor->sizeInBytes);
          aType = (ArrayType *) constructor->type;
          if (aType->op != ARRAY_TYPE) {
            programLogicError ("In genConstructor, expecting array type");
          }
          elementSize = aType->sizeOfElements;
          //     printf ("  elementSize = %d\n", elementSize);
          counter = newTemp (4);
          IRSet (counter, 0);

          // Run through the count-value pairs and generate code for each...
          IRComment ("  Evaluate any count and value expressions...");
          n = 0;
          for (countValue = constructor->countValueList;
               countValue;
               countValue = countValue->next) {
            if (countValue->count == NULL) {
              // IRComment ("  Missing count, 1 assumed...");
              n++;
            } else {
              intConst = (IntConst *) countValue->count;
              if ((intConst->op == INT_CONST) &&
                  within16Bits (intConst->ivalue) &&
                  within16Bits (n + (intConst->ivalue))) {
                // IRComment ("  Fixed count...");
                n += intConst->ivalue;
                countValue->countTemp = countValue->count;
              } else {
                // IRComment ("  Evaluate this count expr and add to running sum...");
                countValue->countTemp = genExpr (countValue->count, 4);
                IRIAdd (counter, counter, countValue->countTemp);
              }
            }
            // IRComment ("  Evaluate next value expression...");
            countValue->valueTemp = genExpr (countValue->value, elementSize);
          }
          if (n != 0) {
            IRIncrVarDirect (counter, counter, NULL, n, 0);
                         // wantOverflowTest = 0
          }
          byteCount = newTemp (4);
          if (target == NULL) {
            target = newTemp (4);
          }
          IRComment ("  Compute size in bytes and call alloc...");
          IRMultiplyVarImmed (byteCount, counter, elementSize);
          IRIncrVarDirect (byteCount, byteCount, NULL, 4, 1);  // wantOverflowTest = 1
          IRAlloc (target, byteCount);
          tempPtr = newTemp (4);
          IRMove (tempPtr, NULL, target, NULL, 4);
          IRMove (NULL, tempPtr, counter, NULL, 4);
          IRIncrVarDirect (tempPtr, tempPtr, NULL, 4, 0);  // wantOverflowTest = 0
          loopCounter = newTemp (4);

          // Run through the count-value pairs and generate code to initialize array...
          for (countValue = constructor->countValueList;
               countValue;
               countValue = countValue->next) {
            if (countValue->count == NULL) {
              IRComment ("  Initialize 1 element...");
              IRMove (NULL, tempPtr, countValue->valueTemp, NULL, elementSize);
              IRIncrVarDirect (tempPtr, tempPtr, NULL, elementSize, 0);
                                                   // wantOverflowTest = 0
            } else {
              intConst = (IntConst *) countValue->count;
              IRComment ("  Initialize many entries in a loop...");
              // Generate the following code:
              //      loopCounter = count
              //      if loopCounter <= 0 goto runtimeError
              //      loop:
              //        *tempPtr = value
              //        tempPtr = tempPtr + elementSize
              //        loopCounter = loopCounter - 1
              //        if loopCount != 0 goto loop
              IRMove (loopCounter, NULL, countValue->countTemp, NULL, 4);
              IRIntLeZero (loopCounter, "_runtimeErrorArrayCountNotPositive");
              label = newLabel ();
              IRLabel (label);
              IRMove (NULL, tempPtr, countValue->valueTemp, NULL, elementSize);
              IRIncrVarDirect (tempPtr, tempPtr, NULL, elementSize, 0);
                                                   // wantOverflowTest = 0
              IRIncrVarDirect (loopCounter, loopCounter, NULL, -1, 0);
                                                   // wantOverflowTest = 0
              IRIntNeZero (loopCounter, label);
            }
          }
          return target;

        case CLASS:              // genConstructor: ALLOC CLASS
        case RECORD:             // genConstructor: ALLOC RECORD

          if (constructor->kind == RECORD) {
            IRComment ("  ALLOC RECORD Constructor...");
          } else {
            IRComment ("  ALLOC CLASS Constructor...");
          }
          size = constructor->sizeInBytes;
          //    printf ("alloc CLASS/RECORD = ");
          //    pretty (constructor);
          //    printf ("size = %d\n", size);

          IRComment ("  Call alloc...");
          byteCount = newTemp (4);
          IRSet (byteCount, size);
          if (target == NULL) {
            target = newTemp (4);
          }
          IRAlloc (target, byteCount);

          // If this is an object initialization with missing field inits...
          if ((constructor->kind == CLASS) &&
              (size > 4) &&
              (constructor->fieldInits == NULL)) {
            IRZeroMemory (NULL, target, size);
          }

          // If it is a class, then save ptr to dispatch table...
          if (constructor->kind == CLASS) {
            IRLoadAddrIndirect (target, constructor->myClass->newName);
          }

          // Run through the fields and initialize them...
          constructor->fieldInits = sortFieldInits (constructor->fieldInits);
          tempPtr = newTemp (4);
          for (f = constructor->fieldInits; f; f = f->next) {
            value = genExpr (f->expr, f->sizeInBytes);
            IRLoadAddrWithIncr2 (tempPtr, target, f);
            IRMove (NULL, tempPtr, value, NULL, f->sizeInBytes);
          }
          return target;


        default:
          printf ("\nconstructor->kind = %s\n", symbolName (constructor->kind));
          programLogicError ("Unexpected kind in genConstructor");
      }

    default:
      printf ("\nconstructor->allocKind = %s\n", symbolName (constructor->allocKind));
      programLogicError ("Unexpected allocKind in genConstructor");
  }
}



// newLabel ()
//
// This function returns a newly created string of the form "_Label_43".
//
char * newLabel () {
  return newName ("Label");
}



// newName (char*) --> char*
//
// This function returns a newly created string of the form "_xxxx_43".  The
// integer part is incremented on each call, making the returned string unique
// from all previous strings returned by this function.  The "xxxx" part is
// from the "str" argument.
//
char * newName (char * str) {
  static int next = 1;
  char * p = (char *) calloc (1, strlen (str) + 12);
  if (p==0) {
    programLogicError ("Calloc failed in newName; out of memory perhaps???");
  }
  sprintf (p, "_%s_%d", str, next++);
  return p;
}



// sanitizedPackageName (packageName) --> newPackName
//
// This routine takes a String such as
//    MyPack
// and returns something like
//    _P_MyPack_
//
// If the package name contains underscore characters, then each is
// replaced with __.  For example, the package name:
//    MyPack_10_5
// is translated into:
//    _P_MyPack__10__5_
//
// We use two underscores instead of one to make sure no name-collisions
// occur.  This is necessary since we will concatenate package names
// with other names, such as variable names.
//
// As an example, consider two different package names and two variables;
// without doubling, the sanitized version of the variable names might
// end up being identical:
//
//    Package   Variable   Name, no doubling   Name, with doubling
//    =======   ========   =================   ===================
//    xx        yy_zz      _P_xx_yy_zz         _P_xx_yy_zz
//    xx_yy     zz         _P_xx_yy_zz         _P_xx__yy_zz
//
char * sanitizedPackageName (String * packageName) {
  char * result = (char *) calloc (1, 2 * packageName->length + 5);
  char * p = result;
  char * from = packageName->chars;
  int i;
  *(p++) = '_';
  *(p++) = 'P';
  *(p++) = '_';
  for (i = packageName->length; i>0; i--) {
    if (isalpha (*from) || isdigit (*from)) {
      *(p++) = *(from);
    } else {
      *(p++) = '_';
      *(p++) = '_';
    }
    from++;
  }
  *(p++) = '_';
  *(p++) = '\0';
  return result;
}



// newMethodName (sanitizedClassName, i) --> char*
//
// This routine is passed a sanitizedClassName such as "_P_MyPackageName_MyClass",
// and an integer.  It returns a newly created string, such as
//        _Method_P_MyPackageName_MyClass_43
// This is composed of 3 parts:
//        _Method   _P_MyPackageName_MyClass_    43
//
// The integer part is normally incremented for each method in the class.
// Thus, the methods in "MyClass" in "MyPackageName" will be given the names:
//        _Method_P_MyPackageName_MyClass_1
//        _Method_P_MyPackageName_MyClass_2
//        _Method_P_MyPackageName_MyClass_3
//        ...
//
char * newMethodName (char * sanitizedClassName, int i) {
  char * p = (char *) calloc (1, strlen (sanitizedClassName) + 20);
  if (p==0) {
    programLogicError ("Calloc failed in newName; out of memory perhaps???");
  }
  sprintf (p, "_Method%s_%d", sanitizedClassName, i);
  return p;
}



//
// genLineNumber (node, stmtCode)
//
// This routine generates a "setLineNumber" instruction.  It is passed an
// AstNode, from which it extracts the current source code line number.
// It is also passed a 2 character code indicating what sort of a statement
// this is.  Later, we'll generate an instruction to move this code into r10.
//
void genLineNumber (AstNode * node, char * stmtCode) {
  if (node == NULL) {
    programLogicError ("node is NULL in genLineNumber");
  }
  IRSetLineNumber (extractLineNumber (node->tokn), stmtCode);
}



// newTemp (sizeInBytes) --> Local
//
// This routine creates a new temp variable and returns a pointer to it.  It is
// passed the size of the temp in bytes.  It assumes that the global variable
// "currentFunOrMeth" points to a function or method.  A new LOCAL is added to
// this function or method.
//
Local * newTemp (int sizeInBytes) {
  Local * local;
  Function * fun;
  Method * meth;

  if (sizeInBytes < 0) {
    programLogicError ("sizeInBytes < 0 within newTemp");
  }

  if (currentFunOrMeth == NULL) {
    programLogicError ("currentFunOrMeth is NULL within newTemp");
  }
  local = new Local ();
  local->positionAt (currentFunOrMeth);
  local->id = lookupAndAdd (newName("temp"), ID);
  // local->type==NULL signals a temp.
  // local->offset was initialized to -1
  local->sizeInBytes = sizeInBytes;
  // local->varDescLabel was initialized to NULL
  // local->initExpr was initialized to NULL
  // local->wasUsed was only needed during checking

  // Add this Local to the front of the "locals" list.
  if (currentFunOrMeth->op == FUNCTION) {
    fun = (Function *) currentFunOrMeth;
    local->next = fun->locals;
    fun->locals = local;
  } else {
    meth = (Method *) currentFunOrMeth;
    local->next = meth->locals;
    meth->locals = local;
  }
  return local;  
}
