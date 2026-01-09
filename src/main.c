#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include "procmon.h"

#define CONFIG_FILE "/data/data/com.termux/files/home/.procmonrc"

int selected=0, tree_mode=0, search_mode=0, scroll_offset=0;
int show_details = 0;
int theme = 0;

char search_filter[64]="";
char local_user[32]="";
char real_user[32]="";

int depth[MAX_PROC];
int collapsed[32768];

static int used[MAX_PROC];

/* ---------- SORT ---------- */
int cmp_cpu(const void *a,const void *b){
    double d=((Process*)b)->cpu_percent-((Process*)a)->cpu_percent;
    return (d>0)-(d<0);
}

/* ---------- TREE ---------- */
void dfs(Process *src,int count,Process *out,int *outc,int pid,int d){
    if(collapsed[pid]) return;

    for(int i=0;i<count;i++){
        if(!used[i] && src[i].ppid==pid){
            used[i]=1;
            out[*outc]=src[i];
            depth[*outc]=d;
            (*outc)++;
            dfs(src,count,out,outc,src[i].pid,d+1);
        }
    }
}

int build_tree(Process *src,int count,Process *out){
    for(int i=0;i<count;i++) used[i]=0;
    int outc=0;

    for(int i=0;i<count;i++){
        int parent=0;
        for(int j=0;j<count;j++)
            if(src[i].ppid==src[j].pid) parent=1;

        if(!parent){
            used[i]=1;
            out[outc]=src[i];
            depth[outc]=0;
            outc++;
            dfs(src,count,out,&outc,src[i].pid,1);
        }
    }
    return outc;
}

/* ---------- CONFIG ---------- */
void load_user(){
    FILE *f=fopen(CONFIG_FILE,"r");
    if(!f) return;
    char line[128];
    if(fgets(line,sizeof(line),f)){
        if(!strncmp(line,"user=",5)){
            strcpy(local_user,line+5);
            local_user[strcspn(local_user,"\n")]=0;
        }
    }
    fclose(f);
}

void save_user(){
    FILE *f=fopen(CONFIG_FILE,"w");
    if(!f) return;
    fprintf(f,"user=%s\n",local_user);
    fclose(f);
}

/* ---------- DETAILS ---------- */
void draw_details(Process *p){
    erase();
    box(stdscr,0,0);
    mvprintw(1,2,"Process Details");
    mvhline(2,1,'-',COLS-2);

    mvprintw(4,4,"PID:     %d",p->pid);
    mvprintw(5,4,"PPID:    %d",p->ppid);
    mvprintw(6,4,"State:   %s",p->state);
    mvprintw(7,4,"CPU:     %.2f %%",p->cpu_percent);
    mvprintw(8,4,"Memory:  %ld KB",p->mem_kb);
    mvprintw(10,4,"Command:");
    mvprintw(11,6,"%s",p->cmdline);

    mvprintw(LINES-2,2,"Press q to return");
    refresh();
}

/* ---------- MAIN ---------- */
int main(){
    struct passwd *pw = getpwuid(getuid());
    if(pw) strcpy(real_user,pw->pw_name);

    Process list[MAX_PROC], tree[MAX_PROC], view[MAX_PROC];

    load_user();
    if(strlen(local_user)==0){
        printf("Enter display name: ");
        fgets(local_user,sizeof(local_user),stdin);
        local_user[strcspn(local_user,"\n")]=0;
        save_user();
    }

    initscr(); noecho(); cbreak(); keypad(stdscr,1); curs_set(0); timeout(200);
    start_color(); use_default_colors();

    int themes[3][6] = {
        {COLOR_CYAN,COLOR_GREEN,COLOR_YELLOW,COLOR_RED,COLOR_MAGENTA,COLOR_BLACK},
        {COLOR_WHITE,COLOR_CYAN,COLOR_GREEN,COLOR_YELLOW,COLOR_RED,COLOR_BLUE},
        {COLOR_MAGENTA,COLOR_CYAN,COLOR_WHITE,COLOR_GREEN,COLOR_YELLOW,COLOR_RED}
    };

    for(int i=0;i<32768;i++) collapsed[i]=0;

    while(1){
        for(int i=1;i<=6;i++) init_pair(i,themes[theme][i-1],-1);

        int count=load_processes(list);
        update_cpu(list,count);
        qsort(list,count,sizeof(Process),cmp_cpu);

        Process *show=list;
        int sc=count;
        if(tree_mode){ sc=build_tree(list,count,tree); show=tree; }

        int vcount=0;
        for(int i=0;i<sc;i++)
            if(process_match(&show[i],search_filter))
                view[vcount++]=show[i];

        if(show_details && vcount>0){
            draw_details(&view[selected]);
            int c=getch();
            if(c=='q') show_details=0;
            continue;
        }

        draw_ui(view+scroll_offset,vcount-scroll_offset);

        int ch=getch();

        if(ch==KEY_F(2)) theme=(theme+1)%3;

        else if(ch=='\n' && vcount>0) show_details=1;

        else if(ch==' ' && tree_mode && vcount>0){
            int pid=view[selected].pid;
            collapsed[pid]=!collapsed[pid];
        }

        else if(ch=='t'){ tree_mode=!tree_mode; scroll_offset=0; }

        else if(ch==KEY_DOWN) selected++;
        else if(ch==KEY_UP) selected--;
        else if(ch=='q') break;

        else if(vcount>0){
            if(ch=='k') kill(view[selected].pid,SIGTERM);
            else if(ch=='K') kill(view[selected].pid,SIGKILL);
            else if(ch=='p') kill(view[selected].pid,SIGSTOP);
            else if(ch=='r') kill(view[selected].pid,SIGCONT);
        }
    }

    endwin();
    return 0;
}
