/*
    PROJECT LIBRARY INCLUSION

*/
#include "../include/common.h"


/*
    FUNCTIONS DECLARATIONS
*/
//utility functions
void lastThings();
double getTimeOpen();

//command execution functions
void getSystemInfo(char*);
void getInfo(char*);
void closingProcedure();
void changeState(bool);
int changeTemperature(int);
int changeFillingState(double);

//Handler for manualMode signal
void commandsOpenClose(char *, char *);
void commandChangeTemp(int, char *);
void commandAddRemove(double, char *);
void signalHandler(int);

/*
    FRIDGE INFORMATION
*/
//parent pipe descriptors
int parentWrite, parentRead;

//represents if the defice is connected to the system or still in home
bool linked = false;

//the type of the device
int type = FRIDGE;

//the id of the device
char id[MAX_SINGLE_INFO_LEN];

//rapresents the state of the fridge
bool state = false;

//represent the position of the hub switch "swOpen"
bool swOpen = false;

//represents the filling percentage
double fillingState = 0.0;

//represents the temperature of the fridge
int temperature = 0;

//represens the delay time of the fridge
double delayTime = 15.0;

//time (in seconds)
time_t timeOpen;
time_t t;

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        return 0;
    }
    parentWrite = atoi(argv[1]); 
    parentRead = atoi(argv[2]);

    signal(SIGURG, signalHandler);
    signal(SIGALRM, closingProcedure);

    char commandBuffer[MAX_COMMAND_LEN];
    bool goOn = true;
    do
    {
        read(parentRead, commandBuffer, MAX_COMMAND_LEN);

        int tokensNumber = 0;
        char **commandTokens = formatCommand(commandBuffer, " ", &tokensNumber);

        char response[MAX_MESSAGE_LEN];

        if(strcmp(commandTokens[0], "setid") == 0)
        {
            strcpy(id, commandTokens[1]);
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "list") == 0)
        {
            getSystemInfo(response);
            write(parentWrite, response, MAX_MESSAGE_LEN);
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "info") == 0 && (tokensNumber ==1 || strcmp(commandTokens[1], id) == 0))
        {
            getInfo(response);
        } else if(strcmp(commandTokens[0], "del") == 0 && tokensNumber == 1)
        {
            //this is the higher device that must be deleted
            sprintf(response, "%d", ACK);

            goOn = false;
        } else if(strcmp(commandTokens[0], "del") == 0 && strcmp(commandTokens[1], id) == 0)
        {    
            sprintf(response, "%d %d", ACK, ACK);
            
            goOn = false;
        } else if(strcmp(commandTokens[0], "fridgesituation") == 0)
        {
            //the parent asked to know if there are opened fridges
            if(state)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }            
        } else if(strcmp(commandTokens[0], "type") == 0 && (tokensNumber == 1 || strcmp(commandTokens[1], id) == 0))
        {
            //the parent or the control unit asked the type
            sprintf(response, "%d", type);
        } else if(tokensNumber == 3 && strcmp(commandTokens[0], id) == 0 && strcmp(commandTokens[1], "linkto") == 0)
        {
            //START MOVING THE BRANCH!
            int movedDevices = 0;

            char infoToTransfer[MAX_MESSAGE_LEN];
            getInfo(infoToTransfer);

            //create the file for the message queue unicity
            if(!createFile(commandTokens[2]))
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                if(writeOnMessageQueue(commandTokens[2], infoToTransfer))
                {
                    //DEBUG
                    //printf("%s write %s on message queue FIRST based on id %s\n", id, infoToTransfer, commandTokens[2]);
                }
                sprintf(response, "%d %d", ACK, ACK);
                goOn = false;
            }            
        } else if(strcmp(commandTokens[0], "linkto") == 0 && tokensNumber == 2 )
        {
            int movedDevices = 0;

            //get my info
            char infoToTransfer[MAX_MESSAGE_LEN];
            getInfo(infoToTransfer);

            //create the file for the message queue unicity
            if(!createFile(commandTokens[1]))
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                //write it on the message queue
                if(writeOnMessageQueue(commandTokens[1], infoToTransfer))
                {
                    //DEBUG:
                    //printf("%s write %s on message queue based on id %s\n", id, infoToTransfer, commandTokens[1]);
                }
                sprintf(response, "%d", ACK);
                goOn = false;
            }

        } else if(strcmp(commandTokens[0], "link") == 0 &&  strcmp(commandTokens[1], id)==0)
        {
            //this device must be linked to the Control Unit
            int movedDevices = 0;

            //get my info
            char infoToTransfer[MAX_MESSAGE_LEN];
            getInfo(infoToTransfer);

            //create the file for the message queue unicity
            if(!createFile(MPQ_CONTROL_UNIT))
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                //write it on the message queue
                if(writeOnMessageQueue(MPQ_CONTROL_UNIT, infoToTransfer))
                {
                    //DEBUG:
                    //printf("%s write %s on message queue based on mqp %s\n", id, infoToTransfer, mqpControlUnit);
                }
                sprintf(response, "%d %d", ACK, 1);
                goOn = false;
            }

        } else if (strcmp(commandTokens[0], "unlink") == 0 && tokensNumber == 1)
        {
            //this device has to unlink because another upper device is unlinked

            //get my info
            char infoToTransfer[MAX_MESSAGE_LEN];
            getInfo(infoToTransfer);

            //create the file for the message queue unicity
            if(!createFile(MPQ_HOME))
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                //write it on the message queue
                if(writeOnMessageQueue(MPQ_HOME, infoToTransfer))
                {
                    //DEBUG:
                    //printf("%s write %s on message queue based on id %s\n", id, infoToTransfer, commandTokens[1]);
                }
            }
            sprintf(response, "%d", 1);
            goOn = false;
        } else if(strcmp(commandTokens[0], "unlink") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            //this device must be unlinked

            //get my info
            char infoToTransfer[MAX_MESSAGE_LEN];
            getInfo(infoToTransfer);

            //create the file for the message queue unicity
            if(!createFile(MPQ_HOME))
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                //write it on the message queue
                if(writeOnMessageQueue(MPQ_HOME, infoToTransfer))
                {
                    //DEBUG:
                    //printf("%s write %s on message queue based on id %s\n", id, infoToTransfer, commandTokens[1]);
                }              
            }
            sprintf(response, "%d %d", ACK, 1);
            goOn = false;
        } else if(strcmp(commandTokens[0], "setinfo") == 0)
        {
            linked = true;

            //send feedback to get information message
            sprintf(response, "%d", ACK);
            write(parentWrite, response, MAX_MESSAGE_LEN);

            //get the device information
            char infoDevice[MAX_MESSAGE_LEN];
            read(parentRead, infoDevice, MAX_MESSAGE_LEN);

            //save it
            int infoTokensNumber = 0;
            char **infoTokens = formatCommand(infoDevice, " ", &infoTokensNumber);

            strcpy(id, infoTokens[1]);

            //NB: if I link a fridge I wanna link it in a close state so it does not matter its old state
            
            //set the properties
            timeOpen = atof(infoTokens[4]);
            fillingState = atof(infoTokens[5]);
            delayTime = atof(infoTokens[6]);
            temperature= atoi(infoTokens[7]);

            freeAllocatedMemory(infoTokens);

            //last feedback
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "find") == 0 && (atoi(commandTokens[1]) == type))
        {
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "switch") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            //DEVICE TO SWITCH
            //if the <label> refers directly to switch change information and propagate
            if(strcmp(commandTokens[2], "sw-fopen") == 0)
            {
                //if this device is a hub it set the information of the hub and propagate the command to every children
                swOpen = stringToState(commandTokens[3]);
                changeState(swOpen);

                //return the fact that this device is switched and the number of children afflicted
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 3)
        {
            //THIS DEVICE HAVE TO SWITCH FOR UPPER REASON
            //if the <label> refers directly to switch change information and propagate
            if(strcmp(commandTokens[1], "sw-fopen") == 0)
            {
                //if this device is a hub it set the information of the hub and propagate the command to every children
                swOpen = stringToState(commandTokens[2]);
                
                changeState(swOpen);

                //return the fact that this device is switched and the number of children afflicted
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if (strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 2)
        {
            //if I received this message a control device over me changed state and I have to change too
            swOpen = atoi(commandTokens[1]);
            changeState(swOpen);
            sprintf(response, "%d", ACK);
        }  else if (strcmp(commandTokens[0], "setdelay") == 0 && strcmp(commandTokens[1], id)==0) {
            delayTime = atof(commandTokens[3]);
            sprintf(response, "%d", ACK);
        } else if (strcmp(commandTokens[0], "settemp") == 0 && strcmp(commandTokens[1], id)==0) {
            int newTemperature = atoi(commandTokens[3]); 
            if(changeTemperature(newTemperature) == 0)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else {
            sprintf(response, "%d", NACK);
        }
        write(parentWrite, response, MAX_MESSAGE_LEN);

        freeAllocatedMemory(commandTokens);
    } while(goOn);
    lastThings();
    return 0;
}

//UTILITY FUNCTIONS
//close every file descriptor
void lastThings()
{
    close(parentWrite);
    close(parentRead);
}

//return the time open so far
double getTimeOpen()
{
    if(t == 0) return timeOpen;
    return timeOpen + difftime(time(NULL), t);
}


//COMMAND EXECUTION FUNCTIONS
//return a char* that contains only the information necessary to print the system list
void getSystemInfo(char *systemInfo)
{
    sprintf(systemInfo, "%d %s %d %d", type, id, getpid(), state);
}

//return a char* that contains every information separated with a space
void getInfo(char* info)
{
    getSystemInfo(info);
    double tm = getTimeOpen();
    sprintf(info, "%s %.2f %.2f%% %.2f %d ", 
            info, tm, fillingState*100, delayTime, temperature );
}

//handle the auto closign procedure
void closingProcedure() 
{
    changeState(false);
}

void changeState(bool newValue)
{
    if(state == newValue) return;

    if (!state)
    {
        //open fridge
        t = time(NULL);

        //the signal SIGALARM will call the closingProcedure() functions
        alarm(delayTime);   
    } else {
        //close fridge
        timeOpen += difftime(time(NULL), t);
        t = 0;
    }
    state = !state;
}


int changeTemperature(int value)
{
    if (value <= -10 || value >= 10)
    {
        return NOT_A_TEMP_VALID;
    }
    temperature = value;

    return 0;
}

int changeFillingState(double value)
{
    if (value+fillingState < 0.0 || value+fillingState > 1.0)
    {
        return NOT_IN_LIMIT_FIRDGE;
    }
    fillingState += value;

    return 0;
}


//HANDLER FOT MANUAL MODE SIGNAL
void commandsOpenClose(char *command, char *response)
{
    if(strcmp(command,"OPEN") == 0 && state || strcmp(command,"CLOSE") == 0 && !state){
        sprintf(response,"%d Fridge is already in this state!",NACK);
    }
    else{
        changeState(!state);
        sprintf(response,"%d",ACK);
    }
}

void commandChangeTemp(int temp, char *response)
{
    if(changeTemperature(temp) == NOT_A_TEMP_VALID)
    {
        sprintf(response,"%d The temparature is over or under the limits!",NACK);
    }
    else
    {
        sprintf(response,"%d",ACK);
    }
}

void commandAddRemove(double value, char *response)
{
    value = value / 100;
    if(changeFillingState(value) == NOT_IN_LIMIT_FIRDGE)
    {
        sprintf(response,"%d The percentage is too high!",NACK);
    }
    else
    {
        sprintf(response,"%d",ACK);
    }
}

void getState(char *response)
{
    double tm = getTimeOpen();
    double fs = fillingState;
    int temp = temperature;
    sprintf(response,"%d %s %s %.2lf %.2lf %d",ACK,"FRIDGE",((state)?"OPEN":"CLOSE"),tm,fs,temp);
}

void signalHandler(int signal)
{
    char *command = readFifo();
    char *response = calloc(MAX_MESSAGE_LEN, sizeof(char));

    int tokensNumber = 0;
    char **commandTokens = formatCommand(command," ",&tokensNumber);

    if(linked)
    {
        if(strcmp(commandTokens[0],"PRINT") == 0)
        {
            getState(response);
        }
        else if(strcmp(commandTokens[0],"CLOSE") == 0 || strcmp(commandTokens[0],"OPEN") == 0){
            commandsOpenClose(command,response);
        }
        else if(strcmp(commandTokens[0],"TEMP") == 0)
        {
            commandChangeTemp(toInt(commandTokens[1]),response);
        }
        else if(strcmp(commandTokens[0],"ADD") == 0)
        {
            commandAddRemove(toDouble(commandTokens[1]),response);
        }
        else if(strcmp(commandTokens[0],"REMOVE") == 0)
        {
            commandAddRemove((toDouble(commandTokens[1])*(-1.0)),response);
        }
        else
        {
            sprintf(response,"%d Command doesn't exist!",NACK);
        }
    }
    else
    {
        sprintf(response,"%d Before connecting to the device you have to linked it to the system!",NACK);
    }
    writeFifo(response);
}
