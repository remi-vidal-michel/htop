#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define MAX_PATH_LEN 512

int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

void get_process_name(const char *pid, char *name) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/proc/%s/comm", pid);

    FILE *file = fopen(path, "r");
    if (file) {
        fgets(name, MAX_PATH_LEN, file);
        fclose(file);

        // Remove newline character if present
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') {
            name[len - 1] = '\0';
        }
    } else {
        // Error handling if unable to open the file
        snprintf(name, MAX_PATH_LEN, "N/A");
    }
}

void display_processes() {
    // Get the list of processes
    DIR *dir;
    struct dirent *entry;
    dir = opendir("/proc");
    if (dir == NULL) {
        endwin();
        printf("Error opening /proc\n");
        exit(1);
    }

    // Display the list of processes with PID and process name
    int row = 0;
    char name[MAX_PATH_LEN];

    while ((entry = readdir(dir)) != NULL) {
        // Check if d_name is a numeric value
        if (is_numeric(entry->d_name)) {
            get_process_name(entry->d_name, name);
            mvprintw(row, 0, "PID : %-10s Name : %s", entry->d_name, name);
            row++;
        }
    }
    closedir(dir);
}

int main() {
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();

    // Display processes
    display_processes();

    // Refresh the window
    refresh();

    // Wait for the user to press a key
    getch();

    // End ncurses
    endwin();

    return 0;
}