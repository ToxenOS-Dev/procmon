#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <pwd.h>
#include "procmon.h"

static int is_pid(const char *s){
    for(int i=0; s[i]; i++)
        if(!isdigit(s[i])) return 0;
    return 1;
}

void uid_to_name(int uid, char *out){
    struct passwd *pw = getpwuid(uid);
    if(pw) strcpy(out, pw->pw_name);
    else strcpy(out, "unknown");
}

static int read_process(Process *p, const char *pidstr){
    char path[64];
    snprintf(path,sizeof(path),"/proc/%s/status",pidstr);
    FILE *f = fopen(path,"r");
    if(!f) return 0;

    memset(p,0,sizeof(Process));
    p->pid = atoi(pidstr);
    p->cpu_time = get_proc_cpu(p->pid);

    char line[256];
    while(fgets(line,sizeof(line),f)){
        if(!strncmp(line,"Name:",5))      sscanf(line,"Name:\t%63s",p->name);
        else if(!strncmp(line,"State:",6)) sscanf(line,"State:\t%31[^\n]",p->state);
        else if(!strncmp(line,"VmRSS:",6)) sscanf(line,"VmRSS:\t%ld",&p->mem_kb);
        else if(!strncmp(line,"PPid:",5))  sscanf(line,"PPid:\t%d",&p->ppid);
        else if(!strncmp(line,"Uid:",4))   sscanf(line,"Uid:\t%d",&p->uid);
    }
    fclose(f);

    uid_to_name(p->uid, p->user);

    /* read full command line */
    snprintf(path, sizeof(path), "/proc/%s/cmdline", pidstr);
    FILE *fcmd = fopen(path, "r");

    if(fcmd){
        int n = fread(p->cmdline, 1, sizeof(p->cmdline)-1, fcmd);
        fclose(fcmd);

        for(int i=0;i<n-1;i++)
            if(p->cmdline[i] == 0) p->cmdline[i] = ' ';

        p->cmdline[n] = 0;
    }

    if(strlen(p->cmdline) == 0)
        strcpy(p->cmdline, p->name);

    return 1;
}

int load_processes(Process *list){
    DIR *d = opendir("/proc");
    struct dirent *e;
    int c = 0;
    if(!d) return 0;

    while((e = readdir(d)) && c < MAX_PROC){
        if(is_pid(e->d_name))
            if(read_process(&list[c], e->d_name))
                c++;
    }
    closedir(d);
    return c;
}

int process_match(Process *p, const char *filter){
    if(filter[0] == 0) return 1;
    return strstr(p->cmdline, filter) != NULL ||
           strstr(p->name, filter) != NULL;
}
