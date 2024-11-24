
@REM Type 'make.bat' to build the Project 2 code.
@REM It will execute the following commands as needed, based on files'
@REM most-recent-update times.


asm Runtime.s
asm Switch.s
kpl System -unsafe
asm System.s
kpl List -unsafe
asm List.s
kpl BitMap -unsafe
asm BitMap.s
kpl Kernel -unsafe
asm Kernel.s
kpl Main -unsafe
asm Main.s
lddd Runtime.o Switch.o System.o List.o BitMap.o Kernel.o Main.o -o os
blitz -g os