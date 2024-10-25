
@REM Type 'make.bat' to build the Project 2 code.
@REM It will execute the following commands as needed, based on files'
@REM most-recent-update times.


asm Runtime.s
asm Switch.s
kpl System -unsafe
asm System.s
kpl List -unsafe
asm List.s
kpl Thread -unsafe
asm Thread.s
kpl Synch
asm Synch.s
kpl Main -unsafe
asm Main.s
lddd System.o List.o Thread.o Switch.o Synch.o Main.o Runtime.o -o os
blitz -g os