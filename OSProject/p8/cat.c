code cat

  -----------------------------------
  ----                           ----
  ----    BLITZ "cat" Program    ----
  ----                           ----
  -----------------------------------

-----------------------------  main  ---------------------------------
  const stdin  = 0
        stdout = 1

  const BUFF_MAX = 30

  var buffer: array [BUFF_MAX] of char

  function main ()
    --
    -- This is a simplified version of the 'cat' program found in Unix.
    --
      var i: int
      *((& buffer) asPtrTo int) = BUFF_MAX
      while true
        i = Sys_Read (stdin, &buffer[0], BUFF_MAX)
        if i <= 0
          break
        endIf
        i = Sys_Write (stdout, &buffer[0], i)
      endWhile
      Sys_Close (stdout)

    endFunction

endCode
