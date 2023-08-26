#include "../include/common.h"

//return the char* that rapresent the device type
void typeToString(int type, char* typeString )
{
    switch (type)
    {
        case HUB: strcat(typeString, "HUB"); break;
        case TIMER: strcat(typeString, "TIMER"); break;
        case BULB: strcat(typeString, "BULB"); break;
        case WINDOW: strcat(typeString, "WINDOW"); break;
        case FRIDGE: strcat(typeString, "FRIDGE"); break;
    }
}

int stringToType(char *typeString)
{
    int type = HUB; //default is hub, typeString is correct for sure
    if(strcmp(typeString, "timer")==0)
    {
        type = TIMER;
    } else if(strcmp(typeString, "bulb")==0)
    {
        type = BULB;
    } else if(strcmp(typeString, "window")==0)
    {
        type = WINDOW;
    } else if(strcmp(typeString, "fridge")==0)
    {
        type = FRIDGE;
    }
    return type;
}

//converts the string in the token (that is a bool string) to a keyword that represent the state
void stateToString(int type, char *state, char *token)
{
    switch (type)
    {
        case HUB:
            //it depends on the mirroring states so see info
            strcpy(state, token);
            break;
        case WINDOW:
            if(strcmp(token, "0") == 0)
            {
                strcpy(state, BOLDRED"closed"NC);
            }
            else
            {
                strcpy(state, BOLDGREEN"open"NC);
            }
            break;
        case FRIDGE:
            if(strcmp(token, "0") == 0)
            {
                strcpy(state, BOLDRED"closed"NC);
            }
            else
            {
                strcpy(state, BOLDGREEN"open"NC);
            }
            break;
        default:
            if(strcmp(token, "0") == 0)
            {
                strcpy(state, BOLDRED"off"NC);
            }
            else
            {
                strcpy(state, BOLDGREEN"on"NC);
            }
            break;
    }
}

//converts the string in the token (that is a bool string) to an override keyword
void overrideToString(char *override, char *token)
{
    if(strcmp(token, "0") == 0)
    {
        strcpy(override, "no");
    }
    else
    {
        strcpy(override, "yes");
    }
}

//converts the <position> passed in form of string to bool
bool stringToState(char *state)
{
    bool res = true;
    if(strcmp(state, "off") == 0)
    {
        res = false;
    }
    return res;
}


//get in file the name of the program to execute
void typeToFile(int type, char *file)
{
    switch (type)
    {
        case HUB:
            strcpy(file, "hub");
            break;
        case TIMER:
            strcpy(file, "timer");
            break;
        case BULB:
            strcpy(file, "bulb");
            break;
        case WINDOW:
            strcpy(file, "window");
            break;
        case FRIDGE:
            strcpy(file, "fridge");
            break;
    }
}

void getPathTmp(char *id, char *path)
{
    sprintf(path, "/tmp/%s", id);
}

bool createFile(char * name)
{
    bool created = true;

    //get path from name
    char path[MAX_PATH_LEN]="";
    getPathTmp(name, path);

    //create the file    
    FILE * fp;
    fp=fopen(path, "a");
    if(fp == NULL)
    {
        perror("Error during fopen()\n");
        created= false;
    }
    fclose(fp);
    return created;
}

bool removeFile(char * name)
{
    bool removed = true;

    //get path from name
    char path[MAX_PATH_LEN]="";
    getPathTmp(name, path);

    //remove the file    
    /*printf("Voglio rimuovere %s\n", path);
    if(remove(path) != 0)
    {
        perror("Error during remove()\n");
        removed = false;
    }*/
    
    //We are sure that the file exist
    remove(path);
    return removed;
}


//sends a system call clear
void clearScreen()
{
    system("clear");
}

int toInt(char a[]) {
    int c, n, tmp;

    n = 0;

    for (c = 0; a[c] != '\0'; c++) {
        tmp = a[c]-'0';
        if(tmp<0 || tmp > 9){ //Controllo se NON sto passando un numero
            return NOT_A_NUMBER;
        }
        n = n * 10 + tmp;
    }

    return n;
}

double toDouble(char a[]) {
    double tmp;
    sscanf(a, "%lf", &tmp);
    return tmp;
}

char* toUpper(char str[]){
    int i=0;
    char* tmp = calloc(strlen(str)+1, sizeof(char));
    while(str[i]) {
        tmp[i]=toupper(str[i]);
        i++;
    }
    tmp[i] = '\0';
    return tmp;
}

//check whether a timer exists and inputs are valid
bool checkTimerSet(char* beginTime, char* endTime) {
    if(atof(beginTime) == 0.0 || atof(endTime) == 0.0) return false;
    if((int)atof(beginTime) < 0 || (int)atof(beginTime) > 23) return false;
    if((int)atof(endTime) < 0 || (int)atof(endTime) > 23) return false;
    int bT = ((int)(atof(beginTime) * 100)) - ((int)(atof(beginTime)) * 100);
    int eT = ((int)(atof(endTime) * 100)) - ((int)(atof(endTime)) * 100);
    if(bT > 59 || eT > 59) return false;
    return true;
}

char* toLower(char str[]){
    int i=0;
    char* tmp = calloc(strlen(str)+1, sizeof(char));
    while(str[i]) {
        tmp[i]=tolower(str[i]);
        i++;
    }
    tmp[i] = '\0';
    return tmp;
}


int diffTimes(struct tm time1, struct tm time2) {
    int tm1 = time1.tm_hour * 3600 + time1.tm_min * 60;
    int tm2 = time2.tm_hour * 3600 + time2.tm_min * 60;
    int seconds = 0;
    if(time2.tm_sec != 0) seconds = time2.tm_sec;
    return tm1 > tm2 ? tm1 - tm2 - seconds : 86400 - (tm2 - tm1) - seconds;
}


