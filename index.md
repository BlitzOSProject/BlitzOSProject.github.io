<style>
@import url('https://fonts.googleapis.com/css2?family=Lateef&display=swap');
</style>


![The BLITZ Home Page - LOGO](logo.jpg)  

## Overview

The BLITZ System is a collection of software designed to support a university-level course on Operating Systems. Over the course of one or two terms, students will implement a small, but complete, operating system kernel. The BLITZ software provides the framework for these student projects.

## Courses

<div dir="rtl" style="font-family: 'Lateef', cursive;font-size:1.2em" markdown="1">


### دانشگاه صنعتی شریف - بهار ۱۴۰۰ - سیستم عامل - دکتر فروغمند
##### هر فاز از پروژه ۱ نمره دارد.
#### فاز نخست پروژه
 - **توضیحات:** [اینجا](courses/sharif-1400-02/phase-one/persian-summary)
 - **تاریخ انتشار:** ۵ اسفند ۱۳۹۹
 - **ویدئو‌ی کلاس حل تمرین:** [اینجا](https://aparat.com/v/gy8MD)
 - **مهلت تحویل**: یک هفته

#### فاز دوم پروژه
 - **توضیحات:** [اینجا](courses/sharif-1400-02/phase-two/persian-summary)
 - **تاریخ انتشار:** ۱۸ اسفند ۱۳۹۹
 - **ویدئو‌ی کلاس حل تمرین:** [اینجا](https://www.aparat.com/v/Z7OWG)
 - **مهلت تحویل**: سه هفته

#### فاز سوم پروژه
 - **توضیحات:** [اینجا](courses/sharif-1400-02/phase-three/persian-summary)
 - **تاریخ انتشار:** ۱۵ فروردین ۱۴۰۰
 - **ویدئو‌ی کلاس حل تمرین:** بزودی
 - **مهلت تحویل**: سه هفته


</div>

## Documentation

### [An Overview of the BLITZ System](docs/overview)
This document gives an introduction to and overview of the BLITZ system.
  

### An Overview of the BLITZ Computer Hardware  
This document introduces the architecture of the emulated CPU and I/O devices. [html](BlitzDoc/HardwareOverview.htm)  [pdf](BlitzDoc/HardwareOverview.pdf)

### The BLITZ Architecture  
This document describes the BLITZ processor hardware. It includes information about the CPU registers, the instruction set architecture, and the BLITZ assembly language. [html](BlitzDoc/BlitzArchitecture.htm)  [pdf](BlitzDoc/BlitzArchitecture.pdf)  
  

### Example BLITZ Assembly Program  
This is a compete, stand-alone BLITZ assembly program. This program can serve as a test of the BLITZ tools and an introduction to using the emulator. [html](BlitzDoc/ExamplePgm.htm)  [pdf](BlitzDoc/ExamplePgm.pdf)  
  

### BLITZ Instruction Set  
This document contains detailed information on each of the BLITZ machine instructions. It will be of interest primarily to assembly language programmers. [html](BlitzDoc/InstructionSet.htm)  [pdf](BlitzDoc/InstructionSet.pdf)  
  

### The BLITZ Emulator  
This document describes the BLITZ virtual machine emulator and debugger. It shows how to run a BLITZ program and describes all of the available debugging commands. It also includes detailed information about the disk and terminal I/O devices. [html](BlitzDoc/Emulator.htm)  [pdf](BlitzDoc/Emulator.pdf)  
  

### An Overview of KPL, A Kernel Programming Language  
This document describes the "KPL" high-level programming language. [html](BlitzDoc/KPLOverview.htm)  [pdf](BlitzDoc/KPLOverview.pdf) 
  

### Context-Free Grammar of KPL  
This document contains a formal specification of the grammar of KPL and can be used as a quick reference, for syntactical questions. [html](BlitzDoc/Syntax.htm)  [pdf](BlitzDoc/Syntax.pdf)  
  

### The Format of BLITZ Object and Executable Files  
This document describes the files produced by the assembler and the linker. This document may be of some interest to students, but is not necessary. [html](BlitzDoc/ObjectFileFormat.htm)  [pdf](BlitzDoc/ObjectFileFormat.pdf)  
  

### BLITZ Tools: Help Information  
Each of the BLITZ tools will produce some "help" information; this document collects such information from each of the tools. This document also includes detailed information about the syntax of BLITZ assembly language programs. [html](BlitzDoc/HelpDisplays.htm)  [pdf](BlitzDoc/HelpDisplays.pdf) 
  

### BLITZ Instruction Set - Sorted Lists  
This document lists all BLITZ machine instructions. This list is first ordered alphabetically by instruction name and then listed numerically by op-code. It will be of interest primarily to assembly language programmers. [html](BlitzDoc/InstSet-SortedLists.htm)  [pdf](BlitzDoc/InstSet-SortedLists.pdf) 
  

### BLITZ Misc. Technical Notes  
This document contains a number of miscellaneous comments and notes. This document may be of some interest to students, but is not necessary. [html](BlitzDoc/TechnicalNotes.htm)  [pdf](BlitzDoc/TechnicalNotes.pdf)  
  

### The BLITZ Assembler  
This document describes the BLITZ assembler tool. This document may be of some interest to students, but is not necessary. [html](BlitzDoc/BlitzAssembler.htm)  [pdf](BlitzDoc/BlitzAssembler.pdf)  
  

### The Thread Scheduler and Concurrency Control Primitives  
This document describes the workings of a kernel thread scheduler using, as a specific example, the scheduler in the BLITZ system. It is aimed at someone just learning about kernel concepts. Familiarity with the BLITZ system is not assumed and is not needed to understand this document. This document is not required by students working on the BLITZ kernel project, but it may be helpful to those working on projects 2 and 3. [html](BlitzDoc/ThreadScheduler.htm)  [pdf](BlitzDoc/ThreadScheduler.pdf)

## The Operating System Project

### Project 1: Introduction to the BLITZ Tools
[document](project/1) - [html](OSProject/p1/proj1.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p1) - [<s>old pdf</s>](OSProject/p1/proj1.pdf)
### Project 2: Threads and Interprocess Communication
document - [html](OSProject/p2/proj2.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p2) - [<s>old pdf</s>](OSProject/p2/proj2.pdf)
### Project 3: Barbers and Gamblers
document - [html](OSProject/p3/proj3.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p3) - [<s>old pdf</s>](OSProject/p3/proj3.pdf)
### Project 4: Kernel Resource Management
document - [html](OSProject/p4/proj4.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p4) - [<s>old pdf</s>](OSProject/p4/proj4.pdf)
### Project 5: User-Level Processes
document - [html](OSProject/p5/proj5.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p5) - [<s>old pdf</s>](OSProject/p5/proj5.pdf)
### Project 6: Multiprogramming with Fork
document - [html](OSProject/p6/proj6.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p6) - [<s>old pdf</s>](OSProject/p6/proj6.pdf)
### Project 7: File-Related Syscalls
document - [html](OSProject/p7/proj7.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p7) - [<s>old pdf</s>](OSProject/p7/proj7.pdf)
### Project 8: The Serial I/O Device Driver
document - [html](OSProject/p8/proj8.htm) - [files](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/OSProject/p8) - [<s>old pdf</s>](OSProject/p8/proj8.pdf)

## The BLITZ Tools

### Executables for Sun / Solaris
[Blitz/BlitzBin/Sun](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzBin/Sun)
### Executables for Linux Ubuntu (64-bit)
[Blitz/BlitzBin/Ubuntu64](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzBin/Ubuntu64)
### Executables for Mac OS X (Intel)
[Blitz/BlitzBin/MacIntel](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzBin/MacIntel)
### Executables for Mac OS X (PPC)
[Blitz/BlitzBin/MacPPC](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzBin/MacPPC)
### Source Code
[Blitz/BlitzSrc](https://github.com/BlitzOSProject/BlitzOSProject.github.io/tree/main/BlitzSrc)

## Information for Instructors

### Notes to Instructors  
This document is the starting point for any instructor who is considering using the system in a classroom setting and should be be read first. [html](InstructorInfo/NotesToInstructors.htm)
*   [Directory with additional support material](InstructorInfo)  

## About the Author

Harry H. Porter III, Ph.D.  
Computer Science Department  
Portland State University  
  
Short Bio: [click here](http://web.cecs.pdx.edu/~harry/Blitz/ShortBio.html)  
Harry's Website: [www.cs.pdx.edu/~harry](http://www.cs.pdx.edu/~harry)  
Email: [porter@pdx.edu](mailto:porter@pdx.edu)  
  

- - -