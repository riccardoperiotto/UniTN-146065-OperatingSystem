#ifndef COMMON_H
#define COMMON_H
/*
    STANDARD LIBRARY INPUT
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <time.h>

/*
    MY LIBRARY INCLUSION
*/
#include "../include/communication.h"
#include "../include/list.h"
#include "../include/process.h"
#include "../include/reader.h"
#include "../include/graphic.h"

/*
    COMMON MACRO'S DEFINITIONS
*/
//to represent a device with an integer
#define EMPTY_CONTROL_DEVICE 0
#define HUB 1
#define TIMER 2
#define BULB 3
#define WINDOW 4
#define FRIDGE 5
#define FULL_CONTROL_DEVICE 6

//simulate the opening and closing time of a window in the system
#define OPENING_WINDOW_TIME 10.0
#define CLOSING_WINDOW_TIME 10.0

#define MAX_COMMAND_LEN 50 //used even in the reader and in the communication
#define MAX_SINGLE_INFO_LEN 35
#define MAX_PATH_LEN 50

#define NOT_A_NUMBER -1
#define NOT_A_TEMP_VALID -1
#define NOT_IN_LIMIT_FIRDGE -1

//refers to message queue communication and to file path that the program has to read
#define MPQ_CONTROL_UNIT "CU"     //message queue path for Control-Unit
#define MPQ_HOME "home"          //message queue path for home
#define SETUP_FILE_PATH "./setup.txt"



//common functions definitions
void typeToString(int,char *);
int stringToType(char *);
void stateToString(int, char *, char *);
void overrideToString(char *, char *);
bool stringToState(char *);
void typeToFile(int, char *);
void getPathTmp(char *, char *);
bool createFile(char * );
bool removeFile(char * );
void clearScreen();
int toInt(char[]);
double toDouble(char[]);
char* toUpper(char[]);
char* toLower(char []);
bool checkTimerSet(char*, char*);
int diffTimes(struct tm, struct tm);




#endif
