// This file is shared by Kernel and User-space application.

// System call numbers
#define SYS_fork    1
#define SYS_exec    2
#define SYS_exit    3
#define SYS_wait    4
#define SYS_getpid  5
#define SYS_getppid 6
#define SYS_kill    7

#define SYS_sleep 10
#define SYS_yield 11

#define SYS_sbrk 20
#define SYS_mmap 21

#define SYS_read  22
#define SYS_write 23

#define SYS_gettimeofday 24
#define SYS_ktest 99

#define SYS_sigaction 30
#define SYS_sigreturn 31
#define SYS_sigprocmask 32
#define SYS_sigkill 33
#define SYS_sigpending 34
#define SYS_alarm 35
