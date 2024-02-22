#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define MAX_PATH_LEN 512
#define MAX_NAME_LEN 256
#define MAX_PROCESSES 20

int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

typedef struct {
    char pid[MAX_NAME_LEN];
    char name[MAX_NAME_LEN];
    unsigned long memory;
} ProcessInfo;

int compare_by_pid(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    int pid_a = atoi(pa->pid);
    int pid_b = atoi(pb->pid);
    return pid_a - pid_b;
}

int compare_by_name(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    return strcasecmp(pa->name, pb->name);
}

int compare_by_memory(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    if (pa->memory < pb->memory) return -1;
    if (pa->memory > pb->memory) return 1;
    return 0;
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
        return mem_pages*4;
    }
    return 0;
}

void display_processes(ProcessInfo *processes, int start_index, int total_processes) {
    int row = 0;
    mvprintw(row, 0, "%-10s %-25s %-15s", "PID", "Nom", "Mémoire");
    row++;

    int end_index = start_index + MAX_PROCESSES;
    if (end_index > total_processes) {
        end_index = total_processes;
    }

    for (int i = start_index; i < end_index; i++) {
        mvprintw(row, 0, "%-10s %-25s %-15lu", processes[i].pid, processes[i].name, processes[i].memory);
        row++;
    }
}

int main() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int start_index = 0;
    int total_processes = 0;
    int current_sort = 0;
    ProcessInfo *processes = NULL;

    DIR *dir;
    struct dirent *entry;
    dir = opendir("/proc");
    if (dir == NULL) {
        endwin();
        printf("Erreur lors de l'ouverture de /proc\n");
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (is_numeric(entry->d_name)) {
            total_processes++;
        }
    }
    closedir(dir);

    processes = malloc(total_processes * sizeof(ProcessInfo));
    if (!processes) {
        endwin();
        printf("Erreur lors de l'allocation de la mémoire\n");
        exit(1);
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        endwin();
        printf("Erreur lors de l'ouverture de /proc\n");
        exit(1);
    }

    int process_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (is_numeric(entry->d_name)) {
            strncpy(processes[process_count].pid, entry->d_name, MAX_NAME_LEN);
            processes[process_count].pid[MAX_NAME_LEN - 1] = '\0';
            char* process_name = get_process_name(entry->d_name);
            if (process_name) {
                strncpy(processes[process_count].name, process_name, MAX_NAME_LEN);
                free(process_name);
            } else {
                strncpy(processes[process_count].name, "N/A", MAX_NAME_LEN);
            }
            processes[process_count].memory = get_process_memory(entry->d_name);
            process_count++;
        }
    }
    closedir(dir);

    qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_pid);

    int ch;

    do {
        clear();
        display_processes(processes, start_index, total_processes);
        refresh();
        ch = getch();

        switch (ch) {
            case KEY_DOWN:
                if (start_index + MAX_PROCESSES < total_processes)
                    start_index++;
                break;
            case KEY_UP:
                if (start_index > 0)
                    start_index--;
                break;
            case '1':
                qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_pid);
                current_sort = 0;
                break;
            case '2':
                qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_name);
                current_sort = 1;
                break;
            case '3':
                qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_memory);
                current_sort = 2;
                break;
            default:
                break;
        }

    } while (ch != 'q');

    free(processes);
    endwin();

    return 0;
}