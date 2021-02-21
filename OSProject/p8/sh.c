code sh

  -----------------------------------
  ----                           ----
  ----    BLITZ Shell Program    ----
  ----                           ----
  -----------------------------------

  const stdin  = 0
        stdout = 1
        EOF = '\xFF'

  const COMMAND_MAX = 30

  var commandLine: array [COMMAND_MAX] of char

-----------------------------  main  ---------------------------------

  function main ()
    --
    -- This is a simplified version of the shell program found in Unix.
    --

      var pid, i: int
          nextPos, len, start: int
          kind: char        -- will be either ' ', '<', '\n', or '>'
          progName, inFile, outFile: array [COMMAND_MAX] of char
          gotCommand, gotInFile, gotOutFile: bool

      commandLine = new array of char { COMMAND_MAX of ' ' }
      progName = commandLine
      inFile = commandLine
      outFile = commandLine

      Print ("Welcome to the BLITZ Shell Program.\n")
      Print ("  Type 'exit' to terminate.\n")
      Print ("  Type 'cat < help' for more info.\n")

      while true

        -- Print prompt...
        Print ("% ")

        -- Read in a command line...
        gotCommand = false
        gotInFile = false
        gotOutFile = false
        ReadLine ()

        -- Run through the command line and find "progName", "inFile", & "outFile"...
        nextPos = 0
        while true
          FindNextWord (&start, &nextPos, &len, &kind)
          if kind == '\n'
            break
          elseIf kind == ' ' && !gotCommand
            GetWord (& progName, start, len)
            gotCommand = true
          elseIf kind == '<' && !gotInFile
            FindNextWord (&start, &nextPos, &len, &kind)
            if kind == ' '
              GetWord (&inFile, start, len)
              gotInFile = true
            else
              Print ("sh: Missing name for redirect.\n")
              gotCommand = false
              break
            endIf
          elseIf kind == '>' && !gotOutFile
            FindNextWord (&start, &nextPos, &len, &kind)
            if kind == ' '
              GetWord (&outFile, start, len)
              gotOutFile = true
            else
              Print ("sh: Missing name for redirect.\n")
              gotCommand = false
              break
            endIf
          else
            Print ("sh: Command line problems\n")
            gotCommand = false
            break
          endIf
        endWhile

        -- Make sure we got a command...
        if !gotCommand
          continue
        endIf
        
        -- Check for exit...
        if StrEqual (&progName, "exit")
          Print ("[Shell process terminated]\n")
          Sys_Exit (0)
        endIf

        -- Execute the program and wait for it to terminate...
        pid = Sys_Fork ()
        if pid == 0
          if gotInFile
            Sys_Close (stdin)
            i = Sys_Open (&inFile)
            if i != stdin
              Print ("sh: No such input file.\n")
              Sys_Exit (-1)
            endIf
          endIf
          if gotOutFile
            Sys_Close (stdout)
            i = Sys_Open (&outFile)
            if i != stdout
              i = Sys_Open ("terminal")
              Print ("sh: No such output file.\n")
              Sys_Exit (-1)
            endIf
          endIf
          i = Sys_Exec (&progName)
          Print ("sh: Command not found.\n")
          Sys_Exit (-1)
        else
          i = Sys_Join (pid)
        endIf

      endWhile

    endFunction

-----------------------------  ReadLine  ---------------------------------

  function ReadLine ()
    --
    -- This function reads in a line of input and it moves the character into
    -- the commandLine buffer, echoing characters as they are typed.  It will
    -- make sure that a \n goes into the commandLine buffer, even if the
    -- user tries to overflow the buffer.
    --
      var i: int
          ch: char
      while true
        ch = GetChar ()
        if ch == EOF
          -- Ignore EOF
        elseIf ch == '\n'
          PutChar (ch)
          commandLine [i] = ch
          i = i + 1
          return
        elseIf ch == '\b' || ch == '\x7f'  -- cntrl-H or "DEL" char on MAC keyboard
          if i > 0
            Print ("\b \b")
            i = i - 1
          else
            PutChar ('\a')  -- bell/alert
          endIf
        else
          PutChar (ch)
          commandLine [i] = ch
          i = i + 1
        endIf
        if i >= COMMAND_MAX-1
          commandLine [i] = '\n'
          PutChar ('\n')
          return
         endIf
      endWhile
    endFunction

-----------------------------  Print  ---------------------------------

  function Print (str: String)
    --
    -- This function is passed a string.  It prints it to stdout.
    --
      var i: int
      i = Sys_Write (stdout, &str[0], str arraySize)     
    endFunction

-----------------------------  GetChar  ---------------------------------

  function GetChar () returns char
    --
    -- This function reads a single character from stdin and returns it.
    --
      var c: char
          i: int
      i = Sys_Read (stdin, &c, 1)
      if i == 0
        return EOF
      elseIf i < 0
        print ("Problems in GetChar\n")
      endIf
      return c    
    endFunction

-----------------------------  PutChar  ---------------------------------

  function PutChar (c: char)
    --
    -- This function writes a single character to stdout.
    --
      var i: int
      i = Sys_Write (stdout, &c, 1)
      if i < 0
        print ("Problems in PutChar\n")
      endIf

    endFunction

-----------------------------  FindNextWord  ---------------------------------

  function FindNextWord (start, pos, len: ptr to int, kind: ptr to char)
    --
    -- This function looks in the command line buffer and finds the next word.
    -- It updates its parameters.
    --
      var ch: char

      -- Find the next non-blank...
      while commandLine[*pos] == ' '
        *pos = *pos + 1
      endWhile

      *start = *pos

      -- If it is < or >...
      if commandLine[*pos] == '<' ||
         commandLine[*pos] == '>'
        *kind = commandLine[*pos]
        *pos = *pos + 1
        *len = 1
        return
      endIf

      -- If it is \n...
      if commandLine[*pos] == '\n'
        *kind = '\n'
        *len = 0
        return
      endIf

      -- Else it must be a normal word...
      *kind = ' '  -- Normal word
      while true
        ch = commandLine[*pos]
        if ch == ' ' ||
           ch == '\n' ||
           ch == '<' ||
           ch == '>'
          break
        endIf
        *pos = *pos + 1
      endWhile
      *len = *pos - *start
 
    endFunction

-----------------------------  GetWord  ---------------------------------

  function GetWord (str: String, start, len: int)
    --
    -- This function moves "len" characters from "commandLine" (starting
    -- at "start") into the string "str".  It assumes that "str" is large
    -- enough.
    --
      MemoryCopy ((&str[0]) asInteger, (&commandLine[start]) asInteger, len)
      *(str asPtrTo int) = len
    endFunction

endCode
