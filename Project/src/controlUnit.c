/*
    PROJECT LIBRARY INCLUSION
*/
#include "../include/common.h"
/*
    FUNCTIONS DECLARATIONS
*/
//utility functions
bool processCommand(char *, bool);
bool controlSwitch(char *);
void int_Handler(int);

//print functions
void printGreetings();
void printMenu();
void printSwitch();
void printInfo(char **);
void printEntry(char *, bool);

//command execution functions
void setupSystem();
void resetSystem();
void storehouseList();
void homeList();
void systemList();
bool addToHome(char *);
bool getDevice(int);
int del(char*);
int linkDevices(char *, char *);
int linkToMe(char *);
int unlinkFromMe(char *);
int switchDevice(char *);
bool getInfo(char*, bool);
void set(char*);
int forwardCommand(char *);

/*
    CONTROL UNIT INFORMATION
*/
//home pipe descriptors
int homeWrite, homeRead;

//command descriptions
int numberOfCommand = 18;
char** commandsDescription [] =
{

    (char *[]){BOLDYELLOW"|"NC"\tlist", "shows all the available devices\t\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tsetup", "set up a base system configuration\t\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\treset", "reset the system to the initial configuration\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tstorehouse", "shows the available devices\t\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tadd <device>", "adds a <device> to the system and shows its details\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tdel <id>", "remove the device <id> with cascading effect\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tlink <id> to <id>", "connects two devices toghether\t\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tlink <id>", "connects the device <id> to the Control Unit\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tunlink <id>", "disconnects the device <id> from the Control Unit\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tswitch sw-general <pos>", "change the state to <pos> of the entire system\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tswitch <id> <label> <pos>", "modify the switch <label> of <id> to <pos>\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tswitch ?", "to get more info about switch\t\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tinfo <id>", "shows the details of the device <id>\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tset <id> <begin> <end>", "sets times for timer <id>, The format must be \"12.35\"\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tsetdelay <id> to <value>", "sets the automatic delay time of the fridge with <id>\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tsettemp <id> to <value>", "sets the temperature of the fridge with <id>\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tclear", "clears the console\t\t\t\t\t\t"BOLDYELLOW"|"NC},
    (char *[]){BOLDYELLOW"|"NC"\tquit", "close the control unit\t\t\t\t\t"BOLDYELLOW"|"NC},
};

//switches name
int numberOfSwitch = 5;
char* switchName[] ={
    (char *){"sw-state"},
    (char *){"sw-bulb"},
    (char *){"sw-wopen"},
    (char *){"sw-wclose"},
    (char *){"sw-fopen"}
};
    //PS: sw-general is not in this list but it's correct for the system

//available devices
char* devices[] = {"hub", "timer", "bulb", "window", "fridge"};

//the switch for the entire system
bool swGeneral = true;

//define if the system is in the initial configuration
bool initialConfiguration = false;

//the general switch for the whole system
bool generalSwitch = false;

//define if the whole system is on or off
bool state = true;

//represent the number of the directly connected devices
int num = 0;

//list of connected devices
struct Node* childrenList = NULL;

//define if go on with the commands reading
bool goOn = false;

int main(int argc, char *argv[])
{
    signal(SIGINT, int_Handler);
    signal(SIGCHLD, SIG_IGN);
    if(argc != 3)
    {
        return 0;
    }
    homeWrite = atoi(argv[1]);
    homeRead = atoi(argv[2]);

    initialConfiguration = true;
    //signal();
    printGreetings();

    char inputBuffer [MAX_COMMAND_LEN];
    char lowerInputBuffer [MAX_COMMAND_LEN] = "";
    goOn = true;
    do
    {

        //variable used to understand the result of procedure call
        int resultCode = readLine(inputBuffer, sizeof(inputBuffer), true);
        strcpy(lowerInputBuffer, toLower(inputBuffer));
        switch (resultCode)
        {
            case TOO_LONG:
                printf (BOLDRED"Input too long [%s]. Type something else\n"NC, inputBuffer);
                break;
            case OK:
                ; bool res = processCommand(lowerInputBuffer, true);
                if(!res)
                {
                    printf(BOLDRED"Command not found\n"NC);
                }
                break;
        }
    } while(goOn);

    return 0;
}

/*
    FUNCTIONS DEFINITIONS
*/
//UTILITY FUNCTIONS
//process a string that can represents a command
bool processCommand(char *commandBuffer, bool print)
{
    bool res = true;
    int tokensNumber = 0;
    char **commandTokens;

    commandTokens = formatCommand(commandBuffer, " ", &tokensNumber);

    //the command with the '*' symbol are the one reported on the pdf
    if (strcmp(commandTokens[0], "help") == 0 && tokensNumber == 1) 
    {
        printMenu();
    }  else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 3
        && (strcmp(commandTokens[1], "sw-general") == 0))
    {
        if((strcmp(commandTokens[2], "on") == 0 || strcmp(commandTokens[2], "off") == 0))
        {
            bool newState = stringToState(commandTokens[2]);
            if(state != newState)
            {
                printf(BOLDGREEN"The system is now %s\n"NC, commandTokens[2]);
                swGeneral = newState;   //first change the switch
                state = swGeneral;      //then change the state
            }
            else
            {
                printf(BOLDGREEN"The system is already %s\n"NC, commandTokens[2]);
            }
        }
        else
        {
            printf(BOLDRED"The <pos> must be \"on\" or \"off\" and nothing else\n"NC);
            res = false;
        }
    } else if (strcmp(commandTokens[0], "quit") == 0 && tokensNumber == 1)
    {
        char tmpBuffer[MAX_MESSAGE_LEN];
        write(homeWrite, commandBuffer, MAX_COMMAND_LEN);
        read(homeRead, tmpBuffer, MAX_MESSAGE_LEN);
        goOn = false;
    } 
    else if (strcmp(commandTokens[0], "clear") == 0 && tokensNumber == 1) {
        clearScreen();
    }
    else if(!state)
    {
        printf("\n");
        printf(BOLDRED"The system is off at the moment\n"NC);
        printf("In this state the only command available are:\n");
        printf(BOLDCYAN);
        printf("\t -\"help\"\n");
        printf("\t -\"switch sw-general <pos>\"\n");
        printf("\t -\"clear\"\n");
        printf("\t -\"quit\"\n");  
        printf(NC); 
        printf(BOLDYELLOW"To switch on the system type command: \"switch sw-general on\"\n"NC);
        printf("\n");
    } else if(strcmp(commandTokens[0], "setup") == 0 && tokensNumber == 1)
    {
        if(initialConfiguration)
        {
            setupSystem();
            printf("\n");
            printf(BOLDGREEN"System configured successfully. Type \"list\" to see the situation.\n"NC);
        }
        else
        {
            printf("\n");
            printf(BOLDRED"System already in use. Type \"reset\" command before use this one\n"NC);
        }
        printf("\n");        
    } else if(strcmp(commandTokens[0], "reset") == 0)
    {
        resetSystem();
        printf("\n");
        printf(BOLDGREEN"System resetted\n"NC);
        printf("\n");
    } else if(strcmp(commandTokens[0], "storehouse") == 0)
    {
        storehouseList();                                       
    } else if(strcmp(commandTokens[0], "list") == 0 && tokensNumber == 1) {
        printf("\n");
        printf("THIS IS THE SITUATION NOW \n");
        homeList();
        systemList();
    } else if(strcmp(commandTokens[0], "add") == 0 && tokensNumber == 2) //*
    {
        if(addToHome(commandTokens[1]))
        {
            initialConfiguration = false;
            if(print)
            {
                printf(BOLDGREEN"%s inserted successfully\n"NC, commandTokens[1]);

                char info[MAX_MESSAGE_LEN];

                write(homeWrite, "lastInfo", MAX_COMMAND_LEN);
                read(homeRead, info, MAX_MESSAGE_LEN);

                int tokensNumber = 0;
                char** infoTokens = formatCommand(info, " ", &tokensNumber);
                printInfo(infoTokens);

                freeAllocatedMemory(infoTokens);
            }
        }
        else
        {
            printf(BOLDRED"Nothing inserted. Type \"storehause\" to see the availble devices.\n"NC);
        }
    } else if(strcmp(commandTokens[0], "del") == 0 && tokensNumber == 2) //*
    {
        int deletedDevices = del(commandTokens[1]);
        if(deletedDevices > 0)
        {
            if(deletedDevices == 1)
            {
                printf(BOLDGREEN"%d device deleted successfully\n"NC, deletedDevices);
            }
            else
            {
                printf(BOLDGREEN"%d devices deleted successfully\n"NC, deletedDevices);
            }
        }
        else
        {
            printf(BOLDRED"Deletion not done\n"NC);
        }
    } else if(strcmp(commandTokens[0], "link") == 0 && tokensNumber == 4) //*
    {
        int res = linkDevices(commandTokens[1], commandTokens[3]);
        if(print)
        {
            if(res > 0)
            {
                printf(BOLDGREEN"Device %s and %s linked successfully\n"NC, commandTokens[1], commandTokens[3]);
                if(res == 1)
                {
                    printf(BOLDGREEN"%d device has moved successfully\n"NC, res);
                }
                else
                {
                    printf(BOLDGREEN"%d devices have moved successfully\n"NC, res);
                }
            } 
            else
            {
                switch (res)
                {
                    case -1:
                        printf(BOLDRED"Error occurred during linking\n"NC);
                        printf(BOLDRED"A device cannot be linked to itself\n"NC);
                        break;
                    case -2:
                        printf(BOLDRED"Error occurred during linking\n"NC);
                        printf(BOLDRED"Device with id %s not found\n"NC, commandTokens[1]);
                        break;
                    case -3:
                        printf(BOLDRED"Error occurred during linking\n"NC);
                        printf(BOLDRED"Device with id %s not found\n"NC, commandTokens[3]);
                        break;
                    case -4:
                        printf(BOLDRED"Error occurred during linking\n"NC);
                        printf(BOLDRED"The reason reguards the type and the control type of the devices\n"NC);
                        break;
                    case -5:
                        printf(BOLDRED"Error occurred during linking\n"NC);
                        printf(BOLDRED"Is not possible link %s to %s for loop reason in the hierarchy\n"NC, commandTokens[1], commandTokens[3]);
                        break;
                    default:
                        printf(BOLDRED"Error occurred during linking\n"NC);
                        break;
                }
            }
        }
    } else if(strcmp(commandTokens[0], "link") == 0 && tokensNumber == 2) {
        //the "link id" command connect the device to the control unit
        int movedDevices = linkToMe(commandTokens[1]);
        if (print)
        {
            if(movedDevices > 0)
            {
                printf(BOLDGREEN"Device %s linked succefully to the control unit\n"NC, commandTokens[1]);
                if(movedDevices == 1)
                {
                    printf(BOLDGREEN"%d device has moved successfully\n"NC, movedDevices);
                }
                else
                {
                    printf(BOLDGREEN"%d devices have moved successfully\n"NC, movedDevices);
                }
            }
            else
            {
                printf(BOLDRED"Error occurred during linking\n"NC);
            }
        }
    } else if(strcmp(commandTokens[0], "unlink") == 0 && tokensNumber == 2)
    {
        int unlinkedDevices = unlinkFromMe(commandTokens[1]);
        if(unlinkedDevices > 0)
        {
            printf(BOLDGREEN"Device %s unlinked successfully\n"NC, commandTokens[1]);
            if(unlinkedDevices == 1)
            {
                printf(BOLDGREEN"%d device unlinked successfully\n"NC, unlinkedDevices);
            }
            else
            {
                printf(BOLDGREEN"%d devices unlinked successfully\n"NC, unlinkedDevices);
            }
        }
        else
        {
            printf(BOLDRED"Error occurred during unlinking\n"NC);
        }
    } else if(strcmp(commandTokens[0], "switch") == 0 && tokensNumber == 4
        && ((strcmp(commandTokens[3], "on") == 0 || strcmp(commandTokens[3], "off") == 0)))
    {
        if(controlSwitch(commandTokens[2]))
        {
            //forward the command to the system
            int switchedDevices = switchDevice(commandBuffer);
            if(switchedDevices > 0)
            {
                printf(BOLDGREEN"Changed the position of switch %s of device %s to %s\n"NC, commandTokens[2], commandTokens[1], commandTokens[3]);
                if(switchedDevices == 1)
                {
                    printf(BOLDGREEN"%d device has switched successfully\n"NC, switchedDevices);
                }
                else
                {
                    printf(BOLDGREEN"%d devices have switched successfully\n"NC, switchedDevices);
                }
            }
            else
            {
                printf(BOLDRED"Device with id %s not found or has not switch %s.\n"NC, commandTokens[1], commandTokens[2]);
                printf("Use command \"link <id>\" to link a device to the control unit.\n");
                printf("Use command \"list\" to show the available device.\n");
                printf("Use command \"switch ?\" to show the format of switch.\n");
            }
        }
        else
        {
            printf(BOLDRED"There isn't a switch with label %s or <pos> has no sense \n"NC, commandTokens[2]);
            printf("Use the \"switch ?\" command to see the avalable option\n");
        }
    } else if((strcmp(commandTokens[0], "switch") == 0) && tokensNumber == 2 && (strcmp(commandTokens[1], "?")== 0))
    {
        printSwitch();
    } else if(strcmp(commandTokens[0], "info") == 0 && tokensNumber == 2)
    {
        if(!getInfo(commandTokens[1], true))
        {
            printf(BOLDRED"Device with id \"%s\" not found\n"NC, commandTokens[1]);
        }
    } else if (strcmp(commandTokens[0], "set") == 0 && tokensNumber == 4) {
        if(checkTimerSet(commandTokens[2], commandTokens[3]) && getInfo(commandTokens[1], false))
            set(commandBuffer);
        else
            printf(BOLDRED"set command not in a correct form\n"NC);
    } else if (strcmp(commandTokens[0], "setdelay") == 0 && tokensNumber == 4 && atof(commandTokens[3]) != 0.0) {
        if(forwardCommand(commandBuffer) != NACK)
        {
            printf(BOLDGREEN"Delay of fridge %s setted successfully\n"NC, commandTokens[1]);
        }
        else
        {
            printf(BOLDRED"Error occured while setting delay\n"NC);
        }
    } else if (strcmp(commandTokens[0], "settemp") == 0 && tokensNumber == 4) {
        if(atoi(commandTokens[3]) != 0.0 && forwardCommand(commandBuffer) != NACK)
        {
            printf(BOLDGREEN"Temperature of fridge %s setted successfully to %s\n"NC, commandTokens[1], commandTokens[3]);
        }
        else
        {
            printf(BOLDRED"Not a valid temperature. It must be an integer beetween -10 and 10\n"NC);
        }
    }  
    else {
        res = false;
    }
    freeAllocatedMemory(commandTokens);
    return res;
}

//verify if the name of the specyfied switch has sense       
bool controlSwitch(char *label)
{
    bool res = false;
    int i;
    for(i=0; i<numberOfSwitch && !res; i++)
    {
        if(strcmp(label, switchName[i]) == 0)
        {
            res = true;
        }
    }
    return res;
}

//manage the "Ctrl+c" command
void int_Handler(int sig)
{
    NodePointer temp = childrenList;
    while (temp != NULL) 
    {  
        close(temp->fdWrite);
        close(temp->fdRead);
        temp = temp->next; 
    } 
    close(homeWrite);
    close(homeRead);

    deleteList(&childrenList);
    printf(BOLDMAGENTA"\nBYE BYE\n\n"NC);

    goOn = false;
    sleep(1);
    exit(0);
}
/*
    PRINT FUNCTIONS
*/
//prints the available list of command
void printMenu()
{
    printf("Here's the list of available command! Just type it ;)\n");
    int i;
    printf(BOLDYELLOW"------------------------------------------------------------------------------------------------\n"NC);
    printf(BOLDYELLOW"|"NC"\t\t\t\t\t\t\t\t\t\t\t\t"BOLDYELLOW"|"NC"\n");
    for(i = 0; i < numberOfCommand; i++)
    {
        printf("%-40s: %s \n", commandsDescription[i][0], commandsDescription[i][1]);
    }
    printf(BOLDYELLOW"|"NC"\t\t\t\t\t\t\t\t\t\t\t\t"BOLDYELLOW"|"NC"\n");
    printf(BOLDYELLOW"------------------------------------------------------------------------------------------------\n"NC);
}

//prints the details of the switch command
void printSwitch()
{
    printf("\n");
    printf(BOLDMAGENTA"That's the list of devices' switch associated with the corrispective type! Just use it ;)\n"NC);
    printf("\n");
    printf(BOLDBLUE"\"sw-general\" "NC "\t of Control-Unit: \tswitch to <on> or <off> the entire system\n");
    printf(BOLDBLUE"\"sw-state\" "NC " \t of HUB devices: \tswitch to <on> or <off> the hub and every connected device\n");
    printf(BOLDBLUE"\"sw-bulb\" "NC "\t of HUB devices: \tswitch to <on> or <off> all the bulbs under the hub\n");
    printf(BOLDBLUE"\"sw-wopen\" "NC "\t of HUB devices: \tswitch to <on> the open switch of every window under the hub\n");
    printf(BOLDBLUE"\"sw-wclose\" "NC "\t of HUB devices: \tswitch to <on> the close switch of every window under the hub\n");
    printf(BOLDBLUE"\"sw-fopen\" "NC "\t of HUB devices: \topen or close every fridge under the hub\n");
    printf(BOLDBLUE"\"sw-bulb\" "NC "\t of BULB devices: \tswitch to <on> or <off> the bulb\n");
    printf(BOLDBLUE"\"sw-wopen\" "NC "\t of WINDOW devices: \tswitch to <on> or <off> the opening switch of the window\n");
    printf(BOLDBLUE"\"sw-wclose\" "NC "\t of WINDOW devices: \tswitch to <on> or <off> the closing switch of the window\n");
    printf(BOLDBLUE"\"sw-fopen\" "NC "\t of FRIDGE devices: \tswitch tfo <on> or <off> the switch of the fridge\n");
    printf("\n");
}

//prints the information of the device
void printInfo(char **infoTokens)
{
    //process data to print
    int type = atoi(infoTokens[0]);
    char typeString[MAX_SINGLE_INFO_LEN] = "";
    typeToString(type, typeString);
    char state[MAX_SINGLE_INFO_LEN] = "";
    stateToString(atoi(infoTokens[0]), state, infoTokens[3]);

    //print system data coomon to every device
    printf("\n");
    printf("\tType:\t\t\t%s\n", typeString);
    printf("\tDevice id:\t\t%s\n", infoTokens[1]);
    printf("\tProcess pid:\t\t%s\n", infoTokens[2]);

    char override[MAX_SINGLE_INFO_LEN]="";

    //print specific device data
    switch (type)
    {
        case HUB:
            printf("\tConn. devices:\t\t%s\n", infoTokens[4]);
            overrideToString(override, infoTokens[5]);
            printf("\tOverrided:\t\t%s\n", override);
            if(strcmp(infoTokens[6], "-1") != 0)
            {
                char stateBulbs[MAX_SINGLE_INFO_LEN] = "";
                stateToString(BULB, stateBulbs, infoTokens[6]);
                printf("\tBulbs state:\t\t%s\n", stateBulbs);
                printf("\tTime on bulbs:\t\t%s\n", infoTokens[7]);
            }
            if(strcmp(infoTokens[8], "-1") != 0)
            {
                char stateWindows[MAX_SINGLE_INFO_LEN] = "";
                stateToString(WINDOW, stateWindows, infoTokens[8]);
                printf("\tWindows state:\t\t%s\n", stateWindows);
                printf("\tTime open windows:\t%s\n", infoTokens[9]);
            }
            if(strcmp(infoTokens[10], "-1") != 0)
            {
                printf("\tFridges:\t\t");
                if(strcmp(infoTokens[10], "0") == 0)
                {
                    printf(BOLDGREEN"all closed\n"NC);
                }
                else
                {
                    printf(BOLDRED"not all closed\n"NC);
                }
            }
            break;
        case TIMER:
            printf("\tActive status:\t\t%s\n", state); 
            printf("\tConn. devices:\t\t%s\n", infoTokens[4]);
            overrideToString(override, infoTokens[5]);
            printf("\tOverrided:\t\t%s\n", override); 
            printf("\tBegin time:\t\t%02d.%02d\n", atoi(infoTokens[6]), atoi(infoTokens[7]));
            printf("\tEnd time:\t\t%02d.%02d\n", atoi(infoTokens[8]), atoi(infoTokens[9]));
            break;
        case BULB:
            printf("\tState:\t\t\t%s\n", state);
            printf("\tTime on:\t\t%s\n", infoTokens[4]);
            break;
        case WINDOW:
            printf("\tState:\t\t\t%s\n", state);
            stateToString(-1, state, infoTokens[4]);
            printf("\tsw-wopen:\t\t%s\n", state);
            stateToString(-1, state, infoTokens[5]);
            printf("\tsw-wclose:\t\t%s\n", state);
            printf("\tTime open:\t\t%s\n", infoTokens[6]);
            break;
        case FRIDGE:
            printf("\tState:\t\t\t%s\n", state);
            printf("\tTime open:\t\t%s\n", infoTokens[4]);
            printf("\tFilling state:\t\t%s\n", infoTokens[5]);
            printf("\tDelay time:\t\t%s\n", infoTokens[6]);
            printf("\tTemperature:\t\t%s°C\n", infoTokens[7]);
            break;
        default:
            break;
    }
    printf("\n");
}

//print a entry of the list table
void printEntry(char *info, bool active)
{
    //info preprocessing
    int infoTokensNumber = 0;
    char **infoTokens = formatCommand(info, " ", &infoTokensNumber);

    char typeString[MAX_SINGLE_INFO_LEN] = "", state[MAX_SINGLE_INFO_LEN] = "";
    typeToString(atoi(infoTokens[0]), typeString);  

    if(!active)
    {
        strcpy(state, "--");
    }
    else
    {
        stateToString(atoi(infoTokens[0]), state, infoTokens[3]);
    }
    
    //info printing
    printf("%s\t\t%s\t\t%s\t\t%s\t\t", typeString, infoTokens[1], infoTokens[2], state);
    if(infoTokensNumber == 5)
    {
        printf("%s", infoTokens[4]);
    } else {
        printf("Control-Unit");
    }
    printf("\n");

    freeAllocatedMemory(infoTokens);
}

//COMMAND EXECUTION FUNCTIONS
//begin a configuration commands procedure from file
void setupSystem()
{
    FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
 
	stream = fopen(SETUP_FILE_PATH, "r");
	if (stream == NULL)
		exit(EXIT_FAILURE);
 
	while ((read = getline(&line, &len, stream)) != -1) {
        if(line[0] != '\n')
        {
            char *pos;
            if ((pos = strchr(line, '\n')) != NULL)
            {
                *pos = '\0';
            }
        }
		processCommand(line, false);
        
	}
	free(line);
	fclose(stream);

    //system set up done!
    initialConfiguration = false;
}

//resrt the system to the initial condition
void resetSystem()
{
    //delete every children in the system
    char command[MAX_COMMAND_LEN] = "del", response[MAX_MESSAGE_LEN];
    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);  //get feedback
        close(temp->fdWrite);
        close(temp->fdRead);
        temp = temp->next;
    }
    deleteList(&childrenList);
    num = 0;

    //reset also the home configuration
    strcpy(command, "reset");
    write(homeWrite, command, MAX_COMMAND_LEN);
    read(homeRead, response, MAX_MESSAGE_LEN);  //get feedback

    //reset done!
    initialConfiguration = true;
}

//prints the available devices in the storehouse
void storehouseList()
{
    printf("\n");

    //sends the list command to home
    char command[MAX_COMMAND_LEN] = "storehouse";
    write(homeWrite, command, MAX_COMMAND_LEN);

    //reads the response
    char response[MAX_MESSAGE_LEN];
    read(homeRead, response, MAX_MESSAGE_LEN);

    //formats the info
    int tokensNumber = 0;
    char **responseTokens = formatCommand(response, " ", &tokensNumber);

    printf("The available devices are:\n");
    printf("\n");
    printf(BOLDGREEN);
    if(atoi(responseTokens[HUB-1]) >= 15){
        printf("%s\thubs\n", responseTokens[HUB-1]);
    }
    if(atoi(responseTokens[TIMER-1]) >= 15){
        printf("%s\ttimers\n", responseTokens[TIMER-1]);
    }
    if(atoi(responseTokens[BULB-1]) >= 15){
        printf("%s\tbulbs\n", responseTokens[BULB-1]);
    }
    if(atoi(responseTokens[WINDOW-1]) >= 15){
        printf("%s\twindows\n", responseTokens[WINDOW-1]);
    }
    if(atoi(responseTokens[FRIDGE-1]) >= 15){
        printf("%s\tfridges\n", responseTokens[FRIDGE-1]);
    }
    printf(NC);
    printf(BOLDYELLOW);
    if(atoi(responseTokens[HUB-1]) < 15 && atoi(responseTokens[HUB-1]) > 5){
        printf("%s\thubs\n", responseTokens[HUB-1]);
    }
    if(atoi(responseTokens[TIMER-1]) < 15 && atoi(responseTokens[TIMER-1]) > 5){
        printf("%s\ttimers\n", responseTokens[TIMER-1]);
    }
    if(atoi(responseTokens[BULB-1]) < 15 && atoi(responseTokens[BULB-1]) > 5){
        printf("%s\tbulbs\n", responseTokens[BULB-1]);
    }
    if(atoi(responseTokens[WINDOW-1]) < 15 && atoi(responseTokens[WINDOW-1]) > 5){
        printf("%s\twindows\n", responseTokens[WINDOW-1]);
    }
    if(atoi(responseTokens[FRIDGE-1]) < 15 && atoi(responseTokens[FRIDGE-1]) > 5){
        printf("%s\tfridges\n", responseTokens[FRIDGE-1]);
    }
    printf(NC);
    printf(BOLDRED);
    if(atoi(responseTokens[HUB-1]) < 5){
        printf("%s\thubs\n", responseTokens[HUB-1]);
    }
    if(atoi(responseTokens[TIMER-1]) < 5){
        printf("%s\ttimers\n", responseTokens[TIMER-1]);
    }
    if(atoi(responseTokens[BULB-1]) < 5){
        printf("%s\tbulbs\n", responseTokens[BULB-1]);
    }
    if(atoi(responseTokens[WINDOW-1]) < 5){
        printf("%s\twindows\n", responseTokens[WINDOW-1]);
    }
    if(atoi(responseTokens[FRIDGE-1]) < 5){
        printf("%s\tfridges\n", responseTokens[FRIDGE-1]);
    }
    printf(NC);
    printf("\n"); 
    freeAllocatedMemory(responseTokens);
}

//prints the devices added and still in home
void homeList()
{
    printf("\n");
    //sends the list command to home
    char command[MAX_COMMAND_LEN] = "list";
    write(homeWrite, command, MAX_COMMAND_LEN);

    //reads the response
    char response[MAX_MESSAGE_LEN];
    read(homeRead, response, MAX_MESSAGE_LEN);

    int homeDevicesNumber = atoi(response);

    printf("\033[0;31mThe not active devices are: %d\033[0m\n", homeDevicesNumber);

    if(homeDevicesNumber > 0)
    {
        printf("Here they are: \n");
        printf("\n");
        printf("TYPE\t\tID\t\tPID\t\tSTATE\t\tParent ID\n");

        //reads every device and print it
        read(homeRead, response, MAX_MESSAGE_LEN);
        do
        {
            printEntry(response, false);
            read(homeRead, response, MAX_MESSAGE_LEN);
        } while (strcmp(response, ACK_STRING)!= 0);
    }
    else
    {
        printf("\n");
        printf("Type \"add <device>\" to add a device to the home \n");

        //clear the pipe
        read(homeRead, response, MAX_MESSAGE_LEN);
    }
    printf("\n");
}

//shows all the available devices 
void systemList()
{
    printf("\n\n");
    printf("\033[0;32mThe active devices are: %d\033[0m\n", num);
    
    if(num != 0)
    {
        printf("Here they are: \n");
        printf("\n");
        printf("TYPE\t\tID\t\tPID\t\tSTATE\t\tParent ID\n");

        char command[MAX_COMMAND_LEN] = "list";
        char info[MAX_MESSAGE_LEN];

        NodePointer temp = childrenList;
        while (temp != NULL) 
        {
            write(temp->fdWrite, command, MAX_COMMAND_LEN);
            temp = temp->next;
        }

        temp = childrenList;
        while (temp != NULL) 
        {
            bool cont = true;
            while(cont)
            {
                read(temp->fdRead, info, MAX_MESSAGE_LEN);
                if(strcmp(info, ACK_STRING) != 0) {
                    printEntry(info, true);
                } else {
                    cont = false;
                }
            }
            temp = temp->next;
        }
    }
    else
    {
        printf("\n");
        printf("Type \"link <device>\" to add a device to the control-unit \n");  
    }
    printf("\n"); 
}

//adds (or we can say: "call the function to add") a <device> (args[0]) to the system and shows its details
bool addToHome(char* device)
{
    bool res = false;
    int type;
    for (type = 0; type < 5 && !res; type++)
    {
        if(strcmp(device, devices[type]) == 0)
        {
            res = true;
        }
    }
    if(res)
    {
        char command[MAX_COMMAND_LEN];
        sprintf(command, "add %s", device);
        write(homeWrite, command, MAX_COMMAND_LEN);

        char response[MAX_MESSAGE_LEN];
        read(homeRead, response, MAX_MESSAGE_LEN);
        if(strcmp(response, NACK_STRING) == 0)
        {
            res = false;
        }
    }
    return res;
}

//brings the information about a device of the system
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

//remove the device <id> (args[0]) with cascading effect
int del(char* id)
{
    int deletedDevices = 0;

    char command[MAX_COMMAND_LEN], response[MAX_MESSAGE_LEN];
    sprintf(command, "del %s", id);

    //sends the command to home
    write(homeWrite, command, MAX_COMMAND_LEN);
    read(homeRead, response, MAX_MESSAGE_LEN);
    if(strcmp(response, ACK_STRING)==0)
    {
        deletedDevices = 1;
    }
    else
    {
        //sends the command to the system
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
                    deletedDevices += atoi(responseTokens[0]);
                    temp = NULL;
                }
                else
                {
                    temp = temp->next;
                }
            }
            else
            {
                deletedDevices += atoi(responseTokens[1]);

                //remove the child from the connected devices list
                close(temp->fdWrite);
                close(temp->fdRead);
                deleteNode(&childrenList, temp->pid);
                temp = NULL;
            }
            freeAllocatedMemory(responseTokens);
        }
        num -= deletedDevices;
    }
    return deletedDevices;
}

//try to connects two devices (idDevice1 to idDevice2) and return the result
int linkDevices(char* idDevice1, char* idDevice2)
{
    //.. a link beetween a device and itself is not possible
    if(strcmp(idDevice1, idDevice2)==0)
    {
        return -1;
    }
    int movedDevices = 0;

    char command[MAX_COMMAND_LEN], response[MAX_MESSAGE_LEN];
    NodePointer temp = NULL;

    //search the device1 type...
    bool isDev1Active = true;
    int typeDevice1 = -1;
    sprintf(command, "type %s", idDevice1);

    //.. in home
    write(homeWrite, command, MAX_COMMAND_LEN);
    read(homeRead, response, MAX_MESSAGE_LEN);
    if(strcmp(response, NACK_STRING) != 0)
    {
        //if the device is found by home it's not active right now
        isDev1Active = false;    
        
        typeDevice1 = atoi(response);
    }

    //.. in the system
    if(typeDevice1 == -1)
    {
        temp = childrenList;
        while (temp != NULL) 
        {
            write(temp->fdWrite, command, MAX_COMMAND_LEN);
            read(temp->fdRead, response, MAX_MESSAGE_LEN);
            if(strcmp(response, NACK_STRING) != 0)
            {
                typeDevice1 = atoi(response);
                temp = NULL;
            }
            else
            {
                temp = temp->next;
            }
        }
    }

    //the idDevice1 exist and is of type typeDevice1
    if(typeDevice1 != -1) 
    {        
        //search the device2 controlled device type
        //idDevice2 must be in the system to continue the link procedure
        int controlledTypeDevice2 = -1;
        sprintf(command, "controltype %s", idDevice2);
        temp = childrenList;
        while (temp != NULL) 
        {
            write(temp->fdWrite, command, MAX_COMMAND_LEN);
            read(temp->fdRead, response, MAX_MESSAGE_LEN);
            if(strcmp(response, NACK_STRING) != 0) {
                controlledTypeDevice2 = atoi(response);
                temp = NULL;
            } else {
                temp = temp->next;
            }
        }
        if(controlledTypeDevice2 == -1)
        {
            //idDevice2 not found
            return -3;
        }
        
        //verify if the link is possible for type reason
        if(typeDevice1 == controlledTypeDevice2 || controlledTypeDevice2 == EMPTY_CONTROL_DEVICE) 
        {
            if(!isDev1Active)
            {
                //if dev1 is not active there is not loop problem and we can simply send the command to home
                sprintf(command, "%s linkto %s", idDevice1, idDevice2);
                write(homeWrite, command, MAX_COMMAND_LEN);
                read(homeRead, response, MAX_MESSAGE_LEN); //it's an ACK for sure
                movedDevices = 1; //the device connected to home has at most one level of chain deep
                num += movedDevices;
            }
            else
            {
                //verify loop situation
                sprintf(command, "loop %s %s", idDevice1, idDevice2);
                temp = childrenList;
                while (temp != NULL)
                {
                    write(temp->fdWrite, command, MAX_COMMAND_LEN);
                    read(temp->fdRead, response, MAX_MESSAGE_LEN);
                    if(strcmp(response, ACK_STRING) == 0)
                    {
                        //linking not possible for loop reason
                        return -5;
                    }
                    else
                    {
                        temp = temp->next; 
                    }
                }

                //if dev1 is active and there's not loop situation we can go on sending the command to the system
                sprintf(command, "%s linkto %s", idDevice1, idDevice2);
                temp = childrenList;
                while (temp != NULL)
                {
                    write(temp->fdWrite, command, MAX_COMMAND_LEN);
                    read(temp->fdRead, response, MAX_MESSAGE_LEN);

                    int tokensNumber = 0;
                    char **responseTokens = formatCommand(response, " ", &tokensNumber);
                    if(tokensNumber == 2) 
                    {
                        //the children queried is the one to move
                        if(strcmp(responseTokens[0], ACK_STRING) == 0) {
                            close(temp->fdWrite);
                            close(temp->fdRead);
                            deleteNode(&childrenList, temp->pid);
                        }

                        //if the responseTokens[0] is different from 0 a device in the branch has moved
                        if(strcmp(responseTokens[1], "0") != 0) {
                            movedDevices = atoi(responseTokens[1]);
                            temp = NULL;
                        } else {
                            temp = temp->next;
                        }
                    }
                    else
                    {
                        //printf("Di qua non passo mai\n");
                        temp = temp->next; 
                    }
                    freeAllocatedMemory(responseTokens);
                }
            }
            //the getlink should never return an error
            if(movedDevices > 0)
            {
                //the command is going to be executed by dev2 and its (new) children
                sprintf(command, "getlink %s", idDevice2);
                temp = childrenList;
                while (temp != NULL) 
                {
                    write(temp->fdWrite, command, MAX_COMMAND_LEN);
                    read(temp->fdRead, response, MAX_MESSAGE_LEN);
                    temp = strcmp(response, NACK_STRING) != 0 ? NULL : temp->next;
                }
            }
        }
        else
        {
            //linking not possible for type reason
            return -4;
        }
    }
    else
    {
        //idDevice1 not found
        return -2;
    }
    
    return movedDevices;
}

//link the device with id args[0] to the control unit
int linkToMe(char *idDevice)
{
    int movedDevices = 0;
    char command[MAX_COMMAND_LEN], response[MAX_MESSAGE_LEN];

    sprintf(command, "link %s", idDevice);

    //control if the device is still connected to home
    write(homeWrite, command, MAX_COMMAND_LEN);
    read(homeRead, response, MAX_MESSAGE_LEN);

    //linkto procedure
    if(strcmp(response, ACK_STRING)==0)
    {
        //the device is still connected to home so it has at most one level of chain deep
        movedDevices = 1; 

        //num is incremented only when a device is linked from home tothe control unit
        num += movedDevices;
    }
    else
    {
        //control if the device is present on the system
        NodePointer temp = childrenList;

        bool linkHasSense = false;
        char tmpCommand[MAX_COMMAND_LEN]= "";
        sprintf(tmpCommand, "info %s", idDevice);

        //this cycle verify if the device is in the system and is not already connected to the CU
        while (temp != NULL && !linkHasSense) 
        { 
            write(temp->fdWrite, tmpCommand, MAX_COMMAND_LEN);
            read(temp->fdRead, response, MAX_MESSAGE_LEN);

            int tokensNumber = 0;
            char **responseTokens = formatCommand(response, " ", &tokensNumber);
            if(tokensNumber > 1)
            {
                if (atoi(responseTokens[2]) == temp->pid)
                {
                    //device found but already connected to CU
                    temp = NULL;
                }
                else
                {
                    //device found and we can send a link to it
                    linkHasSense = true;
                }
            }
            else
            {
                temp = temp->next;
            }
            freeAllocatedMemory(responseTokens);
        }

        if(linkHasSense)
        {
            //sends command to the children that manage the ranch where the device is
            write(temp->fdWrite, command, MAX_COMMAND_LEN);
            read(temp->fdRead, response, MAX_MESSAGE_LEN);

            //operation possible for sure
            movedDevices = atoi(response); 
        }        
    }
    
    //getlink procedure; this time on the Control Unit
    if(movedDevices > 0)
    {
        //START RECREATE THE DEVICE (OR THE BRANCH)
        char infoMovedDevice[MAX_MESSAGE_LEN] = "";

        //for every line on in the message queue create a new child
        while(readFromMessageQueue(MPQ_CONTROL_UNIT, infoMovedDevice))
        {
            int infoTokensNumber = 0;
            char **infoMovedDeviceTokens = formatCommand(infoMovedDevice, " ", &infoTokensNumber);
            
            if(getDevice(atoi(infoMovedDeviceTokens[0])))
            {
                char tmpBuffer[MAX_MESSAGE_LEN];

                //send the set info command to prepare the destination for a message
                char setInfoCommand[MAX_COMMAND_LEN] = "setinfo";
                write(childrenList->fdWrite, setInfoCommand, MAX_COMMAND_LEN);
                read(childrenList->fdRead, tmpBuffer, MAX_MESSAGE_LEN);

                //send the device information
                write(childrenList->fdWrite, infoMovedDevice, MAX_MESSAGE_LEN);
                read(childrenList->fdRead, tmpBuffer, MAX_MESSAGE_LEN);

                //propagate getlink command if a branch has moved
                sprintf(command, "getlink");
                write(childrenList->fdWrite, command, MAX_COMMAND_LEN);
                read(childrenList->fdRead, tmpBuffer, MAX_MESSAGE_LEN);
            }
            freeAllocatedMemory(infoMovedDeviceTokens);
        }

        //when there aren't no more message the file for the mq is removed
        if(!removeFile(MPQ_CONTROL_UNIT))
        {
            //error
        }
    }
    return movedDevices;
}

//unlinks the device from the system and link it (and the connected to it) to home
int unlinkFromMe(char *idDevice)
{
    //REMOVE THE BRANCH FROM THE SYSTEM AND LINK IT TO HOME (alias: unlink)
    int unlinkedDevices = 0;
    char command[MAX_COMMAND_LEN], response[MAX_MESSAGE_LEN];

    sprintf(command, "unlink %s", idDevice);

    NodePointer temp = childrenList;
    while (temp != NULL)
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);

        int tokensNumber = 0;
        char **responseTokens = formatCommand(response, " ", &tokensNumber);
        if(tokensNumber == 2) 
        {
            unlinkedDevices = atoi(responseTokens[1]);
            close(temp->fdWrite);
            close(temp->fdRead);
            deleteNode(&childrenList, temp->pid);
            temp = NULL;
        }
        else
        {
            if(strcmp(response, NACK_STRING)!= 0)
            {
                unlinkedDevices = atoi(responseTokens[0]);
                temp = NULL;
            }
            else
            {
                temp = temp->next;
            }
        }
        freeAllocatedMemory(responseTokens);
    }
    if(unlinkedDevices > 0)
    {
        //there are |unlinkedDevices| less linked to the system
        num -= unlinkedDevices;
        //sends a command to home to get the devices
        sprintf(command, "getlink");
        write(homeWrite, command, MAX_COMMAND_LEN);

        char tmpBuffer[MAX_MESSAGE_LEN];
        read(homeRead, tmpBuffer, MAX_MESSAGE_LEN);
    }
    return unlinkedDevices;
}

//sends the switch command args[0] to the system
int switchDevice(char *command)
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

            //we found the branch so it makes no sense go on 
            temp = NULL;
        }
        else
        {
            temp = temp->next;
        }
    }

    return switchedDevices;
}

//ask at every device in the system to show its details if its id is equal to the specyfied id
bool getInfo(char *id, bool print)
{
    bool res = false;

    char command[MAX_COMMAND_LEN];
    sprintf(command, "info %s", id);

    NodePointer temp = childrenList;
    while (temp != NULL) 
    {
        write(temp->fdWrite, command, MAX_COMMAND_LEN);
        temp = temp->next;
    }

    char info[MAX_MESSAGE_LEN];
    temp = childrenList;
    while (temp != NULL)
    {
        read(temp->fdRead, info, MAX_MESSAGE_LEN);
        if(!res)
        {
            int tokensNumber = 0;
            char** infoTokens = formatCommand(info, " ", &tokensNumber);
            if(strcmp(infoTokens[0], NACK_STRING) != 0) {
                if(print) printInfo(infoTokens);
                res = true;
            }
            freeAllocatedMemory(infoTokens);
        }
        temp = temp->next;
    }

    return res;
}

//sends the command to set a timer to the system
void set(char* inputBuffer) 
{
    NodePointer temp = childrenList;
    char response[MAX_MESSAGE_LEN], found[MAX_MESSAGE_LEN] = NACK_STRING;
    while(temp != NULL) 
    {
        write(temp->fdWrite, inputBuffer, MAX_COMMAND_LEN);
        read(temp->fdRead, response, MAX_MESSAGE_LEN);
        if(strcmp(response, NACK_STRING) != 0) temp = NULL;
        else temp = temp->next;
    }
    sprintf(found, "%s", response);
    if(strcmp(found, NACK_STRING) == 0) printf(BOLDRED"Error occurred while setting the timer\n"NC);
    else printf(BOLDGREEN"Timer set!\n"NC);
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