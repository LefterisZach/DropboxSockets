#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h>

#define DIR_PERMISSIONS 0777

// This function is creating a directory
void create_directory(char *dir_name) {

    if (mkdir(dir_name, DIR_PERMISSIONS) == -1)     // Creating a directory
        fprintf(stderr,"Directory already exists\n");
    else
        printf("Directory %s created\n",dir_name);      // Creation success
}


int dirExists(const char *dirname) {
    struct stat buffer;
    int exist = stat(dirname, &buffer);
    if (exist == 0)
        return 1;
    else
        return 0;
}

int fileExists(const char *filename) {
    struct stat buffer;
    int exist = stat(filename, &buffer);
    if (exist == 0)
        return 1;
    else
        return 0;
}

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

unsigned long getVersion(const char *filename) {
    long length = 0;
    char *buffer = NULL;
    unsigned long res = 0;
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buffer = malloc(length + 1);
        if (buffer) {
            fread(buffer, 1, length + 1, fp);
        }

        fclose(fp);
        buffer[length] = '\0';
    }
    res = hash(buffer);
    return res;
}

//This function returns the N number of files inside a directory
int numOfFiles(char const *dirname) {

    static int count = 0;

    if (!dirExists(dirname)) {
        fprintf(stderr, "Directory %s doesn't exist\n", dirname);
        return EXIT_FAILURE;
    }

    struct dirent *de;  // Pointer for directory entry
    DIR *dr = opendir(dirname); /* opendir() returns a pointer of DIR type.*/

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        fprintf(stderr, "Could not open directory %s", dirname);
        perror("");
        return EXIT_FAILURE;
    }

    while ((de = readdir(dr)) != NULL) {
        // Ignore . and ..
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }

        if (de->d_type == DT_DIR) {

            char *subdir;
            // Allocate memory for the subdirectory.
            // 1 additional for the '/' and the second additional for '\0'.
            subdir = malloc(strlen(dirname) + strlen(de->d_name) + 2);

            // Flesh out the subdirectory name.
            strcpy(subdir, dirname);
            strcat(subdir, "/");
            strcat(subdir, de->d_name);

            numOfFiles(subdir);

            free(subdir);
        }

        if (de->d_type != DT_DIR) {   //Exclude subdirectory names
            count++;
        }
    }
    closedir(dr);
    return count;
}

void sendFiles(char const *dirname, int fd) {

    char buf[1024];
    char fullpath[1024];

    if (!dirExists(dirname)) {
        fprintf(stderr, "Directory %s doesn't exist\n", dirname);
        return;
    }

    struct dirent *de;  // Pointer for directory entry
    DIR *dr = opendir(dirname); /* opendir() returns a pointer of DIR type.*/

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        fprintf(stderr, "Could not open directory %s", dirname);
        perror("");
        return;
    }

    while ((de = readdir(dr)) != NULL) {
        // Ignore . and ..
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }

        if (de->d_type == DT_DIR) {

            char *subdir;
            // Allocate memory for the subdirectory.
            // 1 additional for the '/' and the second additional for '\0'.
            subdir = malloc(strlen(dirname) + strlen(de->d_name) + 2);

            // Flesh out the subdirectory name.
            strcpy(subdir, dirname);
            strcat(subdir, "/");
            strcat(subdir, de->d_name);

            sendFiles(subdir, fd);

            free(subdir);
        }

        if (de->d_type != DT_DIR) {   //Exclude subdirectory names
            sprintf(fullpath, "%s/%s", dirname, de->d_name);    //1.fullpath has /home/lefteris/CLionProjects/client/dir/7ktkQVi.txt
            sprintf(buf, "%s,%4lu", fullpath, getVersion(fullpath));  //2.buf has /home/lefteris/CLionProjects/client/dir/EC.txt,4370388601468017689
            send(fd, buf, 128 + 1 + 4 + 1, 0);
        }
    }
    closedir(dr);
}