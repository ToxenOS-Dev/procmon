#ifndef PROCMON_H
#define PROCMON_H

#include <dirent.h>

#define MAX_PROC 1024

typedef struct {
    int pid;
    int ppid;
    int uid;

    char name[64];
    char state[32];
    char user[32];
    char cmdline[512];

    long mem_kb;
    unsigned long long cpu_time;
    double cpu_percent;
} Process;

/* cpu.c */
unsigned long long get_total_cpu();
unsigned long long get_proc_cpu(int pid);
void update_cpu(Process *list, int count);

/* proc.c */
int load_processes(Process *list);
int process_match(Process *p, const char *filter);

/* ui.c */
void draw_ui(Process *list, int count);

#endif
