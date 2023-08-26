/*
    PROJECT LIBRARY INCLUSION

*/
#include "../include/common.h"

/*
    FUNCTIONS DECLARATIONS
*/
//utility functions
int getConnectedDevicesNumber();
bool getOverride();
double getTimeOn(time_t *, double *);
bool add(int);
void lastThings();

//command execution functions
void getSystemInfo(char*);
void getInfo(char*);
int deleteChildren();
int linkChildren();
int unlinkChildren();
int switchChildren(bool);
int forwardDel(char *);
int forwardLink(char *);
int forwardUnlink(char *);
int forwardSwitch(char *);
int forwardCommand(char *);

//mirroring interaction devices functions
void controlMirroring();
int implementMirroring(char *, bool);
void changeBulbsState(bool);
void changeWindowsState();      //function called by signal SIGALARM

//Handler for manualMode signal
void signalHandler(int);
void commandOnOff(char *,char *);
void getState(char *);

/*
    HUB INFORMATION
*/
//parent pipe descriptors
int parentWrite, parentRead;

//represents if the defice is connected to the system or still in home
bool linked = false;

//the type of the device
int type = HUB;

//the id of the device
char id[MAX_SINGLE_INFO_LEN];

//HUB MIRRORING FOR BULBS WITH STATE, SWITCH AND TIME (if bulb are present)
bool bulbsMirroring = false;
bool bulbsState = false;    //represent the state of the hub connected bulbs
bool swBulbs = false;      //represent the position of the hub bulb switch
double timeOnBulbs = 0.0;
time_t tBulbs = 0;

//HUB MIRRORING FOR WINDOWS STATE, SWITCH AND TIME (if windows are present)
bool windowsMirroring = false;
bool windowsState = false;  //represent the state of the hub's windows
bool swWOpen = false;       //represent the position of the hub open windows switch
bool swWClose = false;      //represent the position of the hub close windows switch
double timeOpenWindows = 0.0;
time_t tWindows = 0;

//HUB MIRRORING FOR FRIDGES
bool fridgesMirroring = false;

//TIMER MIRRORING
bool timersMirroring = false;

//list of connected devices
struct Node* childrenList = NULL; 

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        return 0;
    }

    parentWrite = atoi(argv[1]); 
    parentRead = atoi(argv[2]);

    signal(SIGURG, signalHandler);
    signal(SIGALRM, changeWindowsState);

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

            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, commandTokens[0], MAX_COMMAND_LEN);
                temp = temp->next;
            }

            temp = childrenList;
            while (temp != NULL)
            {
                bool cont = true;
                while(cont)
                {
                    read(temp->fdRead, response, MAX_MESSAGE_LEN);
                    if(strcmp(response, ACK_STRING) != 0)
                    {
                        int infoTokensNumber = 0;
                        char **infoTokens = formatCommand(response, " ", &infoTokensNumber);

                        if(infoTokensNumber == 4)
                        {
                            strcat(response, " ");
                            strcat(response, id);
                        }
                        write(parentWrite, response, MAX_MESSAGE_LEN);
                        freeAllocatedMemory(infoTokens);
                    }
                    else
                    {
                        cont = false;
                    }
                }
                temp = temp->next;
            }
            sprintf(response, "%d", ACK);
        } else if (strcmp(commandTokens[0], "info") == 0 && (tokensNumber == 1 || strcmp(commandTokens[1], id) == 0))
        {
            getInfo(response);
        } else if (strcmp(commandTokens[0], "info") == 0)
        {
            //something like a forward link.. Available only in HUB devices
            sprintf(response, "%d", NACK);

            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, commandBuffer, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                temp = strcmp(response, NACK_STRING) != 0 ? NULL : temp->next;
            }
        } else if (strcmp(commandTokens[0], "del") == 0 && tokensNumber == 1)
        {
            //must be deleted for upper reason
            int deletedDevices = deleteChildren();
            deletedDevices++;
            
            controlMirroring();

            sprintf(response, "%d", deletedDevices);

            goOn = false;
        } else if (strcmp(commandTokens[0], "del") == 0 && strcmp(commandTokens[1], id) == 0)
        {    
            //this is the higher device that must be deleted
            int deletedDevices = deleteChildren();
            deletedDevices++;
            
            controlMirroring();

            sprintf(response, "%d %d", ACK, deletedDevices);
            
            goOn = false;
        } else if (strcmp(commandTokens[0], "del") == 0 && tokensNumber == 2)
        {
            //the delete command does not refer to this device but we have to propagate it to the system
            int deletedDevice = forwardDel(commandBuffer);
            if(deletedDevice != 0)
            {
                sprintf(response, "%d", deletedDevice);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "state") == 0)
        {
            //the parent asked to know the state
            bool override = getOverride();
            sprintf(response, "%d %d", type, override);
        } else if(strcmp(commandTokens[0], "fridgesituation") == 0)
        {
            sprintf(response, "%d", forwardCommand(commandBuffer));
        } else if(strcmp(commandTokens[0], "type") == 0 && (tokensNumber == 1 || strcmp(commandTokens[1], id) == 0))
        {
            //the parent or the control unit asked the type
            sprintf(response, "%d", type);
        } else if(strcmp(commandTokens[0], "type") == 0 && tokensNumber == 2)
        {
            //the parent or the control unit is not interessed to the type of this device but maybe to the type of a child of it
            sprintf(response, "%d", forwardCommand(commandBuffer));
        } else if(strcmp(commandTokens[0], "controltype") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            //the control unit asked to know the type of the device connected to this device
            //in case of interaction devices it must return NACK
            int type = EMPTY_CONTROL_DEVICE;
            if(childrenList != NULL)
            {
                //TVTTB - ILU PERIOT <3
                //sends type command one of the children
                char commandToSend[MAX_COMMAND_LEN];
                sprintf(commandToSend, "type");
                write(childrenList->fdWrite, commandToSend, MAX_COMMAND_LEN);

                //read the type that must be the same for every one 
                char controltypeString[MAX_MESSAGE_LEN];
                read(childrenList->fdRead, controltypeString, MAX_MESSAGE_LEN);
                type = atoi(controltypeString);
            }
            sprintf(response, "%d", type);
        } else if(strcmp(commandTokens[0], "controltype") == 0)
        {
            //the control unit does not matter the type of the device connected here but maybe of some device more in deep in the system
            sprintf(response, "%d", forwardCommand(commandBuffer));
        } else if(strcmp(commandTokens[0], "loop") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            char command[MAX_COMMAND_LEN], tmpBuffer[MAX_MESSAGE_LEN];
            sprintf(command, "info %s", commandTokens[2]);
            
            //search if a child is the device where this one must be linked
            bool loop = false;
            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, command, MAX_COMMAND_LEN);
                read(temp->fdRead, tmpBuffer, MAX_MESSAGE_LEN);
                if(strcmp(tmpBuffer, NACK_STRING) != 0)
                {
                    loop = true;
                    temp = NULL;
                }
                else
                {
                    temp = temp->next;
                }
            }
            
            //return ACK if the loop command can cause loop situation
            if(loop)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            } 
        } else if(strcmp(commandTokens[0], "loop") == 0)
        {
            sprintf(response, "%d", forwardCommand(commandBuffer));
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

                //forward the link command on the branch
                movedDevices = linkChildren();

                //return the children moved number plus one (itself)
                movedDevices++;

                //return two result: the fact that this device is moved and the number of children of it
                sprintf(response, "%d %d", ACK, movedDevices);

                goOn = false;
            }            
        } else if(tokensNumber == 3 && strcmp(commandTokens[1], "linkto") == 0)
        {            
            //the linkto command does not refer to this device but we have to propagate it to the system
            int movedDevices = forwardLink(commandBuffer);

            //control the mirroring situation because the device under the hub might be changed (I don't know english)
            controlMirroring();

            //return NACK so that the parent does not delete the fd to this device
            sprintf(response, "%d %d", NACK, movedDevices);
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

                //forward the link command on the branch
                movedDevices = linkChildren();

                //return the children moved number plus one (itself)
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

                //forward the link command on the branch
                movedDevices = linkChildren();

                //return the children moved number plus one (itself)
                movedDevices++;

                sprintf(response, "%d %d", ACK, movedDevices);

                goOn = false;
            }
        } else if(strcmp(commandTokens[0], "link") == 0)
        {
            //the link command does not refer to this device but we have to propagate it to the system
            int movedDevices = forwardLink(commandBuffer);

            //control the mirroring situation because the device under the hub might be changed (I don't know english)
            if(movedDevices > 0)
            {
                controlMirroring();
                sprintf(response, "%d", movedDevices);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "unlink") == 0 && tokensNumber == 1)
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
            unlinkedDevices = unlinkChildren(commandBuffer);
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
            //forward the link command on the branch
            unlinkedDevices = unlinkChildren();
            unlinkedDevices++;
            sprintf(response, "%d %d", ACK, unlinkedDevices);
            goOn = false;
        } else if(strcmp(commandTokens[0], "unlink") == 0)
        {
            //the unlink command does not refer to this device but we have to propagate it to the system
            int unlinkedDevices = forwardUnlink(commandBuffer);
            if(unlinkedDevices != 0)
            {
                sprintf(response, "%d", unlinkedDevices);
                controlMirroring();
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "getlink") == 0 && (tokensNumber == 1 || strcmp(commandTokens[1], id) == 0))
        {
            //START CREATING THE BRANCH!
            char infoMovedDevice[MAX_MESSAGE_LEN] = "";

            //for every line on in the message queue create a new child
            while(readFromMessageQueue(id, infoMovedDevice))
            {
                int infoTokensNumber = 0;
                char **infoMovedDeviceTokens = formatCommand(infoMovedDevice, " ", &infoTokensNumber);

                if(add(atoi(infoMovedDeviceTokens[0])))
                {
                    char tmpBuffer[MAX_MESSAGE_LEN];

                    //send the set info command to prepare the destination for a message
                    char setInfoCommand[MAX_COMMAND_LEN] = "setinfo";
                    write(childrenList->fdWrite, setInfoCommand, MAX_COMMAND_LEN);
                    read(childrenList->fdRead, tmpBuffer, MAX_MESSAGE_LEN);

                    //send the old information
                    write(childrenList->fdWrite, infoMovedDevice, MAX_MESSAGE_LEN);
                    read(childrenList->fdRead, tmpBuffer, MAX_MESSAGE_LEN);
                }
                freeAllocatedMemory(infoMovedDeviceTokens);
            }

            //control the mirroring situation because for sure the devices are more than before
            controlMirroring();

            //when there aren't no more message the file for the mq is removed
            if(!removeFile(id))
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                forwardCommand("getlink");

                //return ACK so that the parent will stop to send this type of message
                sprintf(response, "%d", ACK);
            }
        } else if(strcmp(commandTokens[0], "getlink") == 0)
        {
            //the getlink command does not refer to this device but we have to propagate it to the system
            forwardCommand(commandBuffer);

            //control the mirroring situation because this hub has not more children but maybe more nipoti xD
            controlMirroring();

            //return NACK so that the forwardCommand will forward to every children
            sprintf(response, "%d", NACK);
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

            //bulbs mirroring information
            if(strcmp(infoTokens[6], "-1") != 0)
            {
                timeOnBulbs = atof(infoTokens[7]);
            }

            //windows mirroring information
            if(strcmp(infoTokens[8], "-1") != 0)
            {
                timeOpenWindows = atof(infoTokens[9]);
            }

            freeAllocatedMemory(infoTokens);

            //last feedback
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "find") == 0)
        {
            int type = atoi(commandTokens[1]);
            if(bulbsMirroring && type == BULB ||
                windowsMirroring && type == WINDOW ||
                fridgesMirroring && type == FRIDGE ||
                timersMirroring && type == TIMER)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", forwardCommand(commandBuffer));
            }
        } else if(strcmp(commandTokens[0], "switch") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            //FIRST DEVICE TO SWITCH
            int switchedDevices = 0;

            //if the <label> refers directly to switch change information and propagate
            if(strcmp(commandTokens[2], "sw-state") == 0)
            {
                bool pos = stringToState(commandTokens[3]);

                //to mantain state and time on of the bulbs under the hub
                if(bulbsMirroring)
                {
                    swBulbs = pos;
                    changeBulbsState(swBulbs);
                }
                //to mantain state and time open of the window under the hub
                if(windowsMirroring)
                {
                    if(!windowsState && pos)
                    {
                        swWOpen = true;
                        alarm(OPENING_WINDOW_TIME);
                    } else if(windowsState && !pos)
                    {
                        swWClose = true;
                        alarm(CLOSING_WINDOW_TIME);
                    }
                }
                if(bulbsMirroring || windowsMirroring || fridgesMirroring || timersMirroring)
                {
                    //forward the "switch state" command on the branch
                    switchedDevices = switchChildren(pos);

                    //return the fact that this device is switched and the number of children afflicted
                    sprintf(response, "%d", switchedDevices);
                }
            }
            else
            {
                switchedDevices = implementMirroring(commandTokens[2], stringToState(commandTokens[3]));

                //return the number of children afflicted or NACK
                if(switchedDevices != 0)
                {
                    sprintf(response, "%d", switchedDevices);
                }
                else
                {
                    sprintf(response, "%d", NACK);
                }
            }
        } else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 3)
        {
            //THIS DEVICE HAVE TO SWITCH FOR UPPER REASON
            int switchedDevices = 0;

            //if the <label> refers directly to switch change information and propagate
            if(strcmp(commandTokens[1], "sw-state") == 0)
            {
                //forward the "switch state" command on the branch
                switchedDevices = switchChildren(stringToState(commandTokens[2]));

                //return the fact that this device is switched and the number of children afflicted
                sprintf(response, "%d", switchedDevices);
            }
            else
            {
                switchedDevices = implementMirroring(commandTokens[1], stringToState(commandTokens[2]));

                //return the number of children afflicted or NACK
                if(switchedDevices != 0)
                {
                    sprintf(response, "%d", switchedDevices);
                }
                else
                {
                    sprintf(response, "%d", NACK);
                }
            }
        } else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 2)
        {
            //if I received this message a control device over me changed state and I have to change too
            int switchedDevices = 0;
            
            bool pos = atoi(commandTokens[1]);

            //forward the switch command
            switchedDevices = switchChildren(atoi(commandTokens[1]));

            //set the state of mirroring based on what it has linked to
            //bulbs mirroring state: bulbs state under the hub
            if(bulbsMirroring)
            {
                swBulbs = pos;
                changeBulbsState(swBulbs);
            }

            //window state under the hub with opening/closing procedure
            if(windowsMirroring && pos)
            {
                if(!windowsState)
                {
                    swWOpen = true;
                    alarm(OPENING_WINDOW_TIME);
                }      
            }
            if(windowsMirroring && pos)
            {
                if(windowsState)
                {
                    swWClose = true;
                    alarm(CLOSING_WINDOW_TIME);
                }      
            }
            sprintf(response, "%d", switchedDevices);
        } else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 4)
        {
            //the switch command does not refer to this device but we have to propagate it to the system
            int switchedDevices = forwardSwitch(commandBuffer);

            //return NACK if no children are afflicted
            if(switchedDevices == 0)
            {
                sprintf(response, "%d", NACK);
            }
            else
            {
                sprintf(response, "%d", switchedDevices);
            }
        } else if(strcmp(commandTokens[0], "set") == 0) {
            if(forwardCommand(commandBuffer) != NACK)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }         
        } else if(strcmp(commandTokens[0], "setdelay") == 0) {
            if(forwardCommand(commandBuffer) != NACK)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }        
        } else if(strcmp(commandTokens[0], "settemp") == 0) {
            if(forwardCommand(commandBuffer) != NACK)
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
        //returns the response based on the command processed
        write(parentWrite, response, MAX_MESSAGE_LEN);

        //free the memory allocated to save the command in form of tokens
        freeAllocatedMemory(commandTokens);
    } while(goOn);
    lastThings();
    return 0;
}

//UTILITY FUNCTIONS
//count the number of connected device
int getConnectedDevicesNumber()
{
    int num = 0;
    NodePointer temp = childrenList;
    while (temp != NULL) 
    { 
        num++;
        temp = temp->next; 
    } 
    return num;
}

//ask at every connected device the state in order to find the "override" information
bool getOverride()
{
    bool override = false;

    if(bulbsMirroring || windowsMirroring || fridgesMirroring || timersMirroring)
    {
        char command[MAX_COMMAND_LEN];
        sprintf(command, "state");

        char response[MAX_MESSAGE_LEN];

        NodePointer temp = childrenList;
        while (temp != NULL)
        {
            write(temp->fdWrite, command, MAX_COMMAND_LEN);
            read(temp->fdRead, response, MAX_MESSAGE_LEN);

            int tokensNumber = 0;
            char **responseTokens = formatCommand(response, " ", &tokensNumber);

            if(atoi(responseTokens[0]) == HUB)
            {
                if((bool)atoi(responseTokens[1]))
                {
                    override = true;
                    temp = NULL;
                }
            } else if(atoi(responseTokens[0]) == BULB)
            {
                if((bool)atoi(responseTokens[1]) != bulbsState)
                {
                    override = true;
                    temp = NULL;
                }
            } else if(atoi(responseTokens[0]) == WINDOW)
            {
                if((bool)atoi(responseTokens[1]) != windowsState)
                {
                    override = true;
                    temp = NULL;
                }
            } else if(atoi(responseTokens[0]) == TIMER)
            {
                if((bool)atoi(responseTokens[1]))
                {
                    override = true;
                    temp = NULL;
                }
            }
            if(!override)
            {
                temp = temp->next;
            }

            freeAllocatedMemory(responseTokens);
        }
    }
    return override;
}

//return the timeOn value
double getTimeOn(time_t *t, double *timeVar)
{
    if(*t == 0) return *timeVar;
    return *timeVar + difftime(time(NULL), *t);
}

//add a child of type arg[0]
bool add(int type)
{
    bool res = true;

    char file[MAX_SINGLE_INFO_LEN] ="";
    typeToFile(type, file);

    char programName [MAX_PATH_LEN] = "";
    sprintf(programName, "./bin/%s", file);

    int fd1[2]; int fd2[2];

    if (pipe (fd1))
    {
        perror("cannot create unnamed pipe\n");
        res = false;
    }
    if(pipe(fd2))
    {
        perror("cannot create unnamed pipe\n");
        res = false;
    }

    if(res)
    {
        pid_t childPid = spawnWithPipe(programName, fd1, fd2);

        if (childPid == -1)
        {
            res = false;
        }
        else
        {
            res = true;
            close(fd1[READ]);
            close(fd2[WRITE]);
            push(&childrenList, childPid, fd1[WRITE], fd2[READ]);
        }
    }

}

//close every file descriptor
void lastThings()
{
    NodePointer temp = childrenList;
    while (temp != NULL) 
    {  
        close(temp->fdWrite);
        close(temp->fdRead);
        temp = temp->next; 
    } 
    close(parentWrite);
    close(parentRead);
    deleteList(&childrenList);
}

//COMMAND EXECUTION FUNCTIONS
//return a char* that contains only the information necessary to print the system list
void getSystemInfo(char *systemInfo)
{
    sprintf(systemInfo, "%d %s %d "BOLDCYAN"info"NC, type, id, getpid());
}

//return a char* that contains every information separated with a space
void getInfo(char* info)
{
    //get the basic system info
    getSystemInfo(info);

    //process data to send and apend it to info
    //get the number of directly connected device
    int num = getConnectedDevicesNumber();

    //get the ovveride information
    bool override = getOverride();

    //so far info contains only information about the hub
    sprintf(info, "%s, %d %d", info, num, override);

    //get the mirroring information
    //controls bulbs mirroring
    if(bulbsMirroring)
    {
        //get timeOnBulbs information
        double tm = getTimeOn(&tBulbs, &timeOnBulbs);
        char tmp[10];   sprintf(tmp, " %.2f", tm);
        sprintf(info, "%s %d %s", info, bulbsState, tmp);
    }
    else
    {
        sprintf(info, "%s %d %d", info, -1, -1);
    }

    //control windows mirroring
    if(windowsMirroring)
    {
        //get timeOnWindow information
        double tm = getTimeOn(&tWindows, &timeOpenWindows);
        char tmp[10];   sprintf(tmp, " %.2f", tm);
        sprintf(info, "%s %d %s", info, windowsState, tmp);
    }
    else
    {
        sprintf(info, "%s %d %d", info, -1, -1);
    }

    //control fridges mirroring
    if(fridgesMirroring)
    {
        //returns true if there is at least on open fridge
        if(forwardCommand("fridgesituation") == NACK)
        {
            sprintf(info, "%s %d", info, false);
        }
        else
        {
            sprintf(info, "%s %d", info, true);
        }
    }
    else
    {
        sprintf(info, "%s %d", info, -1);
    }
}

//sends a message at every children to delete it and return the number of deleted devices
int deleteChildren()
{
    int deletedDevices = 0;

    char command[MAX_COMMAND_LEN] = "del", response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        deletedDevices += atoi(response);

        close(temp->fdWrite);
        close(temp->fdRead);

        temp = temp->next;
    }
    deleteList(&childrenList);
    return deletedDevices;
}

//sends command to move to its branch
int linkChildren()
{
    int movedDevices = 0;

    char command[MAX_COMMAND_LEN] = "", response[MAX_MESSAGE_LEN];
    sprintf(command, "linkto %s", id);

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);
        
        movedDevices += atoi(response);
  

        close(temp->fdWrite);
        close(temp->fdRead);
        temp = temp->next;

    }
    deleteList(&childrenList);
    return movedDevices;
}

//sends a message at every children to unlink it and return the number of unlinked devices
int unlinkChildren()
{
    int unlinkedDevices = 0;

    char command[MAX_COMMAND_LEN] = "unlink", response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        unlinkedDevices += atoi(response);

        close(temp->fdWrite);
        close(temp->fdRead);

        temp = temp->next;
    }
    deleteList(&childrenList);
    return unlinkedDevices;
}

//sends command to switch to <pos> on its branch
int switchChildren(bool pos)
{
    int switchedChildren = 0;

    char command[MAX_COMMAND_LEN] = "", response[MAX_MESSAGE_LEN];
    sprintf(command, "switch %d", pos);

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        switchedChildren += atoi(response);

        temp = temp->next;
    }
    return switchedChildren;
}

//propagate the del <id> command received from a level above
int forwardDel(char* command)
{
    int deletedDevice = 0;

    char response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL) 
    { 
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 1)
        {
            if(strcmp(response, NACK_STRING) != 0)
            {
                deletedDevice += atoi(responseTokens[0]);
                temp = NULL;
            }
            else
            {
                temp = temp->next;
            }
        }
        else
        {
            deletedDevice += atoi(responseTokens[1]);

            //remove the child from the connected devices list
            close(temp->fdWrite);
            close(temp->fdRead);
            deleteNode(&childrenList, temp->pid);
            temp = NULL;
        }
        freeAllocatedMemory(responseTokens);
    }
    return deletedDevice;
}

//sends a message at every children and return the eventually sensed response
int forwardLink(char *command)
{
    int movedDevices = 0;

    char response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL) 
    { 
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        //printf("Interrogato %d e ottenuto %s\n", temp->pid, response);
        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 1)
        {
            if(strcmp(responseTokens[0], NACK_STRING) != 0)
            {
                movedDevices += atoi(responseTokens[0]);
                temp = NULL;
            }
            else
            {
                temp = temp->next;
            }
        }
        else
        {
            //in every case the fact that the tokens are 2 means that the device to link was found
            if(strcmp(responseTokens[0], ACK_STRING) == 0)
            {
                //remove the child from the connected devices list
                close(temp->fdWrite);
                close(temp->fdRead);
                deleteNode(&childrenList, temp->pid);               
            }
            movedDevices += atoi(responseTokens[1]);  
            temp = NULL;

        }
        freeAllocatedMemory(responseTokens);
    }
    return movedDevices;
}

//propagate the unlink <id> command received from a level above
int forwardUnlink(char *command)
{
    int unlinkedDevices = 0;

    char response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 1)
        {
            if(strcmp(response, NACK_STRING) != 0)
            {
                unlinkedDevices += atoi(responseTokens[0]);
                temp = NULL;
            }
            else
            {
                temp = temp->next;
            }
        }
        else
        {
            unlinkedDevices += atoi(responseTokens[1]);

            //remove the child from the connected devices list
            close(temp->fdWrite);
            close(temp->fdRead);
            deleteNode(&childrenList, temp->pid);
            temp = NULL;
        }
        freeAllocatedMemory(responseTokens);
    }
    return unlinkedDevices;
}

//propagate "switch label pos" or "switch pos" command
int forwardSwitch(char* command)
{
    int switchedDevices = 0;

    char response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);


        if(strcmp(response, NACK_STRING) != 0)
        {
            switchedDevices += atoi(response);
            //not set to NULL for forward of switch pos
        }
        temp = temp->next;

    }
    return switchedDevices;
}

//sends command to every children or stop when found the destinatary
int forwardCommand(char *command)
{
    int res = NACK;
    char response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);
        if(strcmp(response, NACK_STRING) != 0)
        {
            res = atoi(response);
            temp = NULL;
        }
        else
        {
            temp = temp->next;
        }
    }
    return res;
}


//MIRRORING INTERACTION DEVICE FUNCTIONS
//control the mirroring situation after a link or a deletion
void controlMirroring()
{
    char command[MAX_COMMAND_LEN];

    //look if there are bulbs under the hub
    sprintf(command, "find %d", BULB);

    if(forwardCommand(command) != NACK)
    {
        bulbsMirroring = true;
    }
    else
    {
        bulbsMirroring = false;
        bulbsState = false;   
        swBulbs = false;      //represent the position of the hu's' bulbs switch
        timeOnBulbs = 0.0;
        tBulbs = 0; 
    }


    //look if there are windows under the hub
    sprintf(command, "find %d", WINDOW);
    if(forwardCommand(command) != NACK)
    {
        windowsMirroring = true;
    }
    else
    {
        windowsMirroring = false;
        windowsState = false; 
        swWOpen = false;      
        swWClose = false;  
        timeOpenWindows = 0.0;
        tWindows = 0;
    }

    //look if there are fridges under the hub
    sprintf(command, "find %d", FRIDGE);
    if(forwardCommand(command) != NACK)
    {
        fridgesMirroring = true;
    }
    else
    {
        fridgesMirroring = false;
    }

    //look if there are timers under the hub
    sprintf(command, "find %d", TIMER);
    if(forwardCommand(command) != NACK)
    {
        timersMirroring = true;
    }
    else
    {
        timersMirroring = false;
    }
    
}

//forward the "switch label pos" command received on the branch if there are device with <label> as switch
int implementMirroring(char *switchName, bool pos)
{
    int switchedDevices = 0;
    char command[MAX_COMMAND_LEN];
    sprintf(command, "switch %s %d", switchName, pos);

    if(bulbsMirroring && strcmp(switchName, "sw-bulb") == 0)
    {
        swBulbs = pos;
        changeBulbsState(swBulbs);
        switchedDevices += forwardSwitch(command);
    }

    if(windowsMirroring && (strcmp(switchName, "sw-wopen") == 0) && pos)
    {
        if(!windowsState)
        {
            swWOpen = true;
            alarm(OPENING_WINDOW_TIME);
        }      
        switchedDevices += forwardSwitch(command);
    }

    if(windowsMirroring && (strcmp(switchName, "sw-wclose") == 0) && pos)
    {
        if(windowsState)
        {
            swWClose = true;
            alarm(CLOSING_WINDOW_TIME);
        }      
        switchedDevices += forwardSwitch(command);
    }

    if(fridgesMirroring && strcmp(switchName, "sw-fopen") == 0)
    {
        switchedDevices += forwardSwitch(command);
    }

    return switchedDevices;
}

//changes the state of the hub's bulbs
void changeBulbsState(bool newState) {
    if(bulbsState == newState) return;

    if(!bulbsState) {
        tBulbs = time(NULL);
    } else {
        timeOnBulbs += difftime(time(NULL), tBulbs);
        tBulbs = 0;
    }
    bulbsState = !bulbsState;
}

//function called by signal SIGALARM
//it controls the windows opening and closing procedure and the automatic fridges closing operation
void changeWindowsState()
{
    //DEBUG:
    //to see the window opening/closing time in practice
    //printf("Windows of hub %s changed state successfully\n", id);
    if(windowsMirroring)
    {
        if(swWOpen && !windowsState)
        {
            windowsState = true;

            //save a reference to save the opening time
            tWindows = time(NULL);
            swWOpen = false;
        }
        else
        {
            //that means that (swClose && state) is true
            windowsState = false;

            //add the passed opening time
            timeOpenWindows += difftime(time(NULL), tWindows);
            tWindows = 0;

            swWClose = false;
        }
    }
}

void getState(char *response)
{
    sprintf(response,"%d %s %d %d",ACK,"HUB",(getOverride())?1:0,getConnectedDevicesNumber());
}

void commandOnOff(char *command,char *response)
{
    //Invia il comando ai figli
    int switchedDevices = switchChildren(stringToState(toLower(command)));
    sprintf(response,"%d",ACK);
}

void signalHandler(int signal)
{
    char *command = readFifo();
    char *response = calloc(MAX_MESSAGE_LEN, sizeof(char));

    if(linked)
    {
        if(strcmp(command,"PRINT") == 0){
            getState(response);
        }
        else if(strcmp(command,"OFF") == 0 || strcmp(command,"ON") == 0){
            commandOnOff(command,response);
        }
        else{
            sprintf(response,"%d Command doesn't exist!",NACK);
        }
    }
    else
    {
        sprintf(response,"%d Before connecting to the device you have to linked it to the system!",NACK);
    }

    writeFifo(response);
}