
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
void getInfoString(char*);
void getSystemInfo(char*);
void getInfo(char*);
void changeState();

//Handler for manualMode signal
void signalHandler(int);
void commandClose(char *);
void commandOpen(char *);

/*
    WINDOW INFORMATION
*/
//parent pipe descriptors
int parentWrite, parentRead;

//represents if the defice is connected to the system or still in home
bool linked = false;

//the type of the device
int type = WINDOW;

//the id of the device
char id[MAX_SINGLE_INFO_LEN];

//rapresents the state of the window
bool state = false;

//represent the position of the hub switch "swOpen"
bool swOpen = false;

//represent the position of the hub switch "swClose"
bool swClose = false;

//time (in seconds)
double timeOpen = 0.0;
time_t t;

int main(int argc, char *argv[])
{
    if(argc != 3) return 0;

    parentWrite = atoi(argv[1]); 
    parentRead = atoi(argv[2]);

    signal(SIGURG, signalHandler);
    signal(SIGALRM, changeState);


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
        } else if (strcmp(commandTokens[0], "list") == 0)
        {
            getSystemInfo(response);
            write(parentWrite, response, MAX_MESSAGE_LEN);
            sprintf(response, "%d", ACK);
        } else if (strcmp(commandTokens[0], "info") == 0 && (tokensNumber ==1 || strcmp(commandTokens[1], id) == 0))
        {
            getInfo(response);
        } else if (strcmp(commandTokens[0], "del") == 0 && tokensNumber == 1)
        {
            //this is the higher device that must be deleted
            sprintf(response, "%d", 1);
            goOn = false;
        } else if (strcmp(commandTokens[0], "del") == 0 && strcmp(commandTokens[1], id) == 0)
        {    
            //this is the higher device that must be deleted
            sprintf(response, "%d %d", ACK, 1);
            goOn = false;
        } else if(strcmp(commandTokens[0], "state") == 0)
        {
            //the parent asked to know the state
            sprintf(response, "%d %d", type, state);
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
                movedDevices++;
                sprintf(response, "%d %d", ACK, movedDevices);
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
                movedDevices++;
                sprintf(response, "%d", movedDevices);
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
                movedDevices++;
                sprintf(response, "%d %d", ACK, movedDevices);
                goOn = false;
            }

        }  else if (strcmp(commandTokens[0], "unlink") == 0 && tokensNumber == 1)
        {
            //this device has to unlink because another upper device is unlinked
            int unlinkedDevices = 0;

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
            unlinkedDevices++;
            sprintf(response, "%d", unlinkedDevices);
            goOn = false;
        } else if(strcmp(commandTokens[0], "unlink") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            //this device must be unlinked
            int unlinkedDevices = 0;

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
            unlinkedDevices++;
            sprintf(response, "%d %d", ACK, unlinkedDevices);
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

            timeOpen = atof(infoTokens[4]);

            freeAllocatedMemory(infoTokens);

            //last feedback
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "find") == 0 && (atoi(commandTokens[1]) == type))
        {
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "switch") == 0 && strcmp(commandTokens[1], id) == 0) {
            //DEVICE TO SWITCH
            //if the <label> refers to a correct switch change info
            if(strcmp(commandTokens[2], "sw-wopen") == 0 && strcmp(commandTokens[3], "on")==0) {
                if(!state)
                {
                    swOpen = true;
                    alarm(OPENING_WINDOW_TIME);
                    sprintf(response, "%d", ACK);
                }
                else
                {
                    sprintf(response, "%d", NACK);
                }                               
            } else if(strcmp(commandTokens[2], "sw-wclose") == 0 && strcmp(commandTokens[3], "on")==0) {
                if(state)
                {
                    swClose = true;
                    alarm(CLOSING_WINDOW_TIME);
                    sprintf(response, "%d", ACK);
                }
                else
                {
                    sprintf(response, "%d", NACK);
                }
            } else {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 3)
        {
            //THIS DEVICE HAVE TO SWITCH FOR UPPER REASON. Command received: switch <label> <pos>
            if(strcmp(commandTokens[1], "sw-wopen") == 0 && strcmp(commandTokens[2], "on")==0) {
                if(!state)
                {
                    swOpen = true;
                    alarm(OPENING_WINDOW_TIME);
                    sprintf(response, "%d", ACK);
                }
                else
                {
                    sprintf(response, "%d", NACK);
                }                               
            } else if(strcmp(commandTokens[1], "sw-wclose") == 0 && strcmp(commandTokens[2], "on")==0) {
                if(state)
                {
                    swClose = true;
                    alarm(CLOSING_WINDOW_TIME);
                    sprintf(response, "%d", ACK);
                }
                else
                {
                    sprintf(response, "%d", NACK);
                }
            } else {
                sprintf(response, "%d", NACK);
            }

        } else if (strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 2)
        {
            //if I received this message a control device over me changed state and I have to change too
            bool pos = atoi(commandTokens[1]);
            if(pos && ! state) {
                swOpen = true;
                alarm(OPENING_WINDOW_TIME);
                sprintf(response, "%d", ACK);                              
            } else if(!pos && state) {
                swClose = true;
                alarm(CLOSING_WINDOW_TIME);
                sprintf(response, "%d", ACK);
            } else {
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

//get the opening time of the window
double getTimeOpen() {
    if(t == 0) return timeOpen; //if the fridge is closed
    return timeOpen + difftime(time(NULL), t);
}

//COMMAND EXECUTION FUNCTIONS
//return a char* that contains only the information necessary to print the system list
void getSystemInfo(char *systemInfo) {
    sprintf(systemInfo, "%d %s %d %d", type, id, getpid(), state);
}

//return a char* that contains every information separated with a space
void getInfo(char* info)
{
    getSystemInfo(info);

    //preprocess information
    double tm = getTimeOpen();

    //add these information to the string
    sprintf(info, "%s %d %d %.2f", info, swOpen, swClose, tm);
}

//function handler for SIGALARM
void changeState()
{
    if(swOpen && !state)
    {
        state = true;

        //save a reference to save the opening time
        t = time(NULL);
        swOpen = false;
    }
    else
    {
        //that means that (swClose && state) is true
        state = false;

        //add the passed opening time
        timeOpen += difftime(time(NULL), t);
        t = 0;

        swClose = false;
    }    
}

void getState(char *response){
    double tm = getTimeOpen();
    sprintf(response,"%d %s %s %.2lf",ACK,"WINDOW",((state)?"OPEN":"CLOSE"),tm);
}

void commandOpen(char *response){
    if(!state)
    {
        swOpen = true;
        alarm(OPENING_WINDOW_TIME);
    }
    sprintf(response,"%d",ACK);
}

void commandClose(char *response){
    if(state)
    {
        swClose = true;
        alarm(CLOSING_WINDOW_TIME);
    }
    sprintf(response,"%d",ACK);
}

void signalHandler(int signal){
    char *command = readFifo();
    char *response = calloc(MAX_MESSAGE_LEN, sizeof(char));

    if(linked){
        if(strcmp(command,"PRINT") == 0){
            getState(response);
        }
        else if(strcmp(command,"CLOSE") == 0){
            commandClose(response);
        }
        else if(strcmp(command,"OPEN") == 0){
            commandOpen(response);
        }
        else{
            sprintf(response,"%d Command doesn't exist!",NACK);
        }
    }
    else{
        sprintf(response,"%d Before connecting to the device you have to linked it to the system!",NACK);
    }
    writeFifo(response);
}
