#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "procmon.h"

typedef struct {
    int pid;
    unsigned long long cpu_time;
} CpuSnapshot;

static CpuSnapshot prev[MAX_PROC];
static int prev_count = 0;
static unsigned long long prev_total_cpu = 0;

unsigned long long get_total_cpu() {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;

    unsigned long long u,n,s,i,io,irq,soft;
    fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu",
           &u,&n,&s,&i,&io,&irq,&soft);
    fclose(f);
    return u+n+s+i+io+irq+soft;
}

unsigned long long get_proc_cpu(int pid) {
    char path[64], buf[1024];
    snprintf(path,sizeof(path),"/proc/%d/stat",pid);
    FILE *f = fopen(path,"r");
    if(!f) return 0;

    fgets(buf,sizeof(buf),f);
    fclose(f);

    char *tok = strtok(buf," ");
    int field=1;
    unsigned long long ut=0, st=0;

    while(tok){
        if(field==14) ut=strtoull(tok,NULL,10);
        if(field==15){ st=strtoull(tok,NULL,10); break; }
        tok=strtok(NULL," ");
        field++;
    }
    return ut+st;
}

static unsigned long long find_prev(int pid){
    for(int i=0;i<prev_count;i++)
        if(prev[i].pid==pid) return prev[i].cpu_time;
    return 0;
}

void update_cpu(Process *list,int count){
    unsigned long long total=get_total_cpu();
    unsigned long long diff=total-prev_total_cpu;

    for(int i=0;i<count;i++){
        unsigned long long old=find_prev(list[i].pid);
        unsigned long long d=list[i].cpu_time-old;
        list[i].cpu_percent = diff ? (double)d*100.0/diff : 0.0;
    }

    prev_count=count;
    for(int i=0;i<count;i++){
        prev[i].pid=list[i].pid;
        prev[i].cpu_time=list[i].cpu_time;
    }
    prev_total_cpu=total;
}
