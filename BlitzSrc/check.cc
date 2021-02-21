// check.cc  --  Routines related to semantic checking
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
//   08/01/02 - Harry H. Porter III
//
// Modifcations by:
//   03/15/06 - Harry H. Porter III
//
//

#include "main.h"




// topoProcessAllPackages ()
//
// This function runs through all packages and reorders the header list so that
// all packages come before their sub-packages.  It also sets the "myDef" field
// for each header.
//
void topoProcessAllPackages () {
  Header * tempList = NULL;
  Header * hdr, * next;
  Mapping<String,AstNode> * tempMapping;
  AstNode * def;

  // printf ("Here is the  header list before topoProcessAllPackages:\n");
  // for (hdr = headerList; hdr != NULL; hdr = hdr->next) {
  //   printf ("  ");
  //   printString (stdout, hdr->packageName);
  //   printf ("\n");
  // }

  // headerMapping->print (0);

  // This mapping is from package names to packages.  It is used to make
  // sure each package has a distinct name.
  tempMapping = new Mapping<String,AstNode> (10, NULL);


  // Run through all headers and put them on a temp list.
  for (hdr = headerList; hdr != NULL; hdr = next) {
    def = tempMapping->find (hdr->packageName);
    if (def == NULL) {
      tempMapping->enter (hdr->packageName, hdr);
    } else if (def != hdr) {
      error (hdr, "This .h file uses the same package name as another .h file");
      error2 (def, "Here is the other .h file");
    }
    next = hdr->next;
    hdr->next = NULL;
    hdr->tempNext = tempList;
    tempList = hdr;
  }
  headerList = NULL;
  delete tempMapping;

  // Run through all headers and call topoProcessOnePackage on each.
  for (hdr = tempList; hdr != NULL; hdr = next) {
    next = hdr->tempNext;
    hdr->tempNext = NULL;
    // printf ("Outer LOOP calling topoProcessOnePackage on %s\n",
    //         hdr->packageName->chars);
    topoProcessOnePackage (hdr);
  }

  // Reverse the headerList.
  hdr = headerList;
  headerList = NULL;
  while (hdr != NULL) {
    next = hdr->next;
    hdr->next = headerList;
    headerList = hdr;
    hdr = next;
  }

  // printf ("Here is the header list after topoProcessAllPackages:\n");
  // for (hdr = headerList; hdr != NULL; hdr = hdr->next) {
  //   printf ("  ");
  //   printString (stdout, hdr->packageName);
  //   printf ("\n");
  // }

}



// topoProcessOnePackage (hdr)
//
// Mark this package as "in process".  Then call self recursively to process
// all super-packages.  Finally mark this package as "done" and add it to
// the growing list of packages.  Also check for cycles.
//
void topoProcessOnePackage (Header * hdr) {
  Header * superHeader;
  Uses * uses;

  // printf ("=====  topoProcessOnePackage called on ");
  // printString (stdout, hdr->packageName);
  // printf ("  (mark-in = %d)  =====\n", hdr->mark);

  // This header may have been done earlier in the topoProcessing...
  if (hdr->mark == 2) return;

  // Check for circularity...
  if (hdr->mark == 1) {
    error (hdr, "Circularity in package-use hierarchy (this package appears to use itself, possibly indirectly)");
    return;
  }

  // Set mark to "processing"...
  hdr->mark = 1;

  // Process all super-packages first
  for (uses = hdr->uses; uses != NULL; uses = uses->next) {
    // printf ("LOOKING FOR %s\n", uses->id->chars);
    superHeader = headerMapping->find (uses->id);
    if (superHeader) {
      // printf ("  FOUND %s\n", superHeader->packageName->chars);
      uses->myDef = superHeader;
      topoProcessOnePackage (superHeader);
    }
  }

  // Set mark to "done"...
  hdr->mark = 2;

  // Put this one on the headerList
  hdr->next = headerList;
  headerList = hdr;

  // // Fill in the fieldMapping and the methodMapping
  // // Use the superclass's mapping or the global mapping as the
  // // surrounding mapping.  Make the initial mapping size 2 since most
  // // classes have at least 2 fields and 2 methods.
  // if (hdr->superclass == NULL) {
  //   hdr->fieldMapping = new Mapping<String,AstNode> (2, globalMapping);
  //   hdr->methodMapping = new Mapping<String,AstNode> (2, globalMapping);
  // } else {
  //   hdr->fieldMapping = new Mapping<String,AstNode> (2, hdr->superclass->fieldMapping);
  //   hdr->methodMapping = new Mapping<String,AstNode>
  //                                 (2, hdr->superclass->methodMapping);
  // }

}



// buildPackageMapping (Header * hdr)
//
// Run through all packages and build their symbol tables.
//
void buildPackageMapping (Header * hdr) {
  Header * superHeader;
  ConstDecl * con, * nextCon;
  ErrorDecl * errorDecl, * nextErrorDecl, * otherErrorDecl;
  Global * global, * nextGlobal, * lastGlobal;
  TypeDef * typeDef, * nextTypeDef;
  FunctionProto * functionProto, * proto;
  Function * fun;
  Interface * interface, * nextInterface;
  ClassDef * cl, * nextClass;
  AstNode * oldDef;
  Uses * uses;
  Mapping <String, AstNode> * superMapping;
  Mapping <String, String> * renaming;
  Mapping <String, AstNode> * behaviorMapping;
  Renaming * ren;
  AstNode * val;
  String * key, * newStr;
  String * strKey, * strVal;
  Behavior * behav;
  Mapping<String,AstNode> * tempMapping;
  Method * meth;

  hdr->packageMapping = new Mapping<String, AstNode> (100, NULL);
  hdr->errorMapping = new Mapping<String, ErrorDecl> (5, NULL);

  // This mapping is from package names to anything.  It is used to make
  // sure each use clause has a different package name.
  tempMapping = new Mapping<String,AstNode> (5, NULL);

  // Run through the uses list...
  for (uses=hdr->uses; uses; uses=uses->next) {
    // Build the renaming map...
    renaming = new Mapping<String, String> (100, NULL);
    for (ren = uses->renamings; ren; ren=ren->next) {
      if (renaming->alreadyDefined (ren->from)) {
        error (ren, "This symbol appears more than once as a 'from' symbol in this renaming clause");
      } else {
        renaming->enter (ren->from, ren->to);
        if (ren->to->primitiveSymbol) {
          error (ren, "The 'to' symbol happens to be the same as the name of a built-in primitive function; please select a different symbol");
        }
      }

    }

    // Check to make sure this package is only used once...
    oldDef = tempMapping->find (uses->id);
    if (oldDef == NULL) {
      tempMapping->enter (uses->id, uses);
    } else {
      error (oldDef, "This package appears more than once in the 'uses' clause");
      error2 (uses, "Here is the other occurrence");
    }

    // printf ("Inheriting from %s\n", uses->id->chars);
    // printf ("  =====Renaming=====\n");
    // for (strVal = renaming->getFirst(); strVal; strVal = renaming->getNext()) {
    //   strKey = renaming->getItsKey ();
    //   printf ("    %s --> %s\n", strKey->chars, strVal->chars);
    // }
    // printf ("  ==================\n");

    superHeader = uses->myDef;
    if (superHeader) {
      // Run through the super's package mapping.  Add everything there to this package...
      superMapping = superHeader->packageMapping;
      // superMapping->print (6);
      for (val = superMapping->getFirst (); val; val = superMapping->getNext ()) {
        key = superMapping->getItsKey ();
        // printf ("  Inheriting key=%s\n", key->chars);
        // See if we are renaming this thing...
        newStr = renaming->find (key);
        if (newStr) {
          // printf ("    -- Decided to rename!!!   key=%s newStr=%s\n",
          //         key->chars, newStr->chars);
          key = newStr;
        }
        // See if it is already defined and has a different definition...
        oldDef = hdr->packageMapping->find (key);
        if (oldDef && (oldDef != val)) {
          error (val, "The name to be used for this item also names something else in this package");
          error2 (oldDef, "Here is the other item with the same name");
          error2 (hdr, "The naming conflict was detected within this package");
        } else if (oldDef == NULL) {
          hdr->packageMapping->enter (key, val);
        }
      }
      // Run through the super's "errors" and add these to this package...
      for (errorDecl = superHeader->errors; errorDecl; errorDecl = errorDecl->next) {
        newStr = renaming->find (errorDecl->id);
        if (newStr) {
          // printf ("    -- Decided to rename!!!   key=%s newStr=%s\n",
          //         key->chars, newStr->chars);
          key = newStr;
        } else {
          key = errorDecl->id;
        }
        // See if it is already defined and has a different definition...
        otherErrorDecl = hdr->errorMapping->find (key);
        if (otherErrorDecl && (otherErrorDecl != errorDecl)) {
          error (errorDecl, "The name to be used for this error already names a different error in this package");
          error2 (otherErrorDecl, "Here is the other error with the same name");
          error2 (hdr, "The naming conflict was detected within this package");
        } else if (otherErrorDecl == NULL) {
          hdr->errorMapping->enter (key, errorDecl);
        }
      }
    }
    delete renaming;
  }

  delete tempMapping;

  // Run through the constants and add them...
  for (con=hdr->consts; con; con=con->next) {
    oldDef = hdr->packageMapping->find (con->id);
    if (oldDef) {
      error (con, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, con->id, con);
    }
  }

  // Run through the errors and add them...
  for (errorDecl=hdr->errors; errorDecl; errorDecl=errorDecl->next) {
    oldDef = hdr->errorMapping->find (errorDecl->id);
    if (oldDef) {
      error (errorDecl, "This error has already been declared within this package");
      error2 (oldDef, "Here is the other declaration");
    } else {
      hdr->errorMapping->enter (errorDecl->id, errorDecl);
    }
  }

  // Run through the globals and add them...
  for (global=hdr->globals; global; global= (Global *) global->next) {
    oldDef = hdr->packageMapping->find (global->id);
    if (oldDef) {
      error (global, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, global->id, global);
    }
  }

  // Run through the typeDefs and add them...
  for (typeDef=hdr->typeDefs; typeDef; typeDef=typeDef->next) {
    oldDef = hdr->packageMapping->find (typeDef->id);
    if (oldDef) {
      error (typeDef, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, typeDef->id, typeDef);
    }
  }

  // Run through the functionProtos and add them...
  for (functionProto=hdr->functionProtos;
       functionProto;
       functionProto=functionProto->next) {
    oldDef = hdr->packageMapping->find (functionProto->id);
    if (oldDef) {
      error (functionProto, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, functionProto->id, functionProto);
    }
    functionProto->myHeader = hdr;
    // printf ("linking protoname = %s to header\n", functionProto->id->chars);
  }

  // Run though all functionProtos and link them to their headers...
  for (proto=hdr->functionProtos; proto; proto = proto->next) {
  }

  // Run through the interfaces and add them...
  for (interface=hdr->interfaces;
       interface;
       interface=interface->next) {
    oldDef = hdr->packageMapping->find (interface->id);
    if (oldDef) {
      error (interface, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, interface->id, interface);
    }
  }

  // Run through the classDefs and add them...
  for (cl=hdr->classes;
       cl;
       cl=cl->next) {
    oldDef = hdr->packageMapping->find (cl->id);
    if (oldDef) {
      error (cl, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, cl->id, cl);
    }
  }

  // If this is the package we are compiling, keep going.  Else, return now.
  if (hdr != mainHeader) return;
  if (code == NULL) {
    programLogicError ("Should have quit before now if no code file");
  }

  // printf ("    Processing the code portion of this package...\n");

  // Check that the code and header have the same package name...
  if (code->packageName != hdr->packageName) {
      error (code, "The package name in the code file must be the same as the package name in the header file");
      error2 (hdr, "Here is the name used in the header file");
  }

  // Run though the const's and add them to this header...
  for (con=code->consts; con; con = nextCon) {
    nextCon = con->next;
    oldDef = hdr->packageMapping->find (con->id);
    if (oldDef) {
      error (con, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, con->id, con);
    }
    con->isPrivate = 1;
    // Add this constant to mainHeader's constList...
    con->next = hdr->consts;
    hdr->consts = con;
  }

  // Run though the errorDecl's and add them to this header...
  for (errorDecl=code->errors; errorDecl; errorDecl = nextErrorDecl) {
    nextErrorDecl = errorDecl->next;
    oldDef = hdr->errorMapping->find (errorDecl->id);
    if (oldDef) {
      error (errorDecl, "This error has already been declared within this package");
      error2 (oldDef, "Here is the other declaration");
    } else {
      hdr->errorMapping->enter (errorDecl->id, errorDecl);
    }
    // errorDecl->isPrivate = 1;
    // Add this errorDecl to mainHeader's "errors" list...
    errorDecl->next = hdr->errors;
    hdr->errors = errorDecl;
  }

  // Run though the globals and add them to this header...
  lastGlobal = hdr->globals;
  if (lastGlobal) {
    while (lastGlobal->next) {
      lastGlobal = (Global *) lastGlobal->next;
    }
  }
  for (global=code->globals; global; global = nextGlobal) {
    nextGlobal = (Global *) global->next;
    oldDef = hdr->packageMapping->find (global->id);
    if (oldDef) {
      error (global, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, global->id, global);
    }
    global->isPrivate = 1;
    // Add this global to the end of the mainHeader's global list...
    if (lastGlobal) {
      lastGlobal->next = global;
    } else {
      hdr->globals = global;
    }
    lastGlobal = global;
    global->next = NULL;
  }

  // Run though the typeDef's and add them to this header...
  for (typeDef=code->typeDefs; typeDef; typeDef = nextTypeDef) {
    nextTypeDef = typeDef->next;
    oldDef = hdr->packageMapping->find (typeDef->id);
    if (oldDef) {
      error (typeDef, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, typeDef->id, typeDef);
    }
    // Add this typeDef to mainHeader's typeDef List...
    typeDef->next = hdr-> typeDefs;
    hdr->typeDefs = typeDef;
  }

  // Run though the functions and add match them to their prototypes...
  for (fun=code->functions; fun; fun = fun->next) {
    // printf ("Looking at function %s...\n", fun->id->chars);
    proto = (FunctionProto *) hdr->packageMapping->find (fun->id);
    if (proto) {
      if (proto->op != FUNCTION_PROTO) {
        error (fun, "This name is already defined within this package");
        error2 (proto, "Here is the other item with the same name");
      } else {
        if (proto->myFunction != NULL) {
          error (fun, "There are two definitions of this function");
          error2 (proto->myFunction, "Here is the other definition");
        } else if (proto->isExternal) {
          error (proto, "External functions must not have definitions in the code file");
          error2 (fun, "Here is a definition");
        } else if (proto->myHeader != hdr) {
          error (fun, "This function has the same name as a function supplied in another package");
          error2 (proto, "Here is a matching prototype from other package");
        } else {
          // Link this function to its prototype...
          //    printf ("found proto, name = %s  protoname = %s\n", fun->id->chars, proto->id->chars);
          //    printf ("fun->op = %s   proto->op = %s\n", symbolName (fun->op), symbolName (proto->op));
          //    printf ("hdr = %s\n", proto->myHeader->packageName->chars);
          proto->myFunction = fun;
          fun->myProto = proto;
        }
      }
    } else {
      // Create a new prototype and link it to this function...
      //     printf ("Creating new proto\n");
      proto = new FunctionProto ();
      proto->positionAt (fun);
      proto->id = fun->id;
      proto->parmList = fun->parmList;
      proto->retType = fun->retType;
      proto->isPrivate = 1;
      proto->myHeader = hdr;
      proto->myFunction = fun;
      fun->myProto = proto;
      proto->next = hdr->functionProtos;
      hdr->functionProtos = proto;
      addToPackage (hdr, proto->id, proto);
    }
  }
  // Make sure all prototypes have functions...
  for (proto=hdr->functionProtos; proto; proto = proto->next) {
    if (proto->isExternal == 0) {
      if (proto->myFunction == NULL) {
        error (proto, "The code file is missing a definition for this function");
      }
    }
  }
  // Move all functions into the header...
  hdr->functions = code->functions;

  // Run though the interface's and add them to this header...
  for (interface=code->interfaces; interface; interface = nextInterface) {
    nextInterface = interface->next;
    oldDef = hdr->packageMapping->find (interface->id);
    if (oldDef) {
      error (interface, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, interface->id, interface);
      interface->isPrivate = 1;
    }
    // Add this interface to mainHeader's interface List...
    interface->next = hdr->interfaces;
    hdr->interfaces = interface;
  }

  // Run though the classes and add them to this header...
  for (cl=code->classes; cl; cl = nextClass) {
    nextClass = cl->next;
    oldDef = hdr->packageMapping->find (cl->id);
    if (oldDef) {
      error (cl, "This name is already defined within this package");
      error2 (oldDef, "Here is the other item with the same name");
    } else {
      addToPackage (hdr, cl->id, cl);
      cl->isPrivate = 1;
    }
    // Add this class to mainHeader's class List...
    cl->next = hdr->classes;
    hdr->classes = cl;
  }

  // Run though the behaviors and add their methods to the
  // corresponding classes...
  behaviorMapping = new Mapping <String, AstNode> (20, NULL);
  for (behav=code->behaviors; behav; behav = behav->next) {
    // Make sure their is a corresponding class...
    cl = (ClassDef *) hdr->packageMapping->find(behav->id);
    if (cl) {
      if (cl->op != CLASS_DEF) {
        error (behav, "The name used here must match the name of a class");
        error2 (cl, "Here is another item with the same name which is not a class");
      } else {
        // Make sure this behavior does not occur more than once...
        oldDef = behaviorMapping->find (behav->id);
        if (oldDef) {
          error (behav, "There must not be multiple behaviors for any class");
          error2 (oldDef, "Here is another behavior for this class");
        } else {
          // Make a note of this behavior and add its methods to the class...
          behaviorMapping->enter (behav->id, behav);
          if (behav->id->primitiveSymbol) {
            error (behav, "This item happens to have the same name as that of a built-in primitive function; please select a different name");
          }
          cl->methods = behav->methods;
          // Set all methods to point to their "owning" class...
          for (meth = cl->methods; meth; meth = meth->next) {
            meth->myClass = cl;
          }
        }
      }
    } else {
      error (behav, "There is no corresponding class with this name");
    }
  }
  delete behaviorMapping;

}



// addToPackage (hdr, id, item)
//
// This routine adds this item to the packageMapping in this header.
//
void addToPackage (Header * hdr, String * id, AstNode * item) {
  hdr->packageMapping->enter (id, item);
  if (id->primitiveSymbol) {
    error (item, "This item happens to have the same name as that of a built-in primitive function; please select a different name");
  }
}



// topoProcessInterfaces (hdr)
//
// Run through all interfaces in this package.  Reorder the interface
// list so that all interfaces come after interfaces they use.  Also set
// the myDef field for every "typeArg" encountered.
//
void topoProcessInterfaces (Header * hdr) {
  Interface * inter, * next;

  // printf ("Here is the interface list before topoProcessInterfaces:\n");
  // for (inter = hdr->interfaces; inter; inter = inter->next) {
  //   printf ("  %s\n", inter->id->chars);
  // }

  // Run through all interfaces and set their "myHeader" field.
  for (inter = hdr->interfaces; inter; inter = inter->next) {
    inter->myHeader = hdr;
  }

  // Run through all interfaces in this hdr and call topoProcessOneInterface on each.
  tempInterList = NULL;
  for (inter = hdr->interfaces; inter; inter = inter->next) {
    topoProcessOneInterface (inter);
  }

  // The tempInterList now contains all the interfaces, in order.  Go through it.
  // For each interface, if it is in this header, put it on our "interfaces" list.
  // The order will be reversed.
  // printf ("\n\nGoing though tempInterList...\n");
  hdr->interfaces = NULL;
  for (inter = tempInterList; inter; inter = next) {
    // printf ("  Looking at %s\n", inter->id->chars);
    next = inter->tempNext;
    inter->tempNext = NULL;
    if (inter->myHeader == hdr) {
      // printf ("      Adding it to hdr->interfaces\n");
      inter->next = hdr->interfaces;
      hdr->interfaces = inter;
    }
  }

  // printf ("Here is the interface list after topoProcessInterfaces:\n");
  // for (inter = hdr->interfaces; inter != NULL; inter = inter->next) {
  //   printf ("  %s\n", inter->id->chars);
  // }

}



// topoProcessOneInterface (inter)
//
// Mark this interface as "in process".  Then call self recursively to process
// all super-interfaces.  Finally mark this interface as "done" and add it to
// the growing list of interfaces.  Also check for cycles.
//
void topoProcessOneInterface (Interface * inter) {
  Interface * superInter;
  TypeArg * typeArg;
  NamedType * namedType;

  // printf ("=====  topoProcessOneInterface called on ");
  // printString (stdout, inter->id);
  // printf ("  (mark-in = %d)  =====\n", inter->mark);

  // This interface may have been done earlier in the topoProcessing...
  if (inter->mark == 2) return;

  // Check for circularity...
  if (inter->mark == 1) {
    error (inter, "Circularity in interface-extends hierarchy (this interface appears to extend itself, possibly indirectly)");
    return;
  }

  // Set mark to "processing"...
  inter->mark = 1;

  // Process all super-interfaces first...
  for (typeArg = inter->extends; typeArg; typeArg = typeArg->next) {
    namedType = (NamedType *) typeArg->type;
    // printf ("  Looking at namedType = ");
    // pretty (namedType);
    if ((namedType == NULL) || (namedType->op != NAMED_TYPE)) {
      programLogicError ("ParseInterface calls parseNamedType for extends clause");
    }
    superInter = (Interface *) inter->myHeader->packageMapping->find (namedType->id);
    if (superInter == NULL) {
      error (typeArg, "This interface could not be located");
    } else if (superInter->op != INTERFACE) {
      error (typeArg, "Interfaces may only extend other interfaces; this is not an interface");
    } else {
      namedType->myDef = superInter;
      topoProcessOneInterface (superInter);
    }
  }

  // Set mark to "done"...
  inter->mark = 2;

  // Put this one on the tempInterList...
  inter->tempNext = tempInterList;
  tempInterList = inter;

}



// topoProcessClasses (hdr)
//
// Run through all classes in this package.  Reorder the class
// list so that all classes come after their superclass.  Also set
// the myDef fields in the superclass and implements "namedType"s encountered.
//
void topoProcessClasses (Header * hdr) {
  ClassDef * cl, * next;

  // printf ("Here is the class list before topoProcessClasses:\n");
  // for (cl = hdr->classes; cl; cl = cl->next) {
  //   printf ("  %s\n", cl->id->chars);
  // }

  // Run through all classes and set their "myHeader" field.
  for (cl = hdr->classes; cl; cl = cl->next) {
    cl->myHeader = hdr;
  }

  // Run through all classes in this hdr and call topoProcessOneClass on each.
  tempClassList = NULL;
  for (cl = hdr->classes; cl; cl = cl->next) {
    topoProcessOneClass (cl);
  }

  // The tempClassList now contains all the classes, in order.  Go through it.
  // For each class, if it is in this header, put it on our "classes" list.
  // The order will be reversed.
  // printf ("\n\nGoing though tempClassList...\n");
  hdr->classes = NULL;
  for (cl = tempClassList; cl; cl = next) {
    // printf ("  Looking at %s\n", cl->id->chars);
    next = cl->tempNext;
    cl->tempNext = NULL;
    if (cl->myHeader == hdr) {
      // printf ("      Adding it to hdr->classes\n");
      cl->next = hdr->classes;
      hdr->classes = cl;
    }
  }

  // printf ("Here is the class list after topoProcessClasses:\n");
  // for (cl = hdr->classes; cl != NULL; cl = cl->next) {
  //   printf ("  %s\n", cl->id->chars);
  // }

}



// topoProcessOneClass (cl)
//
// Mark this class as "in process".  Then call self recursively to process
// the super-class.  Finally mark this class as "done" and add it to
// the growing list of classes.  Also check for cycles.
//
void topoProcessOneClass (ClassDef * cl) {
  ClassDef * superclass;
  Interface * superInter;
  TypeArg * typeArg;
  NamedType * namedType;

  // printf ("=====  topoProcessOneClass called on ");
  // printString (stdout, cl->id);
  // printf ("  (mark-in = %d)  =====\n", cl->mark);

  // This class may have been done earlier in the topoProcessing...
  if (cl->mark == 2) return;

  // Check for circularity...
  if (cl->mark == 1) {
    error (cl, "Circularity in superclass hierarchy (this class appears to be a subclass of itself, possibly indirectly)");
    return;
  }

  // Set mark to "processing"...
  cl->mark = 1;

  // Process the superclass first...
  // printf ("  Looking at superclass = ");
  // pretty (cl->superclass);
  if (cl->superclass != NULL) {
    superclass = (ClassDef *) cl->myHeader->packageMapping->find (cl->superclass->id);
    if (superclass == NULL) {
      error (cl->superclass, "This superclass could not be located");
    } else if (superclass->op != CLASS_DEF) {
      error (cl->superclass, "The thing listed in the superclass clause names something other than a class");
    } else {
      cl->superclass->myDef = superclass;
      topoProcessOneClass (superclass);
    }
  }

  // Run through all the interfaces this class implements and set "myDef"...
  for (typeArg = cl->implements; typeArg; typeArg = typeArg->next) {
    namedType = (NamedType *) typeArg->type;
    // printf ("  Looking at namedType = ");
    // pretty (namedType);
    if ((namedType == NULL) || (namedType->op != NAMED_TYPE)) {
      programLogicError ("ParseClass calls parseNamedType for implements clause");
    }
    superInter = (Interface *) cl->myHeader->packageMapping->find (namedType->id);
    if (superInter == NULL) {
      error (typeArg, "This interface could not be located");
    } else if (superInter->op != INTERFACE) {
      error (typeArg, "Classes may only implement interfaces; this is not an interface");
    } else {
      namedType->myDef = superInter;
    }
  }

  // Set mark to "done"...
  cl->mark = 2;

  // Put this one on the tempClassList...
  cl->tempNext = tempClassList;
  tempClassList = cl;

}



// bindTypeNames (node, typeMapping)
//
// This routine walks the entire Abstract Syntax Tree, visiting every node.
// It looks for every "NamedType" node and fills in its "myDef", which can
// point to either a ClassDef, Interface, TypeDef, or TypeParm.
//
// The "typeMapping" is a Mapping in which to look up type names.
// It will be NULL only for Headers.
//
void bindTypeNames (AstNode * node,
                    Mapping <String, AstNode> * typeMapping) {
  Header * header;
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
  CallExpr * callExpr;
  SendExpr * sendExpr;
  FieldAccess * fieldAccess;
  ArrayAccess * arrayAccess;
  Constructor * constructor;
  ClosureExpr * closureExpr;
  AsPtrToExpr * asPtrToExpr;
  AsIntegerExpr * asIntegerExpr;
  ArraySizeExpr * arraySizeExpr;
  IsInstanceOfExpr * isInstanceOfExpr;
  IsKindOfExpr * isKindOfExpr;
  SizeOfExpr * sizeOfExpr;
  Argument * arg;
  CountValue * countValue;
  FieldInit * fieldInit;
  AstNode * def;
  Mapping <String, AstNode> * newMap;
  Type * t1, * t2;
  // Mapping <TypeParm, Type> * testingMap;

  if (node == NULL) return;

  // printf ("Binding types in %s...\n", symbolName (node->op));

  switch (node->op) {

    case HEADER:
      header = (Header *) node;
      // printf ("  %s\n", header->packageName->chars);
      // printf ("  Point 0... header = %08x\n", header);
      if (header->packageMapping == NULL) return;
      bindTypeNames (header->uses, header->packageMapping);
      bindTypeNames (header->consts, header->packageMapping);
      bindTypeNames (header->errors, header->packageMapping);
      bindTypeNames (header->globals, header->packageMapping);
      bindTypeNames (header->typeDefs, header->packageMapping);
      bindTypeNames (header->functionProtos, header->packageMapping);
      bindTypeNames (header->functions, header->packageMapping);
      bindTypeNames (header->interfaces, header->packageMapping);
      bindTypeNames (header->classes, header->packageMapping);
      // bindTypeNames (header->next, typeMapping);
      return;

    case CODE:
      // code = (Code *) node;
      // We have added all this stuff to the header, so skip doing it a second time.
      // bindTypeNames (code->consts, typeMapping);
      // bindTypeNames (code->globals, typeMapping);
      // bindTypeNames (code->typeDefs, typeMapping);
      // bindTypeNames (code->functions, typeMapping);
      // bindTypeNames (code->interfaces, typeMapping);
      // bindTypeNames (code->classes, typeMapping);
      // bindTypeNames (code->behaviors, typeMapping);
      return;

    case USES:
      uses = (Uses *) node;
      // printf ("  %s\n", uses->id->chars);
      bindTypeNames (uses->renamings, typeMapping);
      bindTypeNames (uses->next, typeMapping);
      return;

    case RENAMING:
      renaming = (Renaming *) node;
      bindTypeNames (renaming->next, typeMapping);
      return;

    case INTERFACE:
      interface = (Interface *) node;
      // printf ("  %s\n", interface->id->chars);
      // Run through our type parms and add them to the mapping...
      if (interface->typeParms) {
        newMap = new Mapping <String, AstNode> (5, typeMapping);
        for (typeParm = interface->typeParms; typeParm; typeParm = typeParm->next) {
          def = newMap->find (typeParm->id);
          if (def) {
            error (typeParm, "The name for this type parameter is already in use"); 
            error2 (def, "Here is something else with the same name");
          } else {
            newMap->enter (typeParm->id, typeParm);
            if (typeParm->id->primitiveSymbol) {
              error (typeParm, "This item happens to have the same name as that of a built-in primitive function; please select a different name");
            }
          }
        }
      } else {
        newMap = typeMapping;
      }
      bindTypeNames (interface->typeParms, typeMapping);
      bindTypeNames (interface->extends, newMap);
      bindTypeNames (interface->methodProtos, newMap);
      bindTypeNames (interface->next, typeMapping);
      return;

    case CLASS_DEF:
      cl = (ClassDef *) node;
      // printf ("==========  Processing class %s  ==========\n", cl->id->chars);
      // printf ("  %s\n", cl->id->chars);
      // Run through our type parms and add them to the mapping...
      if (cl->typeParms) {
        newMap = new Mapping <String, AstNode> (5, typeMapping);

// testing:
//        testingMap = new Mapping <TypeParm, Type> (5, NULL);

        for (typeParm = cl->typeParms; typeParm; typeParm = typeParm->next) {
          def = newMap->find (typeParm->id);
          if (def) {
            error (typeParm, "The name for this type parameter is already in use"); 
            error2 (def, "Here is something else with the same name");
          } else {
            newMap->enter (typeParm->id, typeParm);
            if (typeParm->id->primitiveSymbol) {
              error (typeParm, "This item happens to have the same name as that of a built-in primitive function; please select a different name");
            }

// testing:
//          testingMap->enter (typeParm, typeParm->type);

          }
        }
      } else {
        newMap = typeMapping;
      }
      bindTypeNames (cl->typeParms, typeMapping);
      bindTypeNames (cl->implements, newMap);  // class "C[T:O] implements D[T]" is OK
      bindTypeNames (cl->superclass, newMap);  // class "C[T:O] superclass D[T]" is OK
      bindTypeNames (cl->fields, newMap);
      bindTypeNames (cl->methodProtos, newMap);
      bindTypeNames (cl->methods, newMap);
      bindTypeNames (cl->next, typeMapping);
      // printf ("==========  End class %s  ==========\n", cl->id->chars);

//---------- This is for testing only ------------------
//
//  To use this code, use something like this:
//           class C [T1: int, T2: double]
//             superclass Object
//             fields
//               x: ptr to T2
//           endClass
//
//      if (cl->id == lookupAndAdd ("C", ID)) {
//        if (cl->fields) {
//          t1 = cl->fields->type;
//          t2 = copyTypeWithSubst (t1, testingMap);
//          printf ("==========  Before  ==========\n");
//          printAst (6, t1);
//          printf ("==========  After  ===========\n");
//          printAst (6, t2);
//          printf ("==============================\n");
//        }
//      }
//--------------------------------------------------------

      return;

    case BEHAVIOR:
      behavior = (Behavior *) node;
      // printf ("  %s\n", behavior->id->chars);
      bindTypeNames (behavior->methods, typeMapping);
      bindTypeNames (behavior->next, typeMapping);
      return;

    case TYPE_DEF:
      typeDef = (TypeDef *) node;
      // printf ("  %s\n", typeDef->id->chars);
      bindTypeNames (typeDef->type, typeMapping);
      bindTypeNames (typeDef->next, typeMapping);
      return;

    case CONST_DECL:
      constDecl = (ConstDecl *) node;
      // printf ("  %s\n", constDecl->id->chars);
      bindTypeNames (constDecl->expr, typeMapping);
      bindTypeNames (constDecl->next, typeMapping);
      return;

    case ERROR_DECL:
      errorDecl = (ErrorDecl *) node;
      // printf ("  %s\n", errorDecl->id->chars);
      bindTypeNames (errorDecl->parmList, typeMapping);
      bindTypeNames (errorDecl->next, typeMapping);
      return;

    case FUNCTION_PROTO:
      functionProto = (FunctionProto *) node;
      // printf ("  %s\n", functionProto->id->chars);
      bindTypeNames (functionProto->parmList, typeMapping);
      bindTypeNames (functionProto->retType, typeMapping);
      bindTypeNames (functionProto->next, typeMapping);
      return;

    case FUNCTION:
      fun = (Function *) node;
      // if (fun->id) {
      //   printf ("    Within function %s\n", fun->id->chars);
      // } else {
      //   printf ("    Within closure\n");
      // }
      bindTypeNames (fun->parmList, typeMapping);
      bindTypeNames (fun->retType, typeMapping);
      bindTypeNames (fun->locals, typeMapping);
      bindTypeNames (fun->stmts, typeMapping);
      bindTypeNames (fun->next, typeMapping);
      return;

    case METHOD_PROTO:
      methodProto = (MethodProto *) node;
      // printf ("  %s\n", methodProto->selector->chars);
      bindTypeNames (methodProto->parmList, typeMapping);
      bindTypeNames (methodProto->retType, typeMapping);
      bindTypeNames (methodProto->next, typeMapping);
      return;

    case METHOD:
      meth = (Method *) node;
      // printf ("  %s\n", meth->selector->chars);
      bindTypeNames (meth->parmList, typeMapping);
      bindTypeNames (meth->retType, typeMapping);
      bindTypeNames (meth->locals, typeMapping);
      bindTypeNames (meth->stmts, typeMapping);
      bindTypeNames (meth->next, typeMapping);
      return;

    case TYPE_PARM:
      typeParm = (TypeParm *) node;
      bindTypeNames (typeParm->type, typeMapping);
      bindTypeNames (typeParm->next, typeMapping);
      return;

    case TYPE_ARG:
      typeArg = (TypeArg *) node;
      bindTypeNames (typeArg->type, typeMapping);
      bindTypeNames (typeArg->next, typeMapping);
      return;

    case CHAR_TYPE:
      return;

    case INT_TYPE:
      return;

    case DOUBLE_TYPE:
      return;

    case BOOL_TYPE:
      return;

    case VOID_TYPE:
      return;

    case TYPE_OF_NULL_TYPE:
      return;

    case ANY_TYPE:
      return;

    case PTR_TYPE:
      pType = (PtrType *) node;
      bindTypeNames (pType->baseType, typeMapping);
      return;

    case ARRAY_TYPE:
      aType = (ArrayType *) node;
      bindTypeNames (aType->sizeExpr, typeMapping);
      bindTypeNames (aType->baseType, typeMapping);
      return;

    case RECORD_TYPE:
      rType = (RecordType *) node;
      bindTypeNames (rType->fields, typeMapping);
      return;

    case FUNCTION_TYPE:
      fType = (FunctionType *) node;
      bindTypeNames (fType->parmTypes, typeMapping);
      bindTypeNames (fType->retType, typeMapping);
      return;

    case NAMED_TYPE:
      nType = (NamedType *) node;
      // printf ("  %s\n", nType->id->chars);
      def = typeMapping->find (nType->id);
      if (def == NULL) {
        error (nType, "This type name is undefined");
      } else {
        if ((def->op == CLASS_DEF) ||
            (def->op == INTERFACE) ||
            (def->op == TYPE_DEF) ||
            (def->op == TYPE_PARM)) {
          if (nType->myDef) {
            if (nType->myDef != def) {
              programLogicError ("myDef already filled in with something different");
            }
          } else {
            // We'll check for this error in checkTypes, so don't do it here...
            // if ((def->op == TYPE_DEF) ||
            //     (def->op == TYPE_PARM)) {
            //   if (nType->typeArgs != NULL) {
            //     error (nType, "Parameters are not expected after this type name");
            //   }
            // }
            nType->myDef = def;
          }
        } else {
          error (nType, "This type names something that is not a class, interface, typeDef, or type parameter");
          error2 (def, "Here is the item being named");
        }
      }
      bindTypeNames (nType->typeArgs, typeMapping);
      return;

    case IF_STMT:
      ifStmt = (IfStmt *) node;
      bindTypeNames (ifStmt->expr, typeMapping);
      bindTypeNames (ifStmt->thenStmts, typeMapping);
      bindTypeNames (ifStmt->elseStmts, typeMapping);
      bindTypeNames (ifStmt->next, typeMapping);
      return;

    case ASSIGN_STMT:
      assignStmt = (AssignStmt *) node;
      bindTypeNames (assignStmt->lvalue, typeMapping);
      bindTypeNames (assignStmt->expr, typeMapping);
      bindTypeNames (assignStmt->next, typeMapping);
      return;

    case CALL_STMT:
      callStmt = (CallStmt *) node;
      bindTypeNames (callStmt->expr, typeMapping);
      bindTypeNames (callStmt->next, typeMapping);
      return;

    case SEND_STMT:
      sendStmt = (SendStmt *) node;
      bindTypeNames (sendStmt->expr, typeMapping);
      bindTypeNames (sendStmt->next, typeMapping);
      return;

    case WHILE_STMT:
      whileStmt = (WhileStmt *) node;
      bindTypeNames (whileStmt->expr, typeMapping);
      bindTypeNames (whileStmt->stmts, typeMapping);
      bindTypeNames (whileStmt->next, typeMapping);
      return;

    case DO_STMT:
      doStmt = (DoStmt *) node;
      bindTypeNames (doStmt->stmts, typeMapping);
      bindTypeNames (doStmt->expr, typeMapping);
      bindTypeNames (doStmt->next, typeMapping);
      return;

    case BREAK_STMT:
      breakStmt = (BreakStmt *) node;
      bindTypeNames (breakStmt->next, typeMapping);
      return;

    case CONTINUE_STMT:
      continueStmt = (ContinueStmt *) node;
      bindTypeNames (continueStmt->next, typeMapping);
      return;

    case RETURN_STMT:
      returnStmt = (ReturnStmt *) node;
      bindTypeNames (returnStmt->expr, typeMapping);
      bindTypeNames (returnStmt->next, typeMapping);
      return;

    case FOR_STMT:
      forStmt = (ForStmt *) node;
      bindTypeNames (forStmt->lvalue, typeMapping);
      bindTypeNames (forStmt->expr1, typeMapping);
      bindTypeNames (forStmt->expr2, typeMapping);
      bindTypeNames (forStmt->expr3, typeMapping);
      bindTypeNames (forStmt->stmts, typeMapping);
      bindTypeNames (forStmt->next, typeMapping);
      return;

    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) node;
      bindTypeNames (switchStmt->expr, typeMapping);
      bindTypeNames (switchStmt->caseList, typeMapping);
      bindTypeNames (switchStmt->defaultStmts, typeMapping);
      bindTypeNames (switchStmt->next, typeMapping);
      return;

    case TRY_STMT:
      tryStmt = (TryStmt *) node;
      bindTypeNames (tryStmt->stmts, typeMapping);
      bindTypeNames (tryStmt->catchList, typeMapping);
      bindTypeNames (tryStmt->next, typeMapping);
      return;

    case THROW_STMT:
      throwStmt = (ThrowStmt *) node;
      errorDecl = currentHeader->errorMapping->find (throwStmt->id);
      if (errorDecl == NULL) {
        error (throwStmt, "There is no declaration for the error named in this TRY statement");
      } else if (errorDecl->op != ERROR_DECL) {
        programLogicError ("We only store ErrorDecls in this mapping");
      }
      throwStmt->myDef = errorDecl;
      bindTypeNames (throwStmt->argList, typeMapping);
      bindTypeNames (throwStmt->next, typeMapping);
      return;

    case FREE_STMT:
      freeStmt = (FreeStmt *) node;
      bindTypeNames (freeStmt->expr, typeMapping);
      bindTypeNames (freeStmt->next, typeMapping);
      return;

    case DEBUG_STMT:
      debugStmt = (DebugStmt *) node;
      bindTypeNames (debugStmt->next, typeMapping);
      return;

    case CASE:
      cas = (Case *) node;
      bindTypeNames (cas->expr, typeMapping);
      bindTypeNames (cas->stmts, typeMapping);
      bindTypeNames (cas->next, typeMapping);
      return;

    case CATCH:
      cat = (Catch *) node;
      errorDecl = currentHeader->errorMapping->find (cat->id);
      if (errorDecl == NULL) {
        error (cat, "There is no declaration for the error named in this CATCH clause");
      } else if (errorDecl->op != ERROR_DECL) {
        programLogicError ("We only store ErrorDecls in this mapping");
      }
      cat->myDef = errorDecl;
      bindTypeNames (cat->parmList, typeMapping);
      bindTypeNames (cat->stmts, typeMapping);
      bindTypeNames (cat->next, typeMapping);
      return;

    case GLOBAL:
      global = (Global *) node;
      bindTypeNames (global->type, typeMapping);
      bindTypeNames (global->initExpr, typeMapping);
      bindTypeNames (global->next, typeMapping);
      return;

    case LOCAL:
      local = (Local *) node;
      bindTypeNames (local->type, typeMapping);
      bindTypeNames (local->initExpr, typeMapping);
      bindTypeNames (local->next, typeMapping);
      return;

    case PARAMETER:
      parm = (Parameter *) node;
      bindTypeNames (parm->type, typeMapping);
      bindTypeNames (parm->next, typeMapping);
      return;

    case CLASS_FIELD:
      classField = (ClassField *) node;
      bindTypeNames (classField->type, typeMapping);
      bindTypeNames (classField->next, typeMapping);
      return;

    case RECORD_FIELD:
      recordField = (RecordField *) node;
      bindTypeNames (recordField->type, typeMapping);
      bindTypeNames (recordField->next, typeMapping);
      return;

    case INT_CONST:
      return;

    case DOUBLE_CONST:
      return;

    case CHAR_CONST:
      return;

    case STRING_CONST:
      return;

    case BOOL_CONST:
      return;

    case NULL_CONST:
      return;

    case CALL_EXPR:
      callExpr = (CallExpr *) node;
      bindTypeNames (callExpr->argList, typeMapping);
      return;

    case SEND_EXPR:
      sendExpr = (SendExpr *) node;
      bindTypeNames (sendExpr->receiver, typeMapping);
      bindTypeNames (sendExpr->argList, typeMapping);
      return;

    case SELF_EXPR:
      return;

    case SUPER_EXPR:
      return;

    case FIELD_ACCESS:
      fieldAccess = (FieldAccess *) node;
      bindTypeNames (fieldAccess->expr, typeMapping);
      return;

    case ARRAY_ACCESS:
      arrayAccess = (ArrayAccess *) node;
      bindTypeNames (arrayAccess->arrayExpr, typeMapping);
      bindTypeNames (arrayAccess->indexExpr, typeMapping);
      return;

    case CONSTRUCTOR:
      constructor = (Constructor *) node;
      bindTypeNames (constructor->type, typeMapping);
      bindTypeNames (constructor->countValueList, typeMapping);
      bindTypeNames (constructor->fieldInits, typeMapping);
      return;

    case CLOSURE_EXPR:
      closureExpr = (ClosureExpr *) node;
      bindTypeNames (closureExpr->function, typeMapping);
      // Add this closure to the list of closures...
      if (mainHeader == NULL) {
        programLogicError ("mainHeader should not be NULL");
      }
      if (closureExpr->function == NULL) {
        programLogicError ("We should have a function here");
      }
      if (closureExpr->function->next) {
        programLogicError ("closureExpr->function->next should be NULL");
      }
      closureExpr->function->next = mainHeader->closures;
      mainHeader->closures = closureExpr->function;
      return;

    case VARIABLE_EXPR:
      return;

    case AS_PTR_TO_EXPR:
      asPtrToExpr = (AsPtrToExpr *) node;
      bindTypeNames (asPtrToExpr->expr, typeMapping);
      bindTypeNames (asPtrToExpr->type, typeMapping);
      return;

    case AS_INTEGER_EXPR:
      asIntegerExpr = (AsIntegerExpr *) node;
      bindTypeNames (asIntegerExpr->expr, typeMapping);
      return;

    case ARRAY_SIZE_EXPR:
      arraySizeExpr = (ArraySizeExpr *) node;
      bindTypeNames (arraySizeExpr->expr, typeMapping);
      return;

    case IS_INSTANCE_OF_EXPR:
      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      bindTypeNames (isInstanceOfExpr->expr, typeMapping);
      bindTypeNames (isInstanceOfExpr->type, typeMapping);
      return;

    case IS_KIND_OF_EXPR:
      isKindOfExpr = (IsKindOfExpr *) node;
      bindTypeNames (isKindOfExpr->expr, typeMapping);
      bindTypeNames (isKindOfExpr->type, typeMapping);
      return;

    case SIZE_OF_EXPR:
      sizeOfExpr = (SizeOfExpr *) node;
      bindTypeNames (sizeOfExpr->type, typeMapping);
      return;

    case ARGUMENT:
      arg = (Argument *) node;
      bindTypeNames (arg->expr, typeMapping);
      bindTypeNames (arg->next, typeMapping);
      return;

    case COUNT_VALUE:
      countValue = (CountValue *) node;
      bindTypeNames (countValue->count, typeMapping);
      bindTypeNames (countValue->value, typeMapping);
      bindTypeNames (countValue->next, typeMapping);
      return;

    case FIELD_INIT:
      fieldInit = (FieldInit *) node;
      bindTypeNames (fieldInit->expr, typeMapping);
      bindTypeNames (fieldInit->next, typeMapping);
      return;

    default:
      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in bindTypeNames");
  }
}



// checkTypeDefCircularity (hdr)
//
// This routine checks that for the following sort of error:
//     type S = T
//          T = U
//          U = S
// Also, in cases like this:
//     type T = S
//          S = record...
// it redirects the typedef for T to point straight to the underlying type.
//
// Note that it does nothing for typedefs like this:
//     type T = S[U,V]
//
void checkTypeDefCircularity (Header * hdr) {
  TypeDef * typeDef;
  // Run through all typedefs...
  for (typeDef = hdr->typeDefs; typeDef; typeDef = typeDef->next) {
    typeDef->type = findCoreType (typeDef->type);
  }
}



// findCoreType (typ)
//
// This routine is passed a type.  If there are indirections (due to aliasing)
// it returns the underlying type.  If there are problems due to cirularity,
// it returns VoidType.
//
Type * findCoreType (Type * typ) {
  NamedType * nType;
  Type * t;
  TypeDef * typeDef;
  AstNode * def;

  if (typ == NULL) {
    programLogicError ("In findCoreType, 'typ' should never be NULL");
  }
  switch (typ->op) {
    case CHAR_TYPE:
    case INT_TYPE:
    case DOUBLE_TYPE:
    case BOOL_TYPE:
    case VOID_TYPE:
    case TYPE_OF_NULL_TYPE:
    case ANY_TYPE:
    case PTR_TYPE:
    case ARRAY_TYPE:
    case RECORD_TYPE:
    case FUNCTION_TYPE:
      return typ;
    case NAMED_TYPE:
      nType = (NamedType *) typ;
      if (nType->typeArgs) {
        return typ;
      }
      def = nType->myDef;
      if (def != NULL) {
        if (def->op == TYPE_DEF) {
          typeDef = (TypeDef *) def;
          if (typeDef->mark != 0) {
            error (typeDef, "This type definition is involved in a circular definition");
          } else {
            typeDef->mark = 1;
            typeDef->type = findCoreType (typeDef->type);
            typeDef->mark = 0;
            return typeDef->type;
          }
        } else {  // it is CLASS, INTERFACE, TYPE_PARM
          return typ;
        }
      }
      t = new VoidType ();
      t->positionAt (typ);
      return t;
  }
}



// copyTypeWithSubst (type, subst)
//
// This routine is passed a Type and a substitution.  A substitution is
// a mapping from type parameters names to types.  This routine constructs
// and returns a new type, after performing the substitution.
//
// This routine tries to share as much of the source structure as possible,
// so pieces of types may be shared.  Types are DAGS, not trees.
//
// Note: In the case of ArrayTypes which get copied, we will always share
// the 'sizeExpr'.  This expression may contain type parameters, as in:
//      array [4 * sizeOf T] of int
// These type parameters will not get substituted.  The motivation for not
// coying is that copied types are only used in type checking, and never
// in code generation.  The sizeExpr is never used in type checking.
//
// To test this code, consult the disabled testing code in the routine
// "bindTypeNames()", in the CLASS_DEF case.
//
// Tested.
//
Type * copyTypeWithSubst (Type * type, Mapping <TypeParm, Type> * subst) {
  PtrType * pType, * newPType;
  ArrayType * aType, *newAType;
  NamedType * nType, * newNType;
  RecordType * rType, * newRType;
  FunctionType * fType, * newFType;
  RecordField * newFieldList;
  Type * newBase, * newType, * newRetType;
  TypeArg * newArgList;

  if (type == NULL) return NULL;

  // printf ("Entering copyTypeWithSubst.  type = ");
  // pretty (type);

  if (subst == NULL) return type;

  if (type->op == NAMED_TYPE) {
    nType = (NamedType * ) type;
    if (nType->myDef == NULL) return type;    // If prev. errors...
    if ((nType->myDef)->op == TYPE_PARM) {
      newType = subst->find ((TypeParm *) (nType->myDef));
      if (newType == NULL) {
        // printf ("Returning.  This is a TYPE_PARM without a value.  Returning ");
        // pretty (type); 
        return type;
      }
      // printf ("Returning.  This is a TYPE_PARM with a value.  Returning ");
      // pretty (newType); 
      return newType;
    } else {
      // Build a new arglist, performing the substitution.
      newArgList = copyArgListWithSubst (nType->typeArgs, subst);

      // If the new type will look the same, then return the old type.
      if (newArgList == nType->typeArgs) {
        // printf ("Returning.  The type is unchanged... ");
        // pretty (type);
        return type;
      }

      // Build a new NamedType object and return it.
      // printf ("Returning.  Building new NamedType for... ");
      // pretty (type);
      newNType = new NamedType ();
      newNType->positionAt (nType);
      newNType->id = nType->id;
      newNType->myDef = nType->myDef;
      newNType->typeArgs = newArgList;
      // printf ("            Here it is the new thing...   ");
      // pretty (newNType);
      return newNType;
    }
  } else if (type->op == RECORD_TYPE) {
    rType = (RecordType * ) type;
    // Build a new Fieldlist, performing the substitution.
    newFieldList = copyRecordFieldsWithSubst (rType->fields, subst);

    // If the new type will look the same, then return the old type.
    if (newFieldList == rType->fields) {
      // printf ("Returning.  The type is unchanged... ");
      // pretty (type);
      return type;
    }

    // Build a new RecordType object and return it.
    // printf ("Returning.  Building new RecordType for... ");
    // pretty (type);
    newRType = new RecordType ();
    newRType->positionAt (rType);
    newRType->fields = newFieldList;
    // printf ("            Here it is the new thing...   ");
    // pretty (newRType);
    return newRType;
  } else if (type->op == PTR_TYPE) {
    pType = (PtrType *) type;
    newBase = copyTypeWithSubst (pType->baseType, subst);
    // If the new type will look the same, then return the old type.
    if (newBase == pType->baseType) {
      // printf ("Returning.  The type is unchanged... ");
      // pretty (type);
      return type;
    }
    // printf ("Returning.  Building new PtrType for... ");
    // pretty (type);
    newPType = new PtrType ();
    newPType->positionAt (pType);
    newPType->baseType = newBase;
    // printf ("            Here it is the new thing...   ");
    // pretty (newPType);
    return newPType;
  } else if (type->op == ARRAY_TYPE) {
    aType = (ArrayType *) type;
    newBase = copyTypeWithSubst (aType->baseType, subst);
    // If the new type will look the same, then return the old type.
    if (newBase == aType->baseType) {
      // printf ("Returning.  The type is unchanged... ");
      // pretty (type);
      return type;
    }
    // Build a new ArrayType object and return it.
    // printf ("Returning.  Building new ArrayType for... ");
    // pretty (type);
    newAType = new ArrayType ();
    newAType->positionAt (aType);
    // We will not copy the sizeExpr, although it could (in theory) contain
    // instances of type variables....  Perhaps it should be copied, too...
    newAType->sizeExpr = aType->sizeExpr;
    // These two lines added to fix bug...
    newAType->sizeOfElements = aType->sizeOfElements;
    newAType->sizeInBytes = aType->sizeInBytes;
    newAType->baseType = newBase;
    // printf ("            Here it is the new thing...   ");
    // pretty (newAType);
    return newAType;
  } else if (type->op == FUNCTION_TYPE) {
    fType = (FunctionType * ) type;
    // Build a new Arglist, performing the substitution.
    newArgList = copyArgListWithSubst (fType->parmTypes, subst);
    // Build a new Return type, performing the substitution.
    newRetType = copyTypeWithSubst (fType->retType, subst);

    // If the new type will look the same, then return the old type.
    if ((newArgList == fType->parmTypes) && (newRetType == fType->retType)) {
      // printf ("Returning.  The type is unchanged... ");
      // pretty (type);
      return type;
    }

    // Build a new FunctionType object and return it.
    // printf ("Returning.  Building new FunctionType for... ");
    // pretty (type);
    newFType = new FunctionType ();
    newFType->positionAt (fType);
    newFType->parmTypes = newArgList;
    newFType->retType = newRetType;
    // printf ("            Here it is the new thing...   ");
    // pretty (newFType);
    return newFType;
  } else {   // It must be CHAR_TYPE, INT_TYPE, DOUBLE_TYPE, BOOL_TYPE,
             //            VOID_TYPE, TYPE_OF_NULL, or ANY_TYPE...
    // printf ("Returning.  Default case returning: ");
    // pretty (type);
    return type;
  }
}



// copyArgListWithSubst (argList, subst)
//
// Build a new arg list by copying the old list, performing the
// substitution.  Try to share parts of the old list, if there is
// no change.
//
// Tested.
//
TypeArg * copyArgListWithSubst (TypeArg * argList,
                                Mapping <TypeParm, Type> * subst) {
  TypeArg * newTail, * newTypeArg;
  Type * newType;

  if (argList == NULL) return NULL;

  // printf ("    --- copyArgListWithSubst entered with argList = ");
  // pretty (argList);

  newTail = copyArgListWithSubst (argList->next, subst);
  newType = copyTypeWithSubst (argList->type, subst);

  // If the new TypeArg would be the same, just return the old TypeArg object. 
  if ((newType == argList->type) && (newTail == argList->next)) {
    // printf ("    --- Returning without creating anything new = ");
    // pretty (argList);
    return argList;
  }

  // Else, build a new TypeArg object.
  newTypeArg = new TypeArg ();
  newTypeArg->positionAt (argList);
  newTypeArg->type = newType;
  newTypeArg->next = newTail;

  // printf ("    --- Returning; Building newTypeArg = ");
  // pretty (newTypeArg);

  return newTypeArg;
}



// copyRecordFieldsWithSubst (argList, subst)
//
// Build a new field list by copying the old list, performing the
// substitution.  Try to share parts of the old list, if there is
// no change.
//
// Tested.
//
RecordField * copyRecordFieldsWithSubst (RecordField * fieldList,
                                         Mapping <TypeParm, Type> * subst) {
  RecordField * newTail, * newRecordField;
  Type * newType;

  if (fieldList == NULL) return NULL;

  // printf ("    --- copyRecordFieldsWithSubst entered with fieldList = ");
  // pretty (fieldList);

  newTail = copyRecordFieldsWithSubst ((RecordField *) (fieldList->next), subst);
  newType = copyTypeWithSubst (fieldList->type, subst);

  // If the new RecordField would be the same, just return the old RecordField object. 
  if ((newType == fieldList->type) && (newTail == fieldList->next)) {
    // printf ("    --- Returning without creating anything new = ");
    // pretty (fieldList);
    return fieldList;
  }

  // Else, build a new RecordField object.
  newRecordField = new RecordField ();
  newRecordField->positionAt (fieldList);
  newRecordField->id = fieldList->id;
  newRecordField->type = newType;
  newRecordField->next = newTail;

  // printf ("    --- Returning; Building newRecordField = ");
  // pretty (newRecordField);

  return newRecordField;
}



// inheritFields (hdr)
//
// This routine builds the "classMapping" for each ClassDef.  It also re-builds the
// list of fields.  First, copies of the inherited fields are created.  Then the
// inherited fields are placed at the beginning of the list "fields".  Then the
// new fields of this class are added at the end.
//
// When inherited ClassFields are copied, their types are copied with a possible
// substitution.  Here is an example showing why this might be necessary:
//
//      class Super [T:anyType]
//        fields
//          f: ptr to T
//      ...
//      class Sub [S:Number]
//        superclass Super [S]
//
// In "Sub", the field will become:
//          f : ptr to S
//
// This routine also fills in the "superclassDef" field in each ClassDef.
//
void inheritFields (Header * hdr) {
  ClassDef * cl;
  NamedType * nType;
  TypeArg * typeArg;
  ClassDef * super;
  TypeParm * typeParm;
  ClassField * newFields, * newFieldsLast, * f, * oldField;
  AstNode * def;

  for (cl = hdr->classes; cl; cl = cl->next) {

    // Create the classMapping...
    cl->classMapping = new Mapping <String, AstNode> (10, hdr->packageMapping);

    // Create the superclassMapping...
    super = NULL;
    if (cl->superclass) {
      nType = (NamedType *) cl->superclass;
      typeArg = nType->typeArgs;
      super = (ClassDef *) (nType->myDef);
      if (super) {   // else previous errors...
        if (super->op != CLASS_DEF) {   // if previous errors...
          super = NULL;
        } else {
          typeParm = super->typeParms;
          while (1) {
            if ((typeParm == NULL) && (typeArg == NULL)) break;
            if ((typeParm == NULL) || (typeArg == NULL)) {
              error (cl->superclass, "The number of type arguments here does not match the number of type parameters in the superclass definition");
              break;
            }
            if (cl->superclassMapping == NULL) {
              cl->superclassMapping = new Mapping <TypeParm, Type> (3, NULL);
            }
            cl->superclassMapping->enter (typeParm, typeArg->type);
            typeParm = typeParm->next;
            typeArg = typeArg->next;
          }
        }
      }
    }
    cl->superclassDef = super;

    // Build the new field list.
    newFields = NULL;
    newFieldsLast = NULL;

    // Run through the superclass's fields.  For each, copy the field
    // and place it onto the newField list.
    if (super) {
      for (oldField = super->fields; oldField; oldField = (ClassField *) oldField->next) {
        f = new ClassField ();
        f->positionAt (oldField);
        f->id = oldField->id;
        f->type = copyTypeWithSubst (oldField->type, cl-> superclassMapping);
        if (newFields) {
          newFieldsLast->next = f;
        } else {
          newFields = f;
        }
        newFieldsLast = f;
        cl->classMapping->enter (f->id, f);
      }
    }

    // Add all fields new to this class to the classMapping...
    for (f = cl->fields; f; f = (ClassField * ) f->next) {
      def = cl->classMapping->findInTopScope (f->id);
      if (def) {
        error (f, "A field with this name already exists in this class");
        error2 (def, "Here is the other field");
      } else {
        cl->classMapping->enter (f->id, f);
        if (f->id->primitiveSymbol) {
          error (f, "This field happens to have the same name as that of a built-in primitive function; please select a different name");
        }
      }
    }

    // Put the fields in this class at the end of the new field list...
    if (newFields) {
      newFieldsLast->next = cl->fields;
    } else {
      newFields = cl->fields;
    }

    // Make this field list the class's field list...
    cl->fields = newFields;

  }
}



// inheritMethodProtos (hdr)
//
// For each ClassDef, this routine builds the "selectorMapping", which maps selectors
// (like "foo" or "at:put:") to MethodProtos.  First, the MethodProtos of this class
// are placed in the mapping.  Then, we run through all the MethodProtos in the
// superclass.  For each, we make a copy and add that to the mapping.
// We also add the copy to the list "methodProtos" in this class.
//
// When inherited MethodProtos are copied, their types are copied with a possible
// substitution.  Here is an example showing why this might be necessary:
//
//      class Super [T:anyType]
//        methods
//          foo (x:T) returns ptr to T
//      ...
//      class Sub [S:Number]
//        superclass Super [S]
//      ...
// In "Sub", the MethodProto will become:
//          foo (x:S) returns ptr to S
//
// When methods are overridden, then the copy of the MethodProto will have the
// selector prefixed with "_super_".
//
// This routine checks that there are no duplicate Methods or duplicate MethodProtos.
// It checks that for every MethodProto there is a Method, and it checks that for every
// Method there is a MethodProto.  (Only if this class is in the MainHeader.)
//
// This routine makes each Method point back to the MethodProto in its class.  It also
// makes each MethodProto point to the Method that will implement it.  Whenever Methods
// are inherited, the newly created MethodProto in the subclass will be made to point
// to the Method in the superclass.
//
// It is possible that the class will have two Methods with the same operator, if one
// is infix and the other is prefix.  For example:
//
//      class C
//        methods
//          prefix + () returns int
//          infix + (x: int) returns int
//
// In the case of a prefix operator, we will enter it into the selector mapping
// using a key made by adding "_prefix_" to it.  In this example, the mapping key will
// be "_prefix_+".  An infix method will be entered using the selector ("+") as is.
// The selector in the method, however, will not be changed.
//
// When a prefix operator is inherited and overridden in the subclass, the newly
// created MethodProto will have a selector like "_super__prefix_+".  It will be
// entered in the selector mapping with the same selector.
//
void inheritMethodProtos (Header * hdr) {
  ClassDef * cl;
  MethodProto * proto, * newProto, * nextProto;
  AstNode * def, * def2;
  Method * meth;
  String * newSelector, * strKey, * strVal;

  TypeArg * typeArg;

  for (cl = hdr->classes; cl; cl = cl->next) {

    // printf ("    Processing class %s\n", cl->id->chars);

    // Create the "localMethodMapping".  This is a mapping from selectors
    // to the Methods that are actually in this class; it does not include
    // any inherited methods.  The "_prefix_" string will be added to the
    // selector for the keys of prefix methods...
    cl->localMethodMapping = new Mapping <String, Method> (10, NULL);

    // Run thru the methods and add them to "localMethodMapping".
    // Check to make sure each method has a different selctor...
    for (meth=cl->methods; meth; meth = meth->next) {
      newSelector = meth->selector;
      // printf ("newSelector = %s\n", newSelector->chars);
      def = cl->localMethodMapping->findInTopScope (newSelector);
      if (def) {
        error (meth, "Another method with the same selector already exists in this class");
        error2 (def, "Here is the other method");
      } else {
        cl->localMethodMapping->enter (newSelector, meth);
      }
    }

    // Create the "selectorMapping"...
    cl->selectorMapping = new Mapping <String, MethodProto> (10, NULL);

    // Add all MethodProtos in this class to the "selectorMapping".
    // Make sure all MethodProtos have different selectors.
    // For classes in the mainHeader, also make sure that there is
    // a Method for each MethodProto...
    for (proto = cl->methodProtos; proto; proto = proto->next) {
      newSelector = proto->selector;
      def = cl->selectorMapping->findInTopScope (newSelector);
      if (def) {
        error (proto, "A method prototype with this selector already exists in this class");
        error2 (def, "Here is the other method prototype");
      } else {
        cl->selectorMapping->enter (newSelector, proto);
        if (hdr == mainHeader) {
          // Make sure there is a method for this MethodProto...
          meth = (Method *) cl->localMethodMapping->find (newSelector);
          if (meth == NULL) {
            error (proto, "There is no method for this method prototype");
          } else {
            // Link the method and its prototype
            meth->myMethodProto = proto;
            proto->myMethod = meth;
          }
        }
      }
    }

    // Run through the list of Methods in this class and make sure
    // there is a MethodProto for each...
    for (meth=cl->methods; meth; meth = meth->next) {
      newSelector = meth->selector;
      def = cl->selectorMapping->findInTopScope (newSelector);
      if (def == NULL) {
        error (meth, "There is no method prototype for this method");
      }
    }

    // Run through the superclass's MethodProtos and make copies of them...
    if (cl->superclassDef) {

      // printf ("  Processing superclass %s\n", cl->superclassDef->id->chars);

      for (proto = cl->superclassDef->methodProtos; proto; proto=proto->next) {

        //    printf ("    Looking at = %s\n", proto->selector->chars);
        newSelector = proto->selector;

        // See if this methodProto is being overridden by a method in this class,
        // and keep adding "_super_" until not...
        while (cl->selectorMapping->findInTopScope (newSelector)) {
          newSelector = addSuperTo (newSelector);
          //    printf ("      Overriding detected, newSelector = %s\n",
          //            newSelector->chars);
        }

        // Create a new MethodProto and add it to this class...
        //    printf ("      Creating new MethodProto...\n");
        newProto = new MethodProto ();
        newProto->positionAt (proto);
        newProto->kind = proto->kind;
        newProto->selector = newSelector;
        newProto->parmList =
              copyParmListWithSubst (proto->parmList, cl->superclassMapping);
        newProto->retType =
              copyTypeWithSubst (proto->retType, cl->superclassMapping);
        cl->selectorMapping->enter (newSelector, newProto);
        newProto->myMethod = proto->myMethod;
        newProto->next = cl->methodProtos;
        cl->methodProtos = newProto;
      }
    }

  }
}



// addSuperTo (str)
//
// This routine creates a new string by prepending the characters "_super_".
// It returns the new string.
//
String * addSuperTo (String * str) {
  char * newChars;
  newChars = appendStrings ("_super_", str->chars, "");
  return lookupAndAdd (newChars, str->type);
}



// inheritMessages (hdr)
//
// This routine builds the "selectorMapping" for each Interface by placing the
// local MethodProtos into it.  This is done to check for duplicate messages in this
// interface.  Then, we go through each super-interface and make a copy of each
// inherited method.  We do not place these in the selectorMapping yet since,
// because of the semantics of message inheritance, there may be duplicates.
// Instead, we accumulate them into a list for use later.
//
// When inherited MethodProtos are copied, their types are copied with a possible
// substitution.  Here is an example showing why this might be necessary:
//
//      interface Super [T:anyType]
//        messages
//          foo (x:T) returns ptr to T
//      ...
//      interface Sub [S:Number]
//        extends Super [S]
//      ...
//
// In "Sub", the MethodProto will become:
//          foo (x:S) returns ptr to S
//
void inheritMessages (Header * hdr) {
  Interface * inter, * superInter;
  MethodProto * proto, * newProto;
  AstNode * def;
  TypeArg * typeArg, * superTypeArg;
  TypeParm * typeParm;
  Mapping <TypeParm, Type> * subst;
  NamedType * nType;

  // Run through all interfaces...
  for (inter = hdr->interfaces; inter; inter = inter->next) {

    // Create the "selectorMapping"...
    inter->selectorMapping = new Mapping <String, MethodProto> (10, NULL);

    // Add all methodProtos in this interface to the "selectorMapping"...
    for (proto = inter->methodProtos; proto; proto = proto->next) {
      def = inter->selectorMapping->findInTopScope (proto->selector);
      if (def) {
        error (proto, "A message with this selector already exists in this interface");
        error2 (def, "Here is the other message");
      } else {
        inter->selectorMapping->enter (proto->selector, proto);
      }
    }

    // Run through each of the interfaces that we extend...
    for (superTypeArg = inter->extends; superTypeArg; superTypeArg = superTypeArg->next) {
      nType = (NamedType *) superTypeArg->type;
      superInter = (Interface *) (nType->myDef);
      // If no previous errors...
      if (superInter) {
        if (superInter->op == INTERFACE) {

          // Build a substitution for the TypeParms...
          subst = NULL;
          typeParm = superInter->typeParms;
          typeArg = nType->typeArgs;
          while (1) {
            if ((typeParm == NULL) && (typeArg == NULL)) break;
            if ((typeParm == NULL) || (typeArg == NULL)) {
              error (superTypeArg, "The number of type arguments here does not match the number of type parameters in the interface definition");
              break;
            }
            if (subst == NULL) {
              subst = new Mapping <TypeParm, Type> (3, NULL);
            }
            subst->enter (typeParm, typeArg->type);
            typeParm = typeParm->next;
            typeArg = typeArg->next;
          }

          // printf ("subst = \n");
          // if (subst) {
          //   subst->print (6);
          // } else {
          //   printf ("NULL\n");
          // }

          // Run through the superInterfaces's methodProtos and make copies of them...
          for (proto = superInter->methodProtos; proto; proto=proto->next) {
            newProto = new MethodProto ();
            newProto->positionAt (proto);
            newProto->kind = proto->kind;
            newProto->selector = proto->selector;
            newProto->parmList =
                  copyParmListWithSubst (proto->parmList, subst);
            newProto->retType =
                  copyTypeWithSubst (proto->retType, subst);
            // Add to the list of inherited messages...
            newProto->next = inter->inheritedMethodProtos;
            inter->inheritedMethodProtos = newProto;
          }

          // Go through the superInterface's inherited messages (inheritedMethodProtos)
          // and make copies of them, too...
          for (proto = superInter->inheritedMethodProtos; proto; proto=proto->next) {
            newProto = new MethodProto ();
            newProto->positionAt (proto);
            newProto->kind = proto->kind;
            newProto->selector = proto->selector;
            newProto->parmList =
                  copyParmListWithSubst (proto->parmList, subst);
            newProto->retType =
                  copyTypeWithSubst (proto->retType, subst);
            // Add to the list of inherited messages...
            newProto->next = inter->inheritedMethodProtos;
            inter->inheritedMethodProtos = newProto;
          }


        }
      }
    }

  }
}


/***************  OBSOLETE ROUTINE  ***************

// setHetero (methodProto)
//
// See if this MethodProto uses TypeParms for the types of arguments or
// returned type.  Set the "covarianceRequired" or "contravarianceRequired"
// fields in the TypeParms, as necessary.
//
void setHetero (MethodProto * proto) {
  TypeParm * typeParm;
  Parameter * parm;
  NamedType * nType;

  for (parm = proto->parmList; parm; parm = (Parameter *) parm->next) {
    if ((parm->type) && (parm->type->op == NAMED_TYPE)) {
      nType = (NamedType *) parm->type;
      if ((nType->myDef) && (nType->myDef->op == TYPE_PARM)) {
        typeParm = (TypeParm *) nType->myDef;
        typeParm->contravarianceRequired = 1;
      }
    }
  }
  if ((proto->retType) && (proto->retType->op == NAMED_TYPE)) {
    nType = (NamedType *) proto->retType;
    if ((nType->myDef) && (nType->myDef->op == TYPE_PARM)) {
      typeParm = (TypeParm *) nType->myDef;
      typeParm->covarianceRequired = 1;
    }
  }
}

********************************************************/



// copyParmListWithSubst (parmList, subst)
//
// Build a new Parameter list by copying the old Parameter list, performing the
// substitution.  Try to share parts of the old list, if there is no change.
//
Parameter * copyParmListWithSubst (Parameter * parmList,
                                   Mapping<TypeParm,Type> * subst) {
  Parameter * newTail, * newParm;
  Type * newType;

  if (parmList == NULL) return NULL;

  // printf ("==========  copyParmListWithSubst entered, parmList = ");
  // pretty (parmList); 
  // printAst (6, parmList); 

  newTail = copyParmListWithSubst ((Parameter *) parmList->next, subst);
  newType = copyTypeWithSubst (parmList->type, subst);

  // If the new Parm would be the same, just return the old Parm object. 
  if ((newType == parmList->type) && (newTail == parmList->next)) {
    // printf ("==========    No change, returning parmList = ");
    // pretty (parmList); 
    // printAst (6, parmList); 
    return parmList;
  }

  // Else, build a new Parameter object.
  newParm = new Parameter ();
  newParm->positionAt (parmList);
  newParm->id = parmList->id;
  newParm->type = newType;
  newParm->next = newTail;
  // printf ("==========    ... Building new Parameter, newParm = ");
  // pretty (newParm); 
  // printAst (6, newParm); 
  return newParm;
}



// bindVarNames (node, varMapping)
//
// This routine walks the entire Abstract Syntax Tree, visiting every node.
// It looks for every "Variable" node and fills in its "myDef", which will
// point to one of the following:
//    Local
//    Global
//    Parameter
//    ClassField
//    FunctionProto
//    ConstDecl
//
// It catches the following errors:
//         Undefined variable name
//         This name already in use in this scope
//         This variable names something that is not global, local, parm, etc.
//         This variable was never used in this Function / Method
//         Attempt to invoke something that is not a function (or variable)
//
// This routine also fills in the "myDef" field in CallExprs.
//
// This routine sets "fourByteRestricted" whenever a TypeParm is used as the
// type of a local, parameter, record field, or class field.
//
// The "varMapping" is a Mapping in which to look up variable names.
// It will be NULL only for Headers.
//
void bindVarNames (AstNode * node,
                   Mapping <String, AstNode> * varMapping) {
  Header * header;
  Code * code;
  Uses * uses;
  Renaming * renaming;
  Interface * interface;
  ClassDef * cl;
  TypeDef * typeDef;
  ConstDecl * constDecl;
  ErrorDecl * errorDecl;
  FunctionProto * functionProto;
  Function * fun;
  MethodProto * methodProto;
  Method * meth;
  TypeParm * typeParm;
  TypeArg * typeArg;
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
  Parameter * parm, * protoParm, * funParm;
  ClassField * classField;
  RecordField * recordField;
  CallExpr * callExpr;
  SendExpr * sendExpr;
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
  Argument * arg;
  CountValue * countValue;
  FieldInit * fieldInit;
  AstNode * def;
  Mapping <String, AstNode> * newMap;
  Type * t1, * t2;

  if (node == NULL) return;

  // printf ("%s...\n", symbolName (node->op));

  switch (node->op) {

    case HEADER:
      header = (Header *) node;
      if (header->packageMapping == NULL) return;
      bindVarNames (header->uses, header->packageMapping);
      bindVarNames (header->consts, header->packageMapping);
      bindVarNames (header->errors, header->packageMapping);
      bindVarNames (header->globals, header->packageMapping);
      bindVarNames (header->typeDefs, header->packageMapping);
      bindVarNames (header->functionProtos, header->packageMapping);
      bindVarNames (header->functions, header->packageMapping);
      bindVarNames (header->closures, header->packageMapping);
      bindVarNames (header->interfaces, header->packageMapping);
      bindVarNames (header->classes, header->packageMapping);
      // bindVarNames (header->next, varMapping);
      return;

    case CODE:
      code = (Code *) node;
      // All this stuff has been moved into the main header, and we'll do it there.
      // bindVarNames (code->consts, varMapping);
      // bindVarNames (code->globals, varMapping);
      // bindVarNames (code->typeDefs, varMapping);
      // bindVarNames (code->functions, varMapping);
      // bindVarNames (code->interfaces, varMapping);
      // bindVarNames (code->classes, varMapping);
      // bindVarNames (code->behaviors, varMapping);
      return;

    case USES:
      uses = (Uses *) node;
      // printf ("  %s\n", uses->id->chars);
      bindVarNames (uses->renamings, varMapping);
      bindVarNames (uses->next, varMapping);
      return;

    case RENAMING:
      renaming = (Renaming *) node;
      bindVarNames (renaming->next, varMapping);
      return;

    case INTERFACE:
      interface = (Interface *) node;
      // printf ("  %s\n", interface->id->chars);
      bindVarNames (interface->typeParms, varMapping);
      bindVarNames (interface->extends, varMapping);
      bindVarNames (interface->methodProtos, varMapping);
      bindVarNames (interface->next, varMapping);
      return;

    case CLASS_DEF:
      cl = (ClassDef *) node;
      // printf ("==========  Processing class %s  ==========\n", cl->id->chars);
      // Run through our parameters and add them to the mapping...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (classField = cl->fields;
           classField;
           classField = (ClassField * ) classField->next) {
        def = newMap->find (classField->id);
        if (def) {
          error (classField, "The name you have chosen for this field is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (classField->id, classField);
        }
      }
      bindVarNames (cl->typeParms, varMapping);
      bindVarNames (cl->implements, varMapping);
      bindVarNames (cl->superclass, varMapping);
      bindVarNames (cl->fields, varMapping);
      bindVarNames (cl->methodProtos, newMap);
      bindVarNames (cl->methods, newMap);
      // Okay to delete newMap here, but don't waste the time.
      bindVarNames (cl->next, varMapping);
      // printf ("==========  End class %s  ==========\n", cl->id->chars);
      return;

    case BEHAVIOR:
      // behavior = (Behavior *) node;
      // The methods have been moved into their classes and we'll do them there.
      // bindVarNames (behavior->methods, varMapping);
      // bindVarNames (behavior->next, varMapping);
      return;

    case TYPE_DEF:
      typeDef = (TypeDef *) node;
      // printf ("  %s\n", typeDef->id->chars);
      bindVarNames (typeDef->type, varMapping);
      bindVarNames (typeDef->next, varMapping);
      return;

    case CONST_DECL:
      constDecl = (ConstDecl *) node;
      // printf ("  %s\n", constDecl->id->chars);
      bindVarNames (constDecl->expr, varMapping);
      bindVarNames (constDecl->next, varMapping);
      return;

    case ERROR_DECL:
      errorDecl = (ErrorDecl *) node;
      // printf ("  %s\n", errorDecl->id->chars);
      // Make sure that each parm has a distinct name...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (parm = errorDecl->parmList; parm; parm = (Parameter * ) parm->next) {
        def = newMap->find (parm->id);
        if (def) {
          error (parm, "The name you have chosen for this parameter is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (parm->id, parm);
        }
      }
      // Okay to delete newMap here, but don't waste the time.
      bindVarNames (errorDecl->parmList, varMapping);
      bindVarNames (errorDecl->next, varMapping);
      return;

    case FUNCTION_PROTO:
      functionProto = (FunctionProto *) node;
      // printf ("  %s\n", functionProto->id->chars);

      // Make sure that each parm has a distinct name...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (parm = functionProto->parmList; parm; parm = (Parameter * ) parm->next) {
        def = newMap->find (parm->id);
        if (def) {
          error (parm, "The name you have chosen for this parameter is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (parm->id, parm);
          if (parm->id->primitiveSymbol) {
            error (parm, "This happens to be the name of a built-in primitive function; please select a different name");
          }
        }
      }
      // Okay to delete newMap here, but don't waste the time.
      bindVarNames (functionProto->parmList, varMapping);
      bindVarNames (functionProto->retType, varMapping);
      bindVarNames (functionProto->next, varMapping);
      return;

    case FUNCTION:
      fun = (Function *) node;
      // if (fun->id) {
      //   printf ("  Looking at function %s...\n", fun->id->chars);
      //   printf ("     proto = %08x\n", fun->myProto);
      // } else {
      //   printf ("  Looking at some closure (id = NULL)...\n");
      // }

      // Run through our parameters and add them to the mapping...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (parm = fun->parmList; parm; parm = (Parameter * ) parm->next) {
        def = newMap->find (parm->id);
        if (def) {
          error (parm, "The name you have chosen for this parameter is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (parm->id, parm);
          if (parm->id->primitiveSymbol) {
            error (parm, "This happens to be the name of a built-in primitive function; please select a different name");
          }
        }
      }

      // Now walk the sub-trees...
      bindVarNames (fun->parmList, varMapping);
      bindVarNames (fun->retType, varMapping);
      bindVarNames (fun->locals, newMap);
      bindVarNames (fun->stmts, newMap);

      // Okay to delete newMap here, but don't waste the time.

      // Run through the locals and make sure each got used...
      for (local = fun->locals; local; local = (Local *) local->next) {
        if (! local->wasUsed) {
          error (local, "This variable was never used in this function; please eliminate it or comment it out");
         }
      }
      bindVarNames (fun->next, varMapping);
      return;

    case METHOD_PROTO:
      methodProto = (MethodProto *) node;
      // printf ("  %s\n", methodProto->selector->chars);

      // Run through our parameters and add them to the mapping...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (parm = methodProto->parmList; parm; parm = (Parameter * ) parm->next) {
        def = newMap->find (parm->id);
        if (def) {
          error (parm, "The name you have chosen for this parameter is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (parm->id, parm);
          if (parm->id->primitiveSymbol) {
            error (parm, "This happens to be the name of a built-in primitive function; please select a different name");
          }
        }
      }
      // Okay to delete newMap here, but don't waste the time.

      // The locals will be added as they are encountered...

      bindVarNames (methodProto->parmList, varMapping);
      bindVarNames (methodProto->retType, varMapping);
      bindVarNames (methodProto->next, varMapping);
      return;

    case METHOD:
      meth = (Method *) node;
      // if (meth->selector) {
      //   printf ("  Looking at method %s\n", meth->selector->chars);
      // }
      // Run through our parameters and add them to the mapping...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (parm = meth->parmList; parm; parm = (Parameter * ) parm->next) {
        def = newMap->find (parm->id);
        if (def) {
          error (parm, "The name you have chosen for this parameter is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (parm->id, parm);
          if (parm->id->primitiveSymbol) {
            error (parm, "This happens to be the name of a built-in primitive function; please select a different name");
          }
        }
      }
      bindVarNames (meth->parmList, varMapping);
      bindVarNames (meth->retType, varMapping);
      bindVarNames (meth->locals, newMap);
      bindVarNames (meth->stmts, newMap);

      // Okay to delete newMap here, but don't waste the time.

      // Run through the locals and make sure each got used...
      for (local = meth->locals; local; local = (Local *) local->next) {
        if (! local->wasUsed) {
          error (local, "This variable was never used in this method; please eliminate it or comment it out");
         }
      }
      bindVarNames (meth->next, varMapping);
      return;

    case TYPE_PARM:
      typeParm = (TypeParm *) node;
      bindVarNames (typeParm->type, varMapping);
      bindVarNames (typeParm->next, varMapping);
      return;

    case TYPE_ARG:
      typeArg = (TypeArg *) node;
      bindVarNames (typeArg->type, varMapping);
      bindVarNames (typeArg->next, varMapping);
      return;

    case CHAR_TYPE:
      return;

    case INT_TYPE:
      return;

    case DOUBLE_TYPE:
      return;

    case BOOL_TYPE:
      return;

    case VOID_TYPE:
      return;

    case TYPE_OF_NULL_TYPE:
      return;

    case ANY_TYPE:
      return;

    case PTR_TYPE:
      pType = (PtrType *) node;
      bindVarNames (pType->baseType, varMapping);
      return;

    case ARRAY_TYPE:
      aType = (ArrayType *) node;
      bindVarNames (aType->sizeExpr, varMapping);
      bindVarNames (aType->baseType, varMapping);

      // If the baseType is a TypeParm, then set 'fourByteRestricted'...
      setFourByteRestricted (aType->baseType);
      return;

    case RECORD_TYPE:
      rType = (RecordType *) node;
      bindVarNames (rType->fields, varMapping);
      return;

    case FUNCTION_TYPE:
      fType = (FunctionType *) node;
      bindVarNames (fType->parmTypes, varMapping);
      bindVarNames (fType->retType, varMapping);
      return;

    case NAMED_TYPE:
      nType = (NamedType *) node;
      // printf ("  %s\n", nType->id->chars);
      bindVarNames (nType->typeArgs, varMapping);
      return;

    case IF_STMT:
      ifStmt = (IfStmt *) node;
      bindVarNames (ifStmt->expr, varMapping);
      bindVarNames (ifStmt->thenStmts, varMapping);
      bindVarNames (ifStmt->elseStmts, varMapping);
      bindVarNames (ifStmt->next, varMapping);
      return;

    case ASSIGN_STMT:
      assignStmt = (AssignStmt *) node;
      bindVarNames (assignStmt->lvalue, varMapping);
      bindVarNames (assignStmt->expr, varMapping);
      bindVarNames (assignStmt->next, varMapping);
      return;

    case CALL_STMT:
      callStmt = (CallStmt *) node;
      bindVarNames (callStmt->expr, varMapping);
      bindVarNames (callStmt->next, varMapping);
      return;

    case SEND_STMT:
      sendStmt = (SendStmt *) node;
      bindVarNames (sendStmt->expr, varMapping);
      bindVarNames (sendStmt->next, varMapping);
      return;

    case WHILE_STMT:
      whileStmt = (WhileStmt *) node;
      bindVarNames (whileStmt->expr, varMapping);
      bindVarNames (whileStmt->stmts, varMapping);
      bindVarNames (whileStmt->next, varMapping);
      return;

    case DO_STMT:
      doStmt = (DoStmt *) node;
      bindVarNames (doStmt->stmts, varMapping);
      bindVarNames (doStmt->expr, varMapping);
      bindVarNames (doStmt->next, varMapping);
      return;

    case BREAK_STMT:
      breakStmt = (BreakStmt *) node;
      bindVarNames (breakStmt->next, varMapping);
      return;

    case CONTINUE_STMT:
      continueStmt = (ContinueStmt *) node;
      bindVarNames (continueStmt->next, varMapping);
      return;

    case RETURN_STMT:
      returnStmt = (ReturnStmt *) node;
      bindVarNames (returnStmt->expr, varMapping);
      bindVarNames (returnStmt->next, varMapping);
      // Testing...
      // printf ("=====  varMapping  =====\n");
      // if (varMapping) {
      //   varMapping->print (6);
      // } else {
      //   printf ("    NULL\n");
      // }
      // printf ("========================\n");
      return;

    case FOR_STMT:
      forStmt = (ForStmt *) node;
      bindVarNames (forStmt->lvalue, varMapping);
      bindVarNames (forStmt->expr1, varMapping);
      bindVarNames (forStmt->expr2, varMapping);
      bindVarNames (forStmt->expr3, varMapping);
      bindVarNames (forStmt->stmts, varMapping);
      bindVarNames (forStmt->next, varMapping);
      return;

    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) node;
      bindVarNames (switchStmt->expr, varMapping);
      bindVarNames (switchStmt->caseList, varMapping);
      bindVarNames (switchStmt->defaultStmts, varMapping);
      bindVarNames (switchStmt->next, varMapping);
      return;

    case TRY_STMT:
      tryStmt = (TryStmt *) node;
      bindVarNames (tryStmt->stmts, varMapping);
      bindVarNames (tryStmt->catchList, varMapping);
      bindVarNames (tryStmt->next, varMapping);
      return;

    case THROW_STMT:
      throwStmt = (ThrowStmt *) node;
      bindVarNames (throwStmt->argList, varMapping);
      bindVarNames (throwStmt->next, varMapping);
      return;

    case FREE_STMT:
      freeStmt = (FreeStmt *) node;
      bindVarNames (freeStmt->expr, varMapping);
      bindVarNames (freeStmt->next, varMapping);
      return;

    case DEBUG_STMT:
      debugStmt = (DebugStmt *) node;
      bindVarNames (debugStmt->next, varMapping);
      return;

    case CASE:
      cas = (Case *) node;
      bindVarNames (cas->expr, varMapping);
      bindVarNames (cas->stmts, varMapping);
      bindVarNames (cas->next, varMapping);
      return;

    case CATCH:
      cat = (Catch *) node;
      // printf ("  Looking at catch %s\n", cat->id->chars);
      // Run through our parameters and add them to the mapping...
      newMap = new Mapping <String, AstNode> (5, varMapping);
      for (parm = cat->parmList; parm; parm = (Parameter * ) parm->next) {
        def = newMap->find (parm->id);
        if (def) {
          error (parm, "The name you have chosen for this parameter is already in use at this point; please choose a different name"); 
          error2 (def, "Here is something else with the same name");
        } else {
          newMap->enter (parm->id, parm);
          if (parm->id->primitiveSymbol) {
            error (parm, "This happens to be the name of a built-in primitive function; please select a different name");
          }
        }
      }
      bindVarNames (cat->parmList, varMapping);
      bindVarNames (cat->stmts, newMap);
      // Okay to delete newMap here, but don't waste the time.
      bindVarNames (cat->next, varMapping);
      return;

    case GLOBAL:
      global = (Global *) node;
      bindVarNames (global->type, varMapping);
      bindVarNames (global->initExpr, varMapping);
      bindVarNames (global->next, varMapping);
      return;

    case LOCAL:
      local = (Local *) node;
      bindVarNames (local->type, varMapping);
      // If the type is a TypeParm, then set 'fourByteRestricted'...
      setFourByteRestricted (local->type);

      // Look at the initializing expression first, since it must not contain
      // any uses of the variable being defined...
      bindVarNames (local->initExpr, varMapping);

      // Now add this variable to the mapping...
      def = varMapping->find (local->id);
      if (def) {
        error (local, "The name you have chosen for this variable is already in use at this point; please choose a different name"); 
        error2 (def, "Here is something else with the same name");
      } else {
        varMapping->enter (local->id, local);
        if (local->id->primitiveSymbol) {
          error (local, "This happens to be the name of a built-in primitive function; please select a different name");
        }
      }
      bindVarNames (local->next, varMapping);
      return;

    case PARAMETER:
      parm = (Parameter *) node;
      bindVarNames (parm->type, varMapping);

      // If the type is a TypeParm, then set 'fourByteRestricted'...
      setFourByteRestricted (parm->type);
      bindVarNames (parm->next, varMapping);
      return;

    case CLASS_FIELD:
      classField = (ClassField *) node;
      bindVarNames (classField->type, varMapping);

      // If the type is a TypeParm, then set 'fourByteRestricted'...
      setFourByteRestricted (classField->type);
      bindVarNames (classField->next, varMapping);
      return;

    case RECORD_FIELD:
      recordField = (RecordField *) node;
      bindVarNames (recordField->type, varMapping);

      // If the type is a TypeParm, then set 'fourByteRestricted'...
      setFourByteRestricted (recordField->type);
      bindVarNames (recordField->next, varMapping);
      return;

    case INT_CONST:
      return;

    case DOUBLE_CONST:
      return;

    case CHAR_CONST:
      return;

    case STRING_CONST:
      return;

    case BOOL_CONST:
      return;

    case NULL_CONST:
      return;

    case CALL_EXPR:
      callExpr = (CallExpr *) node;
      bindVarNames (callExpr->argList, varMapping);

      // In "foo(...)" find the definition of "foo" and set "myDef" to it...
      def = varMapping->find (callExpr->id);
      if (def == NULL) {
        // Ignore "undefined name" error here, since it could be a primitive...
        // error (callExpr, "There is no function with this name (nor is there a variable with this name known at this point)");
      } else {
        if ((def->op == LOCAL) ||
            (def->op == GLOBAL) ||
            (def->op == PARAMETER) ||
            (def->op == CLASS_FIELD) ||
            (def->op == FUNCTION_PROTO)) {
          callExpr->myDef = def;
          if (def->op == LOCAL) {
            ((Local *) def)->wasUsed = 1;
          }
        } else {
          error (callExpr, "You are attempting to invoke something that is not a function (or local, global, parameter, or class field)");
          error2 (def, "Here is the definition of the item you are trying to invoke");
        }
      }
      return;

    case SEND_EXPR:
      sendExpr = (SendExpr *) node;
      bindVarNames (sendExpr->receiver, varMapping);
      bindVarNames (sendExpr->argList, varMapping);
      return;

    case SELF_EXPR:
      return;

    case SUPER_EXPR:
      return;

    case FIELD_ACCESS:
      fieldAccess = (FieldAccess *) node;
      bindVarNames (fieldAccess->expr, varMapping);
      return;

    case ARRAY_ACCESS:
      arrayAccess = (ArrayAccess *) node;
      bindVarNames (arrayAccess->arrayExpr, varMapping);
      bindVarNames (arrayAccess->indexExpr, varMapping);
      return;

    case CONSTRUCTOR:
      constructor = (Constructor *) node;
      bindVarNames (constructor->type, varMapping);
      bindVarNames (constructor->countValueList, varMapping);
      bindVarNames (constructor->fieldInits, varMapping);
      return;

    case CLOSURE_EXPR:
      closureExpr = (ClosureExpr *) node;
      // Skip this, since we'll walk the function when we do header->closures...
      // bindVarNames (closureExpr->function, varMapping);
      return;

    case VARIABLE_EXPR:
      var = (VariableExpr *) node;
      // printf ("  Binding \"%s\"...\n", var->id->chars);

      // Look this variable up and set its "myDef" to point to the definition...
      def = varMapping->find (var->id);
      if (def == NULL) {
        error (var, "This variable name is undefined at this point");
      } else {
        if ((def->op == LOCAL) ||
            (def->op == GLOBAL) ||
            (def->op == PARAMETER) ||
            (def->op == CLASS_FIELD) ||
            (def->op == FUNCTION_PROTO) ||
            (def->op == CONST_DECL)) {
          var->myDef = def;
          if (def->op == LOCAL) {
            ((Local *) def)->wasUsed = 1;
          }
        } else {
          error (var, "This variable names something that is not a local, global, parameter, class field, function, or constant");
          error2 (def, "Here is the item being named");
        }
      }
      return;

    case AS_PTR_TO_EXPR:
      asPtrToExpr = (AsPtrToExpr *) node;
      bindVarNames (asPtrToExpr->expr, varMapping);
      bindVarNames (asPtrToExpr->type, varMapping);
      return;

    case AS_INTEGER_EXPR:
      asIntegerExpr = (AsIntegerExpr *) node;
      bindVarNames (asIntegerExpr->expr, varMapping);
      return;

    case ARRAY_SIZE_EXPR:
      arraySizeExpr = (ArraySizeExpr *) node;
      bindVarNames (arraySizeExpr->expr, varMapping);
      return;

    case IS_INSTANCE_OF_EXPR:
      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      bindVarNames (isInstanceOfExpr->expr, varMapping);
      bindVarNames (isInstanceOfExpr->type, varMapping);
      return;

    case IS_KIND_OF_EXPR:
      isKindOfExpr = (IsKindOfExpr *) node;
      bindVarNames (isKindOfExpr->expr, varMapping);
      bindVarNames (isKindOfExpr->type, varMapping);
      return;

    case SIZE_OF_EXPR:
      sizeOfExpr = (SizeOfExpr *) node;
      bindVarNames (sizeOfExpr->type, varMapping);
      return;

    case ARGUMENT:
      arg = (Argument *) node;
      bindVarNames (arg->expr, varMapping);
      bindVarNames (arg->next, varMapping);
      return;

    case COUNT_VALUE:
      countValue = (CountValue *) node;
      bindVarNames (countValue->count, varMapping);
      bindVarNames (countValue->value, varMapping);
      bindVarNames (countValue->next, varMapping);
      return;

    case FIELD_INIT:
      fieldInit = (FieldInit *) node;
      bindVarNames (fieldInit->expr, varMapping);
      bindVarNames (fieldInit->next, varMapping);
      return;

    default:
      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in bindVarNames");
  }
}



// setFourByteRestricted (type)
//
// This routine is passed a type.  If that type is a type parameter,
// it will set the "fourByteRestricted" field in the type parameter.
//
void setFourByteRestricted (Type * t) {
  TypeParm * typeParm;
  NamedType * nType = (NamedType *) t;

  if (nType->op == NAMED_TYPE) {
    typeParm = (TypeParm *) nType->myDef;
    if (typeParm && typeParm->op == TYPE_PARM) {
      typeParm->fourByteRestricted = 1;
    }
  }
}



// assignOffsetsAndEvalExprs (hdr)
//
// This routine runs through all classes and assigns offsets
// to each class and record field and computes the size of each class object.
//
// This routine also evaluates all static expressions.
//
// This routine catches...
//    Errors related to problems evaluating static expressions (eg, 4/0)
//    Errors in determining the size of fields, locals, and parms
//    Errors in assigning offsets to fields, locals, and parms
//    Errors in determining the size of classes or records
//
// NOTE: We have a chicken-and-egg problem.  Sizes of types can depend on expressions
// and expressions can depend on the sizes of types.  Here is an example:
//        const MAX = 1000
//        type T1 = array [MAX+1] of char
//        const SIZ = (sizeOf T1) - 4
//        type T2 = array [SIZ] of int
// Therefore, we use an iterative approach, which keeps processing until there are
// no further changes.  Then, in a final pass, it prints errors.
//
void assignOffsetsAndEvalExprs (Header * hdr) {

  // Keep making passes until we are not making any progress.
  changed = 1;
  while (changed) {
    changed = 0;
    // printf ("\n---MAKING NEXT PASS---\n\n");
    assignOffsets2 (hdr, 0);      // wantPrinting = false
    evalExprsIn (hdr);
  }

  // printf ("\n---MAKING FINAL PASS---\n\n");

  // Make one more pass to print any errors.
  assignOffsets2 (hdr, 1);        // wantPrinting = true
}



// assignOffsets2 (node, wantPrinting)
//
// This routine visits all nodes.
//
// For each class, it attempts to assign offsets to each field and to compute the
// size of each class object.
//
// For each record, it attempts to assign offsets to each field and to compute the
// size of each record object.
//
// It also attempts to assign offsets to parameters and local variables.
//
// This routine is passed "wantPrinting".  If true, any errors are reported.
// If false, errors are not printed.
//
// This routine sets 'changed' if anything was changed.
//
void assignOffsets2 (AstNode * node, int wantPrinting) {
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
    CallExpr * callExpr;
    SendExpr * sendExpr;
    SelfExpr * selfExpr;
    SuperExpr * superExpr;
    FieldAccess * fieldAccess;
    ArrayAccess * arrayAccess;
    Constructor * constructor;
    ClosureExpr * closureExpr;
    AsPtrToExpr * asPtrToExpr;
    AsIntegerExpr * asIntegerExpr;
    ArraySizeExpr * arraySizeExpr;
    IsInstanceOfExpr * isInstanceOfExpr;
    IsKindOfExpr * isKindOfExpr;
    SizeOfExpr * sizeOfExpr;
    Argument * arg;
    CountValue * countValue;
    FieldInit * fieldInit;
    int junk;

  if (node == NULL) return;

  // printf ("AssignOffsets in %s...\n", symbolName (node->op));

  switch (node->op) {

    case HEADER:
      header = (Header *) node;
      // printf ("  %s\n", header->packageName->chars);
      assignOffsets2 (header->uses, wantPrinting);
      assignOffsets2 (header->consts, wantPrinting);
      assignOffsets2 (header->errors, wantPrinting);
      assignOffsets2 (header->globals, wantPrinting);
      for (global = header->globals; global; global = (Global *) global->next) {
        global->sizeInBytes = sizeInBytesOfWhole (global->type, global, wantPrinting);
      }
      assignOffsets2 (header->typeDefs, wantPrinting);
      assignOffsets2 (header->functionProtos, wantPrinting);
      assignOffsets2 (header->functions, wantPrinting);
      assignOffsets2 (header->closures, wantPrinting);
      assignOffsets2 (header->interfaces, wantPrinting);
      assignOffsets2 (header->classes, wantPrinting);
      // assignOffsets2 (header->next, wantPrinting);
      return;

//    case CODE:
//      code = (Code *) node;
//      assignOffsets2 (code->consts, wantPrinting);
//      assignOffsets2 (code->globals, wantPrinting);
//      assignOffsets2 (code->typeDefs, wantPrinting);
//      assignOffsets2 (code->functions, wantPrinting);
//      assignOffsets2 (code->interfaces, wantPrinting);
//      assignOffsets2 (code->classes, wantPrinting);
//      assignOffsets2 (code->behaviors, wantPrinting);
//      return;

    case USES:
      uses = (Uses *) node;
      assignOffsets2 (uses->renamings, wantPrinting);
      assignOffsets2 (uses->next, wantPrinting);
      return;

    case RENAMING:
      renaming = (Renaming *) node;
      assignOffsets2 (renaming->next, wantPrinting);
      return;

    case INTERFACE:
      interface = (Interface *) node;
      // printf ("  %s\n", interface->id->chars);
      assignOffsets2 (interface->typeParms, wantPrinting);
      assignOffsets2 (interface->extends, wantPrinting);
      assignOffsets2 (interface->methodProtos, wantPrinting);
      assignOffsets2 (interface->next, wantPrinting);
      return;

    case CLASS_DEF:
      cl = (ClassDef *) node;
      // printf ("  %s\n", cl->id->chars);

      // Assign offsets to the fields...
      assignOffsetsInClass (cl, wantPrinting);

      assignOffsets2 (cl->typeParms, wantPrinting);
      assignOffsets2 (cl->implements, wantPrinting);
      assignOffsets2 (cl->superclass, wantPrinting);
//      assignOffsets2 (cl->renamings, wantPrinting);
      assignOffsets2 (cl->fields, wantPrinting);
      assignOffsets2 (cl->methodProtos, wantPrinting);
      assignOffsets2 (cl->methods, wantPrinting);
      assignOffsets2 (cl->next, wantPrinting);
      return;

//    case BEHAVIOR:
//      behavior = (Behavior *) node;
//      printf ("  %s\n", behavior->id->chars);
//      assignOffsets2 (behavior->methods, wantPrinting);
//      assignOffsets2 (behavior->next, wantPrinting);
//      return;

    case TYPE_DEF:
      typeDef = (TypeDef *) node;
      // printf ("  %s\n", typeDef->id->chars);
      assignOffsets2 (typeDef->type, wantPrinting);
      assignOffsets2 (typeDef->next, wantPrinting);
      return;

    case CONST_DECL:
      constDecl = (ConstDecl *) node;
      // printf ("  %s\n", constDecl->id->chars);
      assignOffsets2 (constDecl->expr, wantPrinting);
      assignOffsets2 (constDecl->next, wantPrinting);
      return;

    case ERROR_DECL:
      errorDecl = (ErrorDecl *) node;
      // printf ("  ASSIGNING OFFSETS IN ERROR_DECL %s\n", errorDecl->id->chars);
      // printf ("    wantPrinting = %d\n", wantPrinting);
      assignOffsets2 (errorDecl->parmList, wantPrinting);
      assignOffsets2 (errorDecl->next, wantPrinting);
      // Assign offsets to the parameters...
      errorDecl->totalParmSize =
            assignOffsetsInParmList (errorDecl->parmList,wantPrinting, 8);
                                                  // starting offset = 8

      return;

    case FUNCTION_PROTO:
      functionProto = (FunctionProto *) node;
      // printf ("  Function Proto %s\n", functionProto->id->chars);
      assignOffsets2 (functionProto->parmList, wantPrinting);
      // Assign offsets to the parameters...
      functionProto->totalParmSize =
             assignOffsetsInParmList (functionProto->parmList, wantPrinting, 8);
                                                        // starting offset = 8
      assignOffsets2 (functionProto->retType, wantPrinting);
      assignOffsets2 (functionProto->next, wantPrinting);
      return;

    case FUNCTION:
      fun = (Function *) node;
      // if (fun->id) {
      //   printf ("  Function %s\n", fun->id->chars);
      // } else {
      //   printf ("  Closure\n");
      // }
      assignOffsets2 (fun->parmList, wantPrinting);

      // Assign offsets to the parameters...
      fun->totalParmSize =
             assignOffsetsInParmList (fun->parmList, wantPrinting, 8);
                                              // starting offset = 8

      assignOffsets2 (fun->retType, wantPrinting);
      assignOffsets2 (fun->locals, wantPrinting);
      // Offsets for locals will be assigned later, since tems may get added later...
      assignOffsets2 (fun->stmts, wantPrinting);
      assignOffsets2 (fun->next, wantPrinting);
      return;

    case METHOD_PROTO:
      methodProto = (MethodProto *) node;
      // printf ("  %s\n", methodProto->selector->chars);
      assignOffsets2 (methodProto->parmList, wantPrinting);

      // Assign offsets to the parameters...
      methodProto->totalParmSize = 4 +                // Space for receiver
             assignOffsetsInParmList (methodProto->parmList, wantPrinting, 12);
                                                      // starting offset = 12
      assignOffsets2 (methodProto->retType, wantPrinting);
      assignOffsets2 (methodProto->next, wantPrinting);
      return;

    case METHOD:
      meth = (Method *) node;
      // printf ("  %s\n", meth->selector->chars);
      assignOffsets2 (meth->parmList, wantPrinting);

      // Assign offsets to the parameters...
      meth->totalParmSize = 4 +                // Space for receiver
             assignOffsetsInParmList (meth->parmList, wantPrinting, 12);
                                               // starting offset = 12

      assignOffsets2 (meth->retType, wantPrinting);
      assignOffsets2 (meth->locals, wantPrinting);
      // Offsets will be assigned to the locals, since temps may get added later...
      assignOffsets2 (meth->stmts, wantPrinting);
      assignOffsets2 (meth->next, wantPrinting);
      return;

    case TYPE_PARM:
      typeParm = (TypeParm *) node;
      assignOffsets2 (typeParm->type, wantPrinting);
      assignOffsets2 (typeParm->next, wantPrinting);
      return;

    case TYPE_ARG:
      typeArg = (TypeArg *) node;
      assignOffsets2 (typeArg->type, wantPrinting);
      assignOffsets2 (typeArg->next, wantPrinting);
      return;

    case CHAR_TYPE:
      return;

    case INT_TYPE:
      return;

    case DOUBLE_TYPE:
      return;

    case BOOL_TYPE:
      return;

    case VOID_TYPE:
      return;

    case TYPE_OF_NULL_TYPE:
      return;

    case ANY_TYPE:
      return;

    case PTR_TYPE:
      pType = (PtrType *) node;
      assignOffsets2 (pType->baseType, wantPrinting);
      return;

    case ARRAY_TYPE:
      aType = (ArrayType *) node;
      assignOffsets2 (aType->sizeExpr, wantPrinting);
      assignOffsets2 (aType->baseType, wantPrinting);
      // Compute the size of the array, if we can.  If not, no problem...
      if (aType->sizeInBytes < 0) {
        aType->sizeInBytes = sizeInBytesOfWhole (aType, node, 0);   // wantPrinting = 0
        aType->sizeOfElements = sizeInBytesOfWhole (aType->baseType, node, 0);
                                                                    // wantPrinting = 0
      }
      junk = sizeInBytesOfWhole (aType->baseType, aType->baseType, wantPrinting);
      return;

    case RECORD_TYPE:
      rType = (RecordType *) node;
      // printf ("  ASSIGNING OFFSETS IN RECORD...\n");

      // Assign offsets to the fields...
      assignOffsetsInRecord (rType, wantPrinting);

      assignOffsets2 (rType->fields, wantPrinting);
      return;

    case FUNCTION_TYPE:
      fType = (FunctionType *) node;
      assignOffsets2 (fType->parmTypes, wantPrinting);
      assignOffsets2 (fType->retType, wantPrinting);
      return;

    case NAMED_TYPE:
      nType = (NamedType *) node;
      assignOffsets2 (nType->typeArgs, wantPrinting);
      return;

    case IF_STMT:
      ifStmt = (IfStmt *) node;
      assignOffsets2 (ifStmt->expr, wantPrinting);
      assignOffsets2 (ifStmt->thenStmts, wantPrinting);
      assignOffsets2 (ifStmt->elseStmts, wantPrinting);
      assignOffsets2 (ifStmt->next, wantPrinting);
      return;

    case ASSIGN_STMT:
      assignStmt = (AssignStmt *) node;
      assignOffsets2 (assignStmt->lvalue, wantPrinting);
      assignOffsets2 (assignStmt->expr, wantPrinting);
      assignOffsets2 (assignStmt->next, wantPrinting);
      return;

    case CALL_STMT:
      callStmt = (CallStmt *) node;
      assignOffsets2 (callStmt->expr, wantPrinting);
      assignOffsets2 (callStmt->next, wantPrinting);
      return;

    case SEND_STMT:
      sendStmt = (SendStmt *) node;
      assignOffsets2 (sendStmt->expr, wantPrinting);
      assignOffsets2 (sendStmt->next, wantPrinting);
      return;

    case WHILE_STMT:
      whileStmt = (WhileStmt *) node;
      assignOffsets2 (whileStmt->expr, wantPrinting);
      assignOffsets2 (whileStmt->stmts, wantPrinting);
      assignOffsets2 (whileStmt->next, wantPrinting);
      return;

    case DO_STMT:
      doStmt = (DoStmt *) node;
      assignOffsets2 (doStmt->stmts, wantPrinting);
      assignOffsets2 (doStmt->expr, wantPrinting);
      assignOffsets2 (doStmt->next, wantPrinting);
      return;

    case BREAK_STMT:
      breakStmt = (BreakStmt *) node;
      assignOffsets2 (breakStmt->next, wantPrinting);
      return;

    case CONTINUE_STMT:
      continueStmt = (ContinueStmt *) node;
      assignOffsets2 (continueStmt->next, wantPrinting);
      return;

    case RETURN_STMT:
      returnStmt = (ReturnStmt *) node;
      assignOffsets2 (returnStmt->expr, wantPrinting);
      assignOffsets2 (returnStmt->next, wantPrinting);
      return;

    case FOR_STMT:
      forStmt = (ForStmt *) node;
      assignOffsets2 (forStmt->lvalue, wantPrinting);
      assignOffsets2 (forStmt->expr1, wantPrinting);
      assignOffsets2 (forStmt->expr2, wantPrinting);
      assignOffsets2 (forStmt->expr3, wantPrinting);
      assignOffsets2 (forStmt->stmts, wantPrinting);
      assignOffsets2 (forStmt->next, wantPrinting);
      return;

    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) node;
      assignOffsets2 (switchStmt->expr, wantPrinting);
      assignOffsets2 (switchStmt->caseList, wantPrinting);
      assignOffsets2 (switchStmt->defaultStmts, wantPrinting);
      assignOffsets2 (switchStmt->next, wantPrinting);
      return;

    case TRY_STMT:
      tryStmt = (TryStmt *) node;
      assignOffsets2 (tryStmt->stmts, wantPrinting);
      assignOffsets2 (tryStmt->catchList, wantPrinting);
      assignOffsets2 (tryStmt->next, wantPrinting);
      return;

    case THROW_STMT:
      throwStmt = (ThrowStmt *) node;
      assignOffsets2 (throwStmt->argList, wantPrinting);
      assignOffsets2 (throwStmt->next, wantPrinting);
      return;

    case FREE_STMT:
      freeStmt = (FreeStmt *) node;
      assignOffsets2 (freeStmt->expr, wantPrinting);
      assignOffsets2 (freeStmt->next, wantPrinting);
      return;

    case DEBUG_STMT:
      debugStmt = (DebugStmt *) node;
      assignOffsets2 (debugStmt->next, wantPrinting);
      return;

    case CASE:
      cas = (Case *) node;
      assignOffsets2 (cas->expr, wantPrinting);
      assignOffsets2 (cas->stmts, wantPrinting);
      assignOffsets2 (cas->next, wantPrinting);
      return;

    case CATCH:
      cat = (Catch *) node;
      assignOffsets2 (cat->parmList, wantPrinting);

      // Assign offsets to the parameters...
      //if (commandOptionS && wantPrinting) {
      //  if (cat->id) {
      //    printf ("==========  Parameter layout for catch \"");
      //    printString (stdout, cat->id);
      //    printf ("\" (%s, line %d)  ==========\n",
      //               extractFilename (cat->tokn),
      //               extractLineNumber (cat->tokn));
      //  }
      //}
      assignOffsetsInParmList (cat->parmList, wantPrinting, 0);   // starting offset = 0

      assignOffsets2 (cat->stmts, wantPrinting);
      assignOffsets2 (cat->next, wantPrinting);
      return;

    case GLOBAL:
      global = (Global *) node;
      assignOffsets2 (global->type, wantPrinting);
      assignOffsets2 (global->initExpr, wantPrinting);
      assignOffsets2 (global->next, wantPrinting);
      return;

    case LOCAL:
      local = (Local *) node;
      assignOffsets2 (local->type, wantPrinting);
      assignOffsets2 (local->initExpr, wantPrinting);
      assignOffsets2 (local->next, wantPrinting);
      return;

    case PARAMETER:
      parm = (Parameter *) node;
      assignOffsets2 (parm->type, wantPrinting);
      assignOffsets2 (parm->next, wantPrinting);
      return;

    case CLASS_FIELD:
      classField = (ClassField *) node;
      assignOffsets2 (classField->type, wantPrinting);
      assignOffsets2 (classField->next, wantPrinting);
      return;

    case RECORD_FIELD:
      recordField = (RecordField *) node;
      assignOffsets2 (recordField->type, wantPrinting);
      assignOffsets2 (recordField->next, wantPrinting);
      return;

    case INT_CONST:
      return;

    case DOUBLE_CONST:
      return;

    case CHAR_CONST:
      return;

    case STRING_CONST:
      return;

    case BOOL_CONST:
      return;

    case NULL_CONST:
      return;

    case CALL_EXPR:
      callExpr = (CallExpr *) node;
      assignOffsets2 (callExpr->argList, wantPrinting);
      return;

    case SEND_EXPR:
      sendExpr = (SendExpr *) node;
      assignOffsets2 (sendExpr->receiver, wantPrinting);
      assignOffsets2 (sendExpr->argList, wantPrinting);
      return;

    case SELF_EXPR:
      selfExpr = (SelfExpr *) node;
      return;

    case SUPER_EXPR:
      superExpr = (SuperExpr *) node;
      return;

    case FIELD_ACCESS:
      fieldAccess = (FieldAccess *) node;
      assignOffsets2 (fieldAccess->expr, wantPrinting);
      return;

    case ARRAY_ACCESS:
      arrayAccess = (ArrayAccess *) node;
      assignOffsets2 (arrayAccess->arrayExpr, wantPrinting);
      assignOffsets2 (arrayAccess->indexExpr, wantPrinting);
      return;

    case CONSTRUCTOR:
      constructor = (Constructor *) node;
      assignOffsets2 (constructor->type, wantPrinting);
      assignOffsets2 (constructor->countValueList, wantPrinting);
      assignOffsets2 (constructor->fieldInits, wantPrinting);
      return;

    case CLOSURE_EXPR:
      closureExpr = (ClosureExpr *) node;
      // Skip this, since we'll walk the function when we do header->closures...
      // assignOffsets2 (closureExpr->function, wantPrinting);
      return;

    case VARIABLE_EXPR:
      return;

    case AS_PTR_TO_EXPR:
      asPtrToExpr = (AsPtrToExpr *) node;
      assignOffsets2 (asPtrToExpr->expr, wantPrinting);
      assignOffsets2 (asPtrToExpr->type, wantPrinting);
      return;

    case AS_INTEGER_EXPR:
      asIntegerExpr = (AsIntegerExpr *) node;
      assignOffsets2 (asIntegerExpr->expr, wantPrinting);
      return;

    case ARRAY_SIZE_EXPR:
      arraySizeExpr = (ArraySizeExpr *) node;
      assignOffsets2 (arraySizeExpr->expr, wantPrinting);
      return;

    case IS_INSTANCE_OF_EXPR:
      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      assignOffsets2 (isInstanceOfExpr->expr, wantPrinting);
      assignOffsets2 (isInstanceOfExpr->type, wantPrinting);
      return;

    case IS_KIND_OF_EXPR:
      isKindOfExpr = (IsKindOfExpr *) node;
      assignOffsets2 (isKindOfExpr->expr, wantPrinting);
      assignOffsets2 (isKindOfExpr->type, wantPrinting);
      return;

    case SIZE_OF_EXPR:
      sizeOfExpr = (SizeOfExpr *) node;
      assignOffsets2 (sizeOfExpr->type, wantPrinting);
      return;

    case ARGUMENT:
      arg = (Argument *) node;
      assignOffsets2 (arg->expr, wantPrinting);
      assignOffsets2 (arg->next, wantPrinting);
      return;

    case COUNT_VALUE:
      countValue = (CountValue *) node;
      assignOffsets2 (countValue->count, wantPrinting);
      assignOffsets2 (countValue->value, wantPrinting);
      assignOffsets2 (countValue->next, wantPrinting);
      return;

    case FIELD_INIT:
      fieldInit = (FieldInit *) node;
      assignOffsets2 (fieldInit->expr, wantPrinting);
      assignOffsets2 (fieldInit->next, wantPrinting);
      return;

    default:
      printf ("node->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in assignOffsets2");
  }
}



// assignOffsetsInClass (cl, wantPrinting)
//
// This routine attempts to assign offsets in a class and sets 'changed' if
// anything was changed.
//
void assignOffsetsInClass (ClassDef * cl, int wantPrinting) {
  int nextOffset, padding, incr;
  ClassField * classField;

// We start numbering class fields at 4.  (The class ptr is at offset 0.)
#define STARTING_CLASS_OFFSET 4

  if (wantPrinting && (cl->sizeInBytes < 0)) {
    error (cl, "I am unable to determine the size of this class");
  }

  // Print the class layout, if desired...
  if (commandOptionS && wantPrinting) {
    printf ("==========  Class ");
    printString (stdout, cl->id);
    printf ("  ==========\n");
    printf ("    Fields:\n");
    printf ("        Offset 0:\t4\t<class ptr>\n");
    nextOffset = STARTING_CLASS_OFFSET;
    for (classField = cl->fields;
         classField != NULL;
         classField = (ClassField *) classField->next) {
      // Display padding bytes, if necessary...
      if (needsAlignment (classField->type)) {
        padding = (3 - ((nextOffset+3) % 4));
        if ((classField->offset >= 0) && (padding != 0)) {
          printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
          nextOffset += padding;
        }
      }
      printf ("        Offset %d:\t%d\t",
        classField->offset,
        sizeInBytesOfWhole (classField->type, classField, 0));
      printString (stdout, classField->id);
      // printf (": ");
      // pretty (classField->type);
      printf ("\n");
      incr = sizeInBytesOfWhole (classField->type, classField, 0);
      nextOffset = nextOffset + incr;
    }
    padding = (3 - ((nextOffset+3) % 4));
    if (padding != 0) {
      printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
    }
    printf ("            Total size of object = %d bytes\n", cl->sizeInBytes);
  }

  // If we have not already done this class...
  if (cl->sizeInBytes != -1) return;

  // printf ("==========  Assigning offsets to fields in class ");
  // printString (stdout, cl->id);
  // printf ("...  ==========\n");

  // Initialize "nextOffset"...
  nextOffset = STARTING_CLASS_OFFSET;

  // Look at each field...
  for (classField = cl->fields;
       classField != NULL;
       classField = (ClassField *) classField->next) {

    // Insert padding bytes, if necessary...
    if (needsAlignment (classField->type)) {
      padding = (3 - ((nextOffset+3) % 4));
      // if (padding != 0) {
      //   printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
      // }
      nextOffset += padding;
    }

    // Determine size, set "offset", and increment "nextOffset"...
    classField->offset = nextOffset;
    incr = sizeInBytesOfWhole (classField->type, classField, wantPrinting);
    classField->sizeInBytes = incr;
    if (incr < 0) {
      nextOffset = -1;
      break;
    }
    nextOffset = nextOffset + incr;

    // printf ("        Offset %d:\t%d\t", classField->offset, incr);
    // printString (stdout, classField->id);
    // printf ("\n");
    
  }

  // Fill in the size of this class...
  if (nextOffset >= 0) {
    padding = (3 - ((nextOffset+3) % 4));
    // if (padding != 0) {
    //   printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
    // }
    nextOffset += padding;
    // printf ("      Success - total size in bytes = %d\n", nextOffset);
    cl->sizeInBytes = nextOffset;
    changed = 1;
  } else {
    // printf ("      Failure!\n");
  }

}



// assignOffsetsInRecord (recordType, wantPrinting)
//
// This routine attempts to assign offsets in a record and sets 'changed' if
// anything was changed.
//
void assignOffsetsInRecord (RecordType * recordType, int wantPrinting) {
  int nextOffset, padding, incr;
  RecordField * recordField;
  AstNode * def;

// We start numbering record fields at 4.
#define STARTING_RECORD_OFFSET 0

  if (wantPrinting && (recordType->sizeInBytes < 0)) {
    error (recordType, "I am unable to determine the size of this record");
  }

  // Print the record layout, if desired...
  if (commandOptionS && wantPrinting) {
    printf ("==========  Record Type on line %d in file %s  ==========\n",
                     extractLineNumber (recordType->tokn),
                     extractFilename (recordType->tokn));
    printf ("    Fields:\n");

    nextOffset = STARTING_RECORD_OFFSET;
    for (recordField = recordType->fields;
         recordField != NULL;
         recordField = (RecordField *) recordField->next) {

      // Display padding bytes, if necessary...
      if (needsAlignment (recordField->type)) {
        padding = (3 - ((nextOffset+3) % 4));
        if ((recordField->offset >= 0) && (padding != 0)) {
          printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
          nextOffset += padding;
        }
      }
      printf ("        Offset %d:\t%d\t",
              recordField->offset,
              sizeInBytesOfWhole (recordField->type, recordField, 0));
      printString (stdout, recordField->id);
      // printf (": ");
      // pretty (recordField->type);
      printf ("\n");
      incr = sizeInBytesOfWhole (recordField->type, recordField, 0);
      nextOffset = nextOffset + incr;
    }
    padding = (3 - ((nextOffset+3) % 4));
    if (padding != 0) {
      printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
    }
    printf ("            Total size of record = %d bytes\n", recordType->sizeInBytes);
  }

  // If this is the last time through and we have not already created the field
  // mapping, then do so...
  if (wantPrinting && (recordType->fieldMapping == NULL)) {
    recordType->fieldMapping = new Mapping<String, RecordField> (5, NULL);
    for (recordField = recordType->fields;
         recordField != NULL;
         recordField = (RecordField *) recordField->next) {
      def = recordType->fieldMapping->find (recordField->id);
      if (def) {
        error (recordField, "Each field name must be different");
      } else {
        recordType->fieldMapping->enter (recordField->id, recordField);
      }
    }
  }

  // Initialize "nextOffset"...
  nextOffset = STARTING_RECORD_OFFSET;

  // If we have not already done this record...
  if (recordType->sizeInBytes != -1) return;

  // printf ("==========  Assigning offsets to fields in RecordType...  ==========\n");

  // Look at each field...
  for (recordField = recordType->fields;
       recordField != NULL;
       recordField = (RecordField *) recordField->next) {

    // Insert padding bytes, if necessary...
    if (needsAlignment (recordField->type)) {
      padding = (3 - ((nextOffset+3) % 4));
      // if (padding != 0) {
      //   printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
      // }
      nextOffset += padding;
    } else {
      padding = 0;
    }

    // Determine size, set "offset", and increment "nextOffset"...
    recordField->offset = nextOffset;
    incr = sizeInBytesOfWhole (recordField->type, recordField, wantPrinting);
    recordField->sizeInBytes = incr;
    if (incr < 0) {
      nextOffset = -1;
      break;
    }
    nextOffset = nextOffset + incr;

    // printf ("        Offset %d:\t%d\t%s\n", recordField->offset, incr,
    //                                     recordField->id->chars);
    
  }

  // Fill in the size of this record.
  if (nextOffset >= 0) {
    padding = (3 - ((nextOffset+3) % 4));
    // if (padding != 0) {
    //   printf ("        Offset %d:\t%d\t...padding...\n", nextOffset, padding);
    // }
    nextOffset += padding;
    // printf ("      Success - total size in bytes = %d\n", nextOffset);
    recordType->sizeInBytes = nextOffset;
    changed = 1;
  } else {
    // printf ("      Failure!\n");
  }
}



// assignOffsetsInParmList (parmList, wantPrinting, startingOffset) --> int
//
// This routine attempts to assign offsets to the parameters in a list
// and sets 'changed' if anything was changed.
//
// It returns the total size of the parameters, including padding.
//
int assignOffsetsInParmList (Parameter * parmList,
                              int wantPrinting,
                              int startingOffset) {
  int nextOffset, padding, incr;
  Parameter * parm;

  // printf ("==========  Assigning offsets to parameters...  ==========\n");

  // We start numbering parameters at the 'startingOffset'...
  nextOffset = startingOffset;

  // Look at each field...
  for (parm = parmList;
       parm != NULL;
       parm = (Parameter *) parm->next) {

    // Insert padding bytes, if necessary...
    if (needsAlignment (parm->type)) {
      padding = (3 - ((nextOffset+3) % 4));
      // if (commandOptionS && wantPrinting && padding != 0) {
      //   printf ("    Offset %d:\t%d\t...padding...\n", nextOffset, padding);
      // }
      nextOffset += padding;
    } else {
      padding = 0;
    }

    // Determine size, set "offset", and increment "nextOffset"...
    incr = sizeInBytesOfWhole (parm->type, parm, wantPrinting);
    if (parm->offset != nextOffset) {
      if (incr < 0) {
        if (wantPrinting) {   // Need this message for cyclic problems...
          error (parm, "Unable to determine the size of this parameter");
        }
        nextOffset = -1;
        break;
      }
      parm->offset = nextOffset;
      parm->sizeInBytes = incr;
      changed = 1;
    }

    // if (commandOptionS && wantPrinting) {
    //   printf ("    Offset %d:\t%d\t%s\n", parm->offset, incr,
    //                                       parm->id->chars);
    // }
    nextOffset = nextOffset + incr;
    
  }

  // Figure out the padding (if necessary)...
  if (nextOffset >= 0) {
    padding = (3 - ((nextOffset+3) % 4));
    // if (commandOptionS && wantPrinting && padding != 0) {
    //   printf ("    Offset %d:\t%d\t...padding...\n", nextOffset, padding);
    // }
    nextOffset += padding;
    // if (commandOptionS && wantPrinting) {
    //   printf ("      Success - total size in bytes = %d\n", nextOffset);
    // }
    // parmList->sizeInBytes = nextOffset;
  } else {
    // printf ("      Failure!\n");
  }
  return nextOffset - startingOffset;
}



// setParmSizeInFunctionType (fType)
//
// This routine computes the total size of all of the parameters and the
// size of the return value.  It then fills in "totalParmSize" in the
// FunctionType with whichever is greater.  Later, this will become the
// amount of space needed to call this type of function.
//
// If problems arise, this routine will print errors.
//
void setParmSizeInFunctionType (FunctionType * fType) {
  TypeArg * parm;
  int nextOffset = 0;
  int padding, incr;

// We start numbering function parameters at offset 8.
#define STARTING_FUNCTION_OFFSET 8

  // If we have already done the work, return immediately...
  if (fType->totalParmSize >= 0) return;

  // printf ("==========  Setting totalParmSize in FunctionType: ");
  // pretty (fType);

  // Look at each parameter...
  for (parm = fType->parmTypes;
       parm != NULL;
       parm = parm->next) {

    // Insert padding bytes, if necessary...
    if (needsAlignment (parm->type)) {
      padding = (3 - ((nextOffset+3) % 4));
      // if (padding != 0) {
      //   printf ("    Offset %d:\t%d\t...padding...\n", nextOffset, padding);
      // }
      nextOffset += padding;
    }

    // Determine the size of this parameter...
    incr = sizeInBytesOfWhole (parm->type, parm, 1);    // wantPrinting = 1
    if (incr < 0) {
      // printf ("    Unable to determine the size of next parameter!\n");
      return;
    }
    // printf ("    Offset %d:\t%d\t", nextOffset, incr);
    // pretty (parm->type);

    // Set the offset and sizeInBytes
    parm->offset = nextOffset + STARTING_FUNCTION_OFFSET;
    parm->sizeInBytes = incr;

    // Increment the running sum...
    nextOffset = nextOffset + incr;
    
  }

  // Figure out the padding (if necessary)...
  padding = (3 - ((nextOffset+3) % 4));
  // if (padding != 0) {
  //   printf ("    Offset %d:\t%d\t...padding...\n", nextOffset, padding);
  // }
  nextOffset += padding;

  // printf ("      Success - total size in bytes = %d\n", nextOffset);

  // Figure out the size of the return value...
  if (isVoidType (fType->retType)) {
    incr = 0;
    fType->retSize = -1;
  } else {
    incr = sizeInBytesOfWhole (fType->retType, fType->retType, 1);    // wantPrinting = 1
    fType->retSize = incr;
    if (incr < 0) {
      // printf ("    Unable to determine the size of return value!\n");
      // printf ("              incr = %d\n", incr);
      incr = 0;
    } else {
      // printf ("    RetType: size = %d   type = ", incr);
      // pretty (fType->retType);
      if (incr < 4) {
        incr = 4;
      }
    }
  }

  // Set "totalParmSize" to whichever is greater...
  if (nextOffset > incr) {
    fType->totalParmSize = nextOffset;
  } else {
    fType->totalParmSize = incr;
  }
  // printf ("==========  TotalParmSize = %d\n", fType->totalParmSize);

  return;
}



// needsAlignment (type) --> boolean
//
// This routine returns true if variables of this type will need to
// be word aligned; and false otherwise.
//
int needsAlignment (Type * type) {
  AstNode * node;
  node = getTypeDef (type);
  if (node == NULL) return 0;
  switch (node->op) {
    case CHAR_TYPE:
      return 0;
    case INT_TYPE:
      return 1;
    case DOUBLE_TYPE:
      return 1;
    case BOOL_TYPE:
      return 0;
    case VOID_TYPE:
      return 0;
    case TYPE_OF_NULL_TYPE:
      return 1;
    case ANY_TYPE:
      return 1;
    case PTR_TYPE:
      return 1;
    case ARRAY_TYPE:
      return 1;
    case RECORD_TYPE:
      return 1;
    case FUNCTION_TYPE:
      return 1;
    case CLASS_DEF:
      return 1;
    case INTERFACE:
      return 1;
    case TYPE_PARM:
      return 1;
    default:
      printf ("node->op = %s\n", symbolName (node->op));
      programLogicError ("Unknown op in needsAlignment");
      return 0;
  }
}



// getTypeDef (type) --> node
//
// This routine handles cases where the type is a NamedType, which will name
//        TypeDef
//        ClassDef
//        Interface
//        TypeParm
// This routine returns a ptr to the underlying definition, i.e., to a...
//        xxxx_TYPE
//        CLASS_DEF
//        INTERFACE
//        TYPE_PARM
//
// If it names a TypeDef, e.g., "type T = ptr to int", we return the underlying type.
// If it names a ClassDef or Interface, we return a ptr to the definition.
// It it names a TypeParm; then we return a ptr to the TYPE_PARM node itself, not the
// constraint type.
//
// This routine returns NULL if problems occur.
//
AstNode * getTypeDef (Type * type) {
  AstNode * def;

  if (type == NULL) return NULL;
  if (type->op != NAMED_TYPE) return type;
  def = ((NamedType *) type)->myDef;
  if (def == NULL) return NULL;

  // printf ("getTypeDef called on: %s...\n", ((NamedType *) type)->id->chars);
  // if (((NamedType *) type)->id == lookupAndAdd ("SomeTypeName", ID)) {
  //   printf ("--------  GOT IT  ----------\n");
  // }

  switch (def->op) {
    case INTERFACE:
    case CLASS_DEF:
    case TYPE_PARM:
      return def;
    case TYPE_DEF:
      return getTypeDef (((TypeDef *) def)->type);
    default:
      printf ("\ndef->op = %s\n", symbolName (def->op));
      programLogicError ("Unexpected op in getTypeDef");
  }
}



// sizeInBytesOfWhole (type, errNode, wantPrinting) --> int
//
// This routine returns the size of objects of the given type.  It returns -1
// if there is a problem or the size cannot be determined.
//
// If "wantPrinting" is true, then we will also print an error message
// if we run into problems.
//
// When passed a NULL, it indicates that previous errors have occured; this
// routine will simply return -1 without printing anything.
//
// This routine sets 'changed' if anything was changed.
//
int sizeInBytesOfWhole (Type * type, AstNode * errNode, int wantPrinting) {
  AstNode * node;
  ArrayType * arrayType;
  int elementSize, size, padding;
  IntConst * intConst;

  // printf ("sizeInBytesOfWhole... ");
  // pretty (type);

  node = getTypeDef (type);
  if (node == NULL) return -1;

  switch (node->op) {

    case CHAR_TYPE:
      return 1;

    case INT_TYPE:
      return 4;

    case DOUBLE_TYPE:
      return 8;

    case BOOL_TYPE:
      return 1;

    case VOID_TYPE:
      if (wantPrinting) {
        error (errNode, "You may not have a variable with type 'void' since it has no specific size (Perhaps you can use a 'ptr to void'...)");
      }
      return -1;

    case TYPE_OF_NULL_TYPE:
      return 4;

    case ANY_TYPE:
      if (wantPrinting) {
        error (errNode, "You may not have a variable with type 'anyType' since it has no specific size (Perhaps you can use a 'ptr to anyType'...)");
      }
      return -1;

    case PTR_TYPE:
      return 4;

    case ARRAY_TYPE:
      arrayType = (ArrayType *) node;
      // printf ("sizeInBytesOfWhole/ARRAY_TYPE...\n");
      // See if we've already done the work...
      if (arrayType->sizeInBytes >= 0) {
        return arrayType->sizeInBytes;
      }
      if (recursionCounter++ > 100) {
        error (arrayType, "This type appears to be recursively defined");
        recursionCounter--;
        arrayType->sizeInBytes = 100;   // Save an arbitrary size to suppress other errors
        return -1;
      }
      elementSize = sizeInBytesOfWhole (arrayType->baseType, errNode, wantPrinting);
      recursionCounter--;
      if (elementSize < 0) {
        if (wantPrinting) {
          error (errNode, "I am unable to determine the size of this variable (since I am unable to determine the size of the array elements)");
        }
        return -1;
      }
      if (arrayType->sizeExpr == NULL) {
        if (wantPrinting) {
          error (errNode, "I am unable to determine the size of this variable since the number of array elements is unspecified; use something like 'array [100] of ...'");
        }
        return -1;
      }
      intConst = (IntConst *) arrayType->sizeExpr;
      if (intConst->op != INT_CONST) {
        if (wantPrinting) {
          error (errNode, "I am unable to determine the size of this variable since the number of array elements cannot be determined at compile time; try using a constant integer value");
        }
        return -1;
      }
      size = intConst->ivalue * elementSize + 4;   // 4 bytes for "number of elements"
      if (intConst->ivalue <= 0) {
        if (wantPrinting) {
          error (errNode, "The number of array elements must be greater than zero");
        }
        return -1;
      }
      if ((((double) (intConst->ivalue)) * ((double) elementSize)) + 4.0 > 2147483647.0) {
        if (wantPrinting) {
          error (errNode, "The size of this array exceeds 2147483647 bytes");
        }
        return -1;
      }
      // Pad to a multiple of 4...
      padding = (3 - ((size+3) % 4));
      size += padding;
      // printf ("  Array basicSize = %d, padding = %d, finalSize= %d\n",
      //           size-padding, padding, size);
      arrayType->sizeInBytes = size;
      arrayType->sizeOfElements = elementSize;
      changed = 1;
      return size;

    case RECORD_TYPE:
      if (wantPrinting && ((RecordType *) node)->sizeInBytes == -1) {
        error (errNode, "I am unable to determine the size of this variable (since I am unable to determine the size of its RecordType)");
      }
      return ((RecordType *) node)->sizeInBytes;

    case FUNCTION_TYPE:
      if (wantPrinting) {
        error (errNode, "You may not have a variable with type 'function' since it has no specific size (Perhaps you can use a 'ptr to function'...)");
      }
      return -1;

    case CLASS_DEF:
      if (wantPrinting && ((ClassDef *) node)->sizeInBytes == -1) {
        error (errNode, "I am unable to determine the size of this variable (since I am unable to determine the size of its Class type)");
      }
      return ((ClassDef *) node)->sizeInBytes;

    case INTERFACE:
      if (wantPrinting) {
        error (errNode, "You may not have a variable whose type is an interface since I cannot determine the size of the object (Perhaps you can use a ptr to it...)");
      }
      return -1;

    case TYPE_PARM:
      if (((TypeParm *) node)->fourByteRestricted) {
        return 4;
      } else {
        if (wantPrinting) {
          error (node, "This Type Parameter is used in way that requires its size to be known, but it cannot be known at compile-time");
          error2 (errNode, "Here is the place where the problem was noticed");
        }
        return -1;
      }

    default:
      printf ("node->op = %s\n", symbolName (node->op));
      programLogicError ("Unknown op in sizeInBytesOfWhole");
      return -1;
  }
}



// evalExprsIn (node) --> node
//
// This routine walks the entire program.  It evaluates several statically
// evaluable expressions, replacing some branches in the tree with
// new branches indicating the reduced, simplified values.  If any simplication
// is made, it sets "changed".
//
// When passed an expression, this routine returns the (possibly revised) expression
// subtree.  Otherwise, it returns the node itself.
//
// This routine performs the following expression evaluations:
//
//    bool == bool
//    bool != bool
//    bool & bool    - logical and
//    bool | bool    - logical or
//    bool ^ bool    - logical xor  (same as "bool != bool")
//    ! bool         - logical not
//
//    - int
//    int + int
//    int - int
//    int * int
//    int / int
//    int % int
//    int == int
//    int != int
//    int <  int
//    int <= int
//    int >  int
//    int >= int
//    int && int   - bitwise and
//    int || int   - bitwise or
//    int ^^ int   - bitwise xor
//    ! int        - bitwise negation
//    int << int   - shift left logical
//    int >> int   - shift right arithmetic
//    int >>> int  - shift right logical
//
//    0 == int    --> intIsZero (int)
//    0 != int    --> intNotZero (int)
//    int == 0    --> intIsZero (int)
//    int != 0    --> intNotZero (int)
//
//    -double
//    double+double
//    double-double
//    double*double
//    double/double
//
//    charToInt (char)
//    intToChar (int)
//    intToDouble (int)
//    doubleToInt (double)
//    negInf ()
//    posInf ()
//    negZero ()
//
//    sizeOf <TYPE>
//
// More compile-time evaluation may be added, if and when they become important.
//
// This routine also scans for uses of variables that have been declared as constants
// and substitutes their initializing expressions in place of the variable.
//
// Note that this routine will be called repeatedly.  It may print errors, but it
// should return a modified tree that will not result in further error messages when
// this routine again visits that same portion of the tree.
// 
AstNode * evalExprsIn (AstNode * node) {
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
    AstNode * x;
    int i, j, overflow;
    double r, d, d2;

  if (node == NULL) return node;

  // printf ("%s...\n", symbolName (node->op));

  switch (node->op) {

    case HEADER:
      header = (Header *) node;
      // printf ("  %s\n", header->packageName->chars);
      x = evalExprsIn (header->uses);
      x = evalExprsIn (header->consts);
      x = evalExprsIn (header->errors);
      x = evalExprsIn (header->globals);
      x = evalExprsIn (header->typeDefs);
      x = evalExprsIn (header->functionProtos);
      x = evalExprsIn (header->functions);
      x = evalExprsIn (header->closures);
      x = evalExprsIn (header->interfaces);
      x = evalExprsIn (header->classes);
      // x = evalExprsIn (header->next);
      return node;

//    case CODE:
//      code = (Code *) node;
//      x = evalExprsIn (code->consts);
//      x = evalExprsIn (code->globals);
//      x = evalExprsIn (code->typeDefs);
//      x = evalExprsIn (code->functions);
//      x = evalExprsIn (code->interfaces);
//      x = evalExprsIn (code->classes);
//      x = evalExprsIn (code->behaviors);
//      return node;

    case USES:
      uses = (Uses *) node;
      x = evalExprsIn (uses->renamings);
      x = evalExprsIn (uses->next);
      return node;

    case RENAMING:
      renaming = (Renaming *) node;
      x = evalExprsIn (renaming->next);
      return node;

    case INTERFACE:
      interface = (Interface *) node;
      // printf ("  %s\n", interface->id->chars);
      x = evalExprsIn (interface->typeParms);
      x = evalExprsIn (interface->extends);
      x = evalExprsIn (interface->methodProtos);
      x = evalExprsIn (interface->next);
      return node;

    case CLASS_DEF:
      cl = (ClassDef *) node;
      // printf ("  %s\n", cl->id->chars);
      x = evalExprsIn (cl->typeParms);
      x = evalExprsIn (cl->implements);
      x = evalExprsIn (cl->superclass);
      x = evalExprsIn (cl->fields);
      x = evalExprsIn (cl->methodProtos);
      x = evalExprsIn (cl->methods);
      x = evalExprsIn (cl->next);
      return node;

//    case BEHAVIOR:
//      behavior = (Behavior *) node;
//      printf ("  %s\n", behavior->id->chars);
//      x = evalExprsIn (behavior->methods);
//      x = evalExprsIn (behavior->next);
//      return node;

    case TYPE_DEF:
      typeDef = (TypeDef *) node;
      // printf ("  %s\n", typeDef->id->chars);
      x = evalExprsIn (typeDef->type);
      x = evalExprsIn (typeDef->next);
      return node;

    case CONST_DECL:
      constDecl = (ConstDecl *) node;
      // printf ("  %s\n", constDecl->id->chars);
      constDecl->expr = (Expression *) evalExprsIn (constDecl->expr);
      x = evalExprsIn (constDecl->next);
      return node;

    case ERROR_DECL:
      errorDecl = (ErrorDecl *) node;
      // printf ("  %s\n", errorDecl->id->chars);
      x = evalExprsIn (errorDecl->parmList);
      x = evalExprsIn (errorDecl->next);
      return node;

    case FUNCTION_PROTO:
      functionProto = (FunctionProto *) node;
      // printf ("  %s\n", functionProto->id->chars);
      x = evalExprsIn (functionProto->parmList);
      x = evalExprsIn (functionProto->retType);
      x = evalExprsIn (functionProto->next);
      return node;

    case FUNCTION:
      fun = (Function *) node;
      // printf ("  %s\n", fun->id->chars);
      x = evalExprsIn (fun->parmList);
      x = evalExprsIn (fun->retType);
      x = evalExprsIn (fun->locals);
      x = evalExprsIn (fun->stmts);
      x = evalExprsIn (fun->next);
      return node;

    case METHOD_PROTO:
      methodProto = (MethodProto *) node;
      // printf ("  %s\n", methodProto->selector->chars);
      x = evalExprsIn (methodProto->parmList);
      x = evalExprsIn (methodProto->retType);
      x = evalExprsIn (methodProto->next);
      return node;

    case METHOD:
      meth = (Method *) node;
      // printf ("  %s\n", meth->selector->chars);
      x = evalExprsIn (meth->parmList);
      x = evalExprsIn (meth->retType);
      x = evalExprsIn (meth->locals);
      x = evalExprsIn (meth->stmts);
      x = evalExprsIn (meth->next);
      return node;

    case TYPE_PARM:
      typeParm = (TypeParm *) node;
      x = evalExprsIn (typeParm->type);
      x = evalExprsIn (typeParm->next);
      return node;

    case TYPE_ARG:
      typeArg = (TypeArg *) node;
      x = evalExprsIn (typeArg->type);
      x = evalExprsIn (typeArg->next);
      return node;

    case CHAR_TYPE:
      charType = (CharType *) node;
      return node;

    case INT_TYPE:
      intType = (IntType *) node;
      return node;

    case DOUBLE_TYPE:
      doubleType = (DoubleType *) node;
      return node;

    case BOOL_TYPE:
      boolType = (BoolType *) node;
      return node;

    case VOID_TYPE:
      voidType = (VoidType *) node;
      return node;

    case TYPE_OF_NULL_TYPE:
      typeOfNullType = (TypeOfNullType *) node;
      return node;

    case ANY_TYPE:
      anyType = (AnyType *) node;
      return node;

    case PTR_TYPE:
      pType = (PtrType *) node;
      x = evalExprsIn (pType->baseType);
      return node;

    case ARRAY_TYPE:
      aType = (ArrayType *) node;
      aType->sizeExpr = (Expression *) evalExprsIn (aType->sizeExpr);
      x = evalExprsIn (aType->baseType);
      return node;

    case RECORD_TYPE:
      rType = (RecordType *) node;
      x = evalExprsIn (rType->fields);
      return node;

    case FUNCTION_TYPE:
      fType = (FunctionType *) node;
      x = evalExprsIn (fType->parmTypes);
      x = evalExprsIn (fType->retType);
      return node;

    case NAMED_TYPE:
      nType = (NamedType *) node;
      x = evalExprsIn (nType->typeArgs);
      return node;

    case IF_STMT:
      ifStmt = (IfStmt *) node;
      ifStmt->expr = (Expression *) evalExprsIn (ifStmt->expr);
      x = evalExprsIn (ifStmt->thenStmts);
      x = evalExprsIn (ifStmt->elseStmts);
      x = evalExprsIn (ifStmt->next);
      return node;

    case ASSIGN_STMT:
      assignStmt = (AssignStmt *) node;
      assignStmt->lvalue = (Expression *) evalExprsIn (assignStmt->lvalue);
      assignStmt->expr = (Expression *) evalExprsIn (assignStmt->expr);
      x = evalExprsIn (assignStmt->next);
      return node;

    case CALL_STMT:
      callStmt = (CallStmt *) node;
      callStmt->expr = (CallExpr *) evalExprsIn (callStmt->expr);
      if (callStmt->expr && (callStmt->expr->op != CALL_EXPR)) {
        error (node, "This built-in function returns a value; it may not be used as a statement");
        callStmt->expr = NULL;
      }
      x = evalExprsIn (callStmt->next);
      return node;

    case SEND_STMT:
      sendStmt = (SendStmt *) node;
      sendStmt->expr = (SendExpr *) evalExprsIn (sendStmt->expr);
      // This next test is theoretically unnecessary, since evalExprs will only
      // modify infix and prefix messages (unless others primitive methods are added
      // later).  Since infix and prefix messages cannot appears at the statement
      // level (else syntax error), the call to evalExprs should never modify
      // the sendStmt node.  Nevertheless, the test is left in since we may later
      // add primitive methods which would get simplified in evalExpr...
      if (sendStmt->expr && (sendStmt->expr->op != SEND_EXPR)) {
        error (node, "This built-in method returns a value; it may not be used as a statement");
        sendStmt->expr = NULL;
      }
      x = evalExprsIn (sendStmt->next);
      return node;

    case WHILE_STMT:
      whileStmt = (WhileStmt *) node;
      whileStmt->expr = (Expression *) evalExprsIn (whileStmt->expr);
      x = evalExprsIn (whileStmt->stmts);
      x = evalExprsIn (whileStmt->next);
      return node;

    case DO_STMT:
      doStmt = (DoStmt *) node;
      x = evalExprsIn (doStmt->stmts);
      doStmt->expr = (Expression *) evalExprsIn (doStmt->expr);
      x = evalExprsIn (doStmt->next);
      return node;

    case BREAK_STMT:
      breakStmt = (BreakStmt *) node;
      x = evalExprsIn (breakStmt->next);
      return node;

    case CONTINUE_STMT:
      continueStmt = (ContinueStmt *) node;
      x = evalExprsIn (continueStmt->next);
      return node;

    case RETURN_STMT:
      returnStmt = (ReturnStmt *) node;
      returnStmt->expr = (Expression *) evalExprsIn (returnStmt->expr);
      x = evalExprsIn (returnStmt->next);
      return node;

    case FOR_STMT:
      forStmt = (ForStmt *) node;
      forStmt->lvalue = (Expression *) evalExprsIn (forStmt->lvalue);
      forStmt->expr1 = (Expression *) evalExprsIn (forStmt->expr1);
      forStmt->expr2 = (Expression *) evalExprsIn (forStmt->expr2);
      forStmt->expr3 = (Expression *) evalExprsIn (forStmt->expr3);
      x = evalExprsIn (forStmt->stmts);
      x = evalExprsIn (forStmt->next);
      return node;

    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) node;
      switchStmt->expr = (Expression *) evalExprsIn (switchStmt->expr);
      x = evalExprsIn (switchStmt->caseList);
      x = evalExprsIn (switchStmt->defaultStmts);
      x = evalExprsIn (switchStmt->next);
      return node;

    case TRY_STMT:
      tryStmt = (TryStmt *) node;
      x = evalExprsIn (tryStmt->stmts);
      x = evalExprsIn (tryStmt->catchList);
      x = evalExprsIn (tryStmt->next);
      return node;

    case THROW_STMT:
      throwStmt = (ThrowStmt *) node;
      x = evalExprsIn (throwStmt->argList);
      x = evalExprsIn (throwStmt->next);
      return node;

    case FREE_STMT:
      freeStmt = (FreeStmt *) node;
      freeStmt->expr = (Expression *) evalExprsIn (freeStmt->expr);
      x = evalExprsIn (freeStmt->next);
      return node;

    case DEBUG_STMT:
      debugStmt = (DebugStmt *) node;
      x = evalExprsIn (debugStmt->next);
      return node;

    case CASE:
      cas = (Case *) node;
      cas->expr = (Expression *) evalExprsIn (cas->expr);
      x = evalExprsIn (cas->stmts);
      x = evalExprsIn (cas->next);
      return node;

    case CATCH:
      cat = (Catch *) node;
      x = evalExprsIn (cat->parmList);
      x = evalExprsIn (cat->stmts);
      x = evalExprsIn (cat->next);
      return node;

    case GLOBAL:
      global = (Global *) node;
      x = evalExprsIn (global->type);
      global->initExpr = (Expression *) evalExprsIn (global->initExpr);
      x = evalExprsIn (global->next);
      return node;

    case LOCAL:
      local = (Local *) node;
      x = evalExprsIn (local->type);
      local->initExpr = (Expression *) evalExprsIn (local->initExpr);
      x = evalExprsIn (local->next);
      return node;

    case PARAMETER:
      parm = (Parameter *) node;
      x = evalExprsIn (parm->type);
      x = evalExprsIn (parm->next);
      return node;

    case CLASS_FIELD:
      classField = (ClassField *) node;
      x = evalExprsIn (classField->type);
      x = evalExprsIn (classField->next);
      return node;

    case RECORD_FIELD:
      recordField = (RecordField *) node;
      x = evalExprsIn (recordField->type);
      x = evalExprsIn (recordField->next);
      return node;

    case INT_CONST:
      intConst = (IntConst *) node;
      return node;

    case DOUBLE_CONST:
      doubleConst = (DoubleConst *) node;
      return node;

    case CHAR_CONST:
      charConst = (CharConst *) node;
      return node;

    case STRING_CONST:
      stringConst = (StringConst *) node;
      return node;

    case BOOL_CONST:
      boolConst = (BoolConst *) node;
      return node;

    case NULL_CONST:
      nullConst = (NullConst *) node;
      return node;

    case CALL_EXPR:
      callExpr = (CallExpr *) node;
      x = evalExprsIn (callExpr->argList);

      // See if we are dealing with a known function, such as "charToInt"...
      switch (callExpr->id->primitiveSymbol) {

       case CHAR_TO_INT:
          if ((argCount (callExpr->argList) == 1) &&
              (isCharConst (callExpr->argList->expr))) {
            i = ((CharConst *) callExpr->argList->expr)->ivalue;
            intConst = new IntConst ();
            intConst->positionAt (node);
            intConst->ivalue = i;
            changed = 1;
            return intConst;
          }
	  break;

       case INT_TO_CHAR:
          if ((argCount (callExpr->argList) == 1) &&
              (isIntConst (callExpr->argList->expr))) {
            i = ((IntConst *) callExpr->argList->expr)->ivalue;
            if (i<-128 || i>255) {
              error (node, "During the compile-time evaluation of intToChar, the argument was not between -128 and 255");
            }
            charConst = new CharConst ();
            charConst->positionAt (node);
            charConst->ivalue = i & 0x000000ff;
            changed = 1;
            return charConst;
          }
	  break;

       case INT_TO_DOUBLE:
          if ((argCount (callExpr->argList) == 1) &&
              (isIntConst (callExpr->argList->expr))) {
            i = ((IntConst *) callExpr->argList->expr)->ivalue;
            doubleConst = new DoubleConst ();
            doubleConst->positionAt (node);
            doubleConst->rvalue = (double) i;
            linkDouble (doubleConst);
            changed = 1;
            return doubleConst;
          }
	  break;

       case DOUBLE_TO_INT:
          if ((argCount (callExpr->argList) == 1) &&
              (isDoubleConst (callExpr->argList->expr))) {
            r = ((DoubleConst *) callExpr->argList->expr)->rvalue;
            intConst = new IntConst ();
            intConst->positionAt (node);
            intConst->ivalue = truncateToInt (r);
            changed = 1;
            return intConst;
          }
	  break;

       case POS_INF:
          if (argCount (callExpr->argList) != 0) {
            error (callExpr, "Primitive POS_INF expects no arguments");
          }
          doubleConst = new DoubleConst ();
          doubleConst->positionAt (node);
          r = 0.0;
          doubleConst->rvalue = 1.0 / r;
          linkDouble (doubleConst);
          changed = 1;
          return doubleConst;


       case NEG_INF:
          if (argCount (callExpr->argList) != 0) {
            error (callExpr, "Primitive NEG_INF expects no arguments");
          }
          doubleConst = new DoubleConst ();
          doubleConst->positionAt (node);
          r = 0.0;
          doubleConst->rvalue = -1.0 / r;
          linkDouble (doubleConst);
          changed = 1;
          return doubleConst;


       case NEG_ZERO:
          if (argCount (callExpr->argList) != 0) {
            error (callExpr, "Primitive NEG_ZERO expects no arguments");
          }
          doubleConst = new DoubleConst ();
          doubleConst->positionAt (node);
          r = 0.0;
          doubleConst->rvalue = -1.0 * r;
          linkDouble (doubleConst);
          changed = 1;
          return doubleConst;

       // The following cases are not handled here, since they are unlikely to occur
       // often and are more likely to be inserted by the compiler later.  By not
       // reducing them here, we make it easier to test them later.
       //
       //    case I_IS_ZERO:
       //    case I_NOT_ZERO:

      }
      return node;

    case SEND_EXPR:
      sendExpr = (SendExpr *) node;
      sendExpr->receiver = (Expression *) evalExprsIn (sendExpr->receiver);
      x = evalExprsIn (sendExpr->argList);

      // See if we are dealing with a known message, such as "+"...
      switch (sendExpr->selector->primitiveSymbol) {

        case PLUS:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER ADDITION DETECTED... i = %d   j = %d\n", i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i + j;
              // Check for overflow...
              if ((i<0) && (j<0)) {
                overflow = ((i+j)>=0);
              } else if ((i>0) && (j>0)) {
                overflow = ((i+j)<=0);
              } else {
                overflow = 0;
              }
              if (overflow) {
                error (node,
                  "Overflow detected during compile-time evaluation of integer addition");
              }
              changed = 1;
              return intConst;
            }
          } else if (isDoubleConst (sendExpr->receiver)) {
            d = ((DoubleConst *) sendExpr->receiver)->rvalue;
            if (sendExpr->argList && isDoubleConst (sendExpr->argList->expr)) {
              d2 = ((DoubleConst *) sendExpr->argList->expr)->rvalue;
              // printf ("DOUBLE ADDITION DETECTED... d = %g   d2 = %g\n", d, d2);
              doubleConst = new DoubleConst ();
              doubleConst->positionAt (node);
              doubleConst->rvalue = d + d2;
              // Check for not-a-number; if result is +inf or -inf it will be okay
              if (isnan (doubleConst->rvalue)) {
                error (node, "During the compile-time evaluation of this double addition operation, the result was not-a-number");
              }
              linkDouble (doubleConst);
              changed = 1;
              return doubleConst;
            }
          }
	  break;

        case MINUS:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER SUBTRACTION DETECTED... i = %d   j = %d\n", i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i - j;
              // Check for overflow...
              if ((i>=0) && (j<0)) {
                overflow = ((i-j)<=0);
              } else if ((i<0) && (j>0)) {
                overflow = ((i-j)>=0);
              } else {
                overflow = 0;
              }
              if (overflow) {
                error (node,
                  "Overflow detected during compile-time evaluation of integer subtraction");
              }
              changed = 1;
              return intConst;
            }
          } else if (isDoubleConst (sendExpr->receiver)) {
            d = ((DoubleConst *) sendExpr->receiver)->rvalue;
            if (sendExpr->argList && isDoubleConst (sendExpr->argList->expr)) {
              d2 = ((DoubleConst *) sendExpr->argList->expr)->rvalue;
              // printf ("DOUBLE SUBTRACTION DETECTED... d = %g   d2 = %g\n", d, d2);
              doubleConst = new DoubleConst ();
              doubleConst->positionAt (node);
              doubleConst->rvalue = d - d2;
              // Check for not-a-number; if result is +inf or -inf it will be okay
              if (isnan (doubleConst->rvalue)) {
                error (node, "During the compile-time evaluation of this double subtraction operation, the result was not-a-number");
              }
              linkDouble (doubleConst);
              changed = 1;
              return doubleConst;
            }
          }
	  break;

        case UNARY_MINUS:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList == NULL) {
              // printf ("UNARY INT-NEGATION DETECTED... i = %d\n", i);
              intConst = new IntConst ();
              intConst->positionAt (node);
              // Check for overflow...
              if (i == 0x80000000) {
                error (node,
                  "Overflow detected during compile-time evaluation of -(0x80000000)");
                changed = 1;
                return sendExpr->receiver;
              }
              intConst->ivalue = - i;
              changed = 1;
              return intConst;
            }
          } else if (isDoubleConst (sendExpr->receiver)) {
            d = ((DoubleConst *) sendExpr->receiver)->rvalue;
            if (sendExpr->argList == NULL) {
              // printf ("UNARY DOUBLE-NEGATION DETECTED... d = %g\n", d);
              doubleConst = new DoubleConst ();
              doubleConst->positionAt (node);
              doubleConst->rvalue = - d;
              linkDouble (doubleConst);
              changed = 1;
              return doubleConst;
            }
          }
	  break;

        case STAR:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER MULTIPLICATION DETECTED... i = %d   j = %d\n", i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i * j;
              // Check for overflow...
              if ((((double) i) * ((double) j)) != ((double) (i*j))) {
                error (node,
                  "Overflow detected during compile-time evaluation of integer multiplication");
              }
              changed = 1;
              return intConst;
            }
          } else if (isDoubleConst (sendExpr->receiver)) {
            d = ((DoubleConst *) sendExpr->receiver)->rvalue;
            if (sendExpr->argList && isDoubleConst (sendExpr->argList->expr)) {
              d2 = ((DoubleConst *) sendExpr->argList->expr)->rvalue;
              // printf ("DOUBLE MULTIPLICATION DETECTED... d = %g   d2 = %g\n", d, d2);
              doubleConst = new DoubleConst ();
              doubleConst->positionAt (node);
              doubleConst->rvalue = d * d2;
              // Check for not-a-number; if result is +inf or -inf it will be okay
              if (isnan (doubleConst->rvalue)) {
                error (node, "During the compile-time evaluation of this double multiplication operation, the result was not-a-number");
              }
              linkDouble (doubleConst);
              changed = 1;
              return doubleConst;
            }
          }
	  break;

        case SLASH:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER DIVISION DETECTED... i = %d   j = %d\n", i, j);
              // Check for overflow...
              if (j == 0) {
                error (node, "During the compile-time evaluation of this integer division operation, the divisor was found to be zero");
              }
              if (i == 0x80000000 && j == -1) {
                error (node, "During the compile-time evaluation of this integer division operation, overflow occurred");
              }
              divide (i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = quo;
              changed = 1;
              return intConst;
            }
          } else if (isDoubleConst (sendExpr->receiver)) {
            d = ((DoubleConst *) sendExpr->receiver)->rvalue;
            if (sendExpr->argList && isDoubleConst (sendExpr->argList->expr)) {
              d2 = ((DoubleConst *) sendExpr->argList->expr)->rvalue;
              // printf ("DOUBLE DIVISION DETECTED... d = %g   d2 = %g\n", d, d2);
              doubleConst = new DoubleConst ();
              doubleConst->positionAt (node);
              doubleConst->rvalue = d / d2;
              // Check for not-a-number; if result is +inf or -inf it will be okay
              if (isnan (doubleConst->rvalue)) {
                error (node, "During the compile-time evaluation of this double division operation, the result was not-a-number");
              }
              linkDouble (doubleConst);
              changed = 1;
              return doubleConst;
            }
          }
	  break;

        case PERCENT:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER REMAINDER DETECTED... i = %d   j = %d\n", i, j);
              // Check for overflow...
              if (j == 0) {
                error (node, "During the compile-time evaluation of this integer remainder operation, the divisor was found to be zero");
              }
              if (i == 0x80000000 && j == -1) {
                error (node, "During the compile-time evaluation of this integer remainder operation, overflow occurred");
              }
              divide (i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = rem;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case UNARY_BANG:
          if (isBoolConst (sendExpr->receiver)) {
            i = ((BoolConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList == NULL) {
              // printf ("UNARY NOT DETECTED... i = %d\n", i);
              if (i) {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList == NULL) {
              // printf ("UNARY BIT-NEGATION DETECTED... i = %d\n", i);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = ~i;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case BAR_BAR:
          if (isBoolConst (sendExpr->receiver)) {
            i = ((BoolConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isBoolConst (sendExpr->argList->expr)) {
              j = ((BoolConst *) sendExpr->argList->expr)->ivalue;
              // printf ("LOGICAL-OR DETECTED... i = %d   j = %d\n", i, j);
              if (i || j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
	  break;

        case AMP_AMP:
          if (isBoolConst (sendExpr->receiver)) {
            i = ((BoolConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isBoolConst (sendExpr->argList->expr)) {
              j = ((BoolConst *) sendExpr->argList->expr)->ivalue;
              // printf ("LOGICAL-AND DETECTED... i = %d   j = %d\n", i, j);
              if (i && j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
	  break;

        case EQUAL_EQUAL:
          if (isBoolConst (sendExpr->receiver)) {
            i = ((BoolConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isBoolConst (sendExpr->argList->expr)) {
              j = ((BoolConst *) sendExpr->argList->expr)->ivalue;
              // printf ("LOGICAL-EQUAL DETECTED... i = %d   j = %d\n", i, j);
              if (i == j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER-EQUAL DETECTED... i = %d   j = %d\n", i, j);
              if (i == j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && i==0) {
              changed = 1;
              return insertIntIsZero (sendExpr->argList->expr);
            }
          }
          if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
            i = ((IntConst *) sendExpr->argList->expr)->ivalue;
            if (i==0) {
              changed = 1;
              return insertIntIsZero (sendExpr->receiver);
            }
          }
	  break;

        case NOT_EQUAL:
          if (isBoolConst (sendExpr->receiver)) {
            i = ((BoolConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isBoolConst (sendExpr->argList->expr)) {
              j = ((BoolConst *) sendExpr->argList->expr)->ivalue;
              // printf ("LOGICAL-NOT-EQUAL DETECTED... i = %d   j = %d\n", i, j);
              if (i != j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                 boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER-NOT-EQUAL DETECTED... i = %d   j = %d\n", i, j);
              if (i != j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && i==0) {
              changed = 1;
              return insertIntNotZero (sendExpr->argList->expr);
            }
          }
          if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
            i = ((IntConst *) sendExpr->argList->expr)->ivalue;
            if (i==0) {
              changed = 1;
              return insertIntNotZero (sendExpr->receiver);
            }
          }
	  break;

        case BAR:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER-BIT-OR DETECTED... i = %d   j = %d\n", i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i | j;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case AMP:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER-BIT-AND DETECTED... i = %d   j = %d\n", i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i & j;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case CARET:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              // printf ("INTEGER-BIT-XOR DETECTED... i = %d   j = %d\n", i, j);
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i ^ j;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case LESS:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (i < j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
	  break;

        case LESS_EQUAL:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (i <= j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
	  break;

        case GREATER:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (i > j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
	  break;

        case GREATER_EQUAL:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (i >= j) {
                boolConst = new BoolConst (1);
                boolConst->positionAt (node);
              } else {
                boolConst = new BoolConst (0);
                boolConst->positionAt (node);
              }
              changed = 1;
              return boolConst;
            }
          }
	  break;

        case LESS_LESS:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (j>31 || j<0) {
                error (node, "During the compile-time evaluation of shift-left-logical, the shift amount was not between 0 and 31");
              }
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i << j;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case GREATER_GREATER:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (j>31 || j<0) {
                error (node, "During the compile-time evaluation of shift-right-arithmetic, the shift amount was not between 0 and 31");
              }
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = i >> j;
              changed = 1;
              return intConst;
            }
          }
	  break;

        case GREATER_GREATER_GREATER:
          if (isIntConst (sendExpr->receiver)) {
            i = ((IntConst *) sendExpr->receiver)->ivalue;
            if (sendExpr->argList && isIntConst (sendExpr->argList->expr)) {
              j = ((IntConst *) sendExpr->argList->expr)->ivalue;
              if (j>31 || j<0) {
                error (node, "During the compile-time evaluation of shift-right-logical, the shift amount was not between 0 and 31");
              }
              intConst = new IntConst ();
              intConst->positionAt (node);
              intConst->ivalue = (signed int) (((unsigned int) i) >> j);
              changed = 1;
              return intConst;
            }
          }
	  break;

      }
      return node;

    case SELF_EXPR:
      selfExpr = (SelfExpr *) node;
      return node;

    case SUPER_EXPR:
      superExpr = (SuperExpr *) node;
      return node;

    case FIELD_ACCESS:
      fieldAccess = (FieldAccess *) node;
      fieldAccess->expr = (Expression *) evalExprsIn (fieldAccess->expr);
      return node;

    case ARRAY_ACCESS:
      arrayAccess = (ArrayAccess *) node;
      arrayAccess->arrayExpr = (Expression *) evalExprsIn (arrayAccess->arrayExpr);
      arrayAccess->indexExpr = (Expression *) evalExprsIn (arrayAccess->indexExpr);
      return node;

    case CONSTRUCTOR:
      constructor = (Constructor *) node;
      x = evalExprsIn (constructor->type);
      x = evalExprsIn (constructor->countValueList);
      x = evalExprsIn (constructor->fieldInits);
      return node;

    case CLOSURE_EXPR:
      closureExpr = (ClosureExpr *) node;
      // Skip this, since we'll x = evalExprsIn the function when we do header->closures...
      // x = evalExprsIn (closureExpr->function);
      return node;

    case VARIABLE_EXPR:
      var = (VariableExpr *) node;
      // If this variable names a constant, and that constant has a definition
      // which is a single value, then return that value...
      if (var->myDef) {
        constDecl = (ConstDecl *) var->myDef;
        if ((constDecl->op == CONST_DECL) && (constDecl->expr->op)) {
          if ((constDecl->expr->op == CHAR_CONST) ||
              (constDecl->expr->op == INT_CONST) ||
              (constDecl->expr->op == DOUBLE_CONST) ||
              (constDecl->expr->op == BOOL_CONST) ||
              (constDecl->expr->op == NULL_CONST) ||
              (constDecl->expr->op == STRING_CONST)) {
            changed = 1;
            return constDecl->expr;
          } else {
            // Otherwise, we must do nothing; perhaps it will be resolved on
            // a subsequent pass.
          }
        }
      }
      return node;

    case AS_PTR_TO_EXPR:
      asPtrToExpr = (AsPtrToExpr *) node;
      asPtrToExpr->expr = (Expression *) evalExprsIn (asPtrToExpr->expr);
      x = evalExprsIn (asPtrToExpr->type);
      return node;

    case AS_INTEGER_EXPR:
      asIntegerExpr = (AsIntegerExpr *) node;
      asIntegerExpr->expr = (Expression *) evalExprsIn (asIntegerExpr->expr);
      return node;

    case ARRAY_SIZE_EXPR:
      arraySizeExpr = (ArraySizeExpr *) node;
      arraySizeExpr->expr = (Expression *) evalExprsIn (arraySizeExpr->expr);
      return node;

    case IS_INSTANCE_OF_EXPR:
      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      isInstanceOfExpr->expr = (Expression *) evalExprsIn (isInstanceOfExpr->expr);
      x = evalExprsIn (isInstanceOfExpr->type);
      return node;

    case IS_KIND_OF_EXPR:
      isKindOfExpr = (IsKindOfExpr *) node;
      isKindOfExpr->expr = (Expression *) evalExprsIn (isKindOfExpr->expr);
      x = evalExprsIn (isKindOfExpr->type);
      return node;

    case SIZE_OF_EXPR:
      sizeOfExpr = (SizeOfExpr *) node;
      x = evalExprsIn (sizeOfExpr->type);
      i = sizeInBytesOfWhole (sizeOfExpr->type, node, 0);   // Want printing = 0
      // (If we have a problem, we'll print the error in checkTypes.)
      if (i >= 0) {
        intConst = new IntConst ();
        intConst->positionAt (node);
        intConst->ivalue = i;
        changed = 1;
        return intConst;
      }
      return node;

    case DYNAMIC_CHECK:
      dynamicCheck = (DynamicCheck *) node;
      x = evalExprsIn (dynamicCheck->expr);
      return node;

    case ARGUMENT:
      arg = (Argument *) node;
      arg->expr = (Expression *) evalExprsIn (arg->expr);
      x = evalExprsIn (arg->next);
      return node;

    case COUNT_VALUE:
      countValue = (CountValue *) node;
      countValue->count = (Expression *) evalExprsIn (countValue->count);
      countValue->value = (Expression *) evalExprsIn (countValue->value);
      x = evalExprsIn (countValue->next);
      return node;

    case FIELD_INIT:
      fieldInit = (FieldInit *) node;
      fieldInit->expr = (Expression *) evalExprsIn (fieldInit->expr);
      x = evalExprsIn (fieldInit->next);
      return node;

    default:
      printf ("node->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in evalExprsIn");
  }
}



// isIntConst (node) --> bool
//
// Returns true if we have an INT_CONST.  Return false if any problems.
//
int isIntConst (AstNode * node) {
  if (node && node->op == INT_CONST) {
    return 1;
  }
  return 0;
}



// isCharConst (node) --> bool
//
// Returns true if we have an CHAR_CONST.  Return false if any problems.
//
int isCharConst (AstNode * node) {
  if (node && node->op == CHAR_CONST) {
    return 1;
  }
  return 0;
}



// isBoolConst (node) --> bool
//
// Returns true if we have an BOOL_CONST.  Return false if any problems.
//
int isBoolConst (AstNode * node) {
  if (node && node->op == BOOL_CONST) {
    return 1;
  }
  return 0;
}



// isDoubleConst (node) --> bool
//
// Returns true if we have an DOUBLE_CONST.  Return false if any problems.
//
int isDoubleConst (AstNode * node) {
  if (node && node->op == DOUBLE_CONST) {
    return 1;
  }
  return 0;
}



// testSubType (hdr)
//
// This routine is used to test...
//         "isSubType"
//         "typesEqual"
//         "assignable"
//
// This routine expects there to be a class named "C" with two fields named "x" and
// "y".  It tests whether the type of "x" is a subtype/equal/assignable to
// the type of "y".
//
// It builds a mapping, and applies that to the types.  This mapping comes
// from parts of the source.  For example:
//      class C [T1: ..., T2: ...]
//        superclass D [int, char]
//        fields
//          x: ptr to List [T1]
//          y: ptr to List [anyType]
//
// This example builds the mapping:
//        T1 --> int
//        T2 --> char
// and then tests whether "ptr to List [T1]" is a subType of "ptr to List [anyType]"
//
void testSubType (Header * hdr) {
  ClassDef * cl;
  ClassField * fx, * fy;
  Mapping <TypeParm, Type> * map;

  cl = (ClassDef *) hdr->packageMapping->find (lookupAndAdd ("C", ID));
  if ((cl == NULL) || (cl->op != CLASS_DEF)) {
    programLogicError ("In 'testSubType', Expecting class 'C' to exist");
  }
  fx = (ClassField *) cl->classMapping->find (lookupAndAdd ("x", ID));
  if ((fx == NULL) || (fx->op != CLASS_FIELD)) {
    programLogicError ("In 'testSubType', Expecting field 'x' to exist");
  }
  fy = (ClassField *) cl->classMapping->find (lookupAndAdd ("y", ID));
  if ((fy == NULL) || (fy->op != CLASS_FIELD)) {
    programLogicError ("In 'testSubType', Expecting field 'y' to exist");
  }

  map = buildSubstitution (cl->typeParms, cl->superclass->typeArgs);
  if (map) {
    printf ("Mapping = \n");
    map->print (6);
  } else {
    printf ("Mapping = NULL\n");
  }

  printf ("\n=============== Testing typesEqual  ===================\n");
  printf ("\n  Type(x)..... ");
  pretty (fx->type);
  printf ("\n  Type(y)..... ");
  pretty (fy->type);
  printf ("\n");
  printf ("========================================================\n");
  if (typesEqual (fx->type, fy->type)) {
    printf ("\n  Type(x) EQUAL Type(y) \n\n");
  } else {
    printf ("\n  Type(x) NOT EQUAL Type(y) \n\n");
  }

  printf ("=============== Testing isSubType  ===================\n");
  printf ("\n  Type(x)..... ");
  pretty (fx->type);
  printf ("\n  Type(y)..... ");
  pretty (fy->type);
  printf ("\n");
  printf ("========================================================\n");
  if (isSubType (fx->type, fy->type)) {
    printf ("\n  Type(x) SUBTYPE Type(y) \n\n");
  } else {
    printf ("\n  Type(x) NOT SUBTYPE Type(y) \n\n");
  }

  printf ("=============== Testing assignable  ===================\n");
  printf ("\n  Type(x)..... ");
  pretty (fx->type);
  printf ("\n  Type(y)..... ");
  pretty (fy->type);
  printf ("\n");
  printf ("========================================================\n");
  if (assignable (fx->type, fy->type)) {
    printf ("\n  Type(x) ASSIGNABLE <- Type(y) \n\n");
  } else {
    printf ("\n  Type(x) NOT ASSIGNABLE Type(y) \n\n");
  }

}



// typesEqual (t1, t2) --> bool
//
// This routine is passed two Types.  It returns true if type t1 is
// equal to type t2.
//
// If problems are encountered, we just return true.
//
// A type may be recursively defined.  For example:
//    type REC1 = record
//                  next: ptr to REC1
//                endRecord
// In general this is OK.  But occassionally it causes problems.  For example,
// if we try to compare REC1 with a similar type, REC2, we get into a recursion
// problem.
//    type REC2 = record
//                  next: ptr to REC2
//                endRecord
// These two types are equal iff the types of each of their fields are equal.  So
// we must compare "ptr to REC1" to "ptr to REC2".  These equal iff the two base types
// are equal, so we compare "REC1" to "REC2", getting into an infinite loop.
//
// For this reason, we use a "recursionCounter" to detect when we *may* be in an
// infinite recursion, comparing two cyclic data structures.
//
int typesEqual (Type * t1, Type * t2) {
  TypeArg * typeArg1, * typeArg2;
  TypeDef * typeDef;
  NamedType * namedType1, * namedType2;
  ClassDef * classDef1;
  Interface * interface1;
  RecordType * rec1, * rec2;
  RecordField * rf1, *rf2;
  FunctionType * fType1, * fType2;
  TypeParm * typeParm, * typeParm2;
  int equal;
  IntConst * intCon1, * intCon2;

  // printf ("                       -----   typesEqual t1 = ");
  // pretty (t1);
  // printf ("                       -----              t2 = ");
  // pretty (t2);

  // printf ("-----  Testing typesEqual...\n");
  // printf ("-----    t1 = ");
  // pretty (t1);
  // printf ("-----    t2 = ");
  // pretty (t2);

  // Quick check...
  if (t1 == t2) return 1;

  // If for any reason, we've had earlier problems, just return immediately...
  if (t1 == NULL || t2 == NULL) return 1;

  // If either of the types is a type-def, then use that...
  if (t1->op == NAMED_TYPE) {
    typeDef = (TypeDef *) (((NamedType *) t1)->myDef);
    if (typeDef) {
      if (typeDef->op == TYPE_DEF) {
        return typesEqual (typeDef->type, t2);
      }
    } else {
      return 1;
    }
  }
  if (t2->op == NAMED_TYPE) {
    typeDef = (TypeDef *) (((NamedType *) t2)->myDef);
    if (typeDef) {
      if (typeDef->op == TYPE_DEF) {
        return typesEqual (t1, typeDef->type);
      }
    } else {
      return 1;
    }
  }

  switch (t1->op) {

    case ANY_TYPE:                            // in typesEqual
       return (t2->op == ANY_TYPE);

    case CHAR_TYPE:                           // in typesEqual
      return (t2->op == CHAR_TYPE);

    case INT_TYPE:                            // in typesEqual
      return (t2->op == INT_TYPE);

    case DOUBLE_TYPE:                         // in typesEqual
      return (t2->op == DOUBLE_TYPE);

    case BOOL_TYPE:                           // in typesEqual
      return (t2->op == BOOL_TYPE);

    case VOID_TYPE:                           // in typesEqual
      return (t2->op == VOID_TYPE);

    case TYPE_OF_NULL_TYPE:                   // in typesEqual
      return (t2->op == TYPE_OF_NULL_TYPE);

    case PTR_TYPE:                            // in typesEqual
      if (t2->op != PTR_TYPE) return 0;
      if (((PtrType *) t1)->baseType &&
          ((PtrType *) t1)->baseType->op == VOID_TYPE) return 1;
      if (((PtrType *) t2)->baseType &&
          ((PtrType *) t2)->baseType->op == VOID_TYPE) return 1;
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      equal = typesEqual (((PtrType *) t1)->baseType,
                           ((PtrType *) t2)->baseType);
      recursionCounter--;
      return equal;

    case FUNCTION_TYPE:                       // in typesEqual
      if (t2->op != FUNCTION_TYPE) return 0;
      fType1 = (FunctionType *) t1;
      fType2 = (FunctionType *) t2;
      typeArg1 = fType1->parmTypes;
      typeArg2 = fType2-> parmTypes;
      while (1) {
        if ((typeArg1 == NULL) && (typeArg2 == NULL)) break;
        if ((typeArg1 == NULL) || (typeArg2 == NULL)) return 0;
        if (recursionCounter++ > 100) {
          error (t1, "This type appears to be recursively defined");
          error2 (t2, "This type appears to be recursively defined");
          recursionCounter--;
          return 1;
        }
        equal = typesEqual (typeArg2->type, typeArg1->type);
        recursionCounter--;
        if (!equal) return 0;
        typeArg1 = typeArg1->next;
        typeArg2 = typeArg2->next;
      }
      if (fType1->retType == NULL) {
        return fType2->retType == NULL;
      } else if (fType2->retType == NULL) {
        return 0;
      }
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined  (24)");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      equal = typesEqual (fType1->retType,
                          fType2->retType);
      recursionCounter--;
      return equal;

    case ARRAY_TYPE:                          // in typesEqual
      if (t2->op != ARRAY_TYPE) return 0;
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      // printf ("t1->sizeExpr = ");  pretty (((ArrayType *) t1)->sizeExpr);
      // printf ("t2->sizeExpr = ");  pretty (((ArrayType *) t2)->sizeExpr);
      equal = typesEqual (((ArrayType *) t1)->baseType,
                          ((ArrayType *) t2)->baseType);
      recursionCounter--;
      if (!equal) return 0;
      // Next, take a look at the size expressions...
      intCon1 = (IntConst *) ((ArrayType *) t1)->sizeExpr;
      intCon2 = (IntConst *) ((ArrayType *) t2)->sizeExpr;
      // Either (1) both must be dynamic or (2) neither are dynamic...
      if ((intCon1 == NULL) && (intCon2 == NULL)) return 1;
      if ((intCon1 == NULL) || (intCon2 == NULL)) return 0;
      // We have run evalExprs already, so we *hope* that all expressions
      // have been evaluated.  However, there may be perfectly reasonable
      // size expressions (e.g., involving constants) which have not been
      // computed at the time "typesEqual" is used.
      if (intCon1->op != INT_CONST) {
        error (intCon1, "The size of the array cannot be determined at this time");
        errorWithType ("when comparing for equality, this type", t1);
        errorWithType ("and this type", t2);
        return 1;
      }
      if (intCon2->op != INT_CONST) {
        error (intCon2, "The size of the array cannot be determined at this time");
        errorWithType ("when comparing for equality, this type", t1);
        errorWithType ("and this type", t2);
        return 1;
      }
      return intCon1->ivalue == intCon2->ivalue;

    case RECORD_TYPE:                         // in typesEqual
      if (t2->op != RECORD_TYPE) return 0;
      // To be a subtype, the records must match exactly...
      rec1 = (RecordType *) t1;
      rec2 = (RecordType *) t2;
      rf1 = rec1->fields;
      rf2 = rec2->fields;
      while (1) {
        if ((rf1 == NULL) && (rf2 == NULL)) return 1;
        if ((rf1 == NULL) || (rf2 == NULL)) return 0;
        if (rf1->id != rf2->id) return 0;
        if (recursionCounter++ > 100) {
          error (t1, "This type appears to be recursively defined");
          error2 (t2, "This type appears to be recursively defined");
          recursionCounter--;
          return 1;
        }
        equal = typesEqual (rf1->type, rf2->type);
        recursionCounter--;
        if (!equal) return 0;
        rf1 = (RecordField *) rf1->next;
        rf2 = (RecordField *) rf2->next;
      }
      return 1;

    case NAMED_TYPE:                          // in typesEqual
      if (t2->op != NAMED_TYPE) return 0;
      namedType1 = (NamedType *) t1;
      namedType2 = (NamedType *) t2;
      // Wouldn't it be better to compare their "myDef" fields, to make sure
      // the ID's refer to the same class/interface/typeParm...?
      if (namedType1->id == namedType2->id) {
        // We have something like "List[int, int]" and "List [char]" (or "T" and "T")...
        typeArg1 = namedType1->typeArgs;
        typeArg2 = namedType2->typeArgs;
        while (1) {
          // We'll check that we have correct # of args later...
          if (typeArg1 == NULL) return 1;
          if (typeArg2 == NULL) return 1;
          // Check that each arg is equal...
          if (recursionCounter++ > 100) {
            error (t1, "This type appears to be recursively defined");
            error2 (t2, "This type appears to be recursively defined");
            recursionCounter--;
            return 1;
          }
          equal = typesEqual (typeArg2->type, typeArg1->type);
          recursionCounter--;
          if (!equal) return 0;
          typeArg1 = typeArg1->next;
          typeArg2 = typeArg2->next;
        }
        return 1;
      } else {
        // Two classes or interfaces can only be equal to themselves; if this had
        // been the case, they would have had NamedTypes with id==id.
        return 0;
      }
 
    default:                                  // in typesEqual
      printf ("t1->op = %s\n", symbolName (t1->op));
      printf ("t2->op = %s\n", symbolName (t2->op));
      programLogicError ("Unknown op in typesEqual");
  }

}



// isSubType (t1, t2) --> bool
//
// This routine is passed two Types.  It returns true if type t1 is
// a subtype of type t2.
//
// A type may be recursively defined.  For example:
//    type REC1 = record
//                  next: ptr to REC1
//                endRecord
//
// In general this is OK.  But occassionally it causes problems.  For example,
// if we try to compare REC1 with a similar type, REC2, we get into a recursion
// problem.
//    type REC2 = record
//                  next: ptr to REC2
//                endRecord
// These two types are equal iff the types of each of their fields are equal.  So
// we must compare "ptr to REC1" to "ptr to REC2".  These are equal iff the two base
// type are equal, so we compare "REC1" to "REC2", getting into an infinite loop.
//
// For this reason, we use a "recursionCounter" to detect when we *may* be in an
// infinite recursion, comparing two cyclic data structures.
//
// If problems, this routine returns true, hoping to supress further error messages.
//
int isSubType (Type * t1, Type * t2) {
  TypeArg * typeArg1, * typeArg2;
  TypeDef * typeDef;
  NamedType * namedType1, * namedType2;
  ClassDef * classDef1;
  Interface * interface1;
  RecordType * rec1, * rec2;
  RecordField * rf1, *rf2;
  FunctionType * fType1, * fType2;
  TypeParm * typeParm, * typeParm2;
  AstNode * def1, * def2;
  Mapping<TypeParm,Type> * subst;
  int sub;

  // printf ("-----  Testing isSubType...\n");
  // printf ("-----    t1 = ");
  // pretty (t1);
  // printf ("-----    t2 = ");
  // pretty (t2);

//  // Perform the substitutions, if they are present...
//  if (subst1 != NULL) {
//    t1 = copyTypeWithSubst (t1, subst1);
//    // printf ("-----      new t1 = ");
//    // pretty (t1);
//  }
//  if (subst2 != NULL) {
//    t2 = copyTypeWithSubst (t2, subst2);
//    // printf ("-----      new t2 = ");
//    // pretty (t2);
//  }

  if (t1 == t2) return 1;

  // If for any reason, we've had earlier problems, just return immediately...
  if (t1 == NULL || t2 == NULL) return 1;

  // If either of the types is a type-def, then use that...
  if (t1->op == NAMED_TYPE) {
    typeDef = (TypeDef *) (((NamedType *) t1)->myDef);
    if (typeDef) {
      if (typeDef->op == TYPE_DEF) {
        return isSubType (typeDef->type, t2);
      }
    } else {
      return 1;
    }
  }
  if (t2->op == NAMED_TYPE) {
    typeDef = (TypeDef *) (((NamedType *) t2)->myDef);
    if (typeDef) {
      if (typeDef->op == TYPE_DEF) {
        return isSubType (t1, typeDef->type);
      }
    } else {
      return 1;
    }
  }

  if (t1 == t2) return 1;

  if (t2->op == ANY_TYPE) return 1;

  switch (t1->op) {

    case ANY_TYPE:                                  // in isSubType
      return 0;

    case CHAR_TYPE:                                 // in isSubType
      return (t2->op == CHAR_TYPE);

    case INT_TYPE:                                  // in isSubType
      return (t2->op == INT_TYPE);

    case DOUBLE_TYPE:                               // in isSubType
      return (t2->op == DOUBLE_TYPE);

    case BOOL_TYPE:                                 // in isSubType
      return (t2->op == BOOL_TYPE);

    case VOID_TYPE:                                 // in isSubType
      return (t2->op == VOID_TYPE);

    case TYPE_OF_NULL_TYPE:                         // in isSubType
      return ((t2->op == PTR_TYPE) || (t2->op == TYPE_OF_NULL_TYPE));

    case PTR_TYPE:                                  // in isSubType
      if (t2->op != PTR_TYPE) return 0;
      if (!safe && isVoidType (((PtrType *) t1)->baseType)) return 1;
      if (!safe && isVoidType (((PtrType *) t2)->baseType)) return 1;
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      sub = isSubType (((PtrType *) t1)->baseType,
                          ((PtrType *) t2)->baseType);
      recursionCounter--;
      return sub;

    case RECORD_TYPE:                               // in isSubType
      if (t2->op != RECORD_TYPE) return 0;
      // To be a subtype, the records must match exactly...
      rec1 = (RecordType *) t1;
      rec2 = (RecordType *) t2;
      rf1 = rec1->fields;
      rf2 = rec2->fields;
      while (1) {
        if ((rf1 == NULL) && (rf2 == NULL)) return 1;
        if ((rf1 == NULL) || (rf2 == NULL)) return 0;
        if (rf1->id != rf2->id) return 0;
        if (!typesEqual (rf1->type, rf2->type)) return 0;
        rf1 = (RecordField *) rf1->next;
        rf2 = (RecordField *) rf2->next;
      }
      return 1;

    case ARRAY_TYPE:                                // in isSubType
      if (t2->op != ARRAY_TYPE) return 0;
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      sub = typesEqual (((ArrayType *) t1)->baseType,
                          ((ArrayType *) t2)->baseType);
      recursionCounter--;
      return sub;

    case FUNCTION_TYPE:                             // in isSubType
      if (t2->op != FUNCTION_TYPE) return 0;
      fType1 = (FunctionType *) t1;
      fType2 = (FunctionType *) t2;
      typeArg1 = fType1->parmTypes;
      typeArg2 = fType2-> parmTypes;
      while (1) {
        if ((typeArg1 == NULL) && (typeArg2 == NULL)) break;
        if ((typeArg1 == NULL) || (typeArg2 == NULL)) return 1;
        if (recursionCounter++ > 100) {
          error (t1, "This type appears to be recursively defined");
          error2 (t2, "This type appears to be recursively defined");
          recursionCounter--;
          return 1;
        }
        sub = assignable (typeArg1->type, typeArg2->type);
        recursionCounter--;
        if (!sub) return 0;
        typeArg1 = typeArg1->next;
        typeArg2 = typeArg2->next;
      }

      // Check the return type.  If either is "void", both must be "void"...
      if (fType1->retType == NULL) {
        programLogicError ("FunctionType.retType will never be NULL (1)");
      }
      if (fType2->retType == NULL) {
        programLogicError ("FunctionType.retType will never be NULL (2)");
      }
      if (isVoidType (fType1->retType) && isVoidType (fType2->retType)) return 1;
      if (isVoidType (fType1->retType)) return 0;
      if (isVoidType (fType2->retType)) return 0;
      // Make sure the return types are assignable...
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      sub = assignable (fType2->retType, fType1->retType);
      recursionCounter--;
      return sub;

    case NAMED_TYPE:                                // in isSubType

      // Get the definition of t1.  If it happens to be
      // undefined, it is an error, so just return true.
      namedType1 = (NamedType *) t1;
      def1 = namedType1->myDef;
      if (def1 == NULL) return 1;

      // If t1 is a TypeParameter, then see if t1.constraint < T.
      if (def1->op == TYPE_PARM) {
        if (recursionCounter++ > 100) {
          error (t1, "This type appears to be recursively defined");
          error2 (t2, "This type appears to be recursively defined");
          recursionCounter--;
          return 1;
        }
        sub = isSubType (((TypeParm *) def1)->type, t2);
        recursionCounter--;
        if (sub) return 1;
      }

      // Make sure t2 is also a NamedType...
      if (t2->op != NAMED_TYPE) return 0;

      // Get the definition of t2.  If it happens to be
      // undefined, it is an error, so just return true.
      namedType2 = (NamedType *) t2;
      def2 = namedType2->myDef;
      if (def2 == NULL) return 1;

      // printf ("SubType  t1 = ");
      // pretty (t1);
      // printf ("         t2 = ");
      // pretty (t2);

      // If both types refer to the same TypeParameter, return true;
      if ((def1->op == TYPE_PARM) && (def2 == def1)) return 1;

      // If t2 is a TypeParameter, then return false.
      if (def2->op == TYPE_PARM) return 0;

      // We have something like "Person < Student" or "List[int] < Set[anyType]"...

      // NamedTypes may be: TypeDefs, TypeParms, Interfaces, Classes, or undefined.
      // By this time, we have checked for everything except classes and interfaces,
      // so check that they both name a class or interface.
      if ((def1->op != CLASS_DEF) && (def1->op != INTERFACE)) {
        printf ("namedType1->id = %s\n", namedType1->id->chars);
        printf ("def1->op = %s\n", symbolName (def1->op));
        programLogicError ("In isSubType: This must be a CLASS_DEF or INTERFACE");
      }
      if ((def2->op != CLASS_DEF) && (def2->op != INTERFACE)) {
        printf ("namedType2->id = %s\n", namedType2->id->chars);
        printf ("def2->op = %s\n", symbolName (def2->op));
        programLogicError ("In isSubType: This must be a CLASS_DEF or INTERFACE");
      }

      // If the class names are the same...
      if (namedType1->id == namedType2->id) {

        // Make sure there are the same number  and make sure they are pairwise equal...
        typeArg1 = namedType1->typeArgs;
        typeArg2 = namedType2->typeArgs;
        while (1) {
          if ((typeArg1 == NULL) && (typeArg2 == NULL)) return 1;
          if ((typeArg1 == NULL) || (typeArg2 == NULL)) return 0;
          if (!typesEqual (typeArg1->type, typeArg2->type)) return 0;
          typeArg1 = typeArg1->next;
          typeArg2 = typeArg2->next;
        }
        programLogicError ("The above infinite loop terminates with returns");

      } else {      // Else, the two IDs are different...

        // If t1 is a CLASS...
        if (namedType1->myDef->op == CLASS_DEF) {

          // printf ("Within class ");  pretty (namedType1);
          classDef1 = (ClassDef *) (namedType1->myDef);

          // We've got "List[int]" and "Collection[...]"; Assume that the class
          // looks like this "List [T:...]".  Build a subst "T->int".
          subst = buildSubstitution (classDef1->typeParms, namedType1->typeArgs);

          // If t1 has a superclass, then try going up that chain...
          if (classDef1->superclass) {
            // printf ("subst = \n");
            // if (subst == NULL) {
            //   printf ("NULL\n");
            // } else {
            //   subst->print (6);
            // }
            // printf ("Testing superclass relationship...\n");
            if (recursionCounter++ > 100) {
              error (t1, "This type appears to be recursively defined");
              error2 (t2, "This type appears to be recursively defined");
              recursionCounter--;
              return 1;
            }
            sub = isSubType (
                     copyTypeWithSubst (classDef1->superclass, subst),
                     t2);
            recursionCounter--;
            if (sub) return 1;
          }

          // Else, run through the interfaces that t1 implements and see if
          // t1 is a subType of any of those...
          // printf ("Testing superImplements relationship in ");  pretty (namedType1);
          for (typeArg1 = classDef1->implements;
               typeArg1;
               typeArg1 = typeArg1->next) {
            // printf ("  Looking at ");  pretty (typeArg1->type);
            if (recursionCounter++ > 100) {
              error (t1, "This type appears to be recursively defined");
              error2 (t2, "This type appears to be recursively defined");
              recursionCounter--;
              return 1;
            }
            sub = isSubType (
                     copyTypeWithSubst (typeArg1->type, subst),
                     t2);
            recursionCounter--;
            if (sub) return 1;
          }
          return 0;

        // Else if t2 is an Interface...
        } else if (namedType1->myDef->op == INTERFACE) {

          // printf ("Within interface ");  pretty (namedType1);
          interface1 = (Interface *) (namedType1->myDef);

          // We've got "List[int]" and "Collection[...]"; Assume that the interface
          // looks like this "List [T:...]".  Build a subst "T->int".
          subst = buildSubstitution (interface1->typeParms, namedType1->typeArgs);

          // Run through the interfaces that t1 extends and see if
          // t1 is a subType of any of those...
          // printf ("Testing superExtends relationship in ");  pretty (namedType1);
          for (typeArg1 = interface1->extends;
                            typeArg1; typeArg1 = typeArg1->next) {
            // printf ("  Looking at ");  pretty (typeArg1->type);
            if (recursionCounter++ > 100) {
              error (t1, "This type appears to be recursively defined");
              error2 (t2, "This type appears to be recursively defined");
              recursionCounter--;
              return 1;
            }
            sub = isSubType (
                     copyTypeWithSubst (typeArg1->type, subst),
                     t2);
            recursionCounter--;
            if (sub) return 1;
          }
          return 0;

        } else {
          programLogicError ("Tested above that t1 was either Class or Interface");
        }
      }
      return 1;

    default:                                        // in isSubType
      printf ("t1->op = %s\n", symbolName (t1->op));
      printf ("t2->op = %s\n", symbolName (t2->op));
      programLogicError ("Unknown op in isSubType");
  }

}



// assignable (t1, t2) --> bool
//
// This routine is passed two Types.  It returns true if
//        t1 <- t2
//
// If problems arise, this routine returns true, hoping to
// supress further error messages.
//
int assignable (Type * t1, Type * t2) {
  TypeArg * typeArg1, * typeArg2;
  TypeDef * typeDef;
  NamedType * namedType1, * namedType2;
  ClassDef * classDef1;
  Interface * interface1;
  RecordType * rec1, * rec2;
  RecordField * rf1, *rf2;
  FunctionType * fType1, * fType2;
  AstNode * def1, * def2;
  int sub, equal;
  IntConst * intCon1, * intCon2;

  // printf ("-----  Testing assignable...\n");
  // printf ("-----    t1 = ");
  // pretty (t1);
  // printf ("-----    t2 = ");
  // pretty (t2);

  // If for any reason, we've had earlier problems, just return immediately...
  if (t1 == NULL || t2 == NULL) return 1;

  // If either of the types is a type-def, then use that...
  if (t1->op == NAMED_TYPE) {
    typeDef = (TypeDef *) (((NamedType *) t1)->myDef);
    if (typeDef) {
      if (typeDef->op == TYPE_DEF) {
        return assignable (typeDef->type, t2);
      }
    } else {
      return 1;
    }
  }
  if (t2->op == NAMED_TYPE) {
    typeDef = (TypeDef *) (((NamedType *) t2)->myDef);
    if (typeDef) {
      if (typeDef->op == TYPE_DEF) {
        return assignable (t1, typeDef->type);
      }
    } else {
      return 1;
    }
  }

  if (t2->op == ANY_TYPE) return 0;

  switch (t1->op) {

    case ANY_TYPE:                                  // in assignable
      return 0;

    case CHAR_TYPE:                                 // in assignable
      return (t2->op == CHAR_TYPE);

    case INT_TYPE:                                  // in assignable
      return (t2->op == INT_TYPE);

    case DOUBLE_TYPE:                               // in assignable
      return (t2->op == DOUBLE_TYPE);

    case BOOL_TYPE:                                 // in assignable
      return (t2->op == BOOL_TYPE);

    case VOID_TYPE:                                 // in assignable
      return (t2->op == VOID_TYPE);

    case TYPE_OF_NULL_TYPE:                         // in assignable
      return (t2->op == TYPE_OF_NULL_TYPE);

    case PTR_TYPE:                                  // in assignable
      if (t2->op == TYPE_OF_NULL_TYPE) return 1;
      if (t2->op != PTR_TYPE) return 0;
      if (!safe && isVoidType (((PtrType *) t1)->baseType)) return 1;
      if (!safe && isVoidType (((PtrType *) t2)->baseType)) return 1;
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }
      sub = isSubType (((PtrType *) t2)->baseType,
                          ((PtrType *) t1)->baseType);
      recursionCounter--;
      return sub;

    case RECORD_TYPE:                               // in assignable
      if (t2->op != RECORD_TYPE) return 0;
      // To be a assignable, the records must match exactly...
      rec1 = (RecordType *) t1;
      rec2 = (RecordType *) t2;
      rf1 = rec1->fields;
      rf2 = rec2->fields;
      while (1) {
        if ((rf1 == NULL) && (rf2 == NULL)) return 1;
        if ((rf1 == NULL) || (rf2 == NULL)) return 0;
        if (rf1->id != rf2->id) return 0;
        if (!typesEqual (rf1->type, rf2->type)) return 0;
        rf1 = (RecordField *) rf1->next;
        rf2 = (RecordField *) rf2->next;
      }
      return 1;

    case ARRAY_TYPE:                                // in assignable
      if (t2->op != ARRAY_TYPE) return 0;
      if (recursionCounter++ > 100) {
        error (t1, "This type appears to be recursively defined");
        error2 (t2, "This type appears to be recursively defined");
        recursionCounter--;
        return 1;
      }

      // printf ("t1->sizeExpr = ");  pretty (((ArrayType *) t1)->sizeExpr);
      // printf ("t2->sizeExpr = ");  pretty (((ArrayType *) t2)->sizeExpr);

      equal = typesEqual (((ArrayType *) t1)->baseType,
                          ((ArrayType *) t2)->baseType);
      recursionCounter--;
      if (!equal) return 0;
      // Next, take a look at the size expressions...
      intCon1 = (IntConst *) ((ArrayType *) t1)->sizeExpr;
      intCon2 = (IntConst *) ((ArrayType *) t2)->sizeExpr;
      // If either is dynamic, not assignable...
      if ((intCon1 == NULL) || (intCon2 == NULL)) return 0;
      // We have run evalExprs already, so we *hope* that all expressions
      // have been evaluated.  However, there may be perfectly reasonable
      // size expressions (e.g., involving constants) which have not been
      // computed at the time "typesEqual" is used.
      if (intCon1->op != INT_CONST) {
        error (intCon1, "The size of the array cannot be determined at this time");
        errorWithType ("when testing for assignability, this type", t1);
        errorWithType ("and this type", t2);
        return 1;
      }
      if (intCon2->op != INT_CONST) {
        error (intCon2, "The size of the array cannot be determined at this time");
        errorWithType ("when testing for assignability, this type", t1);
        errorWithType ("and this type", t2);
        return 1;
      }
      return intCon1->ivalue == intCon2->ivalue;

    case FUNCTION_TYPE:                             // in assignable
      return 0;

    case NAMED_TYPE:                                // in assignable
      // If one type is a NamedType, make sure both types are NamedTypes.
      // If either happens to be undefined, it is an error, so just return true.
      namedType1 = (NamedType *) t1;
      def1 = namedType1->myDef;
      if (def1 == NULL) return 1;

      if (t2->op != NAMED_TYPE) return 0;
      namedType2 = (NamedType *) t2;
      def2 = namedType2->myDef;
      if (def2 == NULL) return 1;

      // If both types refer to the same TypeParameter, return true;
      // Else if either is a typeParm, then return false.
      if (def1->op == TYPE_PARM) return (def2 == def1);
      if (def2->op == TYPE_PARM) return 0;

      // If either type refers to an interface, then return false.
      if (def1->op == INTERFACE) return 0;
      if (def2->op == INTERFACE) return 0;

      // We have something like "Person <- Person" or "List[anyType] <- List[int]"...
      // Make sure the ID's are the same.
      if (namedType1->id != namedType2->id) return 0;

      // The names are known to be defined; make sure they refer to the same class.
      if (def2 != def1) return 0;

      // NamedTypes may be: TypeDefs, TypeParms, Interfaces, Classes, or undefined.
      // By this time, we have checked for everything except classes, so they
      // must both name a class.
      if (def1->op != CLASS_DEF) {
        programLogicError ("In assignable: This must be a CLASS_DEF, but is not");
      }

      // Now run through the typeArgs.  Make sure there are the same number
      // and make sure they are pairwise equal...
      typeArg1 = namedType1->typeArgs;
      typeArg2 = namedType2->typeArgs;
      while (1) {
        if ((typeArg1 == NULL) && (typeArg2 == NULL)) return 1;
        if ((typeArg1 == NULL) || (typeArg2 == NULL)) return 0;
        if (!typesEqual (typeArg1->type, typeArg2->type)) return 0;
        typeArg1 = typeArg1->next;
        typeArg2 = typeArg2->next;
      }
      programLogicError ("The above infinite loop terminates with returns");

    default:                                  // in assignable
      printf ("t1->op = %s\n", symbolName (t1->op));
      printf ("t2->op = %s\n", symbolName (t2->op));
      programLogicError ("Unknown op in assignable");
  }

}



// buildSubstitution (listOfTypeParms, listOfTypeArgs) -> mapping
//
// This routine is passed a list of TypeParms and a list of TypeArgs.
//
// It returns a substitution, which is a mapping from typeParms to types.
//
// Either of the args may be NULL, in which case it returns the NULL
// substitution.  If the lists are not the same length, it just does what it
// can without printing any error messages.
//
Mapping <TypeParm, Type> * buildSubstitution (TypeParm * typeParm,
                                              TypeArg * typeArg) {
  Mapping <TypeParm, Type> * mapping;

  if (typeParm == NULL) return NULL;
  if (typeArg == NULL) return NULL;
  mapping = new Mapping <TypeParm, Type> (5, NULL);
  while (1) {
    // We'll check that we have correct # of args later...
    if (typeParm == NULL) return mapping;
    if (typeArg == NULL) return mapping;
    mapping->enter (typeParm, typeArg->type);
    typeParm = typeParm->next;
    typeArg = typeArg->next;
  }
  return mapping;
}



// checkImplements (hdr)
//
// This routine looks at each class and then looks at the interfaces that it
// "implements".  It makes sure that the class provides a method for each message
// that is in the interface and that the parameter types and return type are correct,
// according to co- and contra-variance.
//
void checkImplements (Header * hdr) {
  ClassDef * cl;
  TypeArg * typeArg, * superTypeArg;
  TypeParm * typeParm;
  NamedType * nType;
  Interface * superInter;
  Mapping <TypeParm, Type> * subst;
  MethodProto * proto, * classProto;
  Parameter * superParm, * subParm;

  // Run through all classes...
  for (cl = hdr->classes; cl; cl = cl->next) {

    // Run through each of the interfaces that this class implements...
    for (superTypeArg = cl->implements;
         superTypeArg;
         superTypeArg = superTypeArg->next) {
      nType = (NamedType *) superTypeArg->type;
      superInter = (Interface *) (nType->myDef);
      // If no previous errors...
      if (superInter) {
        if (superInter->op == INTERFACE) {

          // Build a substitution for the TypeParms...
          subst = NULL;
          typeParm = superInter->typeParms;
          typeArg = nType->typeArgs;
          while (1) {
            if ((typeParm == NULL) && (typeArg == NULL)) break;
            if ((typeParm == NULL) || (typeArg == NULL)) {
              error (superTypeArg, "The number of type arguments here does not match the number of type parameters in the interface definition");
              break;
            }
            if (subst == NULL) {
              subst = new Mapping <TypeParm, Type> (3, NULL);
            }
            subst->enter (typeParm, typeArg->type);
            typeParm = typeParm->next;
            typeArg = typeArg->next;
          }

          // printf ("subst = \n");
          // if (subst) {
          //   subst->print (6);
          // } else {
          //   printf ("NULL\n");
          // }

          // Run through the superInterfaces's methodProtos...
          for (proto = superInter->methodProtos; proto; proto=proto->next) {
            // First, make sure this class has this method...
            classProto = cl->selectorMapping->findInTopScope (proto->selector);
            if (classProto == NULL) {
              error (cl, "This class fails to provide a method");
              error2 (proto, "Here is the message from an interface this class implements");
            } else {
              // Make sure the parameter types are correct...
              superParm = proto->parmList;
              subParm = classProto->parmList;
              while (1) {
                if ((superParm == NULL) && (subParm == NULL)) break;
                if ((superParm == NULL) || (subParm == NULL)) {
                  error (classProto, "This method does not have the correct number of parameters");
                  error2 (proto, "Here is the message from an interface this class implements");
                  break;
                }
                if (!assignable (subParm->type,
                                 copyTypeWithSubst (superParm->type, subst))) {
                  error (subParm, "This parameter's type is not correct (perhaps it is not general enough)");
                  error2 (superParm, "Here is the corresponding parameter from a message in an interface this class implements");
                  errorWithType (
                         "The interface requires that this parameter be able to handle values of type",
                         copyTypeWithSubst (superParm->type, subst));
                  errorWithType (
                         "The type of this method's parameter is",
                         subParm->type);
                }
                superParm = (Parameter *) superParm->next;
                subParm = (Parameter *) subParm->next;
              }
              // Make sure the return type is correct...
              if (!assignable (copyTypeWithSubst (proto->retType, subst),
                               classProto->retType)) {
                error (classProto, "The return type of this method is not correct (perhaps it is too general)");
                error2 (proto, "Here is the corresponding message in an interface this class implements");
                errorWithType (
                         "The interface requires this method to return a type assignable to",
                         copyTypeWithSubst (proto->retType, subst));
                errorWithType (
                         "This method's return type is",
                         classProto->retType);
              }
              // Make sure the kind is correct...
              if (proto->kind != classProto->kind) {
                error (classProto, "This method is not the correct kind (infix, binary, keyword, normal)");
                error2 (proto, "Here is the message from an interface this class implements");
              }
            }
          }        // for all protos

        }        // if superInter == INTERFACE
      }        // if superInter
    }        // for all superTypeArgs
  }        // for all classes
}



// checkMethodProtos (hdr)
//
// This routine looks at each class and does the following:
//
// 1.  For each method, make sure its parameter types and return type match a prototype
//     in that class.
// 2.  When methods are inherited, make sure that the parameter and return types
//     of the super-prototype and the sub-prototype respect co/contra-variance.
// 3.  Make sure the names of the parameters exactly match in the Method and in the
//     MethodPrototype.
//
void checkMethodProtos (Header * hdr) {
  ClassDef * cl;
  Method * meth;
  MethodProto * proto, * protoSuper, * protoSub;
  Parameter * methParm, * protoParm, * subParm;
  String * superSelector;

  // Run through all classes...
  for (cl = hdr->classes; cl; cl = cl->next) {
    // printf ("Looking at class %s...\n", cl->id->chars);

    // Run though all the methods...
    for (meth=cl->methods; meth; meth = meth->next) {

      // Find the prototype in this class...
      proto = cl->selectorMapping->findInTopScope (meth->selector);
      if (proto) {       // If no earlier errors...

        // printf ("Looking at method %s and proto %s...\n",
        //         meth->selector->chars, proto->selector->chars);

        // Make sure it is the same kind.  (This is not strictly necessary, since if
        // they are different kinds, there will always be another error message.)
        if (meth->kind != proto->kind) {
          error (meth, "This method is not the same kind (infix, prefix, keyword, normal) as the corresponding prototype");
          error2 (proto, "Here is the prototype");
        }

        // Make sure each parameter type matches according to contra-variance...
        protoParm = proto->parmList;
        methParm = meth->parmList;
        while (1) {
          if ((protoParm == NULL) && (methParm == NULL)) break;
          if ((protoParm == NULL) || (methParm == NULL)) {
            error (meth, "This method does not have the same number of parameters as the corresponding prototype");
            error2 (proto, "Here is the prototype");
            break;
          }
          if (! assignable (methParm->type, protoParm->type)) {
            error (methParm, "The type of this parameter fails to match the type of the corresponding parameter in the prototype (perhaps the type is not general enough)");
            error2 (proto, "Here is the method prototype");
            errorWithType ("The type of the method parameter is", methParm->type);
            errorWithType ("The expected type is", protoParm->type);
          }
          // Make sure that the parameters have the same names...
          if (protoParm->id != methParm->id) {
            error (methParm, "The name of this parameter does not match the name of the corresponding parameter in the method prototype");
            error2 (protoParm, "Here is the name used in the method prototype");
          }
          protoParm = (Parameter *) protoParm->next;
          methParm = (Parameter *) methParm->next;
        }

        // Make sure the return types match according to covariance...
        if (! assignable (proto->retType, meth->retType)) {
          error (meth, "The return type of this method fails to match the corresponding prototype (perhaps the type is too general)");
          error2 (proto, "Here is the method prototype");
          errorWithType ("The return type of this method is", meth->retType);
          errorWithType ("The expected type is", proto->retType);
        }

        // Make this MethodProto point to this Method, and the Method
        // point to the MethodProto...

      }
    }

    // Run through all methods that override something from the superclass...
    for (meth = cl->methods; meth; meth=meth->next) {

      // printf ("Looking at method %s\n", meth->selector->chars);

      // See if we have prototypes for this method and for an inherited version...
      superSelector = addSuperTo (meth->selector);
      protoSub = cl->selectorMapping->findInTopScope (meth->selector);
      protoSuper = cl->selectorMapping->findInTopScope (superSelector);
      if (protoSub && protoSuper) {
        // printf ("     protoSuper = ");
        // pretty (protoSuper);
        // printf ("     protoSub   = ");
        // pretty (protoSub);
        checkProtos (protoSuper, protoSub);
      }
    }

  }     // for all classes
}



// checkProtos (protoSuper, protoSub)
//
// This routine is passed two MethodProtos.  It checks to make sure they are compatable
// according to co- and contra-variance.  It will print error messages if not.
//
// See also checkProtos2, which is a VERY SIMILAR ROUTINE.
//
void checkProtos (MethodProto * protoSuper, MethodProto * protoSub) {
  Parameter * protoParm, * subParm;

  if ((protoSuper == NULL) || (protoSub == NULL)) {
    programLogicError ("Should have two non-NULL protos");
  }

  // printf ("Looking at protoSuper = ");
  // pretty (protoSuper);
  // printf ("           protoSub = ");
  // pretty (protoSub);

  // They should have the same kind...
  if (protoSuper->kind != protoSub->kind) {
    programLogicError ("Kind mismatch in checkProtos");
  }

  // Make sure each parm type matches according to contra-variance...
  protoParm = protoSuper->parmList;
  subParm = protoSub->parmList;
  while (1) {
    if ((protoParm == NULL) && (subParm == NULL)) break;
    if ((protoParm == NULL) || (subParm == NULL)) {
      error (protoSub, "This method does not have the same number of parameters as the corresponding method in the super class");
      error2 (protoSuper, "Here is the prototype from the super class");
      break;
    }
    if (! assignable (subParm->type, protoParm->type)) {
      error (subParm, "When overriding a method... the type of this parameter fails to match the type of the corresponding parameter in the method from the super class (perhaps it is not general enough)");
      error2 (protoSuper, "Here is the method from the super class");
      errorWithType ("The parameter type in the subclass is", subParm->type);
      errorWithType ("The expected type is", protoParm->type);
    }
    protoParm = (Parameter *) protoParm->next;
    subParm = (Parameter *) subParm->next;
  }

  // Make sure the return types match according to covariance...
  if (! assignable (protoSuper->retType, protoSub->retType)) {
    error (protoSub, "When overriding a method... the return type of this method fails to match the corresponding method from the super class (perhaps it is too general)");
    error2 (protoSuper, "Here is the method from the super class");
    errorWithType ("The type returned in the subclass is", protoSub->retType);
    errorWithType ("The expected type is", protoSuper->retType);
  }
}



// checkProtos2 (protoSuper, protoSub)
//
// This routine is passed two MethodProtos.  It checks to make sure they are compatable
// according to co- and contra-variance.  It will print error messages if not.
//
// This routine differs from "checkProtos" in that it is used for checking "extends"
// among interfaces.  The code is the same, but the error message texts differ slightly.
//
void checkProtos2 (MethodProto * protoSuper, MethodProto * protoSub) {
  Parameter * protoParm, * subParm;

  if ((protoSuper == NULL) || (protoSub == NULL)) {
    programLogicError ("Should have two non-NULL protos");
  }

  // printf ("Looking at protoSuper = ");
  // pretty (protoSuper);
  // printf ("           protoSub = ");
  // pretty (protoSub);

  // They should have the same kind...
  if (protoSuper->kind != protoSub->kind) {
    programLogicError ("Kind mismatch in checkProtos2");
  }

  // Make sure each parm type matches according to contra-variance...
  protoParm = protoSuper->parmList;
  subParm = protoSub->parmList;
  while (1) {
    if ((protoParm == NULL) && (subParm == NULL)) break;
    if ((protoParm == NULL) || (subParm == NULL)) {
      error (protoSub, "This message does not have the same number of parameters as the corresponding message in an interface we are extending");
      error2 (protoSuper, "Here is the prototype from the super-interface");
      break;
    }
    if (! assignable (subParm->type, protoParm->type)) {
      error (subParm, "The type of this parameter fails to match the type of the corresponding parameter in the same message in an interface we are extending (perhaps it is not general enough)");
      error2 (protoSuper, "Here is the message from the super-interface");
      errorWithType ("The parameter type in the sub-interface is", subParm->type);
      errorWithType ("The expected type is", protoParm->type);
    }
    protoParm = (Parameter *) protoParm->next;
    subParm = (Parameter *) subParm->next;
  }

  // Make sure the return types match according to covariance...
  if (! assignable (protoSuper->retType, protoSub->retType)) {
    error (protoSub, "The return type of this message fails to match the corresponding message in an interface we are extending (perhaps it is too general)");
    error2 (protoSuper, "Here is the message from the super-interface");
    errorWithType ("The type returned in the sub-interface is", protoSub->retType);
    errorWithType ("The expected type is", protoSuper->retType);
  }
}



// checkProtos3 (proto1, proto2)
//
// This routine is passed two MethodProtos.  It checks to make sure their parameter and
// return types are equal.  It will print error messages if not.
//
void checkProtos3 (MethodProto * proto1, MethodProto * proto2, Interface * inter) {
  Parameter * parm1, * parm2;

  if ((proto1 == NULL) || (proto2 == NULL)) {
    programLogicError ("Should have two non-NULL protos in checkProtos3");
  }

  // printf ("Looking at proto1 = ");
  // pretty (proto1);
  // printf ("           proto2 = ");
  // pretty (proto2);

  // They should have the same kind...
  if (proto1->kind != proto2->kind) {
    programLogicError ("Kind mismatch in checkProtos3");
  }

  // Make sure each parm type matches exactly...
  parm1 = proto1->parmList;
  parm2 = proto2->parmList;
  while (1) {
    if ((parm1 == NULL) && (parm2 == NULL)) break;
    if ((parm1 == NULL) || (parm2 == NULL)) {
      error (inter, "This interface extends two other interfaces; both contain the same message but the messages do not have the same number of parameters");
      error2 (proto1, "Here is the message in one super-interface");
      error2 (proto2, "Here is the message in the other super-interface");
      break;
    }
    if (! typesEqual (parm1->type, parm2->type)) {
      error (inter, "This interface extends two other interfaces and both contain the same message; since there is no corresponding message in this interface, the types of the parameters must match exactly, but they do not (You might consider adding a message in this interface with a parameter type that is more general)");
      error2 (parm2, "Here is the parameter from one super-interface");
      error2 (parm1, "Here is the parameter from the other super-interface");
    }
    parm1 = (Parameter *) parm1->next;
    parm2 = (Parameter *) parm2->next;
  }

  // Make sure the return types match according to covariance...
  if (! typesEqual (proto2->retType, proto1->retType)) {
    error (inter, "This interface extends two other interfaces and both contain the same message; since there is no corresponding message in this interface, the return types of must match exactly, but they do not (You might consider adding a message in this interface with a more specific return type)");
    error2 (proto1, "Here is the message from one super interface");
    error2 (proto2, "Here is the message from the other super interface");
  }
}



// checkExtends (hdr)
//
// This routine runs through each interface and for each, checks the following:
//
// 1.  If this interface provides a message that is also inherited, we make sure
//     that the parameter and return types respect co- and contra-variance.
// 2.  If this interface inherits the same message from different sources,
//     but does not contain the message itself, we check to make sure that
//     the parameter and return types are equal on all versions of the inherited
//     method.
//
void checkExtends (Header * hdr) {
  Interface * inter;
  MethodProto * protoSuper, * protoSub, * otherProto;
  Mapping <String, MethodProto> * otherMap;

  otherMap = new Mapping <String, MethodProto> (0, NULL);

  // Run through all interfaces...
  for (inter = hdr->interfaces; inter; inter = inter->next) {
    // printf ("Looking at interface %s...\n", inter->id->chars);

    // Run through all inherited methods...
    for (protoSuper = inter->inheritedMethodProtos;
         protoSuper;
         protoSuper = protoSuper->next) {

      // printf ("Looking at proto = %s...\n", protoSuper->selector->chars);

      // See if this interface also provides this method and, if so, check that
      // they are compatible...
      protoSub = inter->selectorMapping->findInTopScope (protoSuper->selector);
      if (protoSub) {
        checkProtos2 (protoSuper, protoSub);

      // If this message is inherited, but not present in this class...
      } else {
        // See if we've seen something similar before...
        // printf ("This proto is not in this method; looking in otherMap...\n");
        otherProto = otherMap->findInTopScope (protoSuper->selector);
        if (otherProto) {
          // printf ("Found another one...\n");
          checkProtos3 (protoSuper, otherProto, inter);
        } else {
          // printf ("Not found; adding it...\n");
          otherMap->enter (protoSuper->selector, protoSuper);
        }
      }
      
    }

    // Now run through all inherited methods and add them to our "selectorMapping".
    // There may be duplicates, due to multiple inheritance, but they must all
    // have equal parm and return types.  If there are multiple occurrences, just
    // add the first one.
    for (protoSuper = inter->inheritedMethodProtos;
         protoSuper;
         protoSuper = protoSuper->next) {

      // printf ("Looking at proto = %s...\n", protoSuper->selector->chars);

      // If not found, add it...
      protoSub = inter->selectorMapping->findInTopScope (protoSuper->selector);
      if (protoSub == NULL) {
        inter->selectorMapping->enter (protoSuper->selector, protoSuper);
      }
    }

  }

}



// checkTypes (node) --> type
//
// This routine walks the entire Abstract Syntax Tree, visiting every node.
// For each expression, it computes the type of the expression and returns it.
// For other sorts of nodes, it simply returns NULL.
//
// If errors occur, it returns NULL.
//
// It catches the following errors:
//        ...
//
// This routine also checks that each Function matches its FunctionProto, making
// sure that the parameters have the same names and types.
//         The parm names on a Function do not match those in the FunctionProto
//         There are a different number of parms in Function and FunctionProto
//         The parm/return types do not match in Function and FunctionProto
//
// This routine also identifies primitive operations.
//
Type * checkTypes (AstNode * node) {
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
    TypeArg * typeArg, * typeArgList, * nextTypeArg;
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
    NamedType * nType, * namedType2;
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
    Type * t, * t2, * resultType;
    VarDecl * varDecl;
    String * nameWithoutSuper, * newSelector;
    AstNode * def;
    int recvrIsSuper, count, i, looksLikeDeref, numberOfCases;
    Mapping <String, AstNode> * tempMap;
    Parameter * protoParm, * subParm, * funParm;
    int arSize, j;
    Case * * caseArray;
    DoubleConst * doubleConst;
    StringConst * stringConst;
    IntConst * intConst;

  if (node == NULL) return NULL;

  // printf ("%s...\n", symbolName (node->op));

  switch (node->op) {

    case HEADER:                               // in checkTypes
      header = (Header *) node;
      // printf ("  %s\n", header->packageName->chars);
      t = checkTypes (header->uses);
      t = checkTypes (header->consts);
      t = checkTypes (header->errors);
      t = checkTypes (header->globals);
      t = checkTypes (header->typeDefs);
      t = checkTypes (header->functionProtos);
      t = checkTypes (header->closures);  // do clos before funs, so debug printing works
      t = checkTypes (header->functions);
      t = checkTypes (header->interfaces);
      t = checkTypes (header->classes);
      // t = checkTypes (header->next);
      return NULL;

//    case CODE:                               // in checkTypes
//      code = (Code *) node;
//      t = checkTypes (code->consts);
//      t = checkTypes (code->globals);
//      t = checkTypes (code->typeDefs);
//      t = checkTypes (code->functions);
//      t = checkTypes (code->interfaces);
//      t = checkTypes (code->classes);
//      t = checkTypes (code->behaviors);
//      return NULL;

    case USES:                               // in checkTypes
      uses = (Uses *) node;
      t = checkTypes (uses->renamings);
      t = checkTypes (uses->next);
      return NULL;

    case RENAMING:                               // in checkTypes
      renaming = (Renaming *) node;
      t = checkTypes (renaming->next);
      return NULL;

    case INTERFACE:                               // in checkTypes
      interface = (Interface *) node;
      // printf ("  %s\n", interface->id->chars);
      t = checkTypes (interface->typeParms);
      t = checkTypes (interface->extends);
      t = checkTypes (interface->methodProtos);
      t = checkTypes (interface->next);
      return NULL;

    case CLASS_DEF:                               // in checkTypes
      cl = (ClassDef *) node;
      currentClass = cl;
      // printf ("  %s\n", cl->id->chars);
      // Create a subtree to represent the type of "self"...
      nType = new NamedType ();
      nType->positionAt (cl);
      nType->id = cl->id;
      nType->myDef = cl;
      typeArgList = NULL;
      // Build a list of parameters...
      for (typeParm = cl->typeParms; typeParm; typeParm = typeParm->next) {
        typeArg = new TypeArg ();
        typeArg->positionAt (cl);
          namedType2 = new NamedType ();
          namedType2->positionAt (cl);
          namedType2->id = typeParm->id;
          namedType2->myDef = typeParm;
        typeArg->type = namedType2;
        typeArg->next = typeArgList;
        typeArgList = typeArg;
      }
      // Reverse the list...
      typeArg = typeArgList;
      while (typeArg) {
        nextTypeArg = typeArg->next;
        typeArg->next = nType->typeArgs;
        nType->typeArgs = typeArg;
        typeArg = nextTypeArg;
      }
      pType = new PtrType ();
      pType->positionAt (cl);
      pType->baseType = nType;
      cl->typeOfSelf = pType;
      t = checkTypes (cl->typeParms);
      t = checkTypes (cl->implements);
      t = checkTypes (cl->superclass);
      t = checkTypes (cl->fields);
      t = checkTypes (cl->methodProtos);
      t = checkTypes (cl->methods);
      t = checkTypes (cl->next);
      currentClass = NULL;
      return NULL;

//    case BEHAVIOR:                               // in checkTypes
//      behavior = (Behavior *) node;
//      printf ("  %s\n", behavior->id->chars);
//      t = checkTypes (behavior->methods);
//      t = checkTypes (behavior->next);
//      return NULL;

    case TYPE_DEF:                               // in checkTypes
      typeDef = (TypeDef *) node;
      // printf ("  %s\n", typeDef->id->chars);
      t = checkTypes (typeDef->type);
      // The following check is not really necessary; it is more of a warning...
      // if (isObjectType (typeDef->type)) {
      //   error (typeDef, "You may not give classes or interfaces a second name");
      // }
      t = checkTypes (typeDef->next);
      return NULL;

    case CONST_DECL:                               // in checkTypes
      constDecl = (ConstDecl *) node;
      // printf ("  %s\n", constDecl->id->chars);
      t = checkTypes (constDecl->expr);
      if (constDecl->expr) {
        if ((constDecl->expr->op != CHAR_CONST) &&
            (constDecl->expr->op != INT_CONST) &&
            (constDecl->expr->op != DOUBLE_CONST) &&
            (constDecl->expr->op != BOOL_CONST) &&
            (constDecl->expr->op != NULL_CONST) &&
            (constDecl->expr->op != STRING_CONST)) {
          error (node, "Unable to determine the value of this constant at compile-time");
        }
      }
      t = checkTypes (constDecl->next);
      return NULL;

    case ERROR_DECL:                               // in checkTypes
      errorDecl = (ErrorDecl *) node;
      // printf ("  %s\n", errorDecl->id->chars);
      t = checkTypes (errorDecl->parmList);
      t = checkTypes (errorDecl->next);
      return NULL;

    case FUNCTION_PROTO:                               // in checkTypes
      functionProto = (FunctionProto *) node;
      // printf ("  %s\n", functionProto->id->chars);

      // Check that the main function is not external and not private...
      if (functionProto->id == stringMain) {
        if (functionProto->isPrivate) {
          error (functionProto, "There is no prototype for the 'main' function in the header file");
        }
        if (functionProto->isExternal) {
          error (functionProto, "The 'main' function must not be 'external'");
        }
       }

      t = checkTypes (functionProto->parmList);
      t = checkTypes (functionProto->retType);
      functionProto->retSize = sizeInBytesOfWhole(functionProto->retType,
                                                  functionProto->retType,
                                                  0);    // wantPrinting = 0
      t = checkTypes (functionProto->next);
      return NULL;

    case FUNCTION:                               // in checkTypes
      fun = (Function *) node;
      // if (fun->id) {
      //   printf ("    CheckTypes: Within function %s\n", fun->id->chars);
      // } else {
      //   printf ("    CheckTypes: Within closure\n");
      // }

      // If this is a "main" function...
      if (fun->id == stringMain) {
        if (fun->parmList != NULL) {
          error (fun, "The 'main' function must not have any parameters");
        }
        if (! isVoidType (fun->retType)) {
          error (fun, "The 'main' function must not return a result");
        }
        // if (fun->myProto == NULL) {
        //   error (fun, "There is no prototype for the 'main' function in the header file");
        // }
      }

      // Run through the prototype and make sure that the parameter names and types
      // match exactly...
      functionProto = fun->myProto;
      if (functionProto != NULL) {

        // Make sure each parameter type matches...
        protoParm = functionProto->parmList;
        funParm = fun->parmList;
        while (1) {
          if ((protoParm == NULL) && (funParm == NULL)) break;
          if ((protoParm == NULL) || (funParm == NULL)) {
            error (fun, "This function does not have the same number of parameters as the corresponding prototype");
            error2 (functionProto, "Here is the prototype");
            break;
          }
          if (! typesEqual (protoParm->type, funParm->type)) {
            error (funParm, "The type of this parameter does not match the type of the corresponding parameter in the prototype");
            error2 (fun, "Here is the function");
            errorWithType ("The parameter type in the function is", funParm->type);
            errorWithType ("The parameter type in the function prototype is", protoParm->type);
          }
          // Make sure that the parameters have the same names...
          if (protoParm->id != funParm->id) {
            error (funParm, "The name of this parameter does not match the name of the corresponding parameter in the function prototype");
            error2 (fun, "Here is the function");
            error2 (protoParm, "Here is the name used in the function prototype");
          }
          protoParm = (Parameter *) protoParm->next;
          funParm = (Parameter *) funParm->next;
        }

        // Make sure the return types match...
        if (! typesEqual (fun->retType, functionProto->retType)) {
          error (fun, "The return type of this function does not match the corresponding prototype");
          errorWithType ("The return type in the function is", fun->retType);
          errorWithType ("The return type in the function prototype is", functionProto->retType);
        }
      }

      maxArgBytesSoFar = 0;
      // printf ("setting maxArgBytesSoFar to zero...\n");

      t = checkTypes (fun->parmList);
      t = checkTypes (fun->retType);
      t = checkTypes (fun->locals);
      t = checkTypes (fun->stmts);

      // printf ("Setting fun->maxArgBytes = %d...\n", maxArgBytesSoFar);
      fun->maxArgBytes = maxArgBytesSoFar;
      maxArgBytesSoFar = -1;

      t = checkTypes (fun->next);

      return NULL;

    case METHOD_PROTO:                               // in checkTypes
      methodProto = (MethodProto *) node;
      // printf ("  %s\n", methodProto->selector->chars);

      // Check to see if this selector could override a built-in operator.
      // For example, overriding "+" makes "ptrToObj + xxx" ambiguous.  However,
      // some methods are okay.  For example, we can have a method for "*" since
      // "ptrToObj * x" would otherwise be illegal...
      if (methodProto->kind == PREFIX) {
        switch (methodProto->selector->primitiveSymbol) {
          case UNARY_STAR:
            error (methodProto, "Defining a prefix method with this selector causes confusion with the built-in dereference operator, in '* ptrToObj'");
            break;
          case UNARY_BANG:
            error (methodProto, "Defining a prefix method with this selector causes confusion with a built-in operator in the boolean expression '! ptrToObj'");
            break;
          case UNARY_AMP:
            error (methodProto, "Defining a prefix method with this selector causes confusion with the built-in address-of operator");
            break;
        }
      }
      if (methodProto->kind == INFIX) {
        switch (methodProto->selector->primitiveSymbol) {
          case MINUS:
            error (methodProto, "Defining an infix method with this selector causes confusion with built-in pointer subtraction in 'ptrToObj - x'");
            break;
          case PLUS:
            error (methodProto, "Defining an infix method with this selector causes confusion with built-in pointer addition in 'ptrToObj + x'");
            break;
          case BAR_BAR:
            error (methodProto, "Defining an infix method with this selector causes confusion with a built-in operator in the boolean expression 'ptrToObj || x'");
            break;
          case AMP_AMP:
            error (methodProto, "Defining an infix method with this selector causes confusion with a built-in operator in the boolean expression 'ptrToObj && x'");
            break;
          case EQUAL_EQUAL:
            error (methodProto, "Defining an infix method with this selector causes confusion with the built-in EQUALS operator");
            break;
          case NOT_EQUAL:
            error (methodProto, "Defining an infix method with this selector causes confusion with the built-in NOT-EQUALS operator");
            break;
        }
      }
      t = checkTypes (methodProto->parmList);
      t = checkTypes (methodProto->retType);
      methodProto->retSize = sizeInBytesOfWhole(methodProto->retType,
                                                methodProto->retType,
                                                0);    // wantPrinting = 0
      t = checkTypes (methodProto->next);
      return NULL;

    case METHOD:                               // in checkTypes
      meth = (Method *) node;
      // printf ("  %s\n", meth->selector->chars);

      maxArgBytesSoFar = 0;

      t = checkTypes (meth->parmList);
      t = checkTypes (meth->retType);
      t = checkTypes (meth->locals);
      t = checkTypes (meth->stmts);

      meth->maxArgBytes = maxArgBytesSoFar;
      maxArgBytesSoFar = -1;

      t = checkTypes (meth->next);

      return NULL;

    case TYPE_PARM:                               // in checkTypes
      typeParm = (TypeParm *) node;
      // Make sure that the constraint type is not a basic type...
      if (isCharType (typeParm->type) ||
          isIntType (typeParm->type) ||
          isBoolType (typeParm->type) ||
          isDoubleType (typeParm->type) ||
          isVoidType (typeParm->type) ||
          isTypeOfNullType (typeParm->type)) {
        error (typeParm, "The constraint on this type parameter is a basic type; this is meaningless, since this parameter can only be instantiated by that type itself");
        errorWithType ("The constraint is", typeParm->type);
      }
      t = checkTypes (typeParm->type);
      t = checkTypes (typeParm->next);
      return NULL;

    case TYPE_ARG:                               // in checkTypes
      typeArg = (TypeArg *) node;
      t = checkTypes (typeArg->type);
      t = checkTypes (typeArg->next);
      return NULL;

    case CHAR_TYPE:                              // in checkTypes
      return NULL;

    case INT_TYPE:                               // in checkTypes
      return NULL;

    case DOUBLE_TYPE:                            // in checkTypes
      return NULL;

    case BOOL_TYPE:                              // in checkTypes
      return NULL;

    case VOID_TYPE:                              // in checkTypes
      return NULL;

    case TYPE_OF_NULL_TYPE:                      // in checkTypes
      return NULL;

    case ANY_TYPE:                               // in checkTypes
      return NULL;

    case PTR_TYPE:                               // in checkTypes
      pType = (PtrType *) node;
      t = checkTypes (pType->baseType);
      if (pType->baseType && pType->baseType->op == VOID_TYPE && safe) {
        error (pType, "Using 'ptr to void' is unsafe; you must compile with the 'unsafe' option if you wish to do this");
      }
      return NULL;

    case ARRAY_TYPE:                             // in checkTypes
      aType = (ArrayType *) node;
      t = checkTypes (aType->sizeExpr);
      if (t && !isIntType (t)) {
        error (aType, "The array size expression must have type 'int'");
      }
      if ((aType->sizeExpr) && (aType->sizeExpr->op != INT_CONST)) {
        error (aType, "The array size expression must be an integer value determinable at compile-time");
      }
      t = checkTypes (aType->baseType);
      return NULL;

    case RECORD_TYPE:                               // in checkTypes
      rType = (RecordType *) node;
      t = checkTypes (rType->fields);
      return NULL;

    case FUNCTION_TYPE:                               // in checkTypes
      fType = (FunctionType *) node;
      t = checkTypes (fType->parmTypes);
      t = checkTypes (fType->retType);
      setParmSizeInFunctionType (fType);
      return NULL;

    case NAMED_TYPE:                               // in checkTypes
      nType = (NamedType *) node;
      t = checkTypes (nType->typeArgs);
      if (nType->myDef != NULL) {
        switch (nType->myDef->op) {

          case TYPE_PARM:
            if (nType->typeArgs) {
              error (node, "This is the name of a type parameter of this class or interface; this type must not be followed with '[TypeArgs]'");
            }
            break;

          case INTERFACE:
            interface = (Interface *) nType->myDef;
            checkTypeInstantiation (nType->typeArgs,
                                    interface->typeParms,
                                    interface,
                                    nType);
            break;

          case CLASS_DEF:
            cl = (ClassDef *) nType->myDef;
            checkTypeInstantiation (nType->typeArgs,
                                    cl->typeParms,
                                    cl,
                                    nType);
            break;

          case TYPE_DEF:
            if (nType->typeArgs) {
              error (node, "This is the name of a defined type; this type must not be followed with '[TypeArgs]'");
              error2 (nType->myDef, "Here is the type definition");
            }
            break;

          default:
            programLogicError ("NamedType->myDef is an unexpected value");
        }
      }
      return NULL;

    case IF_STMT:                               // in checkTypes
      ifStmt = (IfStmt *) node;
      // t = checkTypes (ifStmt->expr);    // This is done in checkAssignment
      // Make sure the conditional expression has type boolean...
      ifStmt->expr = checkAssignment (
                ifStmt->expr,
                basicBoolType,
                "The conditional expression in this IF statement does not have type 'bool'",
                NULL);
      t = checkTypes (ifStmt->thenStmts);
      t = checkTypes (ifStmt->elseStmts);
      t = checkTypes (ifStmt->next);
      return NULL;

    case ASSIGN_STMT:                               // in checkTypes
      assignStmt = (AssignStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      // printf ("     The lefthand side is:\n");
      // printAst (12, assignStmt->lvalue);
      // printf ("     The righthand side is:\n");
      // printAst (12, assignStmt->expr);
      // pretty (checkTypes (assignStmt->expr));
      t2 = checkTypes (assignStmt->lvalue);
      if (!isLValue (assignStmt->lvalue)) {
        error (assignStmt->lvalue, "The lefthand side of this assignment is not an l-value; it is not something that can be assigned to");
      }
      assignStmt->sizeInBytes = sizeInBytesOfWhole (t2, node, 0);  // wantPrinting = 0
      // printf ("    sizeInBytes to be copied is: %d\n", assignStmt->sizeInBytes);
      // t = checkTypes (assignStmt->expr);    // This is done in checkAssignment
      // Make sure the RHS expression has the right type...
      assignStmt->expr = checkAssignment (
                assignStmt->expr,
                t2,
                "The expression on the righthand side of this assignment does not have the correct type",
                assignStmt);
      // printf ("\n     The type of ");
      // pretty (assignStmt->expr);
      // printf ("          is ");
      // pretty (checkTypes (assignStmt->expr));
      t = checkTypes (assignStmt->next);
      return NULL;

    case CALL_STMT:                               // in checkTypes
      callStmt = (CallStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (callStmt->expr);
      if (!isVoidType (t)) {
        error (node, "This function returns a value; you must either assign this value to some variable or change the function to return nothing");
        errorWithType ("Here is the type returned", t);
      }
      t = checkTypes (callStmt->next);
      return NULL;

    case SEND_STMT:                               // in checkTypes
      sendStmt = (SendStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (sendStmt->expr);
      if (!isVoidType (t)) {
        error (node, "This send-statement returns a value; you must either assign this value to some variable or change the method to return nothing");
        errorWithType ("Here is the type returned", t);
      }
      t = checkTypes (sendStmt->next);
      return NULL;

    case WHILE_STMT:                               // in checkTypes
      whileStmt = (WhileStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      // t = checkTypes (whileStmt->expr);  // Done in checkAssignment below...
      // Make sure the conditional expression has type boolean...
      whileStmt->expr = checkAssignment (
                whileStmt->expr,
                basicBoolType,
                "The conditional expression in this WHILE statement does not have type 'bool'",
                NULL);
      t = checkTypes (whileStmt->stmts);
      t = checkTypes (whileStmt->next);
      return NULL;

    case DO_STMT:                               // in checkTypes
      doStmt = (DoStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (doStmt->stmts);
      // t = checkTypes (doStmt->expr);  // Done in checkAssignment below...
      // Make sure the conditional expression has type boolean...
      doStmt->expr = checkAssignment (
                doStmt->expr,
                basicBoolType,
                "The conditional expression in 'DO stmts UNTIL expr' does not have type 'bool'",
                NULL);
      t = checkTypes (doStmt->next);
      return NULL;

    case BREAK_STMT:                               // in checkTypes
      breakStmt = (BreakStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (breakStmt->next);
      return NULL;

    case CONTINUE_STMT:                               // in checkTypes
      continueStmt = (ContinueStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (continueStmt->next);
      return NULL;

    case RETURN_STMT:                               // in checkTypes
      returnStmt = (ReturnStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      // t = checkTypes (returnStmt->expr);   // Checked below in checkAssignment...
      if (returnStmt->enclosingMethOrFunction) {
        if (returnStmt->enclosingMethOrFunction->op == METHOD) {
          meth = (Method *) returnStmt->enclosingMethOrFunction;
          resultType = meth->retType;
        } else if (returnStmt->enclosingMethOrFunction->op == FUNCTION) {
          fun = (Function *) returnStmt->enclosingMethOrFunction;
          resultType = fun->retType;
        } else {
          programLogicError ("NULL retType; we should have void instead");
        }
        if (isVoidType (resultType)) {
          if (returnStmt->expr) {
            // If problems, resultType will be NULL; isVoid will return true...
            error (returnStmt->expr, "This method/function does not return a value, yet one is provided in this RETURN statement");
          }
        } else {
          if (returnStmt->expr) {
            // Make sure the return expression is compatible with the resultType...
            returnStmt->expr = checkAssignment (
                      returnStmt->expr,
                      resultType,
                      "The value in this RETURN statement does not have the type being returned by this method/function",
                      NULL);
            returnStmt->retSize = sizeInBytesOfWhole (resultType, returnStmt, 0);
          } else {
            programLogicError ("Checked during parsing");
            error (returnStmt->expr, "This method/function returns a value, yet none is provided in this RETURN statement");
          }
        }
      }
      t = checkTypes (returnStmt->next);
      return NULL;

    case FOR_STMT:                               // in checkTypes
      forStmt = (ForStmt *) node;
      //   printf ("\n----------------------  About to process:  ");
      //   pretty (node);
      t2 = checkTypes (forStmt->lvalue);
      if (isPtrType (t2)) {
        if (safe) {
          error (forStmt->lvalue, "Using a ptr as an index in a FOR statement is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
        }
      } else if (!isIntType (t2)) {
        error (forStmt->lvalue, "The index of this FOR statement must have type 'int'");
      }
      if (!isLValue (forStmt->lvalue)) {
        error (forStmt->lvalue, "The index of this FOR statement is not an l-value; it is not something that can be assigned to");
      }
      if (!isPtrType (t2) || !isPtrType (checkTypes (forStmt->expr1))) {
        forStmt->expr1 = checkAssignment (
                forStmt->expr1,
                basicIntType,
                "The starting value in this FOR statement does not have the correct type",
                NULL);
      }
      if (!isPtrType (t2) || !isPtrType (checkTypes (forStmt->expr2))) {
        forStmt->expr2= checkAssignment (
                forStmt->expr2,
                basicIntType,
                "The stopping value in this FOR statement does not have the correct type",
                NULL);
      }
      // t = checkTypes (forStmt->expr3);  // Checked below in checkAssignment
      forStmt->expr3 = checkAssignment (
                forStmt->expr3,
                basicIntType,
                "The increment value in this FOR statement does not have the correct type",
                NULL);
      intConst = (IntConst *) forStmt->expr3;
      if (intConst &&
          (intConst->op == INT_CONST) &&
          (intConst->ivalue < 1)) {
        error (forStmt->expr3, "The step expression in this FOR loop is negative");
      }
      t = checkTypes (forStmt->stmts);
      t = checkTypes (forStmt->next);
      return NULL;

    case SWITCH_STMT:                               // in checkTypes
      switchStmt = (SwitchStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      // t = checkTypes (switchStmt->expr);
      switchStmt->expr = checkAssignment (
                switchStmt->expr,
                basicIntType,
                "The expression in this SWITCH statement must have type 'int'",
                NULL);
      // Run through the case clauses and process them...
      numberOfCases = 0;
      for (cas = switchStmt->caseList; cas != NULL; cas = cas->next) {
        numberOfCases = numberOfCases + 1;
        // t = checkTypes (cas->expr);
        cas->expr = checkAssignment (
                  cas->expr,
                  basicIntType,
                  "The expression following CASE must have type 'int'",
                  NULL);
        cas->expr = (Expression *) evalExprsIn (cas->expr);
        if (cas->expr->op != INT_CONST) {
          error (cas->expr, "The value following CASE must be an integer whose value can be determined at compile-time");
        } else {
          cas->ivalue = ((IntConst *) (cas->expr))->ivalue;
          // printf (" cas->ivalue = %d\n", cas->ivalue);
          if ((cas->ivalue) > (switchStmt->highValue)) {
            // printf (" Setting high...\n");
            switchStmt->highValue = cas->ivalue;
          }
          if (cas->ivalue < switchStmt->lowValue) {
            // printf (" Setting low...\n");
            switchStmt->lowValue = cas->ivalue;
          }
        }
        t = checkTypes (cas->stmts);
      }

      // Next, check to make sure we have no duplicate values.
      // Create a hash table of ptrs to CASE nodes.
      arSize = numberOfCases * 2 + 1;    // Make it twice as big & at least 1.
      caseArray = (Case * *) calloc (4, arSize);
      for (cas = switchStmt->caseList; cas; cas = cas->next) {
        i = cas->ivalue;
        if (i<0) {
          j = (-i) % arSize;
        } else {
          j = i % arSize;
        }
        while (caseArray [j] != NULL) {
          if (caseArray [j] -> ivalue == i) {
            error (caseArray [j], "This CASE clause has the same value as some other CASE in this SWITCH");
            error2 (cas, "Here is another CASE clause with the same value");
            break;
          }
          j = (j + 1) % arSize;
        }
        if (caseArray [j] == NULL) {
          caseArray [j] = cas;
        }
      }
      // Print the array...
      // for (j = 0; j < arSize; j++) {
      //   if (caseArray[j] == NULL) {
      //     printf ("         j= %d       caseArray[j]->ivalue= NULL\n", j);
      //   } else {
      //     printf ("         j= %d       caseArray[j]->ivalue= %d\n",
      //             j, caseArray [j] -> ivalue);
      //   }
      // }
      delete caseArray;

      t = checkTypes (switchStmt->defaultStmts);
      t = checkTypes (switchStmt->next);
      return NULL;

    case TRY_STMT:                               // in checkTypes
      tryStmt = (TryStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (tryStmt->stmts);
      t = checkTypes (tryStmt->catchList);
      // Run thru the catch clauses and make sure each is different...
      tempMap = new Mapping <String, AstNode> (5, NULL);
      for (cat = tryStmt->catchList; cat; cat = cat->next) {
        def = tempMap->find (cat->id);
        if (def) {
          error (cat, "This CATCH clause catches the same error as some other CATCH in this TRY statement");
            error2 (def, "Here is another CATCH clause for the same error");
        } else {
          tempMap->enter (cat->id, cat);
        }
      }
      delete tempMap;
      if (tryStmt->catchList == NULL) {
        error (tryStmt, "This TRY statement has no CATCH clauses; either eliminate 'try' and 'endTry' or add some CATCH clauses");
      }
      t = checkTypes (tryStmt->next);
      return NULL;

    case THROW_STMT:                               // in checkTypes
      throwStmt = (ThrowStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      // t = checkTypes (throwStmt->argList);  // Done within checkArgList...
      if (throwStmt->myDef) {
        checkArgList (throwStmt->argList,
                      throwStmt->myDef->parmList,
                      throwStmt->myDef,
                      throwStmt);
        updateMaxArgBytes (throwStmt->myDef->totalParmSize);
      }
      t = checkTypes (throwStmt->next);
      return NULL;

    case FREE_STMT:                               // in checkTypes
      freeStmt = (FreeStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (freeStmt->expr);
      if (!isPtrType (t)) {
        error (freeStmt->expr, "The expression in this FREE statement is not a pointer");
      }
      if (safe) {
        error (node, "Using the FREE statement is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
      }
      t = checkTypes (freeStmt->next);
      return NULL;

    case DEBUG_STMT:                               // in checkTypes
      debugStmt = (DebugStmt *) node;
      // printf ("\n----------------------  About to process:  ");
      // pretty (node);
      t = checkTypes (debugStmt->next);
      return NULL;

    case CASE:                                // in checkTypes
      programLogicError ("This is handled in SWITCH_STMT");
      return NULL;

    case CATCH:                               // in checkTypes
      cat = (Catch *) node;
      t = checkTypes (cat->parmList);
      // Take a look at the errorDecl...
      if (cat->myDef) {
        // Make sure each parm type matches according to contra-variance...
        protoParm = cat->myDef->parmList;
        subParm = cat->parmList;
        while (1) {
          if ((protoParm == NULL) && (subParm == NULL)) break;
          if ((protoParm == NULL) || (subParm == NULL)) {
            error (cat, "This catch does not have the same number of parameters as the corresponding error declaration");
            error2 (cat->myDef, "Here is the error declaration");
            break;
          }
          if (! assignable (subParm->type, protoParm->type)) {
            error (subParm, "The type of this parameter fails to match the type of the corresponding parameter in the error declaration (perhaps it is not general enough)");
            error2 (cat->myDef, "Here is the error declaration");
            errorWithType ("The type of this parameter in the error declaration is",
                           protoParm->type);
            errorWithType ("The type of this parameter in the catch clause is",
                           subParm->type);
          }
          protoParm = (Parameter *) protoParm->next;
          subParm = (Parameter *) subParm->next;
        }
      }
      t = checkTypes (cat->stmts);
      // At this point the "offset"s have been assigned sequentially; these
      // args will be pushed using these offsets during a throw.  Go through
      // the parms and copy all offsets into the "throwSideOffset"s.
      for (parm = cat->parmList; parm; parm = (Parameter *) parm->next) {
        parm->throwSideOffset = parm->offset;
      }

/**********
      // Now add all the parameters here to the list of LOCALS for this function
      // or method.  From now on, they'll be treated as locals.

This is a bad idea.  It makes it impossible to walk the list of parameters for
a catch stmt; we just keep going and hit other stuff in the list.

      printf ("Processing catch %s...\n", cat->id->chars);
      if (cat->parmList) {
        lastParm = cat->parmList;
        while (1)  {
          lastParm->throwSideOffset = lastParm->offset;
          printf ("  Processing parm %s   offset=%d   throwSideOffset=%d\n",
                  lastParm->id->chars,
                  lastParm->offset,
                  lastParm->throwSideOffset);
          if (lastParm->next == NULL) break;
          lastParm = (Parameter *) (lastParm->next);
        }
        if (cat->enclosingMethOrFunction->op = FUNCTION) {
          fun = (Function *) (cat->enclosingMethOrFunction);
          lastParm->next = fun->locals;
          fun->locals = (Local *) cat->parmList;
        } else if (cat->enclosingMethOrFunction->op = METHOD) {
          meth = (Method *) (cat->enclosingMethOrFunction);
          lastParm->next = meth->locals;
          meth->locals = (Local *) cat->parmList;
        } else {
          programLogicError ("cat->enclosingMethOrFunction is not FUNCTION or METHOD");
        }
      }
**********/


      t = checkTypes (cat->next);
      return NULL;

    case GLOBAL:                               // in checkTypes
      global = (Global *) node;
      // printf ("GLOBAL %s\n", global->id->chars);
      t2 = checkTypes (global->type);
      // t = checkTypes (global->initExpr);    // done in checkAssignment
      // First, check the expression to make sure it is static...
      // Make sure the initializing expression (if any) has the correct type...
      if (global->initExpr) {
        global->initExpr = checkAssignment (
                global->initExpr,
                global->type,
                "The initializing expression does not have the correct type",
                NULL);
      }
      global->initExpr = (Expression *) evalExprsIn (global->initExpr);
      checkStaticData (global->initExpr, global);
      t = checkTypes (global->next);
      return t2;

    case LOCAL:                               // in checkTypes
      local = (Local *) node;
      t = checkTypes (local->type);
      local->sizeInBytes = sizeInBytesOfWhole (local->type, local, 1);
      // t2 = checkTypes (local->initExpr);    // done in checkAssignment
      // Make sure the initializing expression (if any) has the correct type...
      if (local->initExpr) {
        local->initExpr = checkAssignment (
                local->initExpr,
                local->type,
                "The initializing expression does not have the correct type",
                NULL);
      }
      t = checkTypes (local->next);
      return NULL;

    case PARAMETER:                               // in checkTypes
      parm = (Parameter *) node;
      t = checkTypes (parm->type);
      parm->sizeInBytes = sizeInBytesOfWhole (parm->type, parm, 1);
      t = checkTypes (parm->next);
      return NULL;

    case CLASS_FIELD:                               // in checkTypes
      classField = (ClassField *) node;
      t = checkTypes (classField->type);
      t = checkTypes (classField->next);
      return NULL;

    case RECORD_FIELD:                               // in checkTypes
      recordField = (RecordField *) node;
      t = checkTypes (recordField->type);
      t = checkTypes (recordField->next);
      return NULL;

    case INT_CONST:                               // in checkTypes
      return basicIntType;

    case DOUBLE_CONST:                               // in checkTypes
      doubleConst = (DoubleConst *) node;
      linkDouble (doubleConst);
      return basicDoubleType;

    case CHAR_CONST:                               // in checkTypes
      return basicCharType;

    case STRING_CONST:                               // in checkTypes
      stringConst = (StringConst *) node;
      if (stringConst->nameOfConstant == NULL) {
        stringConst->next = stringList;
        stringList = stringConst;
        stringConst->nameOfConstant = newName ("StringConst");
      }
      return basicCharArrayPtrType;

    case BOOL_CONST:                               // in checkTypes
      return basicBoolType;

    case NULL_CONST:                               // in checkTypes
      return basicTypeOfNullType;



    case CALL_EXPR:                               // in checkTypes
      callExpr = (CallExpr *) node;
        // printf ("callExpr->id = %s\n", callExpr->id->chars);
        // printf ("callExpr->id->primitiveSymbol = %s\n",
        // symbolName (callExpr->id->primitiveSymbol));

      // Identify primitive functions, and check their types...
      switch (callExpr->id->primitiveSymbol) {

        // Check for primitive functions...

        case INT_TO_DOUBLE:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicIntType,
                "Primitive INT_TO_DOUBLE expects its argument to be of type int",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_INT_TO_DOUBLE;
          } else {
            error (callExpr, "Primitive INT_TO_DOUBLE expects exactly one argument");
          }
          // updateMaxArgBytes (8); // allocate frame space, in case we insert a real call
          return basicDoubleType;

       case DOUBLE_TO_INT:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicDoubleType,
                "Primitive DOUBLE_TO_INT expects its argument to be of type double",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_DOUBLE_TO_INT;
          } else {
            error (callExpr, "Primitive DOUBLE_TO_INT expects exactly one argument");
          }
          // updateMaxArgBytes (8); // allocate frame space, in case we insert a real call
          return basicIntType;

       case INT_TO_CHAR:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicIntType,
                "Primitive INT_TO_CHAR expects its argument to be of type int",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_INT_TO_CHAR;
          } else {
            error (callExpr, "Primitive INT_TO_CHAR expects exactly one argument");
          }
          // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
          return basicCharType;

       case CHAR_TO_INT:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicCharType,
                "Primitive CHAR_TO_INT expects its argument to be of type char",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_CHAR_TO_INT;
          } else {
            error (callExpr, "Primitive CHAR_TO_INT expects exactly one argument");
          }
          // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
          return basicIntType;

       case PTR_TO_BOOL:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicAnyPtrType,
                "Primitive PTR_TO_BOOL expects its argument to be of type ptr",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_PTR_TO_BOOL;
          } else {
            error (callExpr, "Primitive PTR_TO_BOOL expects exactly one argument");
          }
          // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
          return basicBoolType;

       case POS_INF:                      // in checkTypes for CALL_EXPR...
          programLogicError ("POS_INF was eliminated in evalExprs");
          // if (argCount (callExpr->argList) != 0) {
          //   error (callExpr, "Primitive POS_INF expects no arguments");
          // }
          // callExpr->primitiveSymbol = POS_INF;
          // updateMaxArgBytes (8); // allocate frame space, in case we insert a real call
          // return basicDoubleType;

       case NEG_INF:                      // in checkTypes for CALL_EXPR...
          programLogicError ("NEG_INF was eliminated in evalExprs");
          // if (argCount (callExpr->argList) != 0) {
          //   error (callExpr, "Primitive NEG_INF expects no arguments");
          // }
          // callExpr->primitiveSymbol = NEG_INF;
          // updateMaxArgBytes (8); // allocate frame space, in case we insert a real call
          // return basicDoubleType;

       case NEG_ZERO:                      // in checkTypes for CALL_EXPR...
          programLogicError ("NEG_ZERO was eliminated in evalExprs");
          // if (argCount (callExpr->argList) != 0) {
          //   error (callExpr, "Primitive NEG_ZERO expects no arguments");
          // }
          // callExpr->primitiveSymbol = NEG_ZERO;
          // updateMaxArgBytes (8); // allocate frame space, in case we insert a real call
          // return basicDoubleType;

       case I_IS_ZERO:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicIntType,
                "Primitive I_IS_ZERO expects its argument to be of type int",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_I_IS_ZERO;
          } else {
            error (callExpr, "Primitive I_IS_ZERO expects exactly one argument");
          }
          // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
          return basicBoolType;

       case I_NOT_ZERO:                      // in checkTypes for CALL_EXPR...
          if (argCount (callExpr->argList) == 1) {
            callExpr->argList->expr = checkAssignment (
                callExpr->argList->expr,
                basicIntType,
                "Primitive I_NOT_ZERO expects its argument to be of type int",
                NULL);
            callExpr->primitiveSymbol = PRIMITIVE_I_NOT_ZERO;
          } else {
            error (callExpr, "Primitive I_NOT_ZERO expects exactly one argument");
          }
          // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
          return basicBoolType;

        default:
          // Continue below, checking for user-defined functions...
          break;
      }

      if (callExpr->myDef) {
        if ((callExpr->myDef->op == LOCAL) ||
            (callExpr->myDef->op == GLOBAL) ||
            (callExpr->myDef->op == PARAMETER) ||
            (callExpr->myDef->op == CLASS_FIELD)) {
          varDecl = (Local *) callExpr->myDef;
          fType = getFunctionType (varDecl->type);
          if (fType != NULL) {
            // Make sure each arg type matches the corresponding parm type...
            checkArgList2 (callExpr->argList,
                          fType->parmTypes,
                          fType,
                          callExpr);
            // Next line untested
            callExpr->retSize = sizeInBytesOfWhole (fType->retType,
                                                    fType->retType,
                                                    0);    // wantPrinting = 0
            updateMaxArgBytes (fType->totalParmSize);  // includes return value
            return fType->retType;
          } else {
            error (callExpr, "The thing you are invoking here does not have type 'ptr to function'");
            error2 (varDecl, "Here is its definition");
            return NULL;
          }
        } else if (callExpr->myDef->op == FUNCTION_PROTO) {
          functionProto = (FunctionProto *) callExpr->myDef;
          // Make sure each arg type matches the corresponding parm type...
          checkArgList (callExpr->argList,
                        functionProto->parmList,
                        functionProto,
                        callExpr);
          // printf ("Calling updateMaxArgBytes (%d) -- Function %s\n",
          //         functionProto->totalParmSize, functionProto->id->chars);
          updateMaxArgBytes (functionProto->totalParmSize);
          callExpr->retSize = sizeInBytesOfWhole (functionProto->retType,
                                                 functionProto->retType,
                                                 0);    // wantPrinting = 0
          updateMaxArgBytes (callExpr->retSize);
          return functionProto->retType;
        } else {
          programLogicError ("Already checked that callExpr->myDef was the right sort of thing");
        }

      } else {
        error (callExpr, "There is no function with this name (nor is there a variable with this name known at this point)");
        return basicVoidType;
      }



    case SEND_EXPR:                               // in checkTypes
      sendExpr = (SendExpr *) node;

      //    printf ("\nPROCESSING SEND EXPR ======>  ");
      //    pretty (sendExpr);

      // If the recvr is "super"...
      if (sendExpr->receiver->op == SUPER_EXPR) {
        recvrIsSuper = 1;

      // Else, if the recvr is "*super"...
      } else if ((sendExpr->receiver->op == SEND_EXPR) &&
                 (((SendExpr *) sendExpr->receiver)->selector->primitiveSymbol ==
                                                          UNARY_STAR) &&
                 (((SendExpr *) sendExpr->receiver)->receiver->op == SUPER_EXPR)) {
          error (sendExpr->receiver,
                 "Dereferencing super is not allowed; either use '* self' or use super directly, as in 'super.meth (x)' or 'super at: x put: y' or 'super ++ x'");
          return NULL;

      } else {
        recvrIsSuper = 0;
      }

      // Determine the type of the receiver expression...
      if (recvrIsSuper) {
        if (currentClass == NULL) {
          error (sendExpr->receiver,
                 "Super may only be used within a method, not within a function");
        } else {
          t = currentClass->typeOfSelf;
        }
      } else {
        t = checkTypes (sendExpr->receiver);
      }
      t = resolveNamedType (t);
      if (t == NULL) return NULL;

      // printf ("Processing sendExpr = ");
      // pretty (sendExpr);
      // printf ("receiver type = ");
      // pretty (t);
      // printf ("Processing sendExpr...\n");
      // printAst (8, sendExpr);

      // This will be set if we have "*any".  If any doesn't understand prefix *,
      // then we'll print out a special error message such as "deref is invalid".
      looksLikeDeref = 0;

      // See if we are dealing with a primitive message, such as "+"...
      switch (sendExpr->selector->primitiveSymbol) {

        // Check for primitive messages...

        case PLUS:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive (sendExpr, t, PRIMITIVE_I_ADD, PRIMITIVE_D_ADD);
          if (resultType) return resultType;
          // Check for the special case of ptr+int...
          if (isPtrType (t)) {
            if (argCount (sendExpr->argList) == 1) {
              t2 = checkTypes (sendExpr->argList->expr);
              if (isIntType (t2)) {
                if (safe) {
                  error (sendExpr, "Adding ptrs to ints is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
                }
                sendExpr->primitiveSymbol = PRIMITIVE_I_ADD;
                return t;
              } else {
                error (sendExpr, "This built-in operator expects the right operand to be an int");
                return t;
              }
            }
          }
	  break;

        case MINUS:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive (sendExpr, t, PRIMITIVE_I_SUB, PRIMITIVE_D_SUB);
          if (resultType) return resultType;
          // Check for the special case of ptr-int or ptr-ptr...
          if (isPtrType (t)) {
            if (argCount (sendExpr->argList) == 1) {
              t2 = checkTypes (sendExpr->argList->expr);
              if (isIntType (t2)) {
                if (safe) {
                  error (sendExpr, "Subtracting an int from a ptr is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
                }
                sendExpr->primitiveSymbol = PRIMITIVE_I_SUB;
                return t;
              } else if (isPtrType (t2)) {
                if (safe) {
                  error (sendExpr, "Subtracting a ptr from a ptr is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
                }
                sendExpr->primitiveSymbol = PRIMITIVE_I_SUB;
                return basicIntType;
              } else {
                error (sendExpr, "This built-in operator expects the right operand to be an int");
                return t;
              }
            }
          }
	  break;

        case UNARY_MINUS:                            // in checkTypes for SEND_EXPR...
          // Check for the special case of -char, -int, or -double...
          if (argCount (sendExpr->argList) == 0) {
            if (isIntType (t)) {
              sendExpr->primitiveSymbol = PRIMITIVE_I_NEG;
              return t;
            } else if (isDoubleType (t)) {
              sendExpr->primitiveSymbol = PRIMITIVE_D_NEG;
              return t;
            } else if (isCharType (t)) {
              sendExpr->receiver = insertCharToInt (sendExpr->receiver);
              sendExpr->primitiveSymbol = PRIMITIVE_I_NEG;
              return basicIntType;
            }
          }
	  break;

        case UNARY_STAR:                            // in checkTypes for SEND_EXPR...
          looksLikeDeref = 1;
          pType = getPtrType (t);
          if (pType) {
            sendExpr->primitiveSymbol = PRIMITIVE_DEREF;
            return pType->baseType;
          }
	  break;

        case STAR:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive (sendExpr, t, PRIMITIVE_I_MUL, PRIMITIVE_D_MUL);
          if (resultType) return resultType;
	  break;

        case SLASH:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive (sendExpr, t, PRIMITIVE_I_DIV, PRIMITIVE_D_DIV);
          if (resultType) return resultType;
	  break;

        case PERCENT:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_REM);
          if (resultType) return resultType;
	  break;

        case LESS_LESS:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_SLL);
          if (resultType) return resultType;
	  break;

        case GREATER_GREATER:                       // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_SRA);
          if (resultType) return resultType;
	  break;

        case GREATER_GREATER_GREATER:                // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_SRL);
          if (resultType) return resultType;
	  break;

        case BAR:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_OR);
          if (resultType) return resultType;
	  break;

        case CARET:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_XOR);
          if (resultType) return resultType;
	  break;

        case UNARY_AMP:                            // in checkTypes for SEND_EXPR...
          if (sendExpr->receiver->op == VARIABLE_EXPR) {
            var = (VariableExpr *) sendExpr->receiver;
            functionProto = (FunctionProto *) var->myDef;
            if ((functionProto != NULL) &&
                (functionProto->op == FUNCTION_PROTO)) {
              error (sendExpr, "Taking the address of a function is not allowed; just use the function name directly and a 'ptr to function' will be understood");
            }
          }
          if (isLValue (sendExpr->receiver)) {
            sendExpr->primitiveSymbol = PRIMITIVE_ADDRESS_OF;
            pType = new PtrType ();
            pType->positionAt (sendExpr);
            pType->baseType = t;
            // If we are compiling with "safe"
            if (safe) {
              if (sendExpr->receiver->op == FIELD_ACCESS) {
                error (sendExpr, "Taking the address of field within a record or object is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
              }
              if (sendExpr->receiver->op == ARRAY_ACCESS) {
                error (sendExpr, "Taking the address of an element within an array is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
              }
              if (sendExpr->receiver->op == VARIABLE_EXPR) {
                var = (VariableExpr *) sendExpr->receiver;
                varDecl = (VarDecl *) var->myDef;
                if ((varDecl != NULL) &&
                    (varDecl->op != GLOBAL)) {
                  error (sendExpr, "Taking the address of a non-global variable is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
                }
              }
            }
            return pType;
          }
	  break;

        case AMP:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_AND);
          if (resultType) return resultType;
	  break;

        case UNARY_BANG:                            // in checkTypes for SEND_EXPR...
          // Check for the special case of !int or !bool...
          if (argCount (sendExpr->argList) == 0) {
            if (isIntType (t)) {
              sendExpr->primitiveSymbol = PRIMITIVE_I_NOT;
              return basicIntType;
            } else if (isBoolType (t)) {
              sendExpr->primitiveSymbol = PRIMITIVE_B_NOT;
              return basicBoolType;
            } else if (isPtrType (t)) {
              sendExpr->receiver = insertPtrToBool (sendExpr->receiver);
              sendExpr->primitiveSymbol = PRIMITIVE_B_NOT;
              return basicBoolType;
            }
          }
          break;

        case BAR_BAR:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive4 (sendExpr, t, PRIMITIVE_B_OR);
          if (resultType) return resultType;
	  break;

        case AMP_AMP:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive4 (sendExpr, t, PRIMITIVE_B_AND);
          if (resultType) return resultType;
	  break;

        case LESS:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive3 (sendExpr, t, PRIMITIVE_I_LT, PRIMITIVE_D_LT);
          if (resultType) return resultType;
	  break;

        case LESS_EQUAL:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive3 (sendExpr, t, PRIMITIVE_I_LE, PRIMITIVE_D_LE);
          if (resultType) return resultType;
	  break;

        case GREATER:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive3 (sendExpr, t, PRIMITIVE_I_GT, PRIMITIVE_D_GT);
          if (resultType) return resultType;
	  break;

        case GREATER_EQUAL:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive3 (sendExpr, t, PRIMITIVE_I_GE, PRIMITIVE_D_GE);
          if (resultType) return resultType;
	  break;

        case EQUAL_EQUAL:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive5 (sendExpr, t, PRIMITIVE_I_EQ, PRIMITIVE_D_EQ, PRIMITIVE_B_EQ, PRIMITIVE_OBJECT_EQ);
          if (resultType) return resultType;
	  break;

        case NOT_EQUAL:                            // in checkTypes for SEND_EXPR...
          resultType = checkForPrimitive5 (sendExpr, t, PRIMITIVE_I_NE, PRIMITIVE_D_NE, PRIMITIVE_B_NE, PRIMITIVE_OBJECT_NE);
          if (resultType) return resultType;
	  break;

        default:                            // in checkTypes for SEND_EXPR...
          // Continue below, checking for user-defined operators...
          break;
      }

      // printf ("\nSEND EXPR = ");
      // pretty (sendExpr);
      // printf ("    receiver type = ");
      // pretty (t);

      // If this is a ptr, insert a dereference node...
      pType = getPtrType (t);
      if (pType != NULL) {
        sendExpr->receiver = insertDeref (sendExpr->receiver);
        t = pType->baseType;
      }

      nameWithoutSuper = sendExpr->selector;

      // printf ("  At this point sendExpr =");
      // pretty (sendExpr);

      // If we are sending to "super", append "_super_" to the selector...
      if (recvrIsSuper) {
        newSelector = addSuperTo (nameWithoutSuper);
      } else {
        newSelector = nameWithoutSuper;
      }

      // printf ("Binding send.... selector = %s\n", sendExpr->selector->chars);
      // printf ("           newSelector = %s\n", newSelector->chars);

      // Make sure it is a class or interface...
      t = resolveNamedType (t);
      if (t == NULL) return NULL;
      if (t->op == NAMED_TYPE) {
        nType = (NamedType *) t;
        if (nType->myDef == NULL) return NULL;

        // If the reciever is a class...
        if (nType->myDef->op == CLASS_DEF) {
          cl = (ClassDef *) nType->myDef;

          // Build the substitution...
          if (nType->subst == NULL) {
            nType->subst = buildSubstitution (cl->typeParms, nType->typeArgs);
          }
          // printf ("  subst = \n");
          // if (nType->subst) {
          //   nType->subst->print (6);
          // } else {
          //   printf ("NULL\n");
          // }

          // Look for a MethodProto in this class's "selectorMapping"...
          //     printf ("SelectorMapping for class %s...\n", cl->id->chars);
          //     cl->selectorMapping->print(0);
          methodProto = cl->selectorMapping->find (newSelector);
          // If the send is to "super", we must have a local method in this class
          // by the same name and we must have an overriden version of this method...
          if (recvrIsSuper &&
              ((cl->localMethodMapping->find (nameWithoutSuper) == NULL) ||
               (methodProto == NULL))) {
              error (sendExpr, "This message was not overridden in this class; use 'self' not 'super'");
          }
          // printf (" methodProto before subst = ");
          // pretty (methodProto);
          // printf ("  methodProto with subst = ");
          // pretty (copyTypeWithSubst (methodProto, nType->subst));
          // printf ("\n\n");

          // Check that the send expression satisfies the methodProto...
          return checkMessageSend (methodProto, sendExpr, t, nType->subst);

        // If the reciever is an interface...
        } else if (nType->myDef->op == INTERFACE) {
          interface = (Interface *) nType->myDef;

          // Build the substitution...
          if (nType->subst == NULL) {
            nType->subst = buildSubstitution (interface->typeParms, nType->typeArgs);
          }

          // Look for a MethodProto in this interface's "selectorMapping"...
          //     printf ("SelectorMapping for interface %s...\n", interface->id->chars);
          //     interface->selectorMapping->print(0);
          methodProto = interface->selectorMapping->find (newSelector);

          // Check that the send expression satisfies the methodProto...
          return checkMessageSend (methodProto, sendExpr, t, nType->subst);
        }
      }

      // It must be an error...
      if (looksLikeDeref) {
        error (sendExpr, "The dereference operator may only be applied to ptr types (Objects may understand a prefix * method, but this is not an object)");
        errorWithType ("The type of the receiver is", t);
        return NULL;
      }
      error (sendExpr, "This message is not understood by this type of receiver (1)");
      errorWithType ("The type of the receiver is", t);
      return NULL;

    case SELF_EXPR:                               // in checkTypes
      selfExpr = (SelfExpr *) node;
      if (currentClass == NULL) {
        error (node, "Self may only be used within a method, not within a function");
      } else {
        return currentClass->typeOfSelf;
      }
      return NULL;

    case SUPER_EXPR:                               // in checkTypes
      error (node, "Super may only be used as the target of a message send; use 'self' instead");
      return NULL;

    case FIELD_ACCESS:                               // in checkTypes
      fieldAccess = (FieldAccess *) node;
      // printf ("FIELD ACCESS: ");
      // pretty (fieldAccess);
      t = checkTypes (fieldAccess->expr);
      // errorWithType ("FIELD ACCESS: Here is the starting type...", t);
      // If this is a ptr, insert a dereference node...
      pType = getPtrType (t);
      if (pType != NULL) {
        fieldAccess->expr = insertDeref (fieldAccess->expr);
        t = pType->baseType;
      }
      t = resolveNamedType (t);
      if (t == NULL) return NULL;

      // See if this is a record.field access...
      if (t->op == RECORD_TYPE) {
        rType = (RecordType *) t;
        recordField = rType->fieldMapping->find (fieldAccess->id);
        if (recordField == NULL) {
          error (node, "In field access 'record.field', this field is not in this record");
          return NULL;
        }
        fieldAccess->offset = recordField->offset;
        return recordField->type;
      }

      // See if this is a class.field access...
      if (t->op == NAMED_TYPE) {
        nType = (NamedType *) t;
        cl = (ClassDef *) nType->myDef;
        if (cl == NULL) return NULL;
        if (cl->op == CLASS_DEF) {   // Actual type may be a subtype, but that's okay...
          // Find the field...
          classField = (ClassField *) cl->classMapping->find (fieldAccess->id);
          if ((classField == NULL) || (classField->op != CLASS_FIELD)) {
            error (node, "In field access 'ObjectExpr.field', this field is not in this class");
            errorWithType ("The class of ObjectExpr is", nType);
            return NULL;
          }
          fieldAccess->offset = classField->offset;
          // Build the substitution...
          if (nType->subst == NULL) {
            nType->subst = buildSubstitution (cl->typeParms, nType->typeArgs);
          }
          // printf ("  subst = \n");
          // if (nType->subst) {
          //   nType->subst->print (6);
          // } else {
          //   printf ("NULL\n");
          // }
          // printf (" type before subst = ");
          // pretty (classField->type);
          // printf ("  type with subst = ");
          // pretty (copyTypeWithSubst (classField->type, nType->subst));
          // printf ("\n\n");
          // Apply the substitution to this field's type and return the result...
          return copyTypeWithSubst (classField->type, nType->subst);
        }
      }
      error (fieldAccess->expr, "In 'expr.field' the expression before the '.' is not a 'class', 'ptr to class', 'record', or 'ptr to record'");
      errorWithType ("The type of the expression is", t);
      return NULL;

    case ARRAY_ACCESS:                               // in checkTypes
      arrayAccess = (ArrayAccess *) node;
      t = checkTypes (arrayAccess->arrayExpr);
      // If this is a ptr, insert a dereference node...
      pType = getPtrType (t);
      if (pType != NULL) {
        arrayAccess->arrayExpr = insertDeref (arrayAccess->arrayExpr);
        t = pType->baseType;
      }
      // t2 = checkTypes (arrayAccess->indexExpr);  // checkTypes called in checkAssgnmt
      aType = getArrayType (t);
      if (aType == NULL) {
        error (arrayAccess->arrayExpr, "In array access 'a[i]', the expression 'a' is not an array");
        return NULL;
      }
      arrayAccess->indexExpr = checkAssignment (
                arrayAccess->indexExpr,
                basicIntType,
                "In array access 'a[i]', the index expression 'i' is not an int",
                NULL);
      arrayAccess->sizeOfElements = aType->sizeOfElements;
      return aType->baseType;

    case CONSTRUCTOR:                               // in checkTypes
      constructor = (Constructor *) node;
      t = checkConstructor (constructor);
      if (constructor->allocKind == NEW) {
        return t;
      } else if (constructor->allocKind == ALLOC) {
        if (t == NULL) return NULL;
        pType = new PtrType ();
        pType->positionAt (constructor);
        pType->baseType = t;
        return pType;
      } else {
        programLogicError ("Unexpected constructor->allocKind in checkTypes");
      }

    case CLOSURE_EXPR:                               // in checkTypes
      closureExpr = (ClosureExpr *) node;
      // Skip checkTypes, since we'll walk the function when we do header->closures...
      // t = checkTypes (closureExpr->function);
      // Look at the function and construct a new 'ptr to function...' type.
      fun = closureExpr->function;
      fType = new FunctionType ();
      fType->positionAt (node);
      // Build a list of TypeArgs...
      typeArgList = NULL;
      for (parm = fun->parmList; parm; parm = (Parameter *) parm->next) {
        typeArg = new TypeArg ();
        typeArg->positionAt (parm);
        typeArg->type = parm->type;
        typeArg->next = typeArgList;
        typeArgList = typeArg;
      }
      // Reverse the list and set fType->parms to point to it...
      typeArg = typeArgList;
      while (typeArg) {
        nextTypeArg = typeArg->next;
        typeArg->next = fType->parmTypes;
        fType->parmTypes = typeArg;
        typeArg = nextTypeArg;
      }
      // Fill in fType->retType...
      fType->retType = fun->retType;

      pType = new PtrType ();
      pType->positionAt (node);
      pType->baseType = fType;
      return pType;

    case VARIABLE_EXPR:                               // in checkTypes
      var = (VariableExpr *) node;
      if (var->myDef) {
        varDecl = (VarDecl *) var->myDef;

        // If we have a normal variable reference, then return the var's type...
        if ((varDecl->op == LOCAL) ||
            (varDecl->op == GLOBAL) ||
            (varDecl->op == PARAMETER) ||
            (varDecl->op == CLASS_FIELD)) {
          return varDecl->type;

        } else if (varDecl->op == RECORD_FIELD) {
          programLogicError ("Lone record fields will be undefined variables");

        // Else if this variable names a function, build a type such as:
        //          ptr to function (int, int) returns double
        } else if (varDecl->op == FUNCTION_PROTO) {
          functionProto = (FunctionProto *) varDecl;
          fType = new FunctionType ();
          fType->positionAt (node);
          // Build a list of TypeArgs...
          typeArgList = NULL;
          for (parm = functionProto->parmList; parm; parm = (Parameter *) parm->next) {
            typeArg = new TypeArg ();
            typeArg->positionAt (parm);
            typeArg->type = parm->type;
            typeArg->next = typeArgList;
            typeArgList = typeArg;
          }
          // Reverse the list and set fType->parms to point to it...
          typeArg = typeArgList;
          while (typeArg) {
            nextTypeArg = typeArg->next;
            typeArg->next = fType->parmTypes;
            fType->parmTypes = typeArg;
            typeArg = nextTypeArg;
          }
          // Fill in fType->retType...
          fType->retType = functionProto->retType;
          // Build a ptr-to node and return it...
          pType = new PtrType ();
          pType->positionAt (node);
          pType->baseType = fType;
          // printf ("Use of function %s as a variable; type of expr is ",
          //            var->id->chars);
          // pretty (pType);
          return pType;
          
        } else {
//qqqqqqqqq
          // This case can occur as a result of previous errors, e.g., a constant whose
          // value cannot be determined at compile-time.
          error (var, "Something is wrong with this expression; there should be another error message concerning the problem");
        }
      }

      return NULL;

    case AS_PTR_TO_EXPR:                               // in checkTypes
      asPtrToExpr = (AsPtrToExpr *) node;
      if (safe) {
        error (node, "Using 'asPtrTo' is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
      }
      t = checkTypes (asPtrToExpr->expr);
      t2 = checkTypes (asPtrToExpr->type);
      pType = new PtrType ();
      pType->positionAt (node);
      pType->baseType = asPtrToExpr->type;
      if (assignable (pType, t)) {
        if (! isPtrToVoidType (t)) {
          error (node, "You do not need 'asPtrTo' here; the expression already has the desired type");
        }
      }
      if (!isIntType (t) && !isPtrType (t)) {
        error (node, "You may only coerce a ptr or an int into a ptr type");
        errorWithType ("The type of this expression is", t);
      }
      return pType;

    case AS_INTEGER_EXPR:                               // in checkTypes
      asIntegerExpr = (AsIntegerExpr *) node;
      t = checkTypes (asIntegerExpr->expr);
      // if (safe) {
      //   error (node, "Using 'asInteger' is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
      // }
      t = checkTypes (asIntegerExpr->expr);
      if (!isPtrType (t)) {
        error (node, "You may only coerce a ptr into an int");
        errorWithType ("The type of this expression is", t);
      }
      return basicIntType;

    case ARRAY_SIZE_EXPR:                               // in checkTypes
      arraySizeExpr = (ArraySizeExpr *) node;
      t = checkTypes (arraySizeExpr->expr);
      // If this is a ptr, insert a dereference node...
      pType = getPtrType (t);
      if (pType != NULL) {
        arraySizeExpr->expr = insertDeref (arraySizeExpr->expr);
        t = pType->baseType;
      }
      if (getArrayType (t) == NULL) {
        error (arraySizeExpr->expr, "In 'Expr sizeOf', this expression is not an array");
      }
      return basicIntType;

    case IS_INSTANCE_OF_EXPR:                               // in checkTypes
      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      t = checkTypes (isInstanceOfExpr->expr);
      // If this is a ptr, insert a dereference node...
      pType = getPtrType (t);
      if (pType != NULL) {
        isInstanceOfExpr->expr = insertDeref (isInstanceOfExpr->expr);
        t = pType->baseType;
      }

      // Make sure the type of the expression is a class or interface...
      if (!isObjectType (t)) {
        error (node, "In 'EXPR isInstanceOf TYPE', EXPR is not an object; it is impossible to check its type at runtime");
        errorWithType ("The type of 'expr' is", t);
      }

      // Make sure the type on the right is a legal, concrete class...
      checkTypes (isInstanceOfExpr->type);
      checkConcreteClass (
            isInstanceOfExpr->type,
            isInstanceOfExpr->type,
            "In 'EXPR isInstanceOf TYPE', the TYPE must be a concrete class");

      isInstanceOfExpr->classDef = getClassDef (isInstanceOfExpr->type);
      if (isInstanceOfExpr->classDef == NULL) {
        error (isInstanceOfExpr->type,
               "In 'EXPR isInstanceOf TYPE', the TYPE is not a class");
      } else if (isInstanceOfExpr->classDef->typeParms != NULL) {
        error (isInstanceOfExpr, "IMPLEMENTATION RESTRICTION: The type after 'isInstanceOf' must not be parameterized");
        errorWithType ("The type after 'isInstanceOf' is", isInstanceOfExpr->type);
      }

      // OBSOLETE VERSION: "isExactClass" is not quite good enough...
      // if (!isExactClass (isInstanceOfExpr->type)) {
      //   error (node, "In 'expr isInstanceOf type', the 'type' must be a class name");
      //   errorWithType ("The type after 'isInstanceOf' is", isInstanceOfExpr->type);
      // }

      return basicBoolType;

    case IS_KIND_OF_EXPR:                               // in checkTypes
      isKindOfExpr = (IsKindOfExpr *) node;
      t = checkTypes (isKindOfExpr->expr);
      // If this is a ptr, insert a dereference node...
      pType = getPtrType (t);
      if (pType != NULL) {
        isKindOfExpr->expr = insertDeref (isKindOfExpr->expr);
        t = pType->baseType;
      }

      // Make sure the type of the expression is a class or interface...
      if (! isObjectType (t)) {
        error (node, "In 'EXPR isKindOf TYPE', EXPR is not an object; it is impossible to check its type at runtime");
        errorWithType ("The type of 'expr' is", t);
      }

      // Make sure the type on the right is a legal, concrete class or interface...
      checkTypes (isKindOfExpr->type);
      checkConcreteClassOrInterface (
           isKindOfExpr->type,
           isKindOfExpr->type,
           "In 'EXPR isKindOf TYPE', the TYPE must be a concrete class or interface");
      isKindOfExpr->classOrInterface = getClassDef (isKindOfExpr->type);
      if (isKindOfExpr->classOrInterface == NULL) {
        isKindOfExpr->classOrInterface = getInterface (isKindOfExpr->type);
      }
      if (isKindOfExpr->classOrInterface == NULL) {
        error (isKindOfExpr->type,
               "In 'EXPR isKindOf TYPE', the TYPE is not a class or interface");
      }

      return basicBoolType;

    case SIZE_OF_EXPR:                               // in checkTypes
      sizeOfExpr = (SizeOfExpr *) node;
      t = checkTypes (sizeOfExpr->type);
      // We should have eliminated this node by now in evalExprs.  If it still exists,
      // then there must have been a problem in determining the value...
      error (node, "Unable to determine at compile-time the size of this type of data");
      return NULL;

    case DYNAMIC_CHECK:
      dynamicCheck = (DynamicCheck *) node;
      return checkTypes (dynamicCheck->expr);

    case ARGUMENT:                               // in checkTypes
      programLogicError ("We will do each argument in a loop elsewhere");
      arg = (Argument *) node;
      t = checkTypes (arg->expr);
      t = checkTypes (arg->next);
      return NULL;

    case COUNT_VALUE:                               // in checkTypes
      programLogicError ("We are checking CountValues within the constructor case");
      countValue = (CountValue *) node;
      t = checkTypes (countValue->count);
      t = checkTypes (countValue->value);
      t = checkTypes (countValue->next);
      return NULL;

    case FIELD_INIT:                               // in checkTypes
      programLogicError ("We are checking FieldInits within the constructor case");
      fieldInit = (FieldInit *) node;
      t = checkTypes (fieldInit->expr);
      t = checkTypes (fieldInit->next);
      return NULL;

    default:                               // in checkTypes

      printf ("\nnode->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in checkTypes");
  }
  programLogicError ("All cases should contain returns in checkTypes");
}



// checkConstructor (constructor)  -->  Type
//
// This routine checks the types in a constructor and returns either
// an ARRAY_TYPE, RECORD_TYPE, or CLASS_TYPE.  If any problems occur, it
// may return NULL.
//
Type * checkConstructor (Constructor * constructor) {
  Type * t;
  ClassDef * cl;
  NamedType * nType;
  ClassField * classField;
  RecordType * rType;
  RecordField * recordField;
  FieldInit * fieldInit;
  ArrayType * aType, * newAType;
  CountValue * countValue;
  Mapping <String, AstNode> * tempMap;
  AstNode * def;
  int i, count, gotExpr;
  IntConst * intConst;
 
  t = checkTypes (constructor->type);
  t = resolveNamedType2 (constructor->type);  // This won't return a constraint
  if (t == NULL) return NULL;

  // If this is a CLASS constructor...
  cl = getClassDef (t);
  if (cl != NULL) {

    // Previously, we did not allow the type to contain any Type Parameters;
    // Now it is okay to say "alloc List[T]..." or "new List[T]..."
    //    checkConcreteClass (
    //         constructor->type,
    //         constructor,
    //         "The class in a class constructor may not involve type parameters, since the exact class must be known at compile-time");

    // At this point, "t" points to a NamedType...
    if (t->op != NAMED_TYPE) {
      programLogicError ("resolveNamedType2(constructor->type) returns NAMED_TYPE if this is a class");
    }

    // Build the substitution, if we have something like "List[Person]"...
    nType = (NamedType *) t;
    if (nType->subst == NULL) {
      nType->subst = buildSubstitution (cl->typeParms, nType->typeArgs);
    }
    // printf ("  subst = \n");
    // if (nType->subst) {
    //   nType->subst->print (6);
    // } else {
    //   printf ("NULL\n");
    // }

    // Fill in "kind", "myClass" and "sizeInBytes".
    constructor->kind = CLASS;
    constructor->myClass = cl;
    constructor->sizeInBytes = cl->sizeInBytes;

    // I considered overwriting "type" with the resolved(2) type, since there
    // may have been typeDefs involved.  But this only eliminates TypeDefs
    // at the highest level.  It does nothing for "List[MyType]".
    //    constructor->type = t;

    // Check that there is no count-value list...
    if (constructor->countValueList) {
      error (constructor->countValueList,
        "Use count-value lists for array constructors; this is a class constructor");
    }

    // Run thru all FieldInits and match them to ClassFields...
    tempMap = new Mapping <String, AstNode> (5, NULL);
    for (fieldInit = constructor->fieldInits;
         fieldInit;
         fieldInit = fieldInit->next) {
      def = cl->classMapping->find (fieldInit->id);
      if (tempMap->find (fieldInit->id)) {
        error (fieldInit, "You are assigning to this field more than once");
      } else {
        tempMap->enter (fieldInit->id, fieldInit);
      }
      if (def && (def->op == CLASS_FIELD)) {
        classField = (ClassField *) def;
        fieldInit->offset = classField->offset;
        fieldInit->sizeInBytes = classField->sizeInBytes;
        fieldInit->expr = checkAssignment (
            fieldInit->expr,
            copyTypeWithSubst (classField->type, nType->subst),
            "This expression does not have the type expected by this field",
            NULL);
      } else {
        error (fieldInit, "This is not a field in this class");
      }
    }

    // If the are no field inits, then okay.  Else make sure that all
    // fields have initializing expressions...
    if (constructor->fieldInits != NULL) {
      // Run thru all ClassFields and match them to FieldInits; if they
      // initialize any fields, they have to initalize all fields...
      // (Do we really want to check this?  Perhaps it is better to allow
      // some fields to be automatically initialized to zero equivalents...)
      for (classField = cl->fields;
           classField;
           classField = (ClassField *) classField->next) {
        if (! tempMap->find (classField->id)) {
          error (constructor, "You have failed to provide an initial value for some field in this class");
          error2 (classField, "Here is the missing field");
        }
      }
    }

    delete tempMap;

    return t;

  // If this is a RECORD constructor...
  } else if (t->op == RECORD_TYPE) {

    // Get a pointer to the RecordType node...
    rType = (RecordType *) t;

    // Fill in "kind" and "sizeInBytes"...
    constructor->kind = RECORD;
    constructor->sizeInBytes = rType->sizeInBytes;

    // t = checkTypes (constructor->fieldInits);

    // Check that there is no count-value list...
    if (constructor->countValueList) {
      error (constructor->countValueList,
        "Use count-value lists for array constructors; this is a record constructor");
    }

    // Run thru all FieldInits and match them to RecordFields...
    tempMap = new Mapping <String, AstNode> (5, NULL);
    for (fieldInit = constructor->fieldInits;
         fieldInit;
         fieldInit = fieldInit->next) {
      def = rType->fieldMapping->find (fieldInit->id);
      if (tempMap->find (fieldInit->id)) {
        error (fieldInit, "You are assigning to this field more than once");
      } else {
        tempMap->enter (fieldInit->id, fieldInit);
      }
      if (def && (def->op == RECORD_FIELD)) {
        recordField = (RecordField *) def;
        fieldInit->offset = recordField->offset;
        fieldInit->sizeInBytes = recordField->sizeInBytes;
        fieldInit->expr = checkAssignment (
            fieldInit->expr,
            recordField->type,
            "This expression does not have the type expected by this field",
            NULL);
      } else {
        error (fieldInit, "This is not a field in this record");
      }
    }

    // Run thru all RecordFields and match them to FieldInits...
    // (Do we really want to check this?  Perhaps it is better to allow
    //  some fields to be automatically initialized to zero equivalents...)
    for (recordField = rType->fields;
         recordField;
         recordField = (RecordField *) recordField->next) {
      if (! tempMap->find (recordField->id)) {
        error (constructor, "You have failed to provide an initial value for some field in this record");
        error2 (recordField, "Here is the missing field");
      }
    }

    // Return the RecordType node...
    return rType;

  // If this is an ARRAY constructor...
  } else if (t->op == ARRAY_TYPE) {

    // Get a pointer to the ArrayType node...
    aType = (ArrayType *) t;

    // Fill in "kind" and "sizeInBytes"...
    constructor->kind = ARRAY;
    constructor->sizeInBytes = -1;

    // Check that there is no field-init list...
    if (constructor->fieldInits) {
      error (constructor->fieldInits,
             "Use field-init lists for classes or records; this is an array constructor");
    }

    // make sure we have a count-value list...
    if (constructor->countValueList == NULL) {
      error (constructor,
             "This is an array constructor; you must have a count-value list");
    }

    // Run through the count-value list and check it...
    // Also, see if we can determine statically (at this time) the
    // actual size of the array.
    count = 0;
    gotExpr = 0;
    for (countValue = constructor->countValueList;
         countValue;
         countValue = countValue->next) {
      if (countValue->count) {
        countValue->count = checkAssignment (
            countValue->count,
            basicIntType,
            "In array constructor, the count expression is not an int",
            NULL);
        if (countValue->count->op == INT_CONST) {
          i = ((IntConst *) countValue->count)->ivalue;
          if (i <= 0) {
            error (countValue->count,
               "This count expression must be greater than zero");
          } else {
            count += i;
          }
        } else {
          gotExpr = 1;
          if (aType->sizeExpr) {
            error (countValue->count, "This is an array constructor for an array with a known size but I cannot evaluate this count expression at compile-time");
          }
        }
      } else {
        count++;
      }
      countValue->value = checkAssignment (
            countValue->value,
            aType->baseType,
            "In array constructor, this value expression does not have the correct type",
            NULL);
    }

    // If this ArrayType is not dynamic, then make sure that the
    // number of elements provided matches the type...
    if ((aType->sizeExpr) && (aType->sizeExpr->op == INT_CONST)) {
      i = ((IntConst *) aType->sizeExpr)->ivalue;
      if (count != i) {
        error (constructor,
             "This is an array constructor for an array with a known size but the number of initial values you have provided is incorrect");
      }
    }

    // Fill in "sizeInBytes"...
    if (count > 0) {
      i = 4 + count * aType->sizeOfElements;
      i += 3 - ((i+3) % 4);        // Pad, if necessary
      constructor->sizeInBytes = i;
    } else {
      constructor->sizeInBytes = -1;
    }

    // If this is a NEW Constructor, make sure we know the array size at compile-time...
    if ((constructor->allocKind == NEW) && gotExpr) {
      error (constructor, "A NEW ARRAY constructor requires that the number of array elements be computable at compile-time");
    }

    // printf ("ARRAY SIZE... constructor->sizeInBytes = %d\n",
    //         constructor->sizeInBytes);

    // If we saw a fixed number of elements but the given type was dynamic,
    // make a copy of the type, fill in the number of elements, and return it.
    // Also, update this constructor by changing the type.
    if (!gotExpr && (aType->sizeExpr == NULL)) {
      //      printf ("Building new ArrayType for... ");
      //      pretty (aType);
      intConst = new IntConst ();
      intConst->positionAt (aType);
      intConst->ivalue = count; 
      newAType = new ArrayType ();
      newAType->positionAt (aType);
      newAType->baseType = aType->baseType;
      newAType->sizeExpr = intConst;
      newAType->sizeInBytes = constructor->sizeInBytes;
      newAType->sizeOfElements = aType->sizeOfElements;
      //     printf ("            Here it is the new thing...   ");
      //     pretty (newAType);
      constructor->type = newAType;
      return newAType;
    }

    // Return the ArrayType node...
    return aType;

  // Otherwise, we've got an error...
  } else {
    error (constructor, "The type in a constructor must be either a class, record, or array type");
    return NULL;
  }
}



// linkDouble (doubleConst)
//
// This routine add this doubleConst to the floatList (if not already there)
// and give it a name, such as "_FloatConst_45".
//
void linkDouble (DoubleConst * doubleConst) {
    if (doubleConst->nameOfConstant == NULL) {
      doubleConst->next = floatList;
      floatList = doubleConst;
      doubleConst->nameOfConstant = newName ("FloatConst");
      // printf ("DOUBLE = %.17g   name = %s\n",
      //         doubleConst->rvalue,
      //         doubleConst-> nameOfConstant);
    }
}



// checkStaticData (expr, errorNode)
//
// This routine checks that the expr is static and can be used as the
// initializing expression for a global.  If not, it prints an error.
//
void checkStaticData (Expression * expr, AstNode * errorNode) {
  AstNode * where;
  if (expr == NULL) return;
  where = isStaticData (expr);
  if (where) {
    error (errorNode, "The initializing expression for this global variable could not be evaluated at compile-time");
    error2 (where, "The problem is here");
  }
}



// isStaticData (expr)  --> node
//
// This routine checks that the expr is static.  If it is, it returns NULL.
// If it is not static, it returns the node causing the non-static-ness.
//
AstNode *  isStaticData (Expression * expr) {
  VariableExpr * var;
  FunctionProto * funProto;
  Constructor * constructor;
  CountValue * countValue;
  AstNode * node;
  SendExpr * sendExpr;
  Global * glob;
  FieldInit * f;

  // printf ("Within isStaticData, expr = ");
  // pretty (expr);

  switch (expr->op) {

    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case BOOL_CONST:
    case NULL_CONST:
      return NULL;

    case CLOSURE_EXPR:
      return NULL;

    case DYNAMIC_CHECK:
      return isStaticData (((DynamicCheck *) expr)->expr);

    case CONSTRUCTOR:
      constructor = (Constructor *) expr;
      if (constructor->allocKind != NEW) {
        return constructor;
      }
      if (constructor->kind == ARRAY) {
        for (countValue = constructor->countValueList;
             countValue;
             countValue = countValue->next) {
          if (countValue->count != NULL) {
            if (countValue->count->op != INT_CONST) {
              return countValue->count;
            }
          }
          node = isStaticData (countValue->value);
          if (node != NULL) {
            return node;
          }
        }
        return NULL;
      } else if (constructor->kind == RECORD ||
                 constructor->kind == CLASS) {
        for (f = constructor->fieldInits; f; f = f->next) {
          node = isStaticData (f->expr);
          if (node != NULL) return node;
        }
        return NULL;
      }
      return NULL;      //  If problem with type, then skip this error

    case VARIABLE_EXPR:
      var = (VariableExpr *) expr;
      funProto = (FunctionProto *) var->myDef;
      if (funProto == NULL || funProto->op != FUNCTION_PROTO) {
        return expr;
      }
      return NULL;

    case SEND_EXPR:
      sendExpr = (SendExpr *) expr;
      if (sendExpr->primitiveSymbol != PRIMITIVE_ADDRESS_OF) {
        return expr;
      }
      var = (VariableExpr *) sendExpr->receiver;
      if (var->op != VARIABLE_EXPR) {
        return expr;
      }
      glob = (Global *) var->myDef;
      if (glob == NULL || glob->op != GLOBAL) {
        return expr;
      }
      return NULL;
  }
  return expr;
}



// checkArgList (argList, parmList, proto, invocation)
//
// This routine checks to make sure this arg list matches the parm list in the
// prototype.  It is used for:
//    function calls
//    throw stmts
//
// It checks that every argument is assignment compatible with the corresponding
// parameter type.  It may introduce e.g., "charToInt()".
//
// The 'proto' and 'invocation' args are used for error messages only.
//
void checkArgList (Argument * arg,
                   Parameter * parm,
                   AstNode * proto,
                   AstNode * invocation) {

  while (1) {
    if ((parm == NULL) && (arg == NULL)) break;
    if (arg == NULL) {
      error (invocation, "You are missing one or more arguments in this invocation");
      error2 (parm, "Here is a parameter for which there is no argument");
      break;
    }
    if (parm == NULL) {
      error (invocation, "You have supplied too many arguments in this invocation");
      error2 (proto, "Here is the prototype");
      break;
    }
    arg->expr = checkAssignment (
                    arg->expr,
                    parm->type,
                    "The type of this argument is not assignable to the type of the corresponding parameter",
                    NULL);
    arg->offset = parm->offset;
    arg->sizeInBytes = parm->sizeInBytes;
    parm = (Parameter *) parm->next;
    arg = arg->next;
  }
}



// checkArgList2 (argList, typeArgList, proto, invocation)
//
// This routine checks to make sure this arg list matches the parm list in the
// prototype.  It is used for:
//    function calls with FunctionTypes (i.e., closure invocations)
//
// It checks that every argument is assignment compatible with the corresponding
// parameter type.  It may introduce e.g., "charToInt()".
//
// The 'proto' and 'invocation' args are used for error messages only.
//
void checkArgList2 (Argument * arg,
                   TypeArg * typeArg,
                   AstNode * proto,
                   AstNode * invocation) {

  while (1) {
    if ((typeArg == NULL) && (arg == NULL)) break;
    if (arg == NULL) {
      error (invocation, "You are missing one or more arguments in this invocation");
      error2 (typeArg, "Here is a parameter for which there is no argument");
      break;
    }
    if (typeArg == NULL) {
      error (invocation, "You have supplied too many arguments in this invocation");
      error2 (proto, "Here is the prototype");
      break;
    }
    arg->expr = checkAssignment (
                  arg->expr,
                  typeArg->type,
                  "The type of this argument is not assignable the type of the corresponding parameter",
                  NULL);
    arg->offset = typeArg->offset;
    arg->sizeInBytes = typeArg->sizeInBytes;
    //   printf ("offset = %d, sizeInBytes = %d\n", arg->offset, arg->sizeInBytes);
    typeArg = typeArg->next;
    arg = arg->next;
  }
}



// checkTypeInstantiation (typeArgList, typeParmList, proto, invocation)
//
// This routine checks to make sure this arg list matches the parm list in the
// prototype.  It is used to check NamedTypes, like:
//    List [int]
//
// It checks that every argument is a subtype of the corresponding
// parameter type.
//
// The 'proto' and 'invocation' args are used for error messages only.
//
void checkTypeInstantiation (TypeArg * typeArg,
                             TypeParm * typeParm,
                             AstNode * proto,
                             AstNode * invocation) {
  int i;

  // Run thru the lists of typeParms and typeArgs in parallel.  Check each pair...
  while (1) {
    if ((typeParm == NULL) && (typeArg == NULL)) break;
    if (typeArg == NULL) {
      error (invocation, "This class or interface is parameterized but you have given too few types; please add more types in 'Type [Type, Type, ...]'");
      error2 (typeParm, "Here is a parameter for which there is no type");
      break;
    }
    if (typeParm == NULL) {
      error (invocation, "You have supplied too many type arguments in 'Type [ Type, Type, ...]'");
      error2 (proto, "Here is the interface or class definition");
      break;
    }

    if (!isSubType (typeArg->type, typeParm->type)) {
      error (invocation, "One of the arguments in 'Type [Type, Type, ...]' fails to match the corresponding constraint type in the interface or class definition");
      error2 (typeArg, "Here is the argument that is incorrect");
      errorWithType ("It should be equal to (or a subtype of)", typeParm->type);
    }

    if (typeParm->fourByteRestricted) {
      i = sizeInBytesOfWhole (typeArg->type, typeArg->type, 0);
      if ((i > 4) || (i == -1)) {
        error (invocation, "One of the arguments in 'Type [Type, Type, ...]' has a size that is not 4 bytes or smaller");
        error2 (typeArg, "Here is the argument that is causing the problem");
        error2 (typeParm, "This type parameter was used in a variable declaration somewhere within the class; this restricts it to being instantiated only with types of size 4 bytes or smaller");
      }
    }
    typeParm = typeParm->next;
    typeArg = typeArg->next;
  }
}



// checkForPrimitive (sendExpr, t, PRIMITIVE_I_OP, PRIMITIVE_D_OP)
//
// This routine is passed a sendExpr and the type of the receiver.  It checks
// to see if the type of the receiver and argument are correct for a binary
// operator.  If so, it sets the "primitiveSymbol" field accordingly.
//
// This routine will also insert coercions as necessary.
//
// This routine returns the type of the result, or NULL if this is not an
// instance of the primitive type.
//
// An example call (after checking that the selector is "PLUS") would be:
//       resultType = check (sendExpr, t, PRIMITIVE_I_ADD, PRIMITIVE_D_ADD);
//
Type * checkForPrimitive (SendExpr * sendExpr,
                          Type * recvrType,
                          int PRIMITIVE_I_OP,
                          int PRIMITIVE_D_OP) {
  Type * t2, * resultType;

  if (argCount (sendExpr->argList) == 1) {
    t2 = checkTypes (sendExpr->argList->expr);
    if (t2 == NULL) return basicVoidType;
    if (isIntType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else if (isCharType (t2)) {
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else if (isDoubleType (t2)) {
        sendExpr->receiver = insertIntToDouble (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicDoubleType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char, int or double");
        return basicIntType;
      }
    } else if (isDoubleType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->argList->expr = insertIntToDouble (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicDoubleType;
      } else if (isDoubleType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicDoubleType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be int or double");
        return basicDoubleType;
      }
    } else if (isCharType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else if (isCharType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char or int");
        return basicIntType;
      }
    }
  }
  return NULL;
}



// checkForPrimitive2 (sendExpr, t, PRIMITIVE_I_OP)
//
// This routine is passed a sendExpr and the type of the receiver.  It checks
// to see if the type of the receiver and argument are correct for a binary
// operator.  If so, it sets the "primitiveSymbol" field accordingly.
//
// This routine will also insert coercions as necessary.
//
// This routine returns the type of the result, or NULL if this is not an
// instance of the primitive type.
//
// An example call (after checking that the selector is "PERCENT") would be:
//       resultType = check (sendExpr, t, PRIMITIVE_I_REM);
//
Type * checkForPrimitive2 (SendExpr * sendExpr,
                          Type * recvrType,
                          int PRIMITIVE_I_OP) {
  Type * t2, * resultType;

  if (argCount (sendExpr->argList) == 1) {
    t2 = checkTypes (sendExpr->argList->expr);
    if (t2 == NULL) return basicVoidType;
    if (isIntType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else if (isCharType (t2)) {
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char or int");
        return basicIntType;
      }
    } else if (isCharType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else if (isCharType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicIntType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char or int");
        return basicIntType;
      }
    }
  }
  return NULL;
}



// checkForPrimitive3 (sendExpr, t, PRIMITIVE_I_OP, PRIMITIVE_D_OP)
//
// This routine is passed a sendExpr and the type of the receiver.  It checks
// to see if the type of the receiver and argument are correct for a binary
// operator.  If so, it sets the "primitiveSymbol" field accordingly.
//
// This routine will also insert coercions as necessary.
//
// This routine returns the type of the result, or NULL if this is not an
// instance of the primitive type.
//
// An example call (after checking that the selector is "LESS") would be:
//       resultType = check (sendExpr, t, PRIMITIVE_I_LT, PRIMITIVE_D_LT);
//
Type * checkForPrimitive3 (SendExpr * sendExpr,
                          Type * recvrType,
                          int PRIMITIVE_I_OP,
                          int PRIMITIVE_D_OP) {
  Type * t2, * resultType;

  if (argCount (sendExpr->argList) == 1) {
    t2 = checkTypes (sendExpr->argList->expr);
    if (t2 == NULL) return basicVoidType;
    if (isIntType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isCharType (t2)) {
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isDoubleType (t2)) {
        sendExpr->receiver = insertIntToDouble (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicBoolType;
//    Implicit ptr->int coercion is no longer allowed...
//      } else if (isPtrType (t2)) {
//        if (safe) {
//          error (sendExpr, "Comparing ints to ptrs is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
//        }
//        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
//        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char, int or double");
        return basicBoolType;
      }
    } else if (isPtrType (recvrType)) {
//      if (isIntType (t2)) {
//        if (safe) {
//          error (sendExpr, "Comparing ptrs to ints is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
//        }
//        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
//        return basicBoolType;
//      } else
      if (isPtrType (t2)) {
        if (safe) {
          error (sendExpr, "Comparing ptrs to ptrs is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
        }
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be a ptr");
        return basicBoolType;
      }
    } else if (isDoubleType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->argList->expr = insertIntToDouble (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicBoolType;
      } else if (isDoubleType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be int or double");
        return basicBoolType;
      }
    } else if (isCharType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isCharType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char or int");
        return basicBoolType;
      }
    }
  }
  return NULL;
}



// checkForPrimitive4 (sendExpr, t, PRIMITIVE_B_OP)
//
// This routine is passed a sendExpr and the type of the receiver.  It checks
// to see if the type of the receiver and argument are correct for a binary
// operator.  If so, it sets the "primitiveSymbol" field accordingly.
//
// This routine will also insert coercions as necessary.
//
// This routine returns the type of the result, or NULL if this is not an
// instance of the primitive type.
//
// An example call (after checking that the selector is "AMP") would be:
//       resultType = check (sendExpr, t, PRIMITIVE_B_AND);
//
Type * checkForPrimitive4 (SendExpr * sendExpr,
                          Type * recvrType,
                          int PRIMITIVE_B_OP) {
  Type * t2, * resultType;

  if (argCount (sendExpr->argList) == 1) {
    t2 = checkTypes (sendExpr->argList->expr);
    if (t2 == NULL) return basicVoidType;
    if (isBoolType (recvrType)) {
      if (isBoolType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else if (isPtrType (t2)) {
        sendExpr->argList->expr = insertPtrToBool (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be bool or ptr");
        return basicBoolType;
      }
    } else if (isPtrType (recvrType)) {
      if (isBoolType (t2)) {
        sendExpr->receiver = insertPtrToBool (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else if (isPtrType (t2)) {
        sendExpr->receiver = insertPtrToBool (sendExpr->receiver);
        sendExpr->argList->expr = insertPtrToBool (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be bool or ptr");
        return basicBoolType;
      }
    }
  }
  return NULL;
}



// checkForPrimitive5 (sendExpr, t, PRIMITIVE_I_OP, PRIMITIVE_D_OP,
//                                  PRIMITIVE_B_OP, PRIMITIVE_OBJ_OP)
//
// This routine is passed a sendExpr and the type of the receiver.  It checks
// to see if the type of the receiver and argument are correct for a binary
// operator.  If so, it sets the "primitiveSymbol" field accordingly.
//
// This routine will also insert coercions as necessary.
//
// This routine returns the type of the result, or NULL if this is not an
// instance of the primitive type.
//
// An example call (after checking that the selector is "EQUAL") would be:
//       resultType = check (sendExpr, t, PRIMITIVE_I_EQ, PRIMITIVE_D_EQ, PRIMITIVE_B_EQ);
//
Type * checkForPrimitive5 (SendExpr * sendExpr,
                          Type * recvrType,
                          int PRIMITIVE_I_OP,
                          int PRIMITIVE_D_OP,
                          int PRIMITIVE_B_OP,
                          int PRIMITIVE_OBJ_OP) {
  Type * t2, * resultType, * t1;
  ClassDef * cl1, * cl2;
  AstNode * def1, * def2;

  if (argCount (sendExpr->argList) == 1) {
    t2 = checkTypes (sendExpr->argList->expr);
    if (t2 == NULL) return basicVoidType;
    if (isIntType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isCharType (t2)) {
        sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isDoubleType (t2)) {
        sendExpr->receiver = insertIntToDouble (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicBoolType;
//    Implicit int->ptr coercion is no longer allowed
//      } else if (isPtrType (t2)) {
//        if (safe) {
//          error (sendExpr, "Comparing ints to ptrs is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
//        }
//        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
//        return basicBoolType;
      } else if (isTypeOfNullType (t2)) {
        sendExpr->argList->expr = insertZero (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char, int or double");
        return basicBoolType;
      }
    } else if (isDoubleType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->argList->expr = insertIntToDouble (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicBoolType;
      } else if (isDoubleType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_D_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be int or double");
        return basicBoolType;
      }
    } else if (isCharType (recvrType)) {
      if (isIntType (t2)) {
        sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isCharType (t2)) {
        // sendExpr->receiver = insertCharToInt (sendExpr->receiver);
        // sendExpr->argList->expr = insertCharToInt (sendExpr->argList->expr);
        // sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be char or int");
        return basicBoolType;
      }
    } else if (isBoolType (recvrType)) {
      if (isBoolType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else if (isPtrType (t2)) {
        sendExpr->argList->expr = insertPtrToBool (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else if (isTypeOfNullType (t2)) {
        sendExpr->argList->expr = insertFalse (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be a bool or ptr");
        return basicBoolType;
      }
    } else if (isPtrType (recvrType)) {
      if (isBoolType (t2)) {
        sendExpr->receiver = insertPtrToBool (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
//    Implicit int->ptr coercion is no longer allowed
//      } else if (isIntType (t2)) {
//        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
//        if (safe) {
//          error (sendExpr, "Comparing ints to ptrs is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
//        }
//        return basicBoolType;
      } else if (isPtrType (t2)) {
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isTypeOfNullType (t2)) {
        sendExpr->argList->expr = insertZero (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be a bool or ptr");
        return basicBoolType;
      }
    } else if (isTypeOfNullType (recvrType)) {
      if (isBoolType (t2)) {
        sendExpr->receiver = insertFalse (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_B_OP;
        return basicBoolType;
      } else if (isPtrType (t2)) {
        sendExpr->receiver = insertZero (sendExpr->receiver);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else if (isTypeOfNullType (t2)) {
        sendExpr->receiver = insertZero (sendExpr->receiver);
        sendExpr->argList->expr = insertZero (sendExpr->argList->expr);
        sendExpr->primitiveSymbol = PRIMITIVE_I_OP;
        return basicBoolType;
      } else {
        error (sendExpr, "This built-in operator expects the right operand to be a bool or ptr");
        return basicBoolType;
      }

    } else if (isObjectType (recvrType)) {
      if (! isObjectType (t2)) {
        error (sendExpr, "This built-in operator expects the right operand to be an object");
        errorWithType ("The type of the right operand is", t2);
        return basicBoolType;
      }
      if ((! isSubType (recvrType, t2)) && (! isSubType (t2, recvrType))) {
        error (sendExpr, "When comparing objects, the types of the two operands must be related but are not");
        errorWithType ("The type of the left operand is", recvrType);
        errorWithType ("The type of the right operand is", t2);
        return basicBoolType;
      }
      sendExpr->primitiveSymbol = PRIMITIVE_OBJ_OP;
      return basicBoolType;
    }
  }
  return NULL;
}



// checkMessageSend (methodProto, sendExpr, recvrType, subst) --> retType
//
// This routine checks that this send has arguments of appropriate types for
// the given MethodProto.  It returns the return type.
//
// The substitution is applied to the methodProto.
//
Type * checkMessageSend (MethodProto * methodProto,
                         SendExpr * sendExpr,
                         Type * recvrType,
                         Mapping<TypeParm,Type> * subst) {
  Parameter * parm;
  Argument * arg;

  if (methodProto == NULL) {
    error (sendExpr, "This message is not understood by this type of receiver (2)");
    errorWithType ("The type of the receiver is", recvrType);
    return NULL;
  }

  // Put a ptr in the SendExpr back to the MethodProto...
  sendExpr->myProto = methodProto;

  // printf ("Looking at sendExpr = ");
  // pretty (sendExpr);
  // printf ("        methodProto = ");
  // pretty (methodProto);

  // Run through the arguments and make sure they are assignment compatible
  // with the expected parameter types...

  // They should have the same kind...
  if (sendExpr->kind != methodProto->kind) {
    programLogicError ("Kind mismatch in checkMessageSend");
  }

  // Make sure each arg type is assignment-compatible with the parm type...
  parm = methodProto->parmList;
  arg = sendExpr->argList;
  while (1) {
    if ((parm == NULL) && (arg == NULL)) break;
    if (arg == NULL) {
      error (sendExpr, "You are missing one or more arguments in this message-send");
      error2 (parm, "Here is a parameter for which there is no argument");
      break;
    }
    if (parm == NULL) {
      error (sendExpr, "You have supplied too many arguments in this message-send");
      error2 (methodProto, "Here is the method prototype");
      break;
    }
    arg->expr = checkAssignment (
                      arg->expr,
                      copyTypeWithSubst (parm->type, subst),
                      "The type of this argument is not assignable to the type of the corresponding parameter",
                      NULL
                                );
    arg->offset = parm->offset;
    arg->sizeInBytes = parm->sizeInBytes;
    parm = (Parameter *) parm->next;
    arg = arg->next;
  }

  // Update "maxArgBytesSoFar"..
  updateMaxArgBytes (methodProto->totalParmSize);
  updateMaxArgBytes (sizeInBytesOfWhole(methodProto->retType,
                                                methodProto->retType,
                                                0));    // wantPrinting = 0

  // Return the type that this message returns...
  return copyTypeWithSubst (methodProto->retType, subst);
}



// checkAssignment (expr, expectedType, errorMsg, assignStmt) --> modifiedExpr
//
// This routine is passed an expression and an expected type.  It checks to make
// sure that this expression is "assignment compatible" with this type.  If not,
// it prints the given error message.
//
// If we need to insert an implicit coercion to make this expression into the
// correct type, this is done and the modified expression is returned.
//
// Here are the implicit coercions and the functions to be inserted:
//
//    int -> double         INT_TO_DOUBLE
//    char -> int           CHAR_TO_INT
//    ptr -> bool           PTR_TO_BOOL
//    nullType -> bool      false-constant
//    nullType -> ptr       nop
// ---int -> ptr            nop (unsafe)       -- No longer allowed
// ---ptr -> int            nop (unsafe)       -- No longer allowed
//
// If we are checking an Assignment statement, then "assignStmt" will point
// to the node we are checking.  When this routine is used for something besides an
// assignment statement, "assignStmt" will be null.
//
Expression * checkAssignment (Expression * expr,
                              Type * expectedType,
                              char * errorMsg,
                              AssignStmt * assignStmt) {
  Type * t;
  BoolConst * falseConst;
  int i, j;
  ClassDef * classDef;
  ArrayType * aType1, * aType2;
  DynamicCheck * dynamicCheck;

  // printf ("Calling checkAssignment with expr = ");
  // pretty (expr);
  // printf ("                     expectedType = ");
  // pretty (expectedType);

  t = checkTypes (expr);
  if (t == NULL) return expr;

  // Perform the implicit the int->double coercion, if necessary...
  if (isIntType (t) && isDoubleType (expectedType)) {
    return insertIntToDouble (expr);
  }

  // Perform the implicit the char->int coercion, if necessary...
  if (isCharType (t) && isIntType (expectedType)) {
    return insertCharToInt (expr);
  }

  // Perform the implicit the ptr->bool coercion, if necessary...
  if (isPtrType (t) && isBoolType (expectedType)) {
    return insertPtrToBool (expr);
  }

  // Perform the implicit the NullType->bool coercion, if necessary...
  if (isTypeOfNullType (t) && isBoolType (expectedType)) {
    falseConst = new BoolConst (0);    // Replace with "false"
    falseConst->positionAt (expr);
    return falseConst;
  }

  // For the implicit the NullType->ptr coercion, do nothing;
  // It will pass the assignable test below and we should insert nothing...
  //      if (isTypeOfNullType (t) && isPtrType (expectedType)) {
  //        return expr;             // nop coercion
  //      }

//  // Perform the implicit the int->ptr coercion...  No longer allowed.
//  if (isIntType (t) && isPtrType (expectedType)) {
//    if (safe) {
//      error (expr, "Using an int when a ptr is expected is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
//    }
//    return expr;             // nop coercion
//  }

//  // Perform the implicit the ptr->int coercion...  No longer allowed.
//  if (isPtrType (t) && isIntType (expectedType)) {
//    if (safe) {
//      error (expr, "Using a ptr when an int is expected is an unsafe operation; you must compile with the 'unsafe' option if you wish to do this");
//    }
//    return expr;             // nop coercion
//  }

  // OBJECT ASSIGNMENT:
  //         *objPtr = *objPtr
  //         *objPtr = ...
  //         ...     = *objPtr
  if (assignStmt &&
      isObjectType (t) &&
      isObjectType (expectedType) &&
      (isDeref (expr) || isDeref (assignStmt->lvalue))) {

    // Case 1: OBJECT ASSIGNMENT: *objPtr = *objPtr...
    if (isDeref (expr) && isDeref (assignStmt->lvalue)) {
      //   printf ("===> OBJECT ASSIGNMENT: *objPtr = *objPtr...\n");
      if (! isSubType (expectedType, t) &&
          ! isSubType (t, expectedType)) {
        error (assignStmt->lvalue, "In this object assignment, there is no subtype relation between the left-hand side type and the right-hand side type; this object copy cannot succeed at runtime");
        errorWithType ("The type of the left-hand side is", expectedType);
        errorWithType ("The type of the right-hand side is", t);
        return expr;
      }
      // We can just check class name, e.g., List ?= List.  Since we have already
      // checked that a subtype relation holds, we can't have something
      // like "List[Person] ?= List[Student]...
      //    printf ("     Need to insert dynamic check that both point to same class of object at runtime.\n");
      //    printf ("     Will perform a 'dynamic' object copy.\n");
      assignStmt->dynamicCheck = 1;
      return expr;

    // Case 2: OBJECT ASSIGNMENT: *objPtr = x...
    } else if (isDeref (assignStmt->lvalue)) {
      //   printf ("===> OBJECT ASSIGNMENT: *objPtr = x...\n");
      if (! isSubType (t, expectedType)) {
        error (assignStmt->lvalue, "In this object assignment, there is a type error; the pointer on the left-hand side cannot possibly point to an object of the same class as on the right-hand side");
        errorWithType ("The type of the left-hand side is", expectedType);
        errorWithType ("The type of the right-hand side is", t);
        return expr;
      }
      // We can just check class name, e.g., List ?= List.  Since we have already
      // checked that a subtype relation holds, we can't have something
      // like "List[Person] ?= List[Student]...
      //   printf ("     Inserting dynamic class check to check that *objPtr has class: ");
      //   pretty (t);
      classDef = getClassDef (t);
      if (classDef == NULL) {
        error (expr, errorMsg);
        error2 (expr, "This object assignment calls for a dynamic class-check, but the type of this expression is not a class");
        errorWithType ("The type of the expression is", t);
        errorWithType ("The expected type is", expectedType);
        return expr;
      }
      //   printf ("     Will copy %d bytes.\n", classDef->sizeInBytes);
      assignStmt->dynamicCheck = 2;
      assignStmt->classDef = classDef;
      return expr;

    // Case 3: OBJECT ASSIGNMENT: x = *objPtr...
    } else {
      //   printf ("===> OBJECT ASSIGNMENT: x = *objPtr...\n");
      if (! isSubType (expectedType, t)) {
        error (assignStmt->lvalue, "In this object assignment, there is a type error; the pointer on the right-hand side cannot possibly point to an object of the same class as on the left-hand side");
        errorWithType ("The type of the left-hand side is", expectedType);
        errorWithType ("The type of the right-hand side is", t);
        return expr;
      }
      // We can just check class name, e.g., List ?= List.  Since we have already
      // checked that a subtype relation holds, we can't have something
      // like "List[Person] ?= List[Student]...
      //   printf ("     Inserting dynamic class check to check that *objPtr has class: ");
      //   pretty (expectedType);
      classDef = getClassDef (expectedType);
      if (classDef == NULL) {
        error (expr, errorMsg);
        error2 (expr, "This object assignment calls for a dynamic class-check, but the type of this expression is not a class");
        errorWithType ("The type of the expression is", t);
        errorWithType ("The expected type is", expectedType);
        return expr;
      }
      //   printf ("     Will copy %d bytes.\n", classDef->sizeInBytes);
      assignStmt->dynamicCheck = 3;
      assignStmt->classDef = classDef;
      return expr;
    }

  // ARRAY ASSIGNMENT: array = array
  } else if (assignStmt &&
      (aType1 = getArrayType (expectedType)) &&
      (aType2 = getArrayType (t))) {
    // Make sure the base types are type-equal...
    if (!typesEqual (aType1->baseType, aType2->baseType)) {
      error (assignStmt->lvalue, "In this array assignment statement, the types of the array elements must be the same");
      errorWithType ("The type of the left-hand side is", aType1);
      errorWithType ("The type of the right-hand side is", aType2);
      return expr;
    }

    // Here are the possible cases...
    //    arr[10] = arr[10]
    //    arr[10] = arr[*]
    //    arr[*] = arr[10]
    //    arr[*] = arr[*]

    // Case 4: ARRAY ASSIGNMENT: arr[10] = arr[10]...
    if ((aType1->sizeInBytes >= 0) && (aType2->sizeInBytes >= 0)) {
      //   printf ("===> ARRAY ASSIGNMENT: arr[10] = arr[10]...\n");
      //   printf ("     Will copy %d bytes.\n", aType1->sizeInBytes);
      // Check that the sizes are equal...
      if (aType1->sizeInBytes != aType2->sizeInBytes) {
        error (assignStmt->lvalue, "In this array assignment statement, the sizes of the array are not the same");
        errorWithType ("The type of the left-hand side is", aType1);
        errorWithType ("The type of the right-hand side is", aType2);
        return expr;
      }
      if (aType1->sizeExpr->op == INT_CONST) {
        i = ((IntConst *) (aType1->sizeExpr))->ivalue;
      } else {
        programLogicError ("Array size must be known if sizeInBytes is known (1)");
      }
      assignStmt->dynamicCheck = 4;
      assignStmt->arraySize = i;
      return expr;

    // Case 5: ARRAY ASSIGNMENT: arr[10] = arr[*]...
    } else if (aType1->sizeInBytes >= 0) {
      //   printf ("===> ARRAY ASSIGNMENT: arr[10] = arr[*]...\n");
      if (aType1->sizeExpr->op == INT_CONST) {
        i = ((IntConst *) (aType1->sizeExpr))->ivalue;
      } else {
        programLogicError ("Array size must be known if sizeInBytes is known (2)");
      }
      //   printf ("     Need to insert dynamic size check (that N = %d) for source array size.\n", i);
      //   printf ("     Will copy %d bytes.\n", aType1->sizeInBytes);
      assignStmt->dynamicCheck = 5;
      assignStmt->arraySize = i;
      return expr;

    // Case 6: ARRAY ASSIGNMENT: arr[*] = arr[10]...
    } else if (aType2->sizeInBytes >= 0) {
      //   printf ("===> ARRAY ASSIGNMENT: arr[*] = arr[10]...\n");
      if (aType2->sizeExpr->op == INT_CONST) {
        i = ((IntConst *) (aType2->sizeExpr))->ivalue;
        assignStmt->sizeInBytes = aType2->sizeInBytes;
      } else {
        programLogicError ("Array size must be known if sizeInBytes is known (3)");
      }
      //   printf ("     Need to insert dynamic size check (that N = %d) for destination array size...\n", i);
      //   printf ("     Will copy %d bytes.\n", aType2->sizeInBytes);
      assignStmt->dynamicCheck = 6;
      assignStmt->arraySize = i;
      return expr;

    // Case 7: ARRAY ASSIGNMENT: arr[*] = arr[*]...
    } else {
      //   printf ("===> ARRAY ASSIGNMENT: arr[*] = arr[*]...\n");
      //   printf ("     Need to insert dynamic array copy (size not known at compile time)...\n");
      //   printf ("     Must check that sizes are equal at runtime.\n");
      i = aType1->sizeOfElements;
      //   printf ("     Will copy 4 + N * %d bytes. (ok to round up to multiple of 4 at runtime)\n", i);
      assignStmt->dynamicCheck = 7;
      assignStmt->arraySize = i;         // Store the size of each element
      return expr;
    }

  } else {   // This is not an Assignment Statement...

    // Case 8: OBJECT COPY: ... := *objPtr
    if (isDeref (expr) && isObjectType (t)) {
      //   printf ("===> OBJECT COPY: ... := *objPtr\n");
      // Make sure that we are expecting a class type.
      classDef = getClassDef (expectedType);
      if (classDef == NULL) {
        error (expr, errorMsg);
        error2 (expr, "Dereferencing a ptr to an object calls for dynamic class-check, but the expected type is not a class");
        errorWithType ("The type of the expression is", t);
        errorWithType ("The expected type is", expectedType);
        return expr;
      }
      if (!isSubType (expectedType, t)) {
        error (expr, errorMsg);
        errorWithType ("This expression cannot point to the expected class of object; at runtime the class of the object will be... (or one of its subclasses)", t);
        errorWithType ("The expected class is", expectedType);
        return expr;
      }
      // We can just check class name, e.g., List ?= List.  Since we have already
      // checked that a subtype relation holds, we can't have something
      // like "List[Person] ?= List[Student]...
      // Note: the expected type may contain TypeParms, e.g., "ListImpl2[T1]"...
      //   printf ("     Need to insert dynamic check that this expr has class: ");
      //   pretty (expectedType);
      // Need to insert something...
      if (expr->op != DYNAMIC_CHECK) {
        dynamicCheck = new DynamicCheck ();
        dynamicCheck->positionAt (expr);
        dynamicCheck->expr = expr;
        dynamicCheck->kind = 8;
        dynamicCheck->expectedClassDef = classDef;
        return dynamicCheck;
      } else {
        return expr;
      }


    // ARRAY COPY: ... := array
    } else if ((aType1 = getArrayType (expectedType)) &&
               (aType2 = getArrayType (t))) {
      // Make sure the base types are type-equal...
      if (!typesEqual (aType1->baseType, aType2->baseType)) {
        error (expr, errorMsg);
        error2 (expr, "When copying an array, the types of the array elements must be the same");
        errorWithType ("The type of the expression is", aType1);
        errorWithType ("The expected type is", aType2);
        return expr;
      }

      // Here are the possible cases...
      //    arr[10] = arr[10]
      //    arr[10] = arr[*]
      //    arr[*] = arr[10]
      //    arr[*] = arr[*]

      // Case 9: ARRAY COPY: arr[10] = arr[10]...
      if ((aType1->sizeInBytes >= 0) && (aType2->sizeInBytes >= 0)) {
        //   printf ("===> ARRAY COPY: arr[10] := arr[10]...\n");
        // Check that the sizes are equal...
        if (aType1->sizeInBytes != aType2->sizeInBytes) {
          error (expr, errorMsg);
          error2 (expr, "When copying an array, the array sizes must be the same");
          errorWithType ("The type of the expression is", aType2);
          errorWithType ("The expected type is", aType1);
          return expr;
        }
        //   printf ("     Will copy %d bytes.\n", aType1->sizeInBytes);
        if (aType1->sizeExpr->op == INT_CONST) {
          i = ((IntConst *) (aType1->sizeExpr))->ivalue;
        } else {
          programLogicError ("Array size must be known if sizeInBytes is known (3)");
        }
        // Need to add something into the AST...
        if (expr->op != DYNAMIC_CHECK) {
          dynamicCheck = new DynamicCheck ();
          dynamicCheck->positionAt (expr);
          dynamicCheck->expr = expr;
          dynamicCheck->kind = 9;
          dynamicCheck->arraySizeInBytes = aType1->sizeInBytes;
          dynamicCheck->expectedArraySize = i;
          return dynamicCheck;
        } else {
          return expr;
        }

      // Case 10: ARRAY COPY: arr[10] = arr[*]
      } else if (aType1->sizeInBytes >= 0) {
        //   printf ("===> ARRAY COPY: arr[10] := arr[*]...\n");
        if (aType1->sizeExpr->op == INT_CONST) {
          i = ((IntConst *) (aType1->sizeExpr))->ivalue;
        } else {
          programLogicError ("Array size must be known if sizeInBytes is known (4)");
        }
        //   printf ("     Need to insert dynamic size check (that N = %d) for source array size.\n", i);
        //   printf ("     Will copy %d bytes.\n", aType1->sizeInBytes);
        // Need to add something into the AST...
        if (expr->op != DYNAMIC_CHECK) {
          dynamicCheck = new DynamicCheck ();
          dynamicCheck->positionAt (expr);
          dynamicCheck->expr = expr;
          dynamicCheck->kind = 10;
          dynamicCheck->expectedArraySize = i;
          dynamicCheck->arraySizeInBytes = aType1->sizeInBytes;
          return dynamicCheck;
        } else {
          return expr;
        }

      // Case 11: ARRAY COPY: arr[*] = arr[10]
      } else if (aType2->sizeInBytes >= 0) {
        //   printf ("===> ARRAY COPY: arr[*] := arr[10]...\n");
        // This case should never be possible (unless other errors), since
        // expectedType comes from, e.g., parm type, and such a type cannot
        // be a dynamic array.
        error (expr, errorMsg);
        error2 (expr, "In array copy, the target type must not be a dynamic array (1)");
        errorWithType ("The type of the expression is", aType2);
        errorWithType ("The expected type is", aType1);
        return expr;

      // Case 12: ARRAY COPY: arr[*] = arr[*]
      } else {
        //   printf ("===> ARRAY COPY: arr[*] := arr[*]...\n");
        // This case should never be possible (unless other errors), since
        // expectedType comes from, e.g., parm type, and such a type cannot
        // be a dynamic array.
        error (expr, errorMsg);
        error2 (expr, "In array copy, the target type must not be a dynamic array (2)");
        errorWithType ("The type of the expression is", aType2);
        errorWithType ("The expected type is", aType1);
        return expr;
      }

    }
  }

  //    if (isPtrType (expectedType)) {
  //    printf ("expectedType = ");
  //    pretty (expectedType);
  //    printf ("expr = ");
  //    pretty (expr);
  //    printf ("t = ");
  //    pretty (t);
  //    }

  // Check the type...
  if (! assignable (expectedType, t)) {
    error (expr, errorMsg);
    errorWithType ("The type of the expression is", t);
    errorWithType ("The expected type is", expectedType);
    return expr;
  }

  i = sizeInBytesOfWhole (t, expr, 1);
  if (i <= 0) {
    error (expr, errorMsg);
    errorWithType ("I cannot determine the size of the type of this expr, which is",
                   t);
  }

  j = sizeInBytesOfWhole (expectedType, expr, 1);
  if (j <= 0) {
    error (expr, errorMsg);
    errorWithType ("I cannot determine the size of the expected type, which is",
                   expectedType);
  }

  if (i>0 && j>0 && i != j) {
    printf ("expr = ");
    pretty (expr);
    printf ("t = ");
    pretty (expr);
    printf ("expectedType = ");
    pretty (expectedType);
    programLogicError ("Two types are assignable, but have different sizes");
  }

  return expr;
}



// insertIntToDouble (expr) --> expr
//
// This routine wraps an "intToDouble" node around the argument and returns it.
//
Expression * insertIntToDouble (Expression * expr) {
  CallExpr * callExpr;
  Argument * arg;

  arg = new Argument ();
  arg->positionAt (expr);
  arg->expr = expr;

  callExpr = new CallExpr ();
  callExpr->positionAt (expr);
  callExpr->id = stringIntToDouble;
  callExpr->argList = arg;
  // callExpr->myDef = 0;
  callExpr->primitiveSymbol = PRIMITIVE_INT_TO_DOUBLE;
  // updateMaxArgBytes (8); // allocate frame space, in case we insert a real call
  return callExpr;

}



// insertIntIsZero (expr) --> expr
//
// This routine wraps an "intIsZero" node around the argument and returns it.
//
Expression * insertIntIsZero (Expression * expr) {
  CallExpr * callExpr;
  Argument * arg;

  arg = new Argument ();
  arg->positionAt (expr);
  arg->expr = expr;

  callExpr = new CallExpr ();
  callExpr->positionAt (expr);
  callExpr->id = stringIIsZero;
  callExpr->argList = arg;
  // callExpr->myDef = 0;
  callExpr->primitiveSymbol = PRIMITIVE_I_IS_ZERO;
  // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
  return callExpr;

}



// insertIntNotZero (expr) --> expr
//
// This routine wraps an "intNotZero" node around the argument and returns it.
//
Expression * insertIntNotZero (Expression * expr) {
  CallExpr * callExpr;
  Argument * arg;

  arg = new Argument ();
  arg->positionAt (expr);
  arg->expr = expr;

  callExpr = new CallExpr ();
  callExpr->positionAt (expr);
  callExpr->id = stringINotZero;
  callExpr->argList = arg;
  // callExpr->myDef = 0;
  callExpr->primitiveSymbol = PRIMITIVE_I_NOT_ZERO;
  // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
  return callExpr;

}



// insertCharToInt (expr) --> expr
//
// This routine wraps an "charToInt" node around the argument and returns it.
//
Expression * insertCharToInt (Expression * expr) {
  CallExpr * callExpr;
  Argument * arg;

  arg = new Argument ();
  arg->positionAt (expr);
  arg->expr = expr;

  callExpr = new CallExpr ();
  callExpr->positionAt (expr);
  callExpr->id = stringCharToInt;
  callExpr->argList = arg;
  // callExpr->myDef = 0;
  callExpr->primitiveSymbol = PRIMITIVE_CHAR_TO_INT;
  // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
  return callExpr;

}



// insertPtrToBool (expr) --> expr
//
// This routine wraps an "ptrToBool" node around the argument and returns it.
//
Expression * insertPtrToBool (Expression * expr) {
  CallExpr * callExpr;
  Argument * arg;

  arg = new Argument ();
  arg->positionAt (expr);
  arg->expr = expr;

  callExpr = new CallExpr ();
  callExpr->positionAt (expr);
  callExpr->id = stringPtrToBool;
  callExpr->argList = arg;
  // callExpr->myDef = 0;
  callExpr->primitiveSymbol = PRIMITIVE_PTR_TO_BOOL;
  // updateMaxArgBytes (4); // allocate frame space, in case we insert a real call
  return callExpr;

}



// insertDeref (expr) --> expr
//
// This routine wraps an "deref" node around the argument and returns it.
//
Expression * insertDeref (Expression * expr) {
  SendExpr * sendExpr;

  sendExpr = new SendExpr ();
  sendExpr->positionAt (expr);
  sendExpr->selector = stringUnaryStar;
  sendExpr->kind = PREFIX;
  sendExpr->receiver = expr;
  // sendExpr->argList = NULL;
  sendExpr->primitiveSymbol = PRIMITIVE_DEREF;
  return sendExpr;

}



// isDeref (nade)
//
// This routine returns true if the given node is a DEREF node.  If there is
// any problem, it returns false.
//
int isDeref (AstNode * p) {
  if (p == NULL) return 0;
  if (p->op != SEND_EXPR) return 0;
  return ((SendExpr *) p) -> primitiveSymbol == PRIMITIVE_DEREF;
}



// isAddressOf (nade)
//
// This routine returns true if the given node is a ADDRESS_OF node.  If there is
// any problem, it returns false.
//
int isAddressOf (AstNode * p) {
  programLogicError ("Routine isAddressOf is never used");
  if (p == NULL) return 0;
  if (p->op != SEND_EXPR) return 0;
  return ((SendExpr *) p) -> primitiveSymbol == PRIMITIVE_ADDRESS_OF;
}



// insertFalse (expr) --> expr
//
// This routine returns a false constant.
//
Expression * insertFalse (Expression * expr) {
  BoolConst * boolConst;

  boolConst = new BoolConst (0);
  boolConst->positionAt (expr);
  return boolConst;

}



// insertTrue (expr) --> expr
//
// This routine returns a true constant.
//
Expression * insertTrue (Expression * expr) {
  BoolConst * boolConst;

  boolConst = new BoolConst (1);
  boolConst->positionAt (expr);
  return boolConst;

}



// insertZero (expr) --> expr
//
// This routine returns a zero constant.
//
Expression * insertZero (Expression * expr) {
  IntConst * intConst;

  intConst = new IntConst ();
  intConst->positionAt (expr);
  intConst->ivalue = 0;
  return intConst;

}



// argCount (argList)
//
// This routine returns the number of arguments in the given list.
//
int argCount (Argument * argList) {
  int n = 0;
  while (argList) {
    n++;
    argList = argList->next;
  }
  return n;
}



// isCharType (type)
//
// This routine returns true if the given type is "char" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "char"; the actual type may be a
// subtype of this.  However, the only subtype of "char" is itself.
//
int isCharType (Type * type) {
  if ((type == NULL) || (type->op == CHAR_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == CHAR_TYPE)) return 1;
  return 0;
}



// isIntType (type)
//
// This routine returns true if the given type is "int" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "int"; the actual type may be a
// subtype of this.  However, the only subtype of "int" is itself.
//
int isIntType (Type * type) {
  if ((type == NULL) || (type->op == INT_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == INT_TYPE)) return 1;
  return 0;
}



// isDoubleType (type)
//
// This routine returns true if the given type is "double" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "double"; the actual type may be a
// subtype of this.  However, the only subtype of "double" is itself.
//
int isDoubleType (Type * type) {
  if ((type == NULL) || (type->op == DOUBLE_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == DOUBLE_TYPE)) return 1;
  return 0;
}



// isBoolType (type)
//
// This routine returns true if the given type is "bool" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "bool"; the actual type may be a
// subtype of this.  However, the only subtype of "bool" is itself.
//
int isBoolType (Type * type) {
  if ((type == NULL) || (type->op == BOOL_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == BOOL_TYPE)) return 1;
  return 0;
}



// isVoidType (type)
//
// This routine returns true if the given type is void or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "void"; the actual type may be a
// subtype of this.  However, the only subtype of "void" is itself.
//
int isVoidType (Type * type) {
  if ((type == NULL) || (type->op == VOID_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == VOID_TYPE)) return 1;
  return 0;
}



// isTypeOfNullType (type)
//
// This routine returns true if the given type is "NULL_TYPE" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "typeOfNull"; the actual type may be a
// subtype of this.  However, the only subtype of "typeOfNull" is itself.
//
int isTypeOfNullType (Type * type) {
  if ((type == NULL) || (type->op == TYPE_OF_NULL_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == TYPE_OF_NULL_TYPE)) return 1;
  return 0;
}



// isAnyType (type)
//
// This routine returns true if the given type is "ANY_TYPE" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "anyType"; the actual type may be a
// subtype of this!
//
int isAnyType (Type * type) {

  programLogicError ("NOTE: isAnyType is never invoked");

  if ((type == NULL) || (type->op == ANY_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == ANY_TYPE)) return 1;
  return 0;
}



// isPtrType (type)
//
// This routine returns true if the given type is a "PTR_TYPE" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is a ptr type; Since the actual type must be a
// subtype of this, we know the actual type must be a ptr type (or typeOfNull).
//
int isPtrType (Type * type) {
  if (getPtrType (type) == NULL) return 0;
  return 1;
}



// isPtrToVoidType (type)
//
// This routine returns true if the given type is "ptr to void" or if there is
// a problem.
//
int isPtrToVoidType (Type * type) {
  PtrType * pType;
  pType = getPtrType (type);
  if (pType == NULL) return 0;
  return isVoidType (pType->baseType);
}



// getPtrType (type) --> PtrType
//
// If this type is a ptr type, then we return the PTR_TYPE node.  If not, we return
// NULL.
//
// NOTE: The arg may be a TypeParm, in which case we are returning the PTR_TYPE
// describing the CONSTRAINT type; the actual type could be any subtype of this!
//
// Returns NULL if any problems.
//
PtrType * getPtrType (Type * type) {
  if ((type == NULL) || (type->op == PTR_TYPE)) return (PtrType *) type;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op != PTR_TYPE)) return NULL;
  return (PtrType *) type;
}



// isRecordType (type)
//
// This routine returns true if the given type is a "RECORD_TYPE" or if there is
// a problem.
//
// NOTE: The argument "type" may be a TypeParm, in which case we are asking
// whether the CONSTRAINT type is "record"; the actual type may be a
// subtype of this.  However, the only subtype of "record" is itself.
//
int isRecordType (Type * type) {
  if ((type == NULL) || (type->op == RECORD_TYPE)) return 1;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op == RECORD_TYPE)) return 1;
  return 0;
}



// isArrayType (type)
//
// This routine returns true if the given type is a "ARRAY_TPYE" or if there is
// a problem.
//
int isArrayType (Type * type) {
  if (getArrayType (type) == NULL) return 0;
  return 1;
}



// getArrayType (type) --> ArrayType
//
// If this type is a array type, then we return the ARRAY_TYPE node.  If not, we
// return NULL.
//
// NOTE: The arg may be a TypeParm, in which case we are returning the ARRAY_TYPE
// describing the CONSTRAINT type; the actual type may be a subtype of this.
//
// Returns NULL if any problems.
//
ArrayType * getArrayType (Type * type) {
  if ((type == NULL) || (type->op == ARRAY_TYPE)) return (ArrayType *) type;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op != ARRAY_TYPE)) return NULL;
  return (ArrayType *) type;
}



// getFunctionType (type) --> FunctionType
//
// If this type is 'ptr to function', then we return the FUNCTION_TYPE node.
// If not, we return NULL.
//
// NOTE: The arg may be a TypeParm, in which case we are returning the FUNCTION_TYPE
// describing the CONSTRAINT type; the actual type will be any subtype of this!
//
// Returns NULL if any problems.
//
FunctionType * getFunctionType (Type * type) {
  PtrType * pType;
  Type * t;

  pType = getPtrType (type);
  if (pType == NULL) return NULL;
  t = pType->baseType;
  if ((t == NULL) || (t->op == FUNCTION_TYPE)) return (FunctionType *) t;
  t = resolveNamedType (t);
  if ((t == NULL) || (t->op != FUNCTION_TYPE)) return NULL;
  return (FunctionType *) t;
}



// isExactClass (type) --> bool
//
// If this type is a class, return TRUE.
// Returns FALSE if any problems.
//
// Note: calls resolveNamedType2; constraint types are not exact.
//
int isExactClass (Type * type) {
  NamedType * nType;

  programLogicError ("This routine tested, no longer used. Was used for isInstanceOf");

  type = resolveNamedType2 (type);
  if ((type == NULL) || (type->op != NAMED_TYPE)) return 0;
  nType = (NamedType *) type;
  if (nType->myDef == NULL) return 0;
  if (nType->myDef->op == CLASS_DEF) return 1;
  return 0;
}



// isObjectType (type) --> bool
//
// If this type is a class or interface, return TRUE.
// Returns FALSE if any problems.
//
int isObjectType (Type * type) {
  NamedType * nType;
  type = resolveNamedType (type);
  if ((type == NULL) || (type->op != NAMED_TYPE)) return 0;
  nType = (NamedType *) type;
  if (nType->myDef == NULL) return 0;
  if (nType->myDef->op == CLASS_DEF) return 1;
  if (nType->myDef->op == INTERFACE) return 1;
  return 0;
}



// getClassDef (type) --> ClassDef
//
// If this type names a specific class, then return the ClassDef it names.
// Otherwise, return NULL.
//
// If this type names a type parameter, e.g.,
//      t: Person
// then return NULL, since we are looking for a specific class,
// not a constraint.
//
ClassDef * getClassDef (Type * type) {
  NamedType * nType;
  type = resolveNamedType2 (type);
  if ((type == NULL) || (type->op != NAMED_TYPE)) return NULL;
  nType = (NamedType *) type;
  if (nType->myDef == NULL) return NULL;
  if (nType->myDef->op == CLASS_DEF) return (ClassDef *) nType->myDef;
  return NULL;
}



// getInterface (type) --> Interface
//
// If this type names a specific class, then return the Interface it names.
// Otherwise, return NULL.
//
// If this type names a type parameter, e.g.,
//      t: Person
// then return NULL, since we are looking for a specific interface,
// not a constraint.
//
Interface * getInterface (Type * type) {
  NamedType * nType;
  type = resolveNamedType2 (type);
  if ((type == NULL) || (type->op != NAMED_TYPE)) return NULL;
  nType = (NamedType *) type;
  if (nType->myDef == NULL) return NULL;
  if (nType->myDef->op == INTERFACE) return (Interface *) nType->myDef;
  return NULL;
}



// resolveNamedType (type) --> type
//
// If this type is a NamedType, this routine tries to determine the underlying type.
//   If it names a Typedef (e.g., "type T = ptr to int") we return the underlying type.
//   If it names a TypeParm, then we return the constraint type.
//   If it names a ClassDef or Interface, we return a ptr to the NamedType.
//   If it is another type (IntType, PtrType, etc.), it returns that.
//
// If there are problems, it returns NULL.
//
Type * resolveNamedType (Type * type) {
  AstNode * def;

  if (type == NULL) return NULL;
  if (type->op != NAMED_TYPE) return type;
  def = ((NamedType *) type)->myDef;
  if (def == NULL) return NULL;

  switch (def->op) {

    case INTERFACE:
    case CLASS_DEF:
      return type;

    case TYPE_PARM:
      return resolveNamedType (((TypeParm *) def)->type);

    case TYPE_DEF:
      return resolveNamedType (((TypeDef *) def)->type);

    default:
      printf ("\ndef->op = %s\n", symbolName (def->op));
      programLogicError ("Unexpected myDef->op in resolveNamedType");
  }
}



// resolveNamedType2 (type) --> type
//
// If this type is a NamedType, this routine tries to determine the underlying type.
//   If it names a Typedef (e.g., "type T = ptr to int") we return the underlying type.
//   If it names a TypeParm, return the NamedType.
//   If it names a ClassDef or Interface, return the NamedType.
//   If it is another type (IntType, PtrType, etc.), it returns that.
//
// If there are problems, it returns NULL.
//
Type * resolveNamedType2 (Type * type) {
  AstNode * def;

  if (type == NULL) return NULL;
  if (type->op != NAMED_TYPE) return type;
  def = ((NamedType *) type)->myDef;
  if (def == NULL) return NULL;

  switch (def->op) {

    case INTERFACE:
    case CLASS_DEF:
    case TYPE_PARM:
      return type;

    case TYPE_DEF:
      return resolveNamedType2 (((TypeDef *) def)->type);

    default:
      printf ("\ndef->op = %s\n", symbolName (def->op));
      programLogicError ("Unexpected myDef->op in resolveNamedType2");

  }
}



// checkConcreteClass (type, errorNode, errorMsg)
//
// This routine checks to make sure that "type" does not contain
// any Type Parameters.
//
// It also ensures that "type" is a class (not an interface).
// However, it may be an instantiated class:
//
// It is used in the following contexts:
//        new <type>
//        EXPR isInstanceOf TYPE
//
// Here are some examples of what is allowed:
//                int              -- Error
//                Person           -- Okay (where Person is a class)
//                Taxable          -- Error (where Taxable is an interface)
//                List [Person]    -- Okay (where List is a class)
//                List [Taxable]   -- Okay
//                Coll [Person]    -- Error (where Coll is an interface)
//                List [T1]        -- Error
//                T1               -- Error (where T1 is a TypeParm)
//
// If the type has a problem, then the "errorMsg" is printed out.
//
void checkConcreteClass (Type * type,
                         AstNode * errorNode,
                         char * errorMsg) {
  AstNode * def;
  NamedType * namedType;
  TypeArg * typeArg;

  if (type == NULL) return;
  if (type->op != NAMED_TYPE) {
    error (errorNode, errorMsg);
    errorWithType ("This type is causing the problem", type);
    return;
  }

  namedType = (NamedType *) type;
  def = namedType->myDef;
  if (def == NULL) return;

  switch (def->op) {

    case CLASS_DEF:
      // Check the type arguments if any...
      for (typeArg = namedType->typeArgs; typeArg; typeArg = typeArg->next) {
        checkConcreteType (typeArg->type, errorNode, errorMsg);
      }
      return;

    case INTERFACE:
      error (errorNode, errorMsg);
      errorWithType ("This interface is causing the problem", type);
      return;

    case TYPE_PARM:
      error (errorNode, errorMsg);
      errorWithType ("This type parameter is causing the problem", type);
      return;

    case TYPE_DEF:
      checkConcreteClass (((TypeDef *) def)->type, errorNode, errorMsg);
      return;

    default:
      printf ("\ndef->op = %s\n", symbolName (def->op));
      programLogicError ("Unexpected myDef->op in checkConcreteClass");

  }
}



// checkConcreteClassOrInterface (type, errorNode, errorMsg)
//
// This routine checks to make sure that "type" does not contain
// any Type Parameters.
//
// It also ensures that "type" is a class or an interface.
//
// It is used to check:
//        ... isKindOf <type>
//
// Here are some examples of what is allowed:
//                int              -- Error
//                Person           -- Okay (where Person is a class)
//                Taxable          -- Okay (where Taxable is an interface)
//                List [Person]    -- Okay (where List is a class)
//                List [Taxable]   -- Okay
//                Coll [Person]    -- Okay (where Coll is an interface)
//                List [T1]        -- Error
//                T1               -- Error (where T1 is a TypeParm)
//
// If the type has a problem, then the "errorMsg" is printed out.
//
void checkConcreteClassOrInterface (Type * type,
                                    AstNode * errorNode,
                                    char * errorMsg) {
  AstNode * def;
  NamedType * namedType;
  TypeArg * typeArg;

  if (type == NULL) return;
  if (type->op != NAMED_TYPE) {
    error (errorNode, errorMsg);
    errorWithType ("This type is causing the problem", type);
    return;
  }

  namedType = (NamedType *) type;
  def = namedType->myDef;
  if (def == NULL) return;

  switch (def->op) {

    case CLASS_DEF:
      // Check the type arguments if any...
      for (typeArg = namedType->typeArgs; typeArg; typeArg = typeArg->next) {
        checkConcreteType (typeArg->type, errorNode, errorMsg);
        // IMPLEMENTATION RESTRICTION: The type must not be parameterized:
        error (errorNode, errorMsg);
        errorWithType ("IMPLEMENTATION RESTRICTION: The type in IS-KIND-OF must not be parameterized", type);
      }
      return;

    case INTERFACE:
      // Check the type arguments if any...
      for (typeArg = namedType->typeArgs; typeArg; typeArg = typeArg->next) {
        checkConcreteType (typeArg->type, errorNode, errorMsg);
        // IMPLEMENTATION RESTRICTION: The type must not be parameterized:
        error (errorNode, errorMsg);
        errorWithType ("IMPLEMENTATION RESTRICTION: The type in IS-KIND-OF must not be parameterized", type);
      }
      return;

    case TYPE_PARM:
      error (errorNode, errorMsg);
      errorWithType ("This type parameter is causing the problem", type);
      return;

    case TYPE_DEF:
      checkConcreteClassOrInterface (((TypeDef *) def)->type, errorNode, errorMsg);
      return;

    default:
      printf ("\ndef->op = %s\n", symbolName (def->op));
      programLogicError ("Unexpected myDef->op in checkConcreteClassOrInterface");

  }
}



// checkConcreteType (type, errorNode, errorMsg)
//
// This routine checks to make sure that "type" does not contain
// any Type Parameters.
//
// Here are some examples of what is allowed:
//                int              -- Okay
//                Person           -- Okay (where Person is a class)
//                Taxable          -- Okay (where Taxable is an interface)
//                List [Person]    -- Okay (where List is a class)
//                List [Taxable]   -- Okay
//                Coll [Person]    -- Okay (where Coll is an interface)
//                T1               -- Error (where T1 is a TypeParm)
//                List [T1]        -- Error
//                Coll [T1]        -- Error
//
// If the type has a problem, then the "errorMsg" is printed out.
//
void checkConcreteType (Type * type,
                        AstNode * errorNode,
                        char * errorMsg) {
  AstNode * def;
  NamedType * namedType;
  TypeArg * typeArg;

  if (type == NULL) return;
  if (type->op != NAMED_TYPE) return;

  namedType = (NamedType *) type;
  def = namedType->myDef;
  if (def == NULL) return;

  switch (def->op) {

    case CLASS_DEF:
      // Check the type arguments if any...
      for (typeArg = namedType->typeArgs; typeArg; typeArg = typeArg->next) {
        checkConcreteType (typeArg->type, errorNode, errorMsg);
      }
      return;

    case INTERFACE:
      // Check the type arguments if any...
      for (typeArg = namedType->typeArgs; typeArg; typeArg = typeArg->next) {
        checkConcreteType (typeArg->type, errorNode, errorMsg);
      }
      return;

    case TYPE_PARM:
      error (errorNode, errorMsg);
      errorWithType ("This type parameter is causing the problem", type);
      return;

    case TYPE_DEF:
      checkConcreteType (((TypeDef *) def)->type, errorNode, errorMsg);
      return;

    default:
      printf ("\ndef->op = %s\n", symbolName (def->op));
      programLogicError ("Unexpected myDef->op in checkConcreteType");

  }
}



// isLValue (expr) --> bool
//
// This routine returns true if the given expression may be used as an L-Value.
// It does not check the expression or print error messages; it is assumed that
// we will call "checkTypes" on the expression elsewhere.
//
// If there are any problems, we assume this is an L-Value, in an attempt to
// reduce other error messages.
//
// An LValue is...
//       LValue --> Expr [ Expr ]
//              --> Expr . ID
//              --> * Expr           Where the expr has type ptr
//              --> ID               Where id is either a local, global,
//                                     parameter or classField
//
int isLValue (Expression * expr) {
  VariableExpr * var;
  SendExpr * sendExpr;
  AstNode * def;
  Type * t;

  if (expr == NULL) return 1;    // If error, assume it is an LValue
  if (expr->op == VARIABLE_EXPR) {
    var = (VariableExpr *) expr;
    def = var->myDef;
    if (def == NULL) return 1;    // If error, assume it is an LValue
    if ((def->op == LOCAL) ||
        (def->op == GLOBAL) ||
        (def->op == PARAMETER) ||
        (def->op == CLASS_FIELD)) {
      return 1;
    }
    return 0;
  }
  if (isDeref (expr)) {
    sendExpr = (SendExpr *) expr;
    t = checkTypes (sendExpr->receiver);  // This will give redundant error messages...
    if (isPtrType (t)) return 1;
    return 0;
  }
  if (expr->op == ARRAY_ACCESS) return 1;
  if (expr->op == FIELD_ACCESS) return 1;
  return 0;
}


// updateMaxArgBytes (i)
//
// This routine updates the global variable "maxArgBytesSoFar" if a new
// maximum is found.
//
void updateMaxArgBytes (int i) {
  if (i > maxArgBytesSoFar) {
    maxArgBytesSoFar = i;
  }
}



// fallsThru (node)
//
// This routine is passed a pointer to a fragment of the AST.  It walks
// walks this branch of the AST.
//
// If this branch is a statement, it returns TRUE if it might possibly fall through,
// and FALSE if it will never fall through.
//
// If this branch is not a statement, it doesn't really matter what we return;
// but we will return FALSE.
//
int fallsThru (AstNode * node) {
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
    int falls, falls2;
    Statement * stmt;

  if (node == NULL) return 0;

  // printf ("%s...\n", symbolName (node->op));

  switch (node->op) {

    case HEADER:
      header = (Header *) node;
      // printf ("  %s\n", header->packageName->chars);
      fallsThru (header->uses);
      fallsThru (header->consts);
      fallsThru (header->errors);
      fallsThru (header->globals);
      fallsThru (header->typeDefs);
      fallsThru (header->functionProtos);
      fallsThru (header->functions);
      fallsThru (header->closures);
      fallsThru (header->interfaces);
      fallsThru (header->classes);
      // fallsThru (header->next);
      return 0;

//    case CODE:
//      code = (Code *) node;
//      fallsThru (code->consts);
//      fallsThru (code->errors);
//      fallsThru (code->globals);
//      fallsThru (code->typeDefs);
//      fallsThru (code->functions);
//      fallsThru (code->interfaces);
//      fallsThru (code->classes);
//      fallsThru (code->behaviors);
//      return 0;

    case USES:
      uses = (Uses *) node;
      fallsThru (uses->renamings);
      fallsThru (uses->next);
      return 0;

    case RENAMING:
      renaming = (Renaming *) node;
      fallsThru (renaming->next);
      return 0;

    case INTERFACE:
      interface = (Interface *) node;
      // printf ("  %s\n", interface->id->chars);
      fallsThru (interface->typeParms);
      fallsThru (interface->extends);
      fallsThru (interface->methodProtos);
      fallsThru (interface->next);
      return 0;

    case CLASS_DEF:
      cl = (ClassDef *) node;
      // printf ("  %s\n", cl->id->chars);
      fallsThru (cl->typeParms);
      fallsThru (cl->implements);
      fallsThru (cl->superclass);
      fallsThru (cl->fields);
      fallsThru (cl->methodProtos);
      fallsThru (cl->methods);
      fallsThru (cl->next);
      return 0;

//    case BEHAVIOR:
//      behavior = (Behavior *) node;
//      printf ("  %s\n", behavior->id->chars);
//      fallsThru (behavior->methods);
//      fallsThru (behavior->next);
//      return 0;

    case TYPE_DEF:
      typeDef = (TypeDef *) node;
      // printf ("  %s\n", typeDef->id->chars);
      fallsThru (typeDef->type);
      fallsThru (typeDef->next);
      return 0;

    case CONST_DECL:
      constDecl = (ConstDecl *) node;
      // printf ("  %s\n", constDecl->id->chars);
      fallsThru (constDecl->expr);
      fallsThru (constDecl->next);
      return 0;

    case ERROR_DECL:
      errorDecl = (ErrorDecl *) node;
      // printf ("  %s\n", errorDecl->id->chars);
      fallsThru (errorDecl->parmList);
      fallsThru (errorDecl->next);
      return 0;

    case FUNCTION_PROTO:
      functionProto = (FunctionProto *) node;
      // printf ("  %s\n", functionProto->id->chars);
      fallsThru (functionProto->parmList);
      fallsThru (functionProto->retType);
      fallsThru (functionProto->next);
      return 0;

    case FUNCTION:
      fun = (Function *) node;
      // printf ("  %s\n", fun->id->chars);
      fallsThru (fun->parmList);
      fallsThru (fun->retType);
      fallsThru (fun->locals);
      falls = fallsThruStmtList (fun->stmts);
      if (falls) {
        if (isVoidType (fun->retType)) {
          returnStmt = new ReturnStmt ();
          returnStmt->positionAt (fun);
          returnStmt->enclosingMethOrFunction = fun;
          if (fun->stmts) {
            for (stmt=fun->stmts; stmt->next; stmt = stmt->next) {
            }
            stmt->next = returnStmt;
            returnStmt->positionAt (stmt);
          } else {
            fun->stmts = returnStmt;
          }
        } else {
          error (fun, "Execution may fall through the bottom of this function; please add a RETURN statement (with return-expression)");
        }
      }
      fallsThru (fun->next);
      return 0;

    case METHOD_PROTO:
      methodProto = (MethodProto *) node;
      // printf ("  %s\n", methodProto->selector->chars);
      fallsThru (methodProto->parmList);
      fallsThru (methodProto->retType);
      fallsThru (methodProto->next);
      return 0;

    case METHOD:
      meth = (Method *) node;
      // printf ("  %s\n", meth->selector->chars);
      fallsThru (meth->parmList);
      fallsThru (meth->retType);
      fallsThru (meth->locals);
      falls = fallsThruStmtList (meth->stmts);
      if (falls) {
        if (isVoidType (meth->retType)) {
          returnStmt = new ReturnStmt ();
          returnStmt->positionAt (meth);
          returnStmt->enclosingMethOrFunction = meth;
          if (meth->stmts) {
            for (stmt= meth->stmts; stmt->next; stmt = stmt->next) {
            }
            stmt->next = returnStmt;
            returnStmt->positionAt (stmt);
          } else {
            meth->stmts = returnStmt;
          }
        } else {
          error (meth, "Execution may fall through the bottom of this method; please add a RETURN statement (with return-expression)");
        }
      }
      fallsThru (meth->next);
      return 0;

    case TYPE_PARM:
      typeParm = (TypeParm *) node;
      fallsThru (typeParm->type);
      fallsThru (typeParm->next);
      return 0;

    case TYPE_ARG:
      typeArg = (TypeArg *) node;
      fallsThru (typeArg->type);
      fallsThru (typeArg->next);
      return 0;

    case CHAR_TYPE:
    case INT_TYPE:
    case DOUBLE_TYPE:
    case BOOL_TYPE:
    case VOID_TYPE:
    case TYPE_OF_NULL_TYPE:
    case ANY_TYPE:
      return 0;

    case PTR_TYPE:
      pType = (PtrType *) node;
      fallsThru (pType->baseType);
      return 0;

    case ARRAY_TYPE:
      aType = (ArrayType *) node;
      fallsThru (aType->sizeExpr);
      fallsThru (aType->baseType);
      return 0;

    case RECORD_TYPE:
      rType = (RecordType *) node;
      fallsThru (rType->fields);
      return 0;

    case FUNCTION_TYPE:
      fType = (FunctionType *) node;
      fallsThru (fType->parmTypes);
      fallsThru (fType->retType);
      return 0;

    case NAMED_TYPE:
      nType = (NamedType *) node;
      fallsThru (nType->typeArgs);
      return 0;

    case IF_STMT:
      ifStmt = (IfStmt *) node;
      fallsThru (ifStmt->expr);
      falls = fallsThruStmtList (ifStmt->thenStmts);
      falls2 = fallsThruStmtList (ifStmt->elseStmts);
      return falls || falls2;

    case ASSIGN_STMT:
      assignStmt = (AssignStmt *) node;
      fallsThru (assignStmt->lvalue);
      fallsThru (assignStmt->expr);
      return 1;

    case CALL_STMT:
      callStmt = (CallStmt *) node;
      fallsThru (callStmt->expr);
      return 1;

    case SEND_STMT:
      sendStmt = (SendStmt *) node;
      fallsThru (sendStmt->expr);
      return 1;

    case WHILE_STMT:
      whileStmt = (WhileStmt *) node;
      fallsThru (whileStmt->expr);
      falls = fallsThruStmtList (whileStmt->stmts);
      if (!falls && !whileStmt->containsAnyContinues) {
        error (whileStmt, "Execution can never reach the bottom of the loop body; therefore this loop will not iterate");
      }
      if (whileStmt->expr &&
          whileStmt->expr->op == BOOL_CONST &&
          ((BoolConst *) whileStmt->expr)->ivalue == 1 &&
          !whileStmt->containsAnyBreaks) {
        return 0;
      }
      return 1;

    case DO_STMT:
      doStmt = (DoStmt *) node;
      falls = fallsThruStmtList (doStmt->stmts);
      fallsThru (doStmt->expr);
      if (!falls && !doStmt->containsAnyContinues) {
        error (doStmt, "Execution can never reach the bottom of the loop body; therefore this loop will not iterate");
      }
      return 1;

    case BREAK_STMT:
      return 0;

    case CONTINUE_STMT:
      return 0;

    case RETURN_STMT:
      returnStmt = (ReturnStmt *) node;
      fallsThru (returnStmt->expr);
      return 0;

    case FOR_STMT:
      forStmt = (ForStmt *) node;
      fallsThru (forStmt->lvalue);
      fallsThru (forStmt->expr1);
      fallsThru (forStmt->expr2);
      fallsThru (forStmt->expr3);
      falls = fallsThruStmtList (forStmt->stmts);
      if (!falls && !forStmt->containsAnyContinues) {
        error (forStmt, "Execution can never reach the bottom of the loop body; therefore this loop will not iterate");
      }
      return 1;

    case SWITCH_STMT:
      switchStmt = (SwitchStmt *) node;
      fallsThru (switchStmt->expr);
      // fallsThru (switchStmt->caseList);
      falls = 0;
      for (cas = switchStmt->caseList; cas; cas = cas->next) {
        fallsThru (cas->expr);
        falls = fallsThruStmtList (cas->stmts);
      }
      if (switchStmt->defaultIncluded) {
        falls = fallsThruStmtList (switchStmt->defaultStmts);
      }
      return falls || switchStmt->containsAnyBreaks;

    case TRY_STMT:
      tryStmt = (TryStmt *) node;
      falls = fallsThruStmtList (tryStmt->stmts);
      for (cat = tryStmt->catchList; cat; cat = cat->next) {
        fallsThru (cat->parmList);
        falls = falls || fallsThruStmtList (cat->stmts);
      }
      return falls;

    case THROW_STMT:
      throwStmt = (ThrowStmt *) node;
      fallsThru (throwStmt->argList);
      return 0;

    case FREE_STMT:
      freeStmt = (FreeStmt *) node;
      fallsThru (freeStmt->expr);
      return 1;

    case DEBUG_STMT:
      debugStmt = (DebugStmt *) node;
      return 1;

    case CASE:
      programLogicError ("FallsThru: CASEs done in SWITCH");
      return 0;

    case CATCH:
      programLogicError ("FallsThru: CATCHes done in TRY");
      return 0;

    case GLOBAL:
      global = (Global *) node;
      fallsThru (global->type);
      fallsThru (global->initExpr);
      fallsThru (global->next);
      return 0;

    case LOCAL:
      local = (Local *) node;
      fallsThru (local->type);
      fallsThru (local->initExpr);
      fallsThru (local->next);
      return 0;

    case PARAMETER:
      parm = (Parameter *) node;
      fallsThru (parm->type);
      fallsThru (parm->next);
      return 0;

    case CLASS_FIELD:
      classField = (ClassField *) node;
      fallsThru (classField->type);
      fallsThru (classField->next);
      return 0;

    case RECORD_FIELD:
      recordField = (RecordField *) node;
      fallsThru (recordField->type);
      fallsThru (recordField->next);
      return 0;

    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case BOOL_CONST:
    case NULL_CONST:
      return 0;

    case CALL_EXPR:
      callExpr = (CallExpr *) node;
      fallsThru (callExpr->argList);
      return 0;

    case SEND_EXPR:
      sendExpr = (SendExpr *) node;
      fallsThru (sendExpr->receiver);
      fallsThru (sendExpr->argList);
      return 0;

    case SELF_EXPR:
      return 0;

    case SUPER_EXPR:
      return 0;

    case FIELD_ACCESS:
      fieldAccess = (FieldAccess *) node;
      fallsThru (fieldAccess->expr);
      return 0;

    case ARRAY_ACCESS:
      arrayAccess = (ArrayAccess *) node;
      fallsThru (arrayAccess->arrayExpr);
      fallsThru (arrayAccess->indexExpr);
      return 0;

    case CONSTRUCTOR:
      constructor = (Constructor *) node;
      fallsThru (constructor->type);
      fallsThru (constructor->countValueList);
      fallsThru (constructor->fieldInits);
      return 0;

    case CLOSURE_EXPR:
      // closureExpr = (ClosureExpr *) node;
      // Skip this, since we'll fallsThru any functions when we do header->closures...
      // fallsThru (closureExpr->function);
      return 0;

    case VARIABLE_EXPR:
      return 0;

    case AS_PTR_TO_EXPR:
      asPtrToExpr = (AsPtrToExpr *) node;
      fallsThru (asPtrToExpr->expr);
      fallsThru (asPtrToExpr->type);
      return 0;

    case AS_INTEGER_EXPR:
      asIntegerExpr = (AsIntegerExpr *) node;
      fallsThru (asIntegerExpr->expr);
      return 0;

    case ARRAY_SIZE_EXPR:
      arraySizeExpr = (ArraySizeExpr *) node;
      fallsThru (arraySizeExpr->expr);
      return 0;

    case IS_INSTANCE_OF_EXPR:
      isInstanceOfExpr = (IsInstanceOfExpr *) node;
      fallsThru (isInstanceOfExpr->expr);
      fallsThru (isInstanceOfExpr->type);
      return 0;

    case IS_KIND_OF_EXPR:
      isKindOfExpr = (IsKindOfExpr *) node;
      fallsThru (isKindOfExpr->expr);
      fallsThru (isKindOfExpr->type);
      return 0;

    case SIZE_OF_EXPR:
      sizeOfExpr = (SizeOfExpr *) node;
      fallsThru (sizeOfExpr->type);
      return 0;

    case DYNAMIC_CHECK:
      dynamicCheck = (DynamicCheck *) node;
      fallsThru (dynamicCheck->expr);
      return 0;

    case ARGUMENT:
      arg = (Argument *) node;
      fallsThru (arg->expr);
      fallsThru (arg->next);
      return 0;

    case COUNT_VALUE:
      countValue = (CountValue *) node;
      fallsThru (countValue->count);
      fallsThru (countValue->value);
      fallsThru (countValue->next);
      return 0;

    case FIELD_INIT:
      fieldInit = (FieldInit *) node;
      fallsThru (fieldInit->expr);
      fallsThru (fieldInit->next);
      return 0;

    default:
      printf ("node->op = %s\n", symbolName (node->op));
      programLogicError ("Unkown op in fallsThru");
  }
}



// fallsThruStmtList (stmtList) --> bool
//
// Go through all the statements in the given statement list and make sure
// that there are no unreachable statements.  Return TRUE if execution can
// fall out the bottom of the statement list, and FALSE otherwise.
//
int fallsThruStmtList (Statement * stmtList) {
  Statement * stmt;
  int falls = 1;

  for (stmt = stmtList; stmt != NULL; stmt = stmt->next) {
    falls = fallsThru (stmt);
    if (!falls && (stmt->next != NULL)) {
      error (stmt->next, "This statement is unreachable (dead code) and will never be executed");
      falls = 1;
    }
  }
  return falls;
}



// assignLocalOffsets (hdr)
//
// This routine sets "local->offset" in
//    functions,
//    closures, and
//    methods
// It assumes all "local->sizeInBytes" have been set earlier.
//
// This routine also updates:
//    fun->frameSize
//    meth->frameSize
//
// If commandOptionS applies, this routine also prints the offset
// and frameSize info (and "parm->offset"s) for
//    functions,
//    closures,
//    methods, and
//    errors
//
void assignLocalOffsets (Header * hdr) {
  Function * fun;
  ClassDef * cl;
  Method * meth;
  ErrorDecl * errorDecl;

  // Run through all errors...
  if (commandOptionS) {
    for (errorDecl = hdr->errors; errorDecl; errorDecl = errorDecl->next) {
      printf ("==========  Error ");
      printString (stdout, errorDecl->id);
      printf ("  ==========\n");
      printParmOffsets (errorDecl->parmList);
    }
  }

  // Run through all functions...
  for (fun = hdr->functions; fun; fun = fun->next) {
    assignOffsetsInMethOrFunction (fun);
  }

  // Run through all closures...
  for (fun = hdr->closures; fun; fun = fun->next) {
    assignOffsetsInMethOrFunction (fun);
  }

  // Run through all methods in all classes...
  for (cl = hdr->classes; cl; cl = cl->next) {
    for (meth = cl->methods; meth; meth = meth->next) {
      assignOffsetsInMethOrFunction (meth);
    }
  }
}



// assignOffsetsInMethOrFunction (methOrFunction)
//
// This routine assigns offsets to the Locals in this method or function.
//
void assignOffsetsInMethOrFunction (MethOrFunction * methOrFunction) {
  int lastOffset, padding, incr;
  Local * local;
  Function * fun;
  Method * meth;
  Catch * cat;
  Parameter * parm;

  // Print the activation record layout, if desired...
  if (commandOptionS) {
    if (methOrFunction->op == FUNCTION) {
      fun = (Function *) methOrFunction;
      if (fun->id) {
        printf ("==========  Function ");
        printString (stdout, fun->id);
        printf ("  ==========\n");
      } else {
         printf ("==========  Closure on line %d in file %s  ==========\n",
                       extractLineNumber (fun->tokn),
                       extractFilename (fun->tokn));
      }
    } else {
      meth = (Method *) methOrFunction;
      printf ("==========  Method ");
      printString (stdout, meth->myClass->id);
      printf (".");
      printString (stdout, meth->selector);
      printf ("  ==========\n");
    }
    printf ("          Offset 4:\t4\tRet Addr\n");
    printf ("          Offset 0:\t4\tOld FP\n");
    printf ("          Offset -4:\t4\tOld r13\n");
    printf ("          Offset -8:\t4\tRoutine Descriptor Ptr\n");
    printf ("      Local Variables:\n");
  }

  // We will place variables at offsets before this offset...
  lastOffset = -8;

  // Look at each local.  Do only "bools" and "chars"...
  for (local = methOrFunction->locals;
       local != NULL;
       local = (Local *) local->next) {

    lastOffset = assignOffsetToLocal (lastOffset, local, commandOptionS, 1);
  }

  // Look at each parameter used in any catch clauses.  Do only "bools" and "chars"...
  for (cat = methOrFunction->catchList; cat; cat = cat->nextInMethOrFunction) {
    for (parm = cat->parmList; parm; parm = (Parameter *) parm->next) {
      lastOffset = assignOffsetToLocal (lastOffset, parm, commandOptionS, 1);
    }
  }

  // Insert padding bytes after the byte data, if necessary...
  padding = 3 - ((-lastOffset-1) % 4);
  lastOffset -= padding;
  if (commandOptionS && padding != 0) {
    printf ("          Offset %d:\t%d\t...padding...\n", lastOffset, padding);
  }

  // Look at each local.  Skip "bools" and "chars"...
  for (local = methOrFunction->locals;
       local != NULL;
       local = (Local *) local->next) {
    lastOffset = assignOffsetToLocal (lastOffset, local, commandOptionS, 0);
  }

  // Look at each parameter used in any catch clauses.  Skip "bools" and "chars"...
  for (cat = methOrFunction->catchList; cat; cat = cat->nextInMethOrFunction) {
    for (parm = cat->parmList; parm; parm = (Parameter *) parm->next) {
      lastOffset = assignOffsetToLocal (lastOffset, parm, commandOptionS, 0);
    } 
  }

  // Now add space for arguments to functions we will call from this meth or function...
  lastOffset -= methOrFunction->maxArgBytes;
  if (commandOptionS) {
    printf ("          Offset %d:\t%d\tSpace for Args\n",
            lastOffset, methOrFunction->maxArgBytes);
  }

  // Compute and fill-in the frame size of this method or function as...
  //       variables + args (not incl 16 bytes)
  methOrFunction->frameSize = -lastOffset - 8;
  if (commandOptionS) {
    printf ("              Total size of activation record = %d bytes\n",
            methOrFunction->frameSize);
  }

  // Print out the parameter offsets, if commandOptionS is set...
  printParmOffsets (methOrFunction->parmList);

}



// assignOffsetToLocal (lastOffset, varDecl, commandOptionS, doingBytes)
//                                                         --> newLastOffset
//
// This routine is passed "varDecl" (a LOCAL or Catch PARAMETER).  It is passed 
// "lastOffset" and returns the modified "lastOffset".  "LastOffset" is the
// offset assigned to the previous variable; we will place this variable at
// an offset determined by its size and where the last variable was placed.
//
// If "commandOptionS" is set, this routine will print a line on stdout.
// If "doingBytes" is set and this variable has is not a byte-sized
// variable, we just do nothing.  If "doingBytes" is not set, then we only do
// variables that are larger than 1 byte.
//
int assignOffsetToLocal (int lastOffset,
                         VarDecl * varDecl,
                         int commandOptionS,
                         int doingBytes) {
  int sz, newLastOffset;

  // Determine the size of this variable...
  sz = varDecl->sizeInBytes;

  // If we are doing byte-sized data and this is a byte, then do it.
  // If we are NOT doing byte-sized data and this is NOT a byte, then do it.
  // Otherwise, return immediately.
  if (sz == 1) {
    if (!doingBytes) return lastOffset;
  } else {
    if (doingBytes) return lastOffset;
  }

  // Decrement "lastOffset" and set this variable's "offset" field...
  newLastOffset = lastOffset - sz;
  varDecl->offset = newLastOffset;

  // If printing is desired, then print a line...
  if (commandOptionS) {
    printf ("          Offset %d:\t%d\t%s\n", varDecl->offset, sz,
                                      varDecl->id->chars);
  }
  return newLastOffset;
}



// printParmOffsets (parmList)
//
// This routine prints out the offsets of the parameters in the given parmList.
//
void printParmOffsets (Parameter * parmList) {
  int nextOffset, padding, incr;
  Parameter * parm;

  // Print the parameter offsets layout, if desired...
  if (!commandOptionS) return;

  if (parmList == NULL) {
    printf ("      (no parameters)\n");
    return;
  }

  printf ("      Parameter Offsets:\n");
  nextOffset = parmList->offset;
  for (parm = parmList;
       parm != NULL;
       parm = (Parameter *) parm->next) {

    // Display padding bytes, if necessary...
    if (needsAlignment (parm->type)) {
      padding = (3 - ((nextOffset+3) % 4));
      if ((parm->offset >= 0) && (padding != 0)) {
        printf ("          Offset %d:\t%d\t...padding...\n", nextOffset, padding);
        nextOffset += padding;
      }
    }
    printf ("          Offset %d:\t%d\t",
      parm->offset,
      sizeInBytesOfWhole (parm->type, parm, 0));
    printString (stdout, parm->id);
    // printf (": ");
    // pretty (parm->type);
    printf ("\n");
    incr = sizeInBytesOfWhole (parm->type, parm, 0);
    nextOffset = nextOffset + incr;
  }
  padding = (3 - ((nextOffset+3) % 4));
  if (padding != 0) {
    printf ("          Offset %d:\t%d\t...padding...\n", nextOffset, padding);
  }
}



// assignDispatchTableOffsets (hdr)
//
// This routine assigns offset to all selectors in classes and interfaces.
//
// First, it fills in the following sets:
//       knownSubAbstracts
//       SuperAbstractsInThisPackage
//
// Then, it...
//
//  Message naming convention...
//    Inherited message (with no overriding):
//      The sub-abstract will have the message name, unaltered.
//    Inherited message (with overriding):
//      The sub-abstract will have the message name, unaltered; this is the new version.
//      The sub-abstract will have the _super_ version; this is the inherited method.
//    A sub-abstract of this:
//      mess  -  the new version
//      _super_mess  --  the parent's version
//      _super__super_mess  --  the grandparent's version.
//
void assignDispatchTableOffsets (Header * hdr) {
  Interface * inter;
  ClassDef * cl;
  Abstract * absList, * abs, * nextAbs, * subAbs, * other;
  Offset * offset, * prevOff;
  AbstractStack * st;
  String * sel, * otherSel;
  MethodProto * methProto;
  Mapping<Abstract,Abstract> * relatedSet, * affectedSet;
  int okay;
  MethodProto * methProto2;

  // Initialize the Dispatch Table offset list...
  if (firstDispatchOffset == NULL) {
    offset = new Offset;
    offset->ivalue = 4;        // This is the offset of the first message
    offset->nextOffset = NULL;
    firstDispatchOffset = offset;
  }

  // Build a list of all interfaces and classes, in topo-order.  All
  // Interfaces will come first followed by all classes.  Each will
  // come before is sub-classes and sub-interfaces.  Together, classes
  // and interfaces are referred to as "abstracts".
  //
  // Also initialize the per-abstract mappings.
  //
  absList = NULL;
  // Add all interfaces to the list.
  for (inter = hdr->interfaces; inter; inter = inter->next) {
    inter->nextAbstract = absList;
    absList = inter;
  }
  // Add all classes to the list.
  for (cl = hdr->classes; cl; cl = cl->next) {
    cl->nextAbstract = absList;
    absList = cl;
  }
  // Reverse the list and initialize the pre-abstract fields.
  abs = absList;
  absList = NULL;
  while (abs) {
    nextAbs = abs->nextAbstract;
    abs->knownSubAbstracts               = new Mapping <Abstract, Abstract> (10, NULL);
    abs->superAbstractsInThisPackage     = new Mapping <Abstract, Abstract> (10, NULL);
    abs->supersAbstractsInOtherPackages  = new Mapping <Abstract, Abstract> (10, NULL);
    abs->offsetToSelector                = new Mapping <Offset, String> (10, NULL);
    abs->selectorToOffset                = new Mapping <String, Offset> (10, NULL);
    abs->unavailableOffsets              = new Mapping <Offset, String> (10, NULL);
    abs->nextAbstract = absList;
    absList = abs;
    abs = nextAbs;
  }

  // DEBUGGING: Print a list of all Abstracts in this package...
  //    printf ("Here are all abstracts in this package:\n");
  //    for (abs = absList; abs; abs = abs->nextAbstract) {
  //      printf ("  %s\n", abs->id->chars);
  //    }

  // Run through all abstracts in this package and identify all super-abstracts
  // in this package, including self, direct, and indirect supers.
  // Also follow the chains up until we hit any abstract(s) in other packages,
  // but then stop and go no further.  Add all such "minimal" abstracts in 
  // pther packages to "supersAbstractsInOtherPackages".
  //
  //    printf ("Identifying Super-Abstracts...\n");
  for (abs = absList; abs; abs = abs->nextAbstract) {
    //    printf ("  Processing %s\n", abs->id->chars);
    addAllSuperAbstracts (abs,
                          hdr,
                          abs->superAbstractsInThisPackage,
                          abs->supersAbstractsInOtherPackages);
  }

  // Now run through the list of all abstracts and fill in all sub-abstracts
  // that are in this package...
  //
  for (abs = absList; abs; abs = abs->nextAbstract) {
    //    printf ("%s...\n", abs->id->chars);
    nextAbs = abs->superAbstractsInThisPackage->getFirst();
    while (nextAbs) {
      //    printf ("  %s is a super-abstract\n", nextAbs->id->chars);
      if (! nextAbs->knownSubAbstracts->alreadyDefined (abs)) {
        nextAbs->knownSubAbstracts->enter (abs, abs);
      }
      nextAbs = abs->superAbstractsInThisPackage->getNext();
    }
  }  

/*****  PRINTING FOR DEBUGGING  *****
  //
  // List all Abstracts in this package...
  printf ("Here are all abstracts in this package, with their super-Abstracts:\n");
  for (abs = absList; abs; abs = abs->nextAbstract) {
    printf ("  %s\n", abs->id->chars);
    printf ("    SUPER-ABSTRACTS:\n");
    nextAbs = abs->superAbstractsInThisPackage->getFirst();
    while (nextAbs) {
      printf ("      %s\n", nextAbs->id->chars);
      nextAbs = abs->superAbstractsInThisPackage->getNext();
    }
    printf ("    SUB-ABSTRACTS:\n");
    nextAbs = abs->knownSubAbstracts->getFirst();
    while (nextAbs) {
      printf ("      %s\n", nextAbs->id->chars);
      nextAbs = abs->knownSubAbstracts->getNext();
    }
    printf ("    MINIMAL SUPER-ABSTRACTS NOT IN THIS PACKAGE:\n");
    nextAbs = abs->supersAbstractsInOtherPackages->getFirst();
    while (nextAbs) {
      printf ("      %s\n", nextAbs->id->chars);
      nextAbs = abs->supersAbstractsInOtherPackages->getNext();
    }
  }
***************/



  // **********  PASS-1: INHERIT OFFSETS FROM SUPERS IN OTHER PACKAGES  **********
  //
  // Go through all abstracts and inherit all offsets from super-abstracts
  // that are in other packages.  Watch for conflicts and print an error if
  // we run into trouble.
  //
  for (abs = absList; abs; abs = abs->nextAbstract) {
    //    printf ("=======================  %s  =======================\n",
    //            abs->id->chars);

    // Go through all super-abstracts in other packages...
    nextAbs = abs->supersAbstractsInOtherPackages->getFirst();
    while (nextAbs) {
      //    printf ("  Inheriting messages from %s\n", nextAbs->id->chars);

      // Go through all messages in this super-abstract...
      sel = nextAbs->offsetToSelector->getFirst ();
      while (sel) {
        offset = nextAbs->selectorToOffset->findInTopScope (sel);
        //    printf ("    %s: %d\n", sel->chars, offset->ivalue);

        // If we already have an offset for this selector...
        prevOff = abs->selectorToOffset->findInTopScope (sel);
        if (prevOff) {
          // Make sure it is the same...
          if (prevOff != offset) {      // This error occurs in "test9>>F>>mess2"
            //    printf ("***** CONFLICT (1): selector = %s\n",
            //            sel->chars);
            //    printf ("                    abs = %s        prevOff = %d\n",
            //            abs->id->chars, prevOff->ivalue);
            //    printf ("                    nextAbs = %s    offset = %d\n",
            //            nextAbs->id->chars, offset->ivalue);
            error (abs, "A problem has been encountered when laying out the dispatch table for this class or interface.  This class or interface inherits a message from two or more classes or interfaces which are in a different package from the one being compiled now.  When the other package was compiled, this message happens to have been assigned different offsets in the different super-classes or super-interfaces.  This was possible since the classes or interfaces in the other package were unrelated (e.g., had no common sub-class or sub-interface).  The class or interface in which this problem is arising inherits this message from these previously unrelated classes or interfaces.  The problem is that it is not possible now to assign this message a single offset consistent with all super-classes and super-interfaces, without changing the offset assigned to this message in the super-classes or super-interfaces.  You may resolve this problem by forcing this message to be given identical offsets in all relevant super-classes and super-interfaces.  This can be done by adding a dummy class or interface to the other package and making it extend all relevant classes or interfaces in that other package.  (Inheritance conflict type 1)");
            methProto2 = abs->selectorMapping->findInTopScope (sel);
            if (methProto2) {
              error2 (methProto2, "Here is the message that is causing the problem");
            }
            error2 (nextAbs, "Here is one of the classes or interfaces in other packages that are contributing to the problem");

          //    } else {      // This case occurs in "test9>>W>>mess2"
          //      printf ("      This selector already has the same offset...\n");
          }

        // Else if no offset yet, then assign this offset...
        } else {
          //    printf ("      Assigning offset %d to %s in %s...\n",
          //            offset->ivalue,
          //            sel->chars,
          //            abs->id->chars);
          abs->selectorToOffset->enter (sel, offset);
          abs->offsetToSelector->enter (offset, sel);

          // Now run through all supers of this abstract that occur in
          // in the same package and mark this offset as unavailable...
          other = abs->superAbstractsInThisPackage->getFirst();
          while (other) {
            //    printf ("        Marking this offset unavailable in %s\n",
            //            other->id->chars);
            if (! other->unavailableOffsets->findInTopScope (offset)) {
              other->unavailableOffsets->enter (offset, sel);
            }
            other = abs->superAbstractsInThisPackage->getNext();
          }

        } 
        sel = nextAbs->offsetToSelector->getNext ();
      }
      nextAbs = abs->supersAbstractsInOtherPackages->getNext();
    }
  }
  // ********************  END OF PASS-1  ********************



  // **********  PASS-2: ASSIGN OFFSETS TO ALL REMAINING MESSAGES  **********
  //
  // Go through all abstracts.  For each, go through all message selectors.
  //
  // For each selector, begin by identifying all related abstracts with this
  // selector.  (These are abstracts that are related by any chain of sub and/or
  // super and that contain the given message.  Call this the "related" set.)
  //
  // Then identify all classes that will have to block out this offset for that
  // selector.  Call this the "affected" set.  (It includes the related classes.)
  //
  // Next find an offset for this selector.  Start by looking through the "related"
  // set and see if any of them give the selector an offset.  If so, use it.
  // Otherwise, start looking through all possible offsets, one-by-one.  For
  // each offset, see if it is available for use (i.e., not in "unavailableOffsets")
  // for each of the abstracts in the "related" set.
  //
  // Once an offset has been identified, then set the offset for this selector in all
  // abstracts in the "related" set.  Watch for conflicts; this selector could be
  // given a different offset in some abstracts (due to conflicting inheritance from
  // abstracts in other packages).  Also, mark this offset as unavailable in all
  // abstracts in the "affected" set.
  //  

  // Look at each abstract...
  for (abs = absList; abs; abs = abs->nextAbstract) {
    //    printf ("=======================  %s  =======================\n",
    //            abs->id->chars);

    // Look at each message that is in this abstract...
    methProto = abs->selectorMapping->getFirst();
    while (methProto) {
      sel = methProto->selector;
      //    printf ("%s:\n", sel->chars);

      // If this selector already has an offset, then skip to the next selector...
      if (abs->selectorToOffset->findInTopScope (sel)) {
        offset = abs->selectorToOffset->findInTopScope (sel);
        //    printf ("    This message has already been assigned offset %d.\n",
        //            offset->ivalue);
        methProto = abs->selectorMapping->getNext();
        continue;
      }

      // Create the "related" set.  Begin by creating a stack of all abstracts
      // that still to be looked at and push this abstract onto it.
      st = newStack ();
      stackPush (st, abs);
      relatedSet = new Mapping <Abstract, Abstract> (10, NULL);
      // While there is another abstract to consider...
      while (!stackEmpty (st)) {
        nextAbs = stackPop (st);
        //    printf ("  Popping and considering %s...\n", nextAbs->id->chars);
        // If this abstract contains this selector, then we'll need to look at it...
        if (nextAbs->selectorMapping->findInTopScope (sel)) {
          // If it is not already in the "related" set then...
          if (! relatedSet->findInTopScope (nextAbs)) {
            // Add it...
            relatedSet->enter (nextAbs, nextAbs);
            // Run through all subs and push them...
            //    printf ("    pushing these sub-Abstracts:\n");
            other = nextAbs->knownSubAbstracts->getFirst();
            while (other) {
              //    printf ("      %s\n", other->id->chars);
              stackPush (st, other);
              other = nextAbs->knownSubAbstracts->getNext();
            }
            // Run through all supers and push them...
            //    printf ("    pushing these super-Abstracts:\n");
            other = nextAbs->superAbstractsInThisPackage->getFirst();
            while (other) {
              //    printf ("      %s\n", other->id->chars);
              stackPush (st, other);
              other = nextAbs->superAbstractsInThisPackage->getNext();
            }
          }
        }
      }

      // Now compute the "affected set".  This is the set of all super-abstracts
      // of all abstracts in the "related set", that are in this package.  (These
      // are abstracts which may or may not contain the message, but which will
      // need to have the offset occupied by this message reserved; they cannot
      // place any other message at this offset.)
      affectedSet = new Mapping <Abstract, Abstract> (10, NULL);
      nextAbs = relatedSet->getFirst();
      while (nextAbs) {
        // Run through all supers in this package, and add them if not
        // already in the "affected set"...
        other = nextAbs->superAbstractsInThisPackage->getFirst();
        while (other) {
          //    printf ("      %s\n", other->id->chars);
          if (! affectedSet->findInTopScope (other)) {
            affectedSet->enter (other, other);
          }
          other = nextAbs->superAbstractsInThisPackage->getNext();
        }
        nextAbs = relatedSet->getNext();
      }

      // DEBUGGING: Print the related set...
      //    printf ("    RELATED-SET:  ");
      //    nextAbs = relatedSet->getFirst();
      //    while (nextAbs) {
      //      printf ("%s  ", nextAbs->id->chars);
      //      nextAbs = relatedSet->getNext();
      //    }
      //    printf ("\n");

      // DEBUGGING: Print the affected set...
      //    printf ("    AFFECTED-SET:  ");
      //    nextAbs = affectedSet->getFirst();
      //    while (nextAbs) {
      //      printf ("%s  ", nextAbs->id->chars);
      //      nextAbs = affectedSet->getNext();
      //    }
      //    printf ("\n");

      // Look through all abstracts in the "related" set.  See if any one of
      // them assigns this selector an offset.  (This could occur if one of
      // the abstracts inherits an offset from a super in another package.)
      // If so, we will have to use this offset.
      // (This situation occurs in Test9>>II>>m.)
      offset = NULL;
      nextAbs = relatedSet->getFirst ();
      while (nextAbs) {
        offset = nextAbs->selectorToOffset->findInTopScope (sel);
        if (offset != NULL) {
          //    printf ("  The abstract %s in the related set gives this selector offset %d\n",
          //            nextAbs->id->chars, offset->ivalue);
          break;
        }
        nextAbs = relatedSet->getNext ();
      }
      
      // If we did not have an offset imposed by a related abstract, then
      // find an offset that will work...
      if (offset == NULL) {
        //    printf ("    Searching for a new offset for this selector...\n");
        offset = firstDispatchOffset;
        while (1) {
          if (isThisOffsetOK (sel, offset, relatedSet)) {
            break;
          }
          offset = nextDispatchOffset (offset);
        }
        //    printf ("      Offset %d is chosen\n", offset->ivalue);
      }

      // Assign this offset to this selector in all abstracts in the
      // "relatedSet".  Also, mark this offset as unavailable in all the
      // abstracts in the "affectedSet"...
      assignOffsetToSelector (sel, offset, relatedSet, affectedSet);

      // Move on to the next selector...
      methProto = abs->selectorMapping->getNext();
    }

    // Print "offsetToSelector", "selectorToOffset", and "unavailableOffsets"...
    //    abs->offsetToSelector->printOffsetToSelector("OFFSET-TO-SELECTOR:");
    //    abs->selectorToOffset->printSelectorToOffset();
    //    abs->unavailableOffsets->printOffsetToSelector("UNAVAILABLE-OFFSETS:");


    // Now go through all the methodProtos in this class or interface and
    // set their offsets...
    //    printf ("    %s:\n", abs->id->chars);
    methProto = abs->selectorMapping->getFirst();
    while (methProto) {
      offset = abs->selectorToOffset->findInTopScope (methProto->selector);
      if (offset == NULL) {
        programLogicError ("Offset should have been assigned for all selectors");
      }
      methProto->offset = offset->ivalue;
      //    printf ("      %s: %d\n", methProto->selector->chars, methProto->offset);
      methProto = abs->selectorMapping->getNext();
    }
  }

}



// isThisOffsetOK (sel, offset, setToCheck) --> bool
//
// This routine searches the given set to see if this offset is okay for
// use.  It checks to see if this offset is in the "unavailableOffsets" mapping
// in any of the abstracts in the "setToCheck".  If so, then if it is associated
// with the given selector ("sel") then it is okay; return true.  If it is in the
// "unavailableOffsets" mapping, but for a different selector, then return false.
//
int isThisOffsetOK (String * sel,
                    Offset * offset,
                    Mapping<Abstract,Abstract> * setToCheck) {
  Abstract * abs;
  String * otherSel;

  //    printf ("        Considering %s: %d...\n", sel->chars, offset->ivalue);
  abs = setToCheck->getFirst();
  while (abs) {
    //    printf ("          Checking %s...\n", abs->id->chars);
    otherSel = abs->unavailableOffsets->findInTopScope (offset);
    if (otherSel) {
      // if (otherSel == sel) {
      //   programLogicError ("This cannot occur");
      // }
      return 0;
    }
    abs = setToCheck->getNext();
  }
  return 1;
}



// assignOffsetToSelector (sel, offset, relatedSet, affectedSet)
//
// This routine assigns this offset to this selector in the abstracts in
// the "relatedSet".  It then marks this offset as unavailable for all
// the abstracts in the "affectedSet".
//
void assignOffsetToSelector (String * sel,
                             Offset * offset,
                             Mapping <Abstract, Abstract> * relatedSet,
                             Mapping <Abstract, Abstract> * affectedSet) {
  Abstract * abs, * superAbs;
  Offset * prevOff;
  MethodProto * methodProto;

  // Run through all abstracts in "related set"...
  abs = relatedSet->getFirst();
  while (abs) {
    prevOff = abs->selectorToOffset->findInTopScope (sel);
    if (prevOff) {

      // If we found an abstract which has this selector at a different offset...
      if (prevOff != offset) {
        //    printf ("***** CONFLICT (2): selector = %s\n",
        //            sel->chars);
        //    printf ("                    abs = %s        prevOff = %d\n",
        //            abs->id->chars, prevOff->ivalue);
        //    printf ("                    nextAbs = ?    offset = %d\n",
        //            offset->ivalue);
        error (abs, "A problem has been encountered when laying out the dispatch table for this class or interface.  This class or interface shares a message with other classes or interfaces via inheritance.  Two or more related classes or interfaces in this package inherit this message from classes or interfaces which are in a different package from the one being compiled now.  When the other package was compiled, this message happens to have been assigned different offsets in the different super-classes or super-interfaces in that package.  This was possible since the classes or interfaces in the other package were unrelated (e.g., had no common sub-class or sub-interface).  The class or interface in which this problem is arising must assign an offset to this message that is compatible with these (previously unrelated) classes or interfaces in the other package.  The problem is that it is not possible now to assign this message a single offset consistent with all related classes and interfaces, since it is not possible to change the offset assigned to this message in classes or interfaces in other packages.  You may resolve this problem by forcing this message to be given identical offsets in all relevant super-classes and super-interfaces.  This can be done by adding a dummy class or interface to the other package and making it extend all relevant classes or interfaces in that other package.  (Inheritance conflict type 2)");
        methodProto = abs->selectorMapping->findInTopScope (sel);
        if (methodProto) {
          error2 (methodProto, "Here is the message that is causing the problem");
        }

      // If we found another abstract which has this selector at the same offset...
      } else {
        //    printf ("        Offset in %s was found to be identical\n",
        //            abs->id->chars);
      }

    // Else this abstract does not have an offset for this selector...
    } else {

      // Assign this offset to this selector...
      //    printf ("        Setting offset to %d in %s\n",
      //            offset->ivalue, abs->id->chars);
      abs->selectorToOffset->enter (sel, offset);
      abs->offsetToSelector->enter (offset, sel);

      // Make sure this offset is compatible with any supers in other packages.
      // It must be, since we already inherited offsets from supers in the first pass,
      // so signal a logic error if we find any inconsistencies.
      superAbs = abs->supersAbstractsInOtherPackages->getFirst ();
      while (superAbs) {
        prevOff = superAbs->selectorToOffset->findInTopScope (sel);
        if (prevOff != NULL && prevOff != offset) {
          printf ("***** CONFLICT (3) : selector = %s\n",
                  sel->chars);
          printf ("                     abstr = %s    offset = %d\n",
                  abs->id->chars, offset->ivalue);
          printf ("                     super = %s    offset=%d\n",
                  superAbs->id->chars, prevOff->ivalue);
          programLogicError ("We have already inherited all messages from supers");
        }
        superAbs = abs->supersAbstractsInOtherPackages->getNext ();
      }

    }
    abs = relatedSet->getNext();
  }
 
  // Run through all Abstracts in "affected set"...
  abs = affectedSet->getFirst();
  while (abs) {
    //    printf ("      Marking offset %d unavailable in %s\n",
    //            offset->ivalue,
    //            abs->id->chars);
    // If this offset is not already marked "unavailable", then mark it as such...
    if (! abs->unavailableOffsets->findInTopScope (offset)) {
      abs->unavailableOffsets->enter (offset, sel);
    }
    abs = affectedSet->getNext();
  }
 
}



// addAllSuperAbstracts (abs, hdr, supersInThisPackage, supersInOtherPackages)
//
// This routine is passed an abstract "abs".  It visits each of its superclasses
// superInterfaces and adds each of them (if they are in this package) to the
// set "supersInThisPackage".  If the super is not in this package,
// then we add it to "supersInOtherPackages" and we follow the chain no
// further upward.
//
void addAllSuperAbstracts (Abstract * abs,
                           Header * hdr,
                           Mapping <Abstract, Abstract> * supersInThisPackage,
                           Mapping <Abstract, Abstract> * supersInOtherPackages) {
  ClassDef * cl, * supercl;
  Interface * inter, * superInter;
  TypeArg * typeArg;
  NamedType * nType;

  //    printf ("    %s: CALLED; adding this to supersInThisPackage\n", abs->id->chars);

  // Only consider Abstracts in this package...
  if (abs->myHeader != hdr) {
    supersInOtherPackages->enter (abs, abs);
    return;
  }

  // Put this abstract into the set if it is not already there...
  if (! supersInThisPackage->alreadyDefined (abs)) {
    supersInThisPackage->enter (abs, abs);
  }

  if (abs->op == CLASS_DEF) {
    cl = (ClassDef *) abs;
    // Identify the superclass, if any...
    //    printf ("    %s:   Looking at the superclass...\n", abs->id->chars);
    nType = cl->superclass;
    if (nType &&
        nType->myDef &&
        nType->myDef->op == CLASS_DEF) {
      supercl = (ClassDef *) nType->myDef;
      // Recursively consider adding it to the set.
      addAllSuperAbstracts (supercl,
                            hdr,
                            supersInThisPackage,
                            supersInOtherPackages);
    }
    for (typeArg = cl->implements; typeArg; typeArg = typeArg->next) {
      //    printf ("    %s:   Looking next interface in the implements clause...\n",
      //            abs->id->chars);
      nType = (NamedType *) typeArg->type;
      if (nType &&
          nType->op == NAMED_TYPE &&
          nType->myDef &&
          nType->myDef->op == INTERFACE) {
        superInter = (Interface *) nType->myDef;
        //    printf ("    %s:     Looking at interface %s...\n",
        //            abs->id->chars, nType->id->chars);
        // Recursively consider adding it to the set.
        addAllSuperAbstracts (superInter,
                              hdr,
                              supersInThisPackage,
                              supersInOtherPackages);
      }
    }
  } else if (abs->op == INTERFACE) {
    inter = (Interface *) abs;
    for (typeArg = inter->extends; typeArg; typeArg = typeArg->next) {
      //    printf ("    %s:   Looking next interface in the extends clause...\n",
      //            abs->id->chars);
      nType = (NamedType *) typeArg->type;
      if (nType &&
          nType->op == NAMED_TYPE &&
          nType->myDef &&
          nType->myDef->op == INTERFACE) {
        superInter = (Interface *) nType->myDef;
        //    printf ("    %s:     Looking at interface %s...\n",
        //            abs->id->chars, nType->id->chars);
        // Recursively consider adding it to the set.
        addAllSuperAbstracts (superInter,
                              hdr,
                              supersInThisPackage,
                              supersInOtherPackages);
      }
    }
  } else {
    programLogicError ("Unexpected node in addAllSuperAbstracts");
  }
  //    printf ("    %s: DONE\n", abs->id->chars);
}



// nextDispatchOffset (offset)  -->  offset
//
// This routine is passed an offset; it returns the next, successor offset.  It will
// see if we have already created this offset.  If so, it will return a pre-existing
// offset.  (This allows offsets to be compared with ==.)  If not, it will create
// a new offset and add it to the linked list as the successor of the given offset.
// Once created, an offset is never freed.
//
Offset * nextDispatchOffset (Offset * previousOffset) {
  Offset * newOff = previousOffset->nextOffset;
  if (newOff) return newOff;
  newOff = new Offset;
  // The value used here is the increment between message offsets in dispatch tables...
  newOff->ivalue = previousOffset->ivalue + 4;
  newOff->nextOffset = NULL;
  previousOffset->nextOffset = newOff;
  return newOff;
}



// stackPush (stack, Abstract)
//
// The routine "stackPush", "stackPop", "stackEmpty", and "newStack" are used
// to manipulate stacks of Abstracts.  A stack is represented as a pointer to
// a linked list of "AbstractStack" structs.  The top element is a dummy
// element, with "element" == NULL.  The linked list always contains this dummy
// element.
//
// This routine pushes an Abstract onto the stack given stack.
//
void stackPush (AbstractStack * st, Abstract * elt) {
  AbstractStack * n = new AbstractStack;
  n->element = elt;
  n->next = st->next;
  st->next = n;
}



// stackPop (stack)   -->  Abstract
//
// This routine pops the top element from a stack and returns it.
// If the stack is empty, a programLogicError will be generated.
//
Abstract * stackPop (AbstractStack * st) {
  Abstract * result;
  AbstractStack * old = st->next;
  if (old == NULL) {
    programLogicError ("Attempt to pop from an empty stack of Abstracts");
  }
  result = old->element;
  st->next = old->next;
  delete old;
  return result; 
}



// stackEmpty (stack)
//
// This routine returns true if the given stack is empty.
//
int stackEmpty (AbstractStack * st) {
  if (st->next == NULL) return 1;
  return 0;
}



// newStack ()  --> stack
//
// This routine returns a dummy element and is used to initialize the stack.
//
AbstractStack * newStack () {
  AbstractStack * n = new AbstractStack;
  n->element = NULL;
  n->next = NULL;
  return n;
}
