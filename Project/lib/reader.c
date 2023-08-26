#include "../include/common.h"



//reads the command line inserted by the user
int readLine (char *buff, size_t sz,bool printGeq)
{
    if(printGeq){
        printf("> ");
    }
    fgets(buff, sz, stdin);
    if(buff[0] != '\n')
    {
        char *pos;
        if ((pos = strchr(buff, '\n')) != NULL)
        {
            *pos = '\0';
            return OK;
        }
        else
        {
            clearStdin();
            return TOO_LONG;
        }
    }
    else
    {
        return NO_INPUT;
    }    
}

//when the input gets to overflow this function clear the stdin buffer
void clearStdin()
{
    char uselessBuffer[MAX_COMMAND_LEN];
    char *pos = NULL;
    
    while (pos == NULL)
    {
        fgets(uselessBuffer, MAX_COMMAND_LEN, stdin);
        pos = strchr(uselessBuffer, '\n');
    }
}

//splits the input command into an array of tokens
char** formatCommand(char *inputBuffer, const char* sep, int* tokensNumber)
{
    char* tmpBuffer = calloc(strlen(inputBuffer)+1, sizeof(char));
    strcpy(tmpBuffer, inputBuffer);


    char **commandTokens = malloc( sizeof( char * ) );

    if ( commandTokens )
    {
        size_t n = 1;

        char *token = strtok( tmpBuffer, sep );

        while ( token )
        {
            char **tmp = realloc( commandTokens, ( n + 1 ) * sizeof( char * ) );

            if ( tmp == NULL ) break; //eventually add the allocation control

            commandTokens = tmp;
            ++n;

            commandTokens[ n - 2 ] = malloc( strlen( token ) + 1 );
            if ( commandTokens[ n - 2 ] != NULL )
            strcpy( commandTokens[ n - 2 ], token );

            token = strtok( NULL, sep );
        }

        commandTokens[ n - 1 ] = NULL;

        *tokensNumber = (int)n-1;
    }

    return commandTokens;
}

//free the memory alocated dynamically to store the command
void freeAllocatedMemory(char** charPointerArray)
{
    char **p;
    for ( p = charPointerArray; *p; ++p ) free( *p );
        free( charPointerArray );
}