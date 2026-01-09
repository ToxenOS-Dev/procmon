#ifndef PROCMON_H
#define PROCMON_H

#include <dirent.h>

#define MAX_PROC 1024

typedef struct {
    int pid;
    int ppid;
    char name[64];
    char state[32];
    long mem_kb;
    unsigned long long cpu_time;
    double cpu_percent;
} Process;

unsigned long long get_total_cpu();
unsigned long long get_proc_cpu(int pid);
void update_cpu(Process *list, int count);

int load_processes(Process *list);

void draw_ui(Process *list, int count);

int process_match(Process *p, const char *filter);

#endif
