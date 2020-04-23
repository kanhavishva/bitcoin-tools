#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <assert.h>
#include <string.h>

#include <string>
#include <vector>
#include <limits>

#include <base58.h>

uint64_t nprv = 0;
std::string prv = "";
FILE *PrvFile;
FILE *PrvHexFile;
int verbose = 0;

int checkB58() {
    unsigned char c;
    unsigned int i,x,r;
    for (i = 0; i < 50; i++) {
        c = getchar();

        r = 0;
        for (x=0; x<58; x++) {
            if (c == pszBase58[x]) {
                r = 1;
                break;
            }
        }

        if (r) {
            prv += c;
        } else if (i==49) {
            break;
        } else {
            prv = "";
            return -1;
        }
    }

    std::vector<unsigned char> vch;

    if (DecodeBase58Check(prv.c_str(), vch, prv.length())) {
        nprv++;

        if (PrvFile) {
            fprintf(PrvFile, "%s\n", prv.c_str());
            fflush(PrvFile);
        }

        if (PrvHexFile) {
            for (int i=1;i<33;i++)
              fprintf(PrvHexFile, "%02hhx", vch[i]);
            fprintf(PrvHexFile, "\n");
            fflush(PrvHexFile);
        }

        if (verbose>=1) {
            printf("\33[2K\rFOUND - %s - ", prv.c_str());
            for (int i=1;i<33;i++)
              printf("%02hhx", vch[i]);
            printf("\n");
        }

    } else {
        if (verbose>=2)
          printf("\33[2K\rFAILED CHECKSUM - %s\n", prv.c_str());
    }

    prv = "";
    return 0;
}

void usage(char *name) {
    printf("Usage: %s -p [FILE] -h [FILE] [OPTION]...\n\n\
 -p FILE             base58 privates \n\
 -x FILE             hex privates\n\
 -v                  verbose, show found private keys \n\
 -vv                 show failed checksum\n\
 -h                  show this help\n", name);
    exit(1);
}

int main(int argc, char **argv) {

    int i, c;
    uint64_t st = 0;

    while ((c = getopt(argc, argv, "hvp:x:")) != -1) {
        switch (c) {

            case 'p':
                PrvFile = fopen(optarg, "a");
                if (!PrvFile) {
                    fprintf(stderr, "Failed to open output private file\n");
                    exit(1);
                }
                break;

            case 'x':
                PrvHexFile = fopen(optarg, "a");
                if (!PrvHexFile) {
                    fprintf(stderr, "Failed to open output hex private file\n");
                    exit(1);
                }
                break;

            case 'v':
                verbose++;
                break;

            case 'h':
                usage(argv[0]);
                return 0;

            default:
                return 1;
        }
    }


    if (!PrvFile && !PrvHexFile) {
        fprintf(stderr, "Output file not defined\n");
        usage(argv[0]);
        exit(1);
    }

    while ((c = getchar()) != EOF) {

        if ((st % 10000000)==0 && st>0) {
            printf("\33[2K\r %luM - Found %lu private keys", st/1000000, nprv);
            fflush(stdout);
        }
        st++;

        if (c=='K') {
          prv += c;
          c = getchar();
          if (c=='w' || c=='x' || c=='y' || c=='z') {
              prv += c;
              checkB58();

          } else {
            ungetc(c, stdin);
            prv = "";
          }
        } else if (c=='L') {
          prv += c;
          c = getchar();
          if (c=='1' || c=='2' || c=='3' || c=='4' || c=='5') {
              prv += c;
              checkB58();

          } else {
            ungetc(c, stdin);
            prv = "";
          }
        } else if (c=='5') {
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

    printf("\33[2K\rCompleted. Found %lu private keys\n", nprv);
}
