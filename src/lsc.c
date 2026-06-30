#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


typedef struct params  {
    char     target_dir[PATH_MAX];
    bool     long_format;
    bool     all;
    bool     human_readable;
    bool     sort_by_time;
    bool     sort_by_size;
    bool     reverse_order;
} params_t;


static int  parse_args(int argc, char **argv, params_t *params);


int main(int argc, char **argv) {

    params_t params = {
        .long_format = false,
        .all = false,
        .human_readable = false,
        .sort_by_time = false,
        .sort_by_size = false,
        .reverse_order = false
    };

    if (getcwd(params.target_dir, PATH_MAX) == NULL) {
        fprintf(stderr, "lsc: cannot get current directory: %s\n", strerror(errno));
        return -1;
    }

    int parse_res = argc > 1 ? parse_args(argc, argv, &params) : 0;

    if (parse_res != 0) {
        fprintf(stderr, "Usage: lsc [-laAhtrs] [path]\n");
        return -1;
    }

    printf("Long format: %s\n",       params.long_format    ? "True" : "False");
    printf("All: %s\n",               params.all            ? "True" : "False");
    printf("Human readable: %s\n",    params.human_readable ? "True" : "False");
    printf("Sort by time: %s\n",      params.sort_by_time   ? "True" : "False");
    printf("Sort by size: %s\n",      params.sort_by_size   ? "True" : "False");
    printf("Reverse order: %s\n",     params.reverse_order  ? "True" : "False");
    printf("Target directory: %s\n",  params.target_dir);

    return 0;
}


static int parse_args(int argc, char **argv, params_t *params) {
    
    bool path_parsed = false;

    for (int i = 1; i < argc; i++) {

        if (argv[i][0] != '-') { // directory

            if (path_parsed) {
                fprintf(stderr, "lsc: only one path allowed\n");
                return -1;
            }

            char temp_path[PATH_MAX];

            if (argv[i][0] == '/') { // absolute path
                snprintf(temp_path, PATH_MAX, "%s", argv[i]);
            } else {                 // relative path
                snprintf(temp_path, PATH_MAX, "%s/%s", params->target_dir, argv[i]);
            }

            if (realpath(temp_path, params->target_dir) == NULL) {
                fprintf(stderr, "lsc: cannot access '%s': %s\n", argv[i], strerror(errno));
                return -1;
            }

            path_parsed = true;

        } else { // options

            if (argv[i][1] == '-') { // long option
                
                if      (strcmp(argv[i], "--long")           == 0) params->long_format    = true;
                else if (strcmp(argv[i], "--all")            == 0) params->all            = true;
                else if (strcmp(argv[i], "--human-readable") == 0) params->human_readable = true;
                else if (strcmp(argv[i], "--sort-by-time")   == 0) params->sort_by_time   = true;
                else if (strcmp(argv[i], "--sort-by-size")   == 0) params->sort_by_size   = true;
                else if (strcmp(argv[i], "--reverse")        == 0) params->reverse_order  = true;
                else {
                    fprintf(stderr, "lsc: invalid long option '%s'\n", argv[i]);
                    return -1;
                }

            } else { // short option

                for (int j = 1; argv[i][j] != '\0'; j++) {
                    switch (argv[i][j]) {
                        case 'l': params->long_format    = true; break;
                        case 'a': params->all            = true; break;
                        case 'h': params->human_readable = true; break;
                        case 't': params->sort_by_time   = true; break;
                        case 's': params->sort_by_size   = true; break;
                        case 'r': params->reverse_order  = true; break;
                        default:
                            fprintf(stderr, "lsc: invalid short option -- '%c'\n", argv[i][j]);
                            return -1;
                    }
                }

            }

        }

    }

    return 0;
}