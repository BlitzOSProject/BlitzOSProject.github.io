/**********

   hexdump  -  Harry Porter

   This program reads a file from the standard input containing
   arbitrary hex data and outputs it to the standard output
   in a format suitable for human examination.  Unlike various
   versions of 'od', it prints the bytes in exactly the order read.

   This program is most useful for examining files that contain either
   unprintable characters or characters (like CR/NL and TAB/SPACE)
   that are easily confused.

   This program runs on both big- and little-endian architectures.

   Here is an example of the using this program:

        % hexdump < hexdump.c
        00000000:  2F2A 2A2A  2A2A 2A2A  2A2A 2A0A  0A20 2020    /**********..   
        00000010:  6865 7864  756D 7020  202D 2020  4861 7272    hexdump  -  Harr
        00000020:  7920 506F  7274 6572  0A0A 2020  2054 6869    y Porter..   Thi
        00000030:  7320 7072  6F67 7261  6D20 7265  6164 7320    s program reads 
        ...

**********/

#include <stdio.h>

int row [16];
int size;

main () {
   int addr;

   /* Each execution of this loop prints a single output line. */
   addr = 0;
   readline ();
   while (size > 0) {
     putlong (addr);
     printf (":  ");
     printline ();
     addr = addr + 16;
     readline ();
   }

}



/* readline - This routine reads in the next 16 bytes from the file and
              places them in the array named 'row', setting 'size' to
              be the number of bytes read in.  Size will be less than
              16 if EOF was encountered, and may possibly be 0.  */
readline () {
int c;
  size = 0;
  c = getchar ();
  while (c != EOF) {
    row [size] = c;
    size = size + 1;
    if (size >= 16) break;
      c = getchar ();
  }
}



/* putlong - This routine is passed an integer, which it displays as 8
             hex digits.  */
putlong (i) {
  putbyt ((i>>24) & 0x000000ff);
  putbyt ((i>>16) & 0x000000ff);
  putbyt ((i>>8) & 0x000000ff);
  putbyt ((i>>0) & 0x000000ff);
}



/* printline - This routine prints the current 'row'.  */
printline () {
  int i, c;
  if (size > 0) {
    i = 0;
    while (i<16) {
      if (i < size) {
        putbyt (row[i]);
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
    for (i=0; i<size; i++) {
      c = row[i];
      if ((c>=' ') && (c <= '~')) {
        putchar (c);
      } else {
        putchar ('.');
      }
    }
    printf ("\n");
  }
}



/* putbyt - This routine is passed a byte (i.e., an integer < 256) which
          it displays as 2 hex characters.  If passed a number out of that
          range, it outputs nothing.  */
putbyt (c)
   int c;
{
  int i;
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
