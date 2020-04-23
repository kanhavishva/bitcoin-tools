#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>



void usage(char *name) {
    printf("Usage: %s -h [FILE]\n\n\
 -x FILE             hex privates\n\
 -q FILE             quiet mode, don't show status\n\
 -h                  show this help\n", name);
    exit(1);
}

int main(int argc, char **argv) {

    int i, c, w;
    int quiet = 0;
    uint64_t st = 0;
    uint64_t nprv = 0;
    const char *PrvHexFileName;
    FILE *PrvHexFile;

    while ((c = getopt(argc, argv, "hqx:")) != -1) {
        switch (c) {

            case 'x':
                PrvHexFileName = optarg;
                break;

            case 'q':
                quiet = 1;
                break;

            case 'h':
                usage(argv[0]);
                return 0;

            default:
                return 1;
        }
    }

    FILE* outstatus = stderr;

    if (PrvHexFileName) {
        outstatus = stdout;
        PrvHexFile = fopen(PrvHexFileName, "a");
        if (!PrvHexFile) {
            fprintf(stderr, "Failed to open output hex private file\n");
            exit(1);
        }
    }

    while ((c = getchar()) != EOF) {

        if (quiet==0 && (st % 10000000)==0 && st>0) {
            fprintf(outstatus, "\33[2K\r %luM - Found %lu private keys", st/1000000, nprv);
            fflush(outstatus);
        }
        st++;

        if (c == 0x04) {
            if ((c = getchar()) == 0x20) {
                nprv++;

                if (PrvHexFile) {
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

    fprintf(outstatus, "\33[2K\rCompleted. Found %lu private keys\n", nprv);
}
