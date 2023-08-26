#ifndef READER_H
#define READER_H

//reading macro results code
#define OK 0
#define NO_INPUT 1
#define TOO_LONG 2
#define COMMAND_UNKNOWN -1

//reading functions definitions
int readLine(char*, size_t,bool);
void clearStdin();
char** formatCommand(char *, const char *, int*);
void freeAllocatedMemory(char**);

#endif

