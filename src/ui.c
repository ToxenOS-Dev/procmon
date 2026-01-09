#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "procmon.h"

extern int selected;
extern int depth[];
extern char local_user[];

#define C_HEADER   1
#define C_RUN      2
#define C_SLEEP    3
#define C_ZOMBIE   4
#define C_HOT      5
#define C_SELECT   6

static int state_color(const char *s,double cpu){
    if(cpu>20) return C_HOT;
    if(s[0]=='R') return C_RUN;
    if(s[0]=='S') return C_SLEEP;
    if(s[0]=='Z') return C_ZOMBIE;
    return C_SLEEP;
}

void draw_ui(Process *list,int count){
    erase();

    attron(COLOR_PAIR(C_HEADER)|A_BOLD);
    mvprintw(0,2,
    "PROC MON | ↑↓ move | SPACE collapse | ENTER details | F2 theme | t tree | k/K kill p stop r run | q quit");
    attroff(COLOR_PAIR(C_HEADER)|A_BOLD);

    mvprintw(1,0," PID     USER     CPU%%    MEM(KB)   STATE        COMMAND");
    mvhline(2,0,'-',COLS);

    int max=LINES-4;

    for(int i=0;i<count && i<max;i++){
        int col=state_color(list[i].state,list[i].cpu_percent);

        if(i==selected) attron(COLOR_PAIR(C_SELECT)|A_BOLD);
        else attron(COLOR_PAIR(col));

        char indent[64]="";
        for(int d=0;d<depth[i];d++) strcat(indent,"  ");

        mvprintw(3+i,0," %-6d %-8s %5.1f   %-8ld %-12s %s%s",
            list[i].pid,
            list[i].user,
            list[i].cpu_percent,
            list[i].mem_kb,
            list[i].state,
            indent,
            list[i].cmdline);

        if(i==selected) attroff(COLOR_PAIR(C_SELECT)|A_BOLD);
        else attroff(COLOR_PAIR(col));
    }

    refresh();
}
