#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ListTypes.h"

int searchInList(Listptr head, uint32_t ipaddr, uint16_t port) {
    Listptr temp = head;
    while (temp != NULL) {
        if (temp->ipaddr == ipaddr && temp->port == port) {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}


int countListElements(Listptr head) {
    int count = 0;
    Listptr temp = head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}


void addToList(Listptr *head, uint32_t ipaddr, uint16_t port) {
    if (searchInList(*head, ipaddr, port)) {
        return;
    }
    Listptr new_node = (Listptr) malloc(sizeof(Listnode));
    new_node->ipaddr = ipaddr;
    new_node->port = port;
    new_node->next = *head;
    *head = new_node;
}


//https://www.geeksforgeeks.org/linked-list-set-3-deleting-node/
void removeFromList(Listptr *head_ref, uint32_t ipaddr, uint16_t port) {
    // Store head node
    Listptr temp = *head_ref, prev;
    // If head node itself holds the key to be deleted
    if (temp != NULL && ipaddr == temp->ipaddr && temp->port == port) {
        *head_ref = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }
    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && ipaddr == temp->ipaddr && temp->port != port) {
        prev = temp;
        temp = temp->next;
    }
    // If key was not present in
    if (temp == NULL) return;
    // Unlink the node from linked client_list
    prev->next = temp->next;
    free(temp);  // Free memory
}


void deleteList(Listptr head) {
    if (head == NULL) return;
    deleteList(head->next);
    free(head);
}