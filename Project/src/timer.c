/*
    PROJECT LIBRARY INCLUSION

*/
#include "../include/common.h"

/*
    FUNCTIONS DECLARATIONS
*/
//utility functions
bool getOverride();
bool add(int);
void lastThings();

//command execution functions
void getSystemInfo(char*);
void getInfo(char*);
int deleteChildren();
int linkChildren();
int switchChildren(bool);
int forwardDel(char *);
int forwardLink(char *);
int forwardUnlink(char *);
int forwardSwitch(char *);
int forwardCommand(char *);
void setTime(char*, char*);
void timeHandler();

//Handler for manualMode signal
void signalHandler(int);
void getState(char *);
void commandOnOff(char *,char *);

//list of connected devices
struct Node* childrenList = NULL;

/*
    TIMER INFORMATION
*/
//parent pipe descriptors
int parentWrite, parentRead;

//represents if the defice is connected to the system or still in home
bool linked = false;

//the type of the device
int type = TIMER;

//the id of the device
char id[MAX_SINGLE_INFO_LEN];

//rapresents if the timer is in an active period
bool isRunning = false;

//rapresents the times when the timer has to switch his children
struct tm beginTime, endTime;

//timer checks if has to switch on/off his children every "checkTime" seconds
int checkTime = 5;

int main(int argc, char *argv[])
{
    if(argc != 3) return 0;

    parentWrite = atoi(argv[1]);
    parentRead = atoi(argv[2]);

    char commandBuffer[MAX_COMMAND_LEN];
    bool goOn = true;

    signal(SIGURG, signalHandler);
    signal(SIGALRM, timeHandler);

    beginTime.tm_hour = 0;
    beginTime.tm_min = 0;
    endTime.tm_hour = 0;
    endTime.tm_min = 0;

    do
    {
        read(parentRead, commandBuffer, MAX_COMMAND_LEN);

        int tokensNumber = 0;
        char **commandTokens = formatCommand(commandBuffer, " ", &tokensNumber);

        char response[MAX_MESSAGE_LEN];
        if(strcmp(commandTokens[0], "setid") == 0)
        {
            //this command is sent only from home so in this case the device is unlinked
            strcpy(id, commandTokens[1]);
            linked = false;
            sprintf(response, "%d", ACK);
        } else if (strcmp(commandTokens[0], "list") == 0)
        {
            getSystemInfo(response);
            write(parentWrite, response, MAX_MESSAGE_LEN);

            if(childrenList != NULL)
            {
                write(childrenList->fdWrite, commandTokens[0], MAX_COMMAND_LEN);
                bool cont = true;
                while(cont)
                {
                    read(childrenList->fdRead, response, MAX_MESSAGE_LEN);
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
            }
            sprintf(response, "%d", ACK);
        } else if (strcmp(commandTokens[0], "info") == 0 && (tokensNumber ==1 || strcmp(commandTokens[1], id) == 0))
        {
            getInfo(response);
        } else if (strcmp(commandTokens[0], "info") == 0)
        {
            //something like a forward link.. Available only in HUB or TIMER devices
            sprintf(response, "%d", NACK);

            NodePointer temp = childrenList;
            if(childrenList != NULL)
            {
                write(temp->fdWrite, commandBuffer, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
            }
        } else if(strcmp(commandTokens[0], "set") == 0 && strcmp(commandTokens[1], id) == 0) {
            if(!isRunning) 
            {
                setTime(commandTokens[2], commandTokens[3]);
                sprintf(response, "%d", ACK);
            } 
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "set") == 0) {
            sprintf(response, "%d", forwardCommand(commandBuffer));
        } else if (strcmp(commandTokens[0], "del") == 0 && tokensNumber == 1)
        {
            //must be deleted for upper reason
            int deletedDevices = deleteChildren();
            deletedDevices++;
            
            sprintf(response, "%d", deletedDevices);

            goOn = false;
        } else if (strcmp(commandTokens[0], "del") == 0 && strcmp(commandTokens[1], id) == 0)
        {    
            //this is the higher device that must be deleted
            int deletedDevices = deleteChildren();
            deletedDevices++;
            
            //controlMirroring();

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
            int controlType = EMPTY_CONTROL_DEVICE;
            if(childrenList != NULL)
            {
                controlType = FULL_CONTROL_DEVICE;
            }
            sprintf(response, "%d", controlType);
        } else if(strcmp(commandTokens[0], "controltype") == 0)
        {
            //the control unit does not matter the type of the device connected here but maybe of some device more in deep in the system
            sprintf(response, "%d", forwardCommand(commandBuffer));
        } else if(strcmp(commandTokens[0], "loop") == 0 && strcmp(commandTokens[1], id) == 0)
        {
            char command[MAX_COMMAND_LEN], response[MAX_MESSAGE_LEN];
            sprintf(command, "info %s", commandTokens[2]);
            
            //search if a child is the device where this one must be linked
            bool loop = false;
            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, command, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                if(strcmp(response, NACK_STRING) != 0)
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
                sprintf(response, "%d", movedDevices);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if (strcmp(commandTokens[0], "unlink") == 0 && tokensNumber == 1)
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
        } else if (strcmp(commandTokens[0], "unlink") == 0)
        {
            //the unlink command does not refer to this device but we have to propagate it to the system

            int unlinkedDevices = forwardUnlink(commandBuffer);
            if(unlinkedDevices != 0)
            {
                sprintf(response, "%d", unlinkedDevices);
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
            beginTime.tm_hour = atoi(infoTokens[4]);
            beginTime.tm_min = atoi(infoTokens[5]);
            endTime.tm_hour = atoi(infoTokens[6]);
            endTime.tm_min = atoi(infoTokens[7]);

            freeAllocatedMemory(infoTokens);

            //last feedback
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "find") == 0)
        {
            sprintf(response, "%d", forwardCommand(commandBuffer));
        } else if(strcmp(commandTokens[0], "switch") == 0)
        {
            //a timer cannot be switched on or off with this type of command so
            //the only thing it does is propagate the command
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
        }  else if (strcmp(commandTokens[0], "setdelay")==0) {
            if(forwardCommand(commandBuffer) != NACK)
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }        
        } else if (strcmp(commandTokens[0], "settemp") == 0) {
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
        write(parentWrite, response, MAX_MESSAGE_LEN);
        freeAllocatedMemory(commandTokens);
    } while(goOn);
    lastThings();
    return 0;
}

//UTILITY FUNCTIONS

//ask at every connected device the state in order to find the "override" information
bool getOverride()
{
    bool override = false;

    if (childrenList != NULL)
    {
        char command[MAX_COMMAND_LEN];
        sprintf(command, "info");

        char response[MAX_MESSAGE_LEN];

        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        int type = atoi(responseTokens[0]);

        switch (type)
        {
            case HUB:
                if (strcmp(responseTokens[5], ACK_STRING) == 0)
                {
                    override = true;
                }
                break;
            case BULB:
                if((bool)atoi(responseTokens[3]) != isRunning)
                {
                    override = true;
                }
                break;
            case WINDOW:
                if((bool)atoi(responseTokens[3]) != isRunning)
                {
                    override = true;
                }
                break;
            default:
                break;
        }
        freeAllocatedMemory(responseTokens);
    }
    return override;
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
void getSystemInfo(char *systemInfo) {
    sprintf(systemInfo, "%d %s %d %d", type, id, getpid(), isRunning);
}

//return a char* that contains every information separated with a space
void getInfo(char* info) {
    getSystemInfo(info);

    int num = childrenList == NULL ? 0 : 1;
    bool override = getOverride();
    sprintf(info, "%s %d %d %d %d %d %d", info, num, override, beginTime.tm_hour, beginTime.tm_min, endTime.tm_hour, endTime.tm_min);
}

//sends a message at every children to delete it and return the number of deleted devices
int deleteChildren()
{
    int deletedDevices = 0;

    char command[MAX_COMMAND_LEN] = "del", response[MAX_MESSAGE_LEN];

    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        deletedDevices += atoi(response);

        close(childrenList->fdWrite);
        close(childrenList->fdRead);
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

    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);
        
        movedDevices += atoi(response);
  

        close(childrenList->fdWrite);
        close(childrenList->fdRead);
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
    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        unlinkedDevices += atoi(response);

        close(childrenList->fdWrite);
        close(childrenList->fdRead);
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

    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        switchedChildren += atoi(response);
    }
    return switchedChildren;
}

//propagate the del <id> command received from a level above
int forwardDel(char* command)
{
    int deletedDevice = 0;

    char response[MAX_MESSAGE_LEN];

    if (childrenList != NULL) 
    { 
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 1)
        {
            if(strcmp(response, NACK_STRING) != 0)
            {
                deletedDevice += atoi(responseTokens[0]);
            }
        }
        else
        {
            deletedDevice += atoi(responseTokens[1]);

            //remove the child from the connected devices list
            close(childrenList->fdWrite);
            close(childrenList->fdRead);
            deleteList(&childrenList);
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

    if (childrenList != NULL) 
    { 
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 1)
        {
            if(strcmp(responseTokens[0], NACK_STRING) != 0)
            {
                movedDevices += atoi(responseTokens[0]);
            }
        }
        else
        {
            //in every case the fact that the tokens are 2 means that the device to link was found
            if(strcmp(responseTokens[0], ACK_STRING) == 0)
            {
                //remove the child from the connected devices list
                close(childrenList->fdWrite);
                close(childrenList->fdRead);
                deleteList(&childrenList);               
            }
            movedDevices += atoi(responseTokens[1]);  

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

    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 1)
        {
            if(strcmp(response, NACK_STRING) != 0)
            {
                unlinkedDevices += atoi(responseTokens[0]);
            }
        }
        else
        {
            unlinkedDevices += atoi(responseTokens[1]);

            //remove the child from the connected devices list
            close(childrenList->fdWrite);
            close(childrenList->fdRead);
            deleteList(&childrenList);               
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

    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);


        if(strcmp(response, NACK_STRING) != 0)
        {
            switchedDevices += atoi(response);
            //not set to NULL for forward of switch pos
        }

    }
    return switchedDevices;
}

//sends command to every children or stop when found the destinatary
int forwardCommand(char *command)
{
    int res = NACK;
    char response[MAX_MESSAGE_LEN];

    if (childrenList != NULL)
    {
        write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
        read(childrenList->fdRead, response, MAX_MESSAGE_LEN);
        if(strcmp(response, NACK_STRING) != 0)
        {
            res = atoi(response);
        }
    }
    return res;
}

void getState(char *response){
    sprintf(response,"%d %s %d %02d.%02d %02d.%02d",ACK,"TIMER",((isRunning)?1:0),beginTime.tm_hour,beginTime.tm_min,endTime.tm_hour,endTime.tm_min);
}

void signalHandler(int signal){
    char *command = readFifo();
    char *response = calloc(MAX_MESSAGE_LEN, sizeof(char));

    int tokensNumber = 0;
    char **commandTokens = formatCommand(command, " ", &tokensNumber);

    if(linked){
        if(strcmp(commandTokens[0],"PRINT") == 0){
            getState(response);
        }
        else if(strcmp(commandTokens[0],"SET") == 0){
            if(isRunning){
                sprintf(response,"%d You can't set the timer when it's running!",NACK);
            }
            else{
                setTime(commandTokens[1],commandTokens[2]);
                sprintf(response,"%d",ACK);
            }
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

void setTime(char* begin, char* end) {
    double bT = atof(begin);
    double eT = atof(end);
    beginTime.tm_hour = (int)bT;
    endTime.tm_hour = (int)eT;
    beginTime.tm_min = ((int)(bT * 100)) - (beginTime.tm_hour * 100);
    endTime.tm_min = ((int)(eT * 100)) - (endTime.tm_hour * 100);

    time_t rawTime = time(NULL);
    struct tm * actualTimeInfo;
    actualTimeInfo = localtime(&rawTime);
    alarm(diffTimes(beginTime, *actualTimeInfo) + 1);
    //DEBUG:
    // FILE* fp;
    // fp = fopen("test.txt", "w+");
    // fprintf(fp, "Partirò %d secondi\n", diffTimes(beginTime, *actualTimeInfo) + 1);
    // fprintf(fp, "Andrò avanti per %d secondi\n", diffTimes(endTime, beginTime) + 1);
    // fprintf(fp, "Ripartirò dopo %d secondi\n", diffTimes(beginTime, endTime) + 1);
    // fclose(fp);
}

void timeHandler() {
    time_t rawTime = time(NULL);
    struct tm * actualTimeInfo;
    char uselessRes[MAX_MESSAGE_LEN];
    actualTimeInfo = localtime(&rawTime);
    int bT, eT, aT; //begin, end and actual time
    bT = beginTime.tm_hour * 100 + beginTime.tm_min;
    eT = endTime.tm_hour * 100 + endTime.tm_min;
    aT = actualTimeInfo->tm_hour * 100 + actualTimeInfo->tm_min;
    if(bT <= aT && aT < eT) {
        isRunning = true;
        //switch the device under this timer on
        //handle devices in override
        if(childrenList != NULL) {
            write(childrenList->fdWrite, "switch 1", MAX_COMMAND_LEN);
            read(childrenList->fdRead, uselessRes, MAX_MESSAGE_LEN);
        }
        alarm(diffTimes(endTime, beginTime) + 1);
    } else {
        isRunning = false;
        //switch the device under this timer off
        //handle devices in override
        if(childrenList != NULL) {
            write(childrenList->fdWrite, "switch 0", MAX_COMMAND_LEN);
            read(childrenList->fdRead, uselessRes, MAX_MESSAGE_LEN);
        }
        alarm(diffTimes(beginTime, endTime) + 1);
    }
}