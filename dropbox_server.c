
// Server side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include "Messages.h"

#define BACKLOG 10   // backlog connections will be queued waiting to be accepted

typedef struct Node *Listptr;

typedef struct Node {
    uint32_t ipaddr;
    uint16_t port;
    Listptr next;
} Listnode;

Listptr client_list = NULL;


int connectToClient(char *client_ip, uint16_t port) {
    int socket_num;
    struct sockaddr_in client_addr;
    struct hostent *client;

    socket_num = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_num < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        close(socket_num);
        return -1;
    }

    client = gethostbyname(client_ip);
    if (client == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(socket_num);
        return -1;
    }
    bzero((char *) &client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    bcopy((char *) client->h_addr,
          (char *) &client_addr.sin_addr.s_addr,
          client->h_length);
    client_addr.sin_port = htons(port);
    if (connect(socket_num, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {
        printf("ERROR connecting to client %s %u\n", client_ip, port);
        perror("");
        close(socket_num);
        return -1;
    }
    return socket_num;
}


void disconnect(int fd) {
    close(fd);
}


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


void addToList(Listptr *head, uint32_t ipaddr, int port) {
    if (searchInList(*head, ipaddr, port)) {
        return;
    }
    Listptr new_node = (Listptr) malloc(sizeof(Listnode));
    new_node->ipaddr = ipaddr;
    new_node->port = port;
    new_node->next = *head;
    *head = new_node;
}

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


int main(int argc, char *argv[]) {
    int i, portNum;
    if (argc != 3) {
        fprintf(stderr, "Run server like: ./dropbox_server -p portNum\n");
        exit(EXIT_FAILURE);
    }
    for (i = 1; i < argc; i++) {
        if (i + 1 != argc) {
            if (strcmp(argv[i], "-p") == 0) {
                portNum = atoi(argv[i + 1]);
                if (portNum <= 0) {
                    fprintf(stderr, "PortNum must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};


    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        fprintf(stderr, "Setsockopt failed\n");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portNum);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        fprintf(stderr, "Bind failed\n");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, BACKLOG) < 0) {
        fprintf(stderr, "Listen failed\n");
        exit(EXIT_FAILURE);
    }

    while (1) {

        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            fprintf(stderr, "Accept failed\n");
            exit(EXIT_FAILURE);
        }

        read(new_socket, buffer, 12);

        if (strncmp(buffer, LOG_ON, strlen(LOG_ON)) == 0) {
            struct sockaddr_in m_addr;
            struct sockaddr_in sa;
            char *some_ip;
            struct in_addr cip;
            socklen_t len;

            uint32_t client_ip;
            uint16_t client_port;

            read(new_socket, &client_ip, sizeof(uint32_t));
            read(new_socket, &client_port, sizeof(uint16_t));

            len = sizeof(m_addr);
            getpeername(new_socket, (struct sockaddr *) &m_addr, &len);
            printf("RECEIVED LOG_ON from:%s %d\n", inet_ntoa(m_addr.sin_addr), ntohs(client_port));


            if (!searchInList(client_list, ntohl(client_ip), ntohs(client_port))) {
                addToList(&client_list, ntohl(client_ip), ntohs(client_port));
                printf("Just inserted in client list  %s %d  \n", inet_ntoa(m_addr.sin_addr), ntohs(client_port));
                printf("Currently logged on: %d\n", countListElements(client_list));
                cip.s_addr = ntohl(client_ip);
            }

            int existing = 0;
            existing = countListElements(client_list);
            if (existing > 1) {
                Listptr temp = client_list;
                while (temp != NULL) {

                    sa.sin_addr.s_addr = temp->ipaddr;
                    some_ip = inet_ntoa(sa.sin_addr);

                    int client_fd = connectToClient(some_ip, temp->port);
                    if (client_fd > 0) {

                        uint16_t data_port = htons(temp->port);

                        printf("Sending USER_ON to:%s %d\n", inet_ntoa(m_addr.sin_addr), ntohs(data_port));

                        send(client_fd, USER_ON, strlen(USER_ON) + 1, 0);
                        send(client_fd, &client_ip, sizeof(uint32_t), 0);
                        send(client_fd, &client_port, sizeof(uint16_t), 0);

                        disconnect(client_fd);
                    }
                    temp = temp->next;
                }
            }
        }

        else if (strncmp(buffer, GET_CLIENTS, strlen(GET_CLIENTS)) == 0) {
            printf("RECEIVED GET_CLIENTS\n\n");

            uint32_t data;
            uint16_t data_port;

            sprintf(buffer, "%s %4d", CLIENT_LIST, countListElements(client_list));

            send(new_socket, buffer, strlen(buffer) + 1, 0);

            Listptr temp = client_list;
            while (temp != NULL) {
                data = htonl(temp->ipaddr);
                send(new_socket, &data, sizeof(uint32_t), 0);
                data_port = htons(temp->port);
                send(new_socket, &data_port, sizeof(uint16_t), 0);
                temp = temp->next;
            }
        }

        else if (strncmp(buffer, LOG_OFF, strlen(LOG_OFF)) == 0) {
            struct sockaddr_in m_addr;
            struct in_addr cip;
            socklen_t len;

            uint32_t client_ip;
            uint16_t client_port;

            read(new_socket, &client_ip, sizeof(uint32_t));
            read(new_socket, &client_port, sizeof(uint16_t));

            len = sizeof(m_addr);
            getpeername(new_socket, (struct sockaddr *) &m_addr, &len);

            printf("\nRECEIVED LOG_OFF from:%s %d\n", inet_ntoa(m_addr.sin_addr), ntohs(client_port));

            if (searchInList(client_list, ntohl(client_ip), ntohs(client_port))) {

                removeFromList(&client_list, ntohl(client_ip), ntohs(client_port));
                printf("Removed from list client  %s %d \n", inet_ntoa(m_addr.sin_addr), ntohs(client_port));
                printf("Currently logged on: %d\n\n\n", countListElements(client_list));
                cip.s_addr = ntohl(client_ip);

                Listptr temp = client_list;
                while (temp != NULL) {

                    struct sockaddr_in sa;
                    char str[INET_ADDRSTRLEN];

                    sa.sin_addr.s_addr = temp->ipaddr;
                    inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);

                    printf("Connecting to ... %s %d\n", str, temp->port);
                    int client_fd = connectToClient(str, temp->port);
                    if (client_fd > 0) {

                        uint16_t data_port = htons(temp->port);

                        printf("Sending USER_OFF to:%s %d\n", inet_ntoa(m_addr.sin_addr), ntohs(data_port));

                        send(client_fd, USER_OFF, strlen(USER_OFF) + 1, 0);
                        send(client_fd, &client_ip, sizeof(uint32_t), 0);
                        send(client_fd, &client_port, sizeof(uint16_t), 0);

                        disconnect(client_fd);
                    }
                    temp = temp->next;
                }
            } else {
                send(new_socket, ERROR_IP_PORT_NOT_FOUND_IN_LIST, strlen(ERROR_IP_PORT_NOT_FOUND_IN_LIST) + 1, 0);
            }
        }
        close(new_socket);
    }
    return 0;
}
