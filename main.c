#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

FILE *fp;

int BPB_BytsPerSec = 0;
int BPB_SecPerClus = 0;
int BPB_RsvdSecCnt = 0;
int BPB_NumFATs = 0;
unsigned long BPB_FATSz32 = 0;
unsigned long BPB_RootClus = 0;
unsigned long Signature_word = 0;
unsigned char FAT[512];
unsigned  long curr_directory;
int FATOffset;
int ThisFATSecNum;
int ThisFATEntOffset;
char PathName[30][15];
int PathPosition = 0;
int N_Previous[30];
int NPosition = 0;
int FirstDataSector;



struct Directory {
    char names[8][256];
    int next;
}Directory;

char * getInput();//Receives input from the user
int execute(char *input);//Main function of the program, acts as the shell receiving commands
void openISO();
void init();
void info();
void showPath();
void listDirectory(struct Directory Root);
char** tokenize(char *input);
struct Directory Root;
void changeDirectory(char* directory);
void GetDirectory(int N);
void initPath();
void listInformation(char* info);
void get(char *dirname);
unsigned long N;



// readline reads and returns one line of text from the file descriptor,
// stopping at the first newline or error. It can only handle lines with
// 255 characters or less. Upon error, it prints a friendly
// error message and returns whatever text has already been read.
char *readline(int fd) {
    char *s = malloc(256); // assumes line will be 255 characters or less
    int n = 0;
    while (1) {
        int r = read(fd, s + n, 1);
        if (r == 0) {
            printf("Line didn't end, but there is no more data available in file %d\n", fd);
            s[n] = '\0'; // add the terminating zero and return the string
            return s;
        } else if (r < 0) {
            printf("There was a problem reading from file %d: error number %d\n", fd, n);
            s[n] = '\0'; // add the terminating zero and return the string
            return s;
        } else if (s[n] == '\n') {
            s[n] = '\0'; // add the terminating zero and return the string
            return s;
        } else if (n == 255) {
            printf("Already read 255 characters, but line didn't end yet in file %d\n", fd);
            s[n] = '\0'; // add the terminating zero and return the string
            return s;
        } else {
            n++;
        }
    }
}

int main(int ac, char **av) {
    setbuf(stdout, NULL);

    initPath();
    openISO();
    printf("Successfully opened the .img file\n");
    printf("***********************************************************\n");
    printf("//TODO: Get function not fully working\n");
    printf("//TODO: Going to the next Cluster is a little iffy\n");
    printf("***********************************************************\n\n");
    printf("If you are lost type \"help\"\n");
    init();
    int exit = 0;
    ThisFATSecNum = BPB_RsvdSecCnt + (FATOffset / BPB_BytsPerSec);

    fseek( fp, ThisFATSecNum * BPB_BytsPerSec, SEEK_SET);
    fread(FAT, sizeof(FAT), 1, fp);

    curr_directory = ((BPB_FATSz32 * BPB_RootClus) + BPB_RsvdSecCnt) * BPB_BytsPerSec;
    N_Previous[NPosition] = BPB_RootClus;


    while (exit == 0) {
        GetDirectory(N_Previous[NPosition]);
        exit = execute(getInput());
    }
    return(0);
}

char * getInput() {
    showPath();
    char* input = readline(0);
    return input;
}

int execute(char *input) {

    char** token = tokenize(input);
    if (token[0] == NULL) {
        return 0;
    } else if (token[1] == NULL) { // When there is only one user input
        if (strcmp(token[0], "info") == 0) {
            info();
        } else if (strcmp(token[0], "quit") == 0) {
            printf("You have ended the program");
            fclose(fp);
            return 1;
        } else if(strcmp(token[0], "ls") == 0) {
            listDirectory(Root);
        } else if (strcmp(token[0], "help") == 0) {
            printf("info\nls\nls -l\ncd <Directory Name>\nget <File Name>\nquit\n");
        }
    } else if (token[2] == NULL) { // When there are two user inputs
        if (strcmp(input, "cd") == 0) {
            changeDirectory(token[1]);
        } else if(strcmp(token[0], "ls") == 0) {
            listInformation(token[1]);
        } else if(strcmp(token[0], "get") == 0) {
            printf("Warning: This function is not fully working\n");
            printf("I am pretty sure I am close but for some reason it did not work\n");
            get(token[1]);
        }
    }
    return 0;
}

void openISO() {
    char *input = getInput();
    fp = fopen(input, "rb");
    //fp = fopen("C:\\Users\\itaru\\Desktop\\OS\\FAT32\\fat32.img", "rb");

    if(fp == NULL) {
        printf("Error in opening the image\n");
        printf("Please Enter the full path name.\n");
        printf("Example: C:\\Users\\itaru\\Desktop\\fat32.img\n");
        fclose(fp);
        openISO();
    }
}

void init() {

    unsigned char buffer[512];
    fseek( fp, 0, SEEK_SET);
    fread(buffer, sizeof(buffer), 1, fp);

    //find the Bytes per sector
    //offset(byte) 11
    //size(byte) 2
    for(int i = 11+1; i >= 11; i--){
        BPB_BytsPerSec = BPB_BytsPerSec << 8;
        BPB_BytsPerSec = BPB_BytsPerSec + buffer[i];
    }

    BPB_SecPerClus = buffer[13];

    //find the BPB_RsvdSecCnt
    //offset(byte) 14
    //size(byte) 2
    for(int i = 14+1; i >= 14; i--) {
        BPB_RsvdSecCnt = BPB_RsvdSecCnt << 8;
        BPB_RsvdSecCnt = BPB_RsvdSecCnt + buffer[i];
    }

    BPB_NumFATs = buffer[16];

    //find BPB_FATSz32
    //offset(byte) 36
    //size(byte) 4
    for(int i = 36+3; i >= 36; i--) {
        BPB_FATSz32 = BPB_FATSz32 << 8;
        BPB_FATSz32 = BPB_FATSz32 + buffer[i];
    }

    //find BPB_RootClus
    //offset(byte) 44
    //size(byte) 4
    for(int i = 44+3; i >= 44; i--) {
        BPB_RootClus = BPB_RootClus << 8;
        BPB_RootClus = BPB_RootClus + buffer[i];
    }

    //find Signature_word
    //offset(byte) 510
    //size(byte) 2
    for(int i = 510+1; i >= 510; i--) {
        Signature_word = Signature_word << 8;
        Signature_word = Signature_word + buffer[i];
    }

    FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
}

void info() {
    printf("~~~~~~~ Master Boot Record ~~~~~~~\n");
    printf("Bytes Per Sector: %d\n", BPB_BytsPerSec);
    printf("Sector Per Cluster: %d\n", BPB_SecPerClus);
    printf("Number of Reserved Sectors: %lu\n", BPB_RsvdSecCnt);
    printf("Number of FATs: %d\n", BPB_NumFATs);
    printf("Sectors Per FAT: %lu\n", BPB_FATSz32);
    printf("Root Cluster: %lu\n", BPB_RootClus);
    printf("Signature Word: %lu or %X\n", Signature_word, Signature_word);
}

void listInformation(char* info){
    GetDirectory(N);
    if (strcmp("-l", info) == 0) {
        unsigned char buffer[64];
        for (int i = 0; i < 8; i++) {
            if (strcmp(Root.names[i], "\0") == 0) {
                break;
            } else {
                unsigned long time = 0;
                fseek(fp, curr_directory + (64 * i), SEEK_SET);
                fread(buffer, sizeof(buffer), 1, fp);
                printf("%s ", Root.names[i]);
                for(int j = 49; j >= 48; j--) {
                    time = time << 8;
                    time = time + buffer[j];
                }
                int day = 0;
                for (int k = 0; k < 5; k++) {
                    if (time & 1) {
                        day = day + pow(2, k);
                    }
                    time >>= 1;
                }
                int month = 0;
                for (int h = 0; h < 4; h++) {
                    if (time & 1) {
                        month = month + pow(2, h);
                    }
                    time >>= 1;
                }
                int year = 0;
                for (int l = 0; l < 7; l++) {
                    if (time & 1) {
                        year = year + pow(2, l);
                    }
                    time >>= 1;
                }
                time = 0;
                for(int j = 47; j >= 46; j--) {
                    time = time << 8;
                    time = time + buffer[j];
                }
                int seconds = 0;
                for (int k = 0; k < 5; k++) {
                    if (time & 1) {
                        seconds = seconds + pow(2, k);
                    }
                    time >>= 1;
                }
                int minutes = 0;
                for (int h = 0; h < 6; h++) {
                    if (time & 1) {
                        minutes = minutes + pow(2, h);
                    }
                    time >>= 1;
                }
                int hours = 0;
                for (int l = 0; l < 5; l++) {
                    if (time & 1) {
                        hours = hours + pow(2, l);
                    }
                    time >>= 1;
                }
                printf("%d-%d-%d ", year+1980, month, day);
                printf("%d:%d:%d\n", hours, minutes, seconds);
            }
        }
    }
}

void listDirectory(struct Directory Root) {
    GetDirectory(N);
    for (int i = 0; i < 8; i++) {
        printf("%s ", Root.names[i]);
    }
    printf("\n");

}

void initPath() {
    strcat(PathName[0],"~");
    for (int i = 1; i < 30; i++) {
        PathName[i][0] = '\0';
    }
}

void showPath() {
    printf("%s", PathName[PathPosition]);
    printf("/$");
}

char** tokenize(char *input) {
    char *w = strtok(input, " "); // begins process of splitting
    char **word = (char **)malloc(10 * sizeof(char *));
    int i = 0;

    while (w != NULL) {
        word[i] = strdup(w);
        w = strtok(NULL, " ");
        i++;
    }

    for (int j = i; j < 10; j++) {
        word[j] = NULL;
    }
    return word;
}

void changeDirectory(char* directory) {
    if (strcmp("..", directory) == 0) {
        if (PathPosition != 0) {
            PathName[PathPosition][0]= '\0';
            PathPosition--;
            NPosition--;
            N = N_Previous[NPosition];
            curr_directory = (((N - 2) * BPB_SecPerClus) + FirstDataSector)*BPB_BytsPerSec;
            return;
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (strcmp(Root.names[i], directory) == 0) {
                unsigned char buffer[64];
                fseek(fp, curr_directory + (64 * i), SEEK_SET);
                fread(buffer, sizeof(buffer), 1, fp);
                for(int i = 53; i >= 52; i--) {
                    N = N << 8;
                    N = N + buffer[i];
                }
                for(int i = 59; i >= 58; i--) {
                    N = N << 8;
                    N = N + buffer[i];
                }
                NPosition++;
                N_Previous[NPosition] = N;
                PathPosition++;
                strcat(PathName[PathPosition], PathName[PathPosition-1]);
                strcat(PathName[PathPosition], "/");
                strcat(PathName[PathPosition], directory);
                curr_directory = (((N - 2) * BPB_SecPerClus) + FirstDataSector)*BPB_BytsPerSec;
                return;
            }
        }
        printf("Error: not a valid input\n");
    }
}

void get(char *directory) {
    for (int i = 0; i < 8; i++) {
        if (strcmp(Root.names[i], directory) == 0) {
            //unsigned char buffer[64];
            //fseek(fp, curr_directory + (64 * i), SEEK_SET);
            //fread(buffer, sizeof(buffer), 1, fp);
            FILE *file = fopen ("C:\\Users\\itaru\\Desktop\\data", "w");
            file = fopen("Text.txt", "rw");


            /* fopen() return NULL if last operation was unsuccessful */
            if(file == NULL)
            {
                /* File not created hence exit */
                printf("Unable to create file.\n");
                exit(EXIT_FAILURE);
            }

            //int size = 512;
            //unsigned char *ptr = malloc(size);
            //fread(ptr, size, 1, file);
            //fwrite(ptr, size, 1, file);
            fclose(file);
            return;
        }
    }
    printf("File Does not Exist\n");
}

void GetDirectory(int N) {
    FATOffset = N * 4;
    ThisFATEntOffset = FATOffset % BPB_BytsPerSec;

    unsigned  long temp = curr_directory;

    for (int i = 0; i < 16; i++) {
        Root.names[i/2][0] = '\0';
        unsigned char buffer[32];
        fseek(fp, temp, SEEK_SET);
        fread(buffer, sizeof(buffer), 1, fp);
        if (buffer[11] != 0x00 && buffer[11] != 0x0F) {
            for (int j = 0; j < 8; j++) {
                if (buffer[j] != 0x20) {
                    strncat(Root.names[i/2], &buffer[j], 1);
                } else {
                    break;
                }
            }
            if (buffer[8] != 0x20) {
                strncat(Root.names[i/2], ".", 1);
                for(int j = 8; j < 11; j++ ) {
                    strncat(Root.names[i/2], &buffer[j], 1);
                }
            }
        }
        temp += 32;
    }

    if (FAT[ThisFATEntOffset] != 0xf8) {
        for(int i = ThisFATEntOffset+4; i >= ThisFATEntOffset; i--) {
            Root.next = Root.next << 8;
            Root.next = Root.next + FAT[i];
        }
    } else {
        Root.next = 0;
    }
}

