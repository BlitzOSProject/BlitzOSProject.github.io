/* BLITZ Disk Utility for "Stub File System"
**
** Copyright 2004-2007, Harry H. Porter III
**
** This file may be freely copied, modified and compiled, on the sole
** conditions that if you modify it...
**
**   (1) Your name and the date of modification is added to this comment
**       under "Modifications by", and
**
**   (2) Your name and the date of modification is added to the
**       "commandLineHelp()" routine under "Modifications by".
**
** Original Author:
**   10/07/04 - Harry H. Porter III
**
** Modifications by:
**   04/30/07 - Harry H. Porter III - Support for little endian added
**
** Please respect the coding and commenting style of this program.
**
**
** The BLITZ emulator simulates the BLITZ disk using a Unix file on the host
** machine.  This program allows that file to be manipulated.  For example,
** it can be used to copy an executable file containing a user program to the
** BLITZ disk so that the BLITZ OS kernel can then access, load, and run it.
**
** The BLITZ DISK is organized as follows.  The disk contains a single directory
** and this is kept in sector 0.  The files are placed sequentially on the
** disk, one after the other.  Each file will take up an integral number of
** sectors.  Each file has an entry in the directory.  Each entry contains
**      (1)  The starting sector
**      (2)  The file length, in bytes (possibly zero)
**      (3)  The number of characters in the file name
**      (4)  The file name
** The directory begins with three numbers:
**      (1)  Magic Number (0x73747562 = "stub")
**      (2)  Number of files (possibly zero)
**      (3)  Number of the next free sector
** These are followed by the entries for each file.
**
** Once created, a BLITZ file may not have its size increased.  When a file is
** removed, the free sectors become unusable; there is no compaction or any
** attempt to reclaim the lost space.
**
** Each time this program is run, it performs one of the following functions:
**       Initialize  set up a new file system on the BLITZ disk
**       List        list the directory on the BLITZ disk
**       Create      create a new file of a given size
**       Remove      remove a file
**       Add         copy a file from Unix to BLITZ
**       Extract     copy a file from BLITZ to Unix
**       Write       write sectors from a Unix file to the BLITZ disk
**
** For further info, see the function "commandLineHelp" or run the program:
**      diskUtil -h
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


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



#define SECTORS_PER_TRACK      16
#define PAGE_SIZE              8192            /* Page Size (8K) */
#define MAX 2147483647



/*****  Global variables *****/

int commandOptionI = 0;           /* The -i option */
int commandOptionL = 0;           /* The -l option */
int commandOptionC = 0;           /* The -c option */
int commandOptionR = 0;           /* The -r option */
int commandOptionA = 0;           /* The -a option */
int commandOptionE = 0;           /* The -e option */
int commandOptionW = 0;           /* The -w option */
int commandOptionV = 0;           /* The -v option */
char * diskFileName = NULL;       /* The DISK filename */
FILE * diskFile = NULL;           /* The DISK file, possibly NULL */
char * unixFileName = NULL;       /* The UNIX filename */
FILE * unixFile = NULL;           /* The UNIX file descriptor, possibly NULL */
char * blitzFileName = NULL;      /* The BLITZ filename */
int startingSectorNumber = -1;    /* From the command line */
int sizeInBytes = -1;             /* From the command line */
int numberSectorsOnDisk = -1;     /* Number of sectors on the DISK file */
int dirMagic = -1;                /* Magic number "stub" */
int numberOfFiles = -1;           /* Number of files in directory */
int nextFreeSector = -1;          /* Next free sector on disk */
int sizeOfDirectoryInBytes = -1;  /* The number of bytes consumed in sector 0 */
int thisFileSizeInBytes = -1;     /* The size of the selected file */
int thisFileStartingSector = -1;  /* The starting sector of this file */

typedef struct Entry Entry;

struct Entry {
  Entry * next;
  int startingSector;
  int sizeInBytes;
  int lengthOfName;
  char * name;
};

Entry * firstEntry = 0;
Entry * lastEntry = 0;


/*****  Function prototypes  *****/

int main (int argc, char ** argv);
void closeFiles ();
void openDiskFile (int existing);
void writeCommand ();
void initializeCommand ();
void listCommand ();
void createCommand ();
void removeCommand ();
void addCommand ();
void extractCommand ();
void checkHostCompatibility ();
void checkArithmetic ();
void processCommandLine (int argc, char ** argv);
void badOption (char * msg);
void fatalError (char * msg);
void commandLineHelp ();
int readInteger ();
void writeInteger (int i);
void writeString (char * str, int len);
char * readString (int len);
Entry * findFile (char * str);
void seekTo (int i);
void printDirectory ();
void writeDirectory ();



/* main ()
**
** Scan the command line arguments, then enter into the command loop.
*/
main (int argc, char ** argv) {
  checkHostCompatibility ();
  checkArithmetic ();
  processCommandLine (argc, argv);

  /* If the "write" option (-w) was given, write a file to the DISK. */
  if (commandOptionW) {
    writeCommand ();
  } else if (commandOptionI) {
    initializeCommand ();
  } else if (commandOptionL) {
    listCommand ();
  } else if (commandOptionC) {
    createCommand ();
  } else if (commandOptionR) {
    removeCommand ();
  } else if (commandOptionA) {
    addCommand ();
  } else if (commandOptionE) {
    extractCommand ();
  } else {
    fatalError ("No commands given; Use -h for help display");
  }
}



/* initializeCommand ()
**
** Initialize the BLITZ DISK file by writing out an empty directory.
*/
void initializeCommand () {
  long longLength;
  int diskFileLength, diskMagic;

  if (commandOptionV) printf ("INITIALIZING THE FILE SYSTEM...\n");

  openDiskFile (0);               /* Existing = false */

  seekTo (4);                     /* Skip past "BLZd" magic number */
  writeInteger (0x73747562);      /* Magic number = "stub" */
  writeInteger (0);               /* Number of file */
  writeInteger (1);               /* Next Free Sector */

  if (commandOptionV) printf ("Number of files on disk = 0\n");
  if (commandOptionV) printf ("Next free sector = 1\n");
  if (commandOptionV) printf ("Number of available sectors = %d\n", numberSectorsOnDisk-1);

  closeFiles ();
}



/* openDiskFile (existing)
**
** Open the BLITZ DISK file and read in some info.  If "existing" is true,
** then also read in the directory info and make sure things look right.
*/
void openDiskFile (int existing) {
  long longLength;
  int diskFileLength, diskMagic, i;
  Entry * ent;

  if (commandOptionV) printf ("Opening the DISK file...\n");

  // Open the DISK file for updating...
  errno = 0;
  diskFile = fopen (diskFileName, "r+");
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("The DISK file could not be opened for updating");
  }

// qqqqq  PREVIOUSLY THE FOLLOWING PRINT WAS DISABLED FOR AUTOMATED TESTING...
  if (commandOptionV) printf ("  The DISK file \"%s\" has been opened successfully.\n", diskFileName);

  /* Get the size of the DISK file... */
  errno = 0;
  fseek (diskFile, 0l, SEEK_END);
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("Error during call to fseek");
  }
  longLength = ftell (diskFile);

  /* Check to see if the file is too big... */
  if (longLength > ((long) MAX)) {
    printf ("The maximum integer is %d\n", MAX);
    fatalError ("Error in DISK File: The DISK file size exceeds the maximum");
  }

  /* Print the length of the DISK file */
  diskFileLength = (int) longLength;
  if (commandOptionV) printf ("  The length of the DISK file is %d bytes.\n", diskFileLength);
  numberSectorsOnDisk = (diskFileLength - 4) / PAGE_SIZE;
  if (commandOptionV) printf ("  The number of sectors on the DISK = %d\n", numberSectorsOnDisk);

  /* Reposition the DISK file to the beginning. */
  seekTo (0);

  /* Check the DISK magic number. */
  diskMagic = readInteger ();
  if (diskMagic != 0x424C5A64) {
    fatalError ("Error in DISK File: Magic number is not 'BLZd'");
  }

  /* Read the "Stub file system directory" magic number. */
  dirMagic = readInteger ();
  // printf ("  dirMagic = 0x%08x\n", dirMagic);

  /* Read the number of files. */
  numberOfFiles = readInteger ();

  /* Read the next free sector. */
  nextFreeSector = readInteger ();

  if (!existing) {
    return;
  }

  sizeOfDirectoryInBytes = 16;        /* Magic "BLZd", Magic "stub", int, int */

  /* Check the directory magic number */
  if (dirMagic != 0x73747562) {
    printf ("The directory magic number is not 0x73747562 = \"stub\"\n");
    fatalError ("Error in DISK File: No file system found; see the -i option");
  }

  /* Check nextFreeSector */
  if (nextFreeSector < 1 || nextFreeSector > numberSectorsOnDisk) {
    fatalError ("Error in DISK File: The 'nextFreeSector' is incorrect");
  }

  /* Check numberOfFiles */
  if (numberOfFiles < 0 || numberOfFiles > 1000) {
    fatalError ("Error in DISK File: The 'numberOfFiles' is incorrect");
  }

  if (commandOptionV) printf ("  The number of files = %d\n", numberOfFiles);
  if (commandOptionV) printf ("  The next free sector = %d\n", nextFreeSector);

  /* Read in the files and allocate the directory structures. */
  firstEntry = 0;
  lastEntry = 0;
  for (i = numberOfFiles; i > 0; i--) {
    ent = (Entry *) calloc (1, sizeof (Entry));
    ent->startingSector = readInteger ();
    ent->sizeInBytes = readInteger ();
    ent->lengthOfName = readInteger ();
    ent->name = readString (ent->lengthOfName);
    ent->next = 0;
    if (firstEntry) {
      lastEntry->next = ent;
      lastEntry = ent;
    } else {
      firstEntry = ent;
      lastEntry = ent;
    }
    sizeOfDirectoryInBytes += 12 + ent->lengthOfName;
  }

  if (commandOptionV) printf ("  The size of the directory = %d bytes\n", sizeOfDirectoryInBytes);
}



/* closeFiles ()
**
** Close all open files and exit.
*/
void closeFiles () {

  /* Close the DISK file if open. */
  if (diskFile) {
    errno = 0;
    fclose (diskFile);
    if (errno) {
      perror ("Error on BLITZ DISK file");
      fatalError ("The BLITZ DISK file could not be closed");
    }
    if (commandOptionV) printf ("The BLITZ DISK file was closed successfully.\n");
  }

  /* Close the UNIX file if open. */
  if (unixFile) {
    errno = 0;
    fclose (unixFile);
    if (errno) {
      perror ("Error on UNIX file");
      fatalError ("The UNIX file could not be closed");
    }
    if (commandOptionV) printf ("The UNIX file was closed successfully.\n");
  }

  /* Return no error */
  exit (0);
}



/* listCommand ()
**
** Print the directory contents.
*/
void listCommand () {
  if (commandOptionV) printf ("LISTING DIRECTORY...\n");
  openDiskFile (1);         /* existing = true */
  printDirectory ();
  closeFiles ();
}



/* createCommand ()
**
** Create a new file on the BLITZ DISK.
*/
void createCommand () {
  Entry * ent;
  int sizeInSectors;
  if (commandOptionV) printf ("CREATING A NEW FILE...\n");
  if (commandOptionV) printf ("  blitzFileName = \'%s\'\n", blitzFileName);
  if (commandOptionV) printf ("  sizeInBytes = %d\n", sizeInBytes);

  openDiskFile (1);         /* existing = true */

  if (findFile (blitzFileName)) {
    fatalError ("A file with this name already exists on the BLITZ DISK");
  }

  /* Create a new entry */
  ent = (Entry *) calloc (1, sizeof (Entry));
  ent->startingSector = nextFreeSector;
  ent->sizeInBytes = sizeInBytes;
  ent->lengthOfName = strlen (blitzFileName);
  ent->name = blitzFileName;

  /* Add it to the list */
  ent->next = 0;
  if (firstEntry) {
    lastEntry->next = ent;
    lastEntry = ent;
  } else {
    firstEntry = ent;
    lastEntry = ent;
  }

  /* Adjust the file statistics */
  sizeInSectors = (sizeInBytes + PAGE_SIZE-1) / PAGE_SIZE;
  nextFreeSector = nextFreeSector + sizeInSectors;
  numberOfFiles ++;
  sizeOfDirectoryInBytes += 12 + ent->lengthOfName;

  /* Make sure the directory has not grown too large to fit into sector 0. */
  if (sizeOfDirectoryInBytes > PAGE_SIZE) {
    fatalError ("The directory has grown too large to fit into sector 0");
  }

  /* Make sure this file will fit on the disk. */
  if (nextFreeSector > numberSectorsOnDisk) {
    fatalError ("This file will not fit onto the disk");
  }

  if (commandOptionV) {
    printf ("Size of directory info = %d bytes\n", sizeOfDirectoryInBytes);
    printf ("Size of this file = %d sectors\n", sizeInSectors);
    printf ("New total number of sectors used = %d\n", nextFreeSector);
    printf ("Number of free sectors left = %d\n", numberSectorsOnDisk - nextFreeSector);
    printf ("New number of files = %d\n", numberOfFiles);

    /* Print the new directory */
    printDirectory ();
  }

  /* Write out the new directory */
  writeDirectory ();

  closeFiles ();
}



/* removeCommand ()
**
** Remove a file from the BLITZ DISK file.
*/
void removeCommand () {
  Entry * ent, * prev;

  if (commandOptionV) {
    printf ("REMOVING A FILE...\n");
    printf ("  blitzFileName = \'%s\'\n", blitzFileName);
  }

  openDiskFile (1);         /* existing = true */

  /* Set ent to point to the entry to delete;
     set prev to point to the entry before it (or null). */
  ent = firstEntry;
  prev = NULL;
  while (1) {
    if (ent == NULL) {
      fatalError ("This file does not exist on the BLITZ DISK");
    }
    if (strcmp (ent->name, blitzFileName) == 0) {
      break;
    }
    prev = ent;
    ent = ent->next;
  }

  /* Remove the entry */
  if (lastEntry == ent) {
    lastEntry = prev;
  }
  if (firstEntry == ent) {
    firstEntry = ent->next;
  }
  if (prev) {
    prev->next = ent->next;
  }

  /* Adjust the file statistics */
  numberOfFiles--;

  if (commandOptionV) {
    printf ("The file has been removed.\n");
    printf ("  New number of files = %d\n", numberOfFiles);
    printf ("  Total number of sectors used = %d\n", nextFreeSector);
    printf ("  Number of free sectors left = %d\n", numberSectorsOnDisk - nextFreeSector);
    printDirectory ();
  }

  /* Write out the new directory */
  writeDirectory ();
}



/* addCommand ()
**
** Copy a UNIX file to the BLITZ DISK file.
*/
void addCommand () {
  long longLength;
  Entry * ent;
  int unixFileSizeInBytes, unixFileSizeInSectors, s, i;
  char * memory;

  if (commandOptionV) {
    printf ("ADD A UNIX FILE TO THE BLITZ DISK...\n");
    printf ("  unixFileName = \'%s\'\n", unixFileName);
    printf ("  blitzFileName = \'%s\'\n", blitzFileName);
  }

  openDiskFile (1);         /* existing = true */

  // Open the UNIX file for reading...
  errno = 0;
  unixFile = fopen (unixFileName, "r");
  if (errno) {
    perror ("Error on UNIX file");
    fatalError ("The UNIX file could not be opened for reading");
  }

  if (commandOptionV) printf ("The UNIX file \"%s\" has been opened successfully.\n", unixFileName);

  /* Get the size of the UNIX file... */
  errno = 0;
  fseek (unixFile, 0l, SEEK_END);
  if (errno) {
    perror ("Error on UNIX file");
    fatalError ("Error during call to fseek");
  }
  longLength = ftell (unixFile);

  /* Check to see if the file is too big... */
  if (longLength > ((long) MAX)) {
    printf ("The maximum integer is %d\n", MAX);
    fatalError ("Error in UNIX File: The UNIX file size exceeds the maximum");
  }

  /* Print the length of the UNIX file */
  unixFileSizeInBytes = (int) longLength;
  unixFileSizeInSectors = (unixFileSizeInBytes + PAGE_SIZE-1) / PAGE_SIZE;
  if (commandOptionV) {
    printf ("The length of the UNIX file = %d bytes.\n", unixFileSizeInBytes);
    printf ("                            = %d sectors.\n", unixFileSizeInSectors);
  }

  ent = findFile (blitzFileName);

  if (ent) {
    if (commandOptionV) printf ("A file with this name already exists; updating it.\n");
    // We have now set "thisFileSizeInBytes" and "thisFileStartingSector"

    /* Make sure this file will fit into the space allocated on BLITZ disk. */
    s = ((ent->sizeInBytes) + PAGE_SIZE-1) / PAGE_SIZE;
    if (commandOptionV) printf ("The size of the BLITZ file = %d sectors.\n", s);
    if (s < unixFileSizeInSectors) {
      fatalError ("The existing file is too small; please delete it and re-add it");
    }

  } else {
    if (commandOptionV) printf ("A file with this name does not already exist; creating new file.\n");

    /* Create a new entry */
    ent = (Entry *) calloc (1, sizeof (Entry));
    ent->startingSector = nextFreeSector;
    ent->sizeInBytes = unixFileSizeInBytes;
    ent->lengthOfName = strlen (blitzFileName);
    ent->name = blitzFileName;

    /* Add it to the list */
    ent->next = 0;
    if (firstEntry) {
      lastEntry->next = ent;
      lastEntry = ent;
    } else {
      firstEntry = ent;
      lastEntry = ent;
    }

    /* Adjust the file statistics */
    nextFreeSector = nextFreeSector + unixFileSizeInSectors;
    numberOfFiles ++;
    sizeOfDirectoryInBytes += 12 + ent->lengthOfName;

    /* Make sure the directory has not grown too large to fit into sector 0. */
    if (sizeOfDirectoryInBytes > PAGE_SIZE) {
      fatalError ("The directory has grown too large to fit into sector 0");
    }

    /* Make sure this file will fit on the disk. */
    if (nextFreeSector > numberSectorsOnDisk) {
      fatalError ("This file will not fit onto the disk");
    }

    if (commandOptionV) {
      printf ("Size of directory info = %d bytes\n", sizeOfDirectoryInBytes);
      printf ("Size of this file = %d sectors\n", unixFileSizeInSectors);
      printf ("New total number of sectors used = %d\n", nextFreeSector);
      printf ("Number of free sectors left = %d\n", numberSectorsOnDisk - nextFreeSector);
      printf ("New number of files = %d\n", numberOfFiles);
      printDirectory ();
    }

    /* Write out the new directory */
    writeDirectory ();
  }

  /* Allocate a chunk of memory big enough to hold all of the source file. */
  memory = (char *) calloc (1, unixFileSizeInBytes);
  if (memory == 0) {
    fatalError ("Unable to allocate enough memory to hold the entire source file");
  }

  /* Reposition the source file to the beginning. */
  errno = 0;
  fseek (unixFile, 0l, SEEK_SET);
  if (errno) {
    perror ("Error on Unix file");
    fatalError ("The Unix file could not be repositioned");
  }

  /* Read the Unix file entirely into memory. */
  errno = 0;
  i = fread (memory, 1, unixFileSizeInBytes, unixFile);
  if (i != unixFileSizeInBytes) {
    if (errno) perror ("Error reading from Unix file");
    fatalError ("Problems reading from Unix file");
  }

  seekTo (4 + PAGE_SIZE * ent->startingSector);
  
  /* Write the data to the DISK file. */
  if (commandOptionV) printf ("Writing to DISK file (sector = %d)\n", ent->startingSector);
  errno = 0;
  i = fwrite (memory, 1, unixFileSizeInBytes, diskFile);
  if (i != unixFileSizeInBytes) {
    if (errno) perror ("Error writing to DISK file");
    fatalError ("Problems writing to DISK file");
  }

  closeFiles ();
}



/* extractCommand ()
**
** Copy a file from the BLITZ DISK file into a Unix file.
*/
void extractCommand () {
  Entry * ent;
  int i;
  char * memory;

  if (commandOptionV) {
    printf ("EXTRACTING A FILE FROM THE BLITZ DISK...\n");
    printf ("  blitzFileName = \'%s\'\n", blitzFileName);
    printf ("  unixFileName = \'%s\'\n", unixFileName);
  }

  openDiskFile (1);         /* existing = true */

  ent = findFile (blitzFileName);

  if (ent == NULL) {
    fatalError ("There is no file with this name on the BLITZ DISK");
  }

  if (commandOptionV) {
    printf ("This file starts at sector %d\n", ent->startingSector);
    printf ("Size of this file = %d bytes\n", ent->sizeInBytes);
  }

  // Open the UNIX file for updating...
  errno = 0;
  unixFile = fopen (unixFileName, "wa");
  if (errno) {
    perror ("Error on UNIX file");
    fatalError ("The UNIX file could not be opened for updating");
  }

  if (commandOptionV) printf ("The UNIX file \"%s\" has been opened successfully.\n", unixFileName);

  /* Allocate a chunk of memory big enough to hold all of the file. */
  memory = (char *) calloc (1, ent->sizeInBytes);
  if (memory == 0) {
    fatalError ("Unable to allocate enough memory to hold the entire file");
  }

  /* Reposition the UNIX file to the beginning. */
  errno = 0;
  fseek (unixFile, 0l, SEEK_SET);
  if (errno) {
    perror ("Error on UNIX file");
    fatalError ("The UNIX file could not be repositioned");
  }

  /* Reposition the BLITZ file to the correct sector. */
  seekTo (4 + PAGE_SIZE * ent->startingSector);

  /* Read the BLITZ file entirely into memory. */
  errno = 0;
  i = fread (memory, 1, ent->sizeInBytes, diskFile);
  if (i != ent->sizeInBytes) {
    if (errno) perror ("Error reading from BLITZ DISK file");
    fatalError ("Problems reading from BLITZ DISK file");
  }
  
  /* Write the data to the UNIX file. */
  if (commandOptionV) printf ("Writing to DISK file.\n");
  errno = 0;
  i = fwrite (memory, 1, ent->sizeInBytes, unixFile);
  if (i != ent->sizeInBytes) {
    if (errno) perror ("Error writing to UNIX file");
    fatalError ("Problems writing to UNIX file");
  }

  closeFiles ();

}



/* writeCommand ()
**
** Write data from a Unix file straight to some sectors on the BLITZ DISK.
*/
void writeCommand () {
  char * memory = NULL;             /* Pointer to a chunk of memory */
  int i, magic, unixFileLength, where, diskFileLength;
  long longLength;

  if (commandOptionV) {
    printf ("WRITING SECTORS TO THE BLITZ DISK...\n");
    printf ("  diskFileName = \'%s\'\n", diskFileName);
    printf ("  unixFileName = \'%s\'\n", unixFileName);
    printf ("  startingSectorNumber = %d\n", startingSectorNumber);
  }

  // Open the DISK file for updating...
  errno = 0;
  diskFile = fopen (diskFileName, "r+");
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("The DISK file could not be opened for updating");
  }

// qqqqq  PREVIOUSLY THE FOLLOWING PRINT WAS DISABLED FOR AUTOMATED TESTING...
  if (commandOptionV) printf ("The DISK file \"%s\" has been opened successfully.\n", diskFileName);

  /* Get the size of the DISK file... */
  errno = 0;
  fseek (diskFile, 0l, SEEK_END);
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("Error during call to fseek");
  }
  longLength = ftell (diskFile);

  /* Check to see if the file is too big... */
  if (longLength > ((long) MAX)) {
    printf ("The maximum integer is %d\n", MAX);
    fatalError ("Error in DISK File: The DISK file size exceeds the maximum");
  }

  /* Print the length of the DISK file */
  diskFileLength = (int) longLength;
  if (commandOptionV) printf ("The length of the DISK file is %d bytes.\n", diskFileLength);

  /* Check for a file that is too short... */
  if (diskFileLength < (SECTORS_PER_TRACK * PAGE_SIZE + 4)) {
    /*  printf ("The minimum DISK file size is 1 track + 4 bytes
                (where PageSize = %d bytes and SectorsPerTrack = %d).\n",
                PAGE_SIZE, SECTORS_PER_TRACK);  */
    fatalError ("Error in DISK file length: The file must be large enough for at least 1 track");
  }

  /* Check for bad file length... */
  numberSectorsOnDisk = (diskFileLength - 4) / PAGE_SIZE;
  if (numberSectorsOnDisk * PAGE_SIZE + 4 != diskFileLength) {
    /* printf ("PageSize = %d bytes, SectorsPerTrack = %d, DISK file size = %d.\n",
                PAGE_SIZE, SECTORS_PER_TRACK, diskFileLength);  */
    fatalError ("Error in DISK file length: The file size is not an even number of tracks plus 4 bytes");
  }

  if (commandOptionV) printf ("The number of sectors on the DISK = %d\n", numberSectorsOnDisk);

  /* Reposition the DISK file to the beginning. */
  errno = 0;
  fseek (diskFile, 0l, SEEK_SET);
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("The DISK file could not be repositioned");
  }

  /* Check the magic number. */
  magic = readInteger ();
  if (magic != 0x424C5A64) {
    fatalError ("Error in DISK File: Magic number is not 'BLZd'");
  }

  // Open the source file for updating...
  errno = 0;
  unixFile = fopen (unixFileName, "r");
  if (errno) {
    perror ("Error on source file");
    fatalError ("The source file could not be opened for reading");
  }

  if (commandOptionV) printf ("The source file \"%s\" has been opened successfully.\n", unixFileName);

  /* Get the size of the source file... */
  errno = 0;
  fseek (unixFile, 0l, SEEK_END);
  if (errno) {
    perror ("Error on source file");
    fatalError ("Error during call to fseek");
  }
  longLength = ftell (unixFile);

  /* Check to see if the file is too big... */
  if (longLength > ((long) MAX)) {
    printf ("The maximum integer is %d\n", MAX);
    fatalError ("Error in source File: The source file size exceeds the maximum");
  }

  /* Print the length of the source file */
  unixFileLength = (int) longLength;
  if (commandOptionV) printf ("The length of the source file is %d bytes.\n", unixFileLength);

  /* Allocate a chunk of memory big enough to hold all of the source file. */
  memory = (char *) calloc (1, unixFileLength);
  if (memory == 0) {
    fatalError ("Unable to allocate enough memory to hold the entire source file");
  }

  /* Reposition the source file to the beginning. */
  errno = 0;
  fseek (unixFile, 0l, SEEK_SET);
  if (errno) {
    perror ("Error on source file");
    fatalError ("The source file could not be repositioned");
  }

  /* Read the source file entirely into memory. */
  errno = 0;
  i = fread (memory, 1, unixFileLength, unixFile);
  if (i != unixFileLength) {
    if (errno) perror ("Error reading from source file");
    fatalError ("Problems reading from source file");
  }

  /* Compute where in the DISK file to write the data. */
  where = 4 + (startingSectorNumber * PAGE_SIZE);

  /* Check to see if this write will exceed the DISK file's length. */
  if (where + unixFileLength > diskFileLength) {
    fatalError ("The DISK file is not large enough to accomodate this write");
  }

  /* Reposition the DISK file to "where". */
  errno = 0;
  fseek (diskFile, (long) where, SEEK_SET);
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("The DISK file could not be could be repositioned");
  }

  /* Write the data to the DISK file. */
  if (commandOptionV) printf ("Writing to DISK file (address = %d)\n", where);
  errno = 0;
  i = fwrite (memory, 1, unixFileLength, diskFile);
  if (i != unixFileLength) {
    if (errno) perror ("Error writing to DISK file");
    fatalError ("Problems writing to DISK file");
  }

  /* Print how many sectors were involved. */
  if (commandOptionV) printf ("Number of sectors modified = %d\n", (unixFileLength + PAGE_SIZE-1) / PAGE_SIZE);

  /* Close the DISK file */
  errno = 0;
  fclose (diskFile);
  if (errno) {
    perror ("Error on DISK file");
    fatalError ("The DISK file could not be closed");
  }
  if (commandOptionV) printf ("The DISK file has been closed successfully.\n");

  /* Close the source file */
  errno = 0;
  fclose (unixFile);
  if (errno) {
    perror ("Error on source file");
    fatalError ("The source file could not be closed");
  }
  if (commandOptionV) printf ("The source file has been closed successfully.\n");

  /* Return no error */
  exit (0);

}



/* readInteger ()
**
** Read an integer (4-bytes, binary) from the BLITZ DISK file and return it.
*/
int readInteger () {
  int i, numItemsRead;
  errno = 0;
  numItemsRead = fread (&i, 4, 1, diskFile);
  if (numItemsRead != 1) {
    if (errno) perror ("Error when reading from file");
    fatalError ("Problem reading from file");
  }
  return SWAP_BYTES (i);
}



/* writeInteger (x)
**
** Write an integer (4-bytes, binary) to the BLITZ DISK file.
*/
void writeInteger (int x) {
  int i, x2;
  x2 = SWAP_BYTES (x);
  errno = 0;
  i = fwrite (&x2, 1, 4, diskFile);
  if (i != 4) {
    if (errno) perror ("Error writing to BLITZ DISK file");
    fatalError ("Problems writing to BLITZ DISK file");
  }
}



/* writeString (str, len)
**
** Write "len" chartacters to the BLITZ DISK file.
*/
void writeString (char * str, int len) {
  int i;
  while (len) {
    errno = 0;
    i = fwrite (str, 1, 1, diskFile);
    if (i != 1) {
      if (errno) perror ("Error writing to BLITZ DISK file");
      fatalError ("Problems writing to BLITZ DISK file");
    }
    len--;
    str++;
  }
}



/* readString (len) --> ptr to string
**
** Read "len" chartacters from the BLITZ DISK file.  Allocate memory;
** store the characters, and return a pointer to the memory.
*/
char * readString (int len) {
  int i;
  char * str = (char *) calloc (1, len+1);
  char * next = str;
  while (len) {
    errno = 0;
    i = fread (next, 1, 1, diskFile);
    if (i != 1) {
      if (errno) perror ("Error reading from BLITZ DISK file");
      fatalError ("Problems reading from BLITZ DISK file");
    }
    len--;
    next++;
  }
  *next = '\0';
  return str;
}



/* findFile (str) --> ent
**
** Look in the directory info and find this file.  Return a pointer to
** the entry, or NULL if not found.
*/
Entry * findFile (char * str) {
  Entry * ent = firstEntry;
  while (ent) {
    if (0 == strcmp (ent->name, str)) {
      return ent;
    }
    ent = ent->next;
  }
  return NULL;
}



/* seekTo (i)
**
** Seek to the given location on the BLITZ DISK file.
*/
void seekTo (int where) {
  errno = 0;
  fseek (diskFile, (long) where, SEEK_SET);
  if (errno) {
    perror ("Error on BLITZ DISK file");
    fatalError ("The BLITZ DISK file could not be could be repositioned");
  }
}



/* printDirectory ()
**
** Print the linked list of entries.
*/
void printDirectory () {
  Entry * e = firstEntry;
  printf ("   StartingSector   SizeInSectors    SizeInBytes        FileName\n");
  printf ("   ==============   =============    ===========    =====================\n");
  printf ("\t0\t\t1\t\t8192\t\t< directory >\n");
  while (e) {
    printf ("\t%d\t\t%d\t\t%d\t\t%s\n",
            e->startingSector,
            (e->sizeInBytes + PAGE_SIZE-1) / PAGE_SIZE,
            e->sizeInBytes,
            // e->lengthOfName,
            e->name);
    e = e->next;
  }
}



/* writeDirectory ()
**
** Print the directory back to the BLITZ DISK.
*/
void writeDirectory () {
  Entry * e = firstEntry;
  seekTo (8);
  writeInteger (numberOfFiles);
  writeInteger (nextFreeSector);
  while (e) {
    // printf ("writing next entry...\n");
    // printf ("\t%d\t\t%d\t\t%d\t\t%s\n",
    //         e->startingSector,
    //         e->sizeInBytes,
    //         e->lengthOfName,
    //         e->name);
    writeInteger (e->startingSector);
    writeInteger (e->sizeInBytes);
    writeInteger (e->lengthOfName);
    writeString (e->name, e->lengthOfName);
    e = e->next;
  }
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



/* checkArithmetic ()
**
** This routine checks to make sure the machine running this
** programs implements "int" integers with 32 bit arithmetic and
** doubles with 64 bit floating-point numbers.
*/
void checkArithmetic () {
  int x;
  x = 0x00005678;
  x = x << 16;
  x = x >> 16;
  if (x != 0x00005678) {
    fatalError ("This machine does not support 32 bit integer arithmetic");
  }
  if (sizeof(int) != 4) {
    fatalError ("This machine does not support 32 bit integer arithmetic");
  }
  if (sizeof(double) != 8) {
    fatalError ("This machine does not support 64 bit double precision");
  }
}



/* processCommandLine (argc, argv)
**
** This routine processes the command line options.
*/
void processCommandLine (int argc, char ** argv) {
  int argCount;
  int len;
  int gotGOption = 0;
  int gotRawOption = 0;
  int gotRandOption = 0;

  /* Each iteration of this loop looks at the next argument.  In some
     cases (like "-o a.out") one iteration will scan two tokens. */
  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
    argCount = 1;

    /* Scan the -h option */
    if (!strcmp (*argv, "-h")) {
      commandLineHelp ();
      exit (1);

    /* Scan the -d option, which should be followed by a file name */
    } else if (!strcmp (*argv, "-d")) {
      if (argc <= 1) {
        badOption ("Expecting filename after -d option");
      } else {
        argCount++;
        if (diskFileName == NULL) {
          diskFileName = *(argv+1);
        } else {
          badOption ("Invalid command line - multiple DISK files");
        }
      }

    /* Scan the -w option, which should be followed by a file name and an integer */
    } else if (!strcmp (*argv, "-w")) {
      if (commandOptionW) {
        badOption ("Invalid command line - multiple occurences of -w");
      } else {
        commandOptionW = 1;
        if (argc <= 1) {
          badOption ("Expecting UnixFileName after -w option");
        } else if (argc <= 2) {
          badOption ("Expecting SectorNumber after -w UnixFileName");
        } else {
          argCount++;
          argCount++;
          unixFileName = *(argv+1);
          startingSectorNumber = atoi (*(argv+2));  /* Extra chars after int ignored */
        }
      }

    /* Scan the -c option, which should be followed by a file name and an integer */
    } else if (!strcmp (*argv, "-c")) {
      if (commandOptionC) {
        badOption ("Invalid command line - multiple occurences of -c");
      } else {
        commandOptionC = 1;
        if (argc <= 1) {
          badOption ("Expecting BlitzFileName after -c option");
        } else if (argc <= 2) {
          badOption ("Expecting an integer (the size in bytes) after -c BlitzFileName");
        } else {
          argCount++;
          argCount++;
          blitzFileName = *(argv+1);
          sizeInBytes = atoi (*(argv+2));  /* Extra chars after int ignored */
        }
      }

    /* Scan the -a option, which should be followed by two file names */
    } else if (!strcmp (*argv, "-a")) {
      if (commandOptionA) {
        badOption ("Invalid command line - multiple occurences of -a");
      } else {
        commandOptionA = 1;
        if (argc <= 1) {
          badOption ("Expecting UnixFileName after -a option");
        } else if (argc <= 2) {
          badOption ("Expecting BlitzFileName after -a UnixFileName");
        } else {
          argCount++;
          argCount++;
          unixFileName = *(argv+1);
          blitzFileName = *(argv+2);
        }
      }

    /* Scan the -e option, which should be followed by two file names */
    } else if (!strcmp (*argv, "-e")) {
      if (commandOptionE) {
        badOption ("Invalid command line - multiple occurences of -e");
      } else {
        commandOptionE = 1;
        if (argc <= 1) {
          badOption ("Expecting BlitzFileName after -e option");
        } else if (argc <= 2) {
          badOption ("Expecting UnixFileName after -e BlitzFileName");
        } else {
          argCount++;
          argCount++;
          blitzFileName = *(argv+1);
          unixFileName = *(argv+2);
        }
      }

    /* Scan the -r option, which should be followed by a file name */
    } else if (!strcmp (*argv, "-r")) {
      if (commandOptionR) {
        badOption ("Invalid command line - multiple occurences of -r");
      } else {
        commandOptionR = 1;
        if (argc <= 1) {
          badOption ("Expecting FileName after -r option");
        } else {
          argCount++;
          blitzFileName = *(argv+1);
        }
      }

    /* Scan the -l option */
    } else if (!strcmp (*argv, "-l")) {
      if (commandOptionL) {
        badOption ("Invalid command line - multiple occurences of -l");
      } else {
        commandOptionL = 1;
      }

    /* Scan the -v option */
    } else if (!strcmp (*argv, "-v")) {
      if (commandOptionV) {
        badOption ("Invalid command line - multiple occurences of -v");
      } else {
        commandOptionV = 1;
      }

    /* Scan the -i option */
    } else if (!strcmp (*argv, "-i")) {
      if (commandOptionI) {
        badOption ("Invalid command line - multiple occurences of -i");
      } else {
        commandOptionI = 1;
      }

    /* Anything remaining else on the command line is invalid */
    } else {
      fprintf (
        stderr,
        "BLITZ Disk Utility Error: Invalid command line option (%s); Use -h for help display\n",
        *argv);
      exit (1);

    }
  }

  /* Figure out the name of the DISK file. */
  if (diskFileName == NULL) {
    diskFileName = "DISK";
  }

  /* Make sure that only one of -i, -l, -c, -r, -a, -e, -w was used. */
  if (commandOptionI +
      commandOptionL +
      commandOptionC +
      commandOptionR +
      commandOptionA +
      commandOptionE +
      commandOptionW != 1) {
    badOption ("Invalid command line - Must use exactly one of -i, -l, -c, -r, -a, -e, -w");
  }

}



/* badOption (msg)
**
** This routine prints a message on stderr and then aborts this program.
*/
void badOption (char * msg) {
  fprintf (stderr, "BLITZ Disk Utility Error: %s;  Use -h for help display\n", msg);
  exit (1);
}



/* fatalError (msg)
**
** This routine prints the given error message on stderr and calls exit(1).
*/
void fatalError (char * msg) {
  fprintf (stderr, "\nBLITZ Disk Utility Error: %s\n", msg);
  exit (1);
}



/* commandLineHelp ()
**
** This routine prints some documentation.  It is invoked whenever
** the -h option is used on the command line.
*/
void commandLineHelp () {
  printf (
"========================================\n"
"=====                              =====\n"
"=====  The BLITZ Disk Utility      =====\n"
"=====                              =====\n"
"========================================\n"
"\n"
"Copyright 2004-2007, Harry H. Porter III\n"
"========================================\n"
"  Original Author:\n"
"    10/07/04 - Harry H. Porter III\n"
"  Modifications by:\n"
"    04/30/07 - Harry H. Porter III - Support for little endian added\n"
"\n"
"This command can be used to manipulate the BLITZ \"DISK\" file.\n"
"\n"
"The BLITZ emulator simulates the BLITZ disk using a Unix file on the host\n"
"machine.  This program allows that file to be manipulated.  For example,\n"
"it can be used to copy an executable file containing a user program to the\n"
"BLITZ disk so that the BLITZ OS kernel can then access, load, and run it.\n"
"\n"
"The BLITZ DISK is organized as follows.  The disk contains a single directory\n"
"and this is kept in sector 0.  The files are placed sequentially on the\n"
"disk, one after the other.  Each file will take up an integral number of\n"
"sectors.  Each file has an entry in the directory.  Each entry contains\n"
"     (1)  The starting sector\n"
"     (2)  The file length, in bytes (possibly zero)\n"
"     (3)  The number of characters in the file name\n"
"     (4)  The file name\n"
"The directory begins with three numbers:\n"
"     (1)  Magic Number (0x73747562 = \"stub\")\n"
"     (2)  Number of files (possibly zero)\n"
"     (3)  Number of the next free sector\n"
"These are followed by the entries for each file.\n"
"\n"
"Once created, a BLITZ file may not have its size increased.  When a file is\n"
"removed, the free sectors become unusable; there is no compaction or any\n"
"attempt to reclaim the lost space.\n"
"\n"
"Each time this program is run, it performs one of the following functions:\n"
"      Initialize  set up a new file system on the BLITZ disk\n"
"      List        list the directory on the BLITZ disk\n"
"      Create      create a new file of a given size\n"
"      Remove      remove a file\n"
"      Add         copy a file from Unix to BLITZ\n"
"      Extract     copy a file from BLITZ to Unix\n"
"      Write       write sectors from a Unix file to the BLITZ disk\n"
"\n"
"The following command line options tell which function is to be performed:\n"
"\n"
"  -h\n"
"        Print this help info.  Ignore other options and exit.\n"
"  -d DiskFileName\n"
"        The file used to emulate the BLITZ disk.  If missing, \"DISK\" will be used.\n"
"  -i\n"
"        Initialize the file system on the BLITZ \"DISK\" file.  This will\n"
"        effectively remove all files on the BLITZ disk and reclaim all available\n"
"        space.\n"
"  -l\n"
"        List the directory on the BLITZ disk.\n"
"  -c BlitzFileName SizeInBytes\n"
"        Create a file of the given size on the BLITZ disk.  The BLITZ\n"
"        disk must not already contain a file with this name.  Only the\n"
"        directory will be modified; the actual data in the file will be\n"
"        whatever bytes happened to be on the disk already.\n"
"  -r BlitzFileName\n"
"        Remove the file with the given name from the directory on the BLITZ disk.\n"
"  -a UnixFilename BlitzFileName\n"
"        Copy a file from Unix to the BLITZ disk.  If BlitzFileName already\n"
"        exists, it must be large enough to accomodate the new data.\n"
"  -e BlitzFileName UnixFileName\n"
"        Extract a file from the BLITZ disk to Unix.  This command will copy\n"
"        the data from the BLITZ disk to a Unix file.  The Unix file may or may\n"
"        not already exist; its size will be shortened or lengthened as necessary.\n"
"  -w UnixFileName SectorNumber\n"
"        The UnixFileName must be an existing Unix file. The SectorNumber is an\n"
"        integer.  The Unix file data will be  written to the BLITZ disk, starting\n"
"        at sector SectorNumber.  The directory will not be modified.\n"
"  -v\n"
"        Verbose; print lots of messages.\n"
"\n"
"Only one of -i, -l, -c, -r, -a, -e, or -w may be used at a time.\n");

}
