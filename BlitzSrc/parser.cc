// parser.cc  --  Functions related to parsing
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



// nextTokenIsID (msg)
//
// This routine checks to see if the next token is an ID, and returns true
// or false.  If not, it also prints a message.  If an ID happens to occur
// in the next couple of tokens, it advances to it and returns true.
//
int nextTokenIsID (char * msg) {
  if (token.type == ID) {
    return 1;
  } else {
    syntaxError (msg);
    // See if the desired token happens to occur really soon;
    // If so, scan the extra tokens.
    if (token2.type == ID) {
      scan ();
      checkTokenSkipping (1);
      return 1;
    } else if (token3.type == ID) {
      scan ();
      scan ();
      checkTokenSkipping (2);
      return 1;
    } else {
      checkTokenSkipping (0);
      return 0;
    }
  }
}



// mustHaveID (msg)
//
// This routine checks to see if the next token is an ID.  If not, it
// prints the message.  In any case, it returns a valid String.  It
// may scan zero tokens, if there is an error.
//
String * mustHaveID (char * msg) {
  String * retVal;
  if (nextTokenIsID (msg)) {
    retVal = token.value.svalue;
    scan ();
    return retVal;
  } else {
    return lookupAndAdd ("<missingIdSyntaxError>", ID);
  }
}



// mustHave (tok, msg)
//
// The next token must be 'tok'.  If so, scan it.  If not, print a syntax
// error.
//
// THIS ROUTINE MAY NOT SCAN ANY TOKENS!!!  Therefore, the caller MUST ensure that
// the parser will scan at least one token in any loop calling this routine.
//
void mustHave (int tok, char * msg) {
  if (token.type == tok) {
    scan ();
  } else {
    syntaxError (msg);
    // See if the desired token happens to occur really soon;
    // If so, scan the extra tokens and get it.
    if (token2.type == tok) {
      scan ();
      scan ();
      checkTokenSkipping (1);
    } else if (token3.type == tok) {
      scan ();
      scan ();
      scan ();
      checkTokenSkipping (2);
    } else {
      checkTokenSkipping (0);
    }
  }
}



// mustHaveOrScanUntilInScanSet (tok, msg)  --> bool
//
// The next token must be 'tok'.  If so, scan it.  If not, print a syntax
// error and keep scanning until we get it or we hit EOF or something that
// is in the "scan set" of tokens.
//
// THIS ROUTINE MAY NOT SCAN ANY TOKENS!!!  (...if we are already positioned
// on a token that is in the "scan set".)  Therefore, the caller MUST ensure that
// the parser will scan at least one token in any loop calling this routine.
//
// This routine returns TRUE if we got the token we were searching for (even if
// there was an error and we had to scan some tokens) and FALSE if we stopped on
// some other token.
//
int mustHaveOrScanUntilInScanSet (int tok, char * msg) {
  int count;
  int retValue = 0;
  printf ("**********  OBSOLETE ROUTINE  **********\n");
  if (token.type == tok) {
    scan ();
    return 1;
  } else {
    syntaxError (msg);
    count = 0;
    while (1) {
      if (token.type == EOF) {
        break;
      } else if (token.type == tok) {
        scan ();
        count++;
        retValue = 1;
        break;
      } else if (inScanSet ()) {
        if (token.type == tok) {
          scan ();
          count++;
        }
        break;
      }
      scan ();
      count++;
    }
    checkTokenSkipping (count);
    return retValue;
  }
}



// scanToFollowType ()
//
// This routine scans until we hit a token that can follow a Type.  It may
// scan zero tokens.
//
void scanToFollowType () {
  int count = 0;
  while ((token.type != EOF) && !inFollowType ()) {
    scan ();
    count++;
  }
  checkTokenSkipping (count);
  return;
}



// inScanSet () --> bool
//
// Return true if the next token could follow a Statement or StmtList, or is one of
// the tokens listed in the first section of misc. tokens:
//   
int inScanSet () {
  switch (token.type) {
    // First section of token of misc. tokens...
    case RETURNS:
    case HEADER:
    case END_HEADER:
    case CODE:
    case END_CODE:
    case INTERFACE:
    case EXTENDS:
    case MESSAGES:
    case END_INTERFACE:
    case CLASS:
    case IMPLEMENTS:
    case SUPER_CLASS:
    case RENAMING:
    case FIELDS:
    case METHODS:
    case END_CLASS:
    case BEHAVIOR:
    case END_BEHAVIOR:
    case USES:
    case TO:
    case CONST:
    case ERRORS:
    case VAR:
    case TYPE:
    case ENUM:
    case FUNCTIONS:
//  case FUNCTION:          // listed below
//  case END_FUNCTION:      // listed below
    case INFIX:
    case PREFIX:
    case METHOD:
//  case END_METHOD:        // listed below
    case COLON:
    // These are from FIRST(TYPE)...
    case INT:
    case BOOL:
    case CHAR:
    case DOUBLE:
    case TYPE_OF_NULL:
    case ANY_TYPE:
    case VOID:
    case RECORD:
    case PTR:
    case ARRAY:
//  case FUNCTION:
//  case ID:
    // These are from FOLLOW(STMT)...
    case ELSE_IF:
    case ELSE:
    case END_IF:
    case END_WHILE:
    case END_FOR:
    case CASE:
    case DEFAULT:
    case END_SWITCH:
    case CATCH:
    case END_TRY:
    case END_FUNCTION:
    case END_METHOD:
    case SEMI_COLON:
    case R_PAREN:
    case UNTIL:
    // These are from FIRST(STMT)...
    case IF:
    case WHILE:
    case DO:
    case BREAK:
    case CONTINUE:
    case RETURN:
    case FOR:
    case SWITCH:
    case TRY:
    case THROW:
    // These are from FIRST(EXPR)...
    case TRUE:
    case FALSE:
    case NULL_KEYWORD:
    case SELF:
    case SUPER:
    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case FUNCTION:
    case ID:
    case NEW:
    case ALLOC:
    case FREE:
    case DEBUG:
    case SIZE_OF:
    case L_PAREN:
    case OPERATOR:
      return 1;
    default:
      return 0;
  }
}



// inFirstStmt () --> bool
//
// Return true if the next token could be the first token in a Statement or StmtList.
//
int inFirstStmt () {
  switch (token.type) {
    case IF:
    case WHILE:
    case DO:
    case BREAK:
    case CONTINUE:
    case RETURN:
    case FOR:
    case SWITCH:
    case TRY:
    case THROW:
    // These are from FIRST(EXPR)...
    case TRUE:
    case FALSE:
    case NULL_KEYWORD:
    case SELF:
    case SUPER:
    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case FUNCTION:
    case ID:
    case NEW:
    case ALLOC:
    case FREE:
    case DEBUG:
    case SIZE_OF:
    case L_PAREN:
    case OPERATOR:
      return 1;
    default:
      return 0;
  }
}



// inFollowStmt () --> bool
//
// Return true if the next token could follow a Statement or StmtList.
//
int inFollowStmt () {
  switch (token.type) {
    case ELSE_IF:
    case ELSE:
    case END_IF:
    case END_WHILE:
    case END_FOR:
    case CASE:
    case DEFAULT:
    case END_SWITCH:
    case CATCH:
    case END_TRY:
    case END_FUNCTION:
    case END_METHOD:
    case SEMI_COLON:
    case R_PAREN:
    case UNTIL:
    // These are from FIRST(STMT)...
    case IF:
    case WHILE:
    case DO:
    case BREAK:
    case CONTINUE:
    case RETURN:
    case FOR:
    case SWITCH:
    case TRY:
    case THROW:
    // These are from FIRST(EXPR)...
    case TRUE:
    case FALSE:
    case NULL_KEYWORD:
    case SELF:
    case SUPER:
    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case FUNCTION:
    case ID:
    case NEW:
    case ALLOC:
    case FREE:
    case DEBUG:
    case SIZE_OF:
    case L_PAREN:
    case OPERATOR:
      return 1;
    default:
      return 0;
  }
}



// inFirstExpr (tok) --> bool
//
// Return true if the given token could be the first token in an Expression.
//
int inFirstExpr (Token tok) {
  switch (tok.type) {
    case TRUE:
    case FALSE:
    case NULL_KEYWORD:
    case SELF:
    case SUPER:
    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case FUNCTION:
    case ID:
    case NEW:
    case ALLOC:
    case FREE:
    case DEBUG:
    case SIZE_OF:
    case L_PAREN:
    case OPERATOR:
      return 1;
    default:
      return 0;
  }
}



// inFollowExpr () --> bool
//
// Return true if the next token could follow an Expression.
//
int inFollowExpr () {
  switch (token.type) {
//  case R_PAREN:
    case COMMA:
    case EQUAL:
    case COLON:
    case PERIOD:
    case R_BRACE:
    case R_BRACK:
    case TO:
    case BY:
    case AS_PTR_TO:
    case AS_INTEGER:
    case ARRAY_SIZE:
    case IS_INSTANCE_OF:
    case IS_KIND_OF:
    case OF:
    case CONST:
    case ERRORS:
    case VAR:
    case ENUM:
    case TYPE:
    case FUNCTIONS:
//  case FUNCTION:      // listed below
    case INTERFACE:
    case CLASS:
    case END_HEADER:
    case END_CODE:
    case BEHAVIOR:
    // These are from FOLLOW(STMT)...
    case ELSE_IF:
    case ELSE:
    case END_IF:
    case END_WHILE:
    case END_FOR:
    case CASE:
    case DEFAULT:
    case END_SWITCH:
    case CATCH:
    case END_TRY:
    case END_FUNCTION:
    case END_METHOD:
    case SEMI_COLON:
    case R_PAREN:
    case UNTIL:
    // These are from FIRST(STMT)...
    case IF:
    case WHILE:
    case DO:
    case BREAK:
    case CONTINUE:
    case RETURN:
    case FOR:
    case SWITCH:
    case TRY:
    case THROW:
    // These are from FIRST(EXPR)...
    case TRUE:
    case FALSE:
    case NULL_KEYWORD:
    case SELF:
    case SUPER:
    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case FUNCTION:
    case ID:
    case NEW:
    case ALLOC:
    case FREE:
    case DEBUG:
    case SIZE_OF:
    case L_PAREN:
    case OPERATOR:
      return 1;
    default:
      return 0;
  }
}



// inFirstType (tok) --> bool
//
// Return true if the given token could be the first token in a Type.
//
int inFirstType (Token tok) {
  switch (tok.type) {
    case CHAR:
    case INT:
    case DOUBLE:
    case BOOL:
    case VOID:
    case TYPE_OF_NULL:
    case ANY_TYPE:
    case ID:
    case PTR:
    case RECORD:
    case ARRAY:
    case FUNCTION:
      return 1;
    default:
      return 0;
  }
}



// inFollowType () --> bool
//
// Return true if the next token could follow a Type.
//
// [I'm not 100% sure that this set is correct, but since it is only used for
// an intelligent recovery from syntax errors, it should be adequate.]
//
int inFollowType () {
  switch (token.type) {
//  case COMMA:
//  case R_BRACK:
//  case ID:
//  case VAR:
//  case END_FUNCTION:
//  case R_PAREN:
    case END_RECORD:
//  case EQUAL:
    case EXTERNAL:
    case END_CLASS:
    // These are from FOLLOW(EXPR)...
//  case R_PAREN:
    case COMMA:
    case EQUAL:
    case COLON:
    case PERIOD:
    case R_BRACE:
    case R_BRACK:
    case TO:
    case BY:
    case AS_PTR_TO:
    case AS_INTEGER:
    case ARRAY_SIZE:
    case IS_INSTANCE_OF:
    case IS_KIND_OF:
    case OF:
    case CONST:
    case ERRORS:
    case VAR:
    case ENUM:
    case TYPE:
    case FUNCTIONS:
//  case FUNCTION:      // listed below
    case INTERFACE:
    case CLASS:
    case END_HEADER:
    case END_CODE:
    case BEHAVIOR:
    // These are from FOLLOW(STMT)...
    case ELSE_IF:
    case ELSE:
    case END_IF:
    case END_WHILE:
    case END_FOR:
    case CASE:
    case DEFAULT:
    case END_SWITCH:
    case CATCH:
    case END_TRY:
    case END_FUNCTION:
    case END_METHOD:
    case SEMI_COLON:
    case R_PAREN:
    case UNTIL:
    // These are from FIRST(STMT)...
    case IF:
    case WHILE:
    case DO:
    case BREAK:
    case CONTINUE:
    case RETURN:
    case FOR:
    case SWITCH:
    case TRY:
    case THROW:
    // These are from FIRST(EXPR)...
    case TRUE:
    case FALSE:
    case NULL_KEYWORD:
    case SELF:
    case SUPER:
    case INT_CONST:
    case DOUBLE_CONST:
    case CHAR_CONST:
    case STRING_CONST:
    case FUNCTION:
    case ID:
    case NEW:
    case ALLOC:
    case FREE:
    case DEBUG:
    case SIZE_OF:
    case L_PAREN:
    case OPERATOR:
      return 1;
    default:
      return 0;
  }
}



// inHeaderSet () --> bool
//
// Return true if the next token could be something in a header file, or it is EOF.
//
int inHeaderSet () {
  switch (token.type) {
    case CONST:
    case ERRORS:
    case VAR:
    case ENUM:
    case TYPE:
    case FUNCTIONS:
    case INTERFACE:
    case CLASS:
    case END_HEADER:
    case EOF:
      return 1;
    default:
      return 0;
  }
}



// inCodeSet () --> bool
//
// Return true if the next token could be something in a code file, or it is EOF.
//
int inCodeSet () {
  switch (token.type) {
    case CONST:
    case ERRORS:
    case VAR:
    case ENUM:
    case TYPE:
    case FUNCTION:
    case INTERFACE:
    case CLASS:
    case BEHAVIOR:
    case END_CODE:
    case EOF:
      return 1;
    default:
      return 0;
  }
}



// appendStmtLists (stmtList1, stmtList2) --> stmtList
//
// This routine is passed 2 lists of statements.  It appends them and returns
// the result.  Either input list may be NULL.
//
Statement * appendStmtLists (Statement * stmtList1,
                           Statement * stmtList2) {
  Statement * p;

  if (stmtList1 == NULL) return stmtList2;
  if (stmtList2 == NULL) return stmtList1;
  for (p = stmtList1; p->next; p = p->next) {
  }
  p->next = stmtList2;
  return stmtList1;

}



// parseStmtList (enclosingStmtForBreak,
//                enclosingStmtForContinue,
//                enclosingMethOrFunction,
//                endsWithSemiColon)
//
// This routine parses zero or more consequtive statements and returns
// a pointer to a statement list, possibly NULL.  This routine may scan
// ZERO tokens.
//
// Normally statements are not followed by semi-colons, but it one case they
// are.  (E.g., 'FOR (stmtList; expr; stmtList) stmtList END_FOR')  It is a common
// mistake (from C/Java) to terminate a statement with a semi-colon.  The
// 'endsWithSemiColon' is true only in the 'FOR' case; it is used to print an
// appropriate error and continue otherwise.
//
Statement * parseStmtList (Statement * enclosingStmtForBreak,
                           Statement * enclosingStmtForContinue,
                           MethOrFunction * enclosingMethOrFunction,
                           int endsWithSemiColon) {
  Statement *firstStmt, *lastStmt, *nextStmt;
  firstStmt = NULL;
  lastStmt = NULL;
  int count2;
  int count = 0;
      
  while (1) {
    count++;
    if (count > 100) {
      programLogicError ("Looping in parseStmtList");
    }
    if (token.type == EOF) {
      syntaxError ("Expecting a statement here - EOF encountered");
      return firstStmt;
    } else if (inFirstStmt ()) {
      nextStmt = parseStmt (enclosingStmtForBreak,
                            enclosingStmtForContinue,
                            enclosingMethOrFunction);
      if (token.type == SEMI_COLON && !endsWithSemiColon) {
        syntaxError ("Unexpected SEMI_COLON; statements are not terminated with ';' in this language");
        scan ();
      }
      if (nextStmt != NULL) {
        count = 0;
        if (firstStmt == NULL) {
          firstStmt = nextStmt;
        }
        if (lastStmt != NULL) {
          lastStmt->next = nextStmt;
        }
        lastStmt = nextStmt;
        while (lastStmt->next != NULL) {
          lastStmt = lastStmt->next;
        }
      }
    } else if (inFollowStmt ()) {
      return firstStmt;
    } else {
      syntaxError ("Expecting a statement here");
      count2 = 0;
      while (1) {
        if (inFollowStmt ()) break;
        if (token.type == EOF) break;
        scan ();
        count2++;
      }
      checkTokenSkipping (count2);
    }
  }
}



// parseStmt (enclosingStmtForBreak, enclosingStmtForContinue, enclosingMethOrFunction)
//
// This routine parses a single statement and returns a pointer to it.
// It is assumed that we know that the next token is really the first token
// of a statement.  If it runs into problems, it may return NULL.
//
// The 'enclosingStmtForBreak' points to the innermost FOR, WHILE, DO,
// or SWITCH statement, containing the statement we are parsing.  The
// 'enclosingStmtForContinue' points to the innermost FOR, WHILE, or DO
// statement, containing the statement we are parsing.  The purpose of these
// is to allow us to associate a BREAK or CONTINUE statement with the
// statement it refers to.
//
// The 'enclosingMethOrFunction' points to the lexically surrounding function or
// method.  This is needed when we parse a RETURN statement: we need to know whether
// or not to pick up an expression.
//
Statement * parseStmt (Statement * enclosingStmtForBreak,
                       Statement * enclosingStmtForContinue,
                       MethOrFunction * enclosingMethOrFunction) {
  IfStmt * firstIfStmt, * lastIfStmt, * nextIfStmt;
  WhileStmt * whileStmt;
  DoStmt * doStmt;
  BreakStmt * breakStmt;
  ContinueStmt * continueStmt;
  ReturnStmt * returnStmt;
  ForStmt * forStmt;
  SwitchStmt * switchStmt;
  Case * cas, * lastCase;
  TryStmt * tryStmt;
  Catch * cat, * lastCatch;
  ThrowStmt * throwStmt;
  FreeStmt * freeStmt;
  DebugStmt * debugStmt;
  Function * fun;
  Method * meth;
  CallStmt * callStmt;
  SendStmt * sendStmt;
  AssignStmt * assignStmt;
  Expression * expr1, * expr2;
  Statement * initStmts, * incrStmts, * bodyStmts;

  if (token.type == IF) {
    firstIfStmt = new IfStmt ();
    scan ();
    firstIfStmt->expr = parseExpr ("Expecting a boolean expression after 'if'");
    firstIfStmt->thenStmts = parseStmtList (enclosingStmtForBreak,
                                            enclosingStmtForContinue,
                                            enclosingMethOrFunction,
                                            0);
    nextIfStmt = lastIfStmt = firstIfStmt;
    while (token.type == ELSE_IF) {
        nextIfStmt = new IfStmt ();   
        scan (); 
        nextIfStmt->expr = parseExpr ("Expecting a boolean expression after 'elseIf'");
        nextIfStmt->thenStmts = parseStmtList (enclosingStmtForBreak,
                                               enclosingStmtForContinue,
                                               enclosingMethOrFunction,
                                               0);
        lastIfStmt->elseStmts = nextIfStmt;
        lastIfStmt = nextIfStmt;
    }
    if (token.type == ELSE) {
      scan ();
      lastIfStmt->elseStmts = parseStmtList (enclosingStmtForBreak,
                                             enclosingStmtForContinue,
                                             enclosingMethOrFunction,
                                             0);
    }
    mustHave (END_IF, "Expecting 'endIf'");
    return firstIfStmt;

  } else if (token.type == WHILE) {
    whileStmt = new WhileStmt ();
    scan ();
    whileStmt->expr = parseExpr ("Expecting a boolean expression after 'while'");
    whileStmt->stmts = parseStmtList (whileStmt, whileStmt, enclosingMethOrFunction, 0);
    mustHave (END_WHILE, "Expecting 'endWhile'");
    return whileStmt;

  } else if (token.type == DO) {
    doStmt = new DoStmt ();
    scan ();
    doStmt->stmts = parseStmtList (doStmt, doStmt, enclosingMethOrFunction, 0);
    mustHave (UNTIL, "Expecting 'until' in 'do StmtList until Expr'");
    doStmt->expr = parseExpr ("Expecting a boolean expression after 'until'");
    return doStmt;

  } else if ((token.type == FOR) && (token2.type == L_PAREN)) {
    whileStmt = new WhileStmt ();
    scan ();   // FOR
    scan ();   // '('
    initStmts = parseStmtList (whileStmt,
                               whileStmt,
                               enclosingMethOrFunction,
                               1);    // endsWithSemiColon = 1
    mustHave (SEMI_COLON, "Expecting first ';' in 'for ( initStmts ; expr ; incrStmts ) bodyStmts endFor'");
    if (token.type == SEMI_COLON) {
      whileStmt->expr = new BoolConst (1);
    } else {
      whileStmt->expr = parseExpr ("Expecting expr in 'for ( initStmts ; expr ; incrStmts ) bodyStmts endFor'");
    }
    mustHave (SEMI_COLON, "Expecting second ';' in 'for ( initStmts ; expr ; incrStmts ) bodyStmts endFor'");
    incrStmts = parseStmtList (whileStmt,
                               whileStmt,
                               enclosingMethOrFunction,
                               1);    // endsWithSemiColon = 1
    mustHave (R_PAREN, "Expecting ')' in 'for ( initStmts ; expr ; incrStmts ) bodyStmts endFor'");
    bodyStmts = parseStmtList (whileStmt,
                               whileStmt,
                               enclosingMethOrFunction,
                               0);
    mustHave (END_FOR, "Expecting in 'for ( initStmts ; expr ; incrStmts ) bodyStmts endFor'");
    whileStmt->stmts = appendStmtLists (bodyStmts, incrStmts);
    return appendStmtLists (initStmts, whileStmt);

/***
    forStmt = new ForStmt ();
    scan ();
    forStmt->lvalue = parseExpr ("Expecting an L-Value after 'for'");
    mustHave (EQUAL, "Expecting '=' in 'for lvalue = Expr to Expr...'");
    forStmt->expr1= parseExpr ("Expecting Expr1 in 'for lvalue = Expr1 to Expr2...");
    mustHave (TO, "Expecting 'to' in 'for lvalue = Expr to Expr...'");
    forStmt->expr2 = parseExpr ("Expecting Expr2 in 'for lvalue = Expr1 to Expr2...");
    if (token.type == BY) {
      scan ();
      forStmt->expr3 = parseExpr ("Expecting Expr3 in 'for lvalue = Expr1 to Expr2 by Expr3");
    }
    forStmt->stmts = parseStmtList (forStmt, forStmt, enclosingMethOrFunction, 0);
    mustHave (END_FOR, "Expecting 'endFor'");
    return forStmt;
***/

  } else if (token.type == FOR) {
    forStmt = new ForStmt ();
    scan ();
    forStmt->lvalue = parseExpr ("Expecting an L-Value after 'for'");
    mustHave (EQUAL, "Expecting '=' in 'for lvalue = Expr to Expr...'");
    forStmt->expr1= parseExpr ("Expecting Expr1 in 'for lvalue = Expr1 to Expr2...");
    mustHave (TO, "Expecting 'to' in 'for lvalue = Expr to Expr...'");
    forStmt->expr2 = parseExpr ("Expecting Expr2 in 'for lvalue = Expr1 to Expr2...");
    if (token.type == BY) {
      scan ();
      forStmt->expr3 = parseExpr ("Expecting Expr3 in 'for lvalue = Expr1 to Expr2 by Expr3");
    }
    forStmt->stmts = parseStmtList (forStmt, forStmt, enclosingMethOrFunction, 0);
    mustHave (END_FOR, "Expecting 'endFor'");
    return forStmt;

  } else if (token.type == SWITCH) {
    switchStmt = new SwitchStmt ();
    scan ();
    switchStmt->expr = parseExpr ("Expecting an expression after 'switch'");
    lastCase = NULL;
    while (token.type == CASE) {
      cas = new Case ();
      scan ();
      if (lastCase == NULL) {
        switchStmt->caseList = cas;
      } else {
        lastCase->next = cas;
      }
      lastCase = cas;
      cas->expr = parseExpr ("Expecting an expression after 'case'");
      mustHave (COLON, "Expecting ':' in 'case Expr: ...'");
      cas->stmts = parseStmtList (switchStmt,
                                  enclosingStmtForContinue,
                                  enclosingMethOrFunction,
                                  0);
    }
    if (token.type == DEFAULT) {
      scan ();
      switchStmt->defaultIncluded = 1;
      mustHave (COLON, "Expecting ':' after 'default'");
      switchStmt->defaultStmts = parseStmtList (switchStmt,
                                                enclosingStmtForContinue,
                                                enclosingMethOrFunction,
                                                0);
    }
    mustHave (END_SWITCH, "Expecting 'endSwitch'");
    return switchStmt;

  } else if (token.type == TRY) {
    tryStmt = new TryStmt ();
    scan ();
    if (enclosingMethOrFunction == NULL) {
      programLogicError ("enclosingMethOrFunction is NULL for TRY stmt");
    }
    enclosingMethOrFunction->containsTry = 1;
    tryStmt->stmts = parseStmtList (enclosingStmtForBreak,
                                    enclosingStmtForContinue,
                                    enclosingMethOrFunction,
                                    0);
    lastCatch = NULL;
    while (token.type == CATCH) {
      scan ();    // CATCH
      cat = new Catch ();
      if (lastCatch == NULL) {
        tryStmt->catchList = cat;
      } else {
        lastCatch->next = cat;
      }
      lastCatch = cat;
      if (token.type == ID) {
        cat->id = token.value.svalue;
        scan ();
      } else {
        syntaxError ("Expecting ID after 'catch'");
        cat->id = lookupAndAdd ("<missingIdSyntaxError>", ID);
      }
      cat->parmList = parseParmList ();
      mustHave (COLON, "Expecting ':' in 'catch ID (...) : Stmts'");
      cat->stmts = parseStmtList (enclosingStmtForBreak,
                                  enclosingStmtForContinue,
                                  enclosingMethOrFunction,
                                  0);
      cat->enclosingMethOrFunction = enclosingMethOrFunction;
      cat->nextInMethOrFunction = enclosingMethOrFunction->catchList;
      enclosingMethOrFunction->catchList = cat;
    }
    mustHave (END_TRY, "Expecting 'endTry'");
    return tryStmt;

  } else if (token.type == BREAK) {
    breakStmt = new BreakStmt ();
    if (enclosingStmtForBreak == NULL) {
      syntaxError ("This BREAK is not within a WHILE, DO, FOR, or SWITCH statement");
      breakStmt = NULL;
    } else {
      breakStmt->enclosingStmt = enclosingStmtForBreak;
      // Set "containsAnyBreaks" in WHILE, FOR, or DO.
      switch (enclosingStmtForBreak->op) {
        case WHILE_STMT:
          whileStmt = (WhileStmt *) (breakStmt->enclosingStmt);
          whileStmt->containsAnyBreaks = 1;
          break;
        case FOR_STMT:
          forStmt = (ForStmt *) (breakStmt->enclosingStmt);
          forStmt->containsAnyBreaks = 1;
          break;
        case DO_STMT:
          doStmt = (DoStmt *) (breakStmt->enclosingStmt);
          doStmt->containsAnyBreaks = 1;
          break;
        case SWITCH_STMT:
          switchStmt = (SwitchStmt *) (breakStmt->enclosingStmt);
          switchStmt->containsAnyBreaks = 1;
          break;
        default:
          printf ("breakStmt->enclosingStmt->op = %s\n",
                  symbolName (breakStmt->enclosingStmt->op));
          programLogicError ("Unkown op in fallsThru, breakStmt");
      }
    }
    scan ();  // BREAK
    return breakStmt;
  } else if (token.type == CONTINUE) {
    continueStmt = new ContinueStmt ();
    continueStmt->enclosingStmt = enclosingStmtForContinue;
    if (enclosingStmtForContinue == NULL) {
      syntaxError ("This CONTINUE is not within a WHILE, DO, or FOR statement");
      continueStmt = NULL;
    } else {
      // Set "containsAnyContinues" in WHILE, FOR, or DO
      switch (enclosingStmtForContinue->op) {
        case WHILE_STMT:
          whileStmt = (WhileStmt *) (continueStmt->enclosingStmt);
          whileStmt->containsAnyContinues = 1;
          break;
        case FOR_STMT:
          forStmt = (ForStmt *) (continueStmt->enclosingStmt);
          forStmt->containsAnyContinues = 1;
          break;
        case DO_STMT:
          doStmt = (DoStmt *) (continueStmt->enclosingStmt);
          doStmt->containsAnyContinues = 1;
          break;
        default:
          printf ("enclosingStmtForContinue->op = %s\n",
                  symbolName (enclosingStmtForContinue->op));
          programLogicError ("Unkown op in fallsThru, continue stmt");
      }
    }
    scan ();  // CONTINUE
    return continueStmt;
  } else if (token.type == RETURN) {
    returnStmt = new ReturnStmt ();
    scan ();
    if (enclosingMethOrFunction == NULL) {
      programLogicError ("EnclosingMethOrFunction is NULL");
    }
    if (! gotVoidType (enclosingMethOrFunction->retType)) {
      returnStmt->expr = parseExpr ("Expecting a return-value after 'return'");
    }
    returnStmt->enclosingMethOrFunction = enclosingMethOrFunction;
    return returnStmt;
  } else if (token.type == THROW) {
    scan ();    // THROW
    throwStmt = new ThrowStmt ();
    throwStmt->id = mustHaveID ("Expecting ID after 'throw'");
    if (token.type == L_PAREN) {
      scan ();   // L_PAREN
      throwStmt->argList = parseArgList ();
    } else {
      syntaxError ("Expecting '( arglist )' after ID");
    }
    return throwStmt;
  } else if (token.type == FREE) {
      freeStmt = new FreeStmt ();
      scan ();     // FREE
      freeStmt->expr = parseExpr ("Expecting an expression after 'free'");
      return freeStmt;
  } else if (token.type == DEBUG) {
      debugStmt = new DebugStmt ();
      scan ();     // DEBUG
      return debugStmt;
  } else if (inFirstExpr (token)) {
    expr1 = parseExpr ("In assignment, call, or send statement");
    if (token.type == EQUAL) {
      scan ();
      expr2 = parseExpr ("In expression after '=' in assignment statement");
    } else {
      expr2 = NULL;
    }
    // printf ("expr1 = ");  pretty (expr1);
    // printf ("expr2 = ");  pretty (expr2);
    // printf ("\n");
    if (expr2 != NULL) {
      assignStmt = new AssignStmt ();
      assignStmt->positionAt (expr1);
      assignStmt->lvalue = expr1;
      assignStmt->expr = expr2;
      return assignStmt;
    } else if (expr1->op == CALL_EXPR) {
      callStmt = new CallStmt ();
      callStmt->positionAt (expr1);
      callStmt->expr = (CallExpr *) expr1;
      return callStmt;
    } else if ((expr1->op == SEND_EXPR) &&
               ((((SendExpr *) expr1)->kind == KEYWORD) ||
                (((SendExpr *) expr1)->kind == NORMAL ))) {
      sendStmt = new SendStmt ();
      sendStmt->positionAt (expr1);
      sendStmt->expr = (SendExpr *) expr1;
      return sendStmt;
    } else {
      syntaxErrorWithToken (expr1->tokn,
                 "Unexpected expression when expecting a statement");
      return NULL;
    }
  } else {
    return NULL;
  }
}



// gotVoidType (type)
//
// Returns true iff this type is 'void'.
//
int gotVoidType (Type * type) {
  if (type == NULL) {
    programLogicError ("Type is NULL within gotVoidType()");
  }
  return (type->op == VOID_TYPE);
}



// parseParmList ()
//
// This routine parses a ParmList and returns a pointer to linked list of
// Parameter nodes (or NULL).
//   ParmList --> '(' ')'
//            --> '(' Decl { , Decl } ')'
//   Decl     --> ID { , ID } : Type
//
// If syntax errors, this routine may scan ZERO tokens.
//
Parameter * parseParmList () {
  Parameter * first, * last, * idListFirst, * idListLast, * parm;
  Type * type;
  first = last = NULL;
  if (token.type != L_PAREN) {
    syntaxError ("Expecting '(' in parameter list");
    return NULL;
  }
  scan ();    // L_PAREN
  while (token.type == ID) {
    // Pick up a list of ID's
    idListFirst = idListLast = NULL;
    while (token.type == ID) {
      parm = new Parameter ();
      parm->id = token.value.svalue;
      scan ();    // ID
      if (idListFirst == NULL) {
        idListFirst = parm;
      } else {
        idListLast->next = parm;
      }
      idListLast = parm;
      if (token.type != COMMA) {
        break;
      }
      scan ();     // COMMA
    }
    // Pick up a type
    mustHave (COLON, "Expecting ':' in 'parameterID: Type'");
    type = parseType ("Expecting a type after ':' in 'parameterID: Type'");
    // Now run through the ID's.  Add in the type and add to the growing parm list.
    while (idListFirst) {
      idListFirst->type = type;
      if (first == NULL) {
        first = idListFirst;
      } else {
        last->next = idListFirst;
      }
      last = idListFirst;
      idListFirst = (Parameter *) idListFirst->next;
    }
    last->next = NULL;
    // Check for COMMA or R_PAREN
    if (token.type == COMMA) {
      scan ();
    } else if (token.type == R_PAREN) {
      break;
    } else if (token.type == RETURNS) {
      syntaxError ("Expecting ')' to terminate parameter list");
      return first;
    }
  }
  mustHave (R_PAREN, "Expecting next ID or ')' in parameter list");
  return first;
}



// parseFunction (expectingID)
//
// This routine parses a Function and returns a pointer to a Function node.
//
Function * parseFunction (int expectingID) {
  Function * fun;
  fun = new Function ();
  if (token.type != FUNCTION) {
    programLogicError ("Already checked for 'function' keyword");
  }
  scan ();  // FUNCTION
  if (expectingID) {    // Normal function defn.
    if (token.type == ID) {
      fun->tokn = token;
      fun->id = token.value.svalue;
      scan ();    // ID
    } else {
      syntaxError ("Expecting ID after 'function'");
      fun->id = lookupAndAdd ("<missingIdSyntaxError>", ID);
    }
  } else {              // Closure
    if (token.type == ID) {
      syntaxError ("Not expecting ID after 'function' in closure");
      scan ();
    }
  }
  fun->parmList = parseParmList ();
  if (token.type == RETURNS) {
    scan ();
    fun->retType = parseType ("Expecting return type after RETURNS");
    if (gotVoidType (fun->retType)) {
      syntaxError ("Do not say 'returns void'; just leave it out");
    }
  } else if ((token.type == RETURN) &&
             (token2.type != END_FUNCTION)) {
    syntaxError ("Expecting RETURNS, not RETURN");
    scan ();
    fun->retType = parseType ("Expecting return type after RETURNS");
  } else {
    fun->retType = new VoidType ();
  }
  fun->locals = parseLocalVarDecls ();
  fun->stmts = parseStmtList (NULL,     // enclosingStmtForBreak
                              NULL,     // enclosingStmtForContinue
                              fun,      // enclosingMethOrFunction
                              0);
  mustHave (END_FUNCTION, "Expecting 'endFunction'");
  return fun;
}



// parseFunctionProtos ()
//
// This routine parses this syntax rule:
//    FunctionProtos --> functions { [ external ] FunProto }+
//    FunProto       --> ID ParmList [ returns Type ]
//
FunctionProto * parseFunctionProtos () {
  FunctionProto * first, * last, * funProto;
  first = last = NULL;
  if (token.type != FUNCTIONS) {
    programLogicError ("Already checked for 'functions' keyword");
  }
  scan ();   // FUNCTIONS
  while ((token.type == ID) || (token.type == EXTERNAL)) {
    funProto = new FunctionProto ();
    if (token.type == EXTERNAL) {
      scan ();
      funProto->isExternal = 1;
    }
    if (token.type == ID) {
      funProto->tokn = token;
      funProto->id = token.value.svalue;
      scan ();    // ID
    } else {
      syntaxError ("Expecting ID after 'external'");
      funProto->id = lookupAndAdd ("<missingIdSyntaxError>", ID);
    }
    funProto->parmList = parseParmList ();
    if (token.type == RETURNS) {
      scan ();
      funProto->retType = parseType ("Expecting return type after RETURNS");
      if (gotVoidType (funProto->retType)) {
        syntaxError ("Do not say 'returns void'; just leave it out");
      }
    } else if (token.type == RETURN) {
      syntaxError ("Expecting RETURNS, not RETURN");
      scan ();
      funProto->retType = parseType ("Expecting return type after RETURNS");
    } else {
      funProto->retType = new VoidType ();
    }
    if (first == NULL) {
      first = funProto;
    } else {
      last->next = funProto;
    }
    last = funProto;
  }
  return first;
}



// parseMethod ()
//
// This routine parses a Method and returns a pointer to a Method node.
//
Method * parseMethod () {
  Method * meth;
  MethodProto * methodProto;

  if (token.type != METHOD) {
    programLogicError ("Already checked for 'method' keyword");
  }
  scan ();   // METHOD
  meth = new Method ();
  methodProto = parseMethodProto ();
  meth->positionAt (methodProto);
  meth->kind = methodProto->kind;
  meth->selector = methodProto->selector;
  meth->parmList = methodProto-> parmList;
  meth->retType = methodProto-> retType;
  if (token.type == METHOD) {
    syntaxError ("Expecting method body");
    return meth;
  }
  if (token.type == END_BEHAVIOR) {
    syntaxError ("Expecting method body");
    return meth;
  }
  meth->locals = parseLocalVarDecls ();
  meth->stmts = parseStmtList (NULL,     // enclosingStmtForBreak
                               NULL,     // enclosingStmtForContinue
                               meth,     // enclosingMethOrFunction
                               0);
  mustHave (END_METHOD, "Expecting 'endMethod'");
  return meth;
}



// parseMethodProto ()
//
// This routine parses this syntax rule:
//    MethProto  --> ID ParmList [ returns Type ]
//               --> infix OPERATOR '(' ID : Type ')' returns Type
//               --> prefix OPERATOR '(' ')' returns Type
//               --> { ID : '(' ID : Type ')' }+ [ returns Type ]
//
// It may scan ZERO tokens if syntax errors.

//
MethodProto * parseMethodProto () {
  MethodProto * methodProto;
  char * newSelector;
  Parameter * parm, * lastParm;


  // Parse a INFIX method prototype...
  if (token.type == INFIX) {
    scan ();
    methodProto = new MethodProto ();
    methodProto->kind = INFIX;
    if (token.type != OPERATOR) {
      syntaxError ("Expecting OPERATOR");
      methodProto->selector = lookupAndAdd ("<missingSelectorSyntaxError>", ID);
    } else {
      methodProto->selector = token.value.svalue;
      scan ();     // OPERATOR
    }
    methodProto->parmList = parseParmList ();
    if (methodProto->parmList == NULL) {
      syntaxError ("Expecting exactly one parameter in infix method prototype");
    } else if (methodProto->parmList->next != NULL) {
      syntaxError ("Expecting exactly one parameter in infix method prototype");
    }
    if (token.type == RETURNS) {
      scan ();
      methodProto->retType = parseType ("Expecting return type after RETURNS");
      if (gotVoidType (methodProto->retType)) {
        syntaxError ("Infix methods must return a value");
      }
    } else {
      syntaxError ("Expecting 'returns Type' for infix method prototype");
      methodProto->retType = new VoidType ();
    }

  // Parse a PREFIX method prototype...
  } else if (token.type == PREFIX) {
    scan ();
    methodProto = new MethodProto ();
    methodProto->kind = PREFIX;
    if (token.type != OPERATOR) {
      syntaxError ("Expecting OPERATOR");
      methodProto->selector = lookupAndAdd ("<missingSelectorSyntaxError>", ID);
    } else {
      methodProto->selector = lookupAndAdd (appendStrings (
                                "_prefix_", token.value.svalue->chars, ""), OPERATOR);
      scan ();     // OPERATOR
    }
    mustHave (L_PAREN, "Expecting '()' after 'prefix OP' in method prototype");
    mustHave (R_PAREN, "Expecting '()' after 'prefix OP' in method prototype");
    if (token.type == RETURNS) {
      scan ();
      methodProto->retType = parseType ("Expecting return type after RETURNS");
      if (gotVoidType (methodProto->retType)) {
        syntaxError ("Prefix methods must return a value");
      }
    } else {
      syntaxError ("Expecting 'returns Type' for prefix method prototype");
      methodProto->retType = new VoidType ();
    }

  // Parse a NORMAL method prototype...
  } else if ((token.type == ID) && (token2.type == L_PAREN)) {
    methodProto = new MethodProto ();
    methodProto->selector = token.value.svalue;
    methodProto->kind = NORMAL;
    scan ();     // ID
    methodProto->parmList = parseParmList ();
    if (token.type == RETURNS) {
      scan ();
      methodProto->retType = parseType ("Expecting return type after RETURNS");
      if (gotVoidType (methodProto->retType)) {
        syntaxError ("Do not say 'returns void'; just leave it out");
      }
    } else if ((token.type == RETURN) &&
               (token2.type != END_METHOD)) {
      syntaxError ("Expecting RETURNS, not RETURN");
      scan ();
      methodProto->retType = parseType ("Expecting return type after RETURNS");
    } else {
      methodProto->retType = new VoidType ();
    }

  // Parse a KEYWORD method prototype...
  } else if ((token.type == ID) && (token2.type == COLON)) {
    methodProto = new MethodProto ();
    methodProto->kind = KEYWORD;
    newSelector = appendStrings (token.value.svalue->chars, ":", "");
    scan ();      // ID
    scan ();      // COLON
    mustHave (L_PAREN, "Expecting '(ID: Type)' in keyword method prototype");
    parm = new Parameter ();
    parm->id =  mustHaveID ("Expecting ID in '(ID: Type)'");
    mustHave (COLON, "Expecting ':' in '(ID: Type)'");
    parm->type = parseType ("Expecting type after ':' in '(ID: Type)'");
    mustHave (R_PAREN, "Expecting ')' in '(ID: Type)'");
    methodProto->parmList = parm;
    lastParm = parm;
    // Each iteration of this loop parses "ID: (ID: Type)"...
    while (1) {
      if ((token.type != ID) || (token2.type != COLON)) {
        break;
      }
      newSelector = appendStrings (newSelector, token.value.svalue->chars, ":");
      scan ();      // ID
      scan ();      // COLON
      mustHave (L_PAREN, "Expecting '(ID: Type)' in keyword method prototype");
      parm = new Parameter ();
      parm->id =  mustHaveID ("Expecting ID in '(ID: Type)'");
      mustHave (COLON, "Expecting : in '(ID: Type)'");
      parm->type = parseType ("Expecting type after ':' in '(ID: Type)'");
      mustHave (R_PAREN, "Expecting ')' in '(ID: Type)'");
      lastParm->next = parm;
      lastParm = parm;
    }
    methodProto->selector = lookupAndAdd (newSelector, ID);
    // Parse "returns Type"...
    if (token.type == RETURNS) {
      scan ();
      methodProto->retType = parseType ("Expecting return type after RETURNS");
      if (gotVoidType (methodProto->retType)) {
        syntaxError ("Do not say 'returns void'; just leave it out");
      }
    } else if ((token.type == RETURN) &&
               (token2.type != END_METHOD)) {
      syntaxError ("Expecting RETURNS, not RETURN");
      scan ();
      methodProto->retType = parseType ("Expecting return type after RETURNS");
    } else {
      methodProto->retType = new VoidType ();
    }

  // Deal with syntax errors...
  } else if (token.type == ID) {
    methodProto = new MethodProto ();
    syntaxError ("Expecting ParmList or ': (ID:Type)' in Method Prototype");
    methodProto->selector = lookupAndAdd ("<missingSelectorSyntaxError>", ID);
    methodProto->kind = NORMAL;
    methodProto->retType = new VoidType ();
  } else {
    methodProto = new MethodProto ();
    syntaxError ("Expecting a Method Prototype");
    methodProto->selector = lookupAndAdd ("<missingSelectorSyntaxError>", ID);
    methodProto->kind = NORMAL;
    methodProto->retType = new VoidType ();
  }

  return methodProto;
}



// parseHeader ()
//
// This routine parses:
//    HeaderFile   --> header ID
//                       Uses
//                       { Const |
//                         Errors |
//                         VarDecls |
//                         Enum |
//                         TypeDef |
//                         FunctionProtos |
//                         Interface |
//                         Class }
//                     endHeader
//
Header * parseHeader () {
  Header * header;
  Global * newGlobals, * lastGlobal;
  ConstDecl * newConsts, * lastConst;
  ErrorDecl * newErrors, * lastError;
  TypeDef * typeDef, * lastTypeDef;
  FunctionProto * funProto, * lastFunProto;
  Interface * interface, * lastInterface;
  ClassDef * cl, * lastClass;
  int count;

  if (token.type != HEADER) {
    syntaxError ("Expecting 'header' at beginning of header file");
    return NULL;
  }
  scan ();    // HEADER
  header = new Header ();

  if (token.type == ID) {
    header->packageName = token.value.svalue;
    scan ();    // ID
  } else {
    syntaxError ("Expecting package name after 'header'");
    header->packageName = lookupAndAdd ("<missingPackageName>", ID);
  }
  header->uses = parseUses ();
  while (1) {
    switch (token.type) {
      case CONST:
        newConsts = parseConstDecls ();
        if (header->consts == NULL) {
          header->consts = newConsts;
        } else {
          lastConst = header->consts;
          while (lastConst->next != NULL) {
            lastConst = lastConst->next;
          }
          lastConst->next = newConsts;
        }
        break;
      case ERRORS:
        newErrors = parseErrorDecls ();
        if (header->errors == NULL) {
          header->errors = newErrors;
        } else {
          lastError = header->errors;
          while (lastError->next != NULL) {
            lastError = lastError->next;
          }
          lastError->next = newErrors;
        }
        break;
      case VAR:
        newGlobals = parseGlobalVarDecls ();
        if (header->globals == NULL) {
          header->globals = newGlobals;
        } else {
          lastGlobal = header->globals;
          while (lastGlobal->next != NULL) {
            lastGlobal = (Global *) lastGlobal->next;
          }
          lastGlobal->next = newGlobals;
        }
        break;
      case ENUM:
        newConsts = parseEnums ();
        if (header->consts == NULL) {
          header->consts = newConsts;
        } else {
          lastConst = header->consts;
          while (lastConst->next != NULL) {
            lastConst = lastConst->next;
          }
          lastConst->next = newConsts;
        }
        break;
      case TYPE:
        typeDef = parseTypeDefs ();
        if (header->typeDefs == NULL) {
          header->typeDefs = typeDef;
        } else {
          lastTypeDef = header->typeDefs;
          while (lastTypeDef->next != NULL) {
            lastTypeDef = lastTypeDef->next;
          }
          lastTypeDef->next = typeDef;
        }
        break;
      case FUNCTIONS:
        funProto = parseFunctionProtos ();
        if (header->functionProtos == NULL) {
          header->functionProtos = funProto;
        } else {
          lastFunProto = header->functionProtos;
          while (lastFunProto->next != NULL) {
            lastFunProto = lastFunProto->next;
          }
          lastFunProto->next = funProto;
        }
        break;
      case INTERFACE:
        interface = parseInterface ();
        if (header->interfaces == NULL) {
          header->interfaces = interface;
        } else {
          lastInterface = header->interfaces;
          while (lastInterface->next != NULL) {
            lastInterface = lastInterface->next;
          }
          lastInterface->next = interface;
        }
        break;
      case CLASS:
        cl = parseClass ();
        if (header->classes == NULL) {
          header->classes = cl;
        } else {
          lastClass = header->classes;
          while (lastClass->next != NULL) {
            lastClass = lastClass->next;
          }
          lastClass->next = cl;
        }
        break;
      case EOF:
        syntaxError ("Missing 'endHeader'");
        return header;
      case END_HEADER:
        scan ();
        if (token.type != EOF) {
          syntaxError ("There should be nothing more after 'endHeader'");
        }
        header->hashVal = hashVal;     // Based on token sequence up to now
        return header;
      default:
        if (token.type == BEHAVIOR) {
          syntaxError ("Behaviors must be placed in a CodeFile, not in a HeaderFile");
        } else {
          syntaxError ("Expecting CONST, ERRORS, VAR, ENUM, TYPE, FUNCTIONS, INTERFACE, CLASS, or END_HEADER in HeaderFile");
        }
        count = 0;
        while (1) {
          if (inHeaderSet ()) break;
          scan ();
          count++;
        }
        checkTokenSkipping (count);
    }
  }
}



// parseCode ()
//
// This routine parses:
//    CodeFile     --> code ID
//                       { Const |
//                         Errors |
//                         VarDecls |
//                         Enum |
//                         TypeDef |
//                         Function |
//                         Interface |
//                         Class |
//                         Behavior }
//                     endCode
//
Code * parseCode () {
  Code * code;
  Global * newGlobals, * lastGlobal;
  ErrorDecl * newErrors, * lastError;
  ConstDecl * newConsts, * lastConst;
  TypeDef * typeDef, * lastTypeDef;
  Function * fun, * lastFun;
  Interface * interface, * lastInterface;
  ClassDef * cl, * lastClass;
  Behavior * behavior, * lastBehavior;
  int count;

  if (token.type != CODE) {
    syntaxError ("Expecting 'code' at beginning of code file");
    return NULL;
  }
  scan ();    // CODE

  code = new Code ();

  if (token.type == ID) {
    code->packageName = token.value.svalue;
    scan ();    // ID
  } else {
    syntaxError ("Expecting package name after 'code'");
    code->packageName = lookupAndAdd ("<missingPackageName>", ID);
  }
  while (1) {
    switch (token.type) {
      case CONST:
        newConsts = parseConstDecls ();
        if (code->consts == NULL) {
          code->consts = newConsts;
        } else {
          lastConst = code->consts;
          while (lastConst->next != NULL) {
            lastConst = lastConst->next;
          }
          lastConst->next = newConsts;
        }
        break;
      case ERRORS:
        newErrors = parseErrorDecls ();
        if (code->errors == NULL) {
          code->errors = newErrors;
        } else {
          lastError = code->errors;
          while (lastError->next != NULL) {
            lastError = lastError->next;
          }
          lastError->next = newErrors;
        }
        break;
        break;
      case VAR:
        newGlobals = parseGlobalVarDecls ();
        if (code->globals == NULL) {
          code->globals = newGlobals;
        } else {
          lastGlobal = code->globals;
          while (lastGlobal->next != NULL) {
            lastGlobal = (Global *) lastGlobal->next;
          }
          lastGlobal->next = newGlobals;
        }
        break;
      case ENUM:
        newConsts = parseEnums ();
        if (code->consts == NULL) {
          code->consts = newConsts;
        } else {
          lastConst = code->consts;
          while (lastConst->next != NULL) {
            lastConst = lastConst->next;
          }
          lastConst->next = newConsts;
        }
        break;
      case TYPE:
        typeDef = parseTypeDefs ();
        if (code->typeDefs == NULL) {
          code->typeDefs = typeDef;
        } else {
          lastTypeDef = code->typeDefs;
          while (lastTypeDef->next != NULL) {
            lastTypeDef = lastTypeDef->next;
          }
          lastTypeDef->next = typeDef;
        }
        break;
      case FUNCTION:
        fun = parseFunction (1);      //  Expecting ID = 1
        if (code->functions == NULL) {
          code->functions = fun;
        } else {
          lastFun = code->functions;
          while (lastFun->next != NULL) {
            lastFun = lastFun->next;
          }
          lastFun->next = fun;
        }
        break;
      case INTERFACE:
        interface = parseInterface ();
        if (code->interfaces == NULL) {
          code->interfaces = interface;
        } else {
          lastInterface = code->interfaces;
          while (lastInterface->next != NULL) {
            lastInterface = lastInterface->next;
          }
          lastInterface->next = interface;
        }
        break;
      case CLASS:
        cl = parseClass ();
        if (code->classes == NULL) {
          code->classes = cl;
        } else {
          lastClass = code->classes;
          while (lastClass->next != NULL) {
            lastClass = lastClass->next;
          }
          lastClass->next = cl;
        }
        break;
      case BEHAVIOR:
        behavior = parseBehavior ();
        if (code->behaviors == NULL) {
          code->behaviors = behavior;
        } else {
          lastBehavior = code->behaviors;
          while (lastBehavior->next != NULL) {
            lastBehavior = lastBehavior->next;
          }
          lastBehavior->next = behavior;
        }
        break;
      case EOF:
        syntaxError ("Missing 'endCode'");
        return code;
      case END_CODE:
        scan ();
        if (token.type != EOF) {
          syntaxError ("There should be nothing more after 'endCode'");
        }
        code->hashVal = hashVal;     // Based on token sequence up to now
        return code;
      default:
        syntaxError ("Expecting CONST, ERRORS, VAR, ENUM, TYPE, FUNCTION, INTERFACE, CLASS, BEHAVIOR, or END_CODE in CodeFile");
        count = 0;
        while (1) {
          if (inCodeSet ()) break;
          scan ();
          count++;
        }
        checkTokenSkipping (count);
    }
  }
}



// parseBehavior ()
//
// This routine parses:
//    Behavior     --> behavior ID
//                       { Method }
//                     endBehavior
//
Behavior * parseBehavior () {
  Behavior * behavior;
  Method * meth, * lastMeth;
  int count;

  if (token.type != BEHAVIOR) {
    programLogicError ("Already checked for behavior keyword");
  }
  scan ();    // BEHAVIOR
  behavior = new Behavior ();
  behavior->id = mustHaveID ("Expecting name of class after 'behavior' keyword");
  while (1) {
    switch (token.type) {
      case METHOD:
        meth = parseMethod ();
        if (behavior->methods == NULL) {
          behavior->methods = meth;
        } else {
          lastMeth->next = meth;
        }
        lastMeth = meth;
        break;
      case EOF:
        syntaxError ("Missing 'endBehavior'");
        return behavior;
      case END_BEHAVIOR:
        scan ();
        return behavior;
      default:
        syntaxError ("Expecting METHOD, END_CODE, or END_BEHAVIOR");
        count = 0;
        while (1) {
          if (token.type == METHOD) break;
          if (token.type == END_CODE) break;
          if (token.type == END_BEHAVIOR) break;
          if (token.type == EOF) break;
          scan ();
          count++;
        }
        checkTokenSkipping (count);
        if (token.type == END_CODE) return behavior;
    }
  }
}



// parseUses ()
//
// This routine parses the following:
//
//    Uses         --> [ uses OtherPackage { , OtherPackage } ]
//
//    OtherPackage --> ID ID [ renaming Rename { , Rename } ]
//
Uses * parseUses () {
  Uses * first, * last, * uses;

  if (token.type != USES) {
    return NULL;
  }
  first = last = NULL;
  scan ();     // USES

  //  Each iteration of this loop picks up another OtherPackage...
  while (1) {
    uses = new Uses ();
    if ((token.type == ID) || (token.type == STRING_CONST)) {
      uses->id = token.value.svalue;
      scan ();    // ID or STRING_CONST
    } else {
      syntaxError ("Expecting next package name in 'uses' clause");
      uses->id = lookupAndAdd ("<missingIdSyntaxError>", ID);
    }
    if (first) {
      last->next = uses;
    } else {
      first = uses;
    }
    last = uses;
    uses->renamings = parseRenamings ();

    // See if we have more and scan the comma
    if (token.type == COMMA) {
      scan ();       // COMMA
    } else if (token.type == ID) {
      syntaxError ("Expecting COMMA before next package name");
    } else {
      break;
    }
  }
  return first;
}



// parseRenamings ()
//
// This routine parses the following:
//
//                 --> ID [ renaming Rename { , Rename } ]
//    
//    Rename       --> ID to ID
//
Renaming * parseRenamings () {
  Renaming * firstRenaming, * lastRenaming, * renaming;

  if (token.type != RENAMING) {
    return NULL;
  }
  scan ();    // RENAMING
  // Each iteration of this loop picks up another Renaming...
  firstRenaming = lastRenaming = NULL;
  while (1) {
    renaming = new Renaming ();
    if (token.type == ID) {
      renaming->from = token.value.svalue;
      scan ();    // from-ID
      mustHave (TO, "Expecting 'to' in Renaming 'xxx to yyy'");
      renaming->to = mustHaveID ("Expecting ID2 in Renaming 'ID1 to ID2'");
    } else {
      syntaxError ("Expecting 'ID' in next renaming 'xxx to yyy'");
      scan ();
      if (token.type == TO) {
        scan ();
        scan ();
      }
      renaming->from = lookupAndAdd ("<missingIdSyntaxError>", ID);
      renaming->to = lookupAndAdd ("<missingIdSyntaxError>", ID);
    }
    if (firstRenaming) {
      lastRenaming->next = renaming;
    } else {
      firstRenaming = renaming;
    }
    lastRenaming = renaming;
    // See if we have more renamings
    if ( (token.type == COMMA) &&
         ((token3.type == COLON) || (token3.type == TO)) ) {
      scan ();       // COMMA
    } else if ((token2.type == TO) || (token2.type == COLON)) {
      syntaxError ("Expecting COMMA before next renaming clause");
    } else {
      break;
    }
  }
  return firstRenaming;
}


// pickupKeywordSelector ()
//
// This routine parses
//     { ID : }+
// It builds and return a selector.
//
String * pickupKeywordSelector () {
  char * newSelector;

  if ((token.type != ID) || (token2.type != COLON)) {
    syntaxError ("Expecting a keyword selector");
    return lookupAndAdd ("<missingIdSyntaxError>", ID);
  }
  newSelector = appendStrings (token.value.svalue->chars, ":", "");
  scan ();      // ID
  scan ();      // COLON
  while ((token.type == ID) && (token2.type == COLON)) {
    newSelector = appendStrings (newSelector, token.value.svalue->chars, ":");
    scan ();      // ID
    scan ();      // COLON
  }
  return lookupAndAdd (newSelector, ID);
}



// colonCount (char *) --> int
//
// This routine returns the number of colons in the given string.
//
int colonCount (char * str) {
  int i = 0;
  while (*str != '\0') {
    if (*str == ':') {
      i++;
    }
    str++;
  }
  return i;
}



// parseInterface ()
//
// This routine parses the following:
//    Interface    --> interface ID [ TypeParms ]
//                       [ extends TypeInstanceList ]
//                       [ messages { MethProto }+ ]
//                     endInterface

//
Interface * parseInterface () {
  Interface * interface;
  MethodProto * methProto, * lastMethProto;
  int count;
  TypeArg * typeArg, * lastTypeArg;

  if (token.type != INTERFACE) {
    programLogicError ("Already checked for INTERFACE keyword");
  }
  scan ();   // INTERFACE
  interface = new Interface ();
  interface->id = mustHaveID ("Expecting interface name after 'interface'");
  // Pick up TypeParms...
  interface->typeParms = parseTypeParms ();
  // Pick up 'extends' clause...
  if (token.type == EXTENDS) {
    scan ();
    lastTypeArg = NULL;
    while (1) {
      typeArg = new TypeArg ();
      typeArg->type = parseNamedType ("Expecting an interface name after 'extends'");
      if (lastTypeArg == NULL) {
        interface->extends = typeArg;
      } else {
        lastTypeArg->next = typeArg;
      }
      lastTypeArg = typeArg;
      if (token.type != COMMA) {
        break;
      }
      scan ();
    }
  }
  // Pick up 'messages' clause...
  if (token.type == MESSAGES || token.type == METHODS) {
    if (token.type == METHODS) {
      syntaxError ("Expecting 'messages', not 'methods'");
    }
    scan ();
    lastMethProto = parseMethodProto ();
    interface->methodProtos = lastMethProto;
    while (1) {
      if ((token.type == INFIX) ||
          (token.type == PREFIX) ||
          ((token.type == ID) && (token2.type == L_PAREN)) ||
          ((token.type == ID) && (token2.type == COLON)) ) {
        methProto = parseMethodProto ();
        lastMethProto->next = methProto;
        lastMethProto = methProto;
      } else if (token.type == END_INTERFACE) {
        break;
      } else {
        syntaxError ("Expecting next message prototype or 'endInterface'");
        count = 0;
        while (1) {
          if (inHeaderSet ()) break;
          if (token.type == INFIX) break;
          if (token.type == PREFIX) break;
          if ((token.type == ID) && (token2.type == L_PAREN)) break;
          if ((token.type == ID) && (token2.type == COLON)) break;
          if (token.type == END_INTERFACE) break;
          scan ();
          count++;
        }
        checkTokenSkipping (count);
        if (inHeaderSet ()) break;
      }
    }
  }
  mustHave (END_INTERFACE, "Expecting endInterface");
  return interface;
}



// parseTypeParms ()
//
// This routine parses...
//                 --> [ TypeParms ]
//    TypeParms    --> '[' ID : Type { , ID : Type } ']'
//
// It returns a pointer to linked list of TypeParms (or NULL).
//
TypeParm * parseTypeParms () {
  TypeParm * newParm, * first, * last;

  if (token.type != L_BRACK) {
    return NULL;
  }
  scan ();    // L_BRACK
  newParm = new TypeParm ();
  newParm->id = mustHaveID ("Expecting 'ID: Type' after '['");
  mustHave (COLON, "Expecting ':' in 'ID: Type'");
  newParm->type = parseType ("Expecting type after ':' in 'ID: Type'");
  first = last = newParm;
  while (token.type == COMMA) {
    scan ();   // COMMA
    newParm = new TypeParm ();
    newParm->id = mustHaveID ("Expecting 'ID: Type' after '['");
    mustHave (COLON, "Expecting ':' in 'ID: Type'");
    newParm->type = parseType ("Expecting type after ':' in 'ID: Type'");
    last->next = newParm;
    last = newParm;
  }
  if (token.type == R_BRACK) {
    scan ();      // R_BRACK
  } else {
    syntaxError ("Expecting ',' or ']' in type parameter list");
    if (token2.type == R_BRACK) {
      scan ();    // Junk
      scan ();    // R_BRACK
    }
  }
  return first;
}



// parseClass ()
//
// This routine parses the following:
//    Class        --> class ID [ TypeParms ]
//                       [ implements TypeInstanceList ]
//                         superclass TypeInstance
//                       [ fields { Decl }+ ]
//                       [ methods { MethProto }+ ]
//                     endClass

//
ClassDef * parseClass () {
  ClassDef * cl;
  MethodProto * methProto, * lastMethProto;
  int count, gotObject;
  TypeArg * typeArg, * lastTypeArg;

  if (token.type != CLASS) {
    programLogicError ("Already checked for CLASS keyword");
  }
  scan ();   // CLASS
  cl = new ClassDef ();
  cl->id = mustHaveID ("Expecting class name after 'class'");
  if (cl->id == stringObject) {
    gotObject = 1;
  } else {
    gotObject = 0;
  }
  // Pick up TypeParms...
  cl->typeParms = parseTypeParms ();
  if (gotObject && cl->typeParms) {
    syntaxError ("Not expecting typeParms in class Object");
  }

  // Pick up 'implements' clause...
  while ((token.type != IMPLEMENTS) &&
      (token.type != SUPER_CLASS) &&
      (token.type != FIELDS) &&
      (token.type != MESSAGES) &&
      (token.type != METHODS) &&
      (token.type != END_CLASS) &&
      (token.type != EOF)) {
    syntaxError ("Expecting IMPLEMENTS, SUPER_CLASS, FIELDS, METHODS, or END_CLASS in a class definition");
    scan ();
  }
  if (token.type == IMPLEMENTS) {
    scan ();
    lastTypeArg = NULL;
    while (1) {
      typeArg = new TypeArg ();
      typeArg->type = parseNamedType ("Expecting an interface name after 'implements'");
      if (lastTypeArg == NULL) {
        cl->implements = typeArg;
      } else {
        lastTypeArg->next = typeArg;
      }
      lastTypeArg = typeArg;
      if (token.type != COMMA) {
        break;
      }
      scan ();
    }
  }

  // Pick up the superclass clause...
  while ((token.type != SUPER_CLASS) &&
      (token.type != FIELDS) &&
      (token.type != MESSAGES) &&
      (token.type != METHODS) &&
      (token.type != END_CLASS) &&
      (token.type != EOF)) {
    syntaxError ("Expecting SUPER_CLASS, FIELDS, METHODS, or END_CLASS in a class definition");
    scan ();
  }
  if (token.type == SUPER_CLASS) {
    scan ();
    cl->superclass = parseNamedType ("Expecting a class name after 'superclass'");
    if (gotObject) {
      syntaxError ("Not expecting 'superclass...' in class Object");
    }
  } else {
    if (! gotObject) {
      syntaxError ("Expecting 'superclass...'; all classes except 'Object' must have a single superclass");
    }
  }

  // Check for common errors...
  if (token.type == IMPLEMENTS) {
    syntaxError ("The 'implements' clause should come before the 'superclass' clause");
  }

  // Pick up 'fields' clause...
  while ((token.type != FIELDS) &&
      (token.type != MESSAGES) &&
      (token.type != METHODS) &&
      (token.type != END_CLASS) &&
      (token.type != EOF)) {
    syntaxError ("Expecting FIELDS, METHODS, or END_CLASS in a class definition");
    scan ();
  }
  if (token.type == FIELDS) {
    scan ();
    cl->fields = parseClassFields ();
  }

  // Pick up 'methods' clause...
  while ((token.type != METHODS) &&
      (token.type != MESSAGES) &&
      (token.type != END_CLASS) &&
      (token.type != EOF)) {
    syntaxError ("Expecting METHODS or END_CLASS in a class definition");
    scan ();
  }
  if (token.type == METHODS || token.type == MESSAGES) {
    if (token.type == MESSAGES) {
      syntaxError ("Use METHODS, not MESSAGES");
    }
    scan ();
    lastMethProto = parseMethodProto ();
    cl->methodProtos = lastMethProto;
    while (1) {
      if ((token.type == INFIX) ||
          (token.type == PREFIX) ||
          ((token.type == ID) && (token2.type == L_PAREN)) ||
          ((token.type == ID) && (token2.type == COLON)) ) {
        methProto = parseMethodProto ();
        lastMethProto->next = methProto;
        lastMethProto = methProto;
      } else if (token.type == END_CLASS) {
        break;
      } else {
        if (token.type == METHOD) {
          syntaxError ("Methods must be placed in a 'behavior', not in the class specification");
        } else {
          syntaxError ("Expecting next method prototype or 'endClass'");
        }
        count = 0;
        while (1) {
          if (inHeaderSet ()) break;
          if (token.type == INFIX) break;
          if (token.type == PREFIX) break;
          if ((token.type == ID) && (token2.type == L_PAREN)) break;
          if ((token.type == ID) && (token2.type == COLON)) break;
          if (token.type == END_CLASS) break;
          scan ();
          count++;
        }
        checkTokenSkipping (count);
        if (inHeaderSet ()) break;
      }
    }
  }
  mustHave (END_CLASS, "Expecting END_CLASS in a class definition");
  return cl;
}



// parseType (errorMsg)
//
// This routine parses a Type and returns a pointer to a Type node.  If problems,
// it prints the 'msg' and returns a pointer to a VoidType.
//
// If syntax errors occur, IT MAY SCAN ZERO TOKENS.
//
Type * parseType (char * errorMsg) {
  Type * type;
  PtrType * ptrType;
  RecordType * recordType;
  ArrayType * arrayType;
  Token tokenForPos;

  // If the current token is wrong, try just skipping it.
  if (!inFirstType (token) && inFirstType (token2)) {
    syntaxError (errorMsg);
    scan ();
  }

  switch (token.type) {
  
    case INT:
      type = new IntType ();
      scan ();
      return type;
  
    case DOUBLE:
      type = new DoubleType ();
      scan ();
      return type;
  
    case CHAR:
      type = new CharType ();
      scan ();
      return type;
  
    case BOOL:
      type = new BoolType ();
      scan ();
      return type;
  
    case VOID:
      type = new VoidType ();
      scan ();
      return type;
  
    case TYPE_OF_NULL:
      type = new TypeOfNullType ();
      scan ();
      return type;
   
    case ANY_TYPE:
      type = new AnyType ();
      scan ();
      return type;
 
    case PTR:
      ptrType = new PtrType ();
      scan ();
      mustHave (TO, "Expecting 'to' in 'ptr to Type'");
      ptrType->baseType = parseType ("Expecting type after 'to' in 'ptr to Type'");
      return ptrType;
  
    case ARRAY:
      tokenForPos = token;
      scan ();    // ARRAY
      if (token.type == L_BRACK) {
        scan ();
        return parseArrayType (tokenForPos);
      } else {

        // In the old syntax "[*]" was not optional; now it is optional...
        // syntaxError ("Expecting '[' in 'array [...] of Type'");
        // scanToFollowType ();
        // arrayType = new ArrayType ();
        // arrayType->positionAtToken (tokenForPos);
        // arrayType->baseType = new VoidType ();
        // return arrayType;

        arrayType = new ArrayType ();
        arrayType->positionAtToken (tokenForPos);
        if (token.type == OF) {
          scan ();
        } else {
          syntaxError ("Expecting '[' or 'of' in 'array [...] of Type'");
          scanToFollowType ();
          arrayType->baseType = new VoidType ();
          return arrayType;
        }
        arrayType->baseType = parseType ("Expecting base type in 'array of BaseType'");
        return arrayType;
      }
  
    case RECORD:
      recordType = new RecordType ();
      scan ();
      recordType->fields = parseRecordFieldList ();
      return recordType;
  
    case FUNCTION:
      return parseFunctionType ();
  
    case ID:
      return parseNamedType ("Program Logic Error: only prints error if an ID is not seen");
  
    default:
      syntaxError (errorMsg);
      type = new VoidType ();
      return type;
  }
  return new VoidType ();
}



// parseRecordFieldList ()
//
// This routine parses
//    { Decl }+ endRecord
// and returns a pointer to linked list of RecordFields.  If problems, it
// may return NULL.  It may scan zero tokens.
//
RecordField * parseRecordFieldList () {
  RecordField * first, * last, * idListFirst, * idListLast, * field;
  Type * type;
  first = last = NULL;
  while (token.type == ID) {
    // Pick up a list of ID's
    idListFirst = idListLast = NULL;
    while (token.type == ID) {
      field = new RecordField ();
      field->id = token.value.svalue;
      scan ();    // ID
      if (idListFirst == NULL) {
        idListFirst = field;
      } else {
        idListLast->next = field;
      }
      idListLast = field;
      if (token.type != COMMA) {
        break;
      }
      scan ();     // COMMA
    }
    // Pick up a type
    mustHave (COLON, "Expecting ':' in 'fieldID: Type'");
    type = parseType ("Expecting type after ':' in 'fieldID: Type'");
    // Now run through the ID's.  Add in the type and add to the growing field list.
    while (idListFirst) {
      idListFirst->type = type;
      if (first == NULL) {
        first = idListFirst;
      } else {
        last->next = idListFirst;
      }
      last = idListFirst;
      idListFirst = (RecordField *) idListFirst->next;
    }
    last->next = NULL;
  }
  mustHave (END_RECORD, "Expecting next field or 'endRecord' in record type");
  if (first == NULL) {
    syntaxError ("Must have at least one field in a record type");
  }
  return first;
}



// parseFunctionType ()
//
// This routine parses
//    function '(' [ Type { , Type } ] ')' [ returns Type ]
// and returns a FunctionType.  It will scan at least one token.
//
FunctionType * parseFunctionType () {
  FunctionType * functionType;
  TypeArg * typeArg, * last;
  functionType = new FunctionType ();
  scan ();     // FUNCTION keyword
  mustHave (L_PAREN, "Expecting '(Type, Type, ...)' after 'function' in function type");
  if (token.type != R_PAREN) {
    last = NULL;
    while (1) {
      typeArg = new TypeArg ();
      typeArg->type = parseType ("Expecting next type in 'function (Type, Type, ...)'");
      if (last == NULL) {
        functionType->parmTypes = typeArg;
      } else {
        last->next = typeArg;
      }
      last = typeArg;
      if (token.type != COMMA) {
        break;
      }
      scan ();
    }
  }
  if (token.type == COLON) {
    mustHave (R_PAREN, "Parameter names are not allowed in function types");
  } else {
    mustHave (R_PAREN, "Expecting ',' or ')' in parameter type list");
  }
  if (token.type == RETURNS) {
    scan ();
    functionType->retType = parseType ("Expecting return type after RETURNS");
  // A common error is to use 'return' instead of 'returns', but we
  // cannot do a special check for this error.  Consider
  //      if x isKindOf ptr to function (int)
  //        return 123
  //      else
  //        return 456
  //      endIf
  // } else if ((token.type == RETURN) &&
  //            (token2.type != END_FUNCTION)) {
  //   syntaxError ("Expecting RETURNS, not RETURN");
  //   scan ();
  //   functionType->retType = parseType ("Expecting return type after RETURNS");
    if (gotVoidType (functionType->retType)) {
      syntaxError ("Do not say 'returns void'; just leave it out");
    }
  } else {
    functionType->retType = new VoidType ();
  }
  return functionType;
}



// parseNamedType (errorMsg)
//
// This routine parses
//    ID [ '[' Type { , Type } ']' ]
// and returns a NamedType.  It may scan zero tokens, if there are errors.
//
NamedType * parseNamedType (char * errorMsg) {
  TypeArg * typeArg, * last;
  NamedType * namedType;
  namedType = new NamedType ();
  namedType->id = mustHaveID (errorMsg);
  if (token.type == L_BRACK) {
    scan ();    // L_BRACK
    last = NULL;
    while (1) {
      typeArg = new TypeArg ();
      typeArg->type = parseType ("Expecting next type in parameterized type 'ID[Type,Type,...]'");
      if (last == NULL) {
        namedType->typeArgs = typeArg;
      } else {
        last->next = typeArg;
      }
      last = typeArg;
      if (token.type != COMMA) {
        break;
      }
      scan ();   // COMMA
    }
    mustHave (R_BRACK, "Expecting ',' or ']' in parameter type list");
  }
  return namedType;
}



// parseArrayType (tokenForPos)
//
// This routine parses according to this syntax:
//    ( '*' | Expr ) { , ( '*' | Expr ) } ']' of Type
// and returns an ArrayType.  It calls itself recursively; Each invocation
// will pick up one sizeExpression and will allocate one ArrayType node.
//
// It positions the ArrayType node on the "tokenForPos".
//
ArrayType * parseArrayType (Token tokenForPos) {
  ArrayType * arrayType;
  arrayType = new ArrayType ();
  arrayType->positionAtToken (tokenForPos);
  arrayType->baseType = new VoidType ();
  if ((token.type == OPERATOR) && (token.value.svalue == stringStar)) {
    scan ();    // implicitly set arrayType->sizeExpr to NULL
  } else if (inFirstExpr (token)) {
    arrayType->sizeExpr = parseExpr ("Expecting a size expression");
  } else {
    syntaxError ("Expecting Expr or '*' after '[' or ',' in 'array [...] of Type'");
    scanToFollowType ();
    return arrayType;
  }
  if (token.type == R_BRACK) {
    scan ();
    if (token.type == OF) {
      scan ();
    } else {
      syntaxError ("Expecting 'of' in 'array [...] of Type'");
      scanToFollowType ();
      return arrayType;
    }
    arrayType->baseType = parseType ("Expecting base type in 'array [...] of BaseType'");
    return arrayType;
  } else if (token.type == COMMA) {
    scan ();
    arrayType->baseType = parseArrayType (tokenForPos);
    return arrayType;
  } else {
    syntaxError ("Expecting ']' or ',' in 'array [...] of Type'");
    scanToFollowType ();
    return arrayType;
  }
}



/*----------  Here is the parsing precedence table.  All are left-associative.  ----------

1:   Keyword messages

2:   Other Infix

3:   |

4:   ^

5:   &

6:   ||

7:   ^^   <--- Boolean X-OR was eliminated, since it is the same as !=

8:   &&

9:   ==  !=

10:  <  <=  >  >=

11:  <<  >>  >>>

12:  +  -

13:  *  /  %

15:  All Prefix Operators

16:  x.m()  x.f  a[...]  special(asPtr2,asInt,arraySize,isInstOf,isKindOf)

17:  (expr)  constants  closure  x  foo()  new  alloc  sizeOf

----------*/



// parseExpr (errorMsg)
//
// This routine parses the grammar rule:
//    Expr  -->  Expr1
//
// This routine never returns NULL, even if there are parsing errors.
// This routine may scan ZERO tokens.
//
Expression * parseExpr (char * errorMsg) {

  // If the current token is wrong, try just skipping it.
  if (!inFirstExpr (token) && inFirstExpr (token2)) {
    syntaxError (errorMsg);
    scan ();
  }

  return parseExpr1 (errorMsg);
}



// parseExpr0 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr  -->  Expr2
//
// It has basically the same behavior as "parseExpr", except that keyword messages
// (which are handled in parseExpr1) are not recognized unless they are in
// parentheses.  This routine is used in parsing VarDecls (instead of parseExpr),
// since we have a problem with keyword expressions being confused with additional
// declarations.
//
// Consider
//    var  x: T = a at: b
// and
//    var x: T = a
//        y: T ...
// In each case, we have:
//    VAR ID : Type = ID ID : ...
// Since it is impossible to distinguish these two rather different interpretations,
// we needed to do something.  The following changes to the grammar were
// considered and ruled out.
//     (1)   var x: T = a at: b;
//               y: T = ...;
//     (2)   var x: T = a at: b,
//               y: T = ...
//     (3)   var x: T = a at: b
//               y: T = ...
//           endVar
//     (4)   var x: T = a at: b
//               y: T = ...
//           do...
// By using parseExpr0, the user will be required to insert something (namely,
// parentheses) only for this particular problem case:
//     var x: T = a + 1
//         y: T = (a at: b)
//         z: T = c
//
Expression * parseExpr0 (char * errorMsg) {

  // If the current token is wrong, try just skipping it.
  if (!inFirstExpr (token) && inFirstExpr (token2)) {
    syntaxError (errorMsg);
    scan ();
  }

  return parseExpr2 (errorMsg);
}



// parseExpr1 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr1  -->  Expr2 { ID ':' Expr2 }*            // SendExpr
// For example:
//    x at: y put: z
//
// This routine builds up a string such as "at:put:" and (by calling "lookupAndAdd")
// enters it in the string table as an ID.
//
Expression * parseExpr1 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg, * lastArg;
  char * newSelector;

  a = parseExpr2 (errorMsg);
  if ((token.type != ID) || (token2.type != COLON)) {
    return a;
  }
  sendExpr = new SendExpr ();
  newSelector = appendStrings (token.value.svalue->chars, ":", "");
  sendExpr->kind = KEYWORD;
  scan ();      // ID
  scan ();      // COLON
  sendExpr->receiver = a;
  arg = new Argument ();
  arg->expr = parseExpr2 ("Expecting first argument expression after ':'");
  sendExpr->argList = arg;
  lastArg = arg;
  while (1) {
    if ((token.type != ID) || (token2.type != COLON)) {
      sendExpr->selector = lookupAndAdd (newSelector, ID);
      return sendExpr;
    }
    newSelector = appendStrings (newSelector, token.value.svalue->chars, ":");
    arg = new Argument ();
    scan ();      // ID
    scan ();      // COLON
    arg->expr = parseExpr2 ("Expecting next argument expression after ':'");
    lastArg->next = arg;
    lastArg = arg;
  }
}



// parseExpr2 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr2  -->  Expr3 { OPERATOR Expr3 }            // SendExpr (INFIX)
//
Expression * parseExpr2 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr3 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) &&
       (token.value.svalue != stringBarBar) &&
       (token.value.svalue != stringAmpAmp) &&
       (token.value.svalue != stringBar) &&
       (token.value.svalue != stringCaret) &&
       (token.value.svalue != stringAmp) &&
       (token.value.svalue != stringEqualEqual) &&
       (token.value.svalue != stringNotEqual) &&
       (token.value.svalue != stringLess) &&
       (token.value.svalue != stringLessEqual) &&
       (token.value.svalue != stringGreater) &&
       (token.value.svalue != stringGreaterEqual) &&
       (token.value.svalue != stringLessLess) &&
       (token.value.svalue != stringGreaterGreater) &&
       (token.value.svalue != stringGreaterGreaterGreater) &&
       (token.value.svalue != stringPlus) &&
       (token.value.svalue != stringMinus) &&
       (token.value.svalue != stringStar) &&
       (token.value.svalue != stringSlash) &&
       (token.value.svalue != stringPercent)
      ) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr3 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr3 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr3  -->  Expr5 { '||' Expr5 }        // SendExpr (INFIX)
//
Expression * parseExpr3 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr5 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringBarBar)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr5 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr5 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr5  -->  Expr6 { '&&' Expr6 }           // SendExpr (INFIX)
//
Expression * parseExpr5 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr6 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringAmpAmp)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr6 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr6 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr6  -->  Expr7 { '|' Expr7 }           // SendExpr (INFIX)
//
Expression * parseExpr6 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr7 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringBar)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr7 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr7 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr7  -->  Expr8 { '^' Expr8 }           // SendExpr (INFIX)
//
Expression * parseExpr7 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr8 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringCaret)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr8 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr8 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr8  -->  Expr9 { '&' Expr9 }           // SendExpr (INFIX)
//
Expression * parseExpr8 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr9 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringAmp)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr9 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr9 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr9  -->  Expr10 {
//                           '==' Expr10            // SendExpr (INFIX)
//                           '!=' Expr10            // SendExpr (INFIX)
//                                       }
//
Expression * parseExpr9 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr10 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringEqualEqual) ||
           (token.value.svalue == stringNotEqual)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr10 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr10 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr10  -->  Expr11 {
//                           '<' Expr11            // SendExpr (INFIX)
//                           '<=' Expr11            // SendExpr (INFIX)
//                           '>' Expr11            // SendExpr (INFIX)
//                           '>=' Expr11            // SendExpr (INFIX)
//                                       }
//
Expression * parseExpr10 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr11 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringLess) ||
           (token.value.svalue == stringLessEqual) ||
           (token.value.svalue == stringGreater) ||
           (token.value.svalue == stringGreaterEqual)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr11 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr11 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr11  -->  Expr12 {
//                           '<<' Expr12            // SendExpr (INFIX)
//                           '>>' Expr12            // SendExpr (INFIX)
//                           '>>>' Expr12            // SendExpr (INFIX)
//                                       }
//
Expression * parseExpr11 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr12 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringLessLess) ||
           (token.value.svalue == stringGreaterGreater) ||
           (token.value.svalue == stringGreaterGreaterGreater)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr12 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr12 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr12  -->  Expr13 {
//                           '+' Expr13            // SendExpr (INFIX)
//                           '-' Expr13            // SendExpr (INFIX)
//                                       }
//
Expression * parseExpr12 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr13 (errorMsg);
  while (1) {
    if ((token.type == OPERATOR) && (
           (token.value.svalue == stringPlus) ||
           (token.value.svalue == stringMinus)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr13 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr13 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr13  -->  Expr15 {
//                           '*' Expr15            // SendExpr (INFIX)
//                           '/' Expr15            // SendExpr (INFIX)
//                           '%' Expr15            // SendExpr (INFIX)
//                                       }
//
// There is an ambiguity in the grammer.  Consider:
//   expr -->  expr '*' expr
// Any opertor, such as '*', may be interpreted as infix or prefix.
// Here are two examples, that *might* look legal.
//   x = 123 * p.foo()
//
//   x = 123
//   *p.foo()
//
// However, the last stmt is not actually legal since it would be interpreted as:
//   *(p.foo())
// since prefix binds more loosely than message-send-dot.  This violates the
// syntax of the language (prefix expressions always returns values, so they cannot
// be used at the statement level).  If you want the other interpretation, you would
// have to add parentheses anyway, so it would look like this:
//   x = 123
//   (*p).foo()
// This would preclude the interpretation of * as a binary operator.
//
// This applies to all operator symbols, such as +, -, and *.  However, there is a
// second use of "*" that causes parsing problems.  Consider this sequence of tokens:
//   x = 123 * p ...
// This could have two interpretations, as shown by these examples:
//   x = 123
//   *p = 456       -- prefix operator (deref is OK for lhs of assignment)
// or
//   x = 123 * p    -- binary operator
//
// Since "p" could be an arbitrary expression, we must distinguish the cases here in
// this routine before parsing "p".  We do this with the following (non-CFG)
// restrictions:
//
//   (1) If a "*" is preceded by a newline, it is assumed to be a prefix operator
//   (2) If a "*" occurs on the same line as the previous token, we assume it is
//       an infix operator.
//
// In the rare case when you want two such statements on the same line, you would have
// to use parentheses to force the desired interpretation.  For example:
//   x = 123   (*p) = 456
// If you have long infix expressions which must be spread over several lines, do it
// like this:
//   x = aaaaaa *
//         bbbbbb *
//         cccccc
//
Expression * parseExpr13 (char * errorMsg) {
  Expression * a;
  SendExpr * sendExpr;
  Argument * arg;

  a = parseExpr15 (errorMsg);
  while (1) {
    // If this token is "*" and it is on the same line as the last token,
    // or we have "/" or "%"...
    if ((token.type == OPERATOR) &&
        (((token.value.svalue == stringStar) &&
          (extractLineNumber (tokenMinusOne) == extractLineNumber (token))) ||
         (token.value.svalue == stringSlash) ||
         (token.value.svalue == stringPercent)
                                     )) {
      sendExpr = new SendExpr ();
      sendExpr->selector = token.value.svalue;
      sendExpr->kind = INFIX;
      scan ();
      sendExpr->receiver = a;
      arg = new Argument ();
      arg->expr = parseExpr15 (errorMsg);
      sendExpr->argList = arg;
      a = sendExpr;
    } else {
      return a;
    }
  }
}



// parseExpr15 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr15  -->  OPERATOR Expr15       // PREFIX
//            -->  Expr16
//
Expression * parseExpr15 (char * errorMsg) {
  SendExpr * prefix;

  switch (token.type) {
      case OPERATOR:
        prefix = new SendExpr ();
        prefix->selector = lookupAndAdd (appendStrings (
                              "_prefix_", token.value.svalue->chars, ""), OPERATOR);
        prefix->kind = PREFIX;
        scan ();
        prefix->receiver = parseExpr15 (errorMsg);
        return prefix;
    default:
      return parseExpr16 (errorMsg);
  }
}



// parseExpr16 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr16  -->  Expr17 { '.' ID ArgList               // SendExpr
//                        | '.' ID                       // FieldAccess
//                        | 'asPtrTo' Type
//                        | 'asInteger'
//                        | 'arraySize'
//                        | 'isInstanceOf' Type
//                        | 'isKindOf' Type
//                        | '[' Expr { ',' Expr } ']'     // ArrayAccess
//                       }
//
Expression * parseExpr16 (char * errorMsg) {
  Expression * a;
  Expression * newExpr;
  AsIntegerExpr * asIntegerExpr;
  ArraySizeExpr * arraySizeExpr;
  IsInstanceOfExpr * isInstanceOfExpr;
  IsKindOfExpr * isKindOfExpr;
  AsPtrToExpr * asPtrToExpr;
  SendExpr * sendExpr;
  FieldAccess * fieldAccess;
  Token tok;

  a = parseExpr17 (errorMsg);
  while (1) {
    switch (token.type) {

      case PERIOD:
        if (token2.type == ID) {
          // If we have ". ID (" and the ID and "(" are on the same line...
          if (token3.type == L_PAREN &&
                extractLineNumber (token2) == extractLineNumber (token3)) {
            scan ();    // PERIOD
            sendExpr = new SendExpr ();
            sendExpr->receiver = a;
            sendExpr->kind = NORMAL;
            sendExpr->selector = token.value.svalue;
            scan ();    // ID
            scan ();    // L_PAREN
            sendExpr->argList = parseArgList ();
            a = sendExpr;
          } else {
            scan ();    // PERIOD
            fieldAccess = new FieldAccess ();
            fieldAccess->id = token.value.svalue;
            scan ();    // ID
            fieldAccess->expr = a;
            a = fieldAccess;
          }
        } else {
          syntaxError ("Expecting field ID or message selector after '.'");
          scan ();      // PERIOD
        }
        break;

      case AS_PTR_TO:
        asPtrToExpr = new AsPtrToExpr ();
        scan ();
        asPtrToExpr->expr = a;
        asPtrToExpr->type = parseType ("Expecting type in 'expr asPtrTo Type'");
        a = asPtrToExpr;
        break;

      case AS_INTEGER:
        asIntegerExpr = new AsIntegerExpr ();
        scan ();
        asIntegerExpr->expr = a;
        a = asIntegerExpr;
        break;

      case ARRAY_SIZE:
        arraySizeExpr = new ArraySizeExpr ();
        scan ();
        arraySizeExpr->expr = a;
        a = arraySizeExpr;
        break;

      case IS_INSTANCE_OF:
        isInstanceOfExpr = new IsInstanceOfExpr ();
        scan ();
        isInstanceOfExpr->expr = a;
        isInstanceOfExpr->type = parseType ("Expecting type in 'expr isInstanceOf Type'");
        a = isInstanceOfExpr;
        break;

      case IS_KIND_OF:
        isKindOfExpr = new IsKindOfExpr ();
        scan ();
        isKindOfExpr->expr = a;
        isKindOfExpr->type = parseType ("Expecting type in 'expr isKindOf Type'");
        a = isKindOfExpr;
        break;

      case L_BRACK:
        tok = token;
        scan ();      // L_BRACK
        a = parseArrayAccess (tok, a);
        break;

      default:
        return a;
    }
  }
}



// parseExpr17 (errorMsg)
//
// This routine parses the grammar rule:
//    Expr17  -->  '(' Expr ')'
//            -->  'null'
//            -->  'true'
//            -->  'false'
//            -->  'self'
//            -->  'super'
//            -->  INTEGER
//            -->  DOUBLE
//            -->  CHAR
//            -->  STRING
//            -->  Closure
//            -->  ID                   // Variable
//            -->  ID ArgList           // Call Expr
//            -->  'new' Constructor
//            -->  'alloc' Constructor
//            -->  'sizeOf' Type
//
//
// There is an ambiguity in the grammar.  Consider this sequence of tokens:
//     x = f ( ...
// This could have two interpretations, as suggested by these examples:
//     x = f (123)      -- function call
// and
//     x = f
//     (p.foo).bar()    -- next stmt begins with a '('
//
// Unfortunately, when we get an ID followed by a '(', we can't tell whether we are
// looking at a function call or some other expression.  We differentiate these
// two cases by making the following (non-CFG) rules:
//   (1) For a function call, the '(' must appear on the same line as the function name
//   (2) Otherwise, we interpret it as a separate expression.
// If you really must have 2 statements on the same line, but you do not wish the
// interpretation as a function call, you must insert an extra set of parens, e.g.,
//     x = (f)     (p.foo).bar()
// If you really must have a function call that is spread over several lines,
// do it like this:
//     x = f (a,
//            b,
//            g (x,
//               y,
//               z),
//            c)
// or like this if you can't get 'f' and 'a' on one line and want to really spread
// it out:
//     x = ffffffffff (
//            a,
//            b,
//            ggggggggg (
//              x,
//              y,
//              z
//            ),
//            c
//         )
//   
Expression * parseExpr17 (char * errorMsg) {
  IntConst * e1;
  DoubleConst * e2;
  CharConst * e3;
  StringConst * e4;
  Expression * exp;
  SizeOfExpr * sizeOfExpr;
  VariableExpr * var;
  CallExpr * callExpr;
  ClosureExpr * closureExpr;

  switch (token.type) {
  
    case INT_CONST:
      e1 = new IntConst ();
      e1->ivalue = token.value.ivalue;
      scan ();
      return e1;
  
    case DOUBLE_CONST:
      e2 = new DoubleConst ();
      e2->rvalue = token.value.rvalue;
      scan ();
      return e2;
  
    case CHAR_CONST:
      e3 = new CharConst ();
      e3->ivalue = token.value.ivalue;
      scan ();
      return e3;
  
    case STRING_CONST:
      e4 = new StringConst ();
      e4->svalue = token.value.svalue;
      scan ();
      return e4;
  
    case TRUE:
      exp = new BoolConst (1);
      scan ();
      return exp;
  
    case FALSE:
      exp = new BoolConst (0);
      scan ();
      return exp;
  
    case NULL_KEYWORD:
      exp = new NullConst ();
      scan ();
      return exp;
  
    case SELF:
      exp = new SelfExpr ();
      scan ();
      return exp;
  
    case SUPER:
      exp = new SuperExpr ();
      scan ();
      return exp;

    case L_PAREN:
      scan ();
      exp = parseExpr ("Expecting an expression after '('");
      if (token.type == R_PAREN) {
        scan ();
      } else if (token2.type == R_PAREN) {
        syntaxError ("Expecting ')'");
        scan ();
        scan ();
      } else {
        syntaxError ("Expecting ')'");
      }
      return exp;

    case SIZE_OF:
      sizeOfExpr = new SizeOfExpr ();
      scan ();
      sizeOfExpr->type = parseType ("Expecting type after 'sizeOf'");
      return sizeOfExpr;

    case ID:
      // If we have "ID (" and they are on the same line...
      if ((token2.type == L_PAREN) &&
          extractLineNumber (token) == extractLineNumber (token2)) {   // Call Expression
        callExpr = new CallExpr ();
        callExpr->id = token.value.svalue;
        scan ();    // ID
        scan ();    // L_PAREN
        callExpr->argList = parseArgList ();
        return callExpr;
      } else {                                     // Simple variable
        var = new VariableExpr ();
        var->id = token.value.svalue;
        scan ();
        return var;
      }

    case FUNCTION:       // Closure expression
      closureExpr = new ClosureExpr ();
      closureExpr->function = parseFunction (0);    // Expecting ID = False
      return closureExpr;

    case NEW:
    case ALLOC:
      return parseConstructor ();

    default:
      syntaxError (errorMsg);
      scan ();
      return new NullConst ();
  }
}



// parseArrayAccess (tokenForPos, soFar)
//
// This routine parses according to this syntax:
//    Expr { , Expr } ']'
// and returns an ArrayAccess.  It calls itself recursively; Each invocation
// will pick up one Expression and will allocate one ArrayAccess node.
//
// 'soFar' is whatever expression we have parsed so far, e.g., 'a[1,2'.
//
// It positions the ArrayAccess node on the "tokenForPos".
//
ArrayAccess * parseArrayAccess (Token tokenForPos, Expression * soFar) {
  int count;
  ArrayAccess * arrayAccess;

  arrayAccess = new ArrayAccess ();
  arrayAccess->positionAtToken (tokenForPos);
  arrayAccess->arrayExpr = soFar;
  arrayAccess->indexExpr = parseExpr ("Expecting an index expression here");
  if (token.type == R_BRACK) {
    scan ();
    return arrayAccess;
  } else if (token.type == COMMA) {
    scan ();
    return parseArrayAccess (tokenForPos, arrayAccess);
  } else {
    syntaxError ("Expecting ']' or ',' in 'arr [...]'");
    // Scan until we hit either FIRST-EXPR or , or ]
    count = 0;
    while (1) {
      if (token.type == EOF) {
        break;
      } else if (token.type == R_BRACK) {
        scan ();
        break;
      } else if (token.type == COMMA) {
        scan ();
        checkTokenSkipping (count);
        return parseArrayAccess (tokenForPos, arrayAccess);
      } else if (inFirstExpr (token)) {
        checkTokenSkipping (count);
        return parseArrayAccess (tokenForPos, arrayAccess);
      }
      scan ();
      count++;
    }
    checkTokenSkipping (count);
    return arrayAccess;
  }
}



// parseArgList ()
//
// Parse a comma separated list of expressions, followed by the closing ')'.
//     ArgList --> Expr ',' Expr ',' ... ',' Expr ')'
//             --> ')'
// This routine builds and returns a linked-list of Argument nodes.
// If no arguments are present, then it returns NULL.
//
// If there are syntax errors, this routine may scan ZERO tokens.
//
Argument * parseArgList () {
  Argument * newArg, * first, * last;
  if (token.type == R_PAREN) {
    scan ();
    return NULL;
  }
  newArg = new Argument ();
  newArg->expr = parseExpr ("Expecting an argument or ')' here");
  first = last = newArg;
  while (token.type == COMMA) {
    scan ();   // COMMA
    newArg = new Argument ();
    newArg->expr = parseExpr ("Expecting the next argument after comma");
    last->next = newArg;
    last = newArg;
  }
  if (token.type == R_PAREN) {
    scan ();      // R_PAREN
  } else {
    syntaxError ("Expecting ',' or ')' in ArgList");
    if (token2.type == R_PAREN) {
      scan ();    // Junk
      scan ();    // R_PAREN
    }
  }
  return first;
}



// parseConstructor ()
//
// Parse the following syntax rules:
//
// Expr            --> NEW Constructor
//                 --> ALLOC Constructor
//
// Constructor     --> Type ( ClassRecordInit | ArrayInit | <none> )
//
// ClassRecordInit --> '{'     ID = Expr
//                         { , ID = Expr }   '}'  ]
// ArrayInit       --> '{'    [ Expr 'of' ] Expr
//                        { , [ Expr 'of' ] Expr }   '}'
//
// This routine builds a Constructor node.  A constructor can be used to create/
// initialize either:
//    a class instance     Person { name = "harry", age = 46 }
//    a record instance    Node { val = 5, next = p }
//    an array             array of int { 100 of -1, 100 of -2}
//
// It could be that there is no initialization list, for example:
//    MyType
// (Note that we never have an empty list like "MyType { }"...)
// This routine makes no attempt to check that the sort of initialization in any
// way matches the type; this will be done during semantic checking.
//
// Even if errors, this routine will not return NULL.
//
Constructor * parseConstructor () {
  Constructor * constructor;
  FieldInit * fieldInit, * firstFieldInit, * lastFieldInit;
  CountValue * countValue, * firstCountValue, * lastCountValue;

  constructor = new Constructor ();
  constructor->allocKind = token.type;    // NEW or ALLOC
  scan ();
  constructor->type = parseType ("Expecting type in constructor, after NEW or ALLOC");

  if (token.type != L_BRACE) {
    return constructor;
  }
  scan ();

  // If we have a Class/Record initialization list...
  if ((token.type == ID) && (token2.type == EQUAL)) {
    fieldInit = new FieldInit ();
    fieldInit->id = token.value.svalue;
    scan ();    // ID
    scan ();    // EQUAL
    fieldInit->expr = parseExpr ("Expecting an initial value after '='");
    firstFieldInit = lastFieldInit = fieldInit;
    while (token.type == COMMA) {
      scan ();   // COMMA
      fieldInit = new FieldInit ();
      fieldInit->id = mustHaveID ("Expecting ID after ',' in class/record constructor initialization list");
      mustHave (EQUAL, "Expecting '=' after ID in class/record constructor initialization list");
      fieldInit->expr = parseExpr ("Expecting an initial value after '='");
      lastFieldInit->next = fieldInit;
      lastFieldInit = fieldInit;
      if ((token.type != COMMA) && (token.type != R_BRACE)) {
        syntaxError ("Expecting ',' or '}' in class/record constructor initialization list");
        if ((token2.type == COMMA) || (token2.type == R_BRACE)) {
          scan ();    // junk
        }
      }
    }
    mustHave (R_BRACE, "Expecting ',' or '}' in class/record constructor initialization list");
    constructor-> fieldInits = firstFieldInit;
    return constructor;

  // If we have an Array initialization list...
  } else {
    if (token.type == R_BRACE) {
      syntaxError ("Empty list in constructor is not allowed; just omit the '{}' entirely");
      scan ();     // R_BRACE
      return constructor;
    }
    countValue = new CountValue ();
    countValue->value = parseExpr ("Expecting an expression after '[' in array constructor initialization list");
    if (token.type == OF) {
      scan ();
      countValue->count = countValue->value;
      countValue->value = parseExpr ("Expecting an expression after 'of' in 'CountExpr of ValueExpr'");
    }
    firstCountValue = lastCountValue = countValue;
    while (token.type == COMMA) {
      scan ();   // COMMA
      countValue = new CountValue ();
      countValue->value = parseExpr ("Expecting next expression after ',' in array constructor initialization list");
      if (token.type == OF) {
        scan ();
        countValue->count = countValue->value;
        countValue->value = parseExpr ("Expecting an expression after 'of' in 'CountExpr of ValueExpr'");
      }
      lastCountValue->next = countValue;
      lastCountValue = countValue;
      if ((token.type != COMMA) && (token.type != R_BRACE)) {
        syntaxError ("Expecting ',' or '}' in array constructor");
        if ((token2.type == COMMA) || (token2.type == R_BRACE)) {
          scan ();    // junk
        }
      }
    }
    mustHave (R_BRACE, "Expecting ',' or '}' in array constructor initialization list");
    constructor->countValueList = firstCountValue;
    return constructor;
  }
}



//    
//    
//    
//    
//    // parseConstructor ()
//    //
//    // Parse the following syntax rules:
//    //
//    // Constructor --> Type ( ClassRecordInit | ArrayInit | <none> )
//    //
//    // ClassRecordInit --> '{'     ID = Expr
//    //                         { , ID = Expr }   '}'  ]
//    // ArrayInit --> '{'    [ Expr 'of' ] Expr
//    //                  { , [ Expr 'of' ] Expr }   '}'
//    //
//    // This routine builds a Constructor node.  A constructor can be used to create/
//    // initialize either:
//    //    a class instance     Person { name = "harry", age = 46 }
//    //    a record instance    Node { val = 5, next = p }
//    //    an array             array of int { 100 of -1, 100 of -2}
//    //
//    // It could be that there is no initialization list, for example:
//    //    MyType
//    // (Note that we never have an empty list like "MyType { }"...)
//    // This routine makes no attempt to check that the sort of initialization in any
//    // way matches the type; this will be done during semantic checking.
//    //
//    // Even if errors, this routine will not return NULL.
//    //
//    Constructor * parseConstructor () {
//      Constructor * constructor;
//      FieldInit * fieldInit, * firstFieldInit, * lastFieldInit;
//      CountValue * countValue, * firstCountValue, * lastCountValue;
//    
//      constructor = new Constructor ();
//      constructor->type = parseType ("Expecting type in constructor, after NEW or ALLOC");
//    
//      if (token.type != L_BRACE) {
//        return constructor;
//      }
//      scan ();
//    
//      // If we have a Class/Record initialization list...
//      if ((token.type == ID) && (token2.type == EQUAL)) {
//        fieldInit = new FieldInit ();
//        fieldInit->id = token.value.svalue;
//        scan ();    // ID
//        scan ();    // EQUAL
//        fieldInit->expr = parseExpr ("Expecting an initial value after '='");
//        firstFieldInit = lastFieldInit = fieldInit;
//        while (token.type == COMMA) {
//          scan ();   // COMMA
//          fieldInit = new FieldInit ();
//          fieldInit->id = mustHaveID ("Expecting ID after ',' in class/record constructor initialization list");
//          mustHave (EQUAL, "Expecting '=' after ID in class/record constructor initialization list");
//          fieldInit->expr = parseExpr ("Expecting an initial value after '='");
//          lastFieldInit->next = fieldInit;
//          lastFieldInit = fieldInit;
//          if ((token.type != COMMA) && (token.type != R_BRACE)) {
//            syntaxError ("Expecting ',' or '}' in class/record constructor initialization list");
//            if ((token2.type == COMMA) || (token2.type == R_BRACE)) {
//              scan ();    // junk
//            }
//          }
//        }
//        mustHave (R_BRACE, "Expecting ',' or '}' in class/record constructor initialization list");
//        constructor-> fieldInits = firstFieldInit;
//        return constructor;
//    
//      // If we have an Array initialization list...
//      } else {
//        if (token.type == R_BRACE) {
//          syntaxError ("Empty list in constructor is not allowed; just omit the '{}' entirely");
//          scan ();     // R_BRACE
//          return constructor;
//        }
//        countValue = new CountValue ();
//        countValue->value = parseExpr ("Expecting an expression after '[' in array constructor initialization list");
//        if (token.type == OF) {
//          scan ();
//          countValue->count = countValue->value;
//          countValue->value = parseExpr ("Expecting an expression after 'of' in 'CountExpr of ValueExpr'");
//        }
//        firstCountValue = lastCountValue = countValue;
//        while (token.type == COMMA) {
//          scan ();   // COMMA
//          countValue = new CountValue ();
//          countValue->value = parseExpr ("Expecting next expression after ',' in array constructor initialization list");
//          if (token.type == OF) {
//            scan ();
//            countValue->count = countValue->value;
//            countValue->value = parseExpr ("Expecting an expression after 'of' in 'CountExpr of ValueExpr'");
//          }
//          lastCountValue->next = countValue;
//          lastCountValue = countValue;
//          if ((token.type != COMMA) && (token.type != R_BRACE)) {
//            syntaxError ("Expecting ',' or '}' in array constructor");
//            if ((token2.type == COMMA) || (token2.type == R_BRACE)) {
//              scan ();    // junk
//            }
//          }
//        }
//        mustHave (R_BRACE, "Expecting ',' or '}' in array constructor initialization list");
//        constructor->countValueList = firstCountValue;
//        return constructor;
//      }
//    }
//    



// parseLocalVarDecls ()
//
// This routine parses the following syntax:
//     VarDecls --> [ 'var' { VarDecl }+ ]
//     VarDecl  --> Decl [ '=' Expr0 ]
//     Decl     --> ID { ',' ID } ':' Type
//
// It returns a linked list of Locals.
//
// NOTE: THIS ROUTINE IS VERY SIMILAR TO parseGlobalVarDecls.
//
Local * parseLocalVarDecls () {
  Local * first, * last, * idListFirst, * idListLast, * local;
  Type * type;
  Expression * init;
  VariableExpr * var;
  if (token.type != VAR) {
    return NULL;
  }
  first = last = NULL;
  scan ();    // VAR
  do {
    // Pick up a list of ID's
    idListFirst = idListLast = NULL;
    do {
      local = new Local ();
      local->id = mustHaveID ("Expecting a variable name ID");
      if (idListFirst == NULL) {
        idListFirst = local;
      } else {
        idListLast->next = local;
      }
      idListLast = local;
      if (token.type != COMMA) {
        break;
      }
      scan ();     // COMMA
      // Check for common errors (i.e., use of keyword as a name)...
      if (token.type != ID) {
        syntaxError ("Expecting next local variable name ID after COMMA");
        scan ();
        if (token.type == COMMA) {
          scan ();
        } else if (token.type == COLON) {
          break;
        }
      }
    } while (token.type == ID);
    // Pick up a type
    mustHave (COLON, "Expecting ': Type' in variable decl");
    type = parseType ("Expecting Type after ':' in variable decl");
    // See if we have an initializing expression
    init = NULL;
    if (token.type == EQUAL) {
      scan ();    // EQUAL
      init = parseExpr0 ("Expecting initializing expression after '='");
    }
    // Now run through the ID's.  "idListFirst" points to the next one to examine.
    while (idListFirst) {
      // Add in a ptr to the (shared) type.
      idListFirst->type = type;
      // Add "idListFirst" to the end of the main list.
      if (first == NULL) {
        first = idListFirst;
      } else {
        last->next = idListFirst;
      }
      last = idListFirst;
      // Move to the next id in the secondary list.
      idListFirst = (Local *) idListFirst->next;
      // Add in an initializing expression if there is one.
      if (init) {
        last->initExpr = init;
        // We will treat "x,y,z: int = 123" as "x=123  y=x  z=y"
        var = new VariableExpr ();
        var->positionAt (init);
        var->id = last->id;
        init = var; 
      }
    }
    last->next = NULL;
    // Continue looping if we have...
    //      ID :
    //      ID ,
    // Anything else (like a lone ID) might be an expression or stmt...
  } while ((token.type == ID) && ((token2.type == COMMA) || (token2.type == COLON)));
  return first;
}



// parseGlobalVarDecls ()
//
// This routine parses the following syntax:
//     VarDecls --> [ 'var' { VarDecl }+ ]
//     VarDecl  --> Decl [ '=' Expr0 ]
//     Decl     --> ID { ',' ID } ':' Type
//
// It returns a linked list of Globals.
//
// NOTE: THIS ROUTINE IS VERY SIMILAR TO parseLocalVarDecls.
//
Global * parseGlobalVarDecls () {
  Global * first, * last, * idListFirst, * idListLast, * global;
  Type * type;
  Expression * init;
  VariableExpr * var;
  if (token.type != VAR) {
    programLogicError ("Already checked for 'var' keyword");
  }
  first = last = NULL;
  scan ();    // VAR
  while (1) {
    // Pick up a list of ID's
    idListFirst = idListLast = NULL;
    do {
      global = new Global ();
      global->id = mustHaveID ("Expecting a global variable name ID");
      if (idListFirst == NULL) {
        idListFirst = global;
      } else {
        idListLast->next = global;
      }
      idListLast = global;
      if (token.type != COMMA) {
        break;
      }
      scan ();     // COMMA
      // Check for common errors (i.e., use of keyword as a name)...
      if (token.type != ID) {
        syntaxError ("Expecting next global variable name ID after COMMA");
        scan ();
        if (token.type == COMMA) {
          scan ();
        } else if (token.type == COLON) {
          break;
        }
      }
    } while (token.type == ID);
    // Pick up a type
    mustHave (COLON, "Expecting ': Type' in variable decl");
    type = parseType ("Expecting Type after ':' in variable decl");
    // See if we have an initializing expression
    init = NULL;
    if (token.type == EQUAL) {
      scan ();    // EQUAL
      init = parseExpr0 ("Expecting initializing expression after '='");
    }
    // Now run through the ID's.  "idListFirst" points to the next one to examine.
    while (idListFirst) {
      // Add in a ptr to the (shared) type.
      idListFirst->type = type;
      // Add "idListFirst" to the end of the main list.
      if (first == NULL) {
        first = idListFirst;
      } else {
        last->next = idListFirst;
      }
      last = idListFirst;
      // Move to the next id in the secondary list.
      idListFirst = (Global *) idListFirst->next;
      // Add in an initializing expression if there is one.
      if (init) {
        last->initExpr = init;
        // We will treat "x,y,z: int = 123" as "x=123  y=x  z=y"
        var = new VariableExpr ();
        var->positionAt (init);
        var->id = last->id;
        init = var; 
      }
    }
    last->next = NULL;
    // Continue looping if we have...
    //      ID
    //      xxx ,      (E.g., using a keyword as a var name)
    //      xxx :
    if ((token.type != ID) && (token2.type != COMMA) && (token2.type != COLON)) {
      break;
    }
  }
  return first;
}



// parseClassFields ()
//
// This routine parses the following syntax:
//     { ID { ',' ID } ':' Type }+
//
// It returns a linked list of ClassFields.
//
ClassField * parseClassFields () {
  ClassField * first, * last, * idListFirst, * idListLast, * field;
  Type * type;

  first = last = NULL;
  while (1) {
    // Pick up a list of ID's
    idListFirst = idListLast = NULL;
    do {
      field = new ClassField ();
      field->id = mustHaveID ("Expecting a field name ID");
      if (idListFirst == NULL) {
        idListFirst = field;
      } else {
        idListLast->next = field;
      }
      idListLast = field;
      if (token.type != COMMA) {
        break;
      }
      scan ();     // COMMA
      // Check for common errors (i.e., use of keyword as a name)...
      if (token.type != ID) {
        syntaxError ("Expecting next field name ID after COMMA");
        scan ();
        if (token.type == COMMA) {
          scan ();
        } else if (token.type == COLON) {
          break;
        }
      }
    } while (token.type == ID);
    // Pick up a type
    mustHave (COLON, "Expecting ': Type' in field decl");
    type = parseType ("Expecting Type after ':' in field decl");
    if (token.type == EQUAL) {
      syntaxError ("Fields may not be initialized; they are always initialized to zero-equivalents");
      scan ();
      parseExpr ("Problems after '=' in field declaration");
    }
    // Now run through the ID's.  "idListFirst" points to the next one to examine.
    while (idListFirst) {
      // Add in a ptr to the (shared) type.
      idListFirst->type = type;
      // Add "idListFirst" to the end of the main list.
      if (first == NULL) {
        first = idListFirst;
      } else {
        last->next = idListFirst;
      }
      last = idListFirst;
      // Move to the next id in the secondary list.
      idListFirst = (ClassField *) idListFirst->next;
    }
    last->next = NULL;
    // Continue looping if we have...
    //      ID
    //      xxx ,      (E.g., using a keyword as a var name)
    //      xxx :
    if ((token.type != ID) && (token2.type != COMMA) && (token2.type != COLON)) {
      break;
    }
  }
  return first;
}



// parseErrorDecls ()
//
// This routine parses the following syntax:
//     Errors --> 'errors' { ID ParmList }+
//
// It returns a linked list of ConstDecls.
//
ErrorDecl * parseErrorDecls () {
  ErrorDecl * first, * last, * errorDecl;
  if (token.type != ERRORS) {
    programLogicError ("Already checked for 'errors' keyword");
  }
  first = last = NULL;
  scan ();    // ERRORS
  if (token.type != ID) {
    syntaxError ("Expecting ID after 'errors'");
    return NULL;
  }
  while (token.type == ID) {
    errorDecl = new ErrorDecl ();
    errorDecl->id = token.value.svalue;
    scan ();    // ID
    errorDecl->parmList = parseParmList ();
    if (first == NULL) {
      first = errorDecl;
    } else {
      last->next = errorDecl;
    }
    last = errorDecl;
  }
  return first;
}



// parseConstDecls ()
//
// This routine parses the following syntax:
//     Const --> 'const' { ID = Expr }+
//
// It returns a linked list of ConstDecls.
//
ConstDecl * parseConstDecls () {
  ConstDecl * first, * last, * constDecl;
  if (token.type != CONST) {
    programLogicError ("Already checked for 'const' keyword");
  }
  first = last = NULL;
  scan ();    // CONST
  if (token.type != ID) {
    syntaxError ("Expecting ID after 'const'");
    return NULL;
  }
  while (token.type == ID) {
    constDecl = new ConstDecl ();
    constDecl->id = token.value.svalue;
    scan ();    // ID
    if (first == NULL) {
      first = constDecl;
    } else {
      last->next = constDecl;
    }
    last = constDecl;
    // Pick up the Expression
    mustHave (EQUAL, "Expecting '= Expr' in const decl");
    constDecl->expr = parseExpr ("In constant initializing expression");
  }
  return first;
}



// parseEnums ()
//
// This routine parses the following syntax:
//     Enum --> 'enum' ID [ = Expr ] { , ID }
//
// It returns a linked list of ConstDecls.
//
ConstDecl * parseEnums () {
  ConstDecl * first, * last, * constDecl;
  int counter, gotInitializer;
  Expression * expr, * junk;
  IntConst * intConst;
  SendExpr * sendExpr;
  Argument * arg;
  VariableExpr * var;
  if (token.type != ENUM) {
    programLogicError ("Already checked for 'enum' keyword");
  }
  first = last = NULL;
  scan ();    // ENUM
  constDecl = new ConstDecl ();
  constDecl->id = mustHaveID ("Expecting ID after 'ENUM'");

  // If there is an expression, pick it up; Else the default is "= 1".
  counter = 1;
  if (token.type == EQUAL) {
    scan ();
    constDecl->expr = parseExpr ("In initializing expression after 'ENUM ID ='");
    gotInitializer = 1;
  } else {
    intConst = new IntConst ();
    intConst->positionAt (constDecl);
    intConst->ivalue = 1;
    constDecl->expr = intConst;
    gotInitializer = 0;
  }
  first = last = constDecl;
  while (token.type == COMMA) {
    scan ();    // COMMA
    constDecl = new ConstDecl ();
    constDecl->id = mustHaveID ("Expecting next ID in 'enum ID, ID, ...'");
    // There should not be "= Expr" here...
    if (token.type == EQUAL) {
      syntaxError ("An initial expression is only allowed for the first 'enum ID'");
      scan ();
      junk = parseExpr ("In ENUM initializing expression");
    }
    last->next = constDecl;
    last = constDecl;
    // If we had an initializing expression, then create "firstID + N"...
    if (gotInitializer) {
      var = new VariableExpr ();
      var->positionAt (constDecl);
      var->id = first->id;
      intConst = new IntConst ();
      intConst->positionAt (constDecl);
      intConst->ivalue = counter;
      arg = new Argument ();
      arg->positionAt (constDecl);
      arg->expr = intConst;
      sendExpr = new SendExpr ();
      sendExpr->positionAt (constDecl);
      sendExpr->receiver = var;
      sendExpr->selector = lookupAndAdd ("+", OPERATOR);
      sendExpr->argList = arg;
      sendExpr->kind = INFIX;
      constDecl->expr = sendExpr;
      counter++;
    } else {    // Else, if no initial expression, then create an integerConst...
      counter++;
      intConst = new IntConst ();
      intConst->positionAt (constDecl);
      intConst->ivalue = counter;
      constDecl->expr = intConst;
    }
  }
  return first;
}



// parseTypeDefs ()
//
// This routine parses the following syntax:
//     TypeDef --> 'type' { ID = Type }+
//
// It returns a linked list of TypeDefs.
//
TypeDef * parseTypeDefs () {
  TypeDef * first, * last, * typeDef;
  if (token.type != TYPE) {
    programLogicError ("Already checked for 'type' keyword");
  }
  first = last = NULL;
  scan ();    // TYPE
  if (token.type != ID) {
    syntaxError ("Expecting ID after 'type'");
    return NULL;
  }
  while (token.type == ID) {
    typeDef = new TypeDef ();
    typeDef->id = token.value.svalue;
    scan ();    // ID
    if (first == NULL) {
      first = typeDef;
    } else {
      last->next = typeDef;
    }
    last = typeDef;
    // Pick up the Type
    mustHave (EQUAL, "Expecting '= Type' in type definition");
    typeDef->type = parseType ("Expecting Type after '=' in type definition");
  }
  return first;
}

