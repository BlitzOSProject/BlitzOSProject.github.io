// check -- Check text file for problem characters
//
// Harry Porter -- 03/02/04
//
// This program runs though its standard input and looks for any strange characters
// such as TAB, CR, LF, BS, DEL, and non-printable characters.  It will print how many
// occurences of each it finds.
//
// This program will also print the maximum line length.  (TAB, DEL, and BS count
// as only 1 character.)
//

#define BS 8
#define CR 13
#define LF 10
#define TAB 9
#define DEL 127

#include <stdio.h>


main () {
  int lineLength = 0,
      maxLineLength = 0,
      countOfBS = 0,
      countOfCR = 0,
      countOfLF = 0,
      countOfTAB = 0,
      countOfDEL = 0,
      countOfOther = 0,
      c;

  // Each iteration looks at the next character.
  while (1) {
    c = getchar();
    if (c == EOF) {
      break;
    } else if (c == BS) {
      lineLength++;
      countOfBS++;
    } else if (c == DEL) {
      lineLength++;
      countOfDEL++;
    } else if (c == TAB) {
      lineLength++;
      countOfTAB++;
    } else if (c == LF) {
      if (lineLength > maxLineLength) {
        maxLineLength = lineLength;
      }
      lineLength = 0;
      countOfLF++;
    } else if (c == CR) {
      if (lineLength > maxLineLength) {
        maxLineLength = lineLength;
      }
      lineLength = 0;
      countOfCR++;
    } else if ((c >= ' ') && (c < DEL)) {
      lineLength++;
    } else {
      lineLength++;
      countOfOther++;
    }
  }

  // Print statistics.
  printf ("The maximum line length is %d\n", maxLineLength);
  printf ("The number of TAB characters is %d\n", countOfTAB);
  printf ("The number of CR characters is %d\n", countOfCR);
  printf ("The number of LF characters is %d\n", countOfLF);
  printf ("The number of BS characters is %d\n", countOfBS);
  printf ("The number of DEL characters is %d\n", countOfDEL);
  printf ("The number of other non-printable characters is %d\n", countOfOther);

}
