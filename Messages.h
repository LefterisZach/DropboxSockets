
#ifndef MESSAGES_H
#define MESSAGES_H

const char LOG_ON[] = "LOG_ON     ";
const char LOG_OFF[] ="LOG_OFF    ";
const char USER_ON[] = "USER_ON    ";
const char USER_OFF[] = "USER_OFF   ";
const char GET_CLIENTS[] = "GET_CLIENTS";
const char GET_FILE_LIST[] = "GET_FILE_LI";
const char FILE_LIST[] = "FILE_LIST  ";
const char GET_FILE[] = "GET_FILE   ";
const char FILE_UP_TO_DATE[] = "FILE_UP_TO_";
const char ERROR_IP_PORT_NOT_FOUND_IN_LIST[] = "ERROR_IP_PORT_NOT_FOUND_IN_LIST";
const char FILE_NOT_FOUND[] = "FILE_NOT_FO";
const char CLIENT_LIST[] = "CLIENT_LIST";


void create_directory(char *dir_name);
int dirExists(const char *dirname);
int fileExists(const char *filename);
unsigned long hash(unsigned char *str);
unsigned long getVersion(const char *filename);
int numOfFiles(char const *dirname);
void sendFiles(char const *dirname, int fd);

#endif //MESSAGES_H
