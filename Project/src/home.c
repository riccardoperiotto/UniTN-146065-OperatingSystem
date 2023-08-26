/*
    PROJECT LIBRARY INCLUSION
*/
#include "../include/common.h"

/*
    FUNCTIONS DECLARATIONS
*/
//utility functions
int getDeviceNumber(int);
int getConnectedDevicesNumber();

//procedure functions
void linkToControlUnit();

//command execution functions
bool add(char*);
bool getDevice(int);
bool del(char*);

/*
    HOME INFORMATION
*/
//list of devices connected to the the home but not active
struct Node* childrenList = NULL;

//fd to the control unit
int controlUnitWrite, controlUnitRead;

//array used to assign the id to the devices when added to home
int deviceIds[] = {1, 1, 1, 1, 1};

//simulate the storehause disponibility
int storeHouse[] = {20, 20, 20, 20, 20};

int main(int argc, char *argv[])
{
   
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    welcomeToHome();
    linkToControlUnit();

    char commandBuffer [MAX_COMMAND_LEN];
    bool goOn = true;
    do
    {
        read(controlUnitRead, commandBuffer, MAX_COMMAND_LEN);

        int tokensNumber = 0;
        char **commandTokens = formatCommand(commandBuffer, " ", &tokensNumber);

        char response[MAX_MESSAGE_LEN]="";
        if(strcmp(commandTokens[0], "storehouse") == 0)
        {
            sprintf(response, "%d %d %d %d %d", 
                storeHouse[HUB-1],
                storeHouse[TIMER-1],
                storeHouse[BULB-1],
                storeHouse[WINDOW-1],
                storeHouse[FRIDGE-1]);          
        } else if(strcmp(commandTokens[0], "reset") == 0)
        {
            //delete every children
            char command[MAX_COMMAND_LEN] = "del", response[MAX_MESSAGE_LEN];
            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, command, MAX_COMMAND_LEN); //sends command
                read(temp->fdRead, response, MAX_MESSAGE_LEN);  //get feedback
                close(temp->fdWrite);
                close(temp->fdRead);
                temp = temp->next;
            }
            deleteList(&childrenList);
            //restore the storehouse information and the id assigner
            int i = 0;
            for(i; i<FRIDGE; i++)
            {
                deviceIds[i] = 1;
                storeHouse[i] = 20;
            }
            sprintf(response, "%d", ACK);
        } else if (strcmp(commandTokens[0], "list") == 0) 
        { 
            //if there aren't children it simply return 0
            sprintf(response, "%d", getConnectedDevicesNumber());
            write(controlUnitWrite, response, MAX_MESSAGE_LEN);

            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, commandTokens[0], MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                strcat(response, " home");
                write(controlUnitWrite, response, MAX_MESSAGE_LEN);

                //the command list on a device returns also an ACK
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                temp = temp->next;
            }
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "add") == 0)
        {
            if(add(commandTokens[1])) 
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "del") == 0)
        {
            if(del(commandBuffer))
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "lastInfo") == 0)
        {
            write(childrenList->fdWrite, "info", MAX_COMMAND_LEN);
            read(childrenList->fdRead, response, MAX_MESSAGE_LEN);
        }else if(strcmp(commandTokens[0], "info") == 0)
        {
            bool found = false;
            NodePointer temp = childrenList;
            while (temp != NULL && !found)
            {
                write(temp->fdWrite, commandBuffer, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                if(strcmp(response, NACK_STRING) != 0)
                {
                    found = true;
                    //the response is already in response
                }
                else
                {
                    temp = temp->next;
                }
            }
            if(!found)
            {
                sprintf(response, "%d", NACK);
            }
        } else if(strcmp(commandTokens[0], "type") == 0)
        {
            int type = -1;
            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, commandBuffer, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                if(strcmp(response, NACK_STRING)!= 0)
                {
                    type = atoi(response);
                    temp = NULL;
                }
                else
                {
                    temp = temp->next;
                }
            }
            sprintf(response, "%d", type);
        } else if(strcmp(commandTokens[0], "link") == 0)
        {
            bool found = false;
            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, commandBuffer, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);
                if(strcmp(response, NACK_STRING) != 0)
                {
                    found = true;
                    close(temp->fdWrite);
                    close(temp->fdRead);
                    deleteNode(&childrenList, temp->pid);
                    temp = NULL;
                }
                else
                {
                    temp = temp->next;
                }
            }
            if( found )
            {
                sprintf(response, "%d", ACK);
            }
            else
            {
                sprintf(response, "%d", NACK);
            }

        } else if(tokensNumber == 3 && strcmp(commandTokens[1], "linkto") == 0)
        {
            //the home receive the message only if the device is really connected
            char response[MAX_MESSAGE_LEN];

            NodePointer temp = childrenList;
            while (temp != NULL)
            {
                write(temp->fdWrite, commandBuffer, MAX_COMMAND_LEN);
                read(temp->fdRead, response, MAX_MESSAGE_LEN);

                int tokensNumber = 0;
                char **responseTokens = formatCommand(response, " ", &tokensNumber);

                if(tokensNumber == 2)
                {
                    if(strcmp(responseTokens[0], ACK_STRING)==0)
                    {
                        close(temp->fdWrite);
                        close(temp->fdRead);
                        deleteNode(&childrenList, temp->pid);               
                        temp = NULL;
                    }
                    else
                    {
                        temp = temp->next;
                    } 
                }
                else
                {
                    temp = temp->next;
                }
            }
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "getlink") == 0)
        {
            //GET INFORMATION ABOUT UNLINKED DEVICES
            char infoMovedDevice[MAX_MESSAGE_LEN] = "";

            //for every line on in the message queue create a new child
            while(readFromMessageQueue(MPQ_HOME, infoMovedDevice))
            {
                int infoTokensNumber = 0;
                char **infoMovedDeviceTokens = formatCommand(infoMovedDevice, " ", &infoTokensNumber);

                if(getDevice(atoi(infoMovedDeviceTokens[0])))
                {
                    char tmpBuffer[MAX_MESSAGE_LEN];

                    //send only the "setid" command and reset the default parameters
                    char setIdCommand[MAX_COMMAND_LEN];
                    sprintf(setIdCommand, "setid %s", infoMovedDeviceTokens[1]);
                    write(childrenList->fdWrite, setIdCommand, MAX_COMMAND_LEN);
                    read(childrenList->fdRead, tmpBuffer, MAX_MESSAGE_LEN);
                }
                freeAllocatedMemory(infoMovedDeviceTokens);
            }
            sprintf(response, "%d", ACK);
        } else if(strcmp(commandTokens[0], "quit") == 0)
        {
            goOn = false;
        }
        freeAllocatedMemory(commandTokens);

        write(controlUnitWrite, response, MAX_MESSAGE_LEN);
    } while(goOn);

    return 0;
}

/*
    FUNCTIONS DEFINITIONS
*/
//UTILITY FUNCTIONS
//create the device id
int getDeviceNumber(int type)
{
    return deviceIds[type-1]++;
}

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

//PROCEDURE FUNCTIONS
//link the home to the control unit and save the pipe information
void linkToControlUnit()
{
    int fd1[2]; int fd2[2];
    pipe (fd1);
    pipe(fd2);

    pid_t childPid = spawnWithPipe("./bin/controlUnit", fd1, fd2);
    close(fd1[READ]);
    close(fd2[WRITE]);
    controlUnitWrite = fd1[WRITE];
    controlUnitRead = fd2[READ];
}

//COMMAND EXECUTION FUNCTIONS
//adds a <device> (args[0]) to the home
bool add(char* device)
{
    bool res = true;

    //control if the device is avalable in the storehause
    int type = stringToType(device);
    
    if(storeHouse[type-1] > 0)
    {
        storeHouse[type-1]--;

        char programName [MAX_PATH_LEN] = "";
        sprintf(programName, "./bin/%s", device);

        int fd1[2]; int fd2[2];
        if (pipe (fd1))
        {
            res = false;
        }
        if(pipe(fd2))
        {
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
                close(fd1[READ]);
                close(fd2[WRITE]);
                push(&childrenList, childPid, fd1[WRITE], fd2[READ]);

                //set the id of the device just insterted
                char setIdCommand[MAX_COMMAND_LEN];
                sprintf(setIdCommand, "setid %s%d", device, getDeviceNumber(type));
                write(fd1[WRITE], setIdCommand, MAX_COMMAND_LEN);

                //get the feeback
                char tmpBuffer[MAX_MESSAGE_LEN];
                read(fd2[READ], tmpBuffer, MAX_MESSAGE_LEN);

                if(strcmp(tmpBuffer, NACK_STRING) == 0)
                {
                    res = false;
                }
            }
        }
        else
        {
            res = false;
        }
    }
    else
    {
        res = false;
    }
    return res;
}

//re-link a device of type args[0] to the home
bool getDevice (int type)
{
    bool res = true;

    char file[MAX_SINGLE_INFO_LEN] ="";
    typeToFile(type, file);

    char programName [MAX_PATH_LEN] = "";
    sprintf(programName, "./bin/%s", file);

    int fd1[2]; int fd2[2];
    if (pipe (fd1))
    {
        res = false;
    }
    if(pipe(fd2))
    {
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
    return res;
}

//remove the device <id> 
bool del(char* command)
{   
    bool res = false;

    char response[MAX_MESSAGE_LEN];

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);

        if(tokensNumber == 2)
        {
            res = true;

            //remove the child from the connected devices list
            close(temp->fdWrite);
            close(temp->fdRead);
            deleteNode(&childrenList, temp->pid);
            temp = NULL;
        }
        else
        {
            temp = temp->next;
        }
        freeAllocatedMemory(responseTokens);
    }
    return res;
}


