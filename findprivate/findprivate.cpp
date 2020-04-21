/*
  REGEX (K[wxyz]|L[0-5]|5[KHJ])[1-9A-HJ-NP-Za-km-z]{49,50}
  g++ findprivate.cpp -I. -lcrypto -o findprivate
*/

#include <stdio.h>
#include <stdint.h>

#include <assert.h>
#include <string.h>

#include <string>
#include <vector>
#include <limits>

#include <base58.h>

std::string prv = "";

int checkB58() {
    unsigned char c;
    unsigned int i,x,r;
    for (i = 0; i < 50; i++) {
        c = getchar();

        r = 0;
        for(x=0; x<58; x++) {
            if (c == pszBase58[x]) {
                r = 1;
                break;
            }
        }

        if(r) {
            prv += c;
        } else if (i==49) {
            break;
        } else {
            //printf("BAD: %s\n", prv.c_str()); //DEBUG
            prv = "";
            return -1;
        }

    }

    std::vector<unsigned char> vch;

    if(DecodeBase58Check(prv.c_str(), vch, prv.length())) {
        printf("FOUND - %s - ", prv.c_str());

        // Print private key in hex
        for (int i=1;i<33;i++)
          printf("%02hhx", vch[i]);
        printf("\n");

    } else {
        printf("FAILED CHECKSUM - %s\n", prv.c_str());
    }

    prv = "";
    fflush(stdout);

    return 0;
}

int main() {
  int i, c;

  while ((c = getchar()) != EOF) {
    if (c == 'K') {
      prv += c;
      c = getchar();
      if (c=='w' || c=='x' || c=='y' || c=='z') {
          prv += c;
          checkB58();

      } else {
        ungetc(c, stdin);
        prv = "";
      }
    } else if (c == 'L') {
      prv += c;
      c = getchar();
      if (c=='0' || c=='1' || c=='2' || c=='3' || c=='4' || c=='5') {
          prv += c;
          checkB58();

      } else {
        ungetc(c, stdin);
        prv = "";
      }
    } else if (c == '5') {
      prv += c;
      c = getchar();
      if (c=='K' || c=='H' || c=='J') {
          prv += c;
          checkB58();

      } else {
        ungetc(c, stdin);
        prv = "";
      }
    }
  }
}
