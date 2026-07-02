#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>


#define PERMISSIONS_MAX 11
#define FILE_SIZE_MAX   20
#define OWNER_MAX       32
#define DATE_MAX        32


typedef struct params  {
    char     target_dir[PATH_MAX];
    bool     long_format;
    bool     all;
    bool     almost_all;
    bool     human_readable;
    bool     sort_by_time;
    bool     sort_by_size;
    bool     reverse_order;
} params_t;

typedef struct entry   {
    char     permissions[PERMISSIONS_MAX];
    char     file_size[FILE_SIZE_MAX];
    char     owner[OWNER_MAX];
    char     date[DATE_MAX];
    char     name[NAME_MAX];
    time_t   time_sort;
    off_t    size_sort;
    char     link_path[PATH_MAX];
} entry_t;

typedef struct entries {
    size_t   capacity;
    size_t   count;
    entry_t *data;
} entries_t;

static int  parse_args(int argc, char **argv, params_t *params);

static int  parse_entries(entries_t *entries, params_t *params);

static void sort(entries_t *entries, int (*predicate)(const void *, const void *));

static int  compare_time_desc(const void *a, const void *b);

static int  compare_time_asc (const void *a, const void *b);

static int  compare_size_desc(const void *a, const void *b);

static int  compare_size_asc (const void *a, const void *b);

static int  compare_name_desc(const void *a, const void *b);

static int  compare_name_asc (const void *a, const void *b);


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

    entries_t entries = {
        .capacity = 10,
        .count = 0,
        .data = calloc(10, sizeof(entry_t))
    };

    if (entries.data == NULL) {
        fprintf(stderr, "lsc: memory allocation failed\n");
        return -1;
    }

    if (parse_entries(&entries, &params) != 0) {
        free(entries.data);
        return -1;
    }

    if (entries.count == 0) {
        free(entries.data);
        return 0;
    }

    if (params.sort_by_time) {
        sort(&entries,  params.reverse_order ? compare_time_asc : compare_time_desc);
    } else if (params.sort_by_size) {
        sort(&entries,  params.reverse_order ? compare_size_asc : compare_size_desc);
    } else {
        sort(&entries, !params.reverse_order ? compare_name_asc : compare_name_desc);
    }

    if (!params.long_format) {
        for (size_t i = 0; i < entries.count; i++) {
            printf("%s ", entries.data[i].name);
        }
        printf("\n");
    } else {
        for (size_t i = 0; i < entries.count; i++) {
            printf("%s ", entries.data[i].permissions);
            printf("%s ", entries.data[i].file_size);
            printf("%s ", entries.data[i].owner);
            printf("%s ", entries.data[i].date);
            printf("%s ", entries.data[i].name);
            printf("\n");
        }
        printf("\n");
    }

    free(entries.data);

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
                else if (strcmp(argv[i], "--almost-all")     == 0) params->almost_all     = true;
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
                        case 'A': params->almost_all     = true; break;
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

static int parse_entries(entries_t *entries, params_t *params) {
    
    DIR *dir = opendir(params->target_dir);
    if (dir == NULL) {
        fprintf(stderr, "lsc: cannot open directory '%s': %s\n", params->target_dir, strerror(errno));
        return -1;
    }

    struct dirent *entry;

    for (; (entry = readdir(dir)) != NULL; entries->count++) {
        
        if (!params->all && !params->almost_all && entry->d_name[0] == '.') {
            continue;
        }
        if (!params->all && params->almost_all && 
            (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        
        if (entries->count >= entries->capacity) {
            entries->capacity *= 2;
            entry_t *temp = realloc(entries->data, entries->capacity * sizeof(entry_t));

            if (temp == NULL) {
                fprintf(stderr, "lsc: memory allocation failed: %s\n", strerror(errno));
                closedir(dir);
                return -1;
            }

            entries->data = temp;
        }

        entry_t *e = &entries->data[entries->count];

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", params->target_dir, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == -1) { // we need lstat always for color output
            fprintf(stderr, "lsc: cannot access '%s': %s\n", entry->d_name, strerror(errno));
            closedir(dir);
            return -1;
        }

        snprintf(e->name, NAME_MAX, "%s", entry->d_name);
        if      (S_ISSOCK(st.st_mode))   e->permissions[0] = 's';
        else if (S_ISREG (st.st_mode))   e->permissions[0] = '-';
        else if (S_ISBLK (st.st_mode))   e->permissions[0] = 'b';
        else if (S_ISDIR (st.st_mode))   e->permissions[0] = 'd';
        else if (S_ISCHR (st.st_mode))   e->permissions[0] = 'c';
        else if (S_ISFIFO(st.st_mode))   e->permissions[0] = 'p';
        else  /*(S_ISLNK (st.st_mode))*/ e->permissions[0] = 'l';

        e->permissions[3]  = (st.st_mode & S_IXUSR) ? 'x' : '-';
        e->permissions[6]  = (st.st_mode & S_IXGRP) ? 'x' : '-';
        e->permissions[9]  = (st.st_mode & S_IXOTH) ? 'x' : '-';

        e->time_sort = st.st_mtime;
        e->size_sort = st.st_size;

        if (!params->long_format) continue;

        char link_target[PATH_MAX];
        ssize_t len = readlink(full_path, link_target, PATH_MAX - 1);
        if (len != -1) {
            link_target[len] = '\0';
            snprintf(e->link_path, PATH_MAX, "%s", link_target);
        } else {
            fprintf(stderr, "lsc: cannot read link '%s': %s\n", entry->d_name, strerror(errno));
        }

        e->permissions[1]  = (st.st_mode & S_IRUSR) ? 'r' : '-';
        e->permissions[2]  = (st.st_mode & S_IWUSR) ? 'w' : '-';
        e->permissions[4]  = (st.st_mode & S_IRGRP) ? 'r' : '-';
        e->permissions[5]  = (st.st_mode & S_IWGRP) ? 'w' : '-';
        e->permissions[7]  = (st.st_mode & S_IROTH) ? 'r' : '-';
        e->permissions[8]  = (st.st_mode & S_IWOTH) ? 'w' : '-';
        e->permissions[10] = '\0';

        if (params->human_readable) {
            off_t size = st.st_size;
            if (size >= 1024 * 1024 * 1024) {
                snprintf(e->file_size, FILE_SIZE_MAX, "%.1fG", (double)size / (1024 * 1024 * 1024));
            } else if (size >= 1024 * 1024) {
                snprintf(e->file_size, FILE_SIZE_MAX, "%.1fM", (double)size / (1024 * 1024));
            } else if (size >= 1024) {
                snprintf(e->file_size, FILE_SIZE_MAX, "%.1fK", (double)size / 1024);
            } else {
                snprintf(e->file_size, FILE_SIZE_MAX, "%ldB",  size);
            }
        } else {
            snprintf(e->file_size, FILE_SIZE_MAX, "%ld", st.st_size);
        }

        struct passwd *pw = getpwuid(st.st_uid);
        snprintf(e->owner, OWNER_MAX, "%s",  pw ? pw->pw_name : "unknown");

        struct tm *tm = localtime(&st.st_mtime);
        strftime(e->date, DATE_MAX, "%d.%m.%Y. %H:%M", tm);
    }

    closedir(dir);

    return 0;
}

static void sort(entries_t *entries, int (*predicate)(const void *, const void *)) {
    qsort(entries->data, entries->count, sizeof(entry_t), predicate);
}

static int compare_time_desc(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *) a;
    const entry_t *eb = (const entry_t *) b;

    return eb->time_sort == ea->time_sort ? 0 : 
           eb->time_sort >  ea->time_sort ? 1 : -1;
}

static int compare_time_asc(const void *a, const void *b) {
    return compare_time_desc(b, a);
}

static int compare_size_desc(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *) a;
    const entry_t *eb = (const entry_t *) b;

    return eb->size_sort == ea->size_sort ? 0 : 
           eb->size_sort >  ea->size_sort ? 1 : -1;
}

static int compare_size_asc(const void *a, const void *b) {
    return compare_size_desc(b, a);
}

static int compare_name_desc(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *) a;
    const entry_t *eb = (const entry_t *) b;

    if (strcmp(ea->name, ".")  == 0) return  1;
    if (strcmp(ea->name, "..") == 0) return  1;
    if (strcmp(eb->name, ".")  == 0) return -1;
    if (strcmp(eb->name, "..") == 0) return -1;

    const char *na = ea->name[0] == '.' ? ea->name + 1 : ea->name;
    const char *nb = eb->name[0] == '.' ? eb->name + 1 : eb->name;

    return strcasecmp(nb, na);
}

static int compare_name_asc(const void *a, const void *b) {
    return compare_name_desc(b, a);
}