#include "../include/common.h"

/* Given a reference (pointer to pointer) to the head of a list 
   and an int,  inserts a new node on the front of the list. */
void push(struct Node** headRef, int pid, int fdWrite, int fdRead) 
{ 
    /* 1. allocate node */
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node)); 
   
    /* 2. put in the data  */
    newNode->pid = pid;
    newNode->fdWrite  = fdWrite; 
    newNode->fdRead  = fdRead; 
   
    /* 3. Make next of new node as head */
    newNode->next = (*headRef); 
   
    /* 4. move the head to point to the new node */
    (*headRef)    = newNode; 
} 

/* Given a reference (pointer to pointer) to the head 
   of a list and an int, appends a new node at the end  */
void append(struct Node** headRef, int pid, int fdWrite, int fdRead) 
{ 
    /* 1. allocate node */
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node)); 
  
    struct Node *last = *headRef;  /* used in step 5*/
   
    /* 2. put in the data  */
    newNode->pid = pid;
    newNode->fdWrite  = fdWrite;
    newNode->fdRead  = fdRead; 

    /* 3. This new node is going to be the last node, so make next  
          of it as NULL*/
    newNode->next = NULL; 
  
    /* 4. If the Linked List is empty, then make the new node as head */
    if (*headRef == NULL) 
    { 
       *headRef = newNode; 
       return; 
    }   
       
    /* 5. Else traverse till the last node */
    while (last->next != NULL) 
        last = last->next; 
   
    /* 6. Change the next of last node */
    last->next = newNode; 
    return;     
}

//return the pointer to the nodewith the pid specyfied
NodePointer search(struct Node** headRef, int pid)
{
    // Store head node 
    struct Node* temp = *headRef; 
  
    // Search for the key 
    while (temp != NULL && temp->pid != pid) 
    { 
        temp = temp->next; 
    } 
    return temp;
}

/* Given a reference (pointer to pointer) to the head of a list 
   and a key, deletes the first occurrence of key in linked list */
void deleteNode(struct Node** headRef, int pid) 
{ 
    // Store head node 
    struct Node* temp = *headRef, *prev; 
  
    // If head node itself holds the key to be deleted 
    if (temp != NULL && temp->pid == pid) 
    { 
        *headRef = temp->next;   // Changed head 
        free(temp);               // free old head 
        return; 
    } 
  
    // Search for the key to be deleted, keep track of the 
    // previous node as we need to change 'prev->next' 
    while (temp != NULL && temp->pid != pid) 
    { 
        prev = temp; 
        temp = temp->next; 
    } 
  
    // If key was not present in linked list 
    if (temp == NULL) return; 
  
    // Unlink the node from linked list 
    prev->next = temp->next; 
  
    free(temp);  // Free memory 
} 

/* Function to delete the entire linked list */
void deleteList(struct Node** headRef)  
{  
      
    /* deref headRef to get the real head */
    struct Node* current = *headRef;  
    struct Node* next;  
    
    while (current != NULL)  
    {  
        next = current->next;  
        free(current);  
        current = next;  
    }  
        
    /* deref headRef to affect the real head back in the caller. */
    *headRef = NULL;  
}

// This function prints contents of linked list starting from head 
void printList(struct Node *node) 
{ 
  while (node != NULL) 
  { 
     printf(" %d ", node->pid); 
     node = node->next; 
  } 
} 