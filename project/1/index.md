# Programming Project 1: Introduction to the BLITZ Tools

**Duration:** One Week

### Overview and Goal

In this course you will be creating an operating system kernel. You’ll be using the BLITZ software tools, which were written for this task. The goals of this project are to make sure that you can use the BLITZ tools and to help you gain familiarity with them.

### Step 1: Read the Overview Document

Read the first document ([An Overview of the BLITZ System](/docs/overview)) before proceeding to next step.

### Step 3: Choose Your Host Platform

You will develop your operating system code on a "host" computer and you will be running the BLITZ tools on that host computer. You should decide now which host computer you will be using.

The BLITZ tools run on the follow host platforms:

 - Apple Macintosh, OS X, either PPC-based or Intel-based machines
 - Unix / Linux Systems
 - Windows, using Cygwin which emulates the Unix POSIX interface (see www.cygwin.com)

You can download the tools (which are written in "C" and "C++". You must then compile them on your computer.

The source code for all the BLITZ tools is available, but you should not need to look at it. Nevertheless, it is available for anyone who is interested.

### The BLITZ Tools

Here are the programs that constitute the BLITZ tool set.

#### kpl
The KPL compiler

#### asm
The BLITZ assembler

#### lddd
The BLITZ linker

#### blitz
The BLITZ machine emulator (the virtual machine and debugger)

#### diskUtil
A utility to manipulate the simulated BLITZ “DISK” file

#### dumpObj
A utility to print BLITZ .o and a.out files

#### hexdump
A utility to print any file in hex

#### check
A utility to run through a file looking for problem ASCII characters

#### endian
A utility to determine if this machine is Big or Little Endian

These tools are listed more-or-less in the order they would be used. You will probably only need to use the first 4 or 5 tools and you may pretty much ignore the remaining tools. (The last three tools are only documented by the comments at the beginning of the source code files, which you may read if interested.)

### Organization of the Course Material

The BLITZ system is accessible via the following URL:

 **https://blitzosproject.github.io**

You may access this material through the BLITZ Home page. You should also be able to download directories of website from https://github.com/BlitzOSProject/BlitzOSProject.github.io .

You can download all resources use [this link](https://github.com/BlitzOSProject/BlitzOSProject.github.io/archive/main.zip), or use git clone:
```
git clone https://github.com/BlitzOSProject/BlitzOSProject.github.io.git
```

### Step 4A: For Unix/Linux/Mac Users...

This section applies to users who have a Unix/Linux box and wish to download and re-compile the BLITZ tools for their machine.

#### Step 1
Create a directory to put the BLITZ source code into. For example, you may wish to create a directory called **BlitzSrc** in your home directory:

```
~/BlitzSrc
```

Then copy all the files from

 **[https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzSrc](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzSrc)**

to your **BlitzSrc** directory.

#### Step 2
Compile the programs in `~/BlitzSrc`

There is a `makefile` so you should be able to execute the following commands to compile the tools.
```
cd ~/BlitzSrc
make
```

This will invoke the "C" and "C++" compilers to produce the following executables:
 - **kpl**
 - **asm**
 - **lddd**
 - **blitz**
 - **diskUtil**
 - **dumpObj**
 - **hexdump**
 - **check**
 - **endian**

*Note:* You will see some warnings, you can ignore them if above files created successfully.

#### Step 3
Create a directory for the executables and move them into it:

```
mkdir ~/BlitzTools
cd ~/BlitzSrc
mv kpl asm lddd blitz diskUtil dumpObj hexDump check endian ~/BlitzTools
```

### Step 4B: For Windows Users...

#### Step 0: Introduction

BLITZ can be used under the Windows operating system and this document describes how.

 
To run under Windows, you’ll have to use a third-party package called __Cygwin__.  Cygwin makes it possible to run most Linux / Unix programs under Windows.  This document includes information about installing Cygwin and then installing BLITZ.

This has been tested for Windows 10 using the current version of Cygwin (3.1.6-1).

#### Step 1: Download and Install Cygwin

Download and install Cygwin from [http://www.cygwin.com](http://www.cygwin.com).

> Cygwin is a UNIX emulator that runs on Windows.  When you download and install it from the Cygwin website, you can select all the default settings, except that BLITZ will require additional libraries that are not part of the default installation.  You must individually select these additional libraries during the Cygwin installation or BLITZ will not run correctly.

From "Choose A Download Source" page, select "Install from internet".

When you get to the "Select Packages" screen, select Category view and then add the following packages:
 - Devel - `gcc-core`
 - Devel - `gcc-g++`
 - Devel - `make`

When you select the above options, several other dependent components will automatically be selected.  Do not change any of these.  Let Cygwin automatically include related components.

Note that the installation does not include any text editors by default.  You’ll need to edit files; you can either edit the files through Windows or you can also install Unix editors like "vi", "emacs" or "nano" with Cygwin.  The Unix editors can be found in the "Editors" section of the Cygwin installation list.
 - Editors - VI (vim)
 - Editors - EMACs
 - Editors - nano

You can also install git to easily download blitz source code.
 - Devel - git

#### Step 2: Create “BlitzSrc” Folder

In cgywin, you can access windows drives files via `/cygdrive`, for example, If you want to put your file in `C:\Workspace\OS\` you can use following command:
```
mkdir -p /cygdrive/c/Workspace/OS
cd /cygdrive/c/Workspace/OS
```

Now, create a folder in your home area in Cygwin called "BlitzSrc".  You can either do this via the Windows host (using something like Explorer) or directly in the Cygwin shell, with the following Unix command:

```
mkdir BlitzSrc
```

You may also want to take a minute to edit your ".bashrc" file at this point, to customize your aliases, etc., if this is something that you are familiar with.

#### Step 3: Download BLITZ Source Files
Download all the files from the BLITZ web site and place them in your new BlitzSrc directory.  The files can be found at:

https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzSrc

You can download them using 
```
git clone --depth 1 https://github.com/BlitzOSProject/BlitzOSProject.github.io.git Blitz
cp -r Blitz/BlitzSrc/* BlitzSrc
```

Go to BlitzSrc directory:
```
cd BlitzSrc
```

If you are using a Windows browser to download the files, the browser may try to save the "makefile" file as "makefile.txt".  If this happens, use the `mv` command in the Cygwin shell to rename the file to just `makefile` with no extension.  You must do this from the Cygwin shell:
```
mv makefile.txt makefile
```

This is important, because as you download other files without extensions, you will need to rename them for use with BLITZ.

You should have these files:
```
asm.c
ast.cc
ast.h
blitz.c
check.c
check.cc
diskUtil.c
dumpObj.c
endian.c
gen.cc
hexdump.c
ir.cc
ir.h
lddd.c
lexer.cc
main.cc
main.h
makefile
makefile-Solaris   (this file is not needed)
mapping.cc
parser.cc
printAst.cc
```

#### Step 4: Compile the BLITZ Source

Compile the downloaded files by running the Unix "make" utility:
```
make
```

Make will generate several lines of output, but you should not see any errors.  If you do see errors go back and verify that all the required Cygwin libraries were downloaded and that all the required BLITZ files were downloaded and placed in the BlitzSrc directory.

You should now have the following new files in this directory:

```
asm.exe
ast.o
blitz.exe
check.exe
check.o
diskUtil.exe
dumpObj.exe
endian.exe
gen.o
hexdump.exe
ir.o
kpl.exe
lddd.exe
lexer.o
main.o
mapping.o
parser.o
printAst.o
```

You can remove the “.o” files with this command:

```
rm *.o
```

#### Step 5: Create “BlitzTools” Directory & Move Executables
Create a new directory called “BlitzTools” in your home area and move the executables into it.  You can use either the Windows host or the following commands in the Cygwin shell:

```
cd ..
mkdir BlitzTools
mv BlitzSrc/*.exe BlitzTools
```

You should now see the following files with the `ls` command:

```
$ ls BlitzTools

asm.exe      check.exe      dumpObj.exe    hexdump.exe    lddd.exe
blitz.exe    diskUtil.exe   endian.exe     kpl.exe
```

### Step 5: Modify Your Search Path and Verify the Tools are Working

You must add the **BlitzTools** directory to your shell’s search path so that when you type in the name of a BLITZ tool (such as **kpl** or **blitz**), your shell can locate the executable file and execute it.

The Unix `shell` program maintains a "shell variable" called `PATH` which it uses to locate an executable whenever a command name is typed. Details of how to change the PATH variable will vary between the different shells.

One approach might be to alter the **.aliases** file in your home directory.

For example, this file may already contain a line that looks something like this:

 **setenv PATH ${PATH}:${HOME}/bin**

(Between each colon (:) is a directory specification. The above command sets PATH to whatever it was before followed by the bin directory in your home directory.)

What you need to do is add the BLITZ tools directory in front of whatever else is in the PATH.

Unix / Linux / Mac users who have placed the executables into a subdirectory in their home directory might add the following command to prepend the appropriate directory to the front of the PATH.

 **setenv PATH ${HOME}/BlitzTools:${PATH}**

The `bash` shell is a little different; these people should add something like this to .bashaliases:

 **export PATH=${HOME}/BlitzTools:${PATH}**

The shell builds an internal hash table that speeds up the location of programs whenever you type a command. After changing your PATH, you’ll need to restart your shell so that it uses the new PATH when it builds this hash table.

You can do this several ways. A Mac user can quit the "Terminal" application and then restart "Terminal". A Unix / Linux / Solaris user can log out and log back in. In some shells you can simply type the command `source .aliases` instead.

Next, verify that whatever you did to the PATH variable worked.

At the UNIX/Linux prompt, type the command.
```
kpl
```

You should see the following:
```
 ***** ERROR: Missing package name on command line

 ************ 1 error detected! **********
```
If you see this, good. If you see anything else, then something is wrong.

### Step 6: Set up a Directory for Project 1

Create a directory in which to place all files concerned with this class. We recommend a name matching your course number, for example:
```
 _~YourUserName_/cs333
```

Create a directory in which to place the files concerned with project 1. We recommend the following name:

```
 _~YourUserName_/cs333/p1
```

Copy all files from: [https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p1](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p1) to your **cs333/p1** directory.

### The BLITZ Assembly Language

In this course you will not have to write any assembly language. However, you will be using some interesting routines which can only be written in assembly. All assembly language routines will be provided to you, but you will need to be able to read them.

Take a look at **Echo.s** and **Hello.s** to see what BLITZ assembly code looks like.

### Step 7: Assemble, Link, and Execute the “Hello” Program

The **p1** directory contains an assembly language program called “Hello.s”. First invoke the assembler (the tool called “asm”) to assemble the program. Type:
```
asm Hello.s
```

This should produce no errors and should create a file called **Hello.o**.

The **Hello.s** program is completely stand-alone. In other words, it does not need any library functions and does not rely on any operating system. Nevertheless, it must be linked to produce an executable (“a.out” file). The linking is done with the tool called “lddd”. (In UNIX, the linker is called “ld”.)

 **lddd** **Hello.o –o Hello**

Normally the executable is called **a.out**, but the `-o Hello` option will name the executable **Hello**.

Finally, execute this program, using the BLITZ virtual machine. (Sometimes the BLITZ virtual machine is referred to as the "emulator") Type:
```
blitz –g Hello
```

The `-g` option is the "auto-go" option and it means begin execution immediately. You should see:
```
 Beginning execution...

 Hello, world!

 **** A 'debug' instruction was encountered *****
```

 Done! The next instruction to execute will be:
```
 000080: A1FFFFB8 jmp 0xFFFFB8 ! targetAddr = main

 Entering machine-level debugger...

 ======================================================

 ===== =====

 ===== The BLITZ Machine Emulator =====

 ===== =====

 ===== Copyright 2001-2007, Harry H. Porter III =====

 ===== =====

 ======================================================
```

 Enter a command at the prompt. Type `quit` to exit or `help` for info about commands.
```
 \>
```

At the prompt, quit and exit by typing “q” (short for “quit”). You should see this:
```
 \> q
 Number of Disk Reads \= 0

 Number of Disk Writes \= 0

 Instructions Executed \= 1705

 Time Spent Sleeping \= 0

 Total Elapsed Time \= 1705
```

This program terminates by executing the **debug** machine instruction. This instruction will cause the emulator to stop executing instructions and will throw the emulator into command mode. In command mode, you can enter commands, such as **quit**. The emulator displays the character “>” as a prompt.

After the debug instruction, the **Hello** program branches back to the beginning. Therefore, if you resume execution (with the **go** command), it will result in another printout of “Hello, world!”.

### Step 8: Run the “Echo” Program

Type in the following commands:
```
asm Echo.s

lddd Echo.o –o Echo
blitz Echo
```

On the last line, we have left out the auto-go "-g" option. Now, the BLITZ emulator will not automatically begin executing; instead it will enter command mode. When it prompts, type the "g"command (short for "go") to begin execution.

Next type some text. Each time the ENTER/RETURN key is pressed, you should see the output echoed. For example:
```
 \> g

 Beginning execution...

 **abcd**

 abcd

 **this is a test**

 this is a test

 **q**

 q

 **** A 'debug' instruction was encountered *****

 Done! The next instruction to execute will be:

 cont:

 0000A4: A1FFFFAC jmp 0xFFFFAC ! targetAddr = loop

 \>
```

This program watches for the "q" character and stops when it is typed. If you resume execution with the **go** command, this program will continue echoing whatever you type.

The **Echo** program is also a stand-alone program, relying on no library functions and no operating system.

The KPL Programming Language
----------------------------

In this course, you will write code in the “KPL” programming language. Begin studying the document titled [An Overview of KPL: A Kernel Programming Language]({{ site.url }}/BlitzDoc/KPLOverview.htm).

### Step 9: Compile and Execute a KPL Program called “HelloWorld”

Type the following commands:
```
kpl -unsafe System
asm System.s
kpl HelloWorld
asm HelloWorld.s
asm Runtime.s
lddd Runtime.o System.o HelloWorld.o -o HelloWorld
```

There should be no error messages.

Take a look at the files **HelloWorld.h** and **HelloWorld.c**. These contain the program code.

The **HelloWorld** program makes use of some other code, which is contained in the files **System.h** and **System.c****.** These must be compiled with the “-unsafe” option. Try leaving this out; you’ll get 17 compiler error messages, such as:
```
 System.h:39: ***** ERROR at PTR: Using 'ptr to void' is unsafe;
 you must compile with the 'unsafe' option
 if you wish to do this
```

Using the UNIX compiler convention, this means that the compiler detected an error on line 39 of file **System.h**.

KPL programs are often linked with routines coded in assembly language. Right now, all the assembly code we need is included in a file called **Runtime.s**. Basically, the assembly code takes care of:

 Starting up the program

 Dealing with runtime errors, by printing a message and aborting

 Printing output (There is no mechanism for input at this stage... This system really needs an OS!)

Now execute this program. Type:
```
blitz –g HelloWorld
```

You should see the “Hello, world...” message. What happens if you type “g” at the prompt, to resume instruction execution?

### The "makefile"

The **p1** directory contains a file called **makefile**, which is used with the UNIX **make** command. Whenever a file in the **p1** directory is changed, you can type “make” to re-compile, re-assemble, and re-link as necessary to rebuild the executables.

Notice that the command
```
kpl HelloWorld
```

will be executed whenever the file **System.h** is changed. In KPL, files ending in “.h” are called “header files” and files ending in “.c” are called “code files.” Each package (such as **HelloWorld**) will have both a header file and a code file. The **HelloWorld** package uses the **System** package. Whenever the header file of a package that **HelloWorld** uses is changed, **HelloWorld** must be recompiled. However, if the code file for **System** is changed, you do not need to recompile **HelloWorld**. You only need to re-link (i.e., you only need to invoke **lddd** to produce the executable).

Consult the KPL documentation for more info about the separate compilation of packages.

### Step 10: Modify the HelloWorld Program

Modify the **HelloWorld.c** program by un-commenting the line
```
 --foo (10)
```

In KPL, comments are “--” through end-of-line. Simply remove the hyphens and recompile as necessary, using “make”.

The **foo** function calls **bar**. **Bar** does the following things:

 Increment its argument

 Print the value

 Execute a “debug” statement

 Recursively call itself

When you run this program it will print a value and then halt. The keyword **debug** is a statement that will cause the emulator to halt execution. In later projects, you will probably want to place **debug** in programs you write when you are debugging, so you can stop execution and look at variables.

If you type the **go** command, the emulator will resume execution. It will print another value and halt again. Type **go** several times, causing **bar** to call itself recursively several times. Then try the **st** command (**st** is short for “stack”). This will print out the execution stack. Try the **fr** command (short for “frame”). You should see the values of the local variables in some activation of **bar**.

Try the **up** and **down** commands. These move around in the activation stack. You can look at different activations of **bar** with the **fr** command.

### Step 11: Try Some of the Emulator Commands

Try the following commands to the emulator.
```
quit (q)
help (h)
go (g)
step (s)
t
reset
info (i)
stack (st)
frame (fr)  
up
down
```

Abbreviations are shown in parentheses.

The “step” command will execute a single machine-language instruction at a time. You can use it to walk through the execution of an assembly language program, line-by-line.

The “t” command will execute a single high-level KPL language statement at a time. Try typing “t” several times to walk through the execution of the **HelloWorld** program. See what gets printed each time you enter the “t” command.

The **i** command (short for **info**) prints out the entire state of the (virtual) BLITZ CPU. You can see the contents of all the CPU registers. There are other commands for displaying and modifying the registers.

The **h** command (short for **help**) lists all the emulator commands. Take a look at what **help** prints.

The **reset** command re-reads the executable file and fully resets the CPU. This command is useful during debugging. Whenever you wish to re-execute a program (without recompiling anything), you could always **quit** the emulator and then start it back up. The **reset** command does the same thing but is faster.

Make sure you get familiar with each of the commands listed above; you will be using them later. Feel free to experiment with other commands, too.

### The “DISK” File

The KPL virtual machine (the emulator tool, called “blitz”) simulates a virtual disk. The virtual disk is implemented using a file on the host machine and this file is called “DISK”. The programs in project 1 do not use the disk, so this file is not necessary. However, if the file is missing, the emulator will print a warning. We have included a file called “DISK” to prevent this warning. For more information, try the “format” command in the emulator.

### What to Hand In

Complete all the above steps.

To verify that you did all this, create a transcript of a terminal session showing what happened. In particular, please include the output associated with the following steps in what you hand in.

 Step 7

 Step 8

 Step 9

 Step 11

We do not need to see the other steps.

Hand in a hardcopy print-out showing what happened. If you do not know about creating a script file, check out the UNIX **script** command by typing
```
man script
```

In LARGE BLOCK LETTERS, write your full name.

Note that if you try to use a text editor while running **script**, a bunch of garbage characters may be put into the file. Please do not do this. After you have created your **script** file, it is okay to edit it to remove the entire editing session. We really don’t want to see a transcript of your editing session. Alternately, you can start and stop **script**, creating several script files and then concatenate them.

PLEASE STAPLE ALL PAGES!

### Grading for this Project

You should get familiar with KPL language and compile  BlitzTools successfully for this project.