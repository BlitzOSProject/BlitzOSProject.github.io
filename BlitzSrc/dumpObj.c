/* Program to dump a BLITZ ".o" or "a.out" file
**
** Copyright 2000-2007, Harry H. Porter III
**
** This file may be freely copied, modified and compiled, on the sole
** condition that if you modify it...
**   (1) Your name and the date of modification is added to this comment
**       under "Modifications by", and
**   (2) Your name and the date of modification is added to the printHelp()
**       routine under "Modifications by".
**
** Original Author:
**   11/12/00 - Harry H. Porter III
**
** Modifcations by:
**   03/15/06 - Harry H. Porter III
**   04/30/07 - Harry H. Porter III - Support for little endian added
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



/*****  Global variables *****/

char * commandInFileName = NULL; /* The input filename, if provided */
FILE * inputFile;                /* The input file */
int textSize;
int dataSize;
int bssSize;
int textAddr;
int dataAddr;
int bssAddr;
int count;
int row [16];
int size;
int symbolNum;
int locationToUpdate;
int value;
int relativeTo;
int len;



/* Function prototypes */

void processCommandLine (int argc, char ** argv);
void printHelp ();
int readInteger ();
int readByte ();
void printSegment (int startingAddr);
void readline ();
int getNext ();
void putlong (int i);
void printline ();
void putbyt (int c);
void fatalError (char * msg);



/* main()
**
** Read and print .o file.
*/
main (int argc, char ** argv) {
    int i, lineNumber;
    int magic;
    int sawEntryLabel;
    processCommandLine (argc, argv);
    printf ("=====  HEADER INFO  =====\n\n");
    magic = readInteger ();
    if (magic == 0x424C5A6F) {
      printf ("magic number = BLZo (This is a '.o' file.)\n");
    } else if (magic == 0x424C5A78) {
      printf ("magic number = BLZx (This is an 'a.out' file.)\n");
    } else {
      fatalError ("Magic number is not 'BLZo' or 'BLZx'");
    }
    if (magic == 0x424C5A78) {   /* if we have an a.out file... */
      textSize = readInteger ();
      dataSize = readInteger ();
      bssSize = readInteger ();
      printf (".text size = %08x (%d)\n", textSize, textSize);
      printf (".data size = %08x (%d)\n", dataSize, dataSize);
      printf (".bss size =  %08x (%d)\n", bssSize, bssSize);
      textAddr = readInteger ();
      dataAddr = readInteger ();
      bssAddr = readInteger ();
      printf (".text load addr = %08x (%d)\n", textAddr, textAddr);
      printf (".data load addr = %08x (%d)\n", dataAddr, dataAddr);
      printf (".bss load addr =  %08x (%d)\n", bssAddr, bssAddr);
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
      printf ("\n=====  TEXT SEGMENT  =====\n\n");
      count = textSize;
      printSegment (textAddr);
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
      printf ("\n=====  DATA SEGMENT  =====\n\n");
      count = dataSize;
      printSegment (dataAddr);
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
      printf ("\n=====  LABEL INFORMATION  =====\n\n");
      printf ("Value   (in decimal)  Label\n");
      printf ("====================  ==================\n");
      while (1) {
        len = readInteger ();
        if (len <= 0) break;
        value = readInteger ();
        printf ("%08x%12d  ", value, value);
        for (; len>0; len--) {
          i = readByte ();
          printf ("%c", i);
        }
        printf ("\n");
      }
  
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
  
    } else {   /* if we have an .o file... */
      sawEntryLabel = readInteger ();
      if (sawEntryLabel == 1) {
        printf (".text contains _entry = TRUE\n");
      } else if (sawEntryLabel == 0) {
        printf (".text contains _entry = FALSE\n");
      } else {
        fatalError ("sawEntryLabel has invalid value");
      }
      textSize = readInteger ();
      dataSize = readInteger ();
      bssSize = readInteger ();
      printf (".text size = %08x (%d)\n", textSize, textSize);
      printf (".data size = %08x (%d)\n", dataSize, dataSize);
      printf (".bss size =  %08x (%d)\n", bssSize, bssSize);
      printf ("\n=====  TEXT SEGMENT  =====\n\n");
      count = textSize;
      printSegment (0);
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
      printf ("\n=====  DATA SEGMENT  =====\n\n");
      count = dataSize;
      printSegment (0);
  
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
  
      printf ("\n=====  SYMBOL TABLE  =====\n\n");
      printf ("Number  Value     Relative to   Symbol\n");
      printf ("======  ========  ===========   ======\n");
      while (1) {
        symbolNum = readInteger ();
        if (symbolNum == 0) break;
        value = readInteger ();
        relativeTo = readInteger ();
        len = readInteger ();
        printf ("%d\t%08x\t%d\t", symbolNum, value, relativeTo);
        for (; len>0; len--) {
          i = readByte ();
          printf ("%c", i);
        }
        printf ("\n");
      }
  
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
  
      printf ("\n=====  RELOCATION INFO  =====\n\n");
      printf ("Type    Locatn-to-modify New-val  Rel-to Src-Line\n");
      printf ("======= ================ ======== ====== ========\n");
      while (1) {
        i = readInteger ();
        if (i == 0) {
          break;
        } else if (i == 1) {
          printf ("8-bit\t");
        } else if (i == 2) {
          printf ("16-bit\t");
        } else if (i == 3) {
          printf ("24-bit\t");
        } else if (i == 4) {
          printf ("32-bit\t");
        } else if (i == 5) {
          printf ("set-hi\t");
        } else if (i == 6) {
          printf ("set-lo\t");
        } else if (i == 7) {
          printf ("ldaddr\t");
        } else {
          fatalError ("Invalid relocation record code");
        }
        value = readInteger ();
        printf ("%08x ", value);
        i = readInteger ();
        if (i == 1) {
          printf (".text\t");
        } else if (i == 2) {
          printf (".data\t");
        } else {
          printf ("*****\t");
        }
        value = readInteger ();
        printf (" %08x ", value);
        value = readInteger ();
        printf ("%4d   ", value);
        lineNumber = readInteger ();
        printf ("%6d  \n", lineNumber);
      }
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
  
      printf ("\n=====  ADDRESS LABELS  =====\n\n");
      printf ("Segment Offset    Label\n");
      printf ("======= ========  =====\n");
      while (1) {
        i = readInteger ();
        if (i == 0) break;
        if (i == 1) printf (".text\t");
        if (i == 2) printf (".data\t");
        if (i == 3) printf (".bss\t");
        value = readInteger ();
        len = readInteger ();
        printf ("%08x  ", value);
        for (; len>0; len--) {
          i = readByte ();
          printf ("%c", i);
        }
        printf ("\n");
      }
  
      magic = readInteger ();
      if (magic != 0x2a2a2a2a) {
        fatalError ("Invalid file format - missing \"****\" separator");
      }
    }
}



/* processCommandLine (argc, argv)
**
** This routine processes the command line options.
*/
void processCommandLine (int argc, char ** argv) {
  int argCount;
  int len;
  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
    argCount = 1;
    /* Scan the -h option */
    if (!strcmp (*argv, "-h")) {
      printHelp ();
      exit (1);
    /* Scan an input file name */
    } else if ((*argv)[0] != '-') {
      if (commandInFileName == NULL) {
        commandInFileName = *argv;
      } else {
        fprintf (stderr,
          "dumpObj: Invalid command line;  Multiple input files;  Use -h for help display\n");
        exit (1);
      }
    } else {
      fprintf (stderr,
        "dumpObj: Invalid command line option (%s);  Use -h for help display\n",
        *argv);
      exit (1);
    }
  }

  /* Open the input (.o) file */
  if (commandInFileName == NULL) {
    inputFile = stdin;
  } else {
    inputFile = fopen (commandInFileName, "r");
    if (inputFile == NULL) {
      fprintf (stderr,
          "dumpObj: Input file \"%s\" could not be opened\n", commandInFileName);
      exit (1);
    }
  }
}



/* printHelp ()
**
** This routine prints some documentation.  It is invoked whenever
** the -h option is used on the command line.
*/
void printHelp () {
  printf (
"================================================\n"
"=====                                      =====\n"
"=====  The BLITZ Object File Dump Program  =====\n"
"=====                                      =====\n"
"================================================\n"
"\n"
"Copyright 2000-2007, Harry H. Porter III\n"
"========================================\n"
"  Original Author:\n"
"    11/12/00 - Harry H. Porter III\n"
"  Modifcations by:\n"
"    03/15/06 - Harry H. Porter III\n"
"    04/30/07 - Harry H. Porter III - Support for little endian added\n"
"\n"
"Overview\n"
"========\n"
"  This program prints out a BLITZ \".o\" or \"a.out\" file in human-readable\n"
"  form.  This program does some (very limited) error checking on the file.\n"
"\n"
"Command Line Options\n"
"====================\n"
"  Command line options may be given in any order.\n"
"    -h\n"
"      Print this info.  The input source is ignored.\n"
"    filename\n"
"      The input source will come from this file.  (This file should be a\n"
"      \".o\" or \"a.out\" file.)  If an input file is not named on the command\n"
"      line, the source must come from stdin.  Only one input source is allowed.\n");
}



/* readInteger ()
**
** Read and return an integer.
*/
int readInteger () {
  int i, numBytesRead;
  numBytesRead = fread (&i, 4, 1, inputFile);
  if (numBytesRead != 1) {
    fatalError ("Problem reading from input file");
  }
  return SWAP_BYTES (i);
}



/* readByte ()
**
** Read 1 byte and return an integer.
*/
int readByte () {
  int i, numBytesRead;
  char c;
  numBytesRead = fread (&c, 1, 1, inputFile);
  if (numBytesRead != 1) {
    fatalError ("Problem reading from input file");
  }
  i = c;
  return i;
}



/* printSegment (startingAddr)
**
** This routine reads raw bytes from the input file and prints them
** out in a formatted way like this:
**
** 00002000: 0000 0000 0000 0000 0000 0000 0000 0000   ................
** 00002010: 6162 6364 6566 6768 3400 0000 696A 6B6C   abcdefgh4...ijkl
** 00002020: 6D6E 6F70 7172 7374 7576 7778 797A 0000   mnopqrstuvwxyz..
**
** It is passed the starting address; in this example it was 0x0000200.
*/
void printSegment (int startingAddr) {
   int addr;

   /* Each execution of this loop prints a single output line. */
   addr = startingAddr;
   readline ();
   while (size > 0) {
     putlong (addr);
     printf (":  ");
     printline ();
     addr = addr + 16;
     readline ();
   }

}



/* readline ()
**
** This routine reads in the next 16 bytes from the file and
** places them in the array named 'row', setting 'size' to
** be the number of bytes read in.  Size will be less than
** 16 if EOF was encountered, and may possibly be 0.
*/
void readline () {
  int c;
  size = 0;
  c = getNext ();
  while (c != -999) {
    row [size] = c;
    size = size + 1;
    if (size >= 16) break;
    c = getNext ();
  }
}



/* getNext ()
**
** Read next char, decrementing count.  Return it, or -999 if count <= 0.
*/
int getNext () {
  if (count <= 0) return -999;
  count--;
  return readByte();
}



/* putlong ()
**
** This routine is passed an integer, which it displays as 8 hex digits.
*/
void putlong (int i) {
  putbyt ((i>>24) & 0x000000ff);
  putbyt ((i>>16) & 0x000000ff);
  putbyt ((i>>8) & 0x000000ff);
  putbyt ((i>>0) & 0x000000ff);
}



/* printline ()
**
** This routine prints the current 'row'.
*/
void printline () {
  int i, c;
  if (size > 0) {
    i = 0;
    while (i<16) {
      if (i < size) {
        putbyt (row[i]);
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
    for (i=0; i<size; i++) {
      c = row[i];
      if ((c>=' ') && (c <= '~')) {
        putchar (c);
      } else {
        putchar ('.');
      }
    }
    printf ("\n");
  }
}



/* putbyt ()
**
** This routine is passed a byte (i.e., an integer -128..255) which
** it displays as 2 hex characters.  If passed a number out of that
** range, it outputs nothing.
*/
void putbyt (int c) {
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



/* fatalError (msg)
**
** Print this message and abort.
*/
void fatalError (char * msg) {
  printf ("\n*****  ERROR: %s  *****\n", msg);
  exit (1);
}
