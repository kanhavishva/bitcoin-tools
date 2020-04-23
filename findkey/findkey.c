#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

uint64_t nprv = 0;
const char *PrvHexFileName;
FILE *PrvHexFile;



void usage(char *name) {
    printf("Usage: %s -p [FILE] -h [FILE] [OPTION]...\n\n\
 -x FILE             hex privates\n\
 -h                  show this help\n", name);
    exit(1);
}

int main(int argc, char **argv) {

    int i, c;
    uint64_t st = 0;

    while ((c = getopt(argc, argv, "hx:")) != -1) {
        switch (c) {

            case 'x':
                PrvHexFileName = optarg;
                break;

            case 'h':
                usage(argv[0]);
                return 0;

            default:
                return 1;
        }
    }

    FILE* outstatus = stderr;

    if(PrvHexFileName) {
        outstatus = stdout;
        PrvHexFile = fopen(PrvHexFileName, "a");
        if(!PrvHexFile) {
            fprintf(stderr, "Failed to open output hex private file\n");
            exit(1);
        }
    }

    while ((c = getchar()) != EOF) {

        if ((st % 10000000)==0 && st>0) {
            fprintf(outstatus, "\33[2K\r %luM - Found %lu private keys", st/1000000, nprv);
            fflush(outstatus);
        }
        st++;

        if (c == 0x04) {
            if ((c = getchar()) == 0x20) {
                nprv++;

                if(PrvHexFile) {
                    for (i = 0; i < 32; i++)
                        fprintf(PrvHexFile, "%02x", getchar());
                    fprintf(PrvHexFile, "\n");
                    fflush(PrvHexFile);
                } else {
                    for (i = 0; i < 32; i++)
                        printf("%02x", getchar());
                    printf("\n");
                    fflush(stdout);
                }
            } else {
                ungetc(c, stdin);  /* push back in case it's 0x04 */
            }
        }
    }

    printf("\33[2K\rCompleted. Found %lu private keys\n", nprv);
}
