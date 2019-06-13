
#ifndef LISTTYPES_H
#define LISTTYPES_H

#include <stdint.h>

typedef struct Node *Listptr;


typedef struct Node {
    uint32_t ipaddr;
    uint16_t port;
    Listptr next;
} Listnode;


int searchInList(Listptr head, uint32_t ipaddr, uint16_t port);
void addToList(Listptr *head, uint32_t ipaddr, uint16_t port);
void removeFromList(Listptr *head_ref, uint32_t ipaddr, uint16_t port);
int countListElements(Listptr head);
void deleteList(Listptr head);


#endif //LISTTYPES_H
