/**
 * psearch.c
 *
 * Performs a parallel directory search over the file system.
 *
 * Author: Anthony Panisales
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>
#include <unistd.h>

void print_usage(char *argv[]);
void print_path(char * fullPathName, char * pathName, char * term);
void search_directory(DIR * directory, char * dirName, void *opts);
void *search_thread(void *opts);
void strlower(char *string);

bool eFlag = false;
char * startDirectory;
sem_t semaphore;

/**
* print_path
*
* Checks if a path name is valid and prints it, if
* it is
*
* @args Path name to test, search term
*/
void print_path(char * fullPathName, char * pathName, char * term) {
    char pathBuffer[PATH_MAX + 1];
    char * pathNameCopy = pathName;

    strlower(pathNameCopy);
    if (eFlag) {
        if (strcmp(pathNameCopy, term) == 0) {
            realpath(fullPathName, pathBuffer);
            printf("%s \n", pathBuffer);
        }
    } else {
        if (strstr(pathNameCopy, term) != NULL) {
            realpath(fullPathName, pathBuffer);
            printf("%s \n", pathBuffer);
        }
    }
}

/**
* strlower
*
* Converts all characters in a string to lowercase, if not
* already lowercase
*
* @args String to convert to lowercase
*/
void strlower(char *string) {
    if (string != NULL) {
        int i = 0;
        for (i = 0; i < strlen(string); i++) {
            string[i] = tolower(string[i]);
        }
    }
}

/**
* search_directory
*
* Recursively searches through a directory for file names that
* contain a specific search term
*
* @args Pointer to directory to search, pointer to search term
*/
void search_directory(DIR * directory, char * dirName, void *opts)
{
    struct dirent * entry;
    DIR * minidir;
    
    while ((entry = readdir(directory)) != NULL) {
        char pathString[PATH_MAX + 1] = "";
        strcpy(pathString, dirName);
        strcat(pathString, "/");
        strcat(pathString, entry->d_name);
        print_path(pathString, entry->d_name, opts);
        if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        if ((minidir = opendir(pathString)) != NULL) {
            search_directory(minidir, pathString, opts);
        }
    }
    closedir(directory);
}

/**
* search_thread
*
* Calls the function "search_directory" in order to recursively
* search through a directory for file names that contain a specific 
* search term
*
* @args Pointer to search term
*/
void *search_thread(void *opts)
{
    search_directory(opendir(startDirectory), startDirectory, opts);
    sem_post(&semaphore);
    return 0;
}

/**
* print_usage
*
* Prints usage information
*
* @args Pointer to command line argument array
*/
void print_usage(char *argv[])
{
    printf("Usage: %s [-eh] [-d directory] [-t threads] "
            "search_term1 search_term2 ... search_termN\n" , argv[0]);
    printf("\n");
    printf("Options:\n"
           "    * -d directory    specify start directory (default: CWD)\n"
           "    * -e              print exact name matches only\n"
           "    * -h              show usage information\n"
           "    * -t threads      set maximum threads (default: # procs)\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    int c, i, maxThreads;
    char * argStartDir = NULL;
    bool dFlag = false;
    bool tFlag = false;
    opterr = 0;
    DIR * dir;

    /* Handle command line options */
    while ((c = getopt(argc, argv, "d:eht:")) != -1) {
        switch (c) {
            case 'd':
                dFlag = true;
                argStartDir = optarg;
                break;
            case 'e':
                eFlag = true;
                break;
            case 'h':
                print_usage(argv);
                return 0;
            case 't':
                tFlag = true;
                maxThreads = atoi(optarg);
                if (maxThreads <= 0) {
                    printf("Error: Invalid argument for option -t.\n");
                    return 1;
                }
                break;
            case '?':
                if (optopt == 'd' || optopt == 't') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                return 1;
            default:
                abort();
        }
    }

    if (!dFlag) {
        startDirectory = ".";
    } else {
        if ((dir = opendir(argStartDir)) == NULL) {
            perror("opendir");
            return 1;
        }
        startDirectory = argStartDir;
    }

    if (!tFlag) {
        maxThreads = get_nprocs();
    }

    pthread_t * threads = malloc((argc-optind) * sizeof(threads));
    int sizeOfThreadsArray = argc-optind;
    sem_init(&semaphore, 0, maxThreads);
    for (i = optind; i < argc; i++) {
        sem_wait(&semaphore);
        strlower(argv[i]);
        pthread_create(&threads[i-optind], NULL, search_thread, argv[i]);
    }

    i = 0;
    for (i = 0; i < sizeOfThreadsArray; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    sem_destroy(&semaphore);

    return 0;
}
