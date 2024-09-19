
@echo off


echo Choose an option:
echo 1. Assemble .s files (Assembly)
echo 2. Assemble .c kpl program (KPL)
set /p choice="Enter your choice (1 or 2): "

if "%choice%"=="1" (
    echo You chose Assembly.

    set /P "assembly_file=Enter the name of the .s file to assemble (include extension): "
    if not exist "%assembly_file%" (
        echo File "%assembly_file%" does not exist.
        pause
        exit /b
    )

    echo Assembling "%assembly_file%"...
    asm.exe "%assembly_file%" -o "%assembly_file%.o"
    lddd.exe "%assembly_file%.o"

) else if "%choice%"=="2" (
    echo You chose KPL.
    set /P "kpl_program=Enter the name of the KPL program (without extension): "

    echo Assembling KPL program "%kpl_program%"...
    kpl.exe "%kpl_program%"
    asm.exe "%kpl_program%"
    asm.exe "System.s"
    asm.exe "Runtime.s"
    lddd.exe "System.o Runtime.o %kpl_program%"
) else (
    echo Invalid choice.
    pause
    exit /b
)

blitz.exe a.out 

pause
endlocal
