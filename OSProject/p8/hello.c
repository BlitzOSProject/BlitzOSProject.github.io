code hello

  -----------------------------------
  ----                           ----
  ----    The "hello" Program    ----
  ----                           ----
  -----------------------------------

-----------------------------  main  ---------------------------------
  const stdin  = 0
        stdout = 1
        message = "Hello, world!\n"

  function main ()
    --
    -- This is the prototypical user-level program.  It can be executed as
    -- a command by typing the name of the executable at the shell prompt.
    --
      var ignore: int

      ignore = Sys_Write (stdout, &message[0], message arraySize)

    endFunction

endCode
