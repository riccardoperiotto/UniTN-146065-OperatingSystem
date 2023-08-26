#include "../include/common.h"



pid_t spawnWithPipe(char* programName, int* fd1, int *fd2)
{
    pid_t childPid;

    //duplicate this process
    childPid = fork();

    int errnum;
    if (childPid == -1) 
    {
        //error section
        errnum = errno;
        fprintf(stderr, "errno = %d\n", errnum);
        fprintf(stderr, "Error occurred in fork()\n");
    } else if(childPid == 0)
    {
       
       

       /* close(fd1[WRITE]);
        close(fd2[READ]);*/
        //children section
        ////////////////////////////////////////////////////////
        int max = 512;
        int pipeCount = 3;
        while(pipeCount != max ){
            if(pipeCount == fd2[1] || pipeCount == fd1[0]) { pipeCount++;}
                else{ close(pipeCount); pipeCount++;} 
            }       
        pipeCount = 3;
        
        /////////////////////////////////////////////////////////
        

        //copy the fd descriptor of the pipe in char* in order to pass it to the exec
        char fdString1[50], fdString2[50];
        snprintf (fdString1, sizeof(fdString1), "%d", fd2[WRITE]);
        snprintf (fdString2, sizeof(fdString2), "%d", fd1[READ]);

        //create the argv list for the exec function
        char* const argv[] = {programName, fdString1, fdString2, NULL};

        //this function execute the program specifyed in programName and pass it argv
        execvp(programName, argv);

        //must never print if the execvp execute successfully
        fprintf(stderr, "errno = %d\n", errno);
        fprintf(stderr, "Error occurred in execvp()\n");
        
        //kill the process
        exit(0);
    } 
    
    return childPid;
};

