#ifndef LIST_H
#define LIST_H

//it saves he minimum information to communicate with a child process
struct Node
{
    int pid;
    int fdRead;
    int fdWrite;
    struct Node * next;
};

typedef struct Node * NodePointer;

//list functions definitions
void push(struct Node**, int, int, int);
void append(struct Node**, int, int, int); 
NodePointer search(struct Node**, int);
void deleteNode(NodePointer*, int);
void deleteList(struct Node**);
void printList(struct Node *);

#endif

