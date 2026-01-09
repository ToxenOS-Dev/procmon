#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ncurses.h>

#define MAX_PROC 1024

typedef struct {
    int pid;
    char name[64];
    char state[32];
    long mem_kb;
    unsigned long long cpu_time;
    double cpu_percent;
} Process;

typedef struct {
    int pid;
    unsigned long long cpu_time;
} CpuSnapshot;

CpuSnapshot prev[MAX_PROC];
int prev_count = 0;
unsigned long long prev_total_cpu = 0;

int selected = 0;

/* ---------------- CPU ENGINE ---------------- */

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

unsigned long long find_prev(int pid){
    for(int i=0;i<prev_count;i++)
        if(prev[i].pid==pid) return prev[i].cpu_time;
    return 0;
}

/* ---------------- PROCESS LOADER ---------------- */

int is_pid(const char *s){
    for(int i=0;s[i];i++) if(!isdigit(s[i])) return 0;
    return 1;
}

int read_process(Process *p, const char *pidstr){
    char path[64];
    snprintf(path,sizeof(path),"/proc/%s/status",pidstr);
    FILE *f=fopen(path,"r");
    if(!f) return 0;

    memset(p,0,sizeof(Process));
    p->pid=atoi(pidstr);
    p->cpu_time=get_proc_cpu(p->pid);

    char line[256];
    while(fgets(line,sizeof(line),f)){
        if(!strncmp(line,"Name:",5)) sscanf(line,"Name:\t%63s",p->name);
        else if(!strncmp(line,"State:",6)) sscanf(line,"State:\t%31[^\n]",p->state);
        else if(!strncmp(line,"VmRSS:",6)) sscanf(line,"VmRSS:\t%ld",&p->mem_kb);
    }
    fclose(f);
    return 1;
}

int load_processes(Process *list){
    DIR *d=opendir("/proc");
    struct dirent *e;
    int c=0;
    if(!d) return 0;

    while((e=readdir(d)) && c<MAX_PROC){
        if(is_pid(e->d_name))
            if(read_process(&list[c],e->d_name)) c++;
    }
    closedir(d);
    return c;
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

/* ---------------- UI ---------------- */

int cmp_cpu(const void*a,const void*b){
    double d=((Process*)b)->cpu_percent-((Process*)a)->cpu_percent;
    return (d>0)-(d<0);
}

void draw_ui(Process *list,int count){
    erase();
    mvprintw(0,2,"PROC MON v6  |  ↑↓ move  k kill  q quit");
    mvprintw(1,0," PID     CPU%%    MEM(KB)   STATE        NAME");
    mvhline(2,0,'-',COLS);

    int max=LINES-4;
    for(int i=0;i<count && i<max;i++){
        if(i==selected) attron(A_REVERSE);
        mvprintw(3+i,0," %-6d  %5.1f   %-8ld %-12s %s",
                  list[i].pid,
                  list[i].cpu_percent,
                  list[i].mem_kb,
                  list[i].state,
                  list[i].name);
        if(i==selected) attroff(A_REVERSE);
    }

    mvprintw(LINES-1,2,"Processes: %d",count);
    refresh();
}

/* ---------------- MAIN ---------------- */

int main(){
    Process list[MAX_PROC];

    initscr();
    noecho();
    cbreak();
    keypad(stdscr,TRUE);
    curs_set(0);
    timeout(800);

    while(1){
        int count=load_processes(list);
        update_cpu(list,count);
        qsort(list,count,sizeof(Process),cmp_cpu);

        if(selected>=count) selected=count-1;
        if(selected<0) selected=0;

        draw_ui(list,count);

        int ch=getch();
        if(ch=='q') break;
        else if(ch==KEY_DOWN) selected++;
        else if(ch==KEY_UP) selected--;
        else if(ch=='k' && count>0){
            kill(list[selected].pid,SIGTERM);
        }
    }

    endwin();
    return 0;
}
