/* The BLITZ Linker
**
** Copyright 2000-2007, Harry H. Porter III
**
** This file may be freely copied, modified and compiled, on the sole
** conditions that if you modify it...
**   (1) Your name and the date of modification is added to this comment
**       under "Modifications by", and
**   (2) Your name and the date of modification is added to the printHelp()
**       routine under "Modifications by".
**
** Original Author:
**   12/29/00 - Harry H. Porter III
**
** Modifcations by:
**   03/15/06 - Harry H. Porter III
**   04/27/07 - Harry H. Porter III - Support for little endian added
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


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



typedef struct FileInfo FileInfo;
typedef struct RelocInfo RelocInfo;
typedef struct TableEntry TableEntry;
typedef struct LabelEntry LabelEntry;



/*****  Global variables *****/

int errorsDetected = 0;          /* The number of errors detected so far */
int warningsDetected = 0;        /* The number of warnings detected so far */
int commandOptionS = 0;          /* True: print the symbol table */
int commandOptionL = 0;          /* True: print the listing */
int pageSize = 8192;             /* Page Size (default = 8K) */
int loadAddr = 0;                /* Addr at which program will be loaded */
char * commandOutFileName = NULL;/* The a.out filename */
FILE * inputFile;                /* The .o input file */
FILE * outputFile;               /* The a.out output file */
FileInfo * inputFileList;        /* Ptr to linked list of FileInfo's */
int textStartAddr;               /* Starting addr of .text segment */
int dataStartAddr;               /* Starting addr of .data segment */
int bssStartAddr;                /* Starting addr of .bss segment */
int totalTextSize;               /* Sum of all .text segment sizes */
int totalDataSize;               /* Sum of all .data segment sizes */
int totalBssSize;                /* Sum of all .bss segment sizes */
char * textSegment;              /* All the bytes of the segment */
char * dataSegment;              /* All the bytes of the segment */
char * pmPtr;
int pmSize;
int pmCount;
int pmRow[16];
int numberOfEntries = 0;         /* Count of files containing _entry */
TableEntry * absoluteEntry;      /* Ptr to table entry for .absolute */



/*****  FileInfo  *****
**
** There is one of the following structures for each input .o file and
** they are kept in a linked list, headed by inputFileList.
*/
struct FileInfo {
  FileInfo *    next;
  char *        filename;
  FILE *        filePtr;
  int           containsEntry;
  int           sizeOfText;
  int           sizeOfData;
  int           sizeOfBss;
  int           textAddr;
  int           dataAddr;
  int           bssAddr;
  RelocInfo *   relocList;
};



/*****  RelocInfo  *****
**
** There is one of the following structures for each field that needs
** to be updated.  Each FileInfo contains a pointer to a linked list
** of these structures.
*/
struct RelocInfo {
  int           type;
  int           locationToUpdate;
  int           inText;
  int           offset;
  TableEntry *  relativeTo;
  int           sourceLineNumber;
  RelocInfo *   next;
};



/*****  Symbol Table  *****
**
** There is one of TableEntry structure for every symbol.  These structures
** are kept in a hash table.  The hash table itself is an array of pointer
** to linked lists of TableEntry structures, which are linked on "next".
*/
struct TableEntry {
  TableEntry * next;        /* Link list of TableEntry's */
  char *       filename;    /* File in which this symbol occurs (for errors) */
  int          value;       /* The value of this symbol */
  int          relativeToS; /* The index of another symbol */
  TableEntry * relativeToT; /* Ptr to the TableEntry for that symbol */
  int          length;      /* The number of chars in the symbol */
  char         string [0];  /* The characters in the symbol */
};

#define SYMBOL_TABLE_HASH_SIZE 2999    /* Size of hash table for sym table */

static TableEntry * symbolTableIndex [SYMBOL_TABLE_HASH_SIZE];

/* The following table maps symbol numbers to TableEntries.  It is
** filled once per input .o file, and re-used for each other file.
** In this table, certain numbers have meaning:
**    0:    There is no entry in position 0; 0=imported
**    1:    .text
**    2:    .data
**    3:    .bss
**    4:    .absolute
**    5...  (other entries, through maxSymbolNumber)
*/

#define MAX_NUMBER_OF_SYMBOLS 500000

TableEntry * tableArray [MAX_NUMBER_OF_SYMBOLS];

int maxSymbolNumber;       /* The index of the last valid entry. */



/*****  Label Table  *****
**
** There is one of LabelEntry structure for every label.  These structures
** are kept in a hash table.  The hash table itself is an array of pointer
** to linked lists of LabelEntry structures, which are linked on "next".
*/
struct LabelEntry {
  LabelEntry * next;        /* Link list of LabelEntry's */
  int          value;       /* The value of this label */
  int          length;      /* The number of characters in the label */
  char         string [0];  /* The characters in the label */
};

#define LABEL_TABLE_HASH_SIZE 211    /* Size of hash table for label table */

static LabelEntry * labelTableIndex [LABEL_TABLE_HASH_SIZE];



/*****  Function prototypes  *****/

int main (int argc, char ** argv);
void checkHostCompatibility ();
void writeInteger (int i);
void processCommandLine (int argc, char ** argv);
void badOption (char * msg);
void errorExit ();
void printErrorAndExit (FileInfo * file, char * msg);
void fatalError (char * msg);
void printHelp ();
int readInteger (FileInfo * file);
int readByte (FileInfo * file);
int roundUpToMultipleOf (int i, int p);
void printMemory (char * ptr, int n);
void get16Bytes ();
int getNextByte ();
void putlong (int i);
void printline ();
void printByte (int c);
int bytesEqual (char * p, char * q, int lengthP, int lengthQ);
void printSymbolTable ();
void printSymbol (TableEntry *, int fieldWidth);
void printSymbolOnFile (FILE * fp, TableEntry *, int fieldWidth);
void printTableArray ();
TableEntry * lookup (TableEntry * givenEntry);
void resolveSymbols ();
void performRelocations ();
int get8 (char * ptr);
int get16 (char * ptr);
int get24 (char * ptr);
int get32 (char * ptr);
void put8 (char * ptr, int value);
void put16 (char * ptr, int value);
void put24 (char * ptr, int value);
void put32 (char * ptr, int value);
void printRelocationHeader ();
void printRelocationRecord (RelocInfo * rel);
void readRelocationRecords (FileInfo * file);
void addLabels ();
int lookupLabel (LabelEntry * newEntry);
void writeLabels ();



/* main()
**
** Read through all input files and produce the output file.
*/
main (int argc, char ** argv) {
    FileInfo * inFile, * fileWithEntry;
    int magic, i, j, unpaddedTextSize, unpaddedDataSize;
    int nextText, nextData, nextBss, inText;
    char * targetAddr, *q;
    int symbolNum, value, relativeToS, len;
    TableEntry * entryPtr, * existingEntry;
    RelocInfo * rel;

    inputFileList = NULL;
    checkHostCompatibility ();
    processCommandLine (argc, argv);
    totalTextSize = 0;
    totalDataSize = 0;
    totalBssSize = 0;
    if (commandOptionL) {
      printf ("==========  Computing Segment Sizes  ==========\n");
      printf (" pageSize = %08x (%d)\n", pageSize, pageSize);
      printf (" loadAddr = %08x (%d)\n", loadAddr, loadAddr);
    }

    /* Each execution of this loop reads from the next .o file. */
    if (commandOptionL) {
      printf (" Scanning files to pickup chunk sizes...\n");
    }
    for (inFile=inputFileList;
         inFile!=NULL;
         inFile=inFile->next) {
      magic = readInteger (inFile);
      if (magic != 0x424C5A6F) {   /* "BLZo" in hex */
        printErrorAndExit (inFile,
           "File is not a .o file (magic number is incorrect)");
      }
      /* Pick up the header info and add it to this FileInfo record. */
      inFile->containsEntry = readInteger (inFile);
      if (inFile->containsEntry == 0) {
      } else if (inFile->containsEntry == 1) {
        numberOfEntries++;
        fileWithEntry = inFile;
      } else {
        printErrorAndExit (inFile, "ContainsEntry is incorrect");
      }
      inFile->sizeOfText = readInteger (inFile);
      inFile->sizeOfData = readInteger (inFile);
      inFile->sizeOfBss = readInteger (inFile);
      /* Add these .text, .data, and .bss chunk sizes to the totals. */
      totalTextSize += roundUpToMultipleOf (inFile->sizeOfText, 4);
      totalDataSize += roundUpToMultipleOf (inFile->sizeOfData, 4);
      totalBssSize += roundUpToMultipleOf (inFile->sizeOfBss, 4);
    }

    /* Check that only one file contains "_entry". */
    if (numberOfEntries < 1) {
      fatalError ("No input file contains \"_entry\"");
    } else if (numberOfEntries > 1) {
      fatalError ("More than one input file contains \"_entry\"");
    }
    /* If that file is not already at the head of the list, put it there. */
    if (inputFileList != fileWithEntry) {
      for (inFile=inputFileList;
           inFile->next != fileWithEntry;
           inFile=inFile->next) {
      }
      inFile->next = inFile->next->next;
      fileWithEntry->next = inputFileList;
      inputFileList = fileWithEntry;
    }

    /* Compute the sizes of the segments. */
    if (commandOptionL) {
      printf (" Computing sizes of the segments...\n");
      printf ("  totalTextSize = %08x (%d)\n", totalTextSize, totalTextSize);
      printf ("  totalDataSize = %08x (%d)\n", totalDataSize, totalDataSize);
      printf ("  totalBssSize =  %08x (%d)\n", totalBssSize,  totalBssSize);
      printf (" Rounding segments up to a multiple of pageSize...\n");
    }
    unpaddedTextSize = totalTextSize;
    unpaddedDataSize = totalDataSize;
    totalTextSize = roundUpToMultipleOf (totalTextSize, pageSize);
    totalDataSize = roundUpToMultipleOf (totalDataSize, pageSize);
    totalBssSize = roundUpToMultipleOf (totalBssSize, pageSize);
    if (commandOptionL) {
      printf ("  totalTextSize = %08x (%d)\n", totalTextSize, totalTextSize);
      printf ("  totalDataSize = %08x (%d)\n", totalDataSize, totalDataSize);
      printf ("  totalBssSize =  %08x (%d)\n", totalBssSize,  totalBssSize);
    }
    i = totalTextSize + totalDataSize + totalBssSize;
    if ((i < 0) || (i > 16777216)) {
      warningsDetected++;
      fprintf (stderr,
        "BLITZ Linker Warning: The size of .text, .data, and .bss (0x%08x) exceeds 16MB limit.\n",
        i);
    }

    /* Allocate memory chunks to hold the .text and .data segments. */
    if (commandOptionL) {
      printf (" Allocating memory for text and data segments...\n");
    }
    textSegment = (char *) calloc (totalTextSize, 1);
    dataSegment = (char *) calloc (totalDataSize, 1);
    if ((textSegment == NULL) || (dataSegment == NULL)) {
      fatalError ("Calloc failed - insufficient memory available");
    }
    if (commandOptionL) {
      printf ("  Memory addr of text segment = %08x (%d)\n",
                                           textSegment, textSegment);
      printf ("  Memory addr of data segment = %08x (%d)\n",
                                           dataSegment, dataSegment);
    }

    /* Compute the starting addresses of the segments. */
    if (commandOptionL) {
      printf (" Computing starting addresses ...\n");
    }
    textStartAddr = loadAddr;
    dataStartAddr = textStartAddr + totalTextSize;
    bssStartAddr = dataStartAddr + totalDataSize;
    if (commandOptionL) {
      printf ("  textStartAddr = %08x (%d)\n", textStartAddr, textStartAddr);
      printf ("  dataStartAddr = %08x (%d)\n", dataStartAddr, dataStartAddr);
      printf ("  bssStartAddr  = %08x (%d)\n\n", bssStartAddr, bssStartAddr);
    }
    nextText = textStartAddr;
    nextData = dataStartAddr;
    nextBss = bssStartAddr;
    
    /* Run through each file again. */
    for (inFile=inputFileList;
         inFile!=NULL;
         inFile=inFile->next) {
      if (commandOptionL) {
        printf ("==========  Processing file %s  ==========\n",
                                                     inFile->filename);
        printf (" contains entry = %d\n", inFile->containsEntry);
      }

      /* Compute the memory location for this .text, .data, and .bss chunk. */
      inFile->textAddr = nextText;
      inFile->dataAddr = nextData;
      inFile->bssAddr = nextBss;
      nextText += roundUpToMultipleOf (inFile->sizeOfText, 4);
      nextData += roundUpToMultipleOf (inFile->sizeOfData, 4);
      nextBss += roundUpToMultipleOf (inFile->sizeOfBss, 4);
      if (commandOptionL) {
        printf (" textAddr       = %08x (%d)\n", inFile->textAddr,
                                                 inFile->textAddr);
        printf (" dataAddr       = %08x (%d)\n", inFile->dataAddr,
                                                 inFile->dataAddr);
        printf (" bssAddr        = %08x (%d)\n", inFile->bssAddr,
                                                 inFile->bssAddr);
        printf (" size of text   = %08x (%d)\n", inFile->sizeOfText,
                                                 inFile->sizeOfText);
        printf (" size of data   = %08x (%d)\n", inFile->sizeOfData,
                                                 inFile->sizeOfData);
        printf (" size of bss    = %08x (%d)\n", inFile->sizeOfBss,
                                                 inFile->sizeOfBss);
      }

      /* Move this .text chunk into the right spot in the in-memory segment. */
      if (commandOptionL) {
        printf (" Reading text chunk into memory...\n");
      }
      /***  printMemory (textSegment, unpaddedTextSize);  ***/
      targetAddr = textSegment + (inFile->textAddr - textStartAddr);
      i = fread (targetAddr, 1, inFile->sizeOfText, inFile->filePtr);
      if (commandOptionL) {
        printf ("    memory addr = %08x (%d)\n", targetAddr, targetAddr);
        printf ("    bytes read  = %08x (%d)\n", i, i);
      }
      if (i != inFile->sizeOfText) {
        printErrorAndExit (inFile,
                           "Problems reading text segment from .o file");
      }
      magic = readInteger (inFile);
      if (magic != 0x2a2a2a2a) {
        printErrorAndExit (inFile, "Invalid **** separator");
      }
      /***  printMemory (textSegment, unpaddedTextSize);  ***/

      /* Move this data chunk into the right spot in the in-memory segment. */
      if (commandOptionL) {
        printf (" Reading data chunk into memory...\n");
      }
      /***  printMemory (dataSegment, unpaddedDataSize);  ***/
      targetAddr = dataSegment + (inFile->dataAddr - dataStartAddr);
      i = fread (targetAddr, 1, inFile->sizeOfData, inFile->filePtr);
      if (commandOptionL) {
        printf ("    memory addr = %08x (%d)\n", targetAddr, targetAddr);
        printf ("    bytes read  = %08x (%d)\n", i, i);
      }
      if (i != inFile->sizeOfData) {
        printErrorAndExit (inFile,
                           "Problems reading data segment from .o file");
      }
      magic = readInteger (inFile);
      if (magic != 0x2a2a2a2a) {
        printErrorAndExit (inFile, "Invalid **** separator");
      }
      /***  printMemory (dataSegment, unpaddedDataSize);  ***/

      /* Run through the symbols in this .o file.  Each iteration reads
         in one symbol and processes it. */
      maxSymbolNumber = 0;
      while (1) {

        /* Read in symbolNumber, value, relativeTo, and length. */
        symbolNum = readInteger (inFile);
        if (symbolNum == 0) break;
        maxSymbolNumber++;
        if (maxSymbolNumber >= MAX_NUMBER_OF_SYMBOLS) {
          printErrorAndExit (inFile, "The linker can handle only a limited number of symbols and this file has more than that");
        }
        if (maxSymbolNumber != symbolNum) {
          printErrorAndExit (inFile, "Invalid symbol number");
        }
        value = readInteger (inFile);
        relativeToS = readInteger (inFile);
        len = readInteger (inFile);

        /* Create a TableEntry and initialize it. */
        entryPtr = (TableEntry *) calloc (1, sizeof (TableEntry) + len + 1);
        if (entryPtr == 0) {
          fatalError ("Calloc failed - insufficient memory available");
        }
        tableArray [symbolNum] = entryPtr;
        entryPtr->length = len;
        entryPtr->value = value;
        entryPtr->relativeToS = relativeToS;
        entryPtr->relativeToT = NULL;
        entryPtr->filename = inFile->filename;

        /* Read in the characters and move into this TableEntry. */
        /*** printf ("Processing "); ***/
        for (q=entryPtr->string, j=len;
             j>0;
             q++, j--) {
          *q = readByte (inFile);
          /*** printf ("%c", *q); ***/
        }
        /*** printf ("...  "); ***/

        /* Look this string up in the symbol table. */
        existingEntry = lookup (entryPtr);
        if (existingEntry == NULL) {    /* if not found... */
          /* ...then it will have gotten added to the symbol table. */
          /*** printf ("Not found.  It was added.\n"); ***/
          /* Fill in values for .text, .data, .bss; save ptr to .absolute. */
          if (symbolNum == 1) {
            entryPtr->value = textStartAddr;
            entryPtr->relativeToS = 4;
          } else if (symbolNum == 2) {
            entryPtr->value = dataStartAddr;
            entryPtr->relativeToS = 4;
          } else if (symbolNum == 3) {
            entryPtr->value = bssStartAddr;
            entryPtr->relativeToS = 4;
          } else if (symbolNum == 4) {
            absoluteEntry = entryPtr;
          }

        /* If found in the table... */
        } else {
          /*** printf ("Found.\n"); ***/
          /* Fill in values for .text, .data, .bss; and ignore .absolute. */
          if (symbolNum == 1) {
            entryPtr->value = inFile->textAddr;
            entryPtr->relativeToS = 4;
          } else if (symbolNum == 2) {
            entryPtr->value = inFile->dataAddr;
            entryPtr->relativeToS = 4;
          } else if (symbolNum == 3) {
            entryPtr->value = inFile->bssAddr;
            entryPtr->relativeToS = 4;
          } else if (symbolNum == 4) {
          } else {  /* i.e., for all other symbols... */
            /* If existing was imported, grab value from new. */
            if (existingEntry->relativeToT == NULL) {
              /*** printf ("  Existing entry is imported.\n"); ***/
              existingEntry->value = entryPtr->value;
              existingEntry->relativeToS = entryPtr->relativeToS;
            } else {
              /*** printf ("  Existing entry has a value.\n"); ***/
              /* If the new entry is also not imported, it is an error. */
              if (entryPtr->relativeToS != 0) {
                fprintf (stderr, "BLITZ Linker Error: The symbol \"");
                printSymbolOnFile (stderr, entryPtr, 1);
                fprintf (stderr,
                         "\" is defined and exported from more than one file, including file \"%s\"\n",
                         inFile->filename);
                errorsDetected++;
              }
            }
            /* From now on use the existing entry. */
            tableArray [symbolNum] = existingEntry;
          }
        }
      }

      /* Read in the magic number after the symbols. */
      magic = readInteger (inFile);
      if (magic != 0x2a2a2a2a) {   /* "****" in hex */
        printErrorAndExit (inFile, "Invalid **** separator");
      }

      /*** printTableArray (); ***/
      fflush (stdout);

      /* Run through the new chunk's symbols.  In the .o file, each symbol's
         relativeTo specified another symbol by number.  (This integer was
         stored in the "relativeToS" field in the TableEntry.  Each iteration
         of this loop will set the relativeTo field (or, more precisely, the
         "relativeToT" field) of the symbol to point directly the TableEntry
         of the other symbol.  */
      if (commandOptionL) {
        printf (" Processing this file's symbols...\n");
      }
      for (i=1; i<=maxSymbolNumber; i++) {
        entryPtr = tableArray [i];
        j = entryPtr->relativeToS;
        if (j<=0) {
        } else if (j<=3) {
          entryPtr->value += tableArray[j] -> value;
          entryPtr->relativeToT = absoluteEntry;
          entryPtr->relativeToS = 4;
        } else if (j==4) {
          entryPtr->relativeToT = absoluteEntry;
        } else {
          entryPtr->relativeToT = tableArray [j];
          entryPtr->relativeToS = 0;
        }
      }

      /*** printTableArray (); ***/

      /* Read in all the relocation records for this file and add each
         to the linked list of RelocInfo structs for this FileInfo. */
      readRelocationRecords (inFile);

      /* Read in the magic number after the relocation records. */
      magic = readInteger (inFile);
      if (magic != 0x2a2a2a2a) {   /* "****" in hex */
        printErrorAndExit (inFile, "Invalid **** separator");
      }

      /* Run through this file's relocation records and print them. */
      if (commandOptionL) {
        printf ("    Here are the relocation records...\n");
        printRelocationHeader ();
        for (rel=inFile->relocList; rel!=NULL; rel=rel->next) {
          printRelocationRecord (rel);
        }
        printf ("\n");
      }

      /* Done with this file */

    }
    /* Done with all files. */

    /* Print the symbol table, if "-l" option was used. */
    if (commandOptionL) {
      printSymbolTable ();
      printf ("\n");
    }

    /* Run through each symbol in the symbol table; compute and fill in
       its absolute value. */
    if (commandOptionL) {
      printf ("Resolving symbols...\n");
    }
    resolveSymbols ();

    /* Print the symbol table again, the time if "-s" option was used. */
    if (commandOptionS) {
      printf ("\n");
      printSymbolTable ();
      printf ("\n");
    }

    /* Perform the relocations */
    if (commandOptionL) {
      printf ("Performing the relocations...\n");
    }
    performRelocations ();

    /* Run through the files again and add the labels to the label table. */
    if (commandOptionL) {
      printf ("Processing Labels...\n");
    }
    addLabels ();

    if (errorsDetected) {
      fprintf (stderr, "BLITZ Linker: %d errors were detected!\n",
                                                         errorsDetected);
      if (commandOptionL) {
        printf ("%d warnings were detected.\n", warningsDetected);
        printf ("%d errors were detected.\n", errorsDetected);
      }
      errorExit ();
    }

    /* Write the magic number to the a.out file. */
    writeInteger (0x424C5A78);   /* "BLZx" as an integer */

    /* Write header info. */
    writeInteger (totalTextSize);
    writeInteger (totalDataSize);
    writeInteger (totalBssSize);
    writeInteger (textStartAddr);
    writeInteger (dataStartAddr);
    writeInteger (bssStartAddr);
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write out the .text segment. */
    i = fwrite (textSegment, 1, totalTextSize, outputFile);
    if (i != totalTextSize) {
      fatalError ("Error while writing to output executable file");
    }
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write out the .data segment. */
    i = fwrite (dataSegment, 1, totalDataSize, outputFile);
    if (i != totalDataSize) {
      fatalError ("Error while writing to output executable file");
    }
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    /* Write the Label info out to the a.out file. */
    writeLabels ();
    writeInteger (0x02a2a2a2a);   /* "****" as an integer */

    if (warningsDetected) {
      fprintf (stderr, "BLITZ Linker: %d warnings were issued!\n",
                                                      warningsDetected);
    }
    if (errorsDetected) {
      fprintf (stderr, "BLITZ Linker: %d errors were detected!\n",
                                                      errorsDetected);
      errorExit ();
    }
    if (commandOptionL) {
      printf ("%d warnings were detected.\n", warningsDetected);
      printf ("%d errors were detected.\n", errorsDetected);
    }
    exit (0);
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
    fatalError ("There is a big/little endian byte ordering problem.");
  }

  // Check that we have at least 4 bytes of precision.
  i = 0x00000001;
  i <<= 20;
  i <<= 10;
  i >>= 20;
  i >>= 10;
  if (i != 0x00000001) {
    fatalError ("This program only runs on computers with 4 byte integers - 1");
  }

  // Check that we have no more than 4 bytes of precision.
  i = 0x00000001;
  i <<= 20;
  i <<= 13;   // Some compilers treat <<33 as a nop!
  i >>= 20;
  i >>= 13;
  if (i != 0x00000000) {
    fatalError ("This program only runs on computers with 4 byte integers - 2");
  }

  // Check that we have the expected overflow behavior for ints.
  i = -2147483647;
  i = i - 2;
  if (i != 2147483647) {
    fatalError ("This program only runs on computers with 4 byte integers - 3");
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
    fatalError ("There is a big/little endian byte ordering problem with doubles - 1.");
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
    fatalError ("There is a big/little endian byte ordering problem with doubles - 2.");
#endif

  // Else, if doubles are stored in some other way...
  } else {
    fatalError ("The host implementation of 'double' is not what I expect.");
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
    fatalError ("The host implementation of double->int casting is not what I expect.");
  }

}



/* writeInteger (i)
**
** This routine writes out 4 bytes to the .o file.
*/
void writeInteger (int i) {
  int j = SWAP_BYTES (i);
  fwrite (&j, 4, 1, outputFile);
}



/* processCommandLine (argc, argv)
**
** This routine processes the command line options.
*/
void processCommandLine (int argc, char ** argv) {
  int argCount;
  int len;
  int gotLoadAddr = 0;
  int gotPageSize = 0;
  FileInfo * p, * q;

  /* Each iteration of this loop looks at the next argument.  In some
     cases (like "-o a.out") one iteration will scan two tokens. */
  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
    argCount = 1;
    /* Scan the -h option */
    if (!strcmp (*argv, "-h")) {
      printHelp ();
      exit (1);
    /* Scan the -s option */
    } else if (!strcmp (*argv, "-s")) {
      commandOptionS = 1;
    /* Scan the -l option */
    } else if (!strcmp (*argv, "-l")) {
      commandOptionL = 1;
    /* Scan the -p option */
    } else if (!strcmp (*argv, "-p")) {
      gotPageSize = 1;
      if (argc <= 1) {
        badOption ("Expecting integer after -p option");
      } else {
        argCount++;
        pageSize = atoi (*(argv+1));  /* Extra chars after int are ignored */
        if ((pageSize <= 0) || (pageSize % 4 != 0)) {
          badOption ("Invalid integer after -p option");
        }
      }
    /* Scan the -a option */
    } else if (!strcmp (*argv, "-a")) {
      gotLoadAddr = 1;
      if (argc <= 1) {
        badOption ("Expecting integer after -a option");
      } else {
        argCount++;
        loadAddr = atoi (*(argv+1));  /* Extra chars after int are ignored */
        if (loadAddr < 0) {
          badOption ("Invalid integer after -a option");
        }
      }
    /* Scan the -o option, which should be followed by a file name */
    } else if (!strcmp (*argv, "-o")) {
      if (argc <= 1) {
        badOption ("Expecting filename after -o option");
      } else {
        argCount++;
        if (commandOutFileName == NULL) {
          commandOutFileName = *(argv+1);
        } else {
          badOption ("Invalid command line - multiple output files");
        }
      }
    /* Deal with any invalid options that begin with "-", such as "-qwerty" */
    } else if ((*argv)[0] == '-') {
      fprintf (
        stderr,
        "BLITZ Linker Error: Invalid command line option (%s); Use -h for help display\n",
        *argv);
      exit (1);
    /* This token will be interpreted as an input file name. */
    } else {
      /* Create a FileInfo and chain it into the linked list. */
      p = (FileInfo *) calloc (1, sizeof (FileInfo));
      if (p == NULL) {
        fprintf (stderr, "BLITZ Linker Error: Calloc failed - insufficient memory available.\n");
        exit (1);
      }
      p->filename = *argv;
      /* Open this .o file for reading. */
      p->filePtr = fopen (p->filename, "r");
      if (p->filePtr == NULL) {
        fprintf (stderr,
                 "BLITZ Linker Error: Input file \"%s\" could not be opened\n",
                 p->filename);
        exit (1);
      }
      p->next = inputFileList;
      inputFileList = p;
    }
  }

  /* If there were no input files, then abort. */
  if (inputFileList == NULL) {
    badOption ("There must be at least one .o input file");
  }

  /* Make sure the starting address is a multiple of the pageSize */
  if (loadAddr % pageSize != 0) {
    badOption ("The load address must be a multiple of the page size");
  }

  /* The list of input file is in reverse order.  Reverse it back. */
  q = inputFileList;
  inputFileList = NULL;
  while (q != NULL) {
    p = q;
    q = q->next;
    p->next = inputFileList;
    inputFileList = p;
  }

  /* Figure out the name of the a.out file. */
  if (commandOutFileName == NULL) {
    commandOutFileName = "a.out";
  }

  /* Open the output (a.out) file. */
  outputFile = fopen (commandOutFileName, "wa");
  if (outputFile == NULL) {
    fprintf (stderr,
             "BLITZ Linker Error: Output file \"%s\" could not be opened\n",
             commandOutFileName);
    exit (1);
  }
}



/* badOption (msg)
**
** This routine prints a message on stderr and then aborts this program.
*/
void badOption (char * msg) {
  fprintf (stderr, "BLITZ Linker Error: %s;  Use -h for help display\n", msg);
  exit (1);
}



/* printErrorAndExit (file, str)
**
** This routine prints a message of the form:
**    Problem with file "xxx.o": yyy
** It then calls errorExit() to abort.
*/
void printErrorAndExit (FileInfo * file, char * msg) {
  fprintf (stderr,
           "BLITZ Linker Error: Problem with file \"%s\"; %s\n",
           file->filename, msg);
  errorExit ();
}



/* fatalError (msg)
**
** This routine prints the given error message on stderr and calls errorExit().
*/
void fatalError (char * msg) {
  fprintf (stderr, "BLITZ Linker Error: %s\n", msg);
  errorExit ();
}



/* errorExit ()
**
** This routine removes the output (a.out) file and calls exit(1).
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
"==============================\n"
"=====                    =====\n"
"=====  The BLITZ Linker  =====\n"
"=====                    =====\n"
"==============================\n"
"\n"
"Copyright 2000-2007, Harry H. Porter III\n"
"========================================\n"
"  Original Author:\n"
"    12/29/00 - Harry H. Porter III\n"
"  Modifcations by:\n"
"    03/15/06 - Harry H. Porter III\n"
"    04/27/07 - Harry H. Porter III - Support for little endian added\n"
"\n"
"Command Line Options\n"
"====================\n"
"  These command line options may be given in any order.\n"
"    filename1 filename2 filename3 ...\n"
"       The input object files, which will normally end with \".o\".\n"
"       There must be at least one input file.\n"
"    -h\n"
"       Print this help info.  Ignore other options and exit.\n"
"    -o filename\n"
"       If there are no errors, an executable file will be created.  This\n"
"       option can be used to give the object file a specific name.\n"
"       If this option is not used, then the output file will be named\n"
"       \"a.out\".\n"
"    -l\n"
"       Print a listing on stdout.\n"
"    -s\n"
"       Print the symbol table on stdout.\n"
"    -p integer\n"
"       The page size.  The integer must be a multiple of 4 greater than\n"
"       zero.  (The default is 8192 = 8K.)\n"
"    -a integer\n"
"       The logical address at which to load the program at run-time.\n"
"       The integer must be a non-negative multiple of the page size.\n"
"       (The default is 0.)\n");
}



/* readInteger (file)
**
** Read a integer (4-bytes, binary) from one of the .o files and return it.
*/
int readInteger (FileInfo * file) {
  int i, numItemsRead;
  numItemsRead = fread (&i, 4, 1, file->filePtr);
  if (numItemsRead != 1) {
    printErrorAndExit (file, "Problem reading from object file");
  }
  return SWAP_BYTES (i);
}



/* readByte (file)
**
** Read 1 byte from one of the .o files and return it as an integer.
*/
int readByte (FileInfo * file) {
  int i, numBytesRead;
  char c;
  numBytesRead = fread (&c, 1, 1, file->filePtr);
  if (numBytesRead != 1) {
    printErrorAndExit (file, "Problem reading from object file");
  }
  i = c;
  return i;
}



/* roundUpToMultipleOf (i, p)
**
** This routine rounds i up to a multiple of p and returns the result.
*/
int roundUpToMultipleOf (int i, int p) {
  if ((i < 0) || (p <= 0)) {
    fatalError ("PROGRAM LOGIC ERROR - Bad arg in roundUpToMultipleOf()");
  }
  if (i % p > 0) {
    return (i / p + 1) * p;
  }
  return i;
}



/* printMemory (ptr, n)
**
** This routine dumps n bytes of memory (located at "ptr") in hex.
*/
void printMemory (char * ptr, int n) {
   int addr;
   pmCount = n;
   pmPtr = ptr;
   addr = (int) ptr;

   /* Each execution of this loop prints a single output line. */
   get16Bytes ();
   while (pmSize > 0) {
     putlong (addr);
     printf (":  ");
     printline ();
     addr = addr + 16;
     get16Bytes ();
   }

}



/* get16Bytes ()
**
** This routine reads in the next 16 bytes from memory and
** places them in the array named "pmRow", setting "pmSize" to
** be the number of bytes moved.  "pmSize" will be less than
** 16 if EOF was encountered, and may possibly be 0.
*/
void get16Bytes () {
  int c;
  pmSize = 0;
  c = getNextByte ();
  while (c != -999) {
    pmRow [pmSize++] = c;
    if (pmSize >= 16) break;
    c = getNextByte ();
  }
}



/* getNextByte () 
**
** Get the next byte from memory, from the location pointed to by
** "pmPtr", incrementing "pmPtr" and decrementing the counter "pmCount."
** This routine will return it as an integer, or return -999 if pmCount <= 0.
*/
int getNextByte () {
  if (pmCount <= 0) return -999;
  pmCount--;
  return *(pmPtr++);
}



/* putlong (i)
**
** This routine is passed an integer, which it prints as 8 hex digits.
*/
void putlong (int i) {
  printByte ((i>>24) & 0x000000ff);
  printByte ((i>>16) & 0x000000ff);
  printByte ((i>>8) & 0x000000ff);
  printByte ((i>>0) & 0x000000ff);
}



/* printline ()
**
** This routine prints the current 'pmRow'.
*/
void printline () {
  int i, c;
  if (pmSize > 0) {
    i = 0;
    while (i<16) {
      if (i < pmSize) {
        printByte (pmRow[i]);
      } else {
        printf ("  ");
      }
      i++;
      if ((i%2) == 0) {
        putchar (' ');
      }
      if ((i%4) == 0) {
        putchar (' ');
      }
    }
    printf ("  ");
    for (i=0; i<pmSize; i++) {
      c = pmRow[i];
      if ((c>=' ') && (c <= '~')) {
        putchar (c);
      } else {
        putchar ('.');
      }
    }
    printf ("\n");
  }
}



/* printByte (c)
**
** This routine is passed a byte (i.e., an integer -128..255) which
** it prints as 2 hex characters.  If passed a number out of that
** range, it outputs nothing.
*/
void printByte (int c) {
  int i;
  if (c<0) c = c + 256;
  if ((c >= 0) && (c <= 255)) {
    i = (c & 0x000000f0) >> 4;
    if (i < 10) {
      putchar ('0' + i);
    } else {
      putchar ('A' + i - 10);
    }
    i = (c & 0x0000000f) >> 0;
    if (i < 10) {
      putchar ('0' + i);
    } else {
      putchar ('A' + i - 10);
    }
  }
}



/* bytesEqual (p, q, lengthP, lengthQ)
**
** This function is passed two pointers to blocks of bytes, and a
** length.  It compares the two sequences of bytes and returns true iff
** they are both equal.
*/
int bytesEqual (char * p, char * q, int lengthP, int lengthQ) {
  if (lengthP != lengthQ) return 0;
  for (; lengthP>0; lengthP--, p++, q++) {
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
  printf ("SYMBOL                                   VALUE     (decimal) RELATIVE-TO\n");
  printf ("======================================== =================== ===========\n");
  for (hashVal = 0; hashVal<SYMBOL_TABLE_HASH_SIZE; hashVal++) {
    for (entryPtr = symbolTableIndex [hashVal];
                       entryPtr;
                       entryPtr = entryPtr->next) {
      printSymbol (entryPtr, 40);
      printf (" %08x", entryPtr->value);
      printf ("%11d", entryPtr->value);
      printf (" %d  ", entryPtr->relativeToS);
      if (entryPtr->relativeToT != NULL) {
        printSymbol (entryPtr->relativeToT, 20);
      }
      printf ("\n");
    }
  }
}



/* printSymbol (tableEntryPtr, fieldWidth)
**
** printSymbolOnFile (filePtr, tableEntryPtr, fieldWidth)
**
** These routines are passed a pointer to a TableEntry.  They print the
** string, translating non-printable characters into escape sequences.
** The String should never contain any non-printables besides
** \n, \t, \0, \r, \\, \", and \'.  If any are encountered, we
** will abort the linker.  We do not print a terminating newline.
** If the string is shorter than "fieldWidth", we pad it out with blanks.
*/

void printSymbol (TableEntry * tableEntryPtr, int fieldWidth) {
  printSymbolOnFile (stdout, tableEntryPtr, fieldWidth);
}

void printSymbolOnFile (FILE * fp, TableEntry * tableEntryPtr, int fieldWidth) {
#define BUF_SIZE 10
  int bufPtr, strPtr;
  char buffer[BUF_SIZE];
  int c;
  strPtr = 0;
  while (1) {
    /* If all characters printed, then exit. */
    if (strPtr >= tableEntryPtr->length) break;
    /* Fill up buffer */
    bufPtr = 0;
    while ((bufPtr < BUF_SIZE-2) && (strPtr < tableEntryPtr->length)) {
      c = tableEntryPtr->string [strPtr++];
      if (c == '\n') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'n';
        fieldWidth -= 2;
      } else if (c == '\t') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 't';
        fieldWidth -= 2;
      } else if (c == '\0') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '0';
        fieldWidth -= 2;
      } else if (c == '\r') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = 'r';
        fieldWidth -= 2;
      } else if (c == '\\') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '\\';
        fieldWidth -= 2;
      } else if (c == '\"') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '\"';
        fieldWidth -= 2;
      } else if (c == '\'') {
        buffer[bufPtr++] = '\\';
        buffer[bufPtr++] = '\'';
        fieldWidth -= 2;
      } else if ((c>=32) && (c<127)) {
        buffer[bufPtr++] = c;
        fieldWidth--;
      } else {
        fatalError ("Bad .o file; Symbol contains an unprintable char");
      }
    }
    /* Add \0 to buffer */
    buffer[bufPtr++] = '\0';
    /* Print buffer */
    fprintf (fp, "%s", buffer);
  }
  while (fieldWidth > 0) {
    fprintf (fp, " ");
    fieldWidth--;
  }
}



/* printTableArray ()
**
** This routine runs through the table array and prints each entry.
*/
void printTableArray () {
  int i;
  TableEntry * entryPtr;
  printf ("NUMB\tSYMBOL              \tVALUE   \tRELATIVE-TO\n");
  printf ("====\t====================\t========\t===========\n");
  for (i = 1; i<=maxSymbolNumber; i++) {
    entryPtr = tableArray [i];
    printf ("%d\t", i);
    printSymbol (entryPtr, 20);
    printf ("\t%08x", entryPtr->value);
    printf ("\t%d  ", entryPtr->relativeToS);
    if (entryPtr->relativeToT != NULL) {
      printSymbol (entryPtr->relativeToT, 20);
    }
    printf ("\n");
  }
}



/* lookup (givenEntry) -> TableEntry
**
** This routine is passed a pointer to a TableEntry.  It looks that
** entry up in the symbol table.  If there is already an entry with the
** same string, then this routine returns a ptr to it.  Otherwise, this
** routine adds this TableEntry to the symbol table and returns NULL to
** indicate that is was added.
*/
TableEntry * lookup (TableEntry * givenEntry) {
  unsigned hashVal = 0, g;
  char * p, * q;
  int i;
  TableEntry * entryPtr;

  /* Compute the hash value for the givenEntry and set hashVal to it. */
  for ( p = givenEntry->string, i=0;
        i < givenEntry->length;
        p++, i++ ) {
    hashVal = (hashVal << 4) + (*p);
    if (g = hashVal & 0xf0000000) {
      hashVal = hashVal ^ (g >> 24);
      hashVal = hashVal ^ g;
    }
  }
  hashVal %= SYMBOL_TABLE_HASH_SIZE;

  /* Search the linked list and return it if we find it. */
  for (entryPtr = symbolTableIndex [hashVal];
                      entryPtr;
                      entryPtr = entryPtr->next) {
    if (bytesEqual (entryPtr->string,
                    givenEntry->string,
                    entryPtr->length,
                    givenEntry->length)) {
      return entryPtr;
    }
  }

  /* Add the givenEntry to the appropriate linked list and return NULL. */
  givenEntry->next = symbolTableIndex [hashVal];
  symbolTableIndex [hashVal] = givenEntry;
  return NULL;
}



/* resolveSymbols ()
**
** This routine runs through the symbol table and sets each to an
** absolute value.
*/
void resolveSymbols () {
  int changes = 1;
  int hashVal;
  TableEntry * entryPtr, * relTo;
  while (changes) {
    changes = 0;
    for (hashVal = 0; hashVal<SYMBOL_TABLE_HASH_SIZE; hashVal++) {
      for (entryPtr = symbolTableIndex [hashVal];
                       entryPtr;
                       entryPtr = entryPtr->next) {
        /***
        printf ("Looking at ");
        printSymbol (entryPtr, 1);
        printf ("...\n");
        ***/
        if (entryPtr->relativeToT == absoluteEntry) {
          /* This symbol already has an absolute value. */
        } else {
          relTo = entryPtr->relativeToT;
          if (relTo == 0) {
            /* This symbol was imported in some file, but not exported
               from any file. */
          } else if (relTo->relativeToT == absoluteEntry) {
            /* This symbol is relative to a symbol with an absolute value
               so go ahead an update it. */
            entryPtr->value += relTo->value;
            entryPtr->relativeToT = absoluteEntry;
            entryPtr->relativeToS = 4;
            changes = 1;
          } else {
            /* This symbol is relative to a symbol that does not yet have
               a value, so we can do nothing during this pass. */
          }
        }
      }
    }
  }

  /* Now make a final pass through to print errors. */
  for (hashVal = 0; hashVal<SYMBOL_TABLE_HASH_SIZE; hashVal++) {
    for (entryPtr = symbolTableIndex [hashVal];
                     entryPtr;
                     entryPtr = entryPtr->next) {
      fflush (stdout);
      if (entryPtr == absoluteEntry) {
      } else if (entryPtr->relativeToT == absoluteEntry) {
      } else {
        relTo = entryPtr->relativeToT;
        if (relTo == 0) {
          errorsDetected++;
          fprintf (stderr, "BLITZ Linker Error: The symbol \"");
          printSymbolOnFile (stderr, entryPtr, 1);
          fprintf (stderr, "\" is imported in file \"%s\", but is not exported from any file.\n", entryPtr->filename);
        } else if (relTo != absoluteEntry) {
          errorsDetected++;
          fprintf (stderr, "BLITZ Linker Error: Symbol \"");
          printSymbolOnFile (stderr, entryPtr, 1);
          fprintf (stderr, "\" depends on \"");
          printSymbolOnFile (stderr, relTo, 1);
          fprintf (stderr, "\", which had problems.\n");

        }
      }
    }
  }
}



/* performRelocations ()
**
** Run thru all relocation records in all files and update
** the segments by modifying the locations.  In other words, perform
** each relocation action.
*/
void performRelocations () {
  FileInfo * file;
  RelocInfo * rel;
  int addrOfInstruction;
  char * ptr;
  int newValue, relOffset;

  /* Each iteration of this loop looks at a .o file. */
  for (file=inputFileList; file!=NULL; file=file->next) {
    if (commandOptionL) {
      printf (" Processing file %s...\n", file->filename);
      printRelocationHeader ();
    }

    /* Each iteration of this loop looks at another relocation action. */
    for (rel=file->relocList; rel!=NULL; rel=rel->next) {

      /* Print the relocation record */
      printRelocationRecord (rel);

      newValue = (rel->relativeTo->value) + (rel->offset);
      if (rel->inText == 1) {
        ptr = (char *) (
                         rel->locationToUpdate
                          + file->textAddr
                          - textStartAddr
                          + (int) textSegment
                       );
      } else if (rel->inText == 2) {
        ptr = (char *) (
                         rel->locationToUpdate
                          + file->dataAddr
                          - dataStartAddr
                          + (int) dataSegment
                       );
      }
      if (rel->type == 1) {  /* 8-bits */
        if (get8 (ptr) != 0) {
          printErrorAndExit (file,
              "PROGRAM LOGIC ERROR - Previous relocation value not 0x00");
        }
        if ((newValue < -128) | (newValue > 255)) {
          warningsDetected++;
          fprintf (stderr,
            "BLITZ Linker Warning: 8-bit value %d (0x%08x) is out of range.\n",
            newValue, newValue);
          fprintf (stderr,
            "                      (File = %s, source line number = %d)\n",
            file->filename, rel->sourceLineNumber);
        }
        put8 (ptr, newValue);
      } else if (rel->type == 2) { /* 16-bits */
        if (get16 (ptr) != 0) {
          printErrorAndExit (file,
               "PROGRAM LOGIC ERROR - Previous relocation value not 0x0000");
        }
        if ((newValue < -32768) | (newValue > 32767)) {
          warningsDetected++;
          fprintf (stderr,
            "BLITZ Linker Warning: 16-bit value %d (0x%08x) is out of range.\n",
            newValue, newValue);
          fprintf (stderr,
            "                      (File = %s, source line number = %d)\n",
            file->filename, rel->sourceLineNumber);
        }
        put16 (ptr, newValue);
      } else if (rel->type == 3) { /* 24-bits */
        if (rel->inText == 1) {
          addrOfInstruction = rel->locationToUpdate + file->textAddr;
        } else if (rel->inText == 2) {
          addrOfInstruction = rel->locationToUpdate + file->dataAddr;
        } else {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Bad switch value during relocation");
        }
        relOffset = newValue - addrOfInstruction;
        if ((addrOfInstruction & 0x00ffffff)
                   + relOffset !=
                   ((newValue  & 0x00ffffff))) {
          warningsDetected++;
          fprintf (stderr,
            "BLITZ Linker Warning: Relative offset (%08x) exceeds 24-bit limit.\n", relOffset);
          fprintf (stderr,
            "                      (File = %s, source line number = %d)\n",
            file->filename, rel->sourceLineNumber);
        }
        if (get24 (ptr) != 0) {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Previous relocation value not 0x000000");
        }
        put24 (ptr, relOffset);
      } else if (rel->type == 4) { /* 32-bits */
        if (get32 (ptr) != 0) {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Previous relocation value not 0x00000000");
        }
        put32 (ptr, newValue);
      } else if (rel->type == 5) { /* set-hi */
        if (get16 (ptr) != 0) {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Previous relocation value not 0x0000");
        }
        put16 (ptr, (newValue >> 16) & 0x0000ffff);
      } else if (rel->type == 6) { /* set-lo */
        if (get16 (ptr) != 0) {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Previous relocation value not 0x0000");
        }
        put16 (ptr, newValue & 0x0000ffff);
      } else if (rel->type == 7) { /* ldaddr */
        if (rel->inText == 1) {
          addrOfInstruction = rel->locationToUpdate + file->textAddr;
        } else if (rel->inText == 2) {
          addrOfInstruction = rel->locationToUpdate + file->dataAddr;
        } else {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Bad switch value during relocation");
        }
        relOffset = newValue - addrOfInstruction;
        if ((addrOfInstruction & 0x0000ffff)
                   + relOffset !=
                   ((newValue  & 0x0000ffff))) {
          warningsDetected++;
          fprintf (stderr,
            "BLITZ Linker Warning: Relative offset (%08x) exceeds 16-bit limit.\n", relOffset);
          fprintf (stderr,
            "                      (File = %s, source line number = %d)\n",
            file->filename, rel->sourceLineNumber);
        }
        if (get16 (ptr+2) != 0) {
          printErrorAndExit (file,
            "PROGRAM LOGIC ERROR - Previous relocation value not 0x0000");
        }
        put16 (ptr+2, relOffset);
      } else {
        printErrorAndExit (file,
          "PROGRAM LOGIC ERROR - Invalid relocation type");
      }
    }
  }
}



/* get8 (ptr) --> int
** get16 (ptr) --> int
** get24 (ptr) --> int
** get32 (ptr) --> int
**
** These routines are passed a pointer.  They read a value from memory
** and return it as an integer.  When getting an 8, 16, or 24 bit value
** from memory, the high-order bits in the result are set to zero.   The
** ptr need not be properly aligned.  In the case of 24-bits, the ptr points
** to the word, i.e., the byte before the 24-bits.  In the other cases,
** the ptr points to the data itself.
*/

int get8 (char * ptr){
  int val = * ptr;
  return (val & 0x000000ff);
}

int get16 (char * ptr){
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  fourBytes.chars[1] = * (ptr+0);
  fourBytes.chars[0] = * (ptr+1);
#else
  fourBytes.chars[2] = * (ptr+0);
  fourBytes.chars[3] = * (ptr+1);
#endif
  return (fourBytes.i & 0x0000ffff); 
}

int get24 (char * ptr){
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  fourBytes.chars[2] = * (ptr+1);
  fourBytes.chars[1] = * (ptr+2);
  fourBytes.chars[0] = * (ptr+3);
#else
  fourBytes.chars[1] = * (ptr+1);
  fourBytes.chars[2] = * (ptr+2);
  fourBytes.chars[3] = * (ptr+3);
#endif
  return fourBytes.i & 0x00ffffff;
}

int get32 (char * ptr){
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  fourBytes.chars[3] = * (ptr+0);
  fourBytes.chars[2] = * (ptr+1);
  fourBytes.chars[1] = * (ptr+2);
  fourBytes.chars[0] = * (ptr+3);
#else
  fourBytes.chars[0] = * (ptr+0);
  fourBytes.chars[1] = * (ptr+1);
  fourBytes.chars[2] = * (ptr+2);
  fourBytes.chars[3] = * (ptr+3);
#endif
  return fourBytes.i;
}



/* put8 (ptr, value)
** put16 (ptr, value)
** put24 (ptr, value)
** put32 (ptr, value)
**
** These routines are passed a pointer and a value.  They store that value
** in memory.  When storing 8, 16, and 24 bits, the high-order bits of
** the value are ignored.  The ptr need not be properly aligned.
** In the case of 24-bits, the ptr points to the word, i.e., the byte before
** the 24-bits.  In the other cases, the ptr points to the data itself.
*/

void put8 (char * ptr, int value){
  int val = value & 0x000000ff;
  *ptr = val;
}

void put16 (char * ptr, int value){
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
  fourBytes.i = SWAP_BYTES (value);
  * (ptr+0) = fourBytes.chars[2];
  * (ptr+1) = fourBytes.chars[3];
}

void put24 (char * ptr, int value){
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
  fourBytes.i = SWAP_BYTES (value);
  * (ptr+1) = fourBytes.chars[1];
  * (ptr+2) = fourBytes.chars[2];
  * (ptr+3) = fourBytes.chars[3];
}

void put32 (char * ptr, int value){
  union fourBytes {
    char chars [4];
    unsigned int i;
  } fourBytes;
  fourBytes.i = SWAP_BYTES (value);
  * (ptr+0) = fourBytes.chars[0];
  * (ptr+1) = fourBytes.chars[1];
  * (ptr+2) = fourBytes.chars[2];
  * (ptr+3) = fourBytes.chars[3];
}



/* printRelocationHeader ()
**
** Print the column headings.
*/
void printRelocationHeader () {
  if (!commandOptionL) return;
  printf ("        Source  Type    Locatn-to-modify New-val  Relative-to\n");
  printf ("        ======= ======= ================ ======== ===========\n");
}



/* printRelocationRecord (rel)
**
** Print a single RelocInfo record.
*/
void printRelocationRecord (RelocInfo * rel) {
  if (!commandOptionL) return;
  printf ("\t");
  printf ("%5d   ", rel->sourceLineNumber);
  if (rel->type == 1) printf ("8-bit\t");
  if (rel->type == 2) printf ("16-bit\t");
  if (rel->type == 3) printf ("24-bit\t");
  if (rel->type == 4) printf ("32-bit\t");
  if (rel->type == 5) printf ("set-hi\t");
  if (rel->type == 6) printf ("set-lo\t");
  if (rel->type == 7) printf ("ldaddr\t");
  printf ("%08x ", rel->locationToUpdate);
  if (rel->inText == 1) {
    printf (".text\t");
  } else if (rel->inText == 2) {
    printf (".data\t");
  } else {
    printf ("*****\t");
  }
  printf (" %08x ", rel->offset);
  printSymbol (rel->relativeTo, 1);
  printf ("\n");
}



/* readRelocationRecords (file)
**
** This routine reads in all the relocation records from a single .o
** file.  It allocates a RelocInfo record for each, chains them togther
** in a linked list, and makes file->relocList point to the new list.
*/
void readRelocationRecords (FileInfo * file) {
  RelocInfo * rel;
  int i;
  if (commandOptionL) {
    printf (" Processing this file's relocation records...\n");
  }
  file->relocList = NULL;
  while (1) {

    /* Read in the "type" field (1, 2, 3, 4, 5, 6, or 7). */
    i = readInteger (file);
    if (i == 0) break;
    if ((i<1) | (i>7)) {
      printErrorAndExit (file, "Invalid reloc record");
    }

    /* Allocate a RelocInfo struct. */
    rel = (RelocInfo *) calloc (1, sizeof (RelocInfo));
    if (rel == NULL) {
      fatalError ("Calloc failed - insufficient memory available");
    }
    rel->type = i;

    /* Read in the "locationToUpdate" field. */
    rel->locationToUpdate = readInteger (file);

    /* Read in inText (1=text, 2=data). */
    rel->inText = readInteger (file);
    if ((rel->inText != 1) & (rel->inText != 2)) {
      printErrorAndExit (file, "Invalid reloc record");
    }

    /* Read in the "offset" field. */
    rel->offset = readInteger (file);

    /* Read in "relativeTo" and adjust from symbol number to TableEntry. */
    i = readInteger (file);
    if ((i<1) | (i>maxSymbolNumber)) {
      printErrorAndExit (file, "Invalid reloc record");
    }
    rel->relativeTo = tableArray [i];

    /* Read in the "sourceLineNumber" field. */
    rel->sourceLineNumber = readInteger (file);

    /* Link this record into the growing linked list. */
    rel->next = file->relocList;
    file->relocList = rel;
  }
}



/* addLabels ()
**
** This routine runs through the .o files.  For each, it reads its
** labels and adds them to the symbol table.  It "uniqifies" each label
** first.  In other words, if there is already an entry for this label, 
** we modify the label (from "aLabel" to "aLabel_43") until it is unique.
** The value of each label is an address in the .text, .data, or .bss
** segment in this file.
**
** We are subverting the symbol table somewhat, here.  We are done processing
** symbols by this time, so we can just add the labels to the symbol table
** as if they were first-class symbols.  We will ignore all fields
** in the symbol table entry except "value", "length" and "string".
** Then, we'll go through the table and add all entries to the a.out file.
*/
void addLabels () {
  FileInfo * file;
  LabelEntry * entryPtr;
  int foundIt, unique;
  int relativeTo, value, len, j, magic;
  char * q, * where;

  for (file=inputFileList; file!=NULL; file=file->next) {
    /***
    printf ("==========  Processing file %s  ==========\n", file->filename);
    ***/
    while (1) {
      relativeTo = readInteger (file);
      if (relativeTo == 0) break;
      value = readInteger (file);
      len = readInteger (file);
  
      /* Create a LabelEntry and initialize it.  Leave space for uniquifier. */
      entryPtr = (LabelEntry *) calloc (1, sizeof (LabelEntry) + len + 1 + 8);
      if (entryPtr == 0) {
        fatalError ("Calloc failed - insufficient memory available");
      }
      entryPtr->length = len;
      entryPtr->value = value;
      if (relativeTo == 1) {
        entryPtr->value = file->textAddr + value;
      } else if (relativeTo == 2) {
        entryPtr->value = file->dataAddr + value;
      } else if (relativeTo == 3) {
        entryPtr->value = file->bssAddr + value;
      } else {
        printErrorAndExit (file, "Invalid label record");
      }
  
      /* Read in the characters and move into this TableEntry. */
      for (q=entryPtr->string, j=len;
           j>0;
           q++, j--) {
        *q = readByte (file);
      }
      /*** printf ("  Processing %s... ", entryPtr->string); ***/
  
      /* Look this string up in the symbol table. */
      foundIt = lookupLabel (entryPtr);
      if (!foundIt) {    /* if not found... */
        /* ...then it will have gotten added to the symbol table. */
        /*** printf ("Not found.  It was added.\n"); ***/
      /* If found in the table... */
      } else {
        /*** printf ("Found.\n"); ***/
        /* Begin uniquifying the label. */
        unique = 1;
        /* Set "where" to point to the addr of the \000 char */
        for (where=entryPtr->string; *where; where++) ;
        while (1) {
          sprintf (where, "_%d", unique++);
          entryPtr->length = strlen (entryPtr->string);
          /*** printf ("    Searching for %s...\n", entryPtr->string); ***/
          if (!lookupLabel (entryPtr)) break;
        }
      }
    }
    magic = readInteger (file);
    if (magic != 0x2a2a2a2a) {
      printErrorAndExit (file, "Invalid **** separator");
    }
  }
}



/* lookupLabel (newEntry) -> Boolean
**
** This routine is passed a pointer to a LabelEntry.  It looks that
** entry up in the label table.  If there is already an entry with the
** same string, then this routine returns TRUE.  Otherwise, this
** routine adds this LabelEntry to the label table and returns FALSE to
** indicate that is was added.
*/
int lookupLabel (LabelEntry * newEntry) {
  unsigned hashVal = 0, g;
  char * p, * q;
  int i;
  LabelEntry * entryPtr;

  /* Compute the hash value for the newEntry and set hashVal to it. */
  for ( p = newEntry->string, i=0;
        i < newEntry->length;
        p++, i++ ) {
    hashVal = (hashVal << 4) + (*p);
    if (g = hashVal & 0xf0000000) {
      hashVal = hashVal ^ (g >> 24);
      hashVal = hashVal ^ g;
    }
  }
  hashVal %= LABEL_TABLE_HASH_SIZE;

  /* Search the linked list and return TRUE if we find it. */
  for (entryPtr = labelTableIndex [hashVal];
                      entryPtr;
                      entryPtr = entryPtr->next) {
    if (strcmp (entryPtr->string, newEntry->string) == 0) {
      return 1;
    }
  }

  /* Add the newEntry to the appropriate linked list and return FALSE. */
  newEntry->next = labelTableIndex [hashVal];
  labelTableIndex [hashVal] = newEntry;
  return 0;
}



/* writeLabels ()
**
** This routine runs through the label table and writes an entry to
** the a.out file.  It also prints the label table, if that is required.
*/
void writeLabels () {
  int hashVal;
  char * p;
  int i, len;
  LabelEntry * entryPtr;
  if (commandOptionS) {
    printf ("ADDRESS  LABEL\n");
    printf ("======== =============\n");
  }
  for (hashVal = 0; hashVal<LABEL_TABLE_HASH_SIZE; hashVal++) {
    for (entryPtr = labelTableIndex [hashVal];
                       entryPtr;
                       entryPtr = entryPtr->next) {
      len = strlen (entryPtr->string);
      writeInteger (len);
      writeInteger (entryPtr->value);
      i = fwrite (entryPtr->string, 1, len, outputFile);
      if (i != len) {
        fatalError ("Error while writing data to output executable file");
      }
      if (commandOptionS) {
        printf ("%08x ", entryPtr->value);
        printf ("%s\n", entryPtr->string);
      }
    }
  }
  writeInteger (0);
  if (commandOptionS) {
    printf ("\n");
  }
}
