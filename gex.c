 /*
  * Copyright 2010 Alvaro Navarro <alnacle@gmail.com>
  */

#define _XOPEN_SOURCE 500 
#include <ftw.h>

#define _GNU_SOURCE
#include <string.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <sys/wait.h>

#define PATH_MAX 4096

int fd[2];

int
do_match(const char* re, const char* str, int size)
{
    int reti,i, pos = 0;
    char token[1024];
    regex_t regex;
    regmatch_t match;

    if (NULL == re) {
        return -1;
    }

    memset(token, 0, 1024);

    for (i=0; i < size; i++) {
        token[pos++] = str[i];

        if (strlen(str) == strlen(token)) {

            token[strlen(str) +1] = '\0';

            reti = regcomp(&regex, re, 0);
            if (reti){ 
                fprintf(stderr, "Could not compile regex\n"); 
                regfree(&regex);
                exit(1);
            }

            reti = regexec(&regex, token, 1, &match, 0);
            if (!reti) {
                regfree(&regex);
                return 1;
            }
            regfree(&regex);
            pos = 0;
        }
    }
    return 0;
}


int 
filter_files(const struct dirent* dire)
{
    if (strncmp(dire->d_name, ".",  2) == 0 ||
        strncmp(dire->d_name, "..", 3) == 0)
        return 0;

    return 1;
}

static int
display_info (const char *fpath, 
              const struct stat *sb, 
              int tflag, 
              struct FTW *ftwbuf)
{
    write(fd[1], fpath, strlen(fpath)+1);
    return 0;
}

        
void
get_files_from_directory(int fd, const char* d)
{
    nftw(d, display_info, 20, 0);
}

void
handle_file(const char* file, const char* re)
{
    FILE* fd = fopen(file, "r");
    char line[256];

    if (!fd) {
        fprintf(stderr, "Error opening %s\n", file);
        return;
    }

    while ( fgets ( line, sizeof line, fd ) != NULL ) {
        if (do_match(re, line, sizeof(line))) {
            fclose(fd);
            printf("%s\n", file);
            return;
        }
    }
    fclose(fd);
}

void
match_files(const char* re, const char* file, int size)
{
    int count, len, sta = 0;
    const char *e = file;
    char *sub;

    for (count = 0; count < size; count++) {
        if (*e == '\0' && len != 0) {
            sub = strndup (file + sta, len);
            handle_file(sub, re);
            free(sub);
            sta = count + 1;
            len = 0;
        } else {
            len++;
        }
        ++e;
    }
}

void
help (char argv[])
{
    printf ("%s <regular_expression> [directory]\n\n", argv);
}

int
main (int argc, char *argv[])
{
    char* cwd;
    int n;
    char readbuffer[BUFSIZ];

    if (2 == argc) {
        cwd = (char*)malloc(PATH_MAX + 1);

        if (NULL == getcwd(cwd, PATH_MAX)) {
            printf("Error while getting current directory: %s\n", strerror(errno));
        }
    }
    else if (3 == argc) {
        cwd = (char*)malloc(strlen(argv[2])+1);

        if (NULL == cwd) {
            perror("malloc() error");
        }

        strncpy(cwd, argv[2], strlen(argv[2]));
    }
    else if (1 == argc) {
        help(argv[0]);
        exit(0);
    }
    else {
        printf ("Too many arguments. Exiting...\n");
        exit(-1);
    }

    pipe(fd);

    switch (fork()) {
        case -1:
            exit (1);
            break;
        case 0:
            close(fd[0]);
            get_files_from_directory(fd[1], cwd);
            break;
        default:
            close(fd[1]);

            for (;;){
                n = read(fd[0], readbuffer, sizeof(readbuffer));

                if (n > 0) {
                    match_files(argv[1], readbuffer, n);
                }
                else {
                    exit(1);
                }
            }
            break;
    }

    free(cwd);
    cwd = NULL;

    return 0;
}
