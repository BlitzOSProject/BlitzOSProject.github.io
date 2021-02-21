/* endian - A program to check the host machine's byte order
**
** Harry Porter
**
** Run this program if you have any questions about whether you are
** compiling and running on a big- or little-endian machine.
**
** NOTE: Some machines can execute in both modes.  For example, the new Macs
** which contain Intel processors (e.g., MacPro, MacBookPro, ...) can compile
** and execute legacy code for the older G4/G5/PowerPC processors.  Therefore the
** output of this little program will only tell you about the environment in which
** programs are executed.
*/

#include <stdio.h>
#include <strings.h>

int i;
char * p;

main () {
  printf ("*******************************\nThis program tests whether it is running on an architecture with Big Endian or Little Endian byte ordering...\n");
  i = 0x12345678;
  printf ("The following line should print 0x12345678...\n");
  printf ("        0x%08x\n", i);
  printf ("This program stores 11, 22, 33, and 44 in sucessive bytes and then accesses it as a 4 byte integer.\n");
  p = (char *) (& i);
  *p = 0x11;
  p++;
  *p = 0x22;
  p++;
  *p = 0x33;
  p++;
  *p = 0x44;
  printf ("Big Endian machines will print 0x11223344 next, while\n");
  printf ("Little Endian machines will print 0x44332211 next...\n");
  printf ("        0x%08x\n", i);
  printf ("*******************************\n");
}
