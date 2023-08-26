/*
    PROJECT LIBRARY INCLUSION

*/
#include "../include/common.h"

//Define my own status code
// Indexes for state returned
#define S_STATE_OPERATION 0
#define S_DEVICE 1
#define S_STATE_DEVICE 2
#define S_TIME 3
#define S_PERCENTAGE 4
#define S_TEMPERATURE 5
#define S_BEGIN 3
#define S_END 4
#define S_N_DEVICE 3
#define S_OVERRIDE 2
#define S_IS_RUNNING 2

//Indexes for command array
#define C_DEVICE 0
#define C_COMMAND 1
#define C_DESCRIPTION 2
#define C_ARGUMENTS 3
#define C_EXCLUSIVE 4


#define SD_DEFAULT 2
#define SD_ON_OR_OPEN 1
#define SD_OFF_OR_CLOSE 0

#define DEVICE_LENGTH 20


//Declaration of the functions
void readCommandDevice(int);
int extractDeviceInfo(char[], int *, int *, char *);
char *returnOnlyCommand(char *);
int returnDeviceState(char[]);
void swapState(int *, char *);
int returnSecondState(int);
bool checkDeviceInList(char *);
int managingResponse(int, char *, char[]);
void printMenu();
void printChoice(char[], int);
void printState(int, char[]);
char *sendCommand(char[]);
int sendSignal(int, int);
int signalSuccessful(int);

/*
    MANUAL MODE INFORMATION
*/
char **commandsConsole[] =
        {
                (char *[]) {"connect <PID>", "Connect to the device with Process ID <PID>"},
                (char *[]) {"clear", "Clear the console"},
                (char *[]) {"quit", "Close the console"},
                (char *[]) {"help", "Print the menu"}
        };

// <DEVICE> <COMMAND> <DESCRIPTION> <N° ARGUMENTS> <EXCLUSIVE COMMAND>
char **commandsDevice[] =
        {
                (char *[]) {"bulb", "on", "Switch on the bulb", "1", "1"},
                (char *[]) {"bulb", "off", "Switch off the bulb", "1", "1"},
                (char *[]) {"window", "open", "Open the window", "1", "0"},
                (char *[]) {"window", "close", "Close the window", "1", "0"},
                (char *[]) {"fridge", "open", "Open the fridge", "1", "1"},
                (char *[]) {"fridge", "close", "Close the fridge", "1", "1"},
                (char *[]) {"fridge", "temp <C>", "Set the temperature of the fridge to <C> Celsius grades", "2", "0"},
                (char *[]) {"fridge", "add <N>", "Add N% of the content of the fridge", "2", "0"},
                (char *[]) {"fridge", "remove <N>", "Remove N% of the content of the fridge", "2", "0"},
                (char *[]) {"hub", "on", "Send OPEN and ON commands to all device connected to the hub", "1", "0"},
                (char *[]) {"hub", "off", "Send CLOSE and OFF commands to all device connected to the hub", "1", "0"},
                (char *[]) {"timer", "set <begin> <end>", "Set to activate the timer from begin to end. Format: <HH.mm>", "3", "0"},
                (char *[]) {"all", "print", "print the status of the device", "1", "0"},
                (char *[]) {"all", "exit", "Disconnect from the device", "1", "0"}
        };

int iCommandsConsole = (int) (sizeof(commandsConsole) / sizeof(char **));
int iCommandsDevice = (int) (sizeof(commandsDevice) / sizeof(char **));

int main() {
    char inputBuffer[MAX_COMMAND_LEN];
    bool goOn = true;
    clearScreen();

    printf("WELCOME! THIS IS THE MANUAL MODE. It allows you to control a single device \n\n");

    printf(BOLDYELLOW);
    printf(" __  __   _   _  _ _   _  _   _      __  __  ___  ___  ___ \n");
    printf("|  \\/  | /_\\ | \\| | | | |/_\\ | |    |  \\/  |/ _ \\|   \\| __|\n");
    printf("| |\\/| |/ _ \\| .` | |_| / _ \\| |__  | |\\/| | (_) | |) | _| \n");
    printf("|_|  |_/_/ \\_\\_|\\_|\\___/_/ \\_\\____| |_|  |_|\\___/|___/|___|\n\n");
    printf(NC);                                                           

    printMenu();

    do {
        printf("manual>");
        int stateReader = readLine(inputBuffer, sizeof(inputBuffer), false);
        switch (stateReader) {
            case TOO_LONG:
                printf(BOLDRED"Input too long [%s]. Type something else\n"NC, inputBuffer);
                break;
            case OK:;
                int tokensNumber = 0;
                char **commandTokens = formatCommand(inputBuffer, " ", &tokensNumber);

                if (strcmp(commandTokens[0], "connect") == 0 && tokensNumber == 2) {
                    //Controllo se il pid passato è un numero
                    int pid = toInt(commandTokens[1]);

                    if (pid == NOT_A_NUMBER) {
                        printf(BOLDRED"The pid isn't a valid number. Try again\n"NC);
                        break;
                    }

                    if (signalSuccessful(pid) == ACK) {
                        printf(BOLDGREEN"Connection established \n"NC);

                        readCommandDevice(pid);
                    }
                } else if (strcmp(commandTokens[0], "clear") == 0 && tokensNumber == 1) {
                    clearScreen();
                } else if (strcmp(commandTokens[0], "quit") == 0 && tokensNumber == 1) {
                    goOn = false;
                    printf(BOLDCYAN"Bye! \n"NC);
                } else if (strcmp(commandTokens[0], "help") == 0 && tokensNumber == 1) {
                    printMenu();
                } else {
                    printf(BOLDRED"Command not found\n"NC);
                }
                freeAllocatedMemory(commandTokens);
                break;
        }
    } while (goOn);

    return 0;

}

void readCommandDevice(int pid) {

    int stateDevice;
    int iExclusiveCommand;
    char *response = sendCommand("PRINT");
    char *device = calloc(DEVICE_LENGTH, sizeof(char));
    int operationCompleted = extractDeviceInfo(response, &stateDevice, &iExclusiveCommand, device);

    if (operationCompleted == NACK) {
        printf(BOLDRED"Process doesn't execute your command! Error: %s \n"NC, response);
        return;
    }
    else if (!checkDeviceInList(device)) {
        printf(BOLDRED"Device '%s' not recognized, try again \n"NC, device);
        return;
    }

    printf(BOLDCYAN);
    printf("\n\n#############################################\n");
    printf("################### %s ################### \n", device);
    printf("#############################################\n");
    printf(NC);
    printState(pid, response);
    printChoice(device, iExclusiveCommand);


    char inputBuffer[MAX_COMMAND_LEN];
    bool goOn = true;
    bool find = false;
    int i = 0;

    do {
        printf("manual:%s>", device);
        int stateReader = readLine(inputBuffer, sizeof(inputBuffer), false);
        switch (stateReader) {
            case TOO_LONG:
                printf(BOLDRED"Input too long [%s]. Type something else\n"NC, inputBuffer);
                break;
            case OK:;
                int tokensNumber = 0;
                char **commandTokens = formatCommand(inputBuffer, " ", &tokensNumber);

                if (strcmp(commandTokens[0], "exit") == 0 && tokensNumber == 1) {
                    printf(BOLDGREEN"Disconnected from device! \n\n"NC);
                    goOn = false;
                } else {
                    find = false;
                    int arg;
                    for (i = 0; i < iCommandsDevice; i++) {
                        arg = toInt(commandsDevice[i][C_ARGUMENTS]);
                        if ((strcmp(device, toUpper(commandsDevice[i][C_DEVICE])) == 0 ||
                             strcmp(commandsDevice[i][C_DEVICE], "all") == 0) &&
                            strcmp(commandTokens[0], returnOnlyCommand(commandsDevice[i][C_COMMAND])) == 0 &&
                            tokensNumber == arg) {
                            if (arg == 2 && toInt(commandTokens[1]) == NOT_A_NUMBER) {
                                printf(BOLDRED"Value passed as parameter isn't valid! \n"NC);
                                break;
                            }
                            else if(arg == 3 && !checkTimerSet(commandTokens[1],commandTokens[2])){
                                printf(BOLDRED"Time passed as parameter isn't in the right format! \n"NC);
                                break;
                            }
                            find = true;
                            break;
                        }
                    }
                    if (!find) {
                        printf(BOLDRED"Command not found\n"NC);
                    } else {
                        //Altro controllo sullo stato nel momento di esecuzione del comando
                        int secondStateDevice = returnSecondState(pid);
                        if (secondStateDevice != NACK) {
                            //Aggiungere controllo se il comando è per cambiare stato
                            if (secondStateDevice != stateDevice) {
                                printf(BOLDRED"State of device is changed, you can't execute this command anymore! \n"NC);
                                if (toInt(commandsDevice[i][C_EXCLUSIVE]) == 1) {
                                    iExclusiveCommand = i;
                                }
                                stateDevice = secondStateDevice;
                            } else if (iExclusiveCommand == i) {
                                printf(BOLDRED"You can't execute this command! The device is already in this state \n"NC);
                            } else if (signalSuccessful(pid) == ACK) {
                                char *response = sendCommand(toUpper(inputBuffer));

                                //Se l'operazione è andata a buon fine sul processo figlio e il comando eseguito è esclusivo (per esempio o si usa on o off non tutti e due nello stesso momento) allora si cambia lo stato salvato
                                if (managingResponse(pid, response, toUpper(inputBuffer)) == ACK &&
                                    toInt(commandsDevice[i][C_EXCLUSIVE]) == 1) {
                                    iExclusiveCommand = i;
                                }
                                //AGGIUNGERE IL CONTROLLO CHE SE E' UN COMANDO DI CAMBIO STATO FA LO SWAP
                                swapState(&stateDevice, inputBuffer);

                                printf("\n");
                                //Stampo il menu solo se sono cambiati i comandi
                                if(toInt(commandsDevice[i][C_EXCLUSIVE]) == 1){
                                    printChoice(device, iExclusiveCommand);
                                }
                            }
                        }
                    }
                }
                freeAllocatedMemory(commandTokens);
                break;
        }
    } while (goOn);
}

int extractDeviceInfo(char state[], int *stateDevice, int *iExclusiveCommand, char *device) {
    *iExclusiveCommand = -1;


    int tokensNumber = 0;
    char **stateTokens = formatCommand(state, " ", &tokensNumber);

    //Controllo che il processo abbia eseguito correttamente l'operazione
    if (toInt(stateTokens[S_STATE_OPERATION]) == NACK) {
        return NACK;
    }
    int i = 0;
    for (i; i < iCommandsDevice; i++) {
        if (strcmp(stateTokens[S_DEVICE], toUpper(commandsDevice[i][C_DEVICE])) == 0 &&
            strcmp(stateTokens[S_STATE_DEVICE], toUpper(commandsDevice[i][C_COMMAND])) == 0
            && toInt(commandsDevice[i][C_EXCLUSIVE]) == 1) {
            *iExclusiveCommand = i;
        }
    }

    *stateDevice = returnDeviceState(stateTokens[S_STATE_DEVICE]);
    strcpy(device, stateTokens[S_DEVICE]);

    return ACK;

}

char *returnOnlyCommand(char *commandComplete) {
    int tokensNumber = 0;
    char **commandTokens = formatCommand(commandComplete, " ", &tokensNumber);
    return commandTokens[0];
}

int returnDeviceState(char state[]) {
    if (strcmp(state, "ON") == 0 || strcmp(state, "OPEN") == 0)
        return SD_ON_OR_OPEN;
    else if (strcmp(state, "OFF") == 0 || strcmp(state, "CLOSE") == 0)
        return SD_OFF_OR_CLOSE;
    else
        return SD_DEFAULT;
}

void swapState(int *state, char *command) {
    if (*state == SD_OFF_OR_CLOSE && (strcmp(toUpper(command), "ON") == 0 || strcmp(toUpper(command), "OPEN") == 0))
        *state = SD_ON_OR_OPEN;
    else if (*state == SD_ON_OR_OPEN &&
             (strcmp(toUpper(command), "OFF") == 0 || strcmp(toUpper(command), "CLOSE") == 0))
        *state = SD_OFF_OR_CLOSE;
}

int returnSecondState(int pid) {
    int signal = signalSuccessful(pid);

    if (signal == ACK) {
        char *state = sendCommand("PRINT");

        int tokensNumber = 0;
        char **stateTokens = formatCommand(state, " ", &tokensNumber);

        //Controllo che il processo abbia eseguito correttamente l'operazione
        if (toInt(stateTokens[S_STATE_OPERATION]) == NACK) {
            return NACK;
        }
        return returnDeviceState(stateTokens[2]);
    }
    return NACK;
}

bool checkDeviceInList(char *device) {
    int i = 0;
    for (i; i < iCommandsDevice; i++) {
        if (strcmp(device, toUpper(commandsDevice[i][C_DEVICE])) == 0 && strcmp(device, "all") != 0) {
            return true;
        }
    }
    return false;
}

int managingResponse(int pid, char *response, char command[]) {
    int tokensNumber = 0;
    char **stateTokens = formatCommand(response, " ", &tokensNumber);

    if (toInt(stateTokens[S_STATE_OPERATION]) == NACK) {
        printf(BOLDRED"Process doesn't execute your command! This is the error: %s\n"NC, response);
        return NACK;
    }

    if (strcmp(command, "PRINT") == 0) {
        printState(pid, response);
    } else {
        printf(BOLDGREEN"OK command executed!\n"NC);
    }
    return ACK;
}

void printMenu() {
    printf(BOLDCYAN"Here's the list of available command! Just type it ;)\n"NC);
    int i = 0;
    for (i; i < iCommandsConsole; i++) {
        printf(BOLDBLUE"%-28s"NC":  %s \n", commandsConsole[i][0], commandsConsole[i][1]);
    }
}

void printChoice(char device[], int iStateDevice) {
    printf(BOLDCYAN"List of device's commands: \n"NC);
    int i = 0;
    for (i; i < iCommandsDevice; i++) {
        if ((strcmp(toUpper(commandsDevice[i][C_DEVICE]), device) == 0 ||
             strcmp(commandsDevice[i][C_DEVICE], "all") == 0) &&
            i != iStateDevice) {
            printf(BOLDBLUE"%-28s"NC":  %s \n", commandsDevice[i][C_COMMAND], commandsDevice[i][C_DESCRIPTION]);
        }
    }
    printf("\n");
}

void printState(int pid, char r_state[]) {
    int tokensNumber = 0;
    char **stateTokens = formatCommand(r_state, " ", &tokensNumber);
    printf("\n");
    printf("PID: %d \t DEVICE: %s \n\n", pid, stateTokens[S_DEVICE]);
    if (strcmp(stateTokens[S_DEVICE], "BULB") == 0) {
        printf("The bulb is %s and it is turned on by %.2lf seconds \n\n", stateTokens[S_STATE_DEVICE],
               toDouble(stateTokens[S_TIME]));
    } else if (strcmp(stateTokens[S_DEVICE], "WINDOW") == 0) {
        printf("The window is %s and it is opened by %.2lf seconds \n\n", stateTokens[S_STATE_DEVICE],
               toDouble(stateTokens[S_TIME]));
    } else if (strcmp(stateTokens[S_DEVICE], "FRIDGE") == 0) {
        printf("The fridge is %s and it is opened by %.2lf seconds \n\n", stateTokens[S_STATE_DEVICE],
               toDouble(stateTokens[S_TIME]));
        printf("The content is %.2lf%c and the setted temperature is: %d° \n\n",
               (toDouble(stateTokens[S_PERCENTAGE]) * 100), '%',
               toInt(stateTokens[S_TEMPERATURE]));
    } else if (strcmp(stateTokens[S_DEVICE], "HUB") == 0) {
        printf("The hub %s in override and it has connected to %d devices \n\n", (toInt(stateTokens[S_OVERRIDE]) == 1)?"IS":"IS NOT",
               toInt(stateTokens[S_N_DEVICE]));
    } else if (strcmp(stateTokens[S_DEVICE], "TIMER") == 0) {
        printf("The timer %s running. It begin at %.2lf and end at %.2lf \n\n", (toInt(stateTokens[S_IS_RUNNING]) == 1)?"IS":"IS NOT",
               toDouble(stateTokens[S_BEGIN]), toDouble(stateTokens[S_END]));
    }
}

char *sendCommand(char command[]) {

    //Writing on the FIFO
    writeFifo(command);
    //Waiting response
    char *state = readFifo();

    removeFile("cfifo");

    return state;
}

int sendSignal(int pid, int signal) {
    return kill((pid_t) pid, signal);
}

int signalSuccessful(int pid) {
    int statusSignal = sendSignal(pid, SIGURG);
    if (statusSignal != 0) {
        printf(BOLDRED"Something goes wrong! Please try again... Maybe process doesn't exist!\n"NC);
        return NACK;
    }
    return ACK;
}

/*  RETURNED STRING FROM DEVICE:
 *  BULB -->    <STATE COMMAND> <DEVICE> <STATE DEVICE> <TIME>
 *  WINDOW -->  <STATE COMMAND> <DEVICE> <STATE DEVICE> <TIME>
 *  FRIDGE -->  <STATE COMMAND> <DEVICE> <STATE DEVICE> <TIME> <PERCENTAGE> <TEMPERATURE>
 *  HUB -->     <STATE COMMAND> <DEVICE> <OVERRIDE> <N° DEVICE>
 *  TIMER -->   <STATE COMMAND> <DEVICE> <IS RUNNING> <BEGIN TIME> <END TIME>
 * */
