#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_PATH_LEN 512
#define MAX_NAME_LEN 256
#define MAX_PROCESSES_DISPLAYED 20
#define MAX_TOTAL_PROCESSES 256

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

int pid_sort_order = 1;
int name_sort_order = -1;
int memory_sort_order = -1;

int compare_by_pid(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    int pid_a = atoi(pa->pid);
    int pid_b = atoi(pb->pid);
    return pid_sort_order * (pid_a - pid_b);
}

int compare_by_name(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    return name_sort_order * strcasecmp(pa->name, pb->name);
}

int compare_by_memory(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    return memory_sort_order * ((pa->memory > pb->memory) - (pa->memory < pb->memory));
}

char* get_process_name(const char *pid) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/proc/%s/comm", pid);

    FILE *file = fopen(path, "r");
    if (file) {
        static char name[MAX_NAME_LEN];
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
        return mem_pages * 4;
    }
    return 0;
}

void display_processes(ProcessInfo *processes, int start_index, int total_processes, int current_sort) {
    int row = 0;
    switch (current_sort) {
        case 0:
            attron(A_REVERSE);
            mvprintw(row, 0, "%-10s", "PID");
            attroff(A_REVERSE);
            printw(" %-25s %-15s", "NOM", "MEM");
            break;
        case 1:
            mvprintw(row, 0, "%-10s", "PID");
            attron(A_REVERSE);
            printw(" %-25s", "NOM");
            attroff(A_REVERSE);
            printw(" %-15s", "MEM");
            break;
        case 2:
            mvprintw(row, 0, "%-10s %-25s", "PID", "NOM");
            attron(A_REVERSE);
            printw(" %-15s", "MEM");
            attroff(A_REVERSE);
            break;
        default:
            mvprintw(row, 0, "%-10s %-25s %-15s", "PID", "NOM", "MEM");
            break;
    }

    row += 2;

    int end_index = start_index + MAX_PROCESSES_DISPLAYED;
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
    ProcessInfo processes[MAX_TOTAL_PROCESSES];

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

    timeout(1000);

    do {
        clear();
        display_processes(processes, start_index, total_processes, current_sort);
        refresh();
        ch = getch();

        switch (ch) {
            case KEY_DOWN:
                if (start_index + MAX_PROCESSES_DISPLAYED < total_processes)
                    start_index++;
                break;
            case KEY_UP:
                if (start_index > 0)
                    start_index--;
                break;
            case '1':
                pid_sort_order *= -1;
                name_sort_order = -1;
                memory_sort_order = -1;
                qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_pid);
                current_sort = 0;
                break;
            case '2':
                name_sort_order *= -1;
                pid_sort_order = -1;
                memory_sort_order = -1;
                qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_name);
                current_sort = 1;
                break;
            case '3':
                memory_sort_order *= -1;
                pid_sort_order = -1;
                name_sort_order = -1;
                qsort(processes, total_processes, sizeof(ProcessInfo), compare_by_memory);
                current_sort = 2;
                break;
            default:
                break;
        }

    } while (ch != 'q');

    endwin();

    return 0;
}