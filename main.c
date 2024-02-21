#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define MAX_PATH_LEN 512
#define MAX_NAME_LEN 256

int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

char* get_process_name(const char *pid) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/proc/%s/comm", pid);

    FILE *file = fopen(path, "r");
    if (file) {
        char* name = malloc(MAX_NAME_LEN * sizeof(char));
        if (!name) {
            fclose(file);
            return NULL;
        }
        fgets(name, MAX_NAME_LEN, file);
        fclose(file);

        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') {
            name[len - 1] = '\0';
        }
        return name;
    }
    return NULL;
}

unsigned long get_process_memory(const char *pid) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/proc/%s/statm", pid);

    FILE *statm_file = fopen(path, "r");
    if (statm_file) {
        unsigned long mem_pages;
        fscanf(statm_file, "%lu", &mem_pages);
        fclose(statm_file);
        return mem_pages;
    }
    return 0;
}

void display_processes() {
    DIR *dir;
    struct dirent *entry;
    dir = opendir("/proc");
    if (dir == NULL) {
        endwin();
        printf("Erreur lors de l'ouverture de /proc\n");
        exit(1);
    }

    int row = 0;
    mvprintw(row, 0, "%-10s %-25s %-15s", "PID", "Nom", "Mémoire");
    row++;

    while ((entry = readdir(dir)) != NULL) {
        if (is_numeric(entry->d_name)) {
            char* process_name = get_process_name(entry->d_name);
            if (process_name) {
                unsigned long mem_pages = get_process_memory(entry->d_name);
                mvprintw(row, 0, "%-10s %-25s %-15lu", entry->d_name, process_name, mem_pages);
                free(process_name);
                row++;
            }
        }
    }
    closedir(dir);
}

int main() {
    initscr();
    noecho();
    cbreak();
    display_processes();
    refresh();
    getch();
    endwin();

    return 0;
}