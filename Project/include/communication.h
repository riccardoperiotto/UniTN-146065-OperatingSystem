#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define READ 0 // read-side of pipes 
#define WRITE 1 // write-side of pipes 
#define MAX_MESSAGE_LEN 100

//all the anonymous pipe communication is based on a question and answer communication
#define ACK 1
#define NACK -1

//to simplify the response comparison
#define ACK_STRING "1"
#define NACK_STRING "-1"

//used for the link procedure
bool writeOnMessageQueue(char *id, char *message);
bool readFromMessageQueue(char *id, char *message);

//used for the communication with the manual command excecutor
char* readFifo();
void writeFifo(char*);

#endif
