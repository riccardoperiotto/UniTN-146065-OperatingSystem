#include "../include/common.h"

// structure for message queue
struct mesg_buffer {
    long type;
    char text[MAX_MESSAGE_LEN];
} message;

const char *trasmissionFifo = "/tmp/cfifo";
const int permissionFifo = 0666;

bool writeOnMessageQueue(char *id, char *text)
{
    bool res = true;

    //get path from id
    char path[MAX_PATH_LEN]="";
    getPathTmp(id, path);

    // ftok to generate unique key
    key_t key = ftok(path, 65);

    // msgget creates a message queue
    // and returns identifier
    int msgid = msgget(key, 0666 | IPC_CREAT | IPC_PRIVATE);
    if(msgid < 0)
    {
        perror("msgget\n");
        res = false;
    }
    else
    {
        message.type = 1;

        strcpy(message.text, text);

        // msgsnd to send message
        if(msgsnd(msgid, &message, MAX_MESSAGE_LEN, 0) < 0)
        {
            perror("msgsnd\n");
            res = false;
        }
    }

    return res;

}

bool readFromMessageQueue(char *id, char *text)
{
    bool res = true;

    //get path from id
    char path[MAX_PATH_LEN];
    getPathTmp(id, path);

    // ftok to generate unique key
    key_t key = ftok(path, 65);

    // msgget creates a message queue and returns identifier
    //IPC_NOWAIT Return immediately if no message of the requested type is in the queue.
    int msgid = msgget(key, 0666 | IPC_CREAT );

    if(msgid < 0)
    {
        perror("msgget\n");
        res = false;
    }
    else
    {
        // msgrcv to receive message
        if(msgrcv(msgid, &message, MAX_MESSAGE_LEN, 1, IPC_NOWAIT) < 0)
        {
            //perror("msgrcv\n");
            res = false;
        }
        else
        {
            strcpy(text, message.text);
        }
    }

    return res;
}

char* readFifo(){
    mkfifo(trasmissionFifo,permissionFifo);
    char *state = calloc(MAX_MESSAGE_LEN, sizeof(char));

    int fd = open(trasmissionFifo,O_RDONLY);
    read(fd,state,MAX_MESSAGE_LEN);
    close(fd);

    return state;
}

void writeFifo(char* command){
    mkfifo(trasmissionFifo,permissionFifo);
    int fd = open(trasmissionFifo,O_WRONLY);
    write(fd,command,strlen(command)+1);
    close(fd);
}