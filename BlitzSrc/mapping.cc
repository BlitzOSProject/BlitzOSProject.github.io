// mapping.cc  --  Driver routine for the compiler; basic control routines
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
//   07/30/02 - Harry H. Porter III
//
// Modifcations by:
//   03/15/06 - Harry H. Porter III
//   05/01/07 - Harry H. Porter III - Bug fix concerning searching empty mappings
//
//

#include "main.h"




// instantiateMappings ()
//
// This routine is never called.  The purpose of it is to force the C++
// compiler to generate code for the various methods that are needed.
//
void instantiateMappings () {
  Mapping <String, Header>      * dummyMap1;
  Mapping <String, String>      * dummyMap2;
  Mapping <String, TypeParm>    * dummyMap3;
  Mapping <TypeParm, Type>      * dummyMap4;
  Mapping <String, MethodProto> * dummyMap5;
  Mapping <String, Method>      * dummyMap6;
  Mapping <String, RecordField> * dummyMap7;
  Mapping <String, ErrorDecl>   * dummyMap8;
  Mapping <Abstract, Abstract>  * dummyMap9;
  Mapping <Offset, String>      * dummyMap10;
  Mapping <String, Offset>      * dummyMap11;
  Mapping <String, AstNode>     * dummyMap12;

  dummyMap1 = new Mapping <String, Header> (0, NULL);
  dummyMap1->enter (NULL, NULL);
  dummyMap1->find (NULL);
  dummyMap1->findInTopScope (NULL);
  dummyMap1->alreadyDefined (NULL);
  dummyMap1->print (0);
  dummyMap1->getFirst ();
  dummyMap1->getNext ();
  dummyMap1->getItsKey ();
  delete dummyMap1;

  dummyMap2 = new Mapping <String, String> (0, NULL);
  dummyMap2->enter (NULL, NULL);
  dummyMap2->find (NULL);
  dummyMap2->findInTopScope (NULL);
  dummyMap2->alreadyDefined (NULL);
  dummyMap2->print (0);
  dummyMap2->getFirst ();
  dummyMap2->getNext ();
  dummyMap2->getItsKey ();
  delete dummyMap2;

  dummyMap3 = new Mapping <String, TypeParm> (0, NULL);
  dummyMap3->enter (NULL, NULL);
  dummyMap3->find (NULL);
  dummyMap3->findInTopScope (NULL);
  dummyMap3->alreadyDefined (NULL);
  dummyMap3->print (0);
  dummyMap3->getFirst ();
  dummyMap3->getNext ();
  dummyMap3->getItsKey ();
  delete dummyMap3;

  dummyMap4 = new Mapping <TypeParm, Type> (0, NULL);
  dummyMap4->enter (NULL, NULL);
  dummyMap4->find (NULL);
  dummyMap4->findInTopScope (NULL);
  dummyMap4->alreadyDefined (NULL);
  dummyMap4->print (0);
  dummyMap4->getFirst ();
  dummyMap4->getNext ();
  dummyMap4->getItsKey ();
  delete dummyMap4;

  dummyMap5 = new Mapping <String, MethodProto> (0, NULL);
  dummyMap5->enter (NULL, NULL);
  dummyMap5->find (NULL);
  dummyMap5->findInTopScope (NULL);
  dummyMap5->alreadyDefined (NULL);
  dummyMap5->print (0);
  dummyMap5->getFirst ();
  dummyMap5->getNext ();
  dummyMap5->getItsKey ();
  delete dummyMap5;

  dummyMap6 = new Mapping <String, Method> (0, NULL);
  dummyMap6->enter (NULL, NULL);
  dummyMap6->find (NULL);
  dummyMap6->findInTopScope (NULL);
  dummyMap6->alreadyDefined (NULL);
  dummyMap6->print (0);
  dummyMap6->getFirst ();
  dummyMap6->getNext ();
  dummyMap6->getItsKey ();
  delete dummyMap6;

  dummyMap7 = new Mapping <String, RecordField> (0, NULL);
  dummyMap7->enter (NULL, NULL);
  dummyMap7->find (NULL);
  dummyMap7->findInTopScope (NULL);
  dummyMap7->alreadyDefined (NULL);
  dummyMap7->print (0);
  dummyMap7->getFirst ();
  dummyMap7->getNext ();
  dummyMap7->getItsKey ();
  delete dummyMap7;

  dummyMap8 = new Mapping <String, ErrorDecl> (0, NULL);
  dummyMap8->enter (NULL, NULL);
  dummyMap8->find (NULL);
  dummyMap8->findInTopScope (NULL);
  dummyMap8->alreadyDefined (NULL);
  dummyMap8->print (0);
  dummyMap8->getFirst ();
  dummyMap8->getNext ();
  dummyMap8->getItsKey ();
  delete dummyMap8;

  dummyMap9 = new Mapping <Abstract, Abstract> (0, NULL);
  dummyMap9->enter (NULL, NULL);
  dummyMap9->find (NULL);
  dummyMap9->findInTopScope (NULL);
  dummyMap9->alreadyDefined (NULL);
  dummyMap9->print (0);
  dummyMap9->getFirst ();
  dummyMap9->getNext ();
  dummyMap9->getItsKey ();
  delete dummyMap9;

  dummyMap10 = new Mapping <Offset, String> (0, NULL);
  dummyMap10->enter (NULL, NULL);
  dummyMap10->find (NULL);
  dummyMap10->findInTopScope (NULL);
  dummyMap10->alreadyDefined (NULL);
  dummyMap10->print (0);
  dummyMap10->getFirst ();
  dummyMap10->getNext ();
  dummyMap10->getItsKey ();
  dummyMap10->printOffsetToSelector ("Dummy");
  delete dummyMap10;

  dummyMap11 = new Mapping <String, Offset> (0, NULL);
  dummyMap11->enter (NULL, NULL);
  dummyMap11->find (NULL);
  dummyMap11->findInTopScope (NULL);
  dummyMap11->alreadyDefined (NULL);
  dummyMap11->print (0);
  dummyMap11->getFirst ();
  dummyMap11->getNext ();
  dummyMap11->getItsKey ();
  dummyMap11->printSelectorToOffset ();
  delete dummyMap11;

  dummyMap12 = new Mapping <String, AstNode> (0, NULL);
  dummyMap12->enter (NULL, NULL);
  dummyMap12->find (NULL);
  dummyMap12->findInTopScope (NULL);
  dummyMap12->alreadyDefined (NULL);
  dummyMap12->print (0);
  dummyMap12->getFirst ();
  dummyMap12->getNext ();
  dummyMap12->getItsKey ();
  dummyMap12->printSelectorToOffset ();
  delete dummyMap12;

}


//----------  Local to this file  ----------

void searchFor (String *);
Mapping<String, AstNode> * testMap;



// enter (key, value)
//
// Enter this key-value pair in the top scope.
//
template <class Key, class Value>
void Mapping<Key, Value> :: enter (Key * k, Value * v) {
  Bucket<Key,Value> * newBuck = new Bucket<Key,Value> ();
  int i;

  // printf ("Enter  ptr=0x%08x %d    i=%d\n", k, k, arrayIndex (k));

  rehashIfNecessary ();
  if (sizeOfArray == 0) {
    fatalError ("In Mapping>>enter; the array should have just grown");
  }
  i = arrayIndex (k);
  newBuck->key = k;
  newBuck->value = v;
  newBuck->next = array[i];
  array[i] = newBuck;
  numberOfElements++;
  if (firstInsertedBucket == NULL) {
    firstInsertedBucket = newBuck;
    lastInsertedBucket = newBuck;
  } else {
    lastInsertedBucket->nextForIterator = newBuck;
    lastInsertedBucket = newBuck;
  }

  // printf ("ENTER  0x%08x: ", newBuck);
  // printf ("   KEY=0x%08x", newBuck->key);
  // printf ("   VAL=0x%08x", newBuck->value);
  // printf ("   next=0x%08x", newBuck->next);
  // printf ("   nextForIterator=0x%08x\n", newBuck->nextForIterator);
}



// find (key) --> value
//
// Searches all scopes for the given key.  If found, return the
// corresponding value.  If not found, return NULL.
//
template <class Key, class Value>
Value * Mapping<Key, Value> :: find (Key * k) {
  Bucket<Key,Value> * buck;
  if (numberOfElements != 0) {
    int i = arrayIndex (k);
    buck = array[i];
    while (buck != NULL) {
      if (buck->key == k) return buck->value;
      buck = buck->next;
    }
  }
  if (superMap == NULL) return NULL;
  return superMap->find (k);
}



// alreadyDefined (key) --> bool
//
// Determine if an entry for this key exists.
// Search the top scope only.
//
template <class Key, class Value>
int Mapping<Key, Value> :: alreadyDefined (Key * k) {
  Bucket<Key,Value> * buck;
  if (numberOfElements == 0) {
    return 0;
  }
  int i = arrayIndex (k);
  buck = array[i];
  while (buck != NULL) {
    if (buck->key == k) return 1;
    buck = buck->next;
  }
  return 0;
}



// print (indent)
//
// Print all scopes.
//
template <class Key, class Value>
void Mapping<Key, Value> :: print (int indent) {
  int i;
  Bucket<Key, Value> * p;
  String * str;
  IntConst * val;
  Key * key;
  Value * value;
  ppIndent (indent);
  printf ("==========  Mapping  ==========\n");

/***
  printf ("  superMap = 0x%08x\n", superMap);
  printf ("  numberOfElements = %d\n", numberOfElements);
  printf ("  sizeOfArray = %d\n", sizeOfArray);
  printf ("  iteratorLastBucket = 0x%08x\n", iteratorLastBucket);
  printf ("  firstInsertedBucket = 0x%08x\n", firstInsertedBucket);
  printf ("  lastInsertedBucket = 0x%08x\n", lastInsertedBucket);
  for (i=0; i<sizeOfArray; i++) {
    p = array [i];
    printf ("Bucket list %d: 0x%08x:\n", i, p);
    while (p != NULL) {
      printf ("  0x%08x: ", p);
      printf ("  KEY=");
      printString (stdout, (String *) p->key);
      printf ("   VAL=%d", ((IntConst *) p->value)->ivalue);
      printf ("   next=0x%08x    nextForIterator=0x%08x\n", p->next, p->nextForIterator);
      p = p->next;
    }
  }
  printf ("  done.\n");
***/


/*****
  // Print each bucket list, giving an order dependent on the hash function...
  for (i=0; i<sizeOfArray; i++) {
    p = array [i];
    while (p != NULL) {
      printKeyValue (p->key, p->value, indent+6);

      // str = (String *) p->key;
      // val = (IntConst *) p->value;
      // ppIndent (indent);
      // printf ("   KEY=");
      // printString (stdout, str);
      // printf ("   VAL=%d\n", val->ivalue);

      p = p->next;
    }
  }
*****/

  // Print using the insertion order...
  for (p = firstInsertedBucket; p; p = p->nextForIterator) {
    printKeyValue (p->key, p->value, indent+6);
  }

  if (superMap != NULL) {
    superMap->print (indent+4);
  }

  ppIndent (indent);
  printf ("===============================\n");
}



// printKeyValue (key, value, indent)
//
// Print a symbol table entry.
// The value is assumed to be a pointer to a Decl, Class, Function, Method,
// TypeParm, Field, or Parameter.  It prettyPrints it.
//
// It assumes that Key=String and Value=AstNode and WILL CRASH if this is
// not the case.
//
template <class Key, class Value>
void Mapping <Key, Value> :: printKeyValue (Key * k, Value * v, int indent) {
  String * key;
  AstNode * value = (AstNode *) v;

  if (((AstNode *) k)->op == TYPE_PARM) {
    key = ((TypeParm *) k) -> id;
  } else {
    key = (String * ) k;
  }

  ppIndent (indent);
  printString (stdout, key);
  printf (":\n");
  switch (value->op) {
    case GLOBAL:
    case LOCAL:
    case PARAMETER:
    case CLASS_FIELD:
    case RECORD_FIELD:
      ppIndent (indent+4);
      printf ("%s ", symbolName (value->op));
      value->prettyPrint (indent+8);
      printf ("\n");
      break;
    case ERROR_DECL:
      ppIndent (indent+4);
      printf ("ERROR_DECL ");
      value->prettyPrint (indent+4);
      printf ("\n");
      break;
    case CONST_DECL:
      ppIndent (indent+4);
      printf ("CONST_DECL ");
      value->prettyPrint (indent+4);
      printf ("\n");
      break;
    case FUNCTION_PROTO:
      ppIndent (indent+4);
      printf ("FUNCTION_PROTO ");
      value->prettyPrint (indent+4);
      printf ("\n");
      break;
    case METHOD_PROTO:
      ppIndent (indent+4);
      printf ("METHOD_PROTO ");
      value->prettyPrint (indent+4);
      printf ("\n");
      break;
    case METHOD:
      ppIndent (indent+4);
      printf ("METHOD ");
      value->prettyPrint (indent+4);
      printf ("\n");
      break;
    case TYPE_DEF:
      ppIndent (indent+4);
      printf ("TYPE_DEF ");
      value->prettyPrint (indent+4);
      printf ("\n");
      break;
    case CLASS_DEF:
      ppIndent (indent+4);
      printf ("CLASS_DEF ");
      printString (stdout, ((ClassDef *) value)->id);
      printf ("\n");
      break;
    case INTERFACE:
      ppIndent (indent+4);
      printf ("INTERFACE ");
      printString (stdout, ((ClassDef *) value)->id);
      printf ("\n");
      break;
// NEED TO FIX...  PARAMETER->prettyPrint will print ", " and next...
    case TYPE_PARM:
      ppIndent (indent+4);
      printf ("TYPE_PARM ");
      printString (stdout, ((TypeParm *) value)->id);
      printf ("[%08x]", value);
      printf ("\n");
      break;
    case CHAR_TYPE:
    case INT_TYPE:
    case DOUBLE_TYPE:
    case BOOL_TYPE:
    case VOID_TYPE:
    case TYPE_OF_NULL_TYPE:
    case FUNCTION_TYPE:
    case NAMED_TYPE:
    case ARRAY_TYPE:
    case PTR_TYPE:
      ppIndent (indent+4);
      printf ("%s ", symbolName (value->op));
      ((Type *) value)->prettyPrint (0);
      printf ("\n");
      break;
    case INT_CONST:
      ppIndent (indent+4);
      printf ("INT_CONST ");
      ((IntConst *) value)->prettyPrint (0);
      printf ("\n");
      break;
    case HEADER:
      value->prettyPrint (indent+8);
      break;
/*****
    case FUNCTION:
      ppIndent (indent+4);
      printf ("FUNCTION ");
      printString (stdout, ((Function *) value)->id);
      printf ("\n");
      break;
    case METHOD:
      ppIndent (indent+4);
      printf ("METHOD ");
      printString (stdout, ((Method *) value)->id);
      printf ("\n");
      break;
*****/
    default:
      programLogicError ("Unkown op in printKeyValue");
  }
}



// printOffsetToSelector (title)
//
template <class Key, class Value>
void Mapping<Key, Value> :: printOffsetToSelector (char * title) {
  int i;
  Bucket<Key, Value> * p;
  String * sel;
  Offset * off;
  Key * key;
  Value * value;
  printf ("        %s\n", title);
  // Print using the insertion order...
  for (p=firstInsertedBucket; p; p = p->nextForIterator) {
    off = (Offset *) p->key;
    sel = (String *) p->value;
    printf ("          %d: ", off->ivalue);
    printString (stdout, sel);
    printf ("\n");
  }
/******
  for (i=0; i<sizeOfArray; i++) {
    p = array [i];
    while (p != NULL) {
      off = (Offset *) p->key;
      sel = (String *) p->value;
      printf ("          %d: ", off->ivalue);
      printString (stdout, sel);
      printf ("\n");
      p = p->next;
    }
  }
  printf ("        ===============================\n");
*****/
}



// printSelectorToOffset ()
//
template <class Key, class Value>
void Mapping<Key, Value> :: printSelectorToOffset () {
  int i;
  Bucket<Key, Value> * p;
  String * sel;
  Offset * off;
  Key * key;
  Value * value;
  printf ("        SELECTOR-TO-OFFSET:\n");
  // Print using the insertion order...
  for (p=firstInsertedBucket; p; p = p->nextForIterator) {
    sel = (String *) p->key;
    off = (Offset *) p->value;
    printf ("          ");
    printString (stdout, sel);
    printf (": %d\n", off->ivalue);
  }
/*****
  for (i=0; i<sizeOfArray; i++) {
    p = array [i];
    while (p != NULL) {
      sel = (String *) p->key;
      off = (Offset *) p->value;
      printf ("          ");
      printString (stdout, sel);
      printf (": %d\n", off->ivalue);
      p = p->next;
    }
  }
  printf ("        ===============================\n");
*****/
}



// getFirst () --> Value
//
// This routine and getNext() are used to iterate through all the elements
// in the top scope only.  To use, call getFirst(), then call getNext()
// repeatedly.  When there are no more, these routines will return NULL.
//
template <class Key, class Value>
Value * Mapping <Key, Value> :: getFirst () {
  iteratorLastBucket = firstInsertedBucket;
  if (iteratorLastBucket == NULL) return NULL;
  return iteratorLastBucket->value;                // Possibly NULL
}

/*****  old version  *****
template <class Key, class Value>
Value * Mapping <Key, Value> :: getFirst () {
  iteratorLastIndex = 0;
  for (; iteratorLastIndex<sizeOfArray; iteratorLastIndex ++) {
    iteratorLastBucket = array [iteratorLastIndex];
    if (iteratorLastBucket) return iteratorLastBucket->value;
  }
  return NULL;
}
********************/



// getNext () --> Value
//
// This routine and getFirst() are used to iterate through all the elements
// in the top scope only.  To use, call getFirst(), then call getNext()
// repeatedly.  When there are no more, these routines will return NULL.
//
template <class Key, class Value>
Value * Mapping <Key, Value> :: getNext () {
  if (iteratorLastBucket == NULL) {
    programLogicError ("Mapping getNext called after end of list already reached");
  }
  iteratorLastBucket = iteratorLastBucket->nextForIterator;
  if (iteratorLastBucket == NULL) {
    return NULL;
  }
  return iteratorLastBucket->value;
}

/*****  old version  *****
template <class Key, class Value>
Value * Mapping <Key, Value> :: getNext () {
  Bucket<Key, Value> * p;
  iteratorLastBucket = iteratorLastBucket->next;
  if (iteratorLastBucket) return iteratorLastBucket->value; 
  iteratorLastIndex ++;
  for (; iteratorLastIndex < sizeOfArray; iteratorLastIndex ++) {
    iteratorLastBucket = array [iteratorLastIndex];
    if (iteratorLastBucket) return iteratorLastBucket->value;
  }
  return NULL;
}
********************/


// getItsKey () --> Key
//
// This routine should only be called after getFirst() or getNext() has
// returned a non-NULL value.  This routine returns its key.
//
template <class Key, class Value>
Key * Mapping <Key, Value> :: getItsKey () {
  if (iteratorLastBucket == NULL) {
    programLogicError ("Mapping getItsKey called but last was NULL");
  }
  return iteratorLastBucket->key;
}



// Constructor (initialSize, superMap)
//
// The initialSize may be zero.  The pointer to a super Mapping may be
// NULL.
//
template <class Key, class Value>
Mapping<Key, Value> :: Mapping (int initSize, Mapping * supMap) {
  int i;
  superMap = supMap;
  numberOfElements = 0;
  sizeOfArray = initSize;
  array = NULL;
  iteratorLastBucket = NULL;
  firstInsertedBucket = NULL;
  lastInsertedBucket = NULL;
  if (initSize < 0) {
    programLogicError ("Trying to create a mapping with negative size");
  }
  if (initSize == 0) return;
  array = (Bucket<Key, Value> * * ) new Bucket<Key, Value> * [initSize];
  for (i=0; i<sizeOfArray; i++) {
    array [i] = NULL;
  }
}



// Destructor ()
//
// Free the array and all the buckets.
//
template <class Key, class Value>
Mapping<Key, Value> :: ~Mapping () {
  int i;
  Bucket<Key, Value> * buck;
  Bucket<Key, Value> * nextBuck;
  
  for (i=0; i<sizeOfArray; i++) {
    buck = array[i];
    while (buck != NULL) {
      nextBuck = buck->next;
      delete buck;
      buck = nextBuck;
    }
  }
  delete array;
}



// nextLargerSize (oldSize) --> int
//
// Returns next larger size.
//
template <class Key, class Value>
int Mapping<Key, Value> :: nextLargerSize (int oldSize) {
  if (oldSize < 0) return 0;
  if (oldSize < 7) return 7;
  if (oldSize < 41) return 41;
  if (oldSize < 211) return 211;
  return 1021;
}



// rehashIfNecessary ()
//
// If the Mapping has reached 50% full (i.e., if the number of Buckets
// is half the size of the hash table), then re allocate a new array
// and move all Buckets into the new array.
//
template <class Key, class Value>
void Mapping<Key, Value> :: rehashIfNecessary () {
  int oldArraySize, i;
  Bucket<Key,Value> * * oldArray;
  Bucket<Key,Value> * p;
  Bucket<Key,Value> * nextP;

//  printf ("Rehash called: numberOfElements = %d sizeOfArray = %d\n",
//          numberOfElements, sizeOfArray);

  if (numberOfElements < sizeOfArray/2) return;
  if (sizeOfArray == nextLargerSize (sizeOfArray)) return;

  // printf ("== Rehashing: nextLargerSize (sizeOfArray) = %d\n",
  //         nextLargerSize (sizeOfArray));

  // printf ("Rehashing: Before...\n");
  // this->print (0);

  oldArraySize = sizeOfArray;
  oldArray = array;

  // Reinitialize this Mapping...
  sizeOfArray = nextLargerSize (oldArraySize);
  if (sizeOfArray <= oldArraySize) {
    fatalError ("In rehashIfNecessary: problems with nextLargerSize");
  }
  array = (Bucket<Key, Value> * * ) new Bucket<Key, Value> * [sizeOfArray];
  for (i=0; i<sizeOfArray; i++) {
    array [i] = NULL;
  }
  //////  numberOfElements = 0;

  // Go through the elements and add them back to this mapping.
  for (i=0; i<oldArraySize; i++) {
    p = oldArray [i];
    // printf ("Processing bucket list %d: 0x%08x:\n", i, p);
    while (p != NULL) {
      nextP = p->next;
      // String * str = (String *) p->key;
      // IntConst * val = (IntConst *) p->value;
      // printf ("  Moving   KEY=");
      // printString (stdout, str);
      // printf ("   VAL=%d\n", val->ivalue);
      int i2 = arrayIndex (p->key);
      p->next = array[i2];
      array[i2] = p;
      p = nextP;
    }
  }
  delete oldArray;
  // printf ("Rehashing: After...\n");
  // this->print (0);
}



// findInTopScope (key) --> value
//
// Return NULL if not found.  Look only in the top scope.
//
template <class Key, class Value>
Value * Mapping<Key, Value> :: findInTopScope (Key * k) {
  Bucket<Key,Value> * buck;
  if (numberOfElements == 0) {
    return NULL;
  }
  int i = arrayIndex (k);
  buck = array[i];
  while (buck != NULL) {
    if (buck->key == k) return buck->value;
    buck = buck->next;
  }
  return NULL;
}



// arrayIndex (key) --> int
//
// This routine is passed a pointer.  It uses this pointer as a
// hash value and returns an entry into the array.
// It returns an integer between 0 and (arraySize-1).
// We assume the caller has already checked that the mapping is not-empty.
//
template <class Key, class Value>
int Mapping<Key, Value> :: arrayIndex (Key * k) {
  if (sizeOfArray <= 0) {
    fatalError ("In arrayIndex: We should not call this routine for empty mappings!");
  }
  if (sizeOfArray <= 0) {
    printf ("sizeOfArray = %d\n", sizeOfArray);
    // fatalError ("In Mapping...sizeOfArray <= 0");
  }
  return (((int) k) >> 2) % sizeOfArray;
}



// testMapping ()
//
// This routine is used in testing the Mapping class.
//
void testMapping () {
  Mapping<String, Offset> * testMap2;
  Mapping<Offset, String> * testMap3;
  int i;
  Offset * off;

  String * s1 = lookupAndAdd ("string 1", ID);
  String * s2 = lookupAndAdd ("string 2", ID);
  String * s3 = lookupAndAdd ("string 3", ID);
  String * s4 = lookupAndAdd ("string 4", ID);
  String * s5 = lookupAndAdd ("string 5", ID);
  String * s6 = lookupAndAdd ("string 6", ID);
  String * s7 = lookupAndAdd ("string 7", ID);
  String * s8 = lookupAndAdd ("string 8", ID);
  String * s9 = lookupAndAdd ("string 9", ID);
  String * s10 = lookupAndAdd ("string 10", ID);
  String * s11 = lookupAndAdd ("string 11", ID);
  String * s12 = lookupAndAdd ("string 12", ID);
  String * s13 = lookupAndAdd ("string 13", ID);
  String * s14 = lookupAndAdd ("string 14", ID);
  String * s15 = lookupAndAdd ("string 15", ID);
  String * s16 = lookupAndAdd ("string 16", ID);
  String * s17 = lookupAndAdd ("string 17", ID);
  String * s18 = lookupAndAdd ("string 18", ID);
  String * s19 = lookupAndAdd ("string 19", ID);
  String * s20 = lookupAndAdd ("string 20", ID);

  IntConst * v1 = new IntConst (); v1->ivalue = 10001;
  IntConst * v2 = new IntConst (); v2->ivalue = 10002;
  IntConst * v3 = new IntConst (); v3->ivalue = 10003;
  IntConst * v4 = new IntConst (); v4->ivalue = 10004;
  IntConst * v5 = new IntConst (); v5->ivalue = 10005;
  IntConst * v6 = new IntConst (); v6->ivalue = 10006;
  IntConst * v7 = new IntConst (); v7->ivalue = 10007;
  IntConst * v8 = new IntConst (); v8->ivalue = 10008;
  IntConst * v9 = new IntConst (); v9->ivalue = 10009;
  IntConst * v10 = new IntConst (); v10->ivalue = 10010;
  IntConst * v11 = new IntConst (); v11->ivalue = 10011;
  IntConst * v12 = new IntConst (); v12->ivalue = 10012;
  IntConst * v13 = new IntConst (); v13->ivalue = 10013;
  IntConst * v14 = new IntConst (); v14->ivalue = 10014;
  IntConst * v15 = new IntConst (); v15->ivalue = 10015;
  IntConst * v16 = new IntConst (); v16->ivalue = 10016;
  IntConst * v17 = new IntConst (); v17->ivalue = 10017;
  IntConst * v18 = new IntConst (); v18->ivalue = 10018;
  IntConst * v19 = new IntConst (); v19->ivalue = 10019;
  IntConst * v20 = new IntConst (); v20->ivalue = 10020;

  testMap = (Mapping<String, AstNode> *)
      new Mapping<String, AstNode> (7, NULL);
  // Test printing for empty mappings...
  testMap->print (0);

  printf ("Testing iterator functions for empty mapping...\n");
  for (IntConst * vvv = (IntConst *) testMap->getFirst();
       vvv;
       vvv = (IntConst *) testMap->getNext()) {
    printf ("   KEY=");
    printString (stdout, testMap->getItsKey());
    printf ("   VAL=%d\n", vvv->ivalue);
  }
  printf ("Done testing iterator functions for empty mapping.\n");

  testMap->enter (s1,v1);
  testMap->enter (s2,v2);
  testMap->enter (s3,v3);

  testMap->print (0);

  testMap = (Mapping<String, AstNode> *)
      new Mapping<String, AstNode> (3, testMap);

  testMap->print (0);

  testMap->enter (s4,v4);
  testMap->enter (s5,v5);
  testMap->enter (s6,v6);
  testMap->enter (s7,v7);
  testMap->enter (s8,v8);
  testMap->enter (s9,v9);
  testMap->enter (s10,v10);
  testMap->enter (s11,v11);
  testMap->enter (s12,v12);
  testMap->enter (s13,v13);
  testMap->enter (s14,v14);
  testMap->enter (s15,v15);
  testMap->enter (s16,v16);
  testMap->enter (s17,v17);
  testMap->enter (s18,v18);
  testMap->enter (s19,v19);
  testMap->enter (s20,v20);

  testMap->print (0);

  searchFor (s1);
  searchFor (s2);
  searchFor (s3);
  searchFor (s4);
  searchFor (s5);
  searchFor (s6);
  searchFor (s7);
  searchFor (s8);
  searchFor (s9);

  printf ("Testing iterator functions...\n");
  for (v1 = (IntConst *) testMap->getFirst(); v1; v1 = (IntConst *) testMap->getNext()) {
    printf ("   KEY=");
    printString (stdout, testMap->getItsKey());
    printf ("   VAL=%d\n", v1->ivalue);
  }

  delete testMap;

  // Initialize the Dispatch Table offset list...
  if (firstDispatchOffset == NULL) {
    off = new Offset;
    off->ivalue = 4;        // This is the offset of the first message
    off->nextOffset = NULL;
    firstDispatchOffset = off;
  }

  testMap2 = (Mapping<String, Offset> *)
      new Mapping<String, Offset> (10, NULL);
  testMap2->printSelectorToOffset ();
  off = firstDispatchOffset;
  testMap2->enter (s1,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s2,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s3,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s4,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s5,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s6,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s7,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s8,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s9,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s10,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s11,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s12,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s13,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s14,off);
  off = nextDispatchOffset (off);
  testMap2->enter (s15,off);

  testMap2->printSelectorToOffset ();
  delete testMap2;

  testMap3 = (Mapping<Offset, String> *)
      new Mapping<Offset, String> (10, NULL);
  testMap3->printOffsetToSelector ("here is a title");
  off = firstDispatchOffset;
  testMap3->enter (off, s1);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s2);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s3);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s4);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s5);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s6);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s7);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s8);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s9);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s10);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s11);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s12);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s13);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s14);
  off = nextDispatchOffset (off);
  testMap3->enter (off, s15);

  testMap3->printOffsetToSelector ("here is a title");
  delete testMap3;

  exit (0);
}



// searchFor (str)
//
// This routine is used in testing the Mapping class.  It is passed a
// key and it calls several functions (such as "find" and "alreadyDefined")
// using this key and prints the results.
//
void searchFor (String * str) {
  IntConst * val;
  int i;
  printString (stdout, str);
  printf ("...\n");
  printf ("   find () =              ");
  val = (IntConst *) testMap->find (str);
  if (val == NULL) {
    printf ("   NULL RETURNED\n");
  } else {
    printf ("   KEY=");
    printString (stdout, str);
    printf ("   VAL=%d\n", val->ivalue);
  }
  printf ("   findInTopScope () =    ");
  val = (IntConst *) testMap->findInTopScope (str);
  if (val == NULL) {
    printf ("   NULL RETURNED\n");
  } else {
    printf ("   KEY=");
    printString (stdout, str);
    printf ("   VAL=%d\n", val->ivalue);
  }
  i = testMap->alreadyDefined (str);
  printf ("   alreadyDefined () = %d\n", i);
}
