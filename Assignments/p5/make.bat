@echo off

REM Build UserRuntime.o
asm UserRuntime.s

REM Build UserSystem
kpl UserSystem -unsafe
asm UserSystem.s

REM Build MyProgram
kpl MyProgram -unsafe
asm MyProgram.s
lddd UserRuntime.o UserSystem.o MyProgram.o -o MyProgram

REM Build TestProgram1
kpl TestProgram1 -unsafe
asm TestProgram1.s
lddd UserRuntime.o UserSystem.o TestProgram1.o -o TestProgram1

REM Build TestProgram2
kpl TestProgram2 -unsafe
asm TestProgram2.s
lddd UserRuntime.o UserSystem.o TestProgram2.o -o TestProgram2

REM Build Runtime.o
asm Runtime.s

REM Build Switch.o
asm Switch.s

REM Build System
kpl System -unsafe
asm System.s

REM Build List
kpl List -unsafe
asm List.s

REM Build BitMap
kpl BitMap -unsafe
asm BitMap.s

REM Build Kernel
kpl Kernel -unsafe
asm Kernel.s

REM Build Main
kpl Main -unsafe
asm Main.s

REM Build the OS kernel 'os'
lddd Runtime.o Switch.o System.o List.o BitMap.o Kernel.o Main.o -o os

REM Initialize the DISK and add programs
diskUtil -i
diskUtil -a MyProgram MyProgram
diskUtil -a TestProgram1 TestProgram1
diskUtil -a TestProgram2 TestProgram2
