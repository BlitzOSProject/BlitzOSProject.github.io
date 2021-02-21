/* The BLITZ Machine Emulator
**
** Copyright 2001-2007, Harry H. Porter III
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
**   02/05/01 - Harry H. Porter III
**
** Modifcations by:
**   09/28/04 - Harry - Fix bug with randomSeed, disk errno
**   11/26/04 - Harry - Fix bugs with serial input
**   03/15/06 - Harry - Renamed "SPANK" to "BLITZ"
**   04/27/07 - Harry - Support for little endian added
**   12/03/07 - Harry - Fix bug relating to EOF on stdin by calling clearerr()
**
** Please respect the coding and commenting style of this program.
**
*/

#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <math.h>
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



typedef struct TableEntry TableEntry;



/*****  Global variables *****/

#define PAGE_SIZE 8192            /* Page Size (8K) */
char * memory;                    /* Where machine memory is stored */
int currentMemoryLock = -1;       /* Address of word holding lock, or -1 */
int userRegisters [16];           /* User regs r0, r1, ... r15 */
int systemRegisters [16];         /* System regs r0, r1, ... r15 */
double floatRegisters [16];       /* Floating regs f0, f1, ... f15 */
int pc;                           /* The current program counter */
int ptbr;                         /* The page table base register */
int ptlr;                         /* The page table length register */
int statusN;                      /* Status register, N bit */
int statusV;                      /* Status register, V bit */
int statusZ;                      /* Status register, Z bit */
int statusP;                      /* Status register, P bit */
int statusS;                      /* Status register, S bit */
int statusI;                      /* Status register, I bit */
int instruction;                  /* The current instruction being executed */
int commandOptionG = 0;           /* Automatically begin emulation, bypass interface */
int commandOptionRaw = 0;         /* Prefer "raw" mode (default is "cooked") */
int commandOptionWait = 0;        /* On 'wait' instruction, with for input */
int commandOptionRand = 0;        /* Set if "-r" was on command line */
int randSeedFromOption;           /* The value following "-r" */
int randomSeed;                   /* The running pseudo-random number */
char * executableFileName = NULL; /* The a.out filename */
FILE * executableFile;            /* The a.out file */
char * diskFileName = NULL;       /* The DISK filename */
FILE * diskFile = NULL;           /* The DISK file, possibly NULL */
char * termInputFileName = NULL;  /* The TERMINPUT filename */
FILE * termInputFile;             /* The TERMINPUT file */
char * termOutputFileName = NULL; /* The TERMOUTPUT filename */
FILE * termOutputFile;            /* The TERMOUTPUT file */
int textAddr;                     /* Starting addr of .text segment */
int dataAddr;                     /* Starting addr of .data segment */
int bssAddr;                      /* Starting addr of .bss segment */
int textSize;                     /* Sum of all .text segment sizes */
int dataSize;                     /* Sum of all .data segment sizes */
int bssSize;                      /* Sum of all .bss segment sizes */
char * pmPtr;
int pmSize;
int pmCount;
int pmRow[16];
int currentAddr;                  /* This is used in commandDis */
int currentFrame;                 /* This is used in commandStackUp, commandStackDown */
char inputBuffer [100];           /* Used for user input */
int interruptsSignaled;           /* 0=none; otherwise one bit per type */
int systemTrapNumber;             /* Set by the syscall exception */
int pageInvalidOffendingAddress;  /* Used by Page Invalid exception */
int pageReadonlyOffendingAddress; /* Used by Page Readonly exception */
int translateCausedException;     /* Set by translate () */
int currentTime;                  /* Measured in "clicks": 1,2,3,... */
int timeSpentAsleep;              /* Incremented by "wait" instruction */
int timeOfNextEvent;              /* Minimum of disk, serial, and timer */
int timeOfNextTimerEvent;         /* Zero = now, MAX = never */
int timeOfNextDiskEvent;          /* Zero = now, MAX = never */
int timeOfNextSerialInEvent;      /* Zero = now, MAX = never */
int timeOfNextSerialOutEvent;     /* Zero = now, MAX = never */
int executionHalted;              /* 0=continue executing; 1=halted */
int controlCPressed;              /* 0=not pressed; 1=pressed */
int wantPrintingInSingleStep;     /* Used by singleStep to control verbosity. */
int q;                            /* Used by the divide() routine */
int r;                            /* Used by the divide() routine */
double POSITIVE_INFINITY;         /* Effectively a constant */
double NEGATIVE_INFINITY;         /* Effectively a constant */
int terminalInRawMode;            /* 1=stdin is in "raw mode" for BLITZ input */
int terminalWantRawProcessing;    /* 1=user wants raw, unbuffered input */
int termInChar;                   /* The input character, zero if none */
int termInCharAvail;              /* Set when key pressed, clr'd when queried */
int termInCharWasUsed;            /* True after BLITZ program looks at char */
int termOutputReady;              /* 1=busy, 0=not busy */
#define TYPE_AHEAD_BUFFER_SIZE 100
                                  /* Characters to be supplied to serial in */
char typeAheadBuffer [TYPE_AHEAD_BUFFER_SIZE];
int typeAheadBufferCount;         /* Number of characters in the buffer */
int typeAheadBufferIn;            /* Next pos to put into */
int typeAheadBufferOut;           /* Next pos to get from */




/*****  Simulation Constants  *****
**
** The following variables are "constants" in the sense that they remain fixed
**   during instruction emulation.  They are set at startup time and when the
**   "reset" command is executed.  If a file called ".blitzrc" exists, their
**   values will be obtained from this; otherwise they will be given default
**   values.  The default values are in the "setSimulationConstants()" routine.
**
** All times are in terms of instruction "cycles."  Except for interrupts, there
**   will be one instruction executed per cycle.  For a 1MHz machine, each
**   cycle corresponds to 1 microsecond.
**    
** To simulate human typing on the serial input, we might use
**     KEYBOARD_WAIT_TIME = 1000    KEYBOARD_WAIT_TIME_VARIATION = 100000
** To simulate an output rate of 19 KBaud on the serial output, we might use
**     TERM_OUT_DELAY = 500         TERM_OUT_DELAY_VARIATION = 10
** To switch processes about 200 times per second, we might use
**     TIME_SLICE = 5000            TIME_SLICE_VARIATION = 10
**   (To turn off Timer Interrupts, use TIME_SLICE = 0.)
** DISK_SEEK_TIME is the time to move the disk head one track.
** DISK_SETTLE_TIME is the time to wait after any head movement.
** DISK_ROTATIONAL_DELAY is the time for the disk to advance 1 sector.
**   This value must be at least 1.  Transfer time is the rotational delay
**   times the number of sectors.  (Note: we do not model the time for track
**   seek and settle during a multi-sector transfer.)
** Disk access is the sum of seek, settle, rotational delay and transfer time.
** There can be a random variation added onto this, given by DISK_ACCESS_VARIATION.
**
** DISK_READ_ERROR_PROBABILITY is the probability that an I/O error will occur
**   on any one read operation.  It is given as a number like 500, which means
**   1 error in every 500.  A value of 0 means errors never occur.
** DISK_WRITE_ERROR_PROBABILITY is the probability that an I/O error will occur
**   on any one write operatiorn.
**
** The MEMORY_SIZE is in bytes.  Typically, this would be something like 16Mbytes.
**
** The "Memory Mapped Area" is a region of memory that behaves differently than
**   normaly memory; when the program reads or writes bytes in this region, the exact
**   behavior depends on the I/O device specification.  The MEMORY_MAPPED_AREA_LOW
**   is the lowest address and the MEMORY_MAPPED_AREA_HIGH is the highest address
**   in this region.  (E.g., 0x00ffff00, 0x00ffffff).
**
** The various words in the emory Mapped Area have specific addresses given by
**   SERIAL_STATUS_WORD_ADDRESS
**   SERIAL_DATA_WORD_ADDRESS
**   DISK_STATUS_WORD_ADDRESS
**   DISK_COMMAND_WORD_ADDRESS
**   DISK_MEMORY_ADDRESS_REGISTER
**   DISK_SECTOR_NUMBER_REGISTER
**   DISK_SECTOR_COUNT_REGISTER
**  
*/

int KEYBOARD_WAIT_TIME = -1;
int KEYBOARD_WAIT_TIME_VARIATION = -1;
int TERM_OUT_DELAY = -1;
int TERM_OUT_DELAY_VARIATION = -1;
int TIME_SLICE = -1;
int TIME_SLICE_VARIATION = -1;
int DISK_SEEK_TIME = -1;
int DISK_SETTLE_TIME = -1;
int DISK_ROTATIONAL_DELAY = -1;
int DISK_ACCESS_VARIATION = -1;
int DISK_READ_ERROR_PROBABILITY = -1;
int DISK_WRITE_ERROR_PROBABILITY = -1;
int INIT_RANDOM_SEED = -1;
int MEMORY_SIZE = -1;
int MEMORY_MAPPED_AREA_LOW = -1;
int MEMORY_MAPPED_AREA_HIGH = -1;
int SERIAL_STATUS_WORD_ADDRESS = -1;
int SERIAL_DATA_WORD_ADDRESS = -1;
int DISK_STATUS_WORD_ADDRESS = -1;
int DISK_COMMAND_WORD_ADDRESS = -1;
int DISK_MEMORY_ADDRESS_REGISTER = -1;
int DISK_SECTOR_NUMBER_REGISTER = -1;
int DISK_SECTOR_COUNT_REGISTER = -1;



/*****  The disk simulation  *****
**
** The disk is divided into sectors, whose size matches the
** "PAGE_SIZE" exaclty (i.e., 8Kbytes).  The disk is assumed to have
** one surface with a single disk head.  In other words, there
** is only 1 track per cylinder.  Hereafter, we speak only of
** tracks and ignore the concept of cylinders.  Each track is
** assumed  to have 16 sectors, giving 128Kbytes per track
** (= SECTORS_PER_TRACK * PAGE_SIZE).
**
** The size of the disk is some multiple of tracks, > 0.  The
** actual size is determined by the size of the disk file.  When
** the file is opened, we determine the "diskTrackCount" and the
** "diskSectorCount" (which is 16 times greater).
**
** At any time time, the disk has a current rotational position
** "currentDiskSector" which is a sector number between 0 and
** SECTORS_PER_TRACK-1.
**
** The status of the disk is given by two variables:
**    currentDiskStatus
**    futureDiskStatus
** Whenever a disk operation is called for, it is done immediately
** during a single insrtuction cycle.  However, the simulation calls
** for the disk to reamin busy for a while.  Thus, the current status
** will be busy, and a future status (eg, completedOK or completedWithError)
** will be determined.  An event will be scheduled.  At that event,
** the status will be changed to the new status, and an interrupt will
** be signalled.
**
** The possible values for these status variables are:
**    DiskBusy                      0x00000000
**    OperationCompletedOK          0x00000001
**    OperationCompletedWithError1  0x00000002
**    OperationCompletedWithError2  0x00000003
**    OperationCompletedWithError3  0x00000004
**    OperationCompletedWithError4  0x00000005
**    OperationCompletedWithError5  0x00000006
**
** All disk operations are in terms of a sector number, between
** 0 and diskSectorCount-1.  When a disk operation is requested, we
** will determine the delay, based on the current position of the
** head (i.e., which track it is on), and the current rotational
** angle of the disk platter (i.e., which sector within the track
** it is on).
**
** The emulator will then go to the DISK file and perform the
** operation.  Finally, it will schedule a "disk event" to initiate
** the interrupt that must be signaled when the disk operation is
** supposed to complete.  In the case of a disk read operation, we
** also mark the words in memory so the memory manager can signal an
** error if the BLITZ program happens to read or write them while the
** disk operation is in progress.  These are given by "diskBufferLow"
** and "diskBufferHigh".
**
*/
#define SECTORS_PER_TRACK                           16

#define DISK_BUSY                           0x00000000
#define OPERATION_COMPLETED_OK              0x00000001
#define OPERATION_COMPLETED_WITH_ERROR_1    0x00000002
#define OPERATION_COMPLETED_WITH_ERROR_2    0x00000003
#define OPERATION_COMPLETED_WITH_ERROR_3    0x00000004
#define OPERATION_COMPLETED_WITH_ERROR_4    0x00000005
#define OPERATION_COMPLETED_WITH_ERROR_5    0x00000006

#define DISK_READ_COMMAND                   0x00000001
#define DISK_WRITE_COMMAND                  0x00000002


int diskTrackCount = 0;                          /* The size of the disk */
int diskSectorCount = 0;                         /* The size of the disk */
int currentDiskSector = 0;                       /* The current disk position */
int currentDiskStatus = OPERATION_COMPLETED_OK;  /* Current status */
int futureDiskStatus = OPERATION_COMPLETED_OK;   /* What to become upon next disk event */
int diskBufferLow = 0;                           /* Used to error-check memory accesses */
int diskBufferHigh = 0;                          /* Used to error-check memory accesses */
int diskMemoryAddressRegister = 0x00000000;      /* What to read from memory-mapped area */
int diskSectorNumberRegister = 0x00000000;       /* What to read from memory-mapped area */
int diskSectorCountRegister = 0x00000000;        /* What to read from memory-mapped area */
int numberOfDiskReads = 0;                       /* How many since last reset (not errors) */
int numberOfDiskWrites = 0;                      /* How many since last reset (not errors) */



/*****  Memory-Mapped I/O Addresses  *****
**
** A special region is physical memory is "memory-mapped I/O".
** Loads and stores to this region go directly to/from the various
** I/O devices, rather than the memory.  These are the locations of
** various special I/O registers that are mapped into the physical
** memory address space.
**                                  default value
**                                  =============
**     MEMORY_MAPPED_AREA_LOW         0x00ffff00
**     MEMORY_MAPPED_AREA_HIGH        0x00ffffff
**
**     SERIAL_STATUS_WORD_ADDRESS     0x00ffff00
**     SERIAL_DATA_WORD_ADDRESS       0x00ffff04
**
**     DISK_STATUS_WORD_ADDRESS       0x00ffff08
**     DISK_COMMAND_WORD_ADDRESS      0x00ffff08
**     DISK_MEMORY_ADDRESS_REGISTER   0x00ffff0c
**     DISK_SECTOR_NUMBER_REGISTER    0x00ffff10
**     DISK_SECTOR_COUNT_REGISTER     0x00ffff14
*/



/*****  Interrupt Codes  *****
**
** These codes are used to set and test bits in the
** interruptSignaled flag.  See also
**    getNextInterrupt
**    getVectorNumber
*/
#define POWER_ON_RESET                      1
#define TIMER_INTERRUPT                     2
#define DISK_INTERRUPT                      4
#define SERIAL_INTERRUPT                    8
#define HARDWARE_FAULT                     16
#define ILLEGAL_INSTRUCTION                32
#define ARITHMETIC_EXCEPTION               64
#define ADDRESS_EXCEPTION                 128
#define PAGE_INVALID_EXCEPTION            256
#define PAGE_READONLY_EXCEPTION           512
#define PRIVILEGED_INSTRUCTION           1024
#define ALIGNMENT_EXCEPTION              2048
#define EXCEPTION_DURING_INTERRUPT       4096
#define SYSCALL_TRAP                     8192


#define MAX 2147483647
int MIN;                 /* Initialized in main to -2147483648 */


/*****  Label Table  *****
**
** Each label is an identifier (consisting of letters, digits, underscores,
** and periods).  Each label has a value, which is a 32-bit integer.
** Label values are absolute, not relative, at run-time.  Each label-value
** pair is represented with a "TableEntry".  The TableEntries are indexed
** in two ways.  First, by name:  There is an array ("alphaIndex") which
** contains pointers to TableEntries.  Second, by value:  There is an array
** ("valueIndex") which contains pointers to TableEntries.  Both arrays will
** have the same number of elements, namely, "numberOfLabels", and both
** arrays are kept in sorted order.
*/
struct TableEntry {
  int   value;
  char  string [0];       /* The characters, including \0 at end */
};

#define MAX_NUMBER_OF_LABELS 500000
TableEntry * alphaIndex [MAX_NUMBER_OF_LABELS];
TableEntry * valueIndex [MAX_NUMBER_OF_LABELS];
int numberOfLabels = 0;



/*****  Statement Codes  *****
**
** These codes are used to identify the various KPL source code
** statements.  These values are placed in memory by the compiler.
** Each value is a 32-bit value, encoding 2 ascii characters, e.g.
**      '\0\0AS'
*/
#define STMT_CODE_AS   ('A' * 256 + 'S')   /* ASSIGNMENT statement                */
#define STMT_CODE_FU   ('F' * 256 + 'U')   /* FUNCTION ENTRY                      */
#define STMT_CODE_ME   ('M' * 256 + 'E')   /* METHOD ENTRY                        */
#define STMT_CODE_IF   ('I' * 256 + 'F')   /* IF statement                        */
#define STMT_CODE_TN   ('T' * 256 + 'N')   /* THEN statement                      */
#define STMT_CODE_EL   ('E' * 256 + 'L')   /* ELSE statement                      */
#define STMT_CODE_CA   ('C' * 256 + 'A')   /* FUNCTION CALL                       */
#define STMT_CODE_CF   ('C' * 256 + 'F')   /* FUNCTION CALL (via pointer)         */
#define STMT_CODE_CE   ('C' * 256 + 'E')   /* FUNCTION CALL (external function)   */
#define STMT_CODE_SE   ('S' * 256 + 'E')   /* SEND                                */
#define STMT_CODE_WH   ('W' * 256 + 'H')   /* WHILE LOOP (expr evaluation)        */
#define STMT_CODE_WB   ('W' * 256 + 'B')   /* WHILE LOOP (body statements)        */
#define STMT_CODE_DO   ('D' * 256 + 'O')   /* DO   ('UNTIL (body statements)      */
#define STMT_CODE_DU   ('D' * 256 + 'U')   /* DO   ('UNTIL (expr evaluation)      */
#define STMT_CODE_BR   ('B' * 256 + 'R')   /* BREAK statement                     */
#define STMT_CODE_CO   ('C' * 256 + 'O')   /* CONTINUE statement                  */
#define STMT_CODE_RE   ('R' * 256 + 'E')   /* RETURN statement                    */
#define STMT_CODE_FO   ('F' * 256 + 'O')   /* FOR statement                       */
#define STMT_CODE_FB   ('F' * 256 + 'B')   /* FOR (body statements)               */
#define STMT_CODE_SW   ('S' * 256 + 'W')   /* SWITCH                              */
#define STMT_CODE_TR   ('T' * 256 + 'R')   /* TRY statement                       */
#define STMT_CODE_TH   ('T' * 256 + 'H')   /* THROW statement                     */
#define STMT_CODE_FR   ('F' * 256 + 'R')   /* FREE statement                      */
#define STMT_CODE_DE   ('D' * 256 + 'E')   /* DEBUG statement                     */
#define STMT_CODE_CC   ('C' * 256 + 'C')   /* CATCH clause                        */



/*****  Function prototypes  *****/

int main (int argc, char ** argv);
void printFinalStats ();
void checkHostCompatibility ();
void checkArithmetic ();
int isNegZero (double d);
void processCommandLine (int argc, char ** argv);
void badOption (char * msg);
void errorExit ();
void fatalError (char * msg);
void commandLineHelp ();
char * getToken ();
char * trimToken (char * str);
char * toLower (char * str);
int readInteger (FILE * file);
int readByte (FILE * file);
int roundUpToMultipleOf (int i, int p);
void printMemory (char * ptr, int n, int addr);
void get16Bytes ();
int getNextByte ();
void putlong (int i);
void puthalf (int i);
void printline ();
void printByte (int c);
int bytesEqual (char * p, char * q, int lengthP, int lengthQ);
void commandHelp ();
void commandAllRegs ();
void commandAllFloatRegs ();
void printDouble (double d);
void printDoubleVal (double d);
void commandInfo ();
void printPendingInterrupts ();
void commandIO ();
void commandSim ();
void commandRaw ();
void commandCooked ();
void printSerialHelp ();
void commandFormat ();
void commandTest ();
void printNumberNL2 (int i);
void printNumberNL (int i);
void commandDumpMemory ();
void commandSetMemory ();
void commandWriteWord ();
void commandReadWord ();
void commandInput ();
void addToTypeAhead (int ch);
void printTypeAheadBuffer ();
void fancyPrintChar (int ch);
int readHexInt ();
int readDecimalInt ();
double readDouble ();
int readYesNo ();
void commandSetI ();
void commandSetS ();
void commandSetP ();
void commandSetZ ();
void commandSetV ();
void commandSetN ();
void commandClearI ();
void commandClearS ();
void commandClearP ();
void commandClearZ ();
void commandClearV ();
void commandClearN ();
void commandSetPC ();
void commandSetPTBR ();
void commandSetPTLR ();
void commandPrintPageTable ();
void commandTranslate ();
void commandCancel ();
void commandLabels ();
void printStringInWidth (char * string, int width);
void commandInit ();
void commandInit2 ();
void commandSort ();
void commandSort2 ();
void quicksortAlpha (int m, int n);
int partitionAlpha (int l, int r);
void quicksortValue (int m, int n);
int partitionValue (int l, int r);
void commandFind ();
void commandFind2 ();
int findLabel (int value);
int findLabel2 (int value, int low, int high);
TableEntry * findLabelByAlpha (char * string, int low, int high);
void commandAdd ();
void insertLabel (TableEntry * p);
void insertLabelUnsorted (TableEntry * p);
void commandReset ();
void resetState ();
void printInfo ();
void commandReg (int reg);
void commandFloatReg (int reg);
void commandFMem ();
void commandDis ();
void commandDis2 ();
void disassemble (int addr);
void printRa (int instr);
void printRb (int instr);
void printRc (int instr);
void printFRa (int instr);
void printFRb (int instr);
void printFRc (int instr);
void printData16 (int instr);
void printData24 (int instr);
void printComment (int i);
void printComment2 (int i);
void printAboutToExecute ();
void commandStepN ();
void commandGo (int count);
void commandStep ();
void commandStepHigh ();
void commandStepHigh2 ();
void singleStep ();
int getRa (int instr);
int getRb (int instr);
int getRc (int instr);
void putRc (int instr, int value);
int getData16 (int instr);
int getData24 (int instr);
double getFRa (int instr);
double getFRb (int instr);
double getFRc (int instr);
void putFRc (int instr, double value);
int buildStatusWord ();
void setStatusFromWord (int word);
void setSR (int value, int overflow);
int getNextInterrupt ();
int getVectorNumber ();
int physicalAddressOk (int addr);
int isAligned (int addr);
int getPhysicalWord (int addr);
int getPhysicalWordAndLock (int addr);
void putPhysicalWord (int addr, int value);
void putPhysicalWordAndRelease (int addr, int value);
void releaseMemoryLock (int physAddr);
int inMemoryMappedArea (int addr);
int translate (int addr, int reading, int wantPrinting, int doUpdates);
int getMemoryMappedWord (int physAddr);
void putMemoryMappedWord (int physAddr, int value);
void pushOntoStack (int reg, int value);
int popFromStack (int reg);
void commandShowStack ();
void commandFrame ();
void commandStackUp ();
void commandStackDown ();
int printFrame (int frameNumber, int longPrint);
int printAsciiDataInWidth (int ptr, int width);
void commandHex ();
void commandDecimal ();
void commandAscii ();
void printHexDecimalAscii (int i);
void jumpIfTrueRaRb (int cond, int instr);
void jumpIfTrueData24 (int cond, int instr);
void controlC (int sig);
int randomBetween (int lo, int high);
int genRandom ();
void doTimerEvent ();
void doDiskEvent ();
void doSerialInEvent (int waitForKeystroke);
void doSerialOutEvent ();
void updateTimeOfNextEvent ();
void divide (int a, int b);
void turnOnTerminal ();
void turnOffTerminal ();
char checkForInput (int waitForKeystroke);
int characterAvailableOnStdin ();
void initializeDisk ();
void performDiskIO (int command);
void checkDiskBufferError (int physAddr);
void setSimulationConstants ();
void defaultSimulationConstants ();
void printKPLStmtCode ();
void printCurrentFileLineAndFunction ();


/* main()
**
** Scan the command line arguments, then enter into the command loop.
*/
main (int argc, char ** argv) {
  int i;
  char * command;

  checkHostCompatibility ();
  checkArithmetic ();
  MIN = -MAX - 1;
  POSITIVE_INFINITY = 1.0 / 0.0;
  NEGATIVE_INFINITY = -1.0 / 0.0;
  processCommandLine (argc, argv);
  terminalInRawMode = 0;
  resetState ();
  signal (SIGINT, controlC);

  /* If the "auto go" option (-g) was given, the just begin execution. */
  if (commandOptionG) {
    commandGo (MAX);
    if (executionHalted) {
      printFinalStats ();
      exit (0);
    } else {
      fprintf (stderr, "\n\rEntering machine-level debugger...\n\r");
    }
  }

  printf (
"======================================================\n"
"=====                                            =====\n"
"=====         The BLITZ Machine Emulator         =====\n"
"=====                                            =====\n"
"=====  Copyright 2001-2007, Harry H. Porter III  =====\n"
"=====                                            =====\n"
"======================================================\n"
"\n"
"Enter a command at the prompt.  Type 'quit' to exit or 'help' for\n"
"info about commands.\n");

  /* Each execution of this loop will read and execute one command. */
  while (1) {
    controlCPressed = 0;  /* Forget about any control-C's */

    printf ("> ");
    command = toLower (getToken ());

    if (!strcmp (command, "quit") || !strcmp (command, "q")) {
      printFinalStats ();
      exit (0);
    } else if (!strcmp (command, "")) {
      // printf ("BLITZ Emulator: You are in the command-line interface.\n");
      /* ignore empty commands */
    } else if (!strcmp (command, "help") || !strcmp (command, "h")) {
      commandHelp ();
    } else if (!strcmp (command, "info") || !strcmp (command, "i")) {
      commandInfo ();
    } else if (!strcmp (command, "io")) {
      commandIO ();
    } else if (!strcmp (command, "sim")) {
      commandSim ();
    } else if (!strcmp (command, "raw")) {
      commandRaw ();
    } else if (!strcmp (command, "cooked")) {
      commandCooked ();
    } else if (!strcmp (command, "format")) {
      commandFormat ();
    } else if (!strcmp (command, "go") || !strcmp (command, "g")) {
      commandGo (MAX);
    } else if (!strcmp (command, "step") || !strcmp (command, "s")) {
      commandStep ();
    } else if (!strcmp (command, "t")) {
      commandStepHigh ();
    } else if (!strcmp (command, "u")) {
      commandStepHigh2 ();
    } else if (!strcmp (command, "stepn")) {
      commandStepN ();
    } else if (!strcmp (command, "dumpmem") || !strcmp (command, "dm")) {
      commandDumpMemory ();
    } else if (!strcmp (command, "setmem")) {
      commandSetMemory ();
    } else if (!strcmp (command, "read")) {
      commandReadWord ();
    } else if (!strcmp (command, "write")) {
      commandWriteWord ();
    } else if (!strcmp (command, "input")) {
      commandInput ();
    } else if (!strcmp (command, "seti")) {
      commandSetI ();
    } else if (!strcmp (command, "sets")) {
      commandSetS ();
    } else if (!strcmp (command, "setp")) {
      commandSetP ();
    } else if (!strcmp (command, "setz")) {
      commandSetZ ();
    } else if (!strcmp (command, "setv")) {
      commandSetV ();
    } else if (!strcmp (command, "setn")) {
      commandSetN ();
    } else if (!strcmp (command, "cleari")) {
      commandClearI ();
    } else if (!strcmp (command, "clears")) {
      commandClearS ();
    } else if (!strcmp (command, "clearp")) {
      commandClearP ();
    } else if (!strcmp (command, "clearz")) {
      commandClearZ ();
    } else if (!strcmp (command, "clearv")) {
      commandClearV ();
    } else if (!strcmp (command, "clearn")) {
      commandClearN ();
    } else if (!strcmp (command, "setpc")) {
      commandSetPC ();
    } else if (!strcmp (command, "setptbr")) {
      commandSetPTBR ();
    } else if (!strcmp (command, "setptlr")) {
      commandSetPTLR ();
    } else if (!strcmp (command, "pt")) {
      commandPrintPageTable ();
    } else if (!strcmp (command, "trans")) {
      commandTranslate ();
    } else if (!strcmp (command, "cancel")) {
      commandCancel ();
    } else if (!strcmp (command, "add")) {
      commandAdd ();
/***
    } else if (!strcmp (command, "init")) {
      commandInit ();
    } else if (!strcmp (command, "init2")) {
      commandInit2 ();
    } else if (!strcmp (command, "sort")) {
      commandSort ();
    } else if (!strcmp (command, "sort2")) {
      commandSort2 ();
***/
    } else if (!strcmp (command, "labels")) {
      commandLabels ();
    } else if (!strcmp (command, "find")) {
      commandFind ();
    } else if (!strcmp (command, "find2")) {
      commandFind2 ();
    } else if (!strcmp (command, "reset")) {
      commandReset ();
    } else if (!strcmp (command, "float") || !strcmp (command, "f")) {
      commandAllFloatRegs (1);
    } else if (!strcmp (command, "r")) {
      commandAllRegs (1);
    } else if (!strcmp (command, "r1")) {
      commandReg (1);
    } else if (!strcmp (command, "r2")) {
      commandReg (2);
    } else if (!strcmp (command, "r3")) {
      commandReg (3);
    } else if (!strcmp (command, "r4")) {
      commandReg (4);
    } else if (!strcmp (command, "r5")) {
      commandReg (5);
    } else if (!strcmp (command, "r6")) {
      commandReg (6);
    } else if (!strcmp (command, "r7")) {
      commandReg (7);
    } else if (!strcmp (command, "r8")) {
      commandReg (8);
    } else if (!strcmp (command, "r9")) {
      commandReg (9);
    } else if (!strcmp (command, "r10")) {
      commandReg (10);
    } else if (!strcmp (command, "r11")) {
      commandReg (11);
    } else if (!strcmp (command, "r12")) {
      commandReg (12);
    } else if (!strcmp (command, "r13")) {
      commandReg (13);
    } else if (!strcmp (command, "r14")) {
      commandReg (14);
    } else if (!strcmp (command, "r15")) {
      commandReg (15);
    } else if (!strcmp (command, "f0")) {
      commandFloatReg (0);
    } else if (!strcmp (command, "f1")) {
      commandFloatReg (1);
    } else if (!strcmp (command, "f2")) {
      commandFloatReg (2);
    } else if (!strcmp (command, "f3")) {
      commandFloatReg (3);
    } else if (!strcmp (command, "f4")) {
      commandFloatReg (4);
    } else if (!strcmp (command, "f5")) {
      commandFloatReg (5);
    } else if (!strcmp (command, "f6")) {
      commandFloatReg (6);
    } else if (!strcmp (command, "f7")) {
      commandFloatReg (7);
    } else if (!strcmp (command, "f8")) {
      commandFloatReg (8);
    } else if (!strcmp (command, "f9")) {
      commandFloatReg (9);
    } else if (!strcmp (command, "f10")) {
      commandFloatReg (10);
    } else if (!strcmp (command, "f11")) {
      commandFloatReg (11);
    } else if (!strcmp (command, "f12")) {
      commandFloatReg (12);
    } else if (!strcmp (command, "f13")) {
      commandFloatReg (13);
    } else if (!strcmp (command, "f14")) {
      commandFloatReg (14);
    } else if (!strcmp (command, "f15")) {
      commandFloatReg (15);
    } else if (!strcmp (command, "dis")) {
      commandDis ();
    } else if (!strcmp (command, "d")) {
      commandDis2 ();
    } else if (!strcmp (command, "dec")) {
      commandDecimal ();
    } else if (!strcmp (command, "ascii")) {
      commandAscii ();
    } else if (!strcmp (command, "hex")) {
      commandHex ();
    } else if (!strcmp (command, "fmem")) {
      commandFMem ();
    } else if (!strcmp (command, "stack") || !strcmp (command, "st")) {
      commandShowStack ();
    } else if (!strcmp (command, "frame") || !strcmp (command, "fr")) {
      commandFrame ();
    } else if (!strcmp (command, "up")) {
      commandStackUp ();
    } else if (!strcmp (command, "down")) {
      commandStackDown ();
    } else {
      printf ("Unrecognized command.\n");
      printf ("Enter a command at the prompt.  Type 'quit' to exit or 'help' for info about commands.\n");
    }
  }
}



/* printFinalStats ()
**
** This routine prints the final statistics.
*/
void printFinalStats () {
      printf ("Number of Disk Reads    = %d\n", numberOfDiskReads);
      printf ("Number of Disk Writes   = %d\n", numberOfDiskWrites);
      printf ("Instructions Executed   = %d\n", currentTime-timeSpentAsleep);
      printf ("Time Spent Sleeping     = %d\n", timeSpentAsleep);
      printf ("    Total Elapsed Time  = %d\n", currentTime);
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



/* isNegZero (double)  -->  bool
**
** This routine returns true if the double is negative-zero.
*/
int isNegZero (double d) {
  int * p;
  p = (int *) &d;
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  if ((*p == 0x00000000) && (*(++p) == 0x80000000)) {
#else
  if ((*p == 0x80000000) && (*(++p) == 0x00000000)) {
#endif
    return 1;
  } else {
    return 0;
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
  int gotWaitOption = 0;

  /* Each iteration of this loop looks at the next argument.  In some
     cases (like "-o a.out") one iteration will scan two tokens. */
  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
    argCount = 1;

    /* Scan the -h option */
    if (!strcmp (*argv, "-h")) {
      commandLineHelp ();
      exit (1);

    /* Scan the -g option */
    } else if (!strcmp (*argv, "-g")) {
      commandOptionG = 1;
      if (gotGOption) {
        badOption ("Multiple occurences of the -g option");
      } else {
        gotGOption = 1;
      }

    /* Scan the -raw option */
    } else if (!strcmp (*argv, "-raw")) {
      commandOptionRaw = 1;
      if (gotRawOption) {
        badOption ("Multiple occurences of the -raw option");
      } else {
        gotRawOption = 1;
      }

    /* Scan the -wait option */
    } else if (!strcmp (*argv, "-wait")) {
      commandOptionWait = 1;
      if (gotWaitOption) {
        badOption ("Multiple occurences of the -wait option");
      } else {
        gotWaitOption = 1;
      }

    /* Scan the -r option */
    } else if (!strcmp (*argv, "-r")) {
      if (argc <= 1) {
        badOption ("Expecting integer after -r option");
      } else {
        argCount++;
        randSeedFromOption = atoi (*(argv+1));  /* Extra chars after int ignored */
        if (randSeedFromOption <= 0) {
          badOption ("Invalid integer after -r option");
        }
        if (commandOptionRand) {
          badOption ("Multiple occurences of the -r option");
        } else {
          commandOptionRand = 1;
        }
      }

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

    /* Scan the -i option, which should be followed by a file name */
    } else if (!strcmp (*argv, "-i")) {
      if (argc <= 1) {
        badOption ("Expecting filename after -i option");
      } else {
        argCount++;
        if (termInputFileName == NULL) {
          termInputFileName = *(argv+1);
        } else {
          badOption ("Multiple occurences of the -i option");
        }
      }

    /* Scan the -o option, which should be followed by a file name */
    } else if (!strcmp (*argv, "-o")) {
      if (argc <= 1) {
        badOption ("Expecting filename after -o option");
      } else {
        argCount++;
        if (termOutputFileName == NULL) {
          termOutputFileName = *(argv+1);
        } else {
          badOption ("Multiple occurences of the -o option");
        }
      }

    /* Deal with any invalid options that begin with "-", such as "-qwerty" */
    } else if ((*argv)[0] == '-') {
      fprintf (
        stderr,
        "BLITZ Emulator Error: Invalid command line option (%s); Use -h for help display\n",
        *argv);
      exit (1);
    /* This token will be interpreted as the a.out file name. */
    } else {
      if (executableFileName == NULL) {
        executableFileName = *argv;
      } else {
        badOption ("Invalid command line - multiple input files");
      }

    }
  }

  /* Check that -raw and -i don't both occur. */
  if (commandOptionRaw && (termInputFileName != NULL)) {
    badOption ("Options -raw and -i are incompatible");
  }

  /* Figure out the name of the a.out file. */
  if (executableFileName == NULL) {
    executableFileName = "a.out";
  }

  /* Open the a.out file. */
  executableFile = fopen (executableFileName, "r");
  if (executableFile == NULL) {
    fprintf (stderr,
             "BLITZ Emulator Error: Input file \"%s\" could not be opened\n",
             executableFileName);
    exit (1);
  }

  /* Figure out the name of the DISK file. */
  if (diskFileName == NULL) {
    diskFileName = "DISK";
  }

  /* Open the TERM INPUT file. */
  if (termInputFileName == NULL) {
    termInputFile = stdin;
  } else {
    if (termOutputFileName != NULL) {
      if (strcmp(termInputFileName, termOutputFileName) == 0) {
        fprintf (stderr,
                 "BLITZ Emulator Error: Terminal input and output files must be different\n");
        exit (1);
      }
    }
    termInputFile = fopen (termInputFileName, "r");
    if (termInputFile == NULL) {
      fprintf (stderr,
               "BLITZ Emulator Error: Input file \"%s\" could not be opened for reading\n",
               termInputFileName);
      exit (1);
    }
  }

  /* Open the TERM OUTPUT file. */
  if (termOutputFileName == NULL) {
    termOutputFile = stdout;
  } else {
    termOutputFile = fopen (termOutputFileName, "w");
    if (termOutputFile == NULL) {
      fprintf (stderr,
               "BLITZ Emulator Error: Output file \"%s\" could not be opened for writing\n",
               termOutputFileName);
      exit (1);
    }
  }
}



/* badOption (msg)
**
** This routine prints a message on stderr and then aborts this program.
*/
void badOption (char * msg) {
  fprintf (stderr, "BLITZ Emulator Error: %s;  Use -h for help display\n", msg);
  exit (1);
}



/* fatalError (msg)
**
** This routine prints the given error message on stderr and calls errorExit().
*/
void fatalError (char * msg) {
  turnOffTerminal ();
  fprintf (stderr, "\nBLITZ Emulator Error: %s\n", msg);
  /* raise (SIGSEGV);  Produce a core dump. */
  errorExit ();
}



/* errorExit ()
**
** This routine performs any final cleanup and calls exit(1).
*/
void errorExit () {
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
"=====  The BLITZ Machine Emulator  =====\n"
"=====                              =====\n"
"========================================\n"
"\n"
"Copyright 2001-2007, Harry H. Porter III\n"
"========================================\n"
"  Original Author:\n"
"    02/05/01 - Harry H. Porter III\n"
/*
   "  Modifcations by:\n"
   "    09/28/04 - Harry - Fix bug with randomSeed, disk errno\n"
   "    11/26/04 - Harry - Fix bugs with serial input\n"
   "    03/15/06 - Harry - Renamed SPANK to BLITZ\n"
   "    04/27/07 - Harry - Support for little endian added\n"
*/
"\n"
"Command Line Options\n"
"====================\n"
"  These command line options may be given in any order.\n"
"    filename\n"
"       The input executable file.  If missing, \"a.out\" will be used.\n"
"    -h\n"
"       Print this help info.  Ignore other options and exit.\n"
"    -d filename\n"
"       Disk file name.  If missing, \"DISK\" will be used.\n"
"    -g\n"
"       Automatically begin emulation of the a.out program, bypassing\n"
"       the command line interface.\n"
"    -i filename\n"
"       Terminal input file name.  If missing, \"stdin\" will be used.\n"
"    -o filename\n"
"       Terminal output file name.  If missing, \"stdout\" will be used.\n"
"    -r integer\n"
"       Set the random seed to the given integer, which must be > 0.\n"
"    -raw\n"
"       User input for BLITZ terminal I/O will be in \"raw\" mode; the\n"
"       default is \"cooked\", in which case the running BLITZ code\n"
"       is relieved from echoing keystrokes, processing backspaces, etc.\n"
"    -wait\n"
"       This option applies only when input is coming from an interactive\n"
"       terminal and a 'wait' instruction is executed with no other pending\n"
"       interrupts.  Without this option, execution will halt; with it the\n"
"       emulator will wait for input.\n"
);

}



/* getToken ()
**
** This routine reads a single token from the input and returns it.
** A token is any sequence of characters not containing whitespace.
** Leading and trailing white space is removed.
*/
char * getToken () {
  int i;
  fflush (stdout);
  fgets (inputBuffer, sizeof(inputBuffer), stdin);

/***
  printf ("inputBuffer = \n");
  for (i=0; inputBuffer[i] != '\0'; i++) {
    printf ("%02X ", (int) inputBuffer[i]);
  }
  printf ("\n");
***/

  if (feof (stdin)) {
    printf ("\nEOF ignored: Type 'q' to exit.\n");
    // fseek (stdin, 0l, SEEK_SET);
    clearerr (stdin);
    inputBuffer [0] = '\0';
    return inputBuffer;
    /*  exit (0);  */
  }
  /* Overwrite the \n with \0 to remove it. */
  inputBuffer [strlen (inputBuffer)-1] = '\0';
  return trimToken (inputBuffer);
}



/* trimToken (str)
**
** This routine is passed a pointer to a string.  It trims leading and
** trailing whitespace off of the string and returns a pointer to the
** new string.  The string is updated in place: a \0 character is written
** into the string to trim trailing white space.  The string is assumed to
** contain a single token, and an error is printed if there is more than one
** token.
*/
char * trimToken (char * str) {
  char * p, * end;
  char * start = str;
  /* Set start to point to the first non-whitespace character. */
  while (1) {
    if ((*start != ' ') && (*start != '\t')) {
      break;
    }
    start++;
  }
  /* Set p to point to the next whitespace or \0 character. */
  p = start;
  while (1) {
    if ((*p == ' ') || (*p == '\t') || (*p == '\0')) {
      break;
    }
    p++;
  }
  end = p;
  /* Make sure that there is nothing but whitespace until the \0 character. */
  while (*p != '\0') {
    if ((*p != ' ') && (*p != '\t')) {
      printf ("WARNING: Input characters (\"%s\") after the first word were ignored.\n", p);
      break;
    }
    p++;
  }
  /* Store \0 character after the last character of the token. */
  *end = '\0';
  return start;
}



/* toLower (str)
**
** This routine is passed a string.  It runs through it, converting any
** uppercase letters to lowercase.  It changes the string in place.  It
** returns a pointer to the string.
*/
char * toLower (char * str) {
  char * p = str;
  while (*p) {
    if (('A' <= *p) && (*p <= 'Z')) {
      *p = *p + ('a' - 'A');
    }
    p++;
  }
  return str;
}



/* readInteger (file)
**
** Read an integer (4-bytes, binary) from the given file and return it.
*/
int readInteger (FILE * file) {
  int i, numItemsRead;
  errno = 0;
  numItemsRead = fread (&i, 4, 1, file);
  if (numItemsRead != 1) {
    if (errno) perror ("Error when reading from binary file");
    fatalError ("Problem reading from file");
  }
  return SWAP_BYTES (i);
}



/* readByte (file)
**
** Read 1 byte from the given file and return it as an integer.
*/
int readByte (FILE * file) {
  int i, numBytesRead;
  char c;
  errno = 0;
  numBytesRead = fread (&c, 1, 1, file);
  if (numBytesRead != 1) {
    if (errno) perror ("Error when reading from binary file");
    fatalError ("Problem reading from file");
  }
  i = c;
  return i;
}



/* roundUpToMultipleOf (i, p)
**
** This routine rounds i up to a multiple of p and returns the result.
*/
int roundUpToMultipleOf (int i, int p) {
  fatalError ("This routine is not ever called");
  if ((i < 0) || (p <= 0)) {
    fatalError ("PROGRAM LOGIC ERROR: Bad arg in roundUpToMultipleOf()");
  }
  if (i % p > 0) {
    return (i / p + 1) * p;
  }
  return i;
}



/* printMemory (ptr, n, addr)
**
** This routine dumps n bytes of memory (located at "ptr") in hex.  It
** Labels the bytes starting with "addr".
*/
void printMemory (char * ptr, int n, int addr) {
   pmCount = n;
   pmPtr = ptr;

   /* Each execution of this loop prints a single output line. */
   get16Bytes ();
   while (pmSize > 0) {
     /* The following test doesn't work well, since Unix uses huge output
        buffers and these tend to get filled quickly. */
     if (controlCPressed) {
       controlCPressed = 0;
       return;
     }
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



/* puthalf (i)
**
** This routine is passed an integer, which it prints as 4 hex digits.
*/
void puthalf (int i) {
  fatalError ("Routine puthalf is never used");
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



/* commandHelp ()
**
** This routine prints the help command material.
*/
void commandHelp () {
  printf (
"===========================================================================\n"
"This program accepts commands typed into the terminal.  Each command\n"
"should be typed without any arguments; the commands will prompt for\n"
"arguments when needed.  Case is not significant.  Some abbreviations\n"
"are allowed, as shown.  Typing control-C will halt execution.\n"
"\n"
"The available commands are:\n"
"\n"
"  quit    - Terminate this program\n"
"  q         \n"
"  help    - Produce this display\n"
"  h         \n"
"  info    - Display the current state of the machine\n"
"  i         \n"
"  dumpMem - Display the contents of memory\n"
"  dm        \n"
"  setmem  - Used to alter memory contents\n"
"  fmem    - Display floating point values from memory\n"
"  go      - Begin or resume BLITZ instruction execution\n"
"  g         \n"
"  step    - Single step; execute one machine-level instruction\n"
"  s         \n"
"  t       - Single step; execute one KPL statement\n"
"  u       - Execute continuously until next KPL call, send, or return statement\n"
"  stepn   - Execute N machine-level instructions\n"
"  r       - Display all the integer registers\n"
"  r1      - Change the value of register r1\n"
"   ...       \n"
"  r15     - Change the value of register r15\n"
"  float   - Display all the floating-point registers\n"
"  f         \n"
"  f0      - Change the value of floating-point register f0\n"
"   ...       \n"
"  f15     - Change the value of floating-point register f15\n"
"  dis     - Disassemble several instructions\n"
"  d       - Disassemble several instructions from the current location\n"
"  hex     - Convert a user-entered hex number into decimal and ascii\n"
"  dec     - Convert a user-entered decimal number into hex and ascii\n"
"  ascii   - Convert a user-entered ascii char into hex and decimal\n"
"  setI    - Set the I bit in the Status Register\n"
"  setS    - Set the S bit in the Status Register\n"
"  setP    - Set the P bit in the Status Register\n"
"  setZ    - Set the Z bit in the Status Register\n"
"  setV    - Set the V bit in the Status Register\n"
"  setN    - Set the N bit in the Status Register\n"
"  clearI  - Clear the I bit in the Status Register\n"
"  clearS  - Clear the S bit in the Status Register\n"
"  clearP  - Clear the P bit in the Status Register\n"
"  clearZ  - Clear the Z bit in the Status Register\n"
"  clearV  - Clear the V bit in the Status Register\n"
"  clearN  - Clear the N bit in the Status Register\n"
"  setPC   - Set the Program Counter (PC)\n"
"  setPTBR - Set the Page Table Base Register (PTBR)\n"
"  setPTLR - Set the Page Table Length Register (PTLR)\n"
"  pt      - Display the Page Table\n"
"  trans   - Perform page table translation on a single address\n"
"  cancel  - Cancel all pending interrupts\n"
"  labels  - Display the label table\n"
"  find    - Find a label by name\n"
"  find2   - Find a label by value\n"
"  add     - Add a new label, inserting it into the indexes\n"
"  reset   - Reset the machine state and re-read the a.out file\n"
"  io      - Display the state of the I/O devices\n"
"  read    - Read a word from memory-mapped I/O region\n"
"  write   - Write a word to memory-mapped I/O region\n"
"  raw     - Switch serial input to raw mode\n"
"  cooked  - Switch serial input to cooked mode\n"
"  input   - Enter input characters for future serial I/O input\n"
"  format  - Create and format a BLITZ disk file\n"
"  sim     - Display the current simulation constants\n"
"  stack   - Display the KPL calling stack\n"
"  st        \n"
"  frame   - Display the current activation frame\n"
"  fr        \n"
"  up      - Move up in the activation frame stack\n"
"  down    - Move down in the activation frame stack\n"
"\n"
"===========================================================================\n");

}



/* commandAllRegs ()
**
** This routine prints the registers (either User or Sytstem, depending
** on the current mode.
*/
void commandAllRegs () {
  int i;
  if (!statusS) {
    printf ("=====  USER REGISTERS  =====\n");
    for (i=0; i<=9; i++) {
      printf ("  r%d  = ", i);
      printNumberNL (userRegisters [i]);
    }
    for (i=10; i<=15; i++) {
      printf ("  r%d = ", i);
      printNumberNL (userRegisters [i]);
    }
  } else {
    printf ("=====  SYSTEM REGISTERS  =====\n");
    for (i=0; i<=9; i++) {
      printf ("  r%d  = ", i);
      printNumberNL (systemRegisters [i]);
    }
    for (i=10; i<=15; i++) {
      printf ("  r%d = ", i);
      printNumberNL (systemRegisters [i]);
    }
  }
  printf ("==============================\n");
}



/* commandAllFloatRegs ()
**
** This routine prints the floating-point registers.
*/
void commandAllFloatRegs () {
  int i;
  double d;
  printf ("=====  FLOATING-POINT REGISTERS  =====\n");
  for (i=0; i<=9; i++) {
    d = floatRegisters [i];
    printf ("  f%d  = ", i);
    printDouble (d);
  }
  for (i=10; i<=15; i++) {
    d = floatRegisters [i];
    printf ("  f%d = ", i);
    printDouble (d);
  }
  printf ("======================================\n");
}



/* printDouble (d)
**
** This rouinte prints out a floating-point number.  It checks for
** NAN, POSITIVE-INFINITY, and NEGATIVE-INFINITY and prints them
** appropriately.  Numbers are printed in the form:
**
**       4484ea0c eff1a9b4   ( value = 1.23456e+22 )
*/
void printDouble (double d) {
  double value = d;
  int * p;
  p = (int *) (& value);
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
  printf ("0x");
  putlong (*(p+1));
  printf (" ");
  putlong (*p);
#else
  printf ("0x");
  putlong (*p);
  printf (" ");
  putlong (*(p+1));
#endif
  printf ("   ( value = ");
  printDoubleVal (value);
  printf (" )\n");
}



/* printDoubleVal (d)
**
** This routine is passed a floating-point number.  It checks for
** NAN, POSITIVE-INFINITY, and NEGATIVE-INFINITY and prints them
** appropriately.  Numbers are printed in the form:
**
**       1.23456e+22
**       Not-a-number
**       Positive-Infinity
**       Negative-Infinity
**       0
**       Negative-Zero
*/
void printDoubleVal (double d) {
  if (isnan(d)) {
    printf ("Not-a-Number");
  } else if (d == POSITIVE_INFINITY) {
    printf ("Positive-Infinity");
  } else if (d == NEGATIVE_INFINITY) {
    printf ("Negative-Infinity");
  } else if (isNegZero (d)) {
    printf ("Negative-Zero");
  } else {
    printf ("%.15g", d);
  }
}




/* commandInfo ()
**
** This routine prints the current state of the machine.
*/
void commandInfo () {
  int i;
  printf ("============================\n");
  printInfo ();
  printf ("=====  USER REGISTERS  =====\n");
  for (i=0; i<=9; i++) {
    printf ("  r%d  = ", i);
    printNumberNL (userRegisters [i]);
  }
  for (i=10; i<=15; i++) {
    printf ("  r%d = ", i);
    printNumberNL (userRegisters [i]);
  }
  printf ("=====  SYSTEM REGISTERS  =====\n");
  for (i=0; i<=9; i++) {
    printf ("  r%d  = ", i);
    printNumberNL (systemRegisters [i]);
  }
  for (i=10; i<=15; i++) {
    printf ("  r%d = ", i);
    printNumberNL (systemRegisters [i]);
  }
  commandAllFloatRegs ();
  printf ("  PC   = ");
  printNumberNL (pc);
  printf ("  PTBR = ");
  printNumberNL (ptbr);
  printf ("  PTLR = ");
  printNumberNL (ptlr);
  printf ("                        ---- ----  ---- ----  ---- ----  --IS PZVN\n");
  printf ("  SR   = 0x%08X  =  0000 0000  0000 0000  0000 0000  00",
          buildStatusWord ());
  if (statusI) printf ("1");    else printf ("0");
  if (statusS) printf ("1 ");   else printf ("0 ");
  if (statusP) printf ("1");    else printf ("0");
  if (statusZ) printf ("1");    else printf ("0");
  if (statusV) printf ("1");    else printf ("0");
  if (statusN) printf ("1\n");  else printf ("0\n");
  if (statusI)
      printf ("           I = 1   Interrupts Enabled\n");
    else
      printf ("           I = 0   Interrupts Disabled\n");
  if (statusS)
      printf ("           S = 1   System Mode\n");
    else
      printf ("           S = 0   User Mode\n");
  if (statusP)
      printf ("           P = 1   Paging Enabled\n");
    else
      printf ("           P = 0   Paging Disabled\n");
  if (statusZ)
      printf ("           Z = 1   Zero\n");
    else
      printf ("           Z = 0   Not Zero\n");
  if (statusV)
      printf ("           V = 1   Overflow Occurred\n");
    else
      printf ("           V = 0   No Overflow\n");
  if (statusN)
      printf ("           N = 1   Negative\n");
    else
      printf ("           N = 0   Not Negative\n");
  printf ("==============================\n");
  printPendingInterrupts ();
  printf ("  System Trap Number                = 0x%08X\n", systemTrapNumber);
  printf ("  Page Invalid Offending Address    = 0x%08X\n", pageInvalidOffendingAddress);
  printf ("  Page Readonly Offending Address   = 0x%08X\n", pageReadonlyOffendingAddress);
  printf ("  Time of next timer event          = %d\n", timeOfNextTimerEvent);
  printf ("  Time of next disk event           = %d\n", timeOfNextDiskEvent);
  printf ("  Time of next serial in event      = %d\n", timeOfNextSerialInEvent);
  printf ("  Time of next serial out event     = %d\n", timeOfNextSerialOutEvent);
  printf ("    Current Time                    = %d\n", currentTime);
  printf ("    Time of next event              = %d\n", timeOfNextEvent);
  printf ("    Time Spent Sleeping             = %d\n", timeSpentAsleep);
  printf ("      Instructions Executed         = %d\n", currentTime-timeSpentAsleep);
  printf ("  Number of Disk Reads              = %d\n", numberOfDiskReads);
  printf ("  Number of Disk Writes             = %d\n", numberOfDiskWrites);
  printf ("==============================\n");
  printAboutToExecute ();
  printKPLStmtCode ();
}



/* printPendingInterrupts ()
**
** This routine prints the current state of "interruptsSignaled".
*/
void printPendingInterrupts () {
  printf ("  Pending Interrupts                = 0x%08X\n", interruptsSignaled);
  if (interruptsSignaled & POWER_ON_RESET)
    printf ("    POWER_ON_RESET\n");
  if (interruptsSignaled & TIMER_INTERRUPT)
    printf ("    TIMER_INTERRUPT\n");
  if (interruptsSignaled & DISK_INTERRUPT)
    printf ("    DISK_INTERRUPT\n");
  if (interruptsSignaled & SERIAL_INTERRUPT)
    printf ("    SERIAL_INTERRUPT\n");
  if (interruptsSignaled & HARDWARE_FAULT)
    printf ("    HARDWARE_FAULT\n");
  if (interruptsSignaled & ILLEGAL_INSTRUCTION)
    printf ("    ILLEGAL_INSTRUCTION\n");
  if (interruptsSignaled & ARITHMETIC_EXCEPTION)
    printf ("    ARITHMETIC_EXCEPTION\n");
  if (interruptsSignaled & ADDRESS_EXCEPTION)
    printf ("    ADDRESS_EXCEPTION\n");
  if (interruptsSignaled & PAGE_INVALID_EXCEPTION)
    printf ("    PAGE_INVALID_EXCEPTION\n");
  if (interruptsSignaled & PAGE_READONLY_EXCEPTION)
    printf ("    PAGE_READONLY_EXCEPTION\n");
  if (interruptsSignaled & PRIVILEGED_INSTRUCTION)
    printf ("    PRIVILEGED_INSTRUCTION\n");
  if (interruptsSignaled & ALIGNMENT_EXCEPTION)
    printf ("    ALIGNMENT_EXCEPTION\n");
  if (interruptsSignaled & EXCEPTION_DURING_INTERRUPT)
    printf ("    EXCEPTION_DURING_INTERRUPT\n");
  if (interruptsSignaled & SYSCALL_TRAP)
    printf ("    SYSCALL_TRAP\n");
}



/* commandIO ()
**
** This routine prints the current state of the I/O devices.
*/
void commandIO () {
  int i;
  printf ("==========  Serial I/O  ==========\n");
  if (termOutputReady) {
    printf ("  Output Status:       Ready\n");
  } else {
    printf ("  Output Status:       Not Ready\n");
  }
  if (termInCharAvail) {
    printf ("  Input Status:        Character Available\n");
  } else {
    printf ("  Input Status:        Character Not Available\n");
  }
  printf ("  Current Input Char:  \'");
  fancyPrintChar (termInChar);
  if (termInCharWasUsed) {
    printf ("\'    (already fetched by CPU)\n");
  } else {
    printf ("\'    (not yet fetched by CPU)\n");
  }
  printf ("    The following characters are currently in the type-ahead buffer:\n      ");
  printTypeAheadBuffer ();
  if (termInputFile == stdin) {
    printf ("  Input coming from: stdin\n");
    if (terminalWantRawProcessing) {
      printf ("    Input Mode: Raw\n");
    } else {
      printf ("    Input Mode: Cooked\n");
    }
  } else {
    printf ("  Input to the serial device is coming from file \"%s\".\n",
            termInputFileName);
  }

  printf ("==========  Disk I/O  ==========\n");
  printf ("  The file used for the disk: \"%s\"\n", diskFileName);
  if (diskFile == NULL) {
    printf ("    DISK File is currently closed.\n");
  } else {
    printf ("    DISK File is currently opened.\n");
  }
  printf ("  Disk size:\n");
  printf ("    Total Tracks = %d\n", diskTrackCount);
  printf ("    Total Sectors = %d\n", diskSectorCount);
  printf ("    Sectors per track = %d\n", SECTORS_PER_TRACK);
  printf ("  Current Status:\n");
  printf ("    Positioned at Sector = %d\n", currentDiskSector);
  if (currentDiskStatus == DISK_BUSY) {
    printf ("    Current Disk Status  = DISK_BUSY\n");
  } else if (currentDiskStatus == OPERATION_COMPLETED_OK) {
    printf ("    Current Disk Status  = OPERATION_COMPLETED_OK\n");
  } else if (currentDiskStatus == OPERATION_COMPLETED_WITH_ERROR_1) {
    printf ("    Current Disk Status  = OPERATION_COMPLETED_WITH_ERROR_1\n");
  } else if (currentDiskStatus == OPERATION_COMPLETED_WITH_ERROR_2) {
    printf ("    Current Disk Status  = OPERATION_COMPLETED_WITH_ERROR_2\n");
  } else if (currentDiskStatus == OPERATION_COMPLETED_WITH_ERROR_3) {
    printf ("    Current Disk Status  = OPERATION_COMPLETED_WITH_ERROR_3\n");
  } else if (currentDiskStatus == OPERATION_COMPLETED_WITH_ERROR_4) {
    printf ("    Current Disk Status  = OPERATION_COMPLETED_WITH_ERROR_4\n");
  } else if (currentDiskStatus == OPERATION_COMPLETED_WITH_ERROR_5) {
    printf ("    Current Disk Status  = OPERATION_COMPLETED_WITH_ERROR_5\n");
  } else {
    printf ("    Current Disk Status  = ***************  ERROR  ***************\n");
  }
  if (futureDiskStatus == DISK_BUSY) {
    printf ("    Future Disk Status   = DISK_BUSY\n");
  } else if (futureDiskStatus == OPERATION_COMPLETED_OK) {
    printf ("    Future Disk Status   = OPERATION_COMPLETED_OK\n");
  } else if (futureDiskStatus == OPERATION_COMPLETED_WITH_ERROR_1) {
    printf ("    Future Disk Status   = OPERATION_COMPLETED_WITH_ERROR_1\n");
  } else if (futureDiskStatus == OPERATION_COMPLETED_WITH_ERROR_2) {
    printf ("    Future Disk Status   = OPERATION_COMPLETED_WITH_ERROR_2\n");
  } else if (futureDiskStatus == OPERATION_COMPLETED_WITH_ERROR_3) {
    printf ("    Future Disk Status   = OPERATION_COMPLETED_WITH_ERROR_3\n");
  } else if (futureDiskStatus == OPERATION_COMPLETED_WITH_ERROR_4) {
    printf ("    Future Disk Status   = OPERATION_COMPLETED_WITH_ERROR_4\n");
  } else if (futureDiskStatus == OPERATION_COMPLETED_WITH_ERROR_5) {
    printf ("    Future Disk Status   = OPERATION_COMPLETED_WITH_ERROR_5\n");
  } else {
    printf ("    Future Disk Status   = ***************  ERROR  ***************\n");
  }
  printf ("  Area of memory being read from / written to:\n");
  printf ("    diskBufferLow  = 0x%08X\n", diskBufferLow);
  printf ("    diskBufferHigh = 0x%08X\n", diskBufferHigh);
  printf ("  Memory-Mapped Register Contents:\n");
  printf ("    DISK_MEMORY_ADDRESS_REGISTER = 0x%08X\n", diskMemoryAddressRegister);
  printf ("    DISK_SECTOR_NUMBER_REGISTER  = 0x%08X\n", diskSectorNumberRegister);
  printf ("    DISK_SECTOR_COUNT_REGISTER   = 0x%08X\n", diskSectorCountRegister);
  printf ("  Number of Disk Reads  = %d\n", numberOfDiskReads);
  printf ("  Number of Disk Writes = %d\n", numberOfDiskWrites);

  printf ("==============================\n");
  printf ("  CPU status:\n");
  if (statusI)
      printf ("    Interrupts:   Enabled\n");
    else
      printf ("    Interrupts:   Disabled\n");
  if (statusS)
      printf ("    Mode:         System\n");
    else
      printf ("    Mode:         User\n");
  printf ("    Pending Interrupts:\n");
  if (interruptsSignaled & POWER_ON_RESET)
    printf ("      POWER_ON_RESET\n");
  if (interruptsSignaled & TIMER_INTERRUPT)
    printf ("      TIMER_INTERRUPT\n");
  if (interruptsSignaled & DISK_INTERRUPT)
    printf ("      DISK_INTERRUPT\n");
  if (interruptsSignaled & SERIAL_INTERRUPT)
    printf ("      SERIAL_INTERRUPT\n");
  if (interruptsSignaled & HARDWARE_FAULT)
    printf ("      HARDWARE_FAULT\n");
  if (interruptsSignaled & ILLEGAL_INSTRUCTION)
    printf ("      ILLEGAL_INSTRUCTION\n");
  if (interruptsSignaled & ARITHMETIC_EXCEPTION)
    printf ("      ARITHMETIC_EXCEPTION\n");
  if (interruptsSignaled & ADDRESS_EXCEPTION)
    printf ("      ADDRESS_EXCEPTION\n");
  if (interruptsSignaled & PAGE_INVALID_EXCEPTION)
    printf ("      PAGE_INVALID_EXCEPTION\n");
  if (interruptsSignaled & PAGE_READONLY_EXCEPTION)
    printf ("      PAGE_READONLY_EXCEPTION\n");
  if (interruptsSignaled & PRIVILEGED_INSTRUCTION)
    printf ("      PRIVILEGED_INSTRUCTION\n");
  if (interruptsSignaled & ALIGNMENT_EXCEPTION)
    printf ("      ALIGNMENT_EXCEPTION\n");
  if (interruptsSignaled & EXCEPTION_DURING_INTERRUPT)
    printf ("      EXCEPTION_DURING_INTERRUPT\n");
  if (interruptsSignaled & SYSCALL_TRAP)
    printf ("      SYSCALL_TRAP\n");
  printf ("  Time of next timer event........ %d\n", timeOfNextTimerEvent);
  printf ("  Time of next disk event......... %d\n", timeOfNextDiskEvent);
  printf ("  Time of next serial in event.... %d\n", timeOfNextSerialInEvent);
  printf ("  Time of next serial out event... %d\n", timeOfNextSerialOutEvent);
  printf ("    Current Time.................. %d\n", currentTime);
  printf ("    Time of next event............ %d\n", timeOfNextEvent);
  printf ("==============================\n");
}



/* commandSim ()
**
** This routine prints the current simulation constants.
*/
void commandSim () {
  int ans;
  FILE * file;

  printf ("=========================  Simulation Constants  ==============================\n");

  printf ("  KEYBOARD_WAIT_TIME           %11d\n", KEYBOARD_WAIT_TIME);
  printf ("  KEYBOARD_WAIT_TIME_VARIATION %11d\n", KEYBOARD_WAIT_TIME_VARIATION);

  printf ("  TERM_OUT_DELAY               %11d\n", TERM_OUT_DELAY);
  printf ("  TERM_OUT_DELAY_VARIATION     %11d\n", TERM_OUT_DELAY_VARIATION);

  printf ("  TIME_SLICE                   %11d (0=no timer interrutps)\n", TIME_SLICE);
  printf ("  TIME_SLICE_VARIATION         %11d\n", TIME_SLICE_VARIATION);

  printf ("  DISK_SEEK_TIME               %11d\n", DISK_SEEK_TIME);
  printf ("  DISK_SETTLE_TIME             %11d\n", DISK_SETTLE_TIME);
  printf ("  DISK_ROTATIONAL_DELAY        %11d\n", DISK_ROTATIONAL_DELAY);    /* Minimum is 1 */
  printf ("  DISK_ACCESS_VARIATION        %11d\n", DISK_ACCESS_VARIATION);

  printf ("  DISK_READ_ERROR_PROBABILITY  %11d (0=never, 1=always, n=\"about 1/n\")\n",
                                                    DISK_READ_ERROR_PROBABILITY);
  printf ("  DISK_WRITE_ERROR_PROBABILITY %11d (0=never, 1=always, n=\"about 1/n\")\n",
                                                    DISK_WRITE_ERROR_PROBABILITY);

  printf ("  INIT_RANDOM_SEED             %11d (between 1 and 2147483646)\n", INIT_RANDOM_SEED);
  if (commandOptionRand) {
    printf ("    (overridden with -r option %11d)\n", randSeedFromOption);
  }

  printf ("  MEMORY_SIZE                   0x%08X (decimal: %d)\n", MEMORY_SIZE, MEMORY_SIZE);

  printf ("  MEMORY_MAPPED_AREA_LOW        0x%08X\n", MEMORY_MAPPED_AREA_LOW);
  printf ("  MEMORY_MAPPED_AREA_HIGH       0x%08X\n", MEMORY_MAPPED_AREA_HIGH);

  printf ("  SERIAL_STATUS_WORD_ADDRESS    0x%08X\n", SERIAL_STATUS_WORD_ADDRESS);
  printf ("  SERIAL_DATA_WORD_ADDRESS      0x%08X\n", SERIAL_DATA_WORD_ADDRESS);

  printf ("  DISK_STATUS_WORD_ADDRESS      0x%08X\n", DISK_STATUS_WORD_ADDRESS);
  printf ("  DISK_COMMAND_WORD_ADDRESS     0x%08X\n", DISK_COMMAND_WORD_ADDRESS);
  printf ("  DISK_MEMORY_ADDRESS_REGISTER  0x%08X\n", DISK_MEMORY_ADDRESS_REGISTER);
  printf ("  DISK_SECTOR_NUMBER_REGISTER   0x%08X\n", DISK_SECTOR_NUMBER_REGISTER);
  printf ("  DISK_SECTOR_COUNT_REGISTER    0x%08X\n", DISK_SECTOR_COUNT_REGISTER);

  printf ("===============================================================================\n");

  printf ("\n"
          "The simulation constants will be read in from the file \".blitzrc\" if it exists\n"
          "when the emulator starts up.  If the file does not exist at startup, defaults\n"
          "will be used.  You may edit the \".blitzrc\" file to change the values and then\n"
          "restart the emulator.\n"
          "\n"
          "Would you like me to write these values out to the file \".blitzrc\" now? ");

  ans = readYesNo ();
  if (ans) {

    // Open the file, creating it if it does not exist, overwriting it if it does...
    file = fopen (".blitzrc", "w");
    if (file == NULL) {
      printf ("The \".blitzrc\" file could not be opened for writing!\n");
      perror ("Host error");
      return;
    }

    // Write out the contents of the file...
    fprintf (file,
             "! BLITZ Simulation Constants\n"
             "!\n"
             "! This file is read by the BLITZ emulator when it starts up and after a\n"
             "! \"reset\" command.  This file is used to initialize various values that\n"
             "! will be used by the emulator.\n"
             "!\n"
             "! This file was produced by the emulator (with the \"sim\" command).  It may\n"
             "! be edited to change any or all values.\n"
             "!\n"
             "! Each line has variable name followed by an integer value.  A value may\n"
             "! be specified in either decimal (e.g., \"1234\") or hex (e.g., \"0x1234abcd\").\n"
             "! Values may be left out if desired, in which case a default will be used.\n"
             "! In the case of the random seed, any value specified here will override a\n"
             "! value given with a command line option (-r).\n"
             "!\n"
             "!\n"
      );

    fprintf (file, "KEYBOARD_WAIT_TIME           %11d\n", KEYBOARD_WAIT_TIME);
    fprintf (file, "KEYBOARD_WAIT_TIME_VARIATION %11d\n", KEYBOARD_WAIT_TIME_VARIATION);

    fprintf (file, "TERM_OUT_DELAY               %11d\n", TERM_OUT_DELAY);
    fprintf (file, "TERM_OUT_DELAY_VARIATION     %11d\n", TERM_OUT_DELAY_VARIATION);

    fprintf (file, "\n");
    fprintf (file, "TIME_SLICE                   %11d\n", TIME_SLICE);
    fprintf (file, "TIME_SLICE_VARIATION         %11d\n", TIME_SLICE_VARIATION);

    fprintf (file, "\n");
    fprintf (file, "DISK_SEEK_TIME               %11d\n", DISK_SEEK_TIME);
    fprintf (file, "DISK_SETTLE_TIME             %11d\n", DISK_SETTLE_TIME);
    fprintf (file, "DISK_ROTATIONAL_DELAY        %11d\n", DISK_ROTATIONAL_DELAY);
    fprintf (file, "DISK_ACCESS_VARIATION        %11d\n", DISK_ACCESS_VARIATION);

    fprintf (file, "DISK_READ_ERROR_PROBABILITY  %11d\n", DISK_READ_ERROR_PROBABILITY);
    fprintf (file, "DISK_WRITE_ERROR_PROBABILITY %11d\n", DISK_WRITE_ERROR_PROBABILITY);

    fprintf (file, "\n");
    fprintf (file, "INIT_RANDOM_SEED             %11d\n", INIT_RANDOM_SEED);

    fprintf (file, "\n");
    fprintf (file, "MEMORY_SIZE                   0x%08X\n", MEMORY_SIZE);

    fprintf (file, "\n");
    fprintf (file, "MEMORY_MAPPED_AREA_LOW        0x%08X\n", MEMORY_MAPPED_AREA_LOW);
    fprintf (file, "MEMORY_MAPPED_AREA_HIGH       0x%08X\n", MEMORY_MAPPED_AREA_HIGH);

    fprintf (file, "\n");
    fprintf (file, "SERIAL_STATUS_WORD_ADDRESS    0x%08X\n", SERIAL_STATUS_WORD_ADDRESS);
    fprintf (file, "SERIAL_DATA_WORD_ADDRESS      0x%08X\n", SERIAL_DATA_WORD_ADDRESS);

    fprintf (file, "\n");
    fprintf (file, "DISK_STATUS_WORD_ADDRESS      0x%08X\n", DISK_STATUS_WORD_ADDRESS);
    fprintf (file, "DISK_COMMAND_WORD_ADDRESS     0x%08X\n", DISK_COMMAND_WORD_ADDRESS);
    fprintf (file, "DISK_MEMORY_ADDRESS_REGISTER  0x%08X\n", DISK_MEMORY_ADDRESS_REGISTER);
    fprintf (file, "DISK_SECTOR_NUMBER_REGISTER   0x%08X\n", DISK_SECTOR_NUMBER_REGISTER);
    fprintf (file, "DISK_SECTOR_COUNT_REGISTER    0x%08X\n", DISK_SECTOR_COUNT_REGISTER);

    fclose (file);
  }

}



/* commandRaw ()
**
** This routine changes the raw/cooked mode.
*/
void commandRaw () {
  if (termInputFileName != NULL) {
    printSerialHelp ();
    printf ("*****  Serial I/O is coming from the file \"%s\"  *****\n",
             termInputFileName);
  } else if (terminalWantRawProcessing) {
    printSerialHelp ();
    printf ("*****  The terminal is already in \"raw\" mode  *****\n");
  } else {
    printf ("Future terminal input will be \"raw\".\n");
    terminalWantRawProcessing = 1;
  }
}



/* commandCooked ()
**
** This routine changes the raw/cooked mode.
*/
void commandCooked () {
  if (termInputFileName != NULL) {
    printSerialHelp ();
    printf ("*****  Serial I/O is coming from the file \"%s\"  *****\n",
             termInputFileName);
  } else if (!terminalWantRawProcessing) {
    printSerialHelp ();
    printf ("*****  The terminal is already in \"cooked\" mode  *****\n");
  } else {
    printf ("Future terminal input will be \"cooked\".\n");
    terminalWantRawProcessing = 0;
  }
}



/* printSerialHelp ()
**
** This routine prints a description of how the serial interface works.
*/
void printSerialHelp () {
    printf (
"=============================================================================\n"
"From time to time a running BLITZ program may read characters from\n"
"  the \"Serial I/O\" device, which is intended to simulate an ASCII\n"
"  terminal.  The character data to be supplied to the running BLITZ\n"
"  program will come from either a file (which is specified using the\n"
"  \"-i filename\" command line option when the emulator is started) or\n"
"  from the interactive user-interface running here.\n"
"With this second option, you may enter characters on \"stdin\" at any\n"
"  time during the emulation of a running BLITZ program.  These\n"
"  characters will be supplied to the running BLITZ program (via the\n"
"  emulated Serial I/O device).  If the emulator seems to hang, it may\n"
"  be because the emulator is waiting for you to type additional\n"
"  characters.  (It may also be because the BLITZ program has gotten\n"
"  into an infinite loop.)\n"
"At any time you may always hit control-C to suspend instruction execution\n"
"  and re-enter the emulator command interface.\n"
"Normally a host OS like UNIX will process user input by echoing characters\n"
"  on the screen, buffering entire lines, and processing special characters\n"
"  like backspaces, etc.  This is called \"cooked\" input.  In \"raw\" mode\n"
"  each character is delivered as-is immediately after the key is pressed,\n"
"  with no buffering and without the normal echoing and processing of\n"
"  special characters.\n"
"The BLITZ emulator runs in either \"raw\" mode or \"cooked\" mode.\n"
"  The mode can be changed with the \"raw\" and \"cooked\" commands.\n"
"  This only affects typed input to be delivered to the running BLITZ\n"
"  program; typed input to the emulator itself is always in cooked mode.\n"
"In cooked mode, the host OS will suspend the emulator until you enter a\n"
"  complete line of data and hit ENTER.  Then, if the BLITZ program is\n"
"  echoing character data properly, you will see all the characters echoed,\n"
"  resulting in a second, identical line.  A good BLITZ program should echo\n"
"  all character data, so this duplication of input is normal in cooked\n"
"  mode.\n"
"In raw mode, the normal echoing of keystrokes by the host OS is turned\n"
"  off.  A good BLITZ program should echo all characters, so you *should*\n"
"  see each keystroke echoed properly.  But of course your BLITZ program\n"
"  may not be working properly.  It may fail to echo characters because\n"
"  it has a bug.  Also, the running BLITZ program may not handle\n"
"  backspaces, newlines, CRs, etc., exactly as you and your terminal\n"
"  expect.  It may be helpful to recall \\n=Control-J, \\r=Control-M, and\n"
"  Backspace=Control-H.  On some terminals, the ENTER key is \\r, while many\n"
"  programs expect to use \\n for END-OF-LINE.\n");
  if (termInputFileName != NULL) {
    printf ("Input for the Serial I/O device will come from file..... \"%s\"\n",
             termInputFileName);
  } else if (terminalWantRawProcessing) {
    printf ("Input for the Serial I/O device will come from..... \"stdin\"\n");
    printf ("The current input mode is.......................... \"raw\"\n");
  } else {
    printf ("Input for the Serial I/O device will come from...... \"stdin\"\n");
    printf ("The current input mode is........................... \"cooked\"\n");
  }
  printf (
"=============================================================================\n");
}



/* commandFormat ()
**
** This routine is used to create a new DISK file or to change the size of an
** existing DISK file.  When a DISK file is enlarged, its data is initialized.
*/
void commandFormat () {
  int oldSectorCount, length, i, newByteLen, sec,
  newNumberOfSectors, newNumberOfTracks;
  long len;
  int magic = 0x424C5A64;   /* ASCII for "BLZd" */
  char sectorData [8192];
  char * charPtr;
#define BEGIN_MESS "<---BEGINNING OF SECTOR---------------"
#define END_MESS "-------------------------END OF SECTOR--->"
  char sectorMess [100];

  printf (
    "================================================================================\n"
    "This command is used to create or modify a file to be used by the BLITZ emulator\n"
    "for the disk.  By default, this file will be called \"DISK\".  The filename may\n"
    "be specified on the emulator command line with the \"-d filename\" option.  This\n"
    "command will create the file if it does not exist.  It will set the file to the\n"
    "desired size and initialize all newly allocated space.\n"
    "\n"
    "The size of the disk file is an integral number of tracks.  Each track will\n"
    "contain %d sectors.  The size of each sector is the same as the page size.\n"
    "Thus, the sector size is %d bytes.  The actual file size will be the number\n"
    "tracks times the number of sectors per track times the sector size, plus an\n"
    "additional 4 bytes, which will contain a \"magic number\".  The magic number\n"
    "is 0x%08X (decimal: %d, ASCII: \"%c%c%c%c\") and is used to identify this\n"
    "file as a BLITZ disk file.\n"
    "\n"
    "Initialization consists of writing the magic number in the first 4 bytes of the\n"
    "file and adjusting the file length.  Any data previously stored in the file will\n"
    "be preserved and any additional sectors created will be initialized.\n"
    "================================================================================\n\n",
    SECTORS_PER_TRACK , PAGE_SIZE, magic, magic,
    (magic>>24) & 0x000000ff,
    (magic>>16) & 0x000000ff,
    (magic>>8) & 0x000000ff,
    (magic>>0) & 0x000000ff
);

  // Display the file name...
  printf ("The name of the disk file is \"%s\".\n", diskFileName);

  // If open, close it...
  if (diskFile != NULL) {
    fclose (diskFile);
  }

  // Try to open the file as an existing file...
  diskFile = fopen (diskFileName, "r");
  if (diskFile == NULL) {
    errno = 0;
    printf ("The file \"%s\" did not previously exist.  (It could not be opened for reading.)\n", diskFileName);
    oldSectorCount = 0;

  // If the file already exists...
  } else {
    printf ("The file \"%s\" existed previously.\n", diskFileName);
    fseek (diskFile, 0l, SEEK_END);
    len = ftell (diskFile);

    // Check for length > MAX...
    if (len > ((long) MAX)) {
      printf ("ERROR: The  maximum integer is %d.\n", MAX);
      printf ("ERROR: The DISK file size exceeds the maximum; Please delete the file and try again.\n");
      initializeDisk ();
      return;
    }

    // Print statistics about the old file...
    length = (int) len;
    printf ("    Old File Length = %d bytes\n", length);
    if (length < (SECTORS_PER_TRACK * PAGE_SIZE + 4)) {
      printf ("      The existing DISK file is too small; it must be large enough for at least 1 track.\n");
      printf ("      The minimum DISK file size is 1 track + 4 bytes (where SectorSize = %d bytes and SectorsPerTrack = %d).\n", PAGE_SIZE, SECTORS_PER_TRACK);
    }
    oldSectorCount = (length - 4) / PAGE_SIZE;
    if (oldSectorCount * PAGE_SIZE + 4 != length) {
      printf ("      The existing DISK file size is not an even number of tracks plus 4 bytes.\n");
      printf ("      (SectorSize = %d bytes, SectorsPerTrack = %d, DISK file size = %d bytes)\n", PAGE_SIZE, SECTORS_PER_TRACK, length);
    }
    printf ("    Old Sector Count = %d\n", oldSectorCount);
    printf ("    Old Track Count = %d\n", oldSectorCount / SECTORS_PER_TRACK);

    // Close the old file...
    fclose (diskFile);
    diskFile = NULL;
  }

  // Ask for the new length...
  printf ("Enter the number of tracks (e.g., 1000; type 0 to abort):\n");
  newNumberOfTracks = readDecimalInt ();

  // If < 0 then abort this command...
  if (newNumberOfTracks <= 0) {
    printf ("Aborting; file not changed!\n");
    initializeDisk ();
    return;

  // Otherwise...
  } else {

    // Compute and print statistics about the new file...
    printf ("Desired number of tracks = %d\n", newNumberOfTracks);
    newNumberOfSectors = newNumberOfTracks * SECTORS_PER_TRACK;
    newByteLen = newNumberOfSectors * PAGE_SIZE;
    printf ("  New number of sectors = %d\n", newNumberOfSectors);
    printf ("  New number of tracks = %d\n", newNumberOfTracks);
    printf ("  New number of data bytes = %d\n", newByteLen);

    // Create the file if it does not exist...
    diskFile = fopen (diskFileName, "a+");
    if (diskFile == NULL) {
      printf ("The file could not be created!\n");
      initializeDisk ();
      return;
    }
    fclose (diskFile);

    // Alter the file's length, if necessary...
    errno = 0;
    i = truncate (diskFileName, (off_t) newByteLen+4);    // off_t is "long long" in sys/types.h
    if (i == 0) {
      printf ("    The magic number will consume 4 additional bytes.\n", newByteLen+4);
      printf ("File length changed to %d bytes.\n", newByteLen+4);
    } else {
      printf ("Problems during call to 'truncate'; Disk I/O has been disabled!\n");
      if (errno) perror ("Error");
      fclose (diskFile);
      diskFile = NULL;
      return;
    }

    // Open the file for updating...
    diskFile = fopen (diskFileName, "r+");
    if (diskFile == NULL) {
      printf ("The file could not be opened!\n");
      initializeDisk ();
      return;
    }

    // Write the magic number...
    errno = 0;
    if (fseek (diskFile, 0l, SEEK_SET)) {
      if (errno) perror ("Error on DISK file");
      fclose (diskFile);
      diskFile = NULL;
      return;
    }
    magic = SWAP_BYTES (magic);      // Write out in Big Endian order
    fwrite (&magic, 4, 1, diskFile);
    if (errno) perror ("Error writing to DISK file");

    // Print a message...
    if (newNumberOfSectors > oldSectorCount) {
      printf ("Initializing sectors %d through %d...\n", oldSectorCount, newNumberOfSectors-1);
    } else if (newNumberOfSectors < oldSectorCount) {
      printf ("The pre-existing data in sectors 0 through %d will be preserved...\n", newNumberOfSectors-1);
    }

    // In a loop, write each new sector data out...
    for (sec = oldSectorCount; sec < newNumberOfSectors; sec++) {

      // printf ("Initializing sector %d...\n", sec);

      // Initialize the default data for the sector...
      sprintf (sectorMess, "disk sector %06d ", sec);
      charPtr = sectorMess;
      for (i=0; i<8192; i++) {
        sectorData [i] = *(charPtr++);
        if (*charPtr == '\0') charPtr = sectorMess;
      }
      charPtr = BEGIN_MESS;
      for (i = 0; i < strlen (BEGIN_MESS); i++) {
        sectorData[i] = *(charPtr++);
      }
      charPtr = END_MESS;
      for (i = 0; i < strlen (END_MESS); i++) {
        sectorData[8192 - strlen(END_MESS) + i] = *(charPtr++);
      }
      sprintf (sectorMess, "disk sector %06d ", 123);

      // Seek to the proper location in the file...
      errno = 0;
      if (fseek (diskFile, ((long) ((sec * PAGE_SIZE) + 4)), SEEK_SET)) {
        printf ("Error from fseek for DISK file; Disk I/O has been disabled!\n");
        if (errno) perror ("Error");
        fclose (diskFile);
        diskFile = NULL;
        return;
      }

      // Write out a sector of data...
      errno = 0;
      fwrite (sectorData, 8192, 1, diskFile);
      if (errno) perror ("Error writing to DISK file");
    }
  }

  // Close the file.
  errno = 0;
  fclose (diskFile);
  if (errno) perror ("Error closing DISK file");
  diskFile = NULL;

  // Call "initializeDisk()" to set up the disk file...
  printf ("Successful completion.\n");
  initializeDisk ();
  return;
}



/* printNumberNL2 (i)
**
** This routine prints a number in the form
**       0x1234abcd   ( decimal: 395441741 )
** followed by a newline.
*/
void printNumberNL2 (int i) {
  char str [100];
  char c;
  TableEntry * tableEntry;
  int index;
  printf ("0x");
  putlong (i);
  sprintf (str, "%d", i);
  printf ("     ( decimal: ");
  printStringInWidth (str, 11);
  printf (" )\n");
}



/* printNumberNL (i)
**
** This routine prints a number in the form
**       0x1234abcd   ( decimal: 395441741     ascii: ".4.."   _my_label )
** followed by a newline.
*/
void printNumberNL (int i) {
  char str [100];
  char c;
  TableEntry * tableEntry;
  int index;
  if (i == 0) {
    printf ("0x00000000     ( decimal: 0 )\n");
    return;
  }
  printf ("0x");
  putlong (i);
  sprintf (str, "%d", i);
  printf ("     ( decimal: ");
  printStringInWidth (str, 11);
  if ((i >= ' ') && (i <= '~')) {
    printf (" ascii: \'");
    putchar (i & 0x000000ff);
    printf ("\'");
  }
/****
  printf (" ascii: \"");
  c = (i >> 24) & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  c = (i >> 16) & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  c = (i >> 8) & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  c = i & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  printf ("\"");
****/
  index = findLabel (i);
  if (index != -1) {
    tableEntry = valueIndex [index];
    printf ("     %s", tableEntry->string);
  }
  printf (" )\n");
}



/* commandDumpMemory ()
**
** This routine asks for a starting address and a length.  It then dumps
** that many bytes of physical memory.
*/
void commandDumpMemory () {
  int addr, len;
  printf ("Enter the starting (physical) memory address in hex: ");
  addr = readHexInt ();
  if (addr < 0 ) {
    printf ("Address must not be negative.\n");
    return;
  }
  if (addr >= MEMORY_SIZE) {
    printf ("This address is beyond the upper limit of physical memory.\n");
    printf ("The address of the upper-most byte is ");
    printNumberNL (MEMORY_SIZE - 1);
    return;
  }
  printf ("Enter the number of bytes in hex (or 0 to abort): ");
  len = readHexInt ();
  if (len <= 0 ) return;
  if (addr+len > MEMORY_SIZE) {
    printf ("This request extends beyond the upper limit of physical memory.\n");
    printf ("The maximum length from this address is ");
    printNumberNL (MEMORY_SIZE - addr);
    return;
  }
  printMemory (& memory [addr], len, addr);
}



/* commandSetMemory ()
**
** This routine asks for a starting address and a value.  It then alters
** a single word of physical memory.
*/
void commandSetMemory () {
  int addr, val;
  printf ("Enter the (physical) memory address in hex of the word to be modified: ");
  addr = readHexInt ();
  if (addr < 0 ) {
    printf ("Address must not be negative.\n");
    return;
  }
  if (!physicalAddressOk (addr)) {
    printf ("This is not a legal physical address.\n");
    return;
  }
  if (!isAligned (addr)) {
    printf ("This is address is not word-aligned.\n");
    return;
  }
  printf ("The old value is:\n");
  printf ("0x%06X: 0x%08X\n", addr, getPhysicalWord (addr));
  printf ("Enter the new value (4 bytes in hex): ");
  val = readHexInt ();
  putPhysicalWord (addr, val);
  printf ("0x%06X: 0x%08X\n", addr, getPhysicalWord (addr));
}



/* commandWriteWord ()
**
** This routine asks for an address and a value.  It then does a clean
** write to a single word of physical memory.  This can be used to
** test the memory-mapped I/O regions.
*/
void commandWriteWord () {
  int addr, val;
  printf ("This command can be used to write to a word of memory that is in the\n");
  printf ("    memory-mapped I/O region, sending data or commands to the I/O device.\n");
  printf ("Enter the (physical) memory address in hex of the word to be written to: ");
  addr = readHexInt ();
  if (addr < 0 ) {
    printf ("Address must not be negative.\n");
    return;
  }
  if (!physicalAddressOk (addr)) {
    printf ("This is not a legal physical address.\n");
    return;
  }
  if (!isAligned (addr)) {
    printf ("This is address is not word-aligned.\n");
    return;
  }
  printf ("Enter the new value (4 bytes in hex): ");
  val = readHexInt ();
  putPhysicalWord (addr, val);
  printf ("Writing word...   address = 0x%06X    value = 0x%08X\n", addr, val);
}



/* commandReadWord ()
**
** This routine asks for an address and a value.  It then does a clean
** read from to a single word of physical memory.  This can be used to
** test the memory-mapped I/O regions.
*/
void commandReadWord () {
  int addr, val;
  printf ("This command can be used to read a word of memory that is in the\n");
  printf ("    memory-mapped I/O region, retrieving I/O device status or data.\n");
  printf ("Enter the (physical) memory address in hex of the word to be read from: ");
  addr = readHexInt ();
  if (addr < 0 ) {
    printf ("Address must not be negative.\n");
    return;
  }
  if (!physicalAddressOk (addr)) {
    printf ("This is not a legal physical address.\n");
    return;
  }
  if (!isAligned (addr)) {
    printf ("This is address is not word-aligned.\n");
    return;
  }
  val = getPhysicalWord (addr);
  printf ("Reading word...   address = 0x%06X    value = 0x%08X\n", addr, val);
}



/* commandInput ()
**
** This command allows the user to type several characters, which will
** be supplied to the serial input of the BLITZ machine as they are needed.
*/
void commandInput () {
  int ch, i;
  char terminalBuffer [1];

  /* Make sure the serial input is not coming from a file. */
  if (termInputFile != stdin) {
    printf ("You cannot type in input since it comes from file \"%s\".\n",
            termInputFileName);
    return;
  }

  printf ("The following characters will be supplied as input to the BLITZ serial\n"
          "input before querying the terminal:\n  ");
  printTypeAheadBuffer ();
  printf ("You may add to this type-ahead buffer now.\n");
  if (terminalWantRawProcessing) {
    printf ("The terminal is now in \"raw\" mode.\nEnter characters followed by control-D...\n");
  } else {
    printf ("The terminal is now in \"cooked\" mode.\nEnter characters followed by control-D...\n");
  }

  /* Read in a bunch of characters. */
  fflush (stdout);
  turnOnTerminal ();
  while (1) {
    /* Get the next character. */
    i = fread (terminalBuffer, 1, 1, termInputFile);
    if (i == 1) {
      ch = terminalBuffer [0];
    } else {
      break;  /* Should not occur... */
    }
    /* Exit if EOF (i.e., control-D) was entered. */
    if (ch == 4) {
      break;
    }
    if (feof (termInputFile)) {
      break;
    }
    /* Echo the character. */
    if (terminalInRawMode) {
      fancyPrintChar (ch);
    }
    /* Add this character to the type-ahead buffer. */
    addToTypeAhead (ch);
  }
  turnOffTerminal ();
  printf ("\n");
  printf ("The following characters will be supplied as input to the BLITZ serial\n"
          "input before querying the terminal:\n  ");
  printTypeAheadBuffer ();
}



/* addToTypeAhead (ch)
**
** This routine is passed a character.  It adds it to the type-ahead
** buffer.  If the buffer is full, a fatal error occurs.
*/
void addToTypeAhead (int ch) {
  if (typeAheadBufferCount < TYPE_AHEAD_BUFFER_SIZE) {
    typeAheadBufferCount++;
    typeAheadBuffer [typeAheadBufferIn++] = ch;
    if (typeAheadBufferIn >= TYPE_AHEAD_BUFFER_SIZE) {
      typeAheadBufferIn = 0;
    }
  } else {
    fatalError ("The type-ahead buffer has overflowed!");
  }
}



/* printTypeAheadBuffer ()
**
** This routine prints out the type-ahead buffer.
*/
void printTypeAheadBuffer () {
  int i, j, ch;
  printf ("\"");
  j = typeAheadBufferOut;
  for (i=typeAheadBufferCount; i>0; i--) {
    ch = typeAheadBuffer[j++];
    fancyPrintChar (ch);
    if (j >= TYPE_AHEAD_BUFFER_SIZE) {
      j = 0;
    }
  }
  printf ("\"\n");
}



/* fancyPrintChar (ch)
**
** This routine is passed a character.  It prints it on stdout.  If
** the character is a non-graphic, it is print using these escapes:
**     \"  \'  \\  \0  \a  \b  \t  \n  \v  \f  \r  \xFF
** The last form is a variation on the \377 octal notation of "C", but
** in hex instead.
*/
void fancyPrintChar (int ch) {

/*****
  if (ch == 0) {
    printf ("\\0");
  } else if (ch == 8) {
    printf ("\\b");
  } else if (ch == 92) {
    printf ("\\\\");
  } else if (ch == 10) {
    printf ("\\n");
  } else if (ch == 9) {
    printf ("\\t");
  } else if (ch == 13) {
    printf ("\\r");
  } else if (ch == 34) {
    printf ("\\\"");
  } else if ((ch >= 32) && (ch < 127)) {
    printf ("%c", ch);
  } else {
    printf ("\\x%02X", ch);
  }
*****/

  if (ch == '\"') {
    printf ("\\\"");
  } else if (ch == '\'') {
    printf ("\\\'");
  } else if (ch == '\\') {
    printf ("\\\\");
  } else if ((ch >= 32) && (ch < 127)) {
    printf ("%c", ch);
  } else if (ch == '\0') {
    printf ("\\0");
  } else if (ch == '\a') {
    printf ("\\a");
  } else if (ch == '\b') {
    printf ("\\b");
  } else if (ch == '\t') {
    printf ("\\t");
  } else if (ch == '\n') {
    printf ("\\n");
  } else if (ch == '\v') {
    printf ("\\v");
  } else if (ch == '\f') {
    printf ("\\f");
  } else if (ch == '\r') {
    printf ("\\r");
  } else {
    printf ("\\x%02X", ch);
  }
}



/* readHexInt ()
**
** This routine reads in an integer from the terminal and returns it.
** This version is preliminary.  Eventually, we would like to be able to
** read an arbitary expression, evaluate it, and then return the integer.
*/
int readHexInt () {
  int i = 0;
  char * str = getToken ();
  sscanf (str, "%x", &i);
  return i;
}



/* readDecimalInt ()
**
** This routine reads in an integer from the terminal and returns it.
** This version is preliminary.  Eventually, we would like to be able to
** read an arbitary expression, evaluate it, and then return the integer.
*/
int readDecimalInt () {
  int i;
  char * str = getToken ();
  sscanf (str, "%d", &i);
  return i;
}



/* readDouble ()
**
** Read a floating-point number from the user and return it as a double.
*/
double readDouble () {
  double x = 0.0;
  char * str = toLower (getToken ());
  if (!strcmp (str, "nan")) {
    return 0.0 / 0.0;
  } else if (!strcmp (str, "inf")) {
    return 1.0 / 0.0;
  } else if (!strcmp (str, "-inf")) {
    return -1.0 / 0.0;
  }
  sscanf (str, "%lg", &x);
  return x;
}



/* readYesNo ()
**
** This routine reads either "yes" or "no" from the user and returns
** true if yes, false if no.
*/
int readYesNo () {
  char * command;
  while (1) {
    command = toLower (getToken ());
    if (!strcmp (command, "yes")) return 1;
    if (!strcmp (command, "y")) return 1;
    if (!strcmp (command, "no")) return 0;
    if (!strcmp (command, "n")) return 0;
    printf ("INVLAID ENTRY:  Please enter \"y\" or \"n\"...");
  }
}



/* commandSetI ()
** commandSetS ()
** commandSetP ()
** commandSetZ ()
** commandSetV ()
** commandSetN ()
**
** commandClearI ()
** commandClearS ()
** commandClearP ()
** commandClearZ ()
** commandClearV ()
** commandClearN ()
**
** These commands set and clear the indicated bit in the status register.
*/
void commandSetI () {
  statusI = 1;
  printf ("The I bit is now 1: Interrupts Enabled.\n");
}
void commandSetS () {
  statusS = 1;
  printf ("The S bit is now 1: System Mode.\n");
}
void commandSetP () {
  statusP = 1;
  printf ("The P bit is now 1: Paging Enabled.\n");
  printAboutToExecute ();
}
void commandSetZ () {
  statusZ = 1;
  printf ("The Z bit is now 1: Zero.\n");
}
void commandSetV () {
  statusV = 1;
  printf ("The V bit is now 1: Overflow Occurred.\n");
}
void commandSetN () {
  statusN = 1;
  printf ("The N bit is now 1: Negative.\n");
}

void commandClearI () {
  statusI = 0;
  printf ("The I bit is now 0: Interrupts Disabled.\n");
}
void commandClearS () {
  statusS = 0;
  printf ("The S bit is now 0: User Mode.\n");
}
void commandClearP () {
  statusP = 0;
  printf ("The P bit is now 0: Paging Disabled.\n");
  printAboutToExecute ();
}
void commandClearZ () {
  statusZ = 0;
  printf ("The Z bit is now 0: Not Zero.\n");
}
void commandClearV () {
  statusV = 0;
  printf ("The V bit is now 0: No Overflow.\n");
}
void commandClearN () {
  statusN = 0;
  printf ("The N bit is now 0: Not Negative.\n");
}



/* commandSetPC ()
**
** This command is used to set the PC register.
*/
void commandSetPC () {
  int i;
  printf ("Please enter the new value for the program counter (PC): ");
  i = readHexInt ();
  if (i%4 != 0) {
    printf ("ERROR: The PC must always be word aligned.  No change.\n");
    return;
  }
  pc = i;
  printf ("  PC   = ");
  printNumberNL (pc);
  printAboutToExecute ();
}



/* commandSetPTBR ()
**
** This command is used to set the PTBR register.
*/
void commandSetPTBR () {
  int i;
  printf ("Enter the new value for the Page Table Base Register (PTBR) in hex: ");
  i = readHexInt ();
  if (i%4 != 0) {
    printf ("ERROR: The PTBR must always be word aligned.  No change.\n");
    return;
  }
  ptbr = i;
  printf ("  PTBR = ");
  printNumberNL (ptbr);
}



/* commandSetPTLR ()
**
** This command is used to set the PTLR register.
*/
void commandSetPTLR () {
  int i;
  printf ("Enter the new value for the Page Table Length Register (PTLR) in hex: ");
  i = readHexInt ();
  if (i%4 != 0) {
    printf ("ERROR: The PTLR must always be word aligned.  No change.\n");
    return;
  }
  ptlr = i;
  printf ("  PTLR = ");
  printNumberNL (ptlr);
}



/* commandPrintPageTable ()
**
** This command is used to print the page table.
*/
void commandPrintPageTable () {
  int tableEntryAddr, tableEntry, frameNumber, i, addInfo;

  printf ("  Page Table:\n");
  printf ("    base   (PTBR) = 0x%08X\n", ptbr);
  printf ("    length (PTLR) = 0x%08X\n", ptlr);
  printf ("    This table describes a logical address space with 0x%08X (decimal %d) bytes (i.e., %d pages)\n", ptlr * PAGE_SIZE / 4,  ptlr * PAGE_SIZE / 4, ptlr / 4);
  printf ("\n");

  printf ("    addr      entry        Logical  Physical  Undefined Bits  Dirty  Referenced  Writeable  Valid\n");
  printf ("  ========   ========     ========  ========  ==============  =====  ==========  =========  =====\n");

  i = 0;
  for (tableEntryAddr = ptbr;
       tableEntryAddr < ptbr + ptlr;
       tableEntryAddr = tableEntryAddr + 4) {

    /* Check for an address exception.  If found, signal it and return. */
    if (!physicalAddressOk (tableEntryAddr)) {
      printf ("   This page table entry is not within physical memory!\n");
      break;
    }

    /* Get the page table entry. */
    /*   (tableEntryAddr must be aligned since ptbr is aligned.) */
    tableEntry = getPhysicalWord (tableEntryAddr);
    frameNumber = tableEntry & 0xffffe000;
    addInfo = (tableEntry & 0x00001ff0) >> 4;

    printf ("  %08X:  %08X     %08X", tableEntryAddr, tableEntry, i<<13);
    printf ("  %08X ", frameNumber);
    printf ("      %03x   ", addInfo);
    /* Print Dirty Bit */
    if (tableEntry & 0x00000008) {
      printf ("       1");
    } else {
      printf ("       0");
    }
    /* Print Referenced Bit */
    if (tableEntry & 0x00000004) {
      printf ("        1");
    } else {
      printf ("        0");
    }
    /* Print Writeable Bit */
    if (tableEntry & 0x00000002) {
      printf ("           1");
    } else {
      printf ("           0");
    }
    /* Print Valid Bit */
    if (tableEntry & 0x00000001) {
      printf ("        1");
    } else {
      printf ("        0");
    }

    if (!physicalAddressOk (frameNumber)) {
      printf ("    *** Bad physical address! ***\n");
    } else {
      printf ("\n");
    }

    i++;
    if (controlCPressed) {
      controlCPressed = 0;
      break;
    }

  }
}



/* commandTranslate ()
**
** This command command asks for a logical address.  It the perform
** address translation on it.
*/
void commandTranslate () {
  int logicalAddr, physAddr, reading, doUpdates;
  char * str;
  printf ("Please enter a logical address: ");
  logicalAddr = readHexInt ();
  printf ("Will this be a read-only operation (y/n)? ");
  reading = readYesNo ();
  printf ("After figuring out the affect of this memory access, do you want me to update the\n"
          "page table and signal exceptions, if any, as if this operation were performed (y/n)? ");
  doUpdates = readYesNo ();
  /*   printf ("reading = %d   doUpdates=%d\n", reading, doUpdates);   */
  printf ("Calling:\n    translate (logicalAddr=0x%08X, reading=", logicalAddr);
  if (reading) {
    printf ("TRUE");
  } else {
    printf ("FALSE");
  }
  printf (", wantPrinting=TRUE, doUpdates=");
  if (doUpdates) {
    printf ("TRUE");
  } else {
    printf ("FALSE");
  }
  printf (")\n");

  physAddr = translate (logicalAddr, reading, 1, doUpdates);

  if (translateCausedException) {
    printf ("An exception occurred during translation.\n");
    if (!doUpdates) {
      printf ("  This exception will be ignored: The processor state has not been modified.\n");
    } else {
      printf ("  The processor state has been modified: this exception is now pending.\n");
    }
    return;
  }
  printf ("The value of the target word in physical memory was not changed.  It is...\n");
  printf ("  0x%06X: 0x%08X\n", physAddr, getPhysicalWord (physAddr));
  if (doUpdates && statusP) {
    printf ("The page table may have been modified by this command.\n");
  } else {
    printf ("The page table has not been modified by this command.\n");
  }
}



/* commandCancel ()
**
** This routine cancels all pending interrupts.
*/
void commandCancel () {
  interruptsSignaled = 0;
  systemTrapNumber = 0;
  pageInvalidOffendingAddress = 0;
  pageReadonlyOffendingAddress = 0;
  printf ("All pending interrupts have been cancelled.\n");
}



/* commandLabels ()
**
** This routine prints the label tables, first by the alphaIndex, then
** by the valueIndex.
*/
void commandLabels () {
  int i, j;
  TableEntry * p;
  printf ("Ordered alphabetically:\n");
  printf ("    Label                          Hex Value  (in decimal)\n");
  printf ("    ============================== =========  ============\n");
  for (i=0; i<numberOfLabels; i++) {
    p = alphaIndex [i];
    printf ("    ");
    printStringInWidth (p->string, 30);
    printf ("  %08X   %11d\n", p->value, p->value);
  }
  printf ("Ordered by value:\n");
  printf ("    Label                          Hex Value  (in decimal)\n");
  printf ("    ============================== =========  ============\n");
  for (i=0; i<numberOfLabels; i++) {
    p = valueIndex [i];
    printf ("    ");
    printStringInWidth (p->string, 30);
    printf ("  %08X   %11d\n", p->value, p->value);
  }
}



/* printStringInWidth (string, width)
**
** This routine prints this string.  It is passed a field width in
** characters.  It will print exactly that many characters.  Shorter
** strings will be padded with blanks; longer string will be truncated.
** If the string length is larger than the available space, the string
** will print like this:
**       aVeryLongAndBor...tring
**
*/
void printStringInWidth (char * string, int width) {
  int i;
  int len = strlen (string);
  if (len <= width) {
    printf ("%s", string);
    for (i=width; i>len; i--) {
      printf (" ");
    }
  } else if (width < 6) {
    for (i=0; i<width; i++) {
      printf ("%c", string[i]);
    }
  } else if (width < 20) {
    for (i=0; i<width-3; i++) {
      printf ("%c", string[i]);
    }
    printf ("...");
  } else {
    for (i=0; i<width-8; i++) {
      printf ("%c", string[i]);
    }
    printf ("...");
    for (i=len-5; i<len; i++) {
      printf ("%c", string[i]);
    }
  }
}



/* commandInit ()
**
** This command reinitializes the label table.  It then allows the user to
** enter several new entries.
*/
void commandInit () {
  int j;
  char * str;
  char c, * p;
  TableEntry * tableEntry;
  int value = 100;

  numberOfLabels = 0;
  while (1) {
    if (numberOfLabels >= MAX_NUMBER_OF_LABELS) {
      printf ("Implementation Restriction: Maximum number of labels would be exceeded; aborted.\n");
      return;
    }
    printf ("Enter the name of the next label (\"\" to terminate): ");
    str = getToken ();
    if (strlen (str) == 0) return;
    /* Check to make sure it contains legal characters only. */
    p = str;
    while ((c = *p) != 0) {
      if (c>='a' && c<='z') {
      } else if (c>='A' && c<='Z') {
      } else if (c>='0' && c<='9') {
      } else if (c=='_' || c=='.') {
      } else {
        printf ("Labels must contain letters, digits, '_' and '.' only; aborted.\n");
        return;
      }
      p++;
    }
  
    tableEntry = (TableEntry *)
                     calloc (1, sizeof (TableEntry) + strlen (str) + 1);
    if (tableEntry == 0) {
      fatalError ("Calloc failed - insufficient memory available");
    }
    tableEntry -> value = genRandom () % 100;
    strcpy (tableEntry->string, str);
    alphaIndex [numberOfLabels] = tableEntry;
    valueIndex [numberOfLabels++] = tableEntry;
  }
}



/* commandInit2 ()
**
** This command reinitializes the label table.  It then allows the user to
** enter several new entries.
*/
void commandInit2 () {
  int j;
  char string [100];
  char c, * p;
  TableEntry * tableEntry;
  int value;

  numberOfLabels = 0;
  while (1) {
    if (numberOfLabels >= MAX_NUMBER_OF_LABELS) {
      printf ("Implementation Restriction: Maximum number of labels would be exceeded; aborted.\n");
      return;
    }
    printf ("Enter the value of the next label in decimal (-999 to terminate): ");
    value = readDecimalInt ();
    if (value == -999) return;

    sprintf (string, "sym_%d", value);
    tableEntry = (TableEntry *)
                     calloc (1, sizeof (TableEntry) + strlen (string) + 1);
    if (tableEntry == 0) {
      fatalError ("Calloc failed - insufficient memory available");
    }
    tableEntry -> value = value;
    strcpy (tableEntry->string, string);
    alphaIndex [numberOfLabels] = tableEntry;
    valueIndex [numberOfLabels++] = tableEntry;
  }
}



/* commandSort ()
**
** Perform a quicksort on the label table, sorting by alpha index.
*/
void commandSort () {
  quicksortAlpha (0, numberOfLabels-1);
  commandLabels ();
}



/* commandSort2 ()
**
** Perform a quicksort on the label table, sorting by value index.
*/
void commandSort2 () {
  quicksortValue (0, numberOfLabels-1);
  commandLabels ();
}



/* quicksortAlpha ()
**
** This routine sorts the elements in the array from m through n (inclusive).
** The sort is on the alphaIndex.
*/
void quicksortAlpha (int m, int n) {
  int i;
  if (n>m) {
    i = partitionAlpha (m, n);
    quicksortAlpha (m, i-1);
    quicksortAlpha (i+1, n);
  }
}



/* partitionAlpha (l, r)
**
** This function is used in quicksorting the array.  It is passed two
** indexes ("l" and "r") which elements of the array are to be examined
** and rearranged.  This routine will rearrange the elements in the
** selected portion of the array, such that, after the partitioning,
** all elements with indexes between "l" and "i" (inclusive) will be <=
** a[i] and all elements with indexes between "i" and "r" (inclusive)
** will be >= a[i].  This routine will compute and return an "i" that
** satisfies the above conditions.
*/
int partitionAlpha (int l, int r) {
  int i, j;
  TableEntry * v, * t;
/***
  printf ("==== Partition (%d, %d)\n", l, r);
  dump ();
***/
  v = alphaIndex[r];
  i = l-1;
  j = r;
  while (1) {
    i++;
    while (strcmp (alphaIndex[i]->string , v->string) < 0) {
      i++;
    }
    j--;
    while ((j>=l) && (strcmp (alphaIndex[j]->string , v->string) > 0)) {
      j--;
    }
    t = alphaIndex[i];
    alphaIndex[i] = alphaIndex[j];
    alphaIndex[j] = t;
    if (j<=i) break;
  }
  alphaIndex[j] = alphaIndex[i];
  alphaIndex[i] = alphaIndex[r];
  alphaIndex[r] = t;
/***
  dump ();
  printf ("====  returns i=  %d\n", i);
***/
  return i;
}



/* quicksortValue ()
**
** This routine sorts the elements in the array from m through n (inclusive).
** The sort is on the valueIndex.
*/
void quicksortValue (int m, int n) {
  int i;
  if (n>m) {
    i = partitionValue (m, n);
    quicksortValue (m, i-1);
    quicksortValue (i+1, n);
  }
}



/* partitionValue (l, r)
**
** This function is used in quicksorting the array.  It is passed two
** indexes ("l" and "r") which elements of the array are to be examined
** and rearranged.  This routine will rearrange the elements in the
** selected portion of the array, such that, after the partitioning,
** all elements with indexes between "l" and "i" (inclusive) will be <=
** a[i] and all elements with indexes between "i" and "r" (inclusive)
** will be >= a[i].  This routine will compute and return an "i" that
** satisfies the above conditions.
*/
int partitionValue (int l, int r) {
  int i, j;
  TableEntry * v, * t;
/***
  printf ("==== Partition (%d, %d)\n", l, r);
  dump ();
***/
  v = valueIndex[r];
  i = l-1;
  j = r;
  while (1) {
    i++;
    while (valueIndex[i]->value < v->value) {
      i++;
    }
    j--;
    while ((j>=l) && (valueIndex[j]->value>v->value)) {
      j--;
    }
    t = valueIndex[i];
    valueIndex[i] = valueIndex[j];
    valueIndex[j] = t;
    if (j<=i) break;
  }
  valueIndex[j] = valueIndex[i];
  valueIndex[i] = valueIndex[r];
  valueIndex[r] = t;
/***
  dump ();
  printf ("====  returns i=  %d\n", i);
***/
  return i;
}



/* commandFind ()
**
** Find a label by name and print it.
*/
void commandFind () {
  int i, prefixLength;
  char * str;
  TableEntry * tableEntry;
  printf ("Enter the first few characters of the label; all matching labels will be printed: ");
  str = getToken ();
  prefixLength = strlen (str);
  printf ("    Label                          Hex Value  (in decimal)\n");
  printf ("    ============================== =========  ============\n");
  for (i=0; i<numberOfLabels; i++) {
    tableEntry = alphaIndex [i];
    if (strlen (tableEntry->string) >= prefixLength) {
      if (bytesEqual (str, tableEntry->string, prefixLength, prefixLength)) {
        printf ("    ");
        printStringInWidth (tableEntry->string, 30);
        printf ("  %08X   %11d\n", tableEntry->value, tableEntry->value);
      }
    }
  }

/*****
  tableEntry = findLabelByAlpha (str, 0, numberOfLabels-1);
  if (tableEntry == NULL) {
    printf ("  Nothing found\n");
  } else {
    printf ("  %08X   (decimal: %d)   %s\n",
            tableEntry->value, tableEntry->value, tableEntry->string);
  }
*****/
}



/* commandFind2 ()
**
** Find a label by value and print it.
*/
void commandFind2 () {
  int value, j, index;
  TableEntry * tableEntry;
  printf ("Enter the value to find (in hex): ");
  value = readHexInt ();
  index = findLabel (value);
  index = findLabel (value);
  if (index == -1) {
    printf ("  Nothing found\n");
  } else {
    while (1) {
      if (index == -1) break;
      if (index >= numberOfLabels) break;
      tableEntry = valueIndex [index++];
      if (tableEntry->value != value) break;
      printf ("  %08X   (decimal: %d)   %s\n",
            tableEntry->value, tableEntry->value, tableEntry->string);
    }
  }
}



/* findLabel (value)
**
** This routine is passed a value.  It searches the label table for a
** label with that value.  If one is found, it returns the index within
** valueIndex of the entry.  There may be several labels with the desired
** value; this routine will return the one with the lowest index.  If the
** table is empty or contains no entries equal to value, it returns
** index = -1.
*/
int findLabel (int value) {
  /* Do the binary search to find a matching label.  */
  int index = findLabel2 (value, 0, numberOfLabels-1);
  /* If none at all, return. */
  if (index == -1) return index;
  /* Find the lowest numbered index that matches. */
  while (1) {
    if (index == 0) return index;
    if (valueIndex[index-1]->value != value) return index;
    index--;
  }
}



/* findLabel2 (value, low, high)
**
** This routine is passed a value.  It searches the label table for a
** label with that value.  If one is found, it returns the index within
** valueIndex of the entry.  There may be several labels with the desired
** value; this routine will return an arbitrary on.  If the table is empty
** or contains no entries equal to value, it returns index = -1.
**
** This routine uses a binary search.  It is passed "low" and "high" which
** indicate which portion of the valueIndex to search.
**
** The initial call should look like this:
**      i = findLabel2 (value, 0, numberOfLabels-1);
*/
int findLabel2 (int value, int low, int high) {
  int mid;
  if (high - low > 2) {
    mid = ((high - low) / 2) + low;
    if (valueIndex [mid]->value <= value) {
      return findLabel2 (value, mid, high);
    } else {
      return findLabel2 (value, low, mid-1);
    }
  } else if (high - low == 2) {
    if (valueIndex [high]->value == value) return high; 
    if (valueIndex [high-1]->value == value) return high-1;
    if (valueIndex [low]->value == value) return low;
    return -1;
  } else if (high - low == 1) {
    if (valueIndex [high]->value == value) return high;
    if (valueIndex [low]->value == value) return low;
    return -1;
  } else if (high == low) {
    if (valueIndex [high]->value == value) return high;
    return -1;
  } else {
    return -1;
  }
}



/* findLabelByAlpha (string, low, high)
**
** This routine is passed a label string.  It searches the label table
** and returns a ptr to the TableEntry of the label with that string.
** If none matches exactly, it returns NULL.
**
** This routine uses a binary search.  It is passed "low" and "high" which
** indicate which portion of the alphaIndex to search.
**
** The initial call should look like this:
**    p = findLabelByAlpha (string, 0, numberOfLabels-1);
*/
TableEntry * findLabelByAlpha (char * string, int low, int high) {
  int mid;
  if (high - low > 2) {
    mid = ((high - low) / 2) + low;
    if (strcmp (alphaIndex [mid]->string , string) <= 0) {
      return findLabelByAlpha (string, mid, high);
    } else {
      return findLabelByAlpha (string, low, mid-1);
    }
  } else if (high - low == 2) {
    if (strcmp (alphaIndex [high]->string , string) == 0)
      return alphaIndex [high];
    if (strcmp (alphaIndex [high-1]->string , string) == 0)
      return alphaIndex [high-1];
    if (strcmp (alphaIndex [low]->string , string) == 0)
      return alphaIndex [low];
    return NULL;
  } else if (high - low == 1) {
    if (strcmp (alphaIndex [high]->string , string) == 0)
      return alphaIndex [high];
    if (strcmp (alphaIndex [low]->string , string) == 0)
      return alphaIndex [low];
    return NULL;
  } else if (high == low) {
    if (strcmp (alphaIndex [high]->string , string) == 0)
      return alphaIndex [high];
    return NULL;
  } else {
    return NULL;
  }
}



/* commandAdd ()
**
** This command prompts for a new label and value.  Then it adds it to
** the label table.
*/
void commandAdd () {
  int j;
  char * str;
  char c, * p;
  TableEntry * tableEntry;

  printf ("Enter the name of the new label: ");
  str = getToken ();

  /* Check to make sure it contains legal characters only. */
  p = str;
  if (strlen (p) <= 0) {
    printf ("Null string -- command aborted\n");
    return;
  }
  while ((c = *p) != 0) {
    if (c>='a' && c<='z') {
    } else if (c>='A' && c<='Z') {
    } else if (c>='0' && c<='9') {
    } else if (c=='_' || c=='.') {
    } else {
      printf ("Labels must contain letters, digits, '_' and '.' only; aborted.\n");
      return;
    }
    p++;
  }

  /* See if this label already exists and abort if so. */
  tableEntry = findLabelByAlpha (str, 0, numberOfLabels-1);
  if (tableEntry != NULL) {
    printf ("  ERROR: Label already exists. You may not delete or change existing labels.\n");
    return;
  }

  /* Create a new TableEntry. */
  tableEntry = (TableEntry *)
                   calloc (1, sizeof (TableEntry) + strlen (str) + 1);
  if (tableEntry == 0) {
    fatalError ("Calloc failed - insufficient memory available");
  }
  strcpy (tableEntry->string, str);

  /* Get the new label's value. */
  printf ("Enter the value of the new label (in hex): ");
  tableEntry -> value = readHexInt ();

  /* Now insert this label into the alphaIndex and the valueIndex. */
  insertLabel (tableEntry);

  printf ("Label \"%s\" has been added.\n", tableEntry->string);
}



/* insertLabel (p)
**
** This routine is passed a pointer to a new TableEntry.  It inserts
** an index for it into the alphaIndex and into the valueIndex.  If there
** is no room, this routine will call fatalError().
*/
void insertLabel (TableEntry * p) {
  int i;
  if (numberOfLabels >= MAX_NUMBER_OF_LABELS) {
    fatalError ("Implementation restriction - the label table is full");
  }
  /* Shift all the labels in the valueIndex up to open up a slot. */
  i = numberOfLabels;
  while (i>0) {
    if (valueIndex[i-1]->value <= p->value) break;
    valueIndex [i] = valueIndex [i-1];
    i--;
  }
  /* Now i points to the vacant entry; put the new label there. */
  valueIndex [i] = p;
  /* Shift all the labels in the alphaIndex up to open up a slot. */
  i = numberOfLabels;
  while (i>0) {
    if (strcmp (alphaIndex[i-1]->string , p->string) <= 0) break;
    alphaIndex [i] = alphaIndex [i-1];
    i--;
  }
  /* Now i points to the vacant entry; put the new label there. */
  alphaIndex [i] = p;
  /* Increment the number of labels. */
  numberOfLabels++;
}



/* insertLabelUnsorted (p)
**
** This routine is passed a pointer to a new TableEntry.  It adds
** an index for it to the alphaIndex and to the valueIndex.  This label
** is simply placed at the end of the array; there is no sorted insert.
** If there is no room, this routine will call fatalError().
*/
void insertLabelUnsorted (TableEntry * p) {
  if (numberOfLabels >= MAX_NUMBER_OF_LABELS) {
    fatalError ("Implementation restriction - the label table is full");
  }
  valueIndex [numberOfLabels] = p;
  alphaIndex [numberOfLabels] = p;
  numberOfLabels++;
}



/* commandReset ()
**
** This command resets the entire machine state and re-reads the
** a.out file into memory.
*/
void commandReset () {
  printf ("Resetting all CPU registers and re-reading file \"%s\"...\n",
    executableFileName);
  resetState ();
  /* printInfo (); */
}



/* resetState ()
**
** This routine resets the entire machine state and re-reads the a.out
** file into memory.
*/
void resetState () {
  int i, magic, len;
  char * targetAddr, * p;
  TableEntry * tableEntry;
  char buffer [2];
  FILE * blitzrc;
  char * first, * second;

  setSimulationConstants ();

  if (commandOptionRand) {
    randomSeed = randSeedFromOption;
  } else {
    randomSeed = INIT_RANDOM_SEED;
  }

  numberOfDiskReads = 0;
  numberOfDiskWrites = 0;

  /* Clear currentAddr, which is used in commandDis. */
  currentAddr = 0;

  /* Clear currentFrame, which is used in commandStackUp, commandStackDown. */
  currentFrame = 0;

  /* Clear out the label table. */
  numberOfLabels = 0;

  /* Reset misc. CPU registers. */
  pc = 0;
  ptbr = 0;
  ptlr = 0;
  statusN = 0;
  statusV = 0;
  statusZ = 0;
  statusP = 0;
  statusS = 1;
  statusI = 0;

  /* Reset any signalled interrupts. */
  interruptsSignaled = 0;
  systemTrapNumber = 0;
  pageInvalidOffendingAddress = 0;
  pageReadonlyOffendingAddress = 0;

  currentTime = 0;
  timeSpentAsleep = 0;
  timeOfNextTimerEvent = MAX;
  timeOfNextDiskEvent = MAX;
  timeOfNextSerialInEvent = 0;
  timeOfNextSerialOutEvent = MAX;
  doTimerEvent ();
  executionHalted = 0;
  controlCPressed = 0;

  /* Reset the disk */
  initializeDisk ();

  /* Clear out the typeAheadBuffer. */
  typeAheadBufferCount = 0;
  typeAheadBufferIn = 0;
  typeAheadBufferOut = 0;

  /* Position the terminal input file to the beginning. */
  if (termInputFile != stdin) {
    errno = 0;
    if (fseek (termInputFile, 0l, SEEK_SET)) {
      if (errno) perror ("Error on terminal input");
      fatalError ("Error from fseek for terminal input file");
    }
  }

  /* Reset the serial device. */
  terminalWantRawProcessing = commandOptionRaw;   /* Default is to have buffered input */
  termInChar = 0;                   /* Last keyboard key pressed */
  termInCharAvail = 0;              /* No input available at this time */
  termInCharWasUsed = 1;            /* Last character was absorbed ok */
  termOutputReady = 1;              /* Output device is not busy */

  updateTimeOfNextEvent ();

  /* If we have already allocated memory, free it. */
  if (memory != NULL) {
    free (memory);
  }

  /* Allocate a chunk of memory which will hold the BLITZ machine memory. */
  memory = (char *) calloc (1, MEMORY_SIZE);
  fflush (stdout);
  if (memory == 0) {
    fatalError ("Calloc failed - insufficient memory available");
  }
  currentMemoryLock = -1;

  /* Initialize all integer and floating-point registers to zero. */
  for (i=0; i<=15; i++) {
    userRegisters [i] = 0;
    systemRegisters [i] = 0;
    floatRegisters [i] = 0.0;
  }

  /* Reposition the a.out file to the beginning. */
  errno = 0;
  if (fseek (executableFile, 0l, SEEK_SET)) {
    if (errno) perror ("Error on a.out file");
    fatalError ("Error from fseek for a.out file");
  }

  /* Check the magic number. */
  magic = readInteger (executableFile);
  if (magic != 0x424C5A78) {
    fatalError ("Error in 'a.out' executable file: Magic number is not 'BLZx'");
  }

  /* Read in the header info. */
  textSize = readInteger (executableFile);
  dataSize = readInteger (executableFile);
  bssSize = readInteger (executableFile);
  textAddr = readInteger (executableFile);
  dataAddr = readInteger (executableFile);
  bssAddr = readInteger (executableFile);

  /* Compute the maximum address used and check that this will fit
     into available memory. */
  if (textAddr + textSize > MEMORY_SIZE) {
    fatalError ("The text segment will not fit into memory");
  }
  if (dataAddr + dataSize > MEMORY_SIZE) {
    fatalError ("The data segment will not fit into memory");
  }
  if (bssAddr + bssSize > MEMORY_SIZE) {
    fatalError ("The bss segment will not fit into memory");
  }

  /* Read in **** separator. */
  magic = readInteger (executableFile);
  if (magic != 0x2a2a2a2a) {
    fatalError ("Invalid file format - missing \"****\" separator");
  }

  /* Read in the text segment. */
  targetAddr = memory + textAddr;
  errno = 0;
  i = fread (targetAddr, 1, textSize, executableFile);
  if (i != textSize) {
    if (errno) perror ("Error reading from a.out file");
    fatalError ("Problems reading text segment from a.out file");
  }

  /* Read in **** separator. */
  magic = readInteger (executableFile);
  if (magic != 0x2a2a2a2a) {
    fatalError ("Invalid file format - missing \"****\" separator");
  }

  /* Read in the data segment. */
  targetAddr = memory + dataAddr;
  errno = 0;
  i = fread (targetAddr, 1, dataSize, executableFile);
  if (i != dataSize) {
    if (errno) perror ("Error reading from a.out file");
    fatalError ("Problems reading data segment from a.out file");
  }

  /* Read in **** separator. */
  magic = readInteger (executableFile);
  if (magic != 0x2a2a2a2a) {
    fatalError ("Invalid file format - missing \"****\" separator");
  }

  /* There is nothing to do for the bss segment, since the calloc will
     have cleared the memory to zero already. */

  /* Read in the labels and add to the end of the label table. */
  while (1) {
    len = readInteger (executableFile);
    if (len <= 0) break;
    tableEntry = (TableEntry *)
                     calloc (1, sizeof (TableEntry) + len + 1);
    if (tableEntry == 0) {
      fatalError ("Calloc failed - insufficient memory available");
    }
    tableEntry -> value = readInteger (executableFile);
    p = &( tableEntry->string[0] );
    for (; len>0; len--) {
      *p++ = readByte (executableFile);
    }
    p = 0;
    insertLabelUnsorted (tableEntry);
  }

  /* Sort the label table by alpha and value. */
  quicksortAlpha (0, numberOfLabels-1);
  quicksortValue (0, numberOfLabels-1);

  /* Read in **** separator. */
  magic = readInteger (executableFile);
  if (magic != 0x2a2a2a2a) {
    fatalError ("Invalid file format - missing \"****\" separator");
  }

  /* Finally, set the PC to the first address in the .text segment. */
  pc = textAddr;

}



/* printInfo ()
**
** This routine prints out misc. info about the state of memory.
*/
void printInfo () {
  printf ("Memory size = ");
  printNumberNL2 (MEMORY_SIZE);
  printf ("Page size   = ");
  printNumberNL2 (PAGE_SIZE);
/***
  printf ("Memory loc  = ");
  printNumberNL2 ((int) memory);
***/

  printf (".text Segment\n");
  printf ("    addr    = ");
  printNumberNL2 (textAddr);
  printf ("    size    = ");
  printNumberNL2 (textSize);

  printf (".data Segment\n");
  printf ("    addr    = ");
  printNumberNL2 (dataAddr);
  printf ("    size    = ");
  printNumberNL2 (dataSize);

  printf (".bss Segment\n");
  printf ("    addr    = ");
  printNumberNL2 (bssAddr);
  printf ("    size    = ");
  printNumberNL2 (bssSize);
}



/* commandReg (reg)
**
** This command allows the user to change the value of one of the
** registers, r1..r15.  It looks at the status register mode bit
** and either changes a system register or a user register.
*/
void commandReg (int reg) {
  int value;

  /* Print out the old value. */
  if (statusS == 1) {
    printf ("  SYSTEM r%d = ", reg);
    printNumberNL (systemRegisters [reg]);
  } else {
    printf ("  USER r%d = ", reg);
    printNumberNL (userRegisters [reg]);
  }

  /* Read in the new value. */
  printf ("Enter the new value (in hex): ");
  value = readHexInt ();

  /* Change the register and print out the new value. */
  if (statusS == 1) {
    systemRegisters [reg] = value;
    printf ("  SYSTEM r%d = ", reg);
    printNumberNL (systemRegisters [reg]);
  } else {
    userRegisters [reg] = value;
    printf ("  USER r%d = ", reg);
    printNumberNL (userRegisters [reg]);
  }
}



/* commandFloatReg (reg)
**
** This command allows the user to change the value of one of the
** floating-point registers, f0..f15.
*/
void commandFloatReg (int reg) {
  double value, d;

  /* Print out the old value. */
  d = floatRegisters [reg];
  printf ("  f%d = ", reg);
  printDouble (d);

  /* Read in the new value. */
  printf ("Enter the new value (e.g., 1.1, -123.456e-10, nan, inf, -inf): ");
  value = readDouble ();

  /* Change the register and print out the new value. */
  floatRegisters [reg] = value;
  printf ("  f%d = ", reg);
  printDouble (value);
}



/* commandFMem ()
**
** This command allows the user to dump memory, interpreting the
** contents as floating point numbers.
*/
void commandFMem () {
  int i, count, index, implAddr, physAddr, from, to;
  double d;
  TableEntry * tableEntry;

  printf ("Enter the beginning phyical address (in hex): ");
  physAddr = readHexInt ();
  physAddr = physAddr & 0xfffffffc;
  if (!physicalAddressOk (physAddr)) {
    printf ("This is not a valid physical memory address.\n");
    return;
  }
  count = 32;  /* Display 32 floating point numbers. */
  printf ("Dumping 256 bytes as 32 double-precision floating-points...\n");
  physAddr = physAddr & 0xfffffffc;
  for (i=0; i<count; i++) {

    /* Check the address to make sure it is legal. */
    if (!physicalAddressOk (physAddr) || !isAligned (physAddr)) {
      printf ("Bad address - aborting.\n");
      return;
    }

    /* Fetch the data. */
    implAddr = ((int) memory) + physAddr;
    to = (int) & d;
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
    * (int *) (to + 0) = SWAP_BYTES (* (int *) (implAddr+4));
    * (int *) (to + 4) = SWAP_BYTES (* (int *) (implAddr+0));
#else
    * (int *) (to + 0) = * (int *) (implAddr+0);
    * (int *) (to + 4) = * (int *) (implAddr+4);
#endif
    /* Print out the label(s) if any. */
    index = findLabel (physAddr);
    while (1) {
      if (index == -1) break;
      if (index >= numberOfLabels) break;
      tableEntry = valueIndex [index++];
      if (tableEntry->value != physAddr) break;
      printf ("%s:\n", tableEntry->string);
    }

    /* Print out the address and the value. */
    printf ("   %06X: %08X %08X   value = %.15g\n",
      physAddr,
      SWAP_BYTES (* (int *) implAddr),
      SWAP_BYTES (* (int *) (implAddr + 4)),
      d);

    physAddr = physAddr + 8;
  }

}



/* commandDis ()
**
** This routine disassembles BLITZ instructions.  It asks the users
** where to begin.
*/
void commandDis () {
  printf ("Enter the beginning phyical address (in hex): ");
  currentAddr = readHexInt ();
  currentAddr = currentAddr & 0xfffffffc;
  if (!physicalAddressOk (currentAddr)) {
    printf ("This is not a valid physical memory address.\n");
    currentAddr = 0;
    return;
  }
  commandDis2 ();
}



/* commandDis2 ()
**
** This routine disassembles several BLITZ instructions from the current
** address.
*/
void commandDis2 () {
  int i, count, index;
  TableEntry * tableEntry;
  count = 30;
  currentAddr = currentAddr & 0xfffffffc;
  for (i=0; i<count; i++) {
    if (!physicalAddressOk (currentAddr)) {
      printf ("Physical memory limit exceeded.\n");
      currentAddr = 0;
      return;
    }
    disassemble (currentAddr);
    currentAddr = currentAddr + 4;
  }
}



/* disassemble (physAddr)
**
** This routine disassembles and prints a single BLITZ instruction.
** This routine is passed the addr.  If this address is not a
** a legal physical address in the BLITZ memory, it prints a message
** and returns.
*/
void disassemble (int physAddr) {
  int opcode, opcode2, cat, n, index, implAddr, instr, instr2, i1, i2;
  TableEntry * tableEntry;
  char c;

  char * opcodes [] = {

/* 0 .. 31 */
"nop", "wait", "debug", "cleari", "seti", "clearp", "setp", "clears",
"reti", "ret", "debug2", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",

/* 32 .. 63 */
"ldptbr", "ldptlr", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",

/* 64 .. 95 */
"call", "jmp", "be", "bne", "bl", "ble", "bg", "bge",
"---", "---", "bvs", "bvc", "bns", "bnc", "bss", "bsc",
"bis", "bic", "bps", "bpc", "push", "pop", "readu", "writeu",
"tset", "ftoi", "itof", "fcmp", "fsqrt", "fneg", "fabs", "---",

/* 96 .. 127 */
"add", "sub", "mul", "div", "sll", "srl", "sra", "or",
"and", "andn", "xor", "load", "loadb", "loadv", "loadbv", "store",
"storeb", "storev", "storebv", "rem", "fadd", "fsub", "fmul", "fdiv",
"fload", "fstore", "---", "---", "---", "---", "---", "---",

/* 128 .. 159 */
"add", "sub", "mul", "div", "sll", "srl", "sra", "or",
"and", "andn", "xor", "load", "loadb", "loadv", "loadbv", "store",
"storeb", "storev", "storebv", "readu", "writeu", "rem", "fload", "fstore",
"---", "---", "---", "---", "---", "---", "---", "---",

/* 160 .. 191 */
"call", "jmp", "be", "bne", "bl", "ble", "bg", "bge",
"---", "---", "bvs", "bvc", "bns", "bnc", "bss", "bsc",
"bis", "bic", "bps", "bpc", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",

/* 192 .. 223 */
"sethi", "setlo", "ldaddr", "syscall", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",

/* 224 .. 255 */
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",
"---", "---", "---", "---", "---", "---", "---", "---",

};

  int  category [] = {
/* 0 .. 31
nop, wait, debug, cleari, seti, clearp, setp, clears,
reti, ret, debug2, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
*/
1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,

/* 32 .. 63
ldptbr, ldptlr, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
*/
17, 17, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,

/* 64 .. 95
call, jmp, be, bne, bl, ble, bg, bge,
xxx, xxx, bvs, bvc, bns, bnc, bss, bsc,
bis, bic, bps, bpc, push, pop, readu, writeu,
tset, ftoi, itof, fcmp, fsqrt, fneg, fabs, xxx,
*/
8, 8, 8, 8, 8, 8, 8, 8,
0, 0, 8, 8, 8, 8, 8, 8,
8, 8, 8, 8, 10, 11, 15, 16,
14, 19, 20, 22, 22, 22, 22, 0,

/* 96 .. 127
add, sub, mul, div, sll, srl, sra, or,
and, andn, xor, load, loadb, loadv, loadbv, store,
storeb, storev, storebv, rem, fadd, fsub, fmul, fdiv,
fload, fstore, xxx, xxx, xxx, xxx, xxx, xxx,
*/
2, 2, 2, 2, 2, 2, 2, 2,
2, 2, 2, 4, 4, 4, 4, 6,
6, 6, 6, 2, 21, 21, 21, 21,
23, 25, 0, 0, 0, 0, 0, 0,

/* 128 .. 159
add, sub, mul, div, sll, srl, sra, or,
and, andn, xor, load, loadb, loadv, loadbv, store,
storeb, storev, storebv, readu, writeu, rem, fload, fstore,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
*/
3, 3, 3, 3, 3, 3, 3, 3,
3, 3, 3, 5, 5, 5, 5, 7,
7, 7, 7, 7, 5, 3, 24, 26,
0, 0, 0, 0, 0, 0, 0, 0,

/* 160 .. 191
call, jmp, be, bne, bl, ble, bg, bge,
xxx, xxx, bvs, bvc, bns, bnc, bss, bsc,
bis, bic, bps, bpc, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
*/
9, 9, 9, 9, 9, 9, 9, 9,
0, 0, 9, 9, 9, 9, 9, 9,
9, 9, 9, 9, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,

/* 192 .. 223
sethi, setlo, ldaddr, syscall, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
*/
12, 12, 18, 13, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,

/* 224 .. 255
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
xxx, xxx, xxx, xxx, xxx, xxx, xxx, xxx,
*/
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,

};

  /* Check the address to make sure it is legal. */
  if (!physicalAddressOk (physAddr) || !isAligned (physAddr)) {
    printf ("Disassemble: Bad address.\n");
    return;
  }

  /* Fetch the instruction. */
  implAddr = ((int) memory) + physAddr;
  instr = SWAP_BYTES (* (int *) implAddr);

  /* Print out the label(s) if any. */
  index = findLabel (physAddr);
  while (1) {
    if (index == -1) break;
    if (index >= numberOfLabels) break;
    tableEntry = valueIndex [index++];
    if (tableEntry->value != physAddr) break;
    printf ("                   %s:\n", tableEntry->string);
  }
  printf ("%06X: %08X       ", physAddr, instr);

  opcode = (instr >> 24) & 0x000000ff;
  printStringInWidth (opcodes[opcode], 8);

  cat = category[opcode];
  switch (cat) {
    case 0:
      printf ("\t\t\t! Not an instruction\n");
      break;
    case 1:
      printf ("\n");
      break;
    case 2:
      printRa (instr);
      printf (",");
      printRb (instr);
      printf (",");
      printRc (instr);
      printf ("\n");
      break;
    case 3:
      printRa (instr);
      printf (",");
      printData16 (instr);
      printf (",");
      printRc (instr);
      printComment (instr & 0x0000ffff);
      break;
    case 4:
      printf ("[");
      printRa (instr);
      printf ("+");
      printRb (instr);
      printf ("],");
      printRc (instr);
      printf ("\n");
      break;
    case 5:
      printf ("[");
      printRa (instr);
      printf ("+");
      printData16 (instr);
      printf ("],");
      printRc (instr);
      printComment2 (instr & 0x0000ffff);
      break;
    case 6:
      printRc (instr);
      printf (",[");
      printRa (instr);
      printf ("+");
      printRb (instr);
      printf ("]");
      printf ("\n");
      break;
    case 7:
      printRc (instr);
      printf (",[");
      printRa (instr);
      printf ("+");
      printData16 (instr);
      printf ("]");
      printComment2 (instr & 0x0000ffff);
      break;
    case 8:
      printRa (instr);
      printf ("+");
      printRc (instr);
      printf ("\n");
      break;
    case 9:
      printData24 (instr);
      n = instr << 8;
      if (n<0) {
        n = instr | 0xff000000;
      } else {
        n = instr & 0x00ffffff;
      }
      index = findLabel (n+physAddr);
      if (index != -1) {
        printf ("\t\t! targetAddr = %s", valueIndex[index]->string);
      } else {
        printf ("\t\t! targetAddr = 0x%06X ", n+physAddr);
      }
      printf ("\n");
      break;
    case 10:
      printRc (instr);
      printf (",[--");
      printRa (instr);
      printf ("]");
      printf ("\n");
      break;
    case 11:
      printf ("[");
      printRa (instr);
      printf ("++],");
      printRc (instr);
      printf ("\n");
      break;
    case 12:      /* sethi, setlo */
      printData16 (instr);
      printf (",");
      printRc (instr);
      /* Fetch the next instruction. */
      if (physicalAddressOk (physAddr+4)) {
        implAddr = ((int) memory) + physAddr + 4;
        instr2 = SWAP_BYTES (* (int *) implAddr);
        opcode2 = (instr2 >> 24) & 0x000000ff;
      } else {
        opcode2 = 0;
      }
      /* If this instruction is a sethi and the next is a setlo, then... */
      if ((opcode == 192) && (opcode2 == 193)) {   /* SETHI == 192, SETLO=193 */
        i1 = instr & 0x0000ffff;
        i2 = instr2 & 0x0000ffff;
        printf ("\t! 0x%04X%04X = %d", i1, i2, (i1<< 16)|i2);
        index = findLabel ((i1<< 16)|i2);
        if (index != -1) {
          printf (" (%s)", valueIndex[index]->string);
        }
      }
      printf ("\n");
      break;
    case 13:
      printRc (instr);
      printf ("+");
      printData16 (instr);
      printComment2 (instr & 0x0000ffff);
      break;
    case 14:
      printf ("[");
      printRa (instr);
      printf ("],");
      printRc (instr);
      printf ("\n");
      break;
    case 15:
      printRc (instr);
      printf (",");
      printRa (instr);
      printf ("\n");
      break;
    case 16:
      printRa (instr);
      printf (",");
      printRc (instr);
      printf ("\n");
      break;
    case 17:
      printRc (instr);
      printf ("\n");
      break;
    case 18:            // i.e., the ldaddr instruction
      printData16 (instr);
      printf (",");
      printRc (instr);
      n = instr << 16;
      if (n<0) {
        n = instr | 0xffff0000;
      } else {
        n = instr & 0x0000ffff;
      }
      index = findLabel (n+physAddr);
      if (index != -1) {
        printf ("\t! targetAddr = %s", valueIndex[index]->string);
      } else {
        printf ("\t! targetAddr = 0x%06X ", n+physAddr);
      }
      printf ("\n");
      break;
    case 19:
      printFRa (instr);
      printf (",");
      printRc (instr);
      printf ("\n");
      break;
    case 20:
      printRa (instr);
      printf (",");
      printFRc (instr);
      printf ("\n");
      break;
    case 21:
      printFRa (instr);
      printf (",");
      printFRb (instr);
      printf (",");
      printFRc (instr);
      printf ("\n");
      break;
    case 22:
      printFRa (instr);
      printf (",");
      printFRc (instr);
      printf ("\n");
      break;
    case 23:
      printf ("[");
      printRa (instr);
      printf ("+");
      printRb (instr);
      printf ("],");
      printFRc (instr);
      printf ("\n");
      break;
    case 24:
      printf ("[");
      printRa (instr);
      printf ("+");
      printData16 (instr);
      printf ("],");
      printFRc (instr);
      printComment2 (instr & 0x0000ffff);
      break;
    case 25:
      printFRc (instr);
      printf (",[");
      printRa (instr);
      printf ("+");
      printRb (instr);
      printf ("]");
      printf ("\n");
      break;
    case 26:
      printFRc (instr);
      printf (",[");
      printRa (instr);
      printf ("+");
      printData16 (instr);
      printf ("]");
      printComment2 (instr & 0x0000ffff);
      break;
    default:
      fatalError ("PROGRAM LOGIC ERROR: within disassemble");
  }
}



/* printRa (instr)
** printRb (instr)
** printRc (instr)
** printFRa (instr)
** printFRb (instr)
** printFRc (instr)
** printData16 (instr)
** printData24 (instr)
**
** These routines print the selected field from the instruction.
*/
void printRa (int instr) {
  printf ("r%d", (instr >> 16) & 0x0000000f);
}

void printRb (int instr) {
  printf ("r%d", (instr >> 12) & 0x0000000f);
}

void printRc (int instr) {
  printf ("r%d", (instr >> 20) & 0x0000000f);
}

void printFRa (int instr) {
  printf ("f%d", (instr >> 16) & 0x0000000f);
}

void printFRb (int instr) {
  printf ("f%d", (instr >> 12) & 0x0000000f);
}

void printFRc (int instr) {
  printf ("f%d", (instr >> 20) & 0x0000000f);
}

void printData16 (int instr) {
  printf ("0x%04X", instr & 0x0000ffff);
}

void printData24 (int instr) {
  printf ("0x%06X", instr & 0x00ffffff);
}



/* printComment (i)
**
** This routine prints a number in the form
**       ! decimal = 32767  ".."  (label)
** There is a leading tab printed and the output is followed by a newline.
*/
void printComment (int i) {
  char c;
  int index;

  /* Sign extend the 16 bit number in i. */
  if ((i << 16) < 0) {
    i = i | 0xffff0000;
  } else {
    i = i & 0x0000ffff;
  }

  if (i != 0) {
    printf ("\t! decimal: %d, ascii: \"", i);
    c = (i >> 8) & 0x000000ff;
    if ((c>=' ') && (c <= '~')) {
      putchar (c);
    } else {
      putchar ('.');
    }
    c = i & 0x000000ff;
    if ((c>=' ') && (c <= '~')) {
      putchar (c);
    } else {
      putchar ('.');
    }
    printf ("\"  ");
    index = findLabel (i);
    if (index != -1) {
      printf ("(%s)", valueIndex [index]->string);
    }
  }
  printf ("\n");
}



/* printComment2 (i)
**
** This routine prints a number in the form
**       ! decimal = -2147483648  (label)
** There is a leading tab printed and the output is followed by a newline.
*/
void printComment2 (int i) {
  char c;
  int index;

  /* Sign extend the 16 bit number in i. */
  if ((i << 16) < 0) {
    i = i | 0xffff0000;
  } else {
    i = i & 0x0000ffff;
  }

  /* Print the number. */
  printf ("\t! decimal: %d", i);

  /* If there is a label with that value, print it too. */
  index = findLabel (i);
  if (index != -1) {
    printf ("  (%s)", valueIndex [index]->string);
  }
  printf ("\n");
}



/* printAboutToExecute ()
**
** This routine disassembles and prints the next instruction, which
** is about to be executed.  It will not cause any exceptions, but will
** notify the user if there would be problems.
*/
void printAboutToExecute () {
  int physAddr;
  /* Call translate with doReading=1, wantPrinting=0, and doUpdates=0. */
  physAddr = translate (pc, 1, 0, 0);
  if (translateCausedException) {
    printf ("*****  WARNING: A PAGE_INVALID_EXCEPTION, ADDRESS_EXCEPTION, or ALIGNMENT_EXCEPTION will occur during the next instruction fetch  *****\n");
    return;
  }
  printf ("The next instruction to execute will be:\n");
  disassemble (physAddr);
}



/* commandStepN ()
**
** Begin BLITZ execution, by repeatedly calling singleStep().
*/
void commandStepN () {
  int count;
  printf ("Enter the number of instructions to execute: ");
  count = readDecimalInt ();
  commandGo (count);
}



/* commandGo (count)
**
** Execute count BLITZ instructions, by calling singleStep() that
** many times.  Count may be MAX, which will achieve infinite execution.
*/
void commandGo (int count) {
  int i;
  printf ("Beginning execution...\n");
  wantPrintingInSingleStep = 0;
  executionHalted = 0;
  turnOnTerminal ();
  for (i=0; i<count; i++) {
    if (executionHalted) {
      break;
    }
    if (controlCPressed) {
      controlCPressed = 0;
      break;
    }
    singleStep ();
  }
  turnOffTerminal ();
  printf ("Done!  ");
  printAboutToExecute ();
}



/* commandStepHigh ()
**
** Execute several instructions, by calling singleStep(), until we detect that
** r10 has changed.  This signals the beginning of the next high-level
** instruction.
*/
void commandStepHigh () {
  int oldR10;
  // printf ("STEP HIGH...\n");
  wantPrintingInSingleStep = 1;
  executionHalted = 0;
  turnOnTerminal ();
  if (statusS) {
    oldR10 = systemRegisters [10];
  } else {
    oldR10 = userRegisters [10];
  }
  while (1) {
    if (executionHalted) {
      break;
    }
    if (controlCPressed) {
      controlCPressed = 0;
      break;
    }

    singleStep ();
    if (statusS) {
      if (oldR10 != systemRegisters [10]) break;
    } else {
      if (oldR10 != userRegisters [10]) break;
    }
  }
  turnOffTerminal ();
  printKPLStmtCode ();
}



/* commandStepHigh2 ()
**
** Execute several instructions, by calling singleStep(), until we detect that
** r10 has been set to "FU", "ME", or "RE".  This indicates that we are
** entering or leaving some KPL function/method.
*/
void commandStepHigh2 () {
  int newR10, oldR10;
  // printf ("STEP HIGH 2...\n");
  wantPrintingInSingleStep = 1;
  executionHalted = 0;
  turnOnTerminal ();
  while (1) {
    if (executionHalted) {
      break;
    }
    if (controlCPressed) {
      controlCPressed = 0;
      break;
    }

    if (statusS) {
      oldR10 = systemRegisters [10];
    } else {
      oldR10 = userRegisters [10];
    }
    singleStep ();
    if (statusS) {
      newR10 = systemRegisters [10];
    } else {
      newR10 = userRegisters [10];
    }
    if ((newR10 != oldR10) &&
        (newR10 == STMT_CODE_FU || newR10 == STMT_CODE_ME || newR10 == STMT_CODE_RE)) {
      break;
    }
  }
  turnOffTerminal ();
  printKPLStmtCode ();
}



/* commandStep ()
**
** Execute a single BLITZ instruction and print the next instruction to
** be executed.
*/
void commandStep () {

  wantPrintingInSingleStep = 1;
  turnOnTerminal ();
  singleStep ();
  turnOffTerminal ();

  /* Print the next instruction. */
  printf ("Done!  ");
  printAboutToExecute ();

  /*****
  if (getNextInterrupt ()) {
    printf ("Done!\n");
    // Don't print next instruction, since it will be pre-emted.
  } else {
    printf ("Done!  ");
    printAboutToExecute ();
  }
  *****/
}



/* singleStep ()
**
** This routine will execute the next BLITZ instruction and return.
*/
void singleStep () {
  int instr, opcode, x, y, z, thisInterrupt, sp, oldStatusReg, i, overflow;
  double d1, d2, d3;
  int physAddr, physAddr2, word, saveP, regNumber, regA, regC;
  int * p;

  // printf ("_");

  /* Increment the current time by 1 cycle.  This will overflow after
     perhaps 5 to 45 minutes of CPU-bound execution time, depending on the
     host speed. */
  if (++currentTime >= MAX) {
    fatalError ("Implementation Restriction: Current time overflow");
  }

  /* Check to see if we need to process an event. */
  i = 0;
  while (currentTime >= timeOfNextEvent) {
    if (currentTime >= timeOfNextTimerEvent) {
      // printf ("t");
      doTimerEvent ();
    }
    if (currentTime >= timeOfNextDiskEvent) {
      doDiskEvent ();
    }
    if (currentTime >= timeOfNextSerialInEvent) {
      // printf ("i");
      doSerialInEvent (0);
    }
    if (currentTime >= timeOfNextSerialOutEvent) {
      // printf ("o");
      doSerialOutEvent ();
    }
    updateTimeOfNextEvent ();
    if (++i > 100) {
      fatalError ("PROGRAM LOGIC ERROR: Too many events");
    }
  }


  /* Check for and process any and all interrupts that have been signaled.
     Each iteration of this loop looks at the next interrupt.  The call
     to getNextInterrupt will ignore maskable interrupts if interrupts
     are currently disabled (i.e., if statusI=0).  */
  if (thisInterrupt = getNextInterrupt ()) {

    // printf (".");

    if (wantPrintingInSingleStep) {
      if (interruptsSignaled & POWER_ON_RESET)
        printf ("*****  POWER_ON_RESET interrupt  *****\n");
      if (interruptsSignaled & TIMER_INTERRUPT)
        printf ("*****  TIMER_INTERRUPT interrupt  *****\n");
      if (interruptsSignaled & DISK_INTERRUPT)
        printf ("*****  DISK_INTERRUPT interrupt  *****\n");
      if (interruptsSignaled & SERIAL_INTERRUPT)
        printf ("*****  SERIAL_INTERRUPT interrupt  *****\n");
      if (interruptsSignaled & HARDWARE_FAULT)
        printf ("*****  HARDWARE_FAULT interrupt  *****\n");
      if (interruptsSignaled & ILLEGAL_INSTRUCTION)
        printf ("*****  ILLEGAL_INSTRUCTION interrupt  *****\n");
      if (interruptsSignaled & ARITHMETIC_EXCEPTION)
        printf ("*****  ARITHMETIC_EXCEPTION interrupt  *****\n");
      if (interruptsSignaled & ADDRESS_EXCEPTION)
        printf ("*****  ADDRESS_EXCEPTION interrupt  *****\n");
      if (interruptsSignaled & PAGE_INVALID_EXCEPTION)
        printf ("*****  PAGE_INVALID_EXCEPTION interrupt  *****\n");
      if (interruptsSignaled & PAGE_READONLY_EXCEPTION)
        printf ("*****  PAGE_READONLY_EXCEPTION interrupt  *****\n");
      if (interruptsSignaled & PRIVILEGED_INSTRUCTION)
        printf ("*****  PRIVILEGED_INSTRUCTION interrupt  *****\n");
      if (interruptsSignaled & ALIGNMENT_EXCEPTION)
        printf ("*****  ALIGNMENT_EXCEPTION interrupt  *****\n");
      if (interruptsSignaled & EXCEPTION_DURING_INTERRUPT)
        printf ("*****  EXCEPTION_DURING_INTERRUPT interrupt  *****\n");
      if (interruptsSignaled & SYSCALL_TRAP)
        printf ("*****  SYSCALL_TRAP interrupt  *****\n");
      printf ("Processing an interrupt and jumping into interrupt vector.\n");
    }

    /* Cancel this interrupt. */
    interruptsSignaled &= ~thisInterrupt;

    /* Save the old Status Register and change to System Mode, Paging
       Disabled, and Interrupts Disabled. */
    oldStatusReg = buildStatusWord ();
    statusI = 0;   
    statusS = 1;   
    statusP = 0;   

    /* For POWER_ON_RESET and HARDWARE_FAULT, cancel all other interrupts. */
    if ((thisInterrupt == POWER_ON_RESET) ||
        (thisInterrupt == HARDWARE_FAULT)) {
      interruptsSignaled = 0;
      pageInvalidOffendingAddress = 0;
      pageReadonlyOffendingAddress = 0;
      systemTrapNumber = 0;

    /* For all other interrupts, push exception info onto the stack. */
    } else {

      /* Check for any problems that might arise with the stack. */
      sp = systemRegisters [15];
      if (!physicalAddressOk (sp-4) || !physicalAddressOk (sp-12)
               || inMemoryMappedArea (sp-4) || inMemoryMappedArea (sp-12)
               || !isAligned (sp)) {

        /* Turn this into an EXCEPTION_DURING_INTERRUPT and cancel all
           other pending interrupts. */
        if (wantPrintingInSingleStep) {
          printf ("*****  An EXCEPTION_DURING_INTERRUPT interrupt has occurred  *****\n");
        }
        thisInterrupt = EXCEPTION_DURING_INTERRUPT;
        interruptsSignaled = 0;
  
      } else {
  
        /* Push the Exception info word onto the stack. */
        if (thisInterrupt == SYSCALL_TRAP) {
          putPhysicalWord (sp-4, systemTrapNumber);
          systemTrapNumber = 0;
        } else if (thisInterrupt == PAGE_INVALID_EXCEPTION) {
          putPhysicalWord (sp-4, pageInvalidOffendingAddress);
          pageInvalidOffendingAddress = 0;
        } else if (thisInterrupt == PAGE_READONLY_EXCEPTION) {
          putPhysicalWord (sp-4, pageReadonlyOffendingAddress);
          pageReadonlyOffendingAddress = 0;
        } else {
          putPhysicalWord (sp-4, 0);
        }
        /* Push the status register and the PC onto the stack. */
        putPhysicalWord (sp-8, oldStatusReg);
        putPhysicalWord (sp-12, pc);
        systemRegisters [15] = sp-12;
      }
    }

    /* Next, set the pc to the address in the low-memory interrupt vector. */
    pc = getVectorNumber (thisInterrupt);

    /* This is enough work for one machine cycle. */
    return;

  }

  /* Fetch the next instruction. */
  /*   Call translate with reading=TRUE, wantPrinting=FALSE, doUpdates=TRUE */
  i = translate (pc, 1, 0, 1);
  if (translateCausedException) {
    if (wantPrintingInSingleStep) {
      printf ("An exception has occurred during instruction fetch!\n");
    }
    return;
  }
  instr = getPhysicalWord (i);

  /* Print the instruction. */
  // printf ("Executing this instruction:\n");
  // printAboutToExecute ();

  /* Switch on the op-code. */
  opcode = (instr >> 24) & 0x000000ff;
  switch (opcode) {

    /***  add      Ra,Rb,Rc  ***/
    case 96:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      if ((x<0) && (y<0)) {
        overflow = (z>=0);
      } else if ((x>0) && (y>0)) {
        overflow = (z<=0);
      } else {
        overflow = 0;
      }
      setSR (z, overflow);
      putRc (instr, z);
      pc += 4;
      break;

    /***  add      Ra,data16,Rc  ***/
    case 128:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      if ((x<0) && (y<0)) {
        overflow = (z>=0);
      } else if ((x>0) && (y>0)) {
        overflow = (z<=0);
      } else {
        overflow = 0;
      }
      setSR (z, overflow);
      putRc (instr, z);
      pc += 4;
      break;

    /***  sub      Ra,Rb,Rc  ***/
    case 97:
      x = getRa (instr);
      y = getRb (instr);
      z = x - y;
      if ((x>=0) && (y<0)) {
        overflow = (z<=0);
      } else if ((x<0) && (y>0)) {
        overflow = (z>=0);
      } else {
        overflow = 0;
      }
      setSR (z, overflow);
      putRc (instr, z);
      pc += 4;
      break;

    /***  sub      Ra,data16,Rc  ***/
    case 129:
      x = getRa (instr);
      y = getData16 (instr);
      z = x - y;
      if ((x>=0) && (y<0)) {
        overflow = (z<=0);
      } else if ((x<0) && (y>0)) {
        overflow = (z>=0);
      } else {
        overflow = 0;
      }
      setSR (z, overflow);
      putRc (instr, z);
      pc += 4;
      break;

    /***  mul      Ra,Rb,Rc  ***/
    case 98:
      x = getRa (instr);
      y = getRb (instr);
      d1 = ((double) x) * ((double) y);
      z = x * y;
      if (d1 != (double) z) {
        setSR (z, 1);
      } else {
        setSR (z, 0);
      }
      putRc (instr, z);
      pc += 4;
      break;

    /***  mul      Ra,data16,Rc  ***/
    case 130:
      x = getRa (instr);
      y = getData16 (instr);
      d1 = ((double) x) * ((double) y);
      z = x * y;
      if (d1 != (double) z) {
        setSR (z, 1);
      } else {
        setSR (z, 0);
      }
      putRc (instr, z);
      pc += 4;
      break;

    /***  div      Ra,Rb,Rc  ***/
    case 99:
      x = getRa (instr);
      y = getRb (instr);
      if (y == 0) {
        interruptsSignaled |= ARITHMETIC_EXCEPTION;
        break;
      }
      if ((x == MIN) && (y == -1)) {
        setSR (MIN, 1);
        putRc (instr, MIN);
        pc += 4;
        break;
      }
      divide (x, y);
      setSR (q, 0);
      putRc (instr, q);
      pc += 4;
      break;

    /***  div      Ra,data16,Rc  ***/
    case 131:
      x = getRa (instr);
      y = getData16 (instr);
      if (y == 0) {
        interruptsSignaled |= ARITHMETIC_EXCEPTION;
        break;
      }
      if ((x == MIN) && (y == -1)) {
        setSR (MIN, 1);
        putRc (instr, MIN);
        pc += 4;
        break;
      }
      divide (x, y);
      setSR (q, 0);
      putRc (instr, q);
      pc += 4;
      break;

    /***  sll      Ra,Rb,Rc  ***/
    case 100:
      x = getRa (instr);
      y = getRb (instr);
      y &= 0x0000001f;   /* Shift value must be 0..31 */
      z = x << y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  sll      Ra,data16,Rc  ***/
    case 132:
      x = getRa (instr);
      y = getData16 (instr);
      y &= 0x0000001f;   /* Shift value must be 0..31 */
      z = x << y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  srl      Ra,Rb,Rc  ***/
    case 101:
      x = getRa (instr);
      y = getRb (instr);
      y &= 0x0000001f;   /* Shift value must be 0..31 */
      z = (signed int) (((unsigned int) x) >> y);
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  srl      Ra,data16,Rc  ***/
    case 133:
      x = getRa (instr);
      y = getData16 (instr);
      y &= 0x0000001f;   /* Shift value must be 0..31 */
      z = (signed int) (((unsigned int) x) >> y);
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  sra      Ra,Rb,Rc  ***/
    case 102:
      x = getRa (instr);
      y = getRb (instr);
      y &= 0x0000001f;   /* Shift value must be 0..31 */
      z = x >> y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  sra      Ra,data16,Rc  ***/
    case 134:
      x = getRa (instr);
      y = getData16 (instr);
      y &= 0x0000001f;   /* Shift value must be 0..31 */
      z = x >> y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  or       Ra,Rb,Rc  ***/
    case 103:
      x = getRa (instr);
      y = getRb (instr);
      z = x | y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  or       Ra,data16,Rc  ***/
    case 135:
      x = getRa (instr);
      y = getData16 (instr);
      z = x | y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  and      Ra,Rb,Rc  ***/
    case 104:
      x = getRa (instr);
      y = getRb (instr);
      z = x & y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  and      Ra,data16,Rc  ***/
    case 136:
      x = getRa (instr);
      y = getData16 (instr);
      z = x & y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  andn     Ra,Rb,Rc  ***/
    case 105:
      x = getRa (instr);
      y = getRb (instr);
      z = x & ~y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  andn     Ra,data16,Rc  ***/
    case 137:
      x = getRa (instr);
      y = getData16 (instr);
      z = x & ~y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  xor      Ra,Rb,Rc  ***/
    case 106:
      x = getRa (instr);
      y = getRb (instr);
      z = x ^ y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  xor      Ra,data16,Rc  ***/
    case 138:
      x = getRa (instr);
      y = getData16 (instr);
      z = x ^ y;
      setSR (z, 0);
      putRc (instr, z);
      pc += 4;
      break;

    /***  rem      Ra,Rb,Rc  ***/
    case 115:
      x = getRa (instr);
      y = getRb (instr);
      if (y == 0) {
        interruptsSignaled |= ARITHMETIC_EXCEPTION;
        break;
      }
      if ((x == MIN) && (y == -1)) {
        setSR (0, 1);
        putRc (instr, 0);
        pc += 4;
        break;
      }
      divide (x, y);
      setSR (r, 0);
      putRc (instr, r);
      pc += 4;
      break;

    /***  rem      Ra,data16,Rc  ***/
    case 149:
      x = getRa (instr);
      y = getData16 (instr);
      if (y == 0) {
        interruptsSignaled |= ARITHMETIC_EXCEPTION;
        break;
      }
      if ((x == MIN) && (y == -1)) {
        setSR (0, 1);
        putRc (instr, 0);
        pc += 4;
        break;
      }
      divide (x, y);
      setSR (r, 0);
      putRc (instr, r);
      pc += 4;
      break;

    /***  load     [Ra+Rb],Rc  ***/
    case 107:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      putRc (instr, getPhysicalWord (physAddr));
      pc += 4;
      break;

    /***  load     [Ra+data16],Rc  ***/
    case 139:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      putRc (instr, getPhysicalWord (physAddr));
      pc += 4;
      break;

    /***  loadb    [Ra+Rb],Rc  ***/
    case 108:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z&0xfffffffc, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWord (physAddr);
      /* Isolate the byte in the word and shift to lower-order 8 bits. */
      word = word >> (24 - ((z & 0x00000003) << 3));
      word &= 0x000000ff;
      putRc (instr, word);
      pc += 4;
      break;

    /***  loadb    [Ra+data16],Rc  ***/
    case 140:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z&0xfffffffc, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWord (physAddr);
      /* Isolate the byte in the word and shift to lower-order 8 bits. */
      word = word >> (24 - ((z & 0x00000003) << 3));
      word &= 0x000000ff;
      putRc (instr, word);
      pc += 4;
      break;

    /***  loadv    [Ra+Rb],Rc  ***/
    case 109:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z, 1, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      putRc (instr, getPhysicalWord (physAddr));
      pc += 4;
      break;

    /***  loadv    [Ra+data16],Rc  ***/
    case 141:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z, 1, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      putRc (instr, getPhysicalWord (physAddr));
      pc += 4;
      break;

    /***  loadbv   [Ra+Rb],Rc  ***/
    case 110:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z&0xfffffffc, 1, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWord (physAddr);
      /* Isolate the byte in the word and shift to lower-order 8 bits. */
      word = word >> (24 - ((z & 0x00000003) << 3));
      word &= 0x000000ff;
      putRc (instr, word);
      pc += 4;
      break;

    /***  loadbv   [Ra+data16],Rc  ***/
    case 142:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z&0xfffffffc, 1, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWord (physAddr);
      /* Isolate the byte in the word and shift to lower-order 8 bits. */
      word = word >> (24 - ((z & 0x00000003) << 3));
      word &= 0x000000ff;
      putRc (instr, word);
      pc += 4;
      break;

    /***  store    Rc,[Ra+Rb]  ***/
    case 111:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      putPhysicalWord (physAddr, getRc (instr));
      pc += 4;
      break;

    /***  store    Rc,[Ra+data16]  ***/
    case 143:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      putPhysicalWord (physAddr, getRc (instr));
      pc += 4;
      break;

    /***  storeb   Rc,[Ra+Rb]  ***/
    case 112:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z&0xfffffffc, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWordAndLock (physAddr);
      /* Get the low-order byte from register C; then shift it to proper
         place in the word; then "or" it into the word. */
      i = getRc (instr);
      if (z%4 == 0) {
        word = (word & 0x00ffffff) | ((i & 0x000000ff) << 24);
      } else if (z%4 == 1) {
        word = (word & 0xff00ffff) | ((i & 0x000000ff) << 16);
      } else if (z%4 == 2) {
        word = (word & 0xffff00ff) | ((i & 0x000000ff) << 8);
      } else if (z%4 == 3) {
        word = (word & 0xffffff00) | (i & 0x000000ff);
      }
      putPhysicalWordAndRelease (physAddr, word);
      pc += 4;
      break;

    /***  storeb   Rc,[Ra+data16]  ***/
    case 144:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z&0xfffffffc, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWordAndLock (physAddr);
      /* Get the low-order byte from register C; then shift it to proper
         place in the word; then "or" it into the word. */
      i = getRc (instr);
      if (z%4 == 0) {
        word = (word & 0x00ffffff) | ((i & 0x000000ff) << 24);
      } else if (z%4 == 1) {
        word = (word & 0xff00ffff) | ((i & 0x000000ff) << 16);
      } else if (z%4 == 2) {
        word = (word & 0xffff00ff) | ((i & 0x000000ff) << 8);
      } else if (z%4 == 3) {
        word = (word & 0xffffff00) | (i & 0x000000ff);
      }
      putPhysicalWordAndRelease (physAddr, word);
      pc += 4;
      break;

    /***  storev   Rc,[Ra+Rb]  ***/
    case 113:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z, 0, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      putPhysicalWord (physAddr, getRc (instr));
      pc += 4;
      pc += 4;
      break;

    /***  storev   Rc,[Ra+data16]  ***/
    case 145:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z, 0, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      putPhysicalWord (physAddr, getRc (instr));
      pc += 4;
      break;

    /***  storebv  Rc,[Ra+Rb]  ***/
    case 114:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z&0xfffffffc, 0, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWordAndLock (physAddr);
      /* Get the low-order byte from register C; then shift it to proper
         place in the word; then "or" it into the word. */
      i = getRc (instr);
      if (z%4 == 0) {
        word = (word & 0x00ffffff) | ((i & 0x000000ff) << 24);
      } else if (z%4 == 1) {
        word = (word & 0xff00ffff) | ((i & 0x000000ff) << 16);
      } else if (z%4 == 2) {
        word = (word & 0xffff00ff) | ((i & 0x000000ff) << 8);
      } else if (z%4 == 3) {
        word = (word & 0xffffff00) | (i & 0x000000ff);
      }
      putPhysicalWordAndRelease (physAddr, word);
      pc += 4;
      break;

    /***  storebv  Rc,[Ra+data16]  ***/
    case 146:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      saveP = statusP;
      statusP = 1;
      physAddr = translate (z&0xfffffffc, 0, 0, 1);
      statusP = saveP;
      if (translateCausedException) {
        break;
      }
      word = getPhysicalWordAndLock (physAddr);
      /* Get the low-order byte from register C; then shift it to proper
         place in the word; then "or" it into the word. */
      i = getRc (instr);
      if (z%4 == 0) {
        word = (word & 0x00ffffff) | ((i & 0x000000ff) << 24);
      } else if (z%4 == 1) {
        word = (word & 0xff00ffff) | ((i & 0x000000ff) << 16);
      } else if (z%4 == 2) {
        word = (word & 0xffff00ff) | ((i & 0x000000ff) << 8);
      } else if (z%4 == 3) {
        word = (word & 0xffffff00) | (i & 0x000000ff);
      }
      putPhysicalWordAndRelease (physAddr, word);
      pc += 4;
      break;

    /***  call     Ra+Rc  ***/
    case 64:
      x = getRa (instr);
      y = getRc (instr);
      z = x + y;
      if (z % 4 != 0) {
        interruptsSignaled |= ALIGNMENT_EXCEPTION;
        break;
      }
      pushOntoStack (15, pc+4);
      if (translateCausedException) {
        break;
      }
      pc = z;
      break;

    /***  call     data24  ***/
    case 160:
      z = getData24 (instr);
      if (z % 4 != 0) {
        interruptsSignaled |= ALIGNMENT_EXCEPTION;
        break; 
      }
      pushOntoStack (15, pc+4);
      if (translateCausedException) {
        break;
      }
      pc += z;
      break;

    /***  jmp      Ra+Rc  ***/
    case 65:
      x = getRa (instr);
      y = getRc (instr);
      z = x + y;
      if (z % 4 != 0) {
        interruptsSignaled |= ALIGNMENT_EXCEPTION;
        break;
      }
      pc = z;
      break;

    /***  jmp      data24  ***/
    case 161:
      z = getData24 (instr);
      if (z % 4 != 0) {
        interruptsSignaled |= ALIGNMENT_EXCEPTION;
        break;
      }
      pc += z;
      break;

    /***  be       Ra+Rc  ***/
    case 66:
      jumpIfTrueRaRb (statusZ, instr);
      break;

    /***  be       data24  ***/
    case 162:
      jumpIfTrueData24 (statusZ, instr);
      break;

    /***  bne      Ra+Rc  ***/
    case 67:
      jumpIfTrueRaRb (!statusZ, instr);
      break;

    /***  bne      data24  ***/
    case 163:
      jumpIfTrueData24 (!statusZ, instr);
      break;

    /***  bl       Ra+Rc  ***/
    case 68:
      jumpIfTrueRaRb ((statusN ^ statusV), instr);
      break;

    /***  bl       data24  ***/
    case 164:
      jumpIfTrueData24 ((statusN ^ statusV), instr);
      break;

    /***  ble      Ra+Rc  ***/
    case 69:
      jumpIfTrueRaRb ((statusZ | (statusN ^ statusV)), instr);
      break;

    /***  ble      data24  ***/
    case 165:
      jumpIfTrueData24 ((statusZ | (statusN ^ statusV)), instr);
      break;

    /***  bg       Ra+Rc  ***/
    case 70:
      jumpIfTrueRaRb (!(statusZ | (statusN ^ statusV)), instr);
      break;

    /***  bg       data24  ***/
    case 166:
      jumpIfTrueData24 (!(statusZ | (statusN ^ statusV)), instr);
      break;

    /***  bge      Ra+Rc  ***/
    case 71:
      jumpIfTrueRaRb (!(statusN ^ statusV), instr);
      break;

    /***  bge      data24  ***/
    case 167:
      jumpIfTrueData24 (!(statusN ^ statusV), instr);
      break;

/***
    case 72:
    case 73:
    case 168:
    case 169:
***/

    /***  bvs      Ra+Rc  ***/
    case 74:
      jumpIfTrueRaRb (statusV, instr);
      break;

    /***  bvs      data24  ***/
    case 170:
      jumpIfTrueData24 (statusV, instr);
      break;

    /***  bvc      Ra+Rc  ***/
    case 75:
      jumpIfTrueRaRb (!statusV, instr);
      break;

    /***  bvc      data24  ***/
    case 171:
      jumpIfTrueData24 (!statusV, instr);
      break;

    /***  bns      Ra+Rc  ***/
    case 76:
      jumpIfTrueRaRb (statusN, instr);
      break;

    /***  bns      data24  ***/
    case 172:
      jumpIfTrueData24 (statusN, instr);
      break;

    /***  bnc      Ra+Rc  ***/
    case 77:
      jumpIfTrueRaRb (!statusN, instr);
      break;

    /***  bnc      data24  ***/
    case 173:
      jumpIfTrueData24 (!statusN, instr);
      break;

    /***  bss      Ra+Rc  ***/
    case 78:
      jumpIfTrueRaRb (statusS, instr);
      break;

    /***  bss      data24  ***/
    case 174:
      jumpIfTrueData24 (statusS, instr);
      break;

    /***  bsc      Ra+Rc  ***/
    case 79:
      jumpIfTrueRaRb (!statusS, instr);
      break;

    /***  bsc      data24  ***/
    case 175:
      jumpIfTrueData24 (!statusS, instr);
      break;

    /***  bis      Ra+Rc  ***/
    case 80:
      jumpIfTrueRaRb (statusI, instr);
      break;

    /***  bis      data24  ***/
    case 176:
      jumpIfTrueData24 (statusI, instr);
      break;

    /***  bic      Ra+Rc  ***/
    case 81:
      jumpIfTrueRaRb (!statusI, instr);
      break;

    /***  bic      data24  ***/
    case 177:
      jumpIfTrueData24 (!statusI, instr);
      break;

    /***  bps      Ra+Rc  ***/
    case 82:
      jumpIfTrueRaRb (statusP, instr);
      break;

    /***  bps      data24  ***/
    case 178:
      jumpIfTrueData24 (statusP, instr);
      break;

    /***  bpc      Ra+Rc  ***/
    case 83:
      jumpIfTrueRaRb (!statusP, instr);
      break;

    /***  bpc      data24  ***/
    case 179:
      jumpIfTrueData24 (!statusP, instr);
      break;

    /***  push     Rc,[--Ra]  ***/
    case 84:
      regNumber = (instr >> 16) & 0x0000000f;
      z = getRc (instr);
      pushOntoStack (regNumber, z);
      if (translateCausedException) {
        break;
      }
      pc += 4;
      break;

    /***  pop      [Ra++],Rc  ***/
    case 85:
      regNumber = (instr >> 16) & 0x0000000f;
      z = popFromStack (regNumber);
      if (translateCausedException) {
        break;
      }
      putRc (instr, z);
      pc += 4;
      break;

    /***  sethi    data16,Rc  ***/
    case 192:
      z = getData16 (instr);
      regNumber = (instr >> 20) & 0x0000000f;
      if (regNumber != 0) {
        if (statusS) {
          systemRegisters [regNumber] =
               (systemRegisters [regNumber] & 0x0000ffff) | (z << 16);
        } else {
          userRegisters [regNumber] =
               (userRegisters [regNumber] & 0x0000ffff) | (z << 16);
        }
      }
      pc += 4;
      break;

    /***  setlo    data16,Rc  ***/
    case 193:
      z = getData16 (instr);
      regNumber = (instr >> 20) & 0x0000000f;
      if (regNumber != 0) {
        if (statusS) {
          systemRegisters [regNumber] =
               (systemRegisters [regNumber] & 0xffff0000) | (z & 0x0000ffff);
        } else {
          userRegisters [regNumber] =
               (userRegisters [regNumber] & 0xffff0000) | (z & 0x0000ffff);
        }
      }
      pc += 4;
      break;

    /***  ldaddr   data16,Rc  ***/
    case 194:
      x = getData16 (instr);
      y = pc + x;
      putRc (instr, y);
      pc += 4;
      break;

    /***  syscall  Rc+data16  ***/
    case 195:
      x = getRc (instr);
      y = getData16 (instr);
      interruptsSignaled |= SYSCALL_TRAP;
      systemTrapNumber = x+y;
      pc += 4;
      break;

    /***  nop  ***/
    case 0:
      printf ("WARNING: Executing a NOP instruction at address 0x%08X - Suspending execution!\r\n", pc);
      controlCPressed = 1;
      pc += 4;
      break;

    /***  wait  ***/
    case 1:

      // printf ("h");
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }

      /* Enable interrupts. */
      statusI = 1;

      /* Compute the time of the next event; we will ignore timer events... */
      i = timeOfNextDiskEvent;

      if (timeOfNextSerialOutEvent < i) {
        i = timeOfNextSerialOutEvent;
      }

      /* If there is something in the type-ahead buffer... */
      if (typeAheadBufferCount > 0) {
        doSerialInEvent (1);
        if (timeOfNextSerialInEvent < i) {
          i = timeOfNextSerialInEvent;
        }

      /* Else, if serial input comes from a file... */
      } else if (termInputFile != stdin) {
        /* Then get more input from the file, if any... */
        if (! feof (termInputFile)) {
          doSerialInEvent (1);
          if (timeOfNextSerialInEvent < i) {
            i = timeOfNextSerialInEvent;
          }
        }

      /* Else input comes from stdin... */
      } else {
        /* See if there is a character available on the input... */
        doSerialInEvent (0);
        if (termInCharAvail && timeOfNextSerialInEvent < i) {
          i = timeOfNextSerialInEvent;
        }
      }

      /* If we still haven't found anything to do
         and there are no pending interrupts
         and input is coming from the terminal
         and the user has asked us to wait instead of terminate...  */
      if ((i == MAX) &&
          (getNextInterrupt () == 0) &&
          (termInputFile == stdin) &&
          (commandOptionWait)) {

        /* We are about to wait on user input.  Print a message so the
           user will know we are waiting (unless we're in raw mode)...  */
        if (! terminalWantRawProcessing) {
          fprintf (stderr, "\n\r*****  Execution suspended on 'wait' instruction; waiting for additional user input  *****\n\r");
        }

        /* Wait for some input from stdin... */
        doSerialInEvent (1);
        if (timeOfNextSerialInEvent < i) {
          i = timeOfNextSerialInEvent;
        }
      }

      /* If we now have an interrupt that will be processed on the next cycle... */
      if (getNextInterrupt ()) {
        /* Do nothing thing; continue execution */

      /* Else if we have a disk or serial event in the future... */
      } else if (i != MAX) {

        /* Move currentTime up to the time of the next event. */
        timeSpentAsleep += (timeOfNextEvent-1) - currentTime;
        currentTime = timeOfNextEvent-1;

      /* Else halt execution. */
      } else {
     
        fprintf (stderr, "\n\r*****  A 'wait' instruction was executed and no more interrupts are scheduled... halting emulation!  *****\n\r\n\r");
        executionHalted = 1;
      }

      /* Increment PC; executing go after a wait will resume execution. */
      pc += 4;
      break;

    /***  debug  ***/
    case 2:
      /* Stop instruction emulation and print a message.  Advance the
         PC so that we can easily resume execution. */
      controlCPressed = 1;
      turnOffTerminal ();
      fprintf (stderr, "\n\r****  A 'debug' instruction was encountered  *****\n\r");
      turnOnTerminal ();
      pc += 4;
      break;

    /***  debug2  ***/
    case 10:
      /* Perform a special function.  The function code is in r1. */
      if (statusS) {
        x = systemRegisters [1];
      } else {
        x = userRegisters [1];
      }
      /* Get the value of register r2 (string ptr, integer, or boolean). */
      if (statusS) {
        y = systemRegisters [2];
      } else {
        y = userRegisters [2];
      }

      /* Function = "printInt" */
      if (x == 1) {
        printf ("%d", y);

      /* Function = "printString" */
      } else if (x == 2) {
        /* z = string length */
        if (statusS) {
          z = systemRegisters [3];
        } else {
          z = userRegisters [3];
        }
        if ((z < 0) | (z > 100000)) {
          controlCPressed = 1;
          fprintf (stderr, "\n\r****  An error occurred during a 'debug2' instruction - invalid string length in print  *****\n\r");
          z = -1;
        }
        while (z > 0) {
          z--;
          /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
          physAddr = translate (y&0xfffffffc, 1, 0, 1);
          if (translateCausedException) {
            controlCPressed = 1;
            fprintf (stderr, "\n\r****  An error occurred during a 'debug2' instruction - address exception in print  *****\n\r");
            break;
          }
          word = getPhysicalWord (physAddr);
          /* Isolate the byte in the word and shift to lower-order 8 bits. */
          word = word >> (24 - ((y & 0x00000003) << 3));
          x = word & 0x000000ff;
          if ((x >= 32) && (x < 127)) {
            printf ("%c", x);
          } else if (x == '\t') {
            printf ("\t");
          } else if (x == '\n') {
            printf ("\r\n");
          } else {
            printf ("\\x%02X", x);
          }
          y++;
        }

      /* Function = "printChar" */
      } else if (x == 3) {
        if ((y >= 32) && (y < 127)) {
          printf ("%c", y);
        } else if (y == '\t') {
          printf ("\t");
        } else if (y == '\n') {
          printf ("\r\n");
        } else {
          printf ("\\x%02X", y);
        }

      /* Function = "printDouble" */
      } else if (x == 4) {
        printDoubleVal (floatRegisters[0]);

      /* Function = "printBool" */
      } else if (x == 5) {
        if (y == 0) {
          printf ("FALSE");
        } else if (y == 1) {
          printf ("TRUE");
        } else {
          controlCPressed = 1;
          fprintf (stderr, "\n\r****  An error occurred during a 'debug2' instruction - invalid boolean value in printBool  *****\n\r");
        }

      /* Function = "printHex" */
      } else if (x == 6) {
        printf ("0x");
        putlong (y);

      /* Function = ERROR */
      } else {
        controlCPressed = 1;
        fprintf (stderr, "\n\r****  An error occurred during a 'debug2' instruction - invalid function code  *****\n\r");
      }

      fflush (stdout);
      pc += 4;
      break;

    /***  cleari  ***/
    case 3:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      statusI = 0;
      pc += 4;
      break;

    /***  seti  ***/
    case 4:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      statusI = 1;
      pc += 4;
      break;

    /***  clearp  ***/
    case 5:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      statusP = 0;
      pc += 4;
      break;

    /***  setp  ***/
    case 6:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      statusP = 1;
      pc += 4;
      break;

    /***  clears  ***/
    case 7:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      statusS = 0;
      pc += 4;
      break;

    /***  reti  ***/
    case 8:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = popFromStack (15);
      if (translateCausedException) {
        break;
      }
      y = popFromStack (15);
      if (translateCausedException) {
        break;
      }
      z = popFromStack (15);
      if (translateCausedException) {
        break;
      }
      setStatusFromWord (y);
      pc = x;
      break;

    /***  ret  ***/
    case 9:
      z = popFromStack (15);
      if (translateCausedException) {
        break;
      }
      pc = z;
      break;

    /***  tset     [Ra],Rc  ***/
    case 88:
      x = getRa (instr);
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (x, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      putRc (instr, getPhysicalWordAndLock (physAddr));
      putPhysicalWordAndRelease (physAddr, 0x00000001);
      pc += 4;
      break;

    /***  readu    Rc,Ra  ***/
    case 86:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      regC = (instr >> 20) & 0x0000000f;
      regA = (instr >> 16) & 0x0000000f;
      pc += 4;
      if (regA == 0) break;
      systemRegisters [regA] = userRegisters [regC];
      break;

    /***  readu    Rc,[Ra+data16]  ***/
    case 147:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      regC = (instr >> 20) & 0x0000000f;
      putPhysicalWord (physAddr, userRegisters [regC]);
      pc += 4;
      break;

    /***  writeu   Ra,Rc  ***/
    case 87:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      regC = (instr >> 20) & 0x0000000f;
      regA = (instr >> 16) & 0x0000000f;
      pc += 4;
      if (regC == 0) break;
      userRegisters [regC] = systemRegisters [regA];
      break;

    /***  writeu   [Ra+data16],Rc  ***/
    case 148:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      regC = (instr >> 20) & 0x0000000f;
      pc += 4;
      if (regC == 0) break;
      userRegisters [regC] = getPhysicalWord (physAddr);
      break;

    /***  ldptbr   Rc  ***/
    case 32:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRc (instr);
      if (x % 4 != 0) {
        interruptsSignaled |= ALIGNMENT_EXCEPTION;
        break;
      }
      ptbr = x;
      pc += 4;
      break;

    /***  ldptlr   Rc  ***/
    case 33:
      if (!statusS) {
        interruptsSignaled |= PRIVILEGED_INSTRUCTION;
        break;
      }
      x = getRc (instr);
      if (x % 4 != 0) {
        interruptsSignaled |= ALIGNMENT_EXCEPTION;
        break;
      }
      ptlr = x;
      pc += 4;
      break;

    /***  ftoi      FRa,Rc  ***/
    case 89:
      d1 = getFRa (instr);
      i = d1;
      putRc (instr, i);
      pc += 4;
      break;

    /***  itof      Ra,FRc  ***/
    case 90:
      i = getRa (instr);
      d1 = i;
      putFRc (instr, d1);
      pc += 4;
      break;

    /***  fadd      FRa,FRb,FRc  ***/
    case 116:
      d1 = getFRa (instr);
      d2 = getFRb (instr);
      d3 = d1 + d2;
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fsub      FRa,FRb,FRc  ***/
    case 117:
      d1 = getFRa (instr);
      d2 = getFRb (instr);
      d3 = d1 - d2;
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fmul      FRa,FRb,FRc  ***/
    case 118:
      d1 = getFRa (instr);
      d2 = getFRb (instr);
      d3 = d1 * d2;
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fdiv      FRa,FRb,FRc  ***/
    case 119:
      d1 = getFRa (instr);
      d2 = getFRb (instr);
      d3 = d1 / d2;
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fcmp      FRa,FRc  ***/
    case 91:
      d1 = getFRa (instr);
      d2 = getFRc (instr);
      statusZ = statusN = statusV = 0;
      if (d1 == d2) {
        statusZ = 1;
      }
      if (d1 < d2) {
        statusN = 1;
      }
      if (isnan(d1)) {
        statusV = 1;
      }
      if (isnan(d2)) {
        statusV = 1;
      }
      pc += 4;
      break;

    /***  fsqrt      FRa,FRc  ***/
    case 92:
      d1 = getFRa (instr);
      d3 = sqrt (d1);
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fneg      FRa,FRc  ***/
    case 93:
      d1 = getFRa (instr);
      d3 = -d1;
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fabs      FRa,FRc  ***/
    case 94:
      d1 = getFRa (instr);
      d3 = fabs (d1);
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fload      [Ra+Rb],FRc  ***/
    case 120:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr2 = translate (z+4, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      p = (int *) (& d3);
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
      *p = getPhysicalWord (physAddr2);
      *(p+1) = getPhysicalWord (physAddr);
#else
      *p = getPhysicalWord (physAddr);
      *(p+1) = getPhysicalWord (physAddr2);
#endif
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fload      [Ra+data16],FRc  ***/
    case 150:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      /* Call translate with reading=true, wantPrinting=0, doUpdates=1 */
      physAddr2 = translate (z+4, 1, 0, 1);
      if (translateCausedException) {
        break;
      }
      p = (int *) (& d3);
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
      *p = getPhysicalWord (physAddr2);
      *(p+1) = getPhysicalWord (physAddr);
#else
      *p = getPhysicalWord (physAddr);
      *(p+1) = getPhysicalWord (physAddr2);
#endif
      putFRc (instr, d3);
      pc += 4;
      break;

    /***  fstore      FRc,[Ra+Rb]  ***/
    case 121:
      x = getRa (instr);
      y = getRb (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr2 = translate (z+4, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      p = (int *) (& d3);
      d3 = getFRc (instr);
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
      putPhysicalWord (physAddr2, *p);
      putPhysicalWord (physAddr, *(p+1));
#else
      putPhysicalWord (physAddr, *p);
      putPhysicalWord (physAddr2, *(p+1));
#endif
      pc += 4;
      break;

    /***  fstore      FRc,[Ra+data16]  ***/
    case 151:
      x = getRa (instr);
      y = getData16 (instr);
      z = x + y;
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr = translate (z, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      /* Call translate with reading=false, wantPrinting=0, doUpdates=1 */
      physAddr2 = translate (z+4, 0, 0, 1);
      if (translateCausedException) {
        break;
      }
      p = (int *) (& d3);
      d3 = getFRc (instr);
#ifdef BLITZ_HOST_IS_LITTLE_ENDIAN
      putPhysicalWord (physAddr2, *p);
      putPhysicalWord (physAddr, *(p+1));
#else
      putPhysicalWord (physAddr, *p);
      putPhysicalWord (physAddr2, *(p+1));
#endif
      pc += 4;
      break;

    /***  illegal instruction  ***/
    default:
      interruptsSignaled |= ILLEGAL_INSTRUCTION;
      break;
  }
}



/* getRa (instr)
** getRb (instr)
** getRc (instr)
** putRc (instr, value)
** getData16 (instr)
** getData24 (instr)
**
** These routine get the value of (put a value to) the indicated register.
** The current SystemMode bit will be checked and the data will be
** moved from/to either the System or User registers as appropriate.
** The "getData16" and "getData24" routines return the literal values
** from the instruction after sign-extending the value to 32 bits.
*/
int getRa (int instr) {
  int reg = (instr >> 16) & 0x0000000f;
  if (statusS) {
    return systemRegisters [reg];
  } else {
    return userRegisters [reg];
  }
}

int getRb (int instr) {
  int reg = (instr >> 12) & 0x0000000f;
  if (statusS) {
    return systemRegisters [reg];
  } else {
    return userRegisters [reg];
  }
}

int getRc (int instr) {
  int reg = (instr >> 20) & 0x0000000f;
  if (statusS) {
    return systemRegisters [reg];
  } else {
    return userRegisters [reg];
  }
}

void putRc (int instr, int value) {
  int reg = (instr >> 20) & 0x0000000f;
  if (reg == 0) return;
  if (statusS) {
    systemRegisters [reg] = value;
  } else {
    userRegisters [reg] = value;
  }
}

int getData16 (int instr) {
  int i = instr << 16;
  if (i < 0) {
    return (instr | 0xffff0000);
  } else {
    return (instr & 0x0000ffff);
  }
}

int getData24 (int instr) {
  int i = instr << 8;
  if (i < 0) {
    return (instr | 0xff000000);
  } else {
    return (instr & 0x00ffffff);
  }
}



/* getFRa (instr)
** getFRb (instr)
** getFRc (instr)
** putFRc (instr, value)
**
** These routines get the value of (put a value to) the indicated register.
*/
double getFRa (int instr) {
  int reg = (instr >> 16) & 0x0000000f;
  return floatRegisters [reg];
}

double getFRb (int instr) {
  int reg = (instr >> 12) & 0x0000000f;
  return floatRegisters [reg];
}

double getFRc (int instr) {
  int reg = (instr >> 20) & 0x0000000f;
  return floatRegisters [reg];
}

void putFRc (int instr, double value) {
  int reg = (instr >> 20) & 0x0000000f;
  floatRegisters [reg] = value;
}



/* buildStatusWord ()
**
** This routine builds and returns a 32-bit word reflecting the status
** bits, which are kept in individual variables by this program.
*/
int buildStatusWord () {
  int word = 0;
  if (statusN) {
    word |= 0x00000001;
  }
  if (statusV) {
    word |= 0x00000002;
  }
  if (statusZ) {
    word |= 0x00000004;
  }
  if (statusP) {
    word |= 0x00000008;
  }
  if (statusS) {
    word |= 0x00000010;
  }
  if (statusI) {
    word |= 0x00000020;
  }
  return word;
}



/* setStatusFromWord (word)
**
** This routine is passed a 32-bit word reflecting the status register.
** It sets the individual status variables used by this program to
** record the current status bits.
*/
void setStatusFromWord (int word) {
  statusI = 0;
  statusS = 0;
  statusP = 0;
  statusZ = 0;
  statusV = 0;
  statusN = 0;
  if (word & 0x00000020) {
  }
  if (word & 0x00000020) {
    statusI = 1;
  }
  if (word & 0x00000010) {
    statusS = 1;
  }
  if (word & 0x00000008) {
    statusP = 1;
  }
  if (word & 0x00000004) {
    statusZ = 1;
  }
  if (word & 0x00000002) {
    statusV = 1;
  }
  if (word & 0x00000001) {
    statusN = 1;
  }
  /* ignore all other bits in the word */
}



/* setSR (value, overflow)
**
** This routine sets the Z, V, and N bits of the status word.
** The Zero (Z) and Negative (N) bits will be set according to the
** "value".  The values of the overflow bit will be passed
** in as an argument.
*/
void setSR (int value, int overflow) {
  statusZ = (value == 0);
  statusV = overflow;
  statusN = (value < 0);
}



/* getNextInterrupt ()
**
** This routine returns the type of the interrupt that is being
** signalled.  It looks at the "interruptsSignaled" variable and finds
** an interrupt to return by finding a bit that is set.  This routine
** determines the order in which several simultaneous interrupts are
** processed.  The first one returned will be processed first.  However,
** on the next instruction cycle, the next interrupt will be chosen and
** will be processed.  Thus, the first interrupt will be interrupted on
** its first instruction and will not be serviced until all other
** interrupts are serviced.
**
** Thus, interrupts are actually processed in the reverse of whatever order
** they are returned from this routine.
**
** This routine also checks the "interrupts enabled" bit in the status
** register.  If a signalled bit is maskable but interrupts are disabled,
** this routine just ignores it.
**
** Note that we return the exceptions and SYSCALL trap first, before TIMER,
** DISK, and SERIAL interrupts.  The reason is that these last three exceptions
** may perform a context switch.  It would be disastrous to have an exception
** in one thread, process a TIMER Interrupt, do a context switch, then service
** the exception or SYSCALL trap.  This would attribute the exception or trap
** to the incorrect thread.
*/
int getNextInterrupt () {
  if (interruptsSignaled & POWER_ON_RESET) {
    return POWER_ON_RESET;
  } else if (interruptsSignaled & HARDWARE_FAULT) {
    return HARDWARE_FAULT;
  } else if (interruptsSignaled & ILLEGAL_INSTRUCTION) {
    return ILLEGAL_INSTRUCTION;
  } else if ((interruptsSignaled & ARITHMETIC_EXCEPTION) && statusI) {
    return ARITHMETIC_EXCEPTION;
  } else if (interruptsSignaled & ADDRESS_EXCEPTION) {
    return ADDRESS_EXCEPTION;
  } else if (interruptsSignaled & PAGE_INVALID_EXCEPTION) {
    return PAGE_INVALID_EXCEPTION;
  } else if (interruptsSignaled & PAGE_READONLY_EXCEPTION) {
    return PAGE_READONLY_EXCEPTION;
  } else if (interruptsSignaled & PRIVILEGED_INSTRUCTION) {
    return PRIVILEGED_INSTRUCTION;
  } else if (interruptsSignaled & ALIGNMENT_EXCEPTION) {
    return ALIGNMENT_EXCEPTION;
  } else if (interruptsSignaled & EXCEPTION_DURING_INTERRUPT) {
    return EXCEPTION_DURING_INTERRUPT;
  } else if ((interruptsSignaled & SYSCALL_TRAP) && statusI) {
    return SYSCALL_TRAP;
  } else if ((interruptsSignaled & TIMER_INTERRUPT) && statusI) {
    return TIMER_INTERRUPT;
  } else if ((interruptsSignaled & DISK_INTERRUPT) && statusI) {
    return DISK_INTERRUPT;
  } else if ((interruptsSignaled & SERIAL_INTERRUPT) && statusI) {
    return SERIAL_INTERRUPT;
  } else {
    return 0;
  }
}



/* getVectorNumber (interruptType)
**
** This routine returns the interrupt vector number for the
** given interrupt type.
**
**     address description                 maskable
**     ======= =========================== ========
**      0000   Power On Reset                 No
**      0004   Timer Interrupt               Yes
**      0008   Disk Interrupt                Yes
**      000C   Serial Interrupt              Yes
**      0010   Hardware Fault                 No
**      0014   Illegal Instruction            No
**      0018   Arithmetic Exception          Yes
**      001C   Address Exception              No
**      0020   Page Invalid Exception         No
**      0024   Page Readonly Exception        No
**      0028   Privileged Instruction         No
**      002C   Alignment Exception            No
**      0030   Exception During Interrupt     No
**      0034   Syscall Trap                  Yes
*/
int getVectorNumber (int interruptType) {
  if (interruptType == POWER_ON_RESET) {
    return 0x00000000;
  } else if (interruptType == TIMER_INTERRUPT) {
    return 0x00000004;
  } else if (interruptType == DISK_INTERRUPT) {
    return 0x00000008;
  } else if (interruptType == SERIAL_INTERRUPT) {
    return 0x0000000C;
  } else if (interruptType == HARDWARE_FAULT) {
    return 0x00000010;
  } else if (interruptType == ILLEGAL_INSTRUCTION) {
    return 0x00000014;
  } else if (interruptType == ARITHMETIC_EXCEPTION) {
    return 0x00000018;
  } else if (interruptType == ADDRESS_EXCEPTION) {
    return 0x0000001C;
  } else if (interruptType == PAGE_INVALID_EXCEPTION) {
    return 0x00000020;
  } else if (interruptType == PAGE_READONLY_EXCEPTION) {
    return 0x00000024;
  } else if (interruptType == PRIVILEGED_INSTRUCTION) {
    return 0x00000028;
  } else if (interruptType == ALIGNMENT_EXCEPTION) {
    return 0x0000002C;
  } else if (interruptType == EXCEPTION_DURING_INTERRUPT) {
    return 0x00000030;
  } else if (interruptType == SYSCALL_TRAP) {
    return 0x00000034;
  } else {
    fatalError ("PROGRAM LOGIC ERROR: Unknown exception vector");
  }
}





/**************************************************************************
**
** MEMORY SUB-SYSTEM
**
** These routines deal with access to the physical memory of the BLITZ
** machine.
**
** We make the distinction between "implementation memory", which is the
** memory used by the program you are currently reading (i.e., the memory
** addressed by direct "C" pointers, such as the "memory" variable, and
** "BLITZ memory", which is the emulated memory.  We make a further
** distinction between the "Physical memory", which is the total emulated
** memory of the BLITZ machine, and "Virtual memory" (i.e., "logical
** memory"), which is the memory of BLITZ user-level processes, before the
** addresses have gone through page table translation.
**
** The BLITZ machine is designed to support multi-processing, i.e., to
** support multiple CPUs operating on a single shared physical memory
** unit.  To this end, the CPU may lock portions of memory during critical
** operations.  Examples of critical operations include: (1) the reading and
** subsequent writing of a page table entry, and (2) the reading and
** subsequent writing during the "tset" instruction.
**
** The granularity of memory locks is not specified at this time.
** It may be the case that each word has its own lock, or perhaps each lock
** covers an entire page, or perhaps there is only a single lock covering all
** of the physical memory.
**
** In this implementation, we assume that individual words are locked.
** Furthermore, we assume that, since this is a single processor implementation,
** that only one lock will be held at a time.  Thus, locking is modelled with a
** single variable (called "currentMemoryLock") which contains the address of
** the word that is locked, or -1 if no word is currently locked.
** 
** Locking is somewhat unnecessary to the goal of this emulator, since only
** a single CPU is emulated, yet it is included in order to model a
** multiprocessor implementation more faithfully.  Toward this end, locks are
** set and released at the correct times and we do some consistency checking
** to make sure we don't (for example) try to release a lock that is not held,
** but beyond that, there is no blocking, waiting, or scheduling associated
** with locks.
**
** The operations that a CPU or I/O device may issue to the memory unit are:
**
**   getPhysicalWord (physAddr)  --> int
**   getPhysicalWordAndLock (physAddr)  --> int
**   putPhysicalWord (physAddr, value)
**   putPhysicalWordAndRelease (physAddr, value)
**   releaseMemoryLock (physAddr)
**
** In addition, there are a couple of support routines:
**
**   physicalAddressOk (physAddr)  -->  bool
**   isAligned (addr)  --> bool
**
** All I/O is memory-mapped.  Occasionally, reads and writes to certain
** addresses should be directed not to the memory, but to various devices.
** Routines that support this are:
**
**   inMemoryMappedArea (physAddr)  --> bool
**   ... etc...
**
** The CPU performs page-table translation.  All the details of the
** page table translation are encapsulated into this routine.
**
**   translate (addr, reading, wantPrinting, doUpdates)  --> physAddr
**
** A typical use is to first call translate to produce a physical address,
** then check to see if an exception has occurred, then to either abort if
** we had an exception or to call a routine (such as getPhysicalWord) to
** fetch or store a word from/to memory.
**
**************************************************************************/




/* physicalAddressOk (addr)
**
** This routine is passed a physical memory address.  It returns TRUE
** if this is a legal physical memory address, i.e., if no "Address
** Exception" should be thrown.  The address may be in the memory mapped
** area; this is still considered OK.
*/
int physicalAddressOk (int addr) {
  return ((0 <= addr ) && (addr < MEMORY_SIZE));
}



/* isAligned (addr)
**
** This function returns TRUE if this address is word aligned.
*/
int isAligned (int addr) {
  return addr%4 == 0;
}



/* getPhysicalWord (physAddr)
**
** This routine is passed a physical memory address.  It returns the
** word stored at that address.  The address is assumed to be OK, i.e.,
** both aligned and within physical memory.  Therefore, no exceptions
** are possible.  If the address is in the memory-mapped region, it calls
** the I/O subsystem.
*/
int getPhysicalWord (int physAddr) {
  int implAddr;
  if (!physicalAddressOk (physAddr)) {
    fatalError ("PROGRAM LOGIC ERROR: Invalid address in getPhysicalWord");
  }
  if (physAddr % 4 != 0) {
    fatalError ("PROGRAM LOGIC ERROR: Unaligned address in getPhysicalWord");
  }
  checkDiskBufferError (physAddr);
  if (inMemoryMappedArea (physAddr)) {
    return getMemoryMappedWord (physAddr);
  } else {
    implAddr = ((int) memory) + physAddr;
    return SWAP_BYTES (* (int *) implAddr);
  }
}



/* getPhysicalWordAndLock (physAddr)
**
** Acquire a lock on this word and then get the word from memory.
*/
int getPhysicalWordAndLock (int physAddr) {
  if (currentMemoryLock != -1) {
    fatalError ("PROGRAM LOGIC ERROR: In getPhysicalWordAndLock - lock is not clear");
  }
  currentMemoryLock = physAddr;
  return getPhysicalWord (physAddr);
}



/* putPhysicalWord (physAddr, value)
**
** This routine is passed a physical memory address.  It stores the word
** "value" at that address.  The address is assumed to be OK, i.e.,
** both aligned and within physical memory.  Therefore, no exceptions
** are possible.  If the address is in the memory-mapped region, it calls
** the I/O subsystem.
*/
void putPhysicalWord (int physAddr, int value) {
  int implAddr;
  if (!physicalAddressOk (physAddr)) {
    fatalError ("PROGRAM LOGIC ERROR: Invalid address in putPhysicalWord");
  }
  if (physAddr % 4 != 0) {
    fatalError ("PROGRAM LOGIC ERROR: Unaligned address in putPhysicalWord");
  }
  checkDiskBufferError (physAddr);
  if (inMemoryMappedArea (physAddr)) {
    putMemoryMappedWord (physAddr, value);
  } else {
    implAddr = ((int) memory) + physAddr;
    * (int *) implAddr = SWAP_BYTES (value);
  }
}



/* putPhysicalWordAndRelease (physAddr, value)
**
** This routine writes a word to memory and then releases the lock.
*/
void putPhysicalWordAndRelease (int physAddr, int value) {
  if (currentMemoryLock != physAddr) {
    fatalError ("PROGRAM LOGIC ERROR: In putPhysicalWordAndRelease - lock is not held for this address");
  }
  currentMemoryLock = -1;
  putPhysicalWord (physAddr, value);
}



/* releaseMemoryLock (physAddr)
**
** This routine releases the lock on this memory location.
*/
void releaseMemoryLock (int physAddr) {
  if (currentMemoryLock != physAddr) {
    fatalError ("PROGRAM LOGIC ERROR: In releaseMemoryLock - Lock not held on this address");
  }
  currentMemoryLock = -1;
}



/* inMemoryMappedArea (addr)
**
** This routine is passed a physical address.  It returns true if
** this address is the range of memory-mapped bytes.
*/
int inMemoryMappedArea (int addr) {
  return (addr >= MEMORY_MAPPED_AREA_LOW) && (addr <= MEMORY_MAPPED_AREA_HIGH);
}



/* translate (virtAddr, reading, wantPrinting, doUpdates) --> physAddr
**
** This routine is passed a logical address "virtAddr", which should be
** word-aligned.
**
** If paging is turned on, it will go through the page tables, compute the
** physical address and return it.  If paging is off, it will return the
** address, as is.  This routine will check for all exceptions that would
** arise if memory were accessed using this address.  This routine will not
** actually read or write the target memory word.
**
** If "reading" is true, it indicates that the coming memory operation
** will be a read, otherwise we intend to write to the address.  This is
** needed since the page may be marked read-only.
**
** If "wantPrinting" is TRUE, this routine will print out a trace of
** how this logical address is processed, and which exception would
** be signalled, if any.
**
** If "doUpdates" is TRUE, this routine will update the page table
** entry and will signal an interrupt (if one should occur).  If "doUpdates"
** is FALSE, the page table entry will be unchanged and no exception will
** be signalled.
**
** This routine has the side-effect of setting the variable
** "translateCausedException" to TRUE iff an exception should be
** generated by this translation.  This variable is set regardless of
** "doUpdates".
*/
int translate (int virtAddr, int reading, int wantPrinting, int doUpdates) {
  int tableIndex, tableEntryAddr, tableEntry, frameNumber, offset, physAddr;

  translateCausedException = 0;
  if (wantPrinting) {
    printf ("*****  PAGE TRANSLATION BEGINNING  *****\n");
    printf ("   Logical address             = 0x%08X\n", virtAddr);
  }

  /* Check for an alignment exception.  If found, signal it and return. */
  if (!isAligned (virtAddr)) {
    if (wantPrinting) {
      printf ("   This address is not divisible by 4: ALIGNMENT_EXCEPTION\n");
    }
    translateCausedException = 1;
    if (doUpdates) {
      interruptsSignaled |= ALIGNMENT_EXCEPTION;
    }
    return 0;
  }

  /* If paging is turned off... */
  if (!statusP) {
    if (wantPrinting) {
      printf ("   Status[P] = 0, Paging is turned off\n");
      printf ("   Physical address            = 0x%08X\n", virtAddr);
    }
    /* Check for an address exception.  If found, signal it and return. */
    if (!physicalAddressOk (virtAddr)) {
      if (wantPrinting) {
        printf ("   Bad physical address: ADDRESS_EXCEPTION\n");
      }
      translateCausedException = 1;
      if (doUpdates) {
        interruptsSignaled |= ADDRESS_EXCEPTION;
      }
      return 0;
    }
    /* Return the virtual address as the physical address. */
    return virtAddr;

  /* If paging is turned on... */
  } else {
    tableIndex = virtAddr & 0x00ffe000;
    offset = virtAddr & 0x00001fff;
    if (wantPrinting) {
      printf ("     Page Number               = 0x%08X\n", tableIndex);
      printf ("     Offset into page          = 0x%08X\n", offset);
      printf ("   Status[P] = 1, Paging is turned on\n");
    }

    /* Check for an address exception.  If found, signal it and return. */
    if (virtAddr & 0xff000000) {
      if (wantPrinting) {
        printf ("   Logical address out of range: ADDRESS_EXCEPTION\n");
      }
      translateCausedException = 1;
      if (doUpdates) {
        interruptsSignaled |= ADDRESS_EXCEPTION;
      }
      return 0;
    }

    /* Determine which page table entry we will be accessing. */
    tableIndex = tableIndex >> 11;
    tableEntryAddr = ptbr + tableIndex;
    if (wantPrinting) {
      printf ("   Page Table:\n");
      printf ("            base               = 0x%08X\n", ptbr);
      printf ("            length             = 0x%08X\n", ptlr);
      printf ("            addr of last entry = 0x%08X\n", ptbr + ptlr - 4);
      printf ("   Page number (shifted)       = 0x%08X\n", tableIndex);
      printf ("   Address of page table entry = 0x%08X\n", tableEntryAddr);
    }

    /* Check for a PAGE_INVALID exception.  If found, signal it and return. */
    if (tableIndex >= ptlr) {
      if (wantPrinting) {
        printf ("   This entry is not within the page table bound: PAGE_INVALID_EXCEPTION\n");
      }
      translateCausedException = 1;
      if (doUpdates) {
        interruptsSignaled |= PAGE_INVALID_EXCEPTION;
        pageInvalidOffendingAddress = virtAddr;
      }
      return 0;
    }

    /* Check for an address exception.  If found, signal it and return. */
    if (!physicalAddressOk (tableEntryAddr)) {
      if (wantPrinting) {
        printf ("   This page table entry is not within physical memory: ADDRESS_EXCEPTION\n");
      }
      translateCausedException = 1;
      if (doUpdates) {
        interruptsSignaled |= ADDRESS_EXCEPTION;
      }
      return 0;
    }

    /* Get the page table entry. */
       /* (tableEntryAddr must be aligned since ptbr is aligned.) */
    tableEntry = getPhysicalWordAndLock (tableEntryAddr);
    frameNumber = tableEntry & 0xffffe000;
    if (wantPrinting) {
      printf ("   Page table entry            = 0x%08X\n", tableEntry);
      printf ("       Frame number = 0x%08X\n", frameNumber);
      if (tableEntry & 0x00000001) {
        printf ("       V=1 (Page is valid)\n");
      } else {
        printf ("       V=0 (Page not valid)\n");
      }
      if (tableEntry & 0x00000002) {
        printf ("       W=1 (Page is writable)\n");
      } else {
        printf ("       W=0 (Page not writable)\n");
      }
      if (tableEntry & 0x00000004) {
        printf ("       R=1 (Page has been referenced)\n");
      } else {
        printf ("       R=0 (Page has not been referenced)\n");
      }
      if (tableEntry & 0x00000008) {
        printf ("       D=1 (Page is dirty)\n");
      } else {
        printf ("       D=0 (Page not dirty)\n");
      }
    }

    /* Check for a PAGE_INVALID exception.  If found, signal it and return. */
    if (!(tableEntry & 0x00000001)) {   /* if V bit is clear... */
      if (wantPrinting) {
        printf ("   Valid bit is clear: PAGE_INVALID_EXCEPTION\n");
      }
      translateCausedException = 1;
      if (doUpdates) {
        interruptsSignaled |= PAGE_INVALID_EXCEPTION;
        pageInvalidOffendingAddress = virtAddr;
      }
      releaseMemoryLock (tableEntryAddr);
      return 0;
    }

    /* If we will be writing to memory...*/
    if (!reading) {

      /* Check for a PAGE_READONLY exception.  If found, signal and return. */
      if (!(tableEntry & 0x00000002)) {   /* if W bit is clear... */
        if (wantPrinting) {
          printf ("   Writable bit is clear: PAGE_READONLY_EXCEPTION\n");
        }
        translateCausedException = 1;
        if (doUpdates) {
          interruptsSignaled |= PAGE_READONLY_EXCEPTION;
          pageReadonlyOffendingAddress = virtAddr;
        }
        releaseMemoryLock (tableEntryAddr);
        return 0;
      }

      /* Set the dirty bit */
      if (wantPrinting) {
        printf ("   Setting the dirty bit\n");
      }
      tableEntry |= 0x00000008;
    }

    /* Set the referenced bit */
    if (wantPrinting) {
      printf ("   Setting the referenced bit\n");
    }
    tableEntry |= 0x00000004;

    /* Compute the physical address. */
    physAddr = frameNumber | offset;
    if (wantPrinting) {
      printf ("   Physical address            = 0x%08X\n", physAddr);
    }

    /* Check for an ADDRESS exception.  If found, signal it and return. */
    if (!physicalAddressOk (physAddr)) {
      if (wantPrinting) {
        printf ("   Bad physical address: ADDRESS_EXCEPTION\n");
      }
      translateCausedException = 1;
      if (doUpdates) {
        interruptsSignaled |= ADDRESS_EXCEPTION;
      }
      releaseMemoryLock (tableEntryAddr);
      return 0;
    }

    /* If we are supposed to update the page table, then update it. */
    if (doUpdates) {
      putPhysicalWordAndRelease (tableEntryAddr, tableEntry);
    } else {
      releaseMemoryLock (tableEntryAddr);
    }
    if (wantPrinting) {
      printf ("   Modified page table entry   = 0x%08X\n", tableEntry);
      if (doUpdates) {
        printf ("     (Page table entry was written back to memory)\n");
      } else {
        printf ("     (Page table entry was NOT modified)\n");
      }
      printf ("   Translation completed with no exceptions\n");
    }

    /* Return the physical address. */
    return physAddr;
  }
}



/* getMemoryMappedWord (physAddr)  ==> int
**
*/
int getMemoryMappedWord (int physAddr) {
  int x;

  /* Terminal status word... */
  if (physAddr == SERIAL_STATUS_WORD_ADDRESS) {
    x = 0;
    if (termInCharAvail) {
      x = x | 0x00000001;
    }
    if (termOutputReady) {
      x = x | 0x00000002;
    }
    return x;

  /* Terminal input buffer... */
  } else if (physAddr == SERIAL_DATA_WORD_ADDRESS) {
    termInCharWasUsed = 1;
    termInCharAvail = 0;
    return termInChar;

  /* Disk status word... */
  } else if (physAddr == DISK_STATUS_WORD_ADDRESS) {
    return currentDiskStatus;

  /* DISK_MEMORY_ADDRESS_REGISTER... */
  } else if (physAddr == DISK_MEMORY_ADDRESS_REGISTER) {
    return diskMemoryAddressRegister;

  /* DISK_SECTOR_NUMBER_REGISTER... */
  } else if (physAddr == DISK_SECTOR_NUMBER_REGISTER) {
    return diskSectorNumberRegister;

  /* DISK_SECTOR_COUNT_REGISTER... */
  } else if (physAddr == DISK_SECTOR_COUNT_REGISTER) {
    return diskSectorCountRegister;

  /* All other words in the memory-mapped I/O region... */
  } else {
    fprintf (stderr, "\n\rERROR: Attempt to access undefined address in memory-mapped area\n\r");
    controlCPressed = 1;
  }
}



/* putMemoryMappedWord (physAddr, value)
**
*/
void putMemoryMappedWord (int physAddr, int value) {
  int x;

  /* Terminal status word... */
  if (physAddr == SERIAL_STATUS_WORD_ADDRESS) {
    fprintf (stderr, "\n\rAttempt to write to the SERIAL_STATUS_WORD in the memory-mapped area\n\r");
    controlCPressed = 1;
    return;

  /* Terminal output buffer... */
  } else if (physAddr == SERIAL_DATA_WORD_ADDRESS) {
    x = value & 0x000000ff;
    if (!termOutputReady) {
      fprintf (stderr, "\n\rERROR: Serial device output overrun; char \"%c\" was lost\n\r",
               x);
      controlCPressed = 1;
      return;
    }
    /***  fprintf (termOutputFile, "OUTPUT >>>%c<<<\n\r", x);  ***/
    fprintf (termOutputFile, "%c", x);
    fflush (termOutputFile);
    termOutputReady = 0;
    timeOfNextSerialOutEvent =
         randomBetween (
            currentTime + TERM_OUT_DELAY,
            currentTime + TERM_OUT_DELAY + TERM_OUT_DELAY_VARIATION);
    if (timeOfNextSerialOutEvent <= currentTime) {
      timeOfNextSerialOutEvent = currentTime + 1;
    }
    updateTimeOfNextEvent ();

  /* DISK_COMMAND_WORD_ADDRESS... */
  } else if (physAddr == DISK_COMMAND_WORD_ADDRESS) {
    if (currentDiskStatus == DISK_BUSY) {
      fprintf (stderr, "\n\rERROR: Attempt to write to DISK_COMMAND_WORD while disk is busy!\n\r");
      controlCPressed = 1;
      return;
    } else {
      performDiskIO (value);
    }

  /* DISK_MEMORY_ADDRESS_REGISTER... */
  } else if (physAddr == DISK_MEMORY_ADDRESS_REGISTER) {
    if (currentDiskStatus == DISK_BUSY) {
      fprintf (stderr, "\n\rERROR: Attempt to write to DISK_MEMORY_ADDRESS_REGISTER while disk is busy!\n\r");
      controlCPressed = 1;
      return;
    } else {
      diskMemoryAddressRegister = value;
    }

  /* DISK_SECTOR_NUMBER_REGISTER... */
  } else if (physAddr == DISK_SECTOR_NUMBER_REGISTER) {
    if (currentDiskStatus == DISK_BUSY) {
      fprintf (stderr, "\n\rERROR: Attempt to write to DISK_SECTOR_NUMBER_REGISTER while disk is busy!\n\r");
      controlCPressed = 1;
      return;
    } else {
      diskSectorNumberRegister = value;
    }

  /* DISK_SECTOR_COUNT_REGISTER... */
  } else if (physAddr == DISK_SECTOR_COUNT_REGISTER) {
    if (currentDiskStatus == DISK_BUSY) {
      fprintf (stderr, "\n\rERROR: Attempt to write to DISK_SECTOR_COUNT_REGISTER while disk is busy!\n\r");
      controlCPressed = 1;
      return;
    } else {
      diskSectorCountRegister = value;
    }

  /* All other words in the memory-mapped I/O region... */
  } else {
    fprintf (stderr, "\n\rERROR: Attempt to access undefined address in memory-mapped area\n\r");
    controlCPressed = 1;
  }
}




/**************************************************************************
**
** END OF MEMORY SUB-SYSTEM
**
**************************************************************************/



/* pushOntoStack (reg, value)
**
** This routine is passed a 32-bit value.  It pushes it onto the stack.
** It is passed the number of the register to use as the stack top
** pointer.  If an exception occurs, it is made pending but no other
** changes are made.
*/
void pushOntoStack (int reg, int value) {
  int top, physAddr;
  if (statusS) {
    top = systemRegisters [reg];
  } else {
    top = userRegisters [reg];
  }
  top -= 4;
  /* Call translate with reading=false, wantPrinting=false, doUpdates=true */
  physAddr = translate (top, 0, 0, 1);
  if (translateCausedException) {
    return;
  }
  putPhysicalWord (physAddr, value);
  if (reg==0) {
    return;
  }
  if (reg==0) {
    return;
  }
  if (statusS) {
    systemRegisters [reg] = top;
  } else {
    userRegisters [reg] = top;
  }
}



/* popFromStack (reg)
**
** This routine pops a word from the stack and returns it.  It is passed
** the number of the register to use as the stack top pointer.  If an
** exception occurs, it is made pending and zero is returned, but no other
** changes are made.
*/
int popFromStack (int reg) {
  int top, physAddr, value;
  if (statusS) {
    top = systemRegisters [reg];
  } else {
    top = userRegisters [reg];
  }
  /* Call translate with reading=true, wantPrinting=false, doUpdates=true */
  physAddr = translate (top, 1, 0, 1);
  if (translateCausedException) {
    return 0;
  }
  value = getPhysicalWord (physAddr);
  if (reg==0) {
    return value;
  }
  top += 4;
  if (statusS) {
    systemRegisters [reg] = top;
  } else {
    userRegisters [reg] = top;
  }
  return value;
}



/* commandShowStack ()
**
** This routine prints out the KPL calling stack.
*/
void commandShowStack () {
  int i = 0;
  printf ("   Function/Method            Frame Addr   Execution at...\n");
  printf ("   ========================   ==========   =====================\n");
  while (1) {
    if (!printFrame (i, 0)) break;
    i++;
    if (i % 50 == 0) {
      printf ("Want more (y/n)? ");
      if (readYesNo () != 1) return;
    }
  }
}



/* commandFrame ()
**
** This routine prints out the current frame in the calling stack in
** the format:
**   foo    testPack.c, line 123
*/
void commandFrame () {
  printf ("=====  Frame number %d (where StackTop = 0)  =====\n",
          currentFrame);
  if (!printFrame (currentFrame, 1)) {
    printf ("Resetting current frame to top of stack!\n");
    currentFrame = 0;
  }
}



/* commandStackUp ()
**
** This routine moves up one frame (toward the stack top) and prints
** the frame.
*/
void commandStackUp () {
  currentFrame--;
  if (currentFrame < 0) {
    printf ("Already at top of stack!\n");
    currentFrame = 0;
  }
  printf ("=====  Frame number %d (where StackTop = 0)  =====\n",
          currentFrame);
  if (!printFrame (currentFrame, 1)) {
    printf ("Resetting current frame to top of stack!\n");
    currentFrame = 0;
  }
}



/* commandStackDown ()
**
** This routine moves down one frame (deeper into the stack) and prints
** the frame.
*/
void commandStackDown () {
  currentFrame++;
  printf ("=====  Frame number %d (where StackTop = 0)  =====\n",
          currentFrame);
  if (!printFrame (currentFrame, 1)) {
    printf ("Resetting current frame to top of stack!\n");
    currentFrame = 0;
  }
}



/* printFrame (frameNumber, longPrint)  -->  bool
**
** This routine prints out frame number "frameNumber" in the calling stack.
** Frame 0 is on the stack top.  If everything is ok, we return TRUE; if
** problems (including end of stack) we return FALSE.
**
** If longPrint is FALSE, we print one line; if TRUE, we print the full frame.
*/
int printFrame (int frameNumber, int longPrint) {
  int r13, fp, sp, physAddr, addrOfRoutineDescriptor,
      filenamePtr, ptrToFunName, lineNum, oldFp, p, i, j, vdAddr, ptrToVarName,
      offset, offset2, code, totalParmSize, frameSize, index, sizeInBytes;
  TableEntry * tableEntry;
  int * ptr;
  double d; 

  // Get "r13" and "r14/fp"...
  if (statusS) {
    r13 = systemRegisters [13];
    fp = systemRegisters [14];
    sp = systemRegisters [15];
  } else {
    r13 = userRegisters [13];
    fp = userRegisters [14];
    sp = userRegisters [15];
  }
  lineNum = r13;
  oldFp = sp;

  while (frameNumber > 0) {
    frameNumber--;

    // Get "lineNum" from this frame...
    physAddr = translate (fp-4, 1, 0, 0);  // reading=1, wantPrinting=0, doUpdates=0
    if (translateCausedException) {
      printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
      return 0;
    }
    lineNum = getPhysicalWord (physAddr);

    // Get new frame pointer "fp" from this frame...
    physAddr = translate (fp, 1, 0, 0);  // reading=1, wantPrinting=0, doUpdates=0
    if (translateCausedException) {
      printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
      return 0;
    }

    oldFp = fp;
    fp = getPhysicalWord (physAddr);

    if (fp != 0 && fp <= oldFp) {
      printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", oldFp);
      return 0;
    }
  }

  if (fp == 0) {
    printf ("Bottom of activation frame stack!\n");
    return 0;
  }

  // printf ("frame pointer = ");
  // printNumberNL (fp);

  // Get ptr to routine descriptor
  physAddr = translate (fp-8, 1, 0, 0);  // reading=1, wantPrinting=0, doUpdates=0
  if (translateCausedException) {
    printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
    return 0;
  }
  addrOfRoutineDescriptor = getPhysicalWord (physAddr);
  // printf ("addrOfRoutineDescriptor = ");
  // printNumberNL (addrOfRoutineDescriptor);

  // Get ptr to filename...
  physAddr = translate (addrOfRoutineDescriptor, 1, 0, 0);
  if (translateCausedException) {
    printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
    return 0;
  }
  filenamePtr = getPhysicalWord (physAddr);
  // printf ("filenamePtr = ");
  // printNumberNL (filenamePtr);

  // Get ptr to function name...
  physAddr = translate (addrOfRoutineDescriptor+4, 1, 0, 0);
  if (translateCausedException) {
    printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
    return 0;
  }
  ptrToFunName = getPhysicalWord (physAddr);
  // printf ("ptrToFunName = ");
  // printNumberNL (ptrToFunName);

  // Get totalParmSize...
  physAddr = translate (addrOfRoutineDescriptor+8, 1, 0, 0);
  if (translateCausedException) {
    printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
    return 0;
  }
  totalParmSize = getPhysicalWord (physAddr);

  // Get frameSize...
  physAddr = translate (addrOfRoutineDescriptor+12, 1, 0, 0);
  if (translateCausedException) {
    printf ("Invalid activation frame stack!  The frame pointer is 0x%08X.\n", fp);
    return 0;
  }
  frameSize = getPhysicalWord (physAddr);

  if (longPrint == 0) {

    // Print the function name...
    printf ("   ");
    if (!printAsciiDataInWidth (ptrToFunName, 27)) return 0;
    printf (" ");

    // Print the frame address...
    putlong (fp);
    printf ("    ");

    // Print the Filename...
    if (!printAsciiDataInWidth (filenamePtr,0)) return 0;

    // Print the line number...
    printf (", line %d\n", lineNum);

    return 1;
  }

  // Perform the long printout of the frame...

  // Print the function name...
  printf ("Function Name:    ");
  printAsciiDataInWidth (ptrToFunName, 0);
  printf ("\n");

  // Print the Filename...
  printf ("Filename:         ");
  printAsciiDataInWidth (filenamePtr, 0);
  printf ("\n");

  // Print the line number...
  printf ("Execution now at: line %d\n", lineNum);

  // Print the frame address...
  printf ("Frame Addr:       ");
  putlong (fp);
  printf ("\n");
  printf ("frameSize:        %d\n", frameSize);
  printf ("totalParmSize:    %d\n", totalParmSize);

  printf ("                         ==========\n");
  if (oldFp != sp) {
    oldFp = oldFp+8;
  }
  for (p = oldFp; p <= fp+totalParmSize+4; p = p+4) {
    if (p == fp) {
      printf ("     fp:");
    } else if (p == fp-4) {
      printf ("    r13:");
    } else if (p == fp-8) {
      printf ("R.D.ptr:");
    } else if (p == fp+4) {
      printf ("RetAddr:");
    } else if (p == fp+8) {
      printf ("   Args:");
    } else if (p == sp) {
      printf ("   sp-->");
    } else {
      printf ("        ");
    }
    printf ("%4d   %08X:  ", p-fp, p);
    physAddr = translate (p, 1, 0, 0);
    if (translateCausedException) {
      printf ("*****  MEMORY EXCEPTION  *****\n");
    } else {
      i = getPhysicalWord (physAddr);
      printf ("%08X\n", i);
    }
    if (p == fp+4) {
      printf ("                         ==========\n");
    }
  }


  printf ("\nPARAMETERS AND LOCAL VARIABLES WITHIN THIS FRAME:\n");
  printf ("=================================================\n");

  // In a loop, print each var descriptor...
  vdAddr = addrOfRoutineDescriptor + 16;
  while (1) {

    // Get ptr to variable name...
    physAddr = translate (vdAddr, 1, 0, 0);
    if (translateCausedException) {
      printf ("Problems in variable descriptor information in memory (addr = 0x%08X - address exception when accessing ptr to var name)!\n",
              vdAddr);
      break;
    }
    ptrToVarName = getPhysicalWord (physAddr);

    // If the pointer is zero, we are done...
    if (ptrToVarName == 0) break;

    // Get offset...
    physAddr = translate (vdAddr+4, 1, 0, 0);
    if (translateCausedException) {
      printf ("Problems in variable descriptor information in memory (addr = 0x%08X - address exception when accessing offset)!\n",
              vdAddr+4);
      break;
    }
    offset = getPhysicalWord (physAddr);

    // Get sizeInBytes...
    physAddr = translate (vdAddr+8, 1, 0, 0);
    if (translateCausedException) {
      printf ("Problems in variable descriptor information in memory (addr = 0x%08X - bad sizeInBytes)!\n",
              vdAddr+8);
      break;
    }
    sizeInBytes = getPhysicalWord (physAddr);

    // Get the type code (e.g., 'D', 'I', etc.)
    physAddr = translate (ptrToVarName, 1, 0, 0);
    if (translateCausedException) {
      printf ("Problems in variable descriptor information in memory (addr = 0x%08X - address exception when accessing type code)!\n",
              ptrToVarName);
      break;
    }
    code = getPhysicalWord (physAddr);
    code = (code >> 24) & 0x000000ff;

    ptrToVarName++;

    // Print the variable name...
    printf ("  ");
    printAsciiDataInWidth (ptrToVarName, 0);
    if (code == 'I') {
      printf (": int\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        printf ("%08X    value = %d\n", i, i);
      }
    } else if (code == '?') {    // This would be for temporaries...
      printf ("\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        printf ("%08X\n", i);
        for (j = 4; j< sizeInBytes; j=j+4) {
          physAddr = translate (fp+offset+j, 1, 0, 0);
          if (translateCausedException) {
            printf ("*****  MEMORY EXCEPTION  *****\n");
          } else {
            i = getPhysicalWord (physAddr);
            printf ("        %4d   %08X:  %08X\n", offset+j, fp+offset+j, i);
          }
        }
      }
    } else if (code == 'D') {
      printf (": double\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        ptr = (int *) (& d);
        *ptr = i;
        ptr++;
        physAddr = translate (fp+offset+4, 1, 0, 0);
        if (translateCausedException) {
          printf ("*****  MEMORY EXCEPTION  *****\n");
        } else {
          j = getPhysicalWord (physAddr);
          *ptr = j;
          printf ("%08X    value = ", i);
          if (isnan(d)) {
            printf ("Not-a-Number\n");
          } else if (d == POSITIVE_INFINITY) {
            printf ("Positive-Infinity\n");
          } else if (d == NEGATIVE_INFINITY) {
            printf ("Negative-Infinity\n");
          } else if (isNegZero (d)) {
            printf ("Negative-Zero\n");
          } else {
            printf ("%.15g\n", d);        // This precision is a little low, to look nice
          }
          printf ("        %4d   %08X:  %08X\n", offset+4, fp+offset+4, j);
        }
      }
    } else if ((code == 'C') || (code == 'B')) {
      if (code == 'C') {
        printf (": char\n");
      } else {
        printf (": bool\n");
      }
      // printf ("offset = 0x%08X  %d\n", offset, offset);
      divide (offset, 4);
      offset2 = q * 4;
      // printf ("offset2 = 0x%08X  %d\n", offset2, offset2);
      printf ("        %4d   %08X:  ", offset, fp+offset2);
      physAddr = translate (fp+offset2, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        if (r == 0) {
          i = (i >> 24) & 0x000000ff;
          printf ("%02X------", i);
        } else if (r == 1) {
          i = (i >> 16) & 0x000000ff;
          printf ("--%02X----", i);
        } else if (r == 2) {
          i = (i >> 8) & 0x000000ff;
          printf ("----%02X--", i);
        } else if (r == 3) {
          i = (i >> 0) & 0x000000ff;
          printf ("------%02X", i);
        }
        if (code == 'C') {
          printf ("    value = '");
          fancyPrintChar (i);
          printf ("'\n");
        } else {
          if (i == 0) {
            printf ("    value = FALSE\n");
          } else if (i == 1) {
            printf ("    value = TRUE\n");
          } else {
            printf ("    value = *****  ERROR IN BOOLEAN VALUE  *****\n");
          }
        }
      }
    } else if (code == 'P') {
      printf (": ptr\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****");
      } else {
        i = getPhysicalWord (physAddr);
        printf ("%08X    --> ", i);
        index = findLabel (i);
        if ((index != -1) && (i != 0)) {
          tableEntry = valueIndex [index];
          printf ("(%s) ", tableEntry->string);
        }
        if (i == 0) {
          printf ("NULL");
        } else {
          divide (i, 4);
          j = q * 4;
          printf ("%08X:  ", j);
          physAddr = translate (j, 1, 0, 0);
          if (translateCausedException) {
            printf ("*****  MEMORY EXCEPTION  *****");
          } else {
            i = getPhysicalWord (physAddr);
            if (r == 0) {
              printf ("%08X", i);
            } else if (r == 1) {
              printf ("--%06X", i & 0x00ffffff);
            } else if (r == 2) {
              printf ("----%04X", i & 0x0000ffff);
            } else {
              printf ("------%02X", i & 0x000000ff);
            }
          }
        }
      }
      printf ("\n");
    } else if (code == 'R') {
      printf (": record\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        printf ("%08X\n", i);
        for (j = 4; j< sizeInBytes; j=j+4) {
          physAddr = translate (fp+offset+j, 1, 0, 0);
          if (translateCausedException) {
            printf ("*****  MEMORY EXCEPTION  *****\n");
          } else {
            i = getPhysicalWord (physAddr);
            printf ("        %4d   %08X:  %08X\n", offset+j, fp+offset+j, i);
          }
        }
      }
    } else if (code == 'O') {
      printf (": object\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        printf ("%08X    (Dispatch Table is at 0x%08X)\n", i, i);
        for (j = 4; j< sizeInBytes; j=j+4) {
          physAddr = translate (fp+offset+j, 1, 0, 0);
          if (translateCausedException) {
            printf ("*****  MEMORY EXCEPTION  *****\n");
          } else {
            i = getPhysicalWord (physAddr);
            printf ("        %4d   %08X:  %08X\n", offset+j, fp+offset+j, i);
          }
        }
      }
    } else if (code == 'A') {
      printf (": array\n");
      printf ("        %4d   %08X:  ", offset, fp+offset);
      physAddr = translate (fp+offset, 1, 0, 0);
      if (translateCausedException) {
        printf ("*****  MEMORY EXCEPTION  *****\n");
      } else {
        i = getPhysicalWord (physAddr);
        printf ("%08X    (Number of elts = %d)\n", i, i);
        for (j = 4; j< sizeInBytes; j=j+4) {
          physAddr = translate (fp+offset+j, 1, 0, 0);
          if (translateCausedException) {
            printf ("*****  MEMORY EXCEPTION  *****\n");
          } else {
            i = getPhysicalWord (physAddr);
            printf ("        %4d   %08X:  %08X\n", offset+j, fp+offset+j, i);
          }
        }
      }
    } else {
      printf ("Problems in variable descriptor information in memory (addr = 0x%08X - address exception when accessing data type code)!\n",
              ptrToVarName);
      break;
    }

    // Increment to next var descriptor...
    vdAddr = vdAddr + 12;
    
  }
  printf ("=================================================\n");

  return 1;

}




/* printAsciiDataInWidth (ptr, width) --> bool
**
** This routine is passed a pointer to a Null-terminated string of characters
** in the BLITZ memory.  It attempts to print it.  The ptr could be anything, so
** this routine checks for all conceivable errors.
**
** If everything seemed ok, then return TRUE.  If problems, return FALSE.
**
** If the string is shorter than 'width' characters, it will be padded to that
** length with blanks.
*/
int printAsciiDataInWidth (int ptr, int width) {
  int physAddr, word, i;

  while (1) {
    physAddr = translate (ptr & 0xfffffffc, 1, 0, 0);
                          // reading=1, wantPrinting=0, doUpdates=0
    if (translateCausedException) {
      printf ("\n*****  Problem with stack frame: A debugging string is in error  *****\n");
      return 0;
    }
    word = getPhysicalWord (physAddr);
    /* Isolate the byte in the word and shift to lower-order 8 bits. */
    word = word >> (24 - ((ptr & 0x00000003) << 3));
    word &= 0x000000ff;
    if (word == 0) {
      for (i = width; i>0; i--) {
        printf (" ");
      }
      return 1;
    }
    if (word >= ' ' && word < 0x7f) {
      printf ("%c", word);
      width--;
    } else {
      printf ("\n*****  Problem with stack frame: A debugging string is in error  *****\n");
      return 0;
    }
    ptr++;
  }
}



/* commandHex ()
**
** This routine asks for a hex number.  It prints it out in hex, decimal,
** and ascii.
*/
void commandHex () {
  int value;
  /* Read in a value. */
  printf ("Enter a value in hex: ");
  value = readHexInt ();

  printHexDecimalAscii (value);
}



/* commandDecimal ()
**
** This routine asks for a decimal number.  It prints it out in hex, decimal,
** and ascii.
*/
void commandDecimal () {
  int value;
  /* Read in a value. */
  printf ("Enter a value in decimal: ");
  value = readDecimalInt ();

  printHexDecimalAscii (value);
}



/* commandAscii ()
**
** This routine asks for a character.  It prints it out in hex, decimal,
** and ascii.
*/
void commandAscii () {
  int value;
  /* Read in a value. */
  printf ("Enter a single character followed by a newline/return: ");
  fflush (stdout);
  fgets (inputBuffer, sizeof(inputBuffer), stdin);
  /* Overwrite the \n with \0 to remove it. */
  inputBuffer [strlen (inputBuffer)-1] = '\0';
  if (strlen (inputBuffer) != 1) {
    printf ("You entered %d characters - Aborted\n", strlen (inputBuffer));
    return;
  }
  printHexDecimalAscii (inputBuffer[0]);
}



/* printHexDecimalAscii (i)
**
** This routine prints a number in the form
**       hex: 0x1234abcd   decimal: 395441741   ascii: "..a."
** followed by a newline.
*/
void printHexDecimalAscii (int i) {
  char str [100];
  char c;
  printf ("     hex: 0x");
  putlong (i);
  printf ("     decimal: %d", i);
  printf ("     ascii: \"");
  c = (i >> 24) & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  c = (i >> 16) & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  c = (i >> 8) & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  c = i & 0x000000ff;
  if ((c>=' ') && (c <= '~')) {
    putchar (c);
  } else {
    putchar ('.');
  }
  printf ("\"\n");
}



/* jumpIfTrueRaRb (cond, instr)
**
** This routine is used in the implementation of the conditional jumping
** instructions.  If the condition is true, it updates pc to effect the
** jump.  Otherwise, it increments the pc.
*/
void jumpIfTrueRaRb (int cond, int instr) {
  int x, y, z;
  if (cond) {
    x = getRa (instr);
    y = getRc (instr);
    z = x + y;
    if (z % 4 != 0) {
      interruptsSignaled |= ALIGNMENT_EXCEPTION;
      return;
    }
    pc = z;
  } else {
    pc += 4;
  }
}



/* jumpIfTrueData24 (cond, instr)
**
** This routine is used in the implementation of the conditional jumping
** instructions.  If the condition is true, it updates pc to effect the
** jump.  Otherwise, it increments the pc.
*/
void jumpIfTrueData24 (int cond, int instr) {
  int z;
  if (cond) {
    z = getData24 (instr);
    if (z % 4 != 0) {
      interruptsSignaled |= ALIGNMENT_EXCEPTION;
      return;
    }
    pc += z;
  } else {
    pc += 4;
  }
}



/* controlC (sig)
**
** This routine is called when the user hits control-C.  It sets
** "controlCPressed" to TRUE.  In the fetch-increment-execute loop, this
** variable is checked before each instruction and execution of BLITZ
** instructions is terminated if it is ever found to be true.  The variable
** is also reset to false at that time.
**
** If "controlCPressed" was already TRUE when this routine is called,
** then it must be that control-C has just been pressed twice in a row, with
** the first press not having been "absorbed".  This could be due to a
** problem in the emulator itself; perhaps the emulator is stuck in an
** infinite loop.  In this case, this routine will abort the emulator.
*/
void controlC (int sig) {
  if (sig != SIGINT) {
    fatalError ("PROGRAM LOGIC ERROR: Expected SIGINT in signal handler");
  }
  signal (SIGINT, controlC);
  printf ("\n*****  Control-C  *****\n");
  if (controlCPressed) {
    turnOffTerminal ();
    fprintf (stderr, "\nBLITZ Emulator quiting\n");
    /* raise (SIGSEGV);  Produce a core dump. */
    errorExit ();
  } else {
    controlCPressed = 1;
  }
}



/* randomBetween (lo, high)
**
** This routine returns the next random number between the given numbers,
** inclusive.
*/
int randomBetween (int lo, int high) {
  int gap = high - lo + 1;
  int i = genRandom () % gap;
  return lo + i;
}



/*
** genRandom ()
**
** This routine returns a random number within the range
** 1,2,3,... 2147483646  (i.e., 2**31-2) and updates the random seed.
** See "Random Number Generators: Good Ones are Hard to Find", by
** Stephen K. Parks and Keith W. Miller, CACM 31:10, October 1988.
*/
int genRandom () {

#define randomA 16807
#define randomM 2147483647
#define randomQ 127773
#define randomR 2836

  int lo,hi,test;

  hi = randomSeed / randomQ;
  lo = randomSeed % randomQ;
  test = (randomA * lo) - (randomR * hi);
  if (test > 0) {
    randomSeed = test;
  } else {
    randomSeed = test + randomM;
  }
  return (randomSeed);      /*  to normalize, use random/m */
} 



/* doTimerEvent ()
**
** This routine is called when a timer event is due.  It will signal and
** interrupt and schedule another timer event in the future.
*/
void doTimerEvent () {

  /* Set the timeOfNextTimerEvent. */
  if (TIME_SLICE <= 0) {
    timeOfNextTimerEvent = MAX;
  } else {
    timeOfNextTimerEvent =
         randomBetween (currentTime + TIME_SLICE,
                        currentTime + TIME_SLICE + TIME_SLICE_VARIATION);
    if (timeOfNextTimerEvent <= currentTime) {
      timeOfNextTimerEvent = currentTime + 1;
    }
    /* Signal a timer interrupt. */
    interruptsSignaled |= TIMER_INTERRUPT;
  }
}



/* doDiskEvent ()
**
** This routine is called when a disk event is due.  It will:
**    Set the "currentDiskStatus" to "futureDiskStatus,"
**    Signal a DISK_INTERRUPT, and
**    Not schedule any future disk events.
*/
void doDiskEvent () {

  /* Signal a disk interrupt. */
  interruptsSignaled |= DISK_INTERRUPT;

  /* Change the disk status to WAITING. */
  currentDiskStatus = futureDiskStatus;

  /* Do not schedule another disk event. */
  timeOfNextDiskEvent = MAX;
}



/* doSerialInEvent (waitForKeystroke)
**
** This routine is called when a serial input event is due.  We need to check
** the input and see if we have a new character on the input.  If so, we
** need to signal a serial interrupt.  Normally, we do not wait for the user
** to type something, but if "waitForKeystroke" is true, we will wait for
** at least one keystroke.
*/
void doSerialInEvent (int waitForKeystroke) {
  int ch;

  /* If the last character was never used, report it. */
  if (!termInCharWasUsed) {
    termInCharWasUsed = 1;
    if ((termInChar >= ' ') && (termInChar < 0x7f)) {
      fprintf (stderr, "\n\rERROR: The serial input character \"%c\" was not fetched in a timely way and has been lost!\n\r",
             termInChar);
    } else {
      fprintf (stderr, "\n\rERROR: The serial input character 0x%02X was not fetched in a timely way and has been lost!\n\r",
             termInChar);
    }
    // controlCPressed = 1;
    return;
  }

  /* See if we have a new key pressed.  If so, signal an interrupt. */
  ch = checkForInput (waitForKeystroke);
  if (ch != 0) {
    termInChar = ch;
    termInCharWasUsed = 0;
    termInCharAvail = 1;
    interruptsSignaled |= SERIAL_INTERRUPT;
  }

  /* Figure out when to check the keyboard next. */
  if (timeOfNextSerialInEvent != MAX) {
    timeOfNextSerialInEvent =
        randomBetween (
           currentTime + KEYBOARD_WAIT_TIME,
           currentTime + KEYBOARD_WAIT_TIME + KEYBOARD_WAIT_TIME_VARIATION);
    if (timeOfNextSerialInEvent <= currentTime) {
      timeOfNextSerialInEvent = currentTime + 1;
    }
  }

}



/* doSerialOutEvent ()
**
** This routine is called when a serial outout event is due.  If we are
** busy sending a character to the terminal, then at this event, we are
** done and the terminal is once again ready.  Set the terminal output
** status to "output ready" and signal an interrupt.
*/
void doSerialOutEvent () {
  if (!termOutputReady) {
    termOutputReady = 1;
    interruptsSignaled |= SERIAL_INTERRUPT;
  }
  timeOfNextSerialOutEvent = MAX;
}



/* updateTimeOfNextEvent ()
**
** This routine sets "timeOfNextEvent".  It looks at the times of all of
** the next scheduled events and uses the minimum of all of these.
*/
void updateTimeOfNextEvent () {
  timeOfNextEvent = MAX;
  if (timeOfNextEvent > timeOfNextTimerEvent) {
    timeOfNextEvent = timeOfNextTimerEvent;
  }
  if (timeOfNextEvent > timeOfNextDiskEvent) {
    timeOfNextEvent = timeOfNextDiskEvent;
  }
  if (timeOfNextEvent > timeOfNextSerialInEvent) {
    timeOfNextEvent = timeOfNextSerialInEvent;
  }
  if (timeOfNextEvent > timeOfNextSerialOutEvent) {
    timeOfNextEvent = timeOfNextSerialOutEvent;
  }
}



/* divide (a, b)
**
** This routine is passed two integers ("a" and "b").  It divides a by b
** to get a quotient ("q") and remainder ("r"), such that
**
**       a = b*q + r
**
** Furthermore, the remainder follows the mathematical definition of the
** "modulo" operator, namely that the remainder will have the same sign
** as b and that
**
**       0 <= abs(r) < abs(b)
**
** Another way to look at this is that the quotient is the real quotient,
** rounded down to the nearest integer.
**
** For example:
**
**       a   b     q   r     a =  b *  q +  r     a/b   rounded
**      ==  ==    ==  ==    =================    ====   =======
**       7   3     2   1     7 =  3 *  2 +  1     2.3      2
**      -7   3    -3   2    -7 =  3 * -3 +  2    -2.3     -3
**       7  -3    -3  -2     7 = -3 * -3 + -2    -2.3     -3
**      -7  -3     2  -1    -7 = -3 *  2 + -1     2.3      2
**
** This routine modifies global variables "q" and "r".  If b=0 it
** sets q and r to zero and returns immediately.
**
** With this definition of "q" and "r", overflow can and will occur in only
** one situation.  Assuming that we are using 32-bit signed integers, the
** following inputs cause a problem...
**      a = -2147483648
**      b = -1
** The mathematically correct answer is...
**      q = +2147483648
**      r = 0
** Unfortunately, this value of q is not representable.  The underlying
** implementation of the C operators / and % will normally fail, and will
** quietly return the wrong answer...
**      q = -2147483648
**      r = 0
** This routine will simply return these incorrect values.
**
** The C language does not define the / and % operators precisely, but
** only requires that a = b*q + r be true.  This routine is designed to
** return consistent, "correct" answers, regardless of the underlying
** implementation of / and %.
**
** Typical variations in integer division are...
**
** (1) "r" is always non-negative.  0 <= r < abs(b)
**     "q" will be negative when either a or b (but not both) are negative.
**         a   b     q   r     a =  b *  q +  r
**        ==  ==    ==  ==    =================
**         7   3     2   1     7 =  3 *  2 +  1
**        -7   3    -3   2    -7 =  3 * -3 +  2
**         7  -3    -2   1     7 = -3 * -2 +  1
**        -7  -3     3   2    -7 = -3 *  3 +  2
**
** (2) Real division, rounded toward zero.
**     "q" = a/b, rounded toward zero.
**     "q" will be negative when either a or b (but not both) are negative.
**     The sign of "r" will be the same as the sign of "a".
**         a   b     q   r     a =  b *  q +  r     a/b   rounded
**        ==  ==    ==  ==    =================    ====   =======
**         7   3     2   1     7 =  3 *  2 +  1     2.3      2
**        -7   3    -2  -1    -7 =  3 * -2 + -1    -2.3     -2
**         7  -3    -2   1     7 = -3 * -2 +  1    -2.3     -2
**        -7  -3     2  -1    -7 = -3 *  2 + -1     2.3      2
**
** (3) Real division, rounded toward negative infinity.
**     "q" = a/b, rounded toward negative infinity.
**     This results in "r" being the mathematically correct "modulo".
**     "q" will be negative when either a or b (but not both) are negative.
**     "r" will be negative whenever "b" is negative.
**
** This routine implements option number (3).  It works assuming that
** the underlying C implementation uses options (1), (2), or (3).
**
** Overflow cannot occur in this routine, assuming 2's complement
** representation of integers.
*/
void divide (int a, int b) {
  if (b==0) {
    q = r = 0;
    return;
  }
  q = a/b;
  r = a%b;
  if (b>0) {
    if (r<0) {
      q--;        /* Overflow iff q=MIN; but then b=1 and r=0... can't be. */
      r = r + b;  /* r is neg, b is pos; cannot overflow. */
    }
  } else {
    if (r>0) {
      q--;        /* Overflow iff q=MIN; but then b=1 and r=0... can't be. */
      r = r + b;  /* r is pos, b is neg; cannot overflow. */
    }
  }
}



/* turnOnTerminal ()
**
** This routine is called before BLITZ instruction execution begins.
** Terminal input may come from "stdin", which is normally used by
** the emulator itself.  If input is coming from stdin, this routine
** sets up the terminal by turning off buffering and echoing, so that
** it will behave like an "ideal BLITZ terminal".  If input is coming
** from a file, then this routine does nothing.
**
** NOTE: We assume that the Emulator is running under Unix and that the
** terminal is normally running with "stty -raw echo".
*/
void turnOnTerminal () {
  if (termInputFile == stdin) {
    clearerr (stdin);  /* clear previous control-D/EOFs */
    if (terminalWantRawProcessing) {
      terminalInRawMode = 1;
      system ("stty raw -echo");
    }
  }
}



/* turnOffTerminal ()
**
** This routine is called when execution of BLITZ instructions is complete.
** If the terminal was in raw mode, it restores it so that it will
** behave normally.
*/
void turnOffTerminal () {
  if (terminalInRawMode) {
    /* Change back to cooked mode */
    terminalInRawMode = 0;
    system ("stty -raw echo");
  }

  /* The following code appears to be useless, or worse... */
  //     /* Clear any pending characters... */
  //     // The following statement causes problems when commands come from a file.
  //     // It can be commented out to prevent infinite looping.
  //     fseek (stdin, 0l, SEEK_SET);
  //     clearerr (stdin);  /* clear previous control-D/EOFs */
}



/* checkForInput (waitForKeystroke) -> char
**
** This routine checks to see if a character has appeared on the input.
** If so, it returns the character.  If "waitForKeystroke" is true, this
** routine will wait for a keystroke; otherwise it will return 0 without
** waiting.  
**
** If the input is coming from stdin, this routine checks to see if the
** input character happens to be control-C; if so, it is processed
** immediately (by setting a flag) and zero is returned.
**
** If we have characters buffered in the type-ahead buffer, then these
** are used before getting characters from stdin.  The characters may also
** be coming from a file besides stdin.
*/
char checkForInput (int waitForKeystroke) {
  int i, j;
  char ch, other;
#define TERM_BUFF_SIZE 1
  char terminalBuffer [TERM_BUFF_SIZE];

  /* Print the current typeAheadBuffer... */
  //  printf ("  typeAheadBuffer = ");
  //  printTypeAheadBuffer ();

  /* See if input comes from stdin and we have something in the type-ahead
     buffer... */
  if (typeAheadBufferCount > 0) {
    ch = typeAheadBuffer [typeAheadBufferOut++];
    if (typeAheadBufferOut >= TYPE_AHEAD_BUFFER_SIZE) {
      typeAheadBufferOut = 0;
    }
    typeAheadBufferCount--;
    return ch;
  }

  /* Return immediately if input=stdin & we don't want to wait & no char is avail. */
  if (termInputFile == stdin) {
    if (!waitForKeystroke)  {
      if (!characterAvailableOnStdin ()) {
        // printf ("  returning(stdin,noWait,nothingReady) \"\\0\"...\n");
        return 0;
      }
    }
  }

  if (feof (termInputFile)) {
    if (termInputFile == stdin) {
      printf ("\n\r*****  EOF on input ignored: Use Control-C to halt execution  *****\n\r");
      // fseek (stdin, 0l, SEEK_SET);
      clearerr (stdin);
      return 0;
    } else {
      if (waitForKeystroke) {
        printf ("\n\r*****  EOF encountered on input  *****\n\r");
        controlCPressed = 1;
        return 0;
      } else {
        /* No further input is available... */
        timeOfNextSerialInEvent = MAX;
        return 0;
      }
    }
  }

  i = fread (terminalBuffer, 1, 1 /*TERM_BUFF_SIZE*/, termInputFile);

  if (feof (termInputFile) && termInputFile == stdin) {
    printf ("\n\r*****  EOF on input ignored: Use Control-C to halt execution  *****\n\r");
    // fseek (stdin, 0l, SEEK_SET);
    clearerr (stdin);
    return 0;
  }

  if (i >= 1) {
    ch = terminalBuffer [0];
  } else {
    return 0;
  }

  /* If not in raw mode, then put the remaining characters on this line
     into the typeAheadBuffer. */
  if (!terminalInRawMode && ch != '\n') {
    while (1) {
      i = fread (terminalBuffer, 1, 1 /*TERM_BUFF_SIZE*/, termInputFile);
      if (i == 0) break;
      other = terminalBuffer [0];
      /* printf ("    Adding 0x%02X to buffer\n", other); */
      addToTypeAhead (other);
      if (other == '\n')  break;
    }
  }

  if (termInputFile == stdin) {
    if (ch == 3) {   /* if ch = Control-C... */
      printf ("\n\r*****  Control-C  *****\n\r");  /* raw mode: use \n\r... */
      controlCPressed = 1;
      // printf ("  returning (ch == contgrol-C): \"\\0\"...\n");
      return 0;
    }
  }

  return ch;
}



/* characterAvailableOnStdin ()
**
** Return true if there is a character available for reading on stdin,
** and return false otherwise.  Do not wait.  This function works by calling
** the Unix function "select".  This function requires <fcntl.h>.
*/
int characterAvailableOnStdin () {
  int numbits = 32;
  int rfd = 1;
  int wfd = 0;
  int xfd = 0;
  int retval;
  struct timeval pollTime;
  pollTime.tv_sec = 0;
  pollTime.tv_usec = 0;
  retval = select (numbits,
                   (fd_set *) &rfd,
                   (fd_set *) &wfd,
                   (fd_set *) &xfd,
                   &pollTime);
  if ((retval != 1) && (retval != 0)) {
    fatalError ("Problems with Unix function select");
  }
  return retval;
}



/* initializeDisk ()
**
** This routine initializes the disk file and variables associated with the DISK.
*/
void initializeDisk () {
  long len;
  int magic, length;

  currentDiskSector = 0;
  currentDiskStatus = OPERATION_COMPLETED_OK;
  futureDiskStatus = OPERATION_COMPLETED_OK;
  diskTrackCount = 0;
  diskSectorCount = 0;
  diskBufferLow = 0;
  diskBufferHigh = 0;
  diskMemoryAddressRegister = 0x00000000;
  diskSectorNumberRegister = 0x00000000;
  diskSectorCountRegister = 0x00000000;

  /* Close the DISK file if open... */
  if (diskFile != NULL) {
    fclose (diskFile);
  }

  /* Open the DISK file for updating... */
  diskFile = fopen (diskFileName, "r+");
  if (diskFile == NULL) {
    fprintf (stderr,
             "Error in DISK File: File \"%s\" could not be opened for updating.  Simulated disk I/O has been disabled!\n",
             diskFileName);

    return;
  }

  /* Get the size of the DISK file... */
  errno = 0;
  if (fseek (diskFile, 0l, SEEK_END)) {
    if (errno) perror ("Error on DISK file");
    fatalError ("Error during call to fseek");
  }
  len = ftell (diskFile);

  /* Check to see if the file is too big... */
  if (len > ((long) MAX)) {
    fprintf (stderr, "The  maximum integer is %d.\n", MAX);
    fprintf (stderr, "Error in DISK File: The DISK file size exceeds the maximum; You may use the 'format' command to change the file.\n");
    errno = 0;
    fclose (diskFile);
    if (errno) perror ("Error closing DISK file");
    diskFile = NULL;
    return;
  }

  length = (int) len;
  /* printf ("The length of the file is %d\n", length); */

  /* Check for bad file length and abort... */
  if (length < (SECTORS_PER_TRACK * PAGE_SIZE + 4)) {
    fprintf (stderr, "The minimum DISK file size is 1 track + 4 bytes (where PageSize = %d bytes and SectorsPerTrack = %d).\n", PAGE_SIZE, SECTORS_PER_TRACK);
    fprintf (stderr, "Error in DISK File: The DISK file must be large enough for at least 1 track.\n");
    errno = 0;
    fclose (diskFile);
    if (errno) perror ("Error closing DISK file");
    diskFile = NULL;
    return;
  }

  /* Check for bad file length and abort... */
  diskSectorCount = (length - 4) / PAGE_SIZE;
  if (diskSectorCount * PAGE_SIZE + 4 != length) {
    fprintf (stderr, "PageSize = %d bytes, SectorsPerTrack = %d, DISK file size = %d.\n", PAGE_SIZE, SECTORS_PER_TRACK, length);
    fprintf (stderr, "Error in DISK File: The DISK file size is not an even number of tracks plus 4 bytes\n");
    errno = 0;
    fclose (diskFile);
    if (errno) perror ("Error closing DISK file");
    diskFile = NULL;
    return;
  }

  diskTrackCount = diskSectorCount / SECTORS_PER_TRACK;
  /* printf ("diskSectorCount = %d\n", diskSectorCount); */
  /* printf ("diskTrackCount = %d\n", diskTrackCount); */

  /* Reposition the DISK file to the beginning. */
  errno = 0;
  if (fseek (diskFile, 0l, SEEK_SET)) {
    if (errno) perror ("Error on DISK file");
    fclose (diskFile);
    diskFile = NULL;
    return;
  }

  /* Check the magic number. */
  magic = readInteger (diskFile);
  if (magic != 0x424C5A64) {
    fprintf (stderr, "Error in DISK File: Magic number is not 'BLZd'\n");
    errno = 0;
    fclose (diskFile);
    if (errno) perror ("Error closing DISK file");
    diskFile = NULL;
    return;
  }

}



/* performDiskIO (command)
**
** This routine is called whenever the program writes into the DISK_COMMAND_WORD.
** It will determine the what operation is called for and perform all error
** checking.  It will then perform the actual operation, reading from or writing to
** the DISK file.  Finally, it will determine what outcome to simulate (e.g., 
** OPERATION_COMPLETED_OK, OPERATION_COMPLETED_WITH_ERROR_3, etc).  It will
** schedule an event in the future.  At that future event, an interrupt will occur
** and the disk status will then change.
*/
void performDiskIO (int command) {
  int seekTime, accessTime, futureTime, currentAngle, desiredAngle, angleChange, transferTime;

  /* Make sure the DISK is working... */
  if (diskFile == NULL) {
    fprintf (stderr, "\n\rERROR:  A store into the DISK_COMMAND_WORD has occurred, but the DISK is currently disabled.  (See the \"format\" command.)\n\r");
    controlCPressed = 1;
    return;
  }

  /* Make sure the command is legal... */
  if (command == DISK_READ_COMMAND) {
    // fprintf (stderr, "\n\rNOTE: A disk read command has been initiated...\n\r");
  } else if (command == DISK_WRITE_COMMAND) {
    // fprintf (stderr, "\n\rNOTE: A disk write command has been initiated...\n\r");
  }  else {
    currentDiskStatus = DISK_BUSY;
    futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_5;
    timeOfNextDiskEvent = currentTime+1;
    updateTimeOfNextEvent ();
    fprintf (stderr, "\n\rERROR: The program has stored an invalid command into the DISK_COMMAND_WORD.  An interrupt will occur when you proceed!\n\r");
    controlCPressed = 1;
    return;
  }

  currentDiskStatus = DISK_BUSY;

  /* Check for ERROR 1... */
  if (diskMemoryAddressRegister % PAGE_SIZE != 0) {
    fprintf (stderr, "\n\rDISK ERROR: The DISK_MEMORY_ADDRESS_REGISTER is not page-aligned.  An interrupt will occur when you proceed!\n\r");
    futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_1;
    timeOfNextDiskEvent = currentTime+1;
    updateTimeOfNextEvent ();
    controlCPressed = 1;
    return;
  }

  /* Make sure the sector count is positive... */
  if (diskSectorCountRegister <= 0) {
    fprintf (stderr, "\n\rDISK ERROR: The DISK_SECTOR_COUNT_REGISTER is not positive.  An interrupt will occur when you proceed!\n\r");
    futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_1;
    timeOfNextDiskEvent = currentTime+1;
    updateTimeOfNextEvent ();
    controlCPressed = 1;
    return;
  }

  /* Compute the area of memory affected... */
  diskBufferLow = diskMemoryAddressRegister;
  diskBufferHigh = diskBufferLow + (diskSectorCountRegister * PAGE_SIZE);

  /* Make sure the memory buffer is in physical memory... */
  if (!physicalAddressOk (diskBufferLow) ||
      !physicalAddressOk (diskBufferHigh-1) ||
      inMemoryMappedArea (diskBufferLow) ||
      inMemoryMappedArea (diskBufferHigh-1)) {
    fprintf (stderr, "\n\rDISK ERROR: The memory buffer is not all in physical memory or is in the memory-mapped region.  An interrupt will occur when you proceed!\n\r");
    futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_2;
    timeOfNextDiskEvent = currentTime+1;
    updateTimeOfNextEvent ();
    controlCPressed = 1;
    return;
  }

  /* Check that we are accessing legal disk sectors... */
  if ((diskSectorNumberRegister < 0) ||
      (diskSectorNumberRegister + diskSectorCountRegister > diskSectorCount)) {
    fprintf (stderr, "\n\rDISK ERROR: Attempting to read sectors that do not exist on the disk.  An interrupt will occur when you proceed!\n\r");
    futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_3;
    timeOfNextDiskEvent = currentTime+1;
    updateTimeOfNextEvent ();
    controlCPressed = 1;
    return;
  }

  /* See if we need to simulate a random disk I/O error... */
  if (command == DISK_READ_COMMAND) {
    if (DISK_READ_ERROR_PROBABILITY > 0) {
      if (1 == randomBetween (1, DISK_READ_ERROR_PROBABILITY)) {
        /***
        fprintf (stderr, "\n\rDISK ERROR: A random disk read error will be simulated at this time.  An interrupt will occur when you proceed!\n\r");
        controlCPressed = 1;
        ***/
        futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_4;
        timeOfNextDiskEvent = currentTime+1;
        updateTimeOfNextEvent ();
        return;
      }
    }
  } else if (command == DISK_WRITE_COMMAND) {
    if (DISK_WRITE_ERROR_PROBABILITY > 0) {
      if (1 == randomBetween (1, DISK_WRITE_ERROR_PROBABILITY)) {
        /***
        fprintf (stderr, "\n\rDISK ERROR: A random disk write error will be simulated at this time.  An interrupt will occur when you proceed!\n\r");
        controlCPressed = 1;
        ***/
        futureDiskStatus = OPERATION_COMPLETED_WITH_ERROR_4;
        timeOfNextDiskEvent = currentTime+1;
        updateTimeOfNextEvent ();
        return;
      }
    }
  }

  /* Everything looks good for a normal disk read/write... */
  futureDiskStatus = OPERATION_COMPLETED_OK;

  /* Compute a realistic delay... */
  // printf ("    Current sector = %d\n", currentDiskSector);
  // printf ("    Future sector = %d\n", diskSectorNumberRegister);
  // printf ("    Current track = %d\n", currentDiskSector / SECTORS_PER_TRACK);
  // printf ("    Future track = %d\n", diskSectorNumberRegister / SECTORS_PER_TRACK);
  accessTime = DISK_SEEK_TIME * abs (
                diskSectorNumberRegister / SECTORS_PER_TRACK -
                currentDiskSector / SECTORS_PER_TRACK);
  // printf ("    Seek time = %d\n", accessTime);
  if (accessTime) {
    accessTime = accessTime + DISK_SETTLE_TIME;
  }
  // printf ("    Access time = %d\n", accessTime);
  // printf ("    Current time before seek,settle = %d\n", currentTime);
  futureTime = currentTime + accessTime;
  // printf ("    Time after seek,settle = %d\n", futureTime);
  currentAngle = (futureTime / DISK_ROTATIONAL_DELAY) % SECTORS_PER_TRACK; 
  // printf ("    Rotational Angle (0..15) at that time = %d\n", currentAngle);
  desiredAngle = diskSectorNumberRegister % SECTORS_PER_TRACK;
  // printf ("    Desired Angle (0..15) = %d\n", desiredAngle);
  angleChange = desiredAngle - currentAngle;
  if (angleChange < 0) {
    angleChange = angleChange + SECTORS_PER_TRACK;
  }
  // printf ("    Angle Change (0..15) = %d\n", angleChange);
  futureTime = futureTime + angleChange * DISK_ROTATIONAL_DELAY;
  // printf ("    Time after seek,settle,rotate = %d\n", futureTime);
  // printf ("    Number of sectors transfered = %d\n", diskSectorCountRegister);
  transferTime = diskSectorCountRegister * DISK_ROTATIONAL_DELAY;
  // printf ("    Transfer time = %d\n", transferTime);
  futureTime = futureTime + transferTime;
  // printf ("    Time after seek,settle,rotate,transfer = %d\n", futureTime);
  futureTime = randomBetween (futureTime, futureTime + DISK_ACCESS_VARIATION);
  // printf ("    Time after seek,settle,rotate,transfer,random = %d\n", futureTime);

  // Set up the next disk event, based on the computed time of completion...
  if (futureTime <= currentTime) {
    timeOfNextDiskEvent = currentTime + 1;
  } else {
    timeOfNextDiskEvent = futureTime;
  }
  updateTimeOfNextEvent ();

  /* Set the new Current Disk Position... */
  currentDiskSector = diskSectorNumberRegister + diskSectorCountRegister;

  /* Seek to the proper location in the file... */
  errno = 0;
  if (fseek (diskFile, ((long) ((diskSectorNumberRegister * PAGE_SIZE) + 4)), SEEK_SET)) {
    fprintf (stderr, "\n\rError from host OS during DISK read/fseek; Disk I/O has been disabled!\n\r");
    if (errno) perror ("Host error");
    fclose (diskFile);
    diskFile = NULL;
    controlCPressed = 1;
    return;
  }

  if (command == DISK_READ_COMMAND) {

    numberOfDiskReads++;

    /* Read in N sectors of data... */
    errno = 0;
    fread ( (void *) (((int) memory) + diskBufferLow),
            PAGE_SIZE, diskSectorCountRegister, diskFile );
    if (errno) {
      fprintf (stderr, "\n\rError from host OS during DISK read; Disk I/O has been disabled!\n\r");
      if (errno) perror ("Host error");
      fclose (diskFile);
      diskFile = NULL;
      controlCPressed = 1;
      return;
    }
    return;

  } else if (command == DISK_WRITE_COMMAND) {

    numberOfDiskWrites++;

    /* Write in N sectors of data... */
    errno = 0;
    fwrite ( (void *) (((int) memory) + diskBufferLow),
            PAGE_SIZE, diskSectorCountRegister, diskFile );
    if (errno) {
      fprintf (stderr, "\n\rError from host OS during DISK write; Disk I/O has been disabled!\n\r");
      if (errno) perror ("Host error");
      fclose (diskFile);
      diskFile = NULL;
      controlCPressed = 1;
      return;
    }
    return;
  }
}



/* checkDiskBufferError (physAddr)
**
** This routine checks the address to see if it is within the memory that is being used
** for a disk operation.  If it is, then a message is printed and execution is halted.
*/
void checkDiskBufferError (int physAddr) {
  if (currentDiskStatus == DISK_BUSY && physAddr >= diskBufferLow && physAddr < diskBufferHigh) {
    fprintf (stderr, "\n\rMEMORY ACCESS ERROR: The BLITZ program has attempted to read or write a memory word that is currently involved in a disk read or write operation!\n\r");
    controlCPressed = 1;
  }
}




/* setSimulationConstants ()
**
** This routine initializes some important constants related to the simulation.
** It gets values from the ".blitzrc" file if it exists, or uses defaults, if not.
*/
void setSimulationConstants () {
  int i, errorInValue;
  FILE * blitzrc;
  char * first, * second, * p;
    errorInValue = 0;

    defaultSimulationConstants ();

    // Try to open the ".blitzrc" file...
    blitzrc = fopen (".blitzrc", "r");
    if (blitzrc != NULL) {
      // printf ("Reading simulation values from file \".blitzrc\"...\n");

      while (! feof (blitzrc)) {
        fgets (inputBuffer, sizeof(inputBuffer), blitzrc);

        if (feof (blitzrc)) break;

        /* Ignore comments... */
        if (inputBuffer[0] == '!') continue;

        /* Overwrite the \n with \0 to remove it. */
        inputBuffer [strlen (inputBuffer)-1] = '\0';
        // printf ("LINE >>>%s<<<\n", inputBuffer);

        /* Set "first" to point to the first non-whitespace character. */
        first = inputBuffer;
        while (1) {
          if ((*first != ' ') && (*first != '\t')) {
            break;
          }
          first++;
        }

        /* Ignore blank lines... */
        if (*first == '\0') continue;

        /* Set "p" to point to the next whitespace or \0 character. */
        p = first;
        while (1) {
          if ((*p == ' ') || (*p == '\t') || (*p == '\0')) {
            break;
          }
          p++;
        }

        // printf ("  First Token >>>%s<<<\n", first);

        if (*p == '\0') {
          fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  The line beginning \"%s\" has no value!\n", first);
          errorInValue = 1;
          continue;
        }

        *p = '\0';
        // printf ("  First Token >>>%s<<<\n", first);

        /* Set "second" to point to the next non-whitespace character. */
        second = p+1;
        while (1) {
          if ((* second != ' ') && (* second != '\t')) {
            break;
          }
          second ++;
        }

        /* Set "p" to point to the next whitespace or \0 character. */
        p = second;
        while (1) {
          if ((*p == ' ') || (*p == '\t') || (*p == '\0')) {
            break;
          }
          p++;
        }

        if (*p != '\0') {
          *p = '\0';
          /* Make sure that there is nothing but whitespace until the \0 character. */
          p++;
          while (*p != '\0') {
            if ((*p != ' ') && (*p != '\t')) {
              fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  Extraneous characters (\"%s\") on line beginning \"%s\"!\n", p, first);
              errorInValue = 1;
              break;
            }
            p++;
          }
        }

        if (*second == '\0') {
          fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  The line beginning \"%s\" has no value!\n", first);
          errorInValue = 1;
          continue;
        }

        // printf ("  PROCESSING: First Token >>>%s<<<    ", first);
        // printf ("  Second Token >>>%s<<<\n", second);
        if (second[0] == '0' &&
            second[1] == 'x') {
          second = second + 2;
          sscanf (second, "%x", &i);
        } else {
          sscanf (second, "%d", &i);
        }
        // printf ("------- i = 0x%08X (decimal: %d)\n", i, i);

        if (!strcmp (first, "KEYBOARD_WAIT_TIME")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  KEYBOARD_WAIT_TIME is negative!\n");
            errorInValue = 1;
          } else {
            KEYBOARD_WAIT_TIME = i;
          }
        } else if (!strcmp (first, "KEYBOARD_WAIT_TIME_VARIATION")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  KEYBOARD_WAIT_TIME_VARIATION is negative!\n");
            errorInValue = 1;
          } else {
            KEYBOARD_WAIT_TIME_VARIATION = i;
          }
        } else if (!strcmp (first, "TERM_OUT_DELAY")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  TERM_OUT_DELAY is negative!\n");
            errorInValue = 1;
          } else {
            TERM_OUT_DELAY = i;
          }
        } else if (!strcmp (first, "TERM_OUT_DELAY_VARIATION")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  TERM_OUT_DELAY_VARIATION is negative!\n");
            errorInValue = 1;
          } else {
            TERM_OUT_DELAY_VARIATION = i;
          }
        } else if (!strcmp (first, "TIME_SLICE")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  TIME_SLICE is negative!\n");
            errorInValue = 1;
          } else {
            TIME_SLICE = i;
          }
        } else if (!strcmp (first, "TIME_SLICE_VARIATION")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  TIME_SLICE_VARIATION is negative!\n");
            errorInValue = 1;
          } else {
            TIME_SLICE_VARIATION = i;
          }
        } else if (!strcmp (first, "DISK_SEEK_TIME")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_SEEK_TIME is negative!\n");
            errorInValue = 1;
          } else {
            DISK_SEEK_TIME = i;
          }
        } else if (!strcmp (first, "DISK_SETTLE_TIME")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_SETTLE_TIME is negative!\n");
            errorInValue = 1;
          } else {
            DISK_SETTLE_TIME = i;
          }
        } else if (!strcmp (first, "DISK_ROTATIONAL_DELAY")) {
          if (i < 1) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_ROTATIONAL_DELAY must be >= 1!\n");
            errorInValue = 1;
          } else {
            DISK_ROTATIONAL_DELAY = i;
          }
        } else if (!strcmp (first, "DISK_ACCESS_VARIATION")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_ACCESS_VARIATION is negative!\n");
            errorInValue = 1;
          } else {
            DISK_ACCESS_VARIATION = i;
          }
        } else if (!strcmp (first, "DISK_READ_ERROR_PROBABILITY")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_READ_ERROR_PROBABILITY is negative!\n");
            errorInValue = 1;
          } else {
            DISK_READ_ERROR_PROBABILITY = i;
          }
        } else if (!strcmp (first, "DISK_WRITE_ERROR_PROBABILITY")) {
          if (i < 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_WRITE_ERROR_PROBABILITY is negative!\n");
            errorInValue = 1;
          } else {
            DISK_WRITE_ERROR_PROBABILITY = i;
          }
        } else if (!strcmp (first, "INIT_RANDOM_SEED")) {
          if (i <= 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  INIT_RANDOM_SEED is <= 0!\n");
            errorInValue = 1;
          } else if (i > 2147483646) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  INIT_RANDOM_SEED is > 2147483646!\n");
            errorInValue = 1;
          } else {
            INIT_RANDOM_SEED = i;
          }
        } else if (!strcmp (first, "MEMORY_SIZE")) {
          /* Check that the MEMORY_SIZE is a multiple of PAGE_SIZE. */
          if (i % PAGE_SIZE != 0 || i <= 0) {
            fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  Memory size is not a multiple of page size!\n");
            errorInValue = 1;
          } else {
            MEMORY_SIZE = i;
          }
        } else if (!strcmp (first, "MEMORY_MAPPED_AREA_LOW")) {
          MEMORY_MAPPED_AREA_LOW = i;
        } else if (!strcmp (first, "MEMORY_MAPPED_AREA_HIGH")) {
          MEMORY_MAPPED_AREA_HIGH = i;
        } else if (!strcmp (first, "SERIAL_STATUS_WORD_ADDRESS")) {
          SERIAL_STATUS_WORD_ADDRESS = i;
        } else if (!strcmp (first, "SERIAL_DATA_WORD_ADDRESS")) {
          SERIAL_DATA_WORD_ADDRESS = i;
        } else if (!strcmp (first, "DISK_STATUS_WORD_ADDRESS")) {
          DISK_STATUS_WORD_ADDRESS = i;
        } else if (!strcmp (first, "DISK_COMMAND_WORD_ADDRESS")) {
          DISK_COMMAND_WORD_ADDRESS = i;
        } else if (!strcmp (first, "DISK_MEMORY_ADDRESS_REGISTER")) {
          DISK_MEMORY_ADDRESS_REGISTER = i;
        } else if (!strcmp (first, "DISK_SECTOR_NUMBER_REGISTER")) {
          DISK_SECTOR_NUMBER_REGISTER = i;
        } else if (!strcmp (first, "DISK_SECTOR_COUNT_REGISTER")) {
          DISK_SECTOR_COUNT_REGISTER = i;
        } else {
          fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  An attempt to set non-existent value \"%s\"!\n", first);
          errorInValue = 1;
        }

      }

      if (MEMORY_MAPPED_AREA_LOW < 0 ||
          MEMORY_MAPPED_AREA_HIGH >= MEMORY_SIZE ||
          MEMORY_MAPPED_AREA_LOW >= MEMORY_MAPPED_AREA_HIGH ||
          MEMORY_MAPPED_AREA_LOW % 4 != 0 ||
          (MEMORY_MAPPED_AREA_HIGH+1) % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  MEMORY_MAPPED_AREA_HIGH or MEMORY_MAPPED_AREA_LOW are invalid!\n");
        errorInValue = 1;
      }
      if (SERIAL_STATUS_WORD_ADDRESS < MEMORY_MAPPED_AREA_LOW ||
          SERIAL_STATUS_WORD_ADDRESS > MEMORY_MAPPED_AREA_HIGH ||
          SERIAL_STATUS_WORD_ADDRESS % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  SERIAL_STATUS_WORD_ADDRESS is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }
      if (SERIAL_DATA_WORD_ADDRESS < MEMORY_MAPPED_AREA_LOW ||
          SERIAL_DATA_WORD_ADDRESS > MEMORY_MAPPED_AREA_HIGH ||
          SERIAL_DATA_WORD_ADDRESS % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  SERIAL_DATA_WORD_ADDRESS is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }
      if (DISK_STATUS_WORD_ADDRESS < MEMORY_MAPPED_AREA_LOW ||
          DISK_STATUS_WORD_ADDRESS > MEMORY_MAPPED_AREA_HIGH ||
          DISK_STATUS_WORD_ADDRESS % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_STATUS_WORD_ADDRESS is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }
      if (DISK_COMMAND_WORD_ADDRESS < MEMORY_MAPPED_AREA_LOW ||
          DISK_COMMAND_WORD_ADDRESS > MEMORY_MAPPED_AREA_HIGH ||
          DISK_COMMAND_WORD_ADDRESS % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_COMMAND_WORD_ADDRESS is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }
      if (DISK_MEMORY_ADDRESS_REGISTER < MEMORY_MAPPED_AREA_LOW ||
          DISK_MEMORY_ADDRESS_REGISTER > MEMORY_MAPPED_AREA_HIGH ||
          DISK_MEMORY_ADDRESS_REGISTER % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_MEMORY_ADDRESS_REGISTER is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }
      if (DISK_SECTOR_NUMBER_REGISTER < MEMORY_MAPPED_AREA_LOW ||
          DISK_SECTOR_NUMBER_REGISTER > MEMORY_MAPPED_AREA_HIGH ||
          DISK_SECTOR_NUMBER_REGISTER % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_SECTOR_NUMBER_REGISTER is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }
      if (DISK_SECTOR_COUNT_REGISTER < MEMORY_MAPPED_AREA_LOW ||
          DISK_SECTOR_COUNT_REGISTER > MEMORY_MAPPED_AREA_HIGH ||
          DISK_SECTOR_COUNT_REGISTER % 4 != 0) {
        fprintf (stderr, "\n*****  ERROR in \".blitzrc\" file:  DISK_SECTOR_COUNT_REGISTER is not within the Memory Mapped Area or is not word-aligned!\n");
        errorInValue = 1;
      }

      if (errorInValue) {
        fprintf (stderr, "*****  ERROR in \".blitzrc\" file:  All values in the file have been ignored.\n");
        defaultSimulationConstants ();
      }

      fclose (blitzrc);
    }
}



/* defaultSimulationConstants ()
**
** This routine sets the simulation constants to their default values.
*/
void defaultSimulationConstants () {
    KEYBOARD_WAIT_TIME =                 30000;
    KEYBOARD_WAIT_TIME_VARIATION =         100;
    TERM_OUT_DELAY =                       100;
    TERM_OUT_DELAY_VARIATION =              10;
    TIME_SLICE =                          5000;
    TIME_SLICE_VARIATION =                  30;
    DISK_SEEK_TIME =                     10000;
    DISK_SETTLE_TIME =                    1000;
    DISK_ROTATIONAL_DELAY =                100;
    DISK_ACCESS_VARIATION =                 10;
    DISK_READ_ERROR_PROBABILITY =          500;  // 0=never, 1=always, n="about 1/n"
    DISK_WRITE_ERROR_PROBABILITY =         500;  // 0=never, 1=always, n="about 1/n"
    INIT_RANDOM_SEED =              1829742401;  // Default random seed
    MEMORY_SIZE =                   0x01000000;  // 16 MBytes
    MEMORY_MAPPED_AREA_LOW =        0x00ffff00;
    MEMORY_MAPPED_AREA_HIGH =       0x00ffffff;
    SERIAL_STATUS_WORD_ADDRESS =    0x00ffff00;
    SERIAL_DATA_WORD_ADDRESS =      0x00ffff04;
    DISK_STATUS_WORD_ADDRESS =      0x00ffff08;
    DISK_COMMAND_WORD_ADDRESS =     0x00ffff08;
    DISK_MEMORY_ADDRESS_REGISTER =  0x00ffff0c;
    DISK_SECTOR_NUMBER_REGISTER =   0x00ffff10;
    DISK_SECTOR_COUNT_REGISTER =    0x00ffff14;
}



/* printKPLStmtCode ()
**
** This routine looks at the value in r10 and uses to print the name of
** some high-level KPL statement, such as "ASSIGN" or "RETURN".
*/
void printKPLStmtCode () {
  int stmtCode;
  if (statusS) {
    stmtCode = systemRegisters [10];
  } else {
    stmtCode = userRegisters [10];
  }
  printf ("About to execute "); 
  switch (stmtCode) {
//qqqqqqqqqq
    case STMT_CODE_AS:  printf ("ASSIGN statement                 ");
                        break;
    case STMT_CODE_FU:  printf ("FUNCTION ENTRY                   ");
                        break;
    case STMT_CODE_ME:  printf ("METHOD ENTRY                     ");
                        break;
    case STMT_CODE_IF:  printf ("IF statement                     ");
                        break;
    case STMT_CODE_TN:  printf ("THEN statement                   ");
                        break;
    case STMT_CODE_EL:  printf ("ELSE statement                   ");
                        break;
    case STMT_CODE_CA:  printf ("FUNCTION CALL                    ");
                        break;
    case STMT_CODE_CF:  printf ("FUNCTION CALL (via pointer)      ");
                        break;
    case STMT_CODE_CE:  printf ("FUNCTION CALL (external function)");
                        break;
    case STMT_CODE_SE:  printf ("SEND                             ");
                        break;
    case STMT_CODE_WH:  printf ("WHILE LOOP (expr evaluation)     ");
                        break;
    case STMT_CODE_WB:  printf ("WHILE LOOP (body statements)     ");
                        break;
    case STMT_CODE_DO:  printf ("DO-UNTIL (body statements)       ");
                        break;
    case STMT_CODE_DU:  printf ("DO-UNTIL (expr evaluation)       ");
                        break;
    case STMT_CODE_BR:  printf ("BREAK statement                  ");
                        break;
    case STMT_CODE_CO:  printf ("CONTINUE statement               ");
                        break;
    case STMT_CODE_RE:  printf ("RETURN statement                 ");
                        break;
    case STMT_CODE_FO:  printf ("FOR statement                    ");
                        break;
    case STMT_CODE_FB:  printf ("FOR (body statements)            ");
                        break;
    case STMT_CODE_SW:  printf ("SWITCH                           ");
                        break;
    case STMT_CODE_TR:  printf ("TRY statement                    ");
                        break;
    case STMT_CODE_TH:  printf ("THROW statement                  ");
                        break;
    case STMT_CODE_FR:  printf ("FREE statement                   ");
                        break;
    case STMT_CODE_DE:  printf ("DEBUG statement                  ");
                        break;
    case STMT_CODE_CC:  printf ("CATCH clause                     ");
                        break;
    default:            printf ("***INVLALID HIGH-LEVEL STATEMENT CODE IN REGISTER r10***\n");
                        return;
  }
  printCurrentFileLineAndFunction ();
  printf ("\n");
}



/* printCurrentFileLineAndFunction ()
**
** This routine looks at the registers and the current frame and prints
** the current file name, the current line number, and the function/method
** name like this:
**        foo (Main.c, line 123)
** If any problems, it simply gives up and returns.
*/
void printCurrentFileLineAndFunction () {
  int lineNum, fp, physAddr, addrOfRoutineDescriptor, filenamePtr,
      ptrToFunName;

  // Get "r13" and "r14/fp"...
  if (statusS) {
    lineNum = systemRegisters [13];
    fp = systemRegisters [14];
  } else {
    lineNum = userRegisters [13];
    fp = userRegisters [14];
  }

  if (fp == 0) {
    printf ("***Invalid FP***\n");
    return;
  }

  // printf ("frame pointer = ");
  // printNumberNL (fp);

  // Get ptr to routine descriptor
  physAddr = translate (fp-8, 1, 0, 0);  // reading=1, wantPrinting=0, doUpdates=0
  if (translateCausedException) {
    printf ("***Invalid fp***\n");
    return;
  }
  addrOfRoutineDescriptor = getPhysicalWord (physAddr);
  // printf ("addrOfRoutineDescriptor = ");
  // printNumberNL (addrOfRoutineDescriptor);

  // Get ptr to filename...
  physAddr = translate (addrOfRoutineDescriptor, 1, 0, 0);
  if (translateCausedException) {
    printf ("***Invalid routine descriptor***");
    return;
  }
  filenamePtr = getPhysicalWord (physAddr);
  // printf ("filenamePtr = ");
  // printNumberNL (filenamePtr);

  // Get ptr to function name...
  physAddr = translate (addrOfRoutineDescriptor+4, 1, 0, 0);
  if (translateCausedException) {
    printf ("***Invalid routine descriptor***");
    return;
  }
  ptrToFunName = getPhysicalWord (physAddr);
  // printf ("ptrToFunName = ");
  // printNumberNL (ptrToFunName);

  // Print the function name...
  printf (" in ");
  if (!printAsciiDataInWidth (ptrToFunName, 0)) return;

  // Print the Filename...
  printf (" (");
  if (!printAsciiDataInWidth (filenamePtr, 0)) return;

  // Print the line number...
  printf (", line %d)", lineNum);

  // Print the cuurent time...
  printf ("  time = %d", currentTime);

}
