#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>
#include "Messages.h"
#include "ListTypes.h"
#include "BufferTypes.h"

pthread_cond_t condVarP = PTHREAD_COND_INITIALIZER;
pthread_cond_t condVarC = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_buffer;
pthread_mutex_t mutex_list;


int isClientOn = 1;
Buffer buffer;
Listptr client_list = NULL;


int establishConnection(char *server_ip, uint16_t port) {
    int socket_num;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    socket_num = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_num < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(server_ip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(socket_num, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        puts("ERROR connecting");
        perror("");
    }
    printf("Connection established with %s at port %d !!\n", server_ip, port);
    return socket_num;
}

void disconnect(int fd) {
    close(fd);
}


void logOn(char *sip, uint16_t sport, char *myip, uint16_t myport) {

    struct sockaddr_in ipaddr;
    ipaddr.sin_addr.s_addr = inet_addr(myip);

    printf("LOG_ON ip: %s port: %u\n", inet_ntoa(ipaddr.sin_addr), myport);

    uint32_t data_ip = htonl(ipaddr.sin_addr.s_addr);
    uint16_t data_port = htons(myport);

    int sockfd = establishConnection(sip, sport);
    if (sockfd < 0){
        fprintf(stderr,"logOn failed to establish connection with server\n");
        return;
    }

    send(sockfd, LOG_ON, strlen(LOG_ON) + 1, 0);
    send(sockfd, &data_ip, sizeof(uint32_t), 0);
    send(sockfd, &data_port, sizeof(uint16_t), 0);

    disconnect(sockfd);
}



void logOff(char *sip, uint16_t sport, char *myip, uint16_t myport) {

    struct sockaddr_in ipaddr;
    ipaddr.sin_addr.s_addr = inet_addr(myip);

    printf("LOG_OFF ip: %s port: %u\n", inet_ntoa(ipaddr.sin_addr), myport);

    uint32_t data_ip = htonl(ipaddr.sin_addr.s_addr);
    uint16_t data_port = htons(myport);

    printf("function call of LOGOFF tries connection\t");
    int sockfd = establishConnection(sip, sport);
    if (sockfd < 0){
        fprintf(stderr,"logOff failed to establish connection with server\n");
        return;
    }

    send(sockfd, LOG_OFF, strlen(LOG_OFF) + 1, 0);
    send(sockfd, &data_ip, sizeof(uint32_t), 0);
    send(sockfd, &data_port, sizeof(uint16_t), 0);

    disconnect(sockfd);
}

void getclients(char *sip, uint16_t sport, char *myip, uint16_t myport) {
    char stream[1024];
    BufferElement request;
    int N;

    printf("function call of getclients tries connection\t");
    int socket_num = establishConnection(sip, sport);

    request.filename[0] = '\0';
    request.version = 0;
    request.fd = -1;

    send(socket_num, GET_CLIENTS, strlen(GET_CLIENTS) + 1, 0);

    read(socket_num, stream, 17);   // 17 = strlen(CLIENT_LIST) + 1 (space) + 4 (N) + 1 (\0)
    sscanf(stream, "CLIENT_LIST %4d", &N);

    printf("(Apo GET_CLIENTS): N sscanffed %d\n", N);

    for (int k = 0; k < N; ++k) {
        uint32_t ip_read;
        uint16_t port_read;
        struct in_addr ip_addr;

        read(socket_num, &ip_read, sizeof(ip_read));
        ip_read = ntohl(ip_read);
        read(socket_num, &port_read, sizeof(port_read));
        port_read = ntohs(port_read);

        ip_addr.s_addr = ip_read;

        if (!strcmp(inet_ntoa(ip_addr), myip) && (port_read == myport)) {
            printf(">>>>>>Ignore me, if found in CLIENT_LIST\n");
            continue;
        }

        pthread_mutex_lock(&mutex_list);
        printf("GET_CLIENTS(inside client):%s %u\n", inet_ntoa(ip_addr), port_read);
        addToList(&client_list, ip_read, port_read);

        request.ipaddr = ip_read;
        request.port = port_read;
        push_in_buffer(&buffer, request);
        printf("(From GET_CLIENTS): in buffer inserted the: \n");
        printElement(request);
        pthread_mutex_unlock(&mutex_list);
    }
    disconnect(socket_num);
}


void *workerThread_job(void *dirname) {

    int N;
    BufferElement request;
    char stream[256];
    char *filname;
    int version;
    char* client_dir = (char *)dirname;

    printf("--------------------------I am Thread %lu \n", pthread_self());

    struct sockaddr_in sa;
    char request_IP[INET_ADDRSTRLEN];

    while (1) {

        pthread_mutex_lock(&mutex_buffer);
        while (buffer_is_empty(buffer) && isClientOn) {     //An o buffer den einai adeios
            pthread_cond_wait(&condVarC, &mutex_buffer);
        }
        printf("workerThread %lu  afairei request apo buffer\n", pthread_self());
        request = pop_from_buffer(&buffer);
        pthread_cond_signal(&condVarP);

        pthread_mutex_unlock(&mutex_buffer);

        if (isClientOn == 0) {
            //logOff(request.ipaddr, serverPort, "127.0.0.1", portNum);
            break;
        }

        if (request.version == 0 || request.filename[0] == '\0') {

            // If the request does not contain a file desriptor
            if (request.fd < 0) {
                sa.sin_addr.s_addr = request.ipaddr;
                inet_ntop(AF_INET, &(sa.sin_addr), request_IP, INET_ADDRSTRLEN);
                printf("workerThread %lu  tries for connection at %s %d   ", pthread_self(),
                       request_IP, request.port);
                int client_fd = establishConnection(request_IP, request.port);
                if (client_fd > 0) {

                    send(client_fd, GET_FILE_LIST, strlen(GET_FILE_LIST) + 1, 0);
                    read(client_fd, stream, strlen(FILE_LIST) + 1);
                    read(client_fd, stream, 4 + 1);

                    N = atoi(stream);

                    for (int i = 0; i < N; ++i) {

                        read(client_fd, stream, 128 + 1 + 4 + 1);
                        filname = strtok(stream, ",");
                        version = atoi(strtok(NULL, ","));


                        pthread_mutex_lock(&mutex_buffer);
                        while (buffer_is_full(buffer) && isClientOn) {
                            pthread_cond_wait(&condVarP, &mutex_buffer);
                        }
                        strcpy(request.filename, filname);
                        request.version = version;
                        push_in_buffer(&buffer, request);
                        pthread_cond_signal(&condVarC);

                        pthread_mutex_unlock(&mutex_buffer);
                    }

                    disconnect(client_fd);
                }
            } else {
                send(request.fd, FILE_LIST, strlen(FILE_LIST) + 1, 0);

                send(request.fd, FILE_LIST, strlen(FILE_LIST) + 1, 0);
                sprintf(stream, " %4d ", numOfFiles(client_dir));
                printf("All files locally in client are %s\n",stream);
                send(request.fd, stream, strlen(stream) + 1, 0);
                sendFiles(dirname, request.fd);

               /* sprintf(stream, "%4d", numOfFiles(client_dir));
                send(request.fd, stream, 4 + 1, 0);
                sendFiles(client_dir, request.fd);*/
                close(request.fd);     // ???
            }
        }
            // GET_FILE
        else {
            if (request.fd < 0) {
                send(request.fd, GET_FILE, strlen(GET_FILE) + 1, 0);
            } else {

            }

        }

    }
    printf("thread %lu exits \n", pthread_self());
    pthread_exit(NULL);
}

char *getLocalFile(BufferElement *request, const char *client_dir, const char *request_IP) {
    char temp[25];
    sprintf(temp, "/%s_%u/", request_IP, (*request).port);
    char *file_name = malloc((strlen(client_dir) + strlen(temp) + strlen((*request).filename) + 1) * sizeof(char));
    strcpy(file_name, client_dir);
    strcat(file_name, temp);
    strcat(file_name, (*request).filename);
    return file_name;
}


int checkSocketStream(int fd, char *stream) {
    int nbytes;

    nbytes = read(fd, stream, 12);

    if (nbytes < 0 || nbytes == 0) {
        return -1;
    } else {
        return 0;
    }
}

//https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
void waitForSelect(int portNum,char* dirName) {

    BufferElement request;
    int i, sock, opt = 1;
    char stream[256];
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in clientname;
    size_t size;

    char *filename;
    int version;

    /* Create the socket and set it up to accept connections. */

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        fprintf(stderr, "Setsockopt failed\n");
        exit(EXIT_FAILURE);
    }

    clientname.sin_family = AF_INET;
    clientname.sin_addr.s_addr = INADDR_ANY;
    clientname.sin_port = htons(portNum);

    // Forcefully attaching socket to the port 8080
    if (bind(sock, (struct sockaddr *) &clientname,
             sizeof(clientname)) < 0) {
        fprintf(stderr, "Bind failed\n");
        exit(EXIT_FAILURE);
    }
    if (listen(sock, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sock, &active_fd_set);

    while (isClientOn) {

        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == sock) {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(clientname);
                    new = accept(sock, (struct sockaddr *) &clientname, (socklen_t *) &size);
                    printf("SPEAKING : %d\n",new);
                    if (new < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET (new, &active_fd_set);
                } else {
                    /* Data arriving on an already-connected socket. */
                    if ((checkSocketStream(i, stream)) < 0) {
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    } else {
                        if (strncmp(stream, GET_FILE_LIST, strlen(GET_FILE_LIST)) == 0) {

                            pthread_mutex_lock(&mutex_buffer);
                            while (buffer_is_full(buffer) && isClientOn) {
                                pthread_cond_wait(&condVarP, &mutex_buffer);
                            }

                            pthread_cond_signal(&condVarC);

                            // Pairnw aoi to buffer monos mou
                            request.filename[0] = '\0';
                            request.version = 0;
                            request.fd = i;

                            push_in_buffer(&buffer, request);

                            pthread_mutex_unlock(&mutex_buffer);
                        } else if (strncmp(stream, GET_FILE, strlen(GET_FILE)) == 0) {


                            read(i, stream, 128 + 1 + 4 + 1);
                            filename = strtok(stream, ",");
                            version = atoi(strtok(NULL, ","));

                            pthread_mutex_lock(&mutex_buffer);
                            while (buffer_is_full(buffer) && isClientOn) {
                                pthread_cond_wait(&condVarP, &mutex_buffer);
                            }

                            // Pairnw aoi to buffer monos mou
                            strcpy(request.filename, filename);
                            request.version = version;
                            request.fd = i;

                            push_in_buffer(&buffer, request);
                            pthread_cond_signal(&condVarC);

                            pthread_mutex_unlock(&mutex_buffer);
                        } else if (strncmp(stream, USER_ON, strlen(USER_ON)) == 0) {

                            struct in_addr ip_rec;
                            uint32_t client_ip;
                            uint16_t client_port;

                            read(i, &client_ip, sizeof(uint32_t));
                            read(i, &client_port, sizeof(uint16_t));

                            ip_rec.s_addr = ntohl(client_ip);

                            char temp[25];
                            sprintf(temp, "/%s_%u/", inet_ntoa(ip_rec), ntohs(client_port));
                            char* mirror_dir = malloc((strlen(dirName)+strlen(temp)+1)* sizeof(char));
                            strcpy(mirror_dir,dirName);
                            strcat(mirror_dir,temp);
                            create_directory(mirror_dir);
                            free(mirror_dir);
                        }else if (strncmp(stream, USER_OFF, strlen(USER_OFF)) == 0){
                            uint32_t client_ip;
                            uint16_t client_port;

                            printf("USEROFF received\n");

                            read(i, &client_ip, sizeof(uint32_t));
                            read(i, &client_port, sizeof(uint16_t));

                            client_ip = ntohl(client_ip);
                            struct in_addr ip_addr;
                            ip_addr.s_addr = client_ip;

                            if (searchInList(client_list, ntohl(client_ip), ntohs(client_port))) {
                                removeFromList(&client_list, ntohl(client_ip), ntohs(client_port));
                                printf("\nRemoved from client_list ---> %s %u\n",inet_ntoa(ip_addr),ntohs(client_port));
                            }
                            close(i);
                            FD_CLR (i, &active_fd_set);
                        }
                    }
                }
            }
    }
}


// This function checks hostname for the local computer
void checkHostName(int hostname) {
    if (hostname == -1) {
        perror("gethostname");
        exit(EXIT_FAILURE);
    }
}

// This function checks host information corresponding to host name
void checkHostEntry(struct hostent *hostentry) {
    if (hostentry == NULL) {
        perror("gethostentry");
        exit(EXIT_FAILURE);
    }
}

//This function checks if inet_ntoa() has failed
void checkIPbuffer(char *IPbuffer) {
    if (NULL == IPbuffer) {
        perror("inet_ntoa");
        exit(EXIT_FAILURE);
    }
}


void handle_sigint(int sig) {
    isClientOn = 0;
    pthread_cond_signal(&condVarC);
    pthread_cond_signal(&condVarP);
}


int main(int argc, char *argv[]) {


    int i, bufferSize, portNum, serverPort, workerThreads;
    char *server_ip,*dirName;

    if (argc != 13) {
        fprintf(stderr, "Run like: ./dropbox_client -d ./dir -p 6666 -w 2 -b 100 -sp 9991 -sip 195.134.65.102\n");
        exit(EXIT_FAILURE);
    }

    for (i = 1; i < argc; i++) {
        if (i + 1 != argc) {
            if (strcmp(argv[i], "-d") == 0) {
                dirName = strdup(argv[i + 1]);
                if (!dirExists(dirName)) {
                    fprintf(stderr,"Directory %s doesn't exist!!\n", dirName);
                    exit(EXIT_FAILURE);
                }
                i++;    // Move to the next flag
            }
            if (strcmp(argv[i], "-p") == 0) {
                portNum = atoi(argv[i + 1]);
                if (portNum <= 0) {
                    fprintf(stderr, "PortNum must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                i++;    // Move to the next flag
            }
            if (strcmp(argv[i], "-w") == 0) {
                workerThreads = atoi(argv[i + 1]);
                if (workerThreads <= 0) {
                    fprintf(stderr, "workerThreads must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                i++;    // Move to the next flag
            }
            if (strcmp(argv[i], "-b") == 0) {
                bufferSize = atoi(argv[i + 1]);
                if (bufferSize <= 0) {
                    fprintf(stderr, "bufferSize must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                i++;    // Move to the next flag
            }
            if (strcmp(argv[i], "-sp") == 0) {
                serverPort = atoi(argv[i + 1]);
                if (serverPort <= 0) {
                    fprintf(stderr, "serverPort must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                i++;    // Move to the next flag
            }
            if (strcmp(argv[i], "-sip") == 0) {
                server_ip = strdup(argv[i + 1]);
                i++;    // Move to the next flag
            }
        }
    }

    signal(SIGINT, handle_sigint);

    char hostbuffer[256];
    char *clientIP;
    struct hostent *host_entry;
    int hostname;

    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    checkHostName(hostname);
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    checkHostEntry(host_entry);
    // To convert an Internet network address into ASCII string
    clientIP = inet_ntoa(*((struct in_addr *)
            host_entry->h_addr_list[0]));
    checkIPbuffer(clientIP);


    pthread_mutex_init(&mutex_buffer, NULL);
    pthread_mutex_init(&mutex_list, NULL);

    buffer = buffer_init(bufferSize);


    logOn(server_ip, serverPort, clientIP, portNum);
    getclients(server_ip, serverPort, clientIP, portNum);

    //logOff(server_ip, serverPort, clientIP, portNum);

    pthread_t *workerThread_array;
    workerThread_array = malloc(workerThreads * sizeof(pthread_t));


    for (int i = 0; i < workerThreads; i++) {
        int rc;
        rc = pthread_create(&workerThread_array[i], NULL, workerThread_job, (void*)dirName);
        if(rc)			/* could not create thread */
        {
            printf("\n ERROR: return code from pthread_create is %d \n", rc);
            exit(1);
        }
    }

    waitForSelect(portNum,dirName);

    for (int j = 0; j < workerThreads; ++j) {
        pthread_join(workerThread_array[j], NULL);     // wait to terminate threads
    }

    logOff(server_ip, serverPort, clientIP, portNum);

    pthread_mutex_destroy(&mutex_buffer);
    pthread_mutex_destroy(&mutex_list);
    pthread_cond_destroy(&condVarC);
    pthread_cond_destroy(&condVarP);

    deleteList(client_list);
    free(workerThread_array);
    free_buffer(buffer);
    free(server_ip);
    free(dirName);

    return 0;
}
