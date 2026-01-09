#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "procmon.h"

#define CONFIG_FILE "/data/data/com.termux/files/home/.procmonrc"

/* ---------- GLOBAL STATE ---------- */

int selected = 0;
int tree_mode = 0;
int search_mode = 0;
int scroll_offset = 0;

char search_filter[64] = "";
char local_user[32] = "";

int depth[MAX_PROC];

/* ---------- SORT ---------- */

int cmp_cpu(const void *a, const void *b){
    double d = ((Process*)b)->cpu_percent - ((Process*)a)->cpu_percent;
    return (d > 0) - (d < 0);
}

/* ---------- TREE ENGINE ---------- */

static int used[MAX_PROC];

void dfs(Process *src, int count, Process *out, int *outc, int pid, int d){
    for(int i=0;i<count;i++){
        if(!used[i] && src[i].ppid == pid){
            used[i] = 1;
            out[*outc] = src[i];
            depth[*outc] = d;
            (*outc)++;
            dfs(src, count, out, outc, src[i].pid, d + 1);
        }
    }
}

int build_tree(Process *src, int count, Process *out){
    for(int i=0;i<count;i++) used[i] = 0;

    int outc = 0;

    for(int i=0;i<count;i++){
        int parent_found = 0;
        for(int j=0;j<count;j++)
            if(src[i].ppid == src[j].pid)
                parent_found = 1;

        if(!parent_found){
            used[i] = 1;
            out[outc] = src[i];
            depth[outc] = 0;
            outc++;
            dfs(src, count, out, &outc, src[i].pid, 1);
        }
    }
    return outc;
}

/* ---------- CONFIG ---------- */

void load_user(){
    FILE *f = fopen(CONFIG_FILE, "r");
    if(!f) return;

    char line[128];
    if(fgets(line, sizeof(line), f)){
        if(strncmp(line, "user=", 5) == 0){
            strcpy(local_user, line + 5);
            local_user[strcspn(local_user, "\n")] = 0;
        }
    }
    fclose(f);
}

void save_user(){
    FILE *f = fopen(CONFIG_FILE, "w");
    if(!f) return;
    fprintf(f, "user=%s\n", local_user);
    fclose(f);
}

/* ---------- MAIN ---------- */

int main(){
    Process list[MAX_PROC];
    Process tree[MAX_PROC];
    Process view[MAX_PROC];

    load_user();

    if(strlen(local_user) == 0){
        printf("Enter display name: ");
        fflush(stdout);

        if(fgets(local_user, sizeof(local_user), stdin)){
            local_user[strcspn(local_user, "\n")] = 0;
        }

        if(strlen(local_user) == 0)
            strcpy(local_user, "unknown");

        save_user();
    }

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(200);

    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN,   -1);
    init_pair(2, COLOR_GREEN,  -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED,    -1);
    init_pair(5, COLOR_MAGENTA,-1);
    init_pair(6, COLOR_BLACK,  COLOR_WHITE);

    while(1){
        int count = load_processes(list);
        update_cpu(list, count);
        qsort(list, count, sizeof(Process), cmp_cpu);

        Process *show_list = list;
        int show_count = count;

        if(tree_mode){
            show_count = build_tree(list, count, tree);
            show_list = tree;
        }

        int vcount = 0;
        for(int i=0;i<show_count;i++){
            if(process_match(&show_list[i], search_filter)){
                view[vcount++] = show_list[i];
            }
        }

        int view_height = LINES - 4;

        if(selected >= vcount) selected = vcount - 1;
        if(selected < 0) selected = 0;

        if(selected < scroll_offset) scroll_offset = selected;
        if(selected >= scroll_offset + view_height)
            scroll_offset = selected - view_height + 1;

        if(scroll_offset < 0) scroll_offset = 0;
        if(scroll_offset > vcount - view_height)
            scroll_offset = vcount - view_height;
        if(scroll_offset < 0) scroll_offset = 0;

        draw_ui(view + scroll_offset, vcount - scroll_offset);

        int ch = getch();

        if(search_mode){
            if(ch == 27){
                search_mode = 0;
            }
            else if(ch == KEY_BACKSPACE || ch == 127){
                int l = strlen(search_filter);
                if(l > 0) search_filter[l-1] = 0;
            }
            else if(ch >= 32 && ch <= 126 && strlen(search_filter) < 63){
                int l = strlen(search_filter);
                search_filter[l] = ch;
                search_filter[l+1] = 0;
            }
        }
        else{
            if(ch == 'q') break;
            else if(ch == KEY_DOWN) selected++;
            else if(ch == KEY_UP)   selected--;
            else if(ch == KEY_NPAGE) selected += view_height;
            else if(ch == KEY_PPAGE) selected -= view_height;
            else if(ch == 'k' && vcount > 0) kill(view[selected].pid, SIGTERM);
            else if(ch == 't'){ tree_mode = !tree_mode; selected = 0; scroll_offset = 0; }
            else if(ch == '/') search_mode = 1;
            else if(ch == 27) search_filter[0] = 0;
        }
    }

    endwin();
    return 0;
}
