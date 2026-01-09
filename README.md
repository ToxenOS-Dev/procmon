# procmon

A terminal-based Linux process monitor written in C.

Features:
- Live process scanning via /proc
- CPU usage engine
- Full-screen ncurses UI
- Color-coded states
- Process tree mode
- Kill processes from UI
- Live search/filter
- Modular architecture
- Makefile build system

Controls:
- ↑ ↓  move selection
- t    toggle tree mode
- /    search
- ESC  clear search
- k    kill selected process
- q    quit

Build:
    make

Run:
    ./procmon
