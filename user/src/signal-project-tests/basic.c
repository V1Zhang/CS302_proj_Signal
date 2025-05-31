#include "../../os/ktest/ktest.h"
#include "../lib/user.h"

// Base Checkpoint 1: sigaction, sigkill, and sigreturn

// send SIGUSR0 to a child process, which default action is to terminate it.
void basic1(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sleep(10);
        exit(1);
    } else {
        // parent
        sigkill(pid, SIGUSR0, 0);
        int ret;
        wait(0, &ret);
        assert(ret == -10 - SIGUSR0);
    }
}

// send SIGUSR0 to a child process, but should be ignored.
void basic2(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = SIG_IGN,
            .sa_mask      = 0,
            .sa_restorer  = NULL,
        };
        sigaction(SIGUSR0, &sa, 0);
        sleep(10);
        sleep(10);
        sleep(10);
        exit(1);
    } else {
        // parent
        sleep(5);
        sigkill(pid, SIGUSR0, 0);
        int ret;
        wait(0, &ret);
        assert(ret == 1);
    }
}

void handler3(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR0);
    getpid();
    sleep(1);
    exit(103);
}

// set handler for SIGUSR0, which call exits to terminate the process.
//  this handler will not return, so sigreturn should not be called.
void basic3(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        //可以自定义传给child的信号的action
        sigaction_t sa = {
            .sa_sigaction = handler3,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR0, &sa, 0);
        while (1);
        exit(1);
    } else {
        // parent
        sleep(10);
        sigkill(pid, SIGUSR0, 0);
        int ret;
        wait(0, &ret);
        assert_eq(ret, 103);
    }
}

volatile int handler4_flag = 0;
void handler4(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR0);
    sleep(1);
    sleep(1);
    fprintf(1, "handler4 triggered\n");
    handler4_flag = 1;
}

// set handler for SIGUSR0, and return from handler.
void basic4(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = handler4,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR0, &sa, 0);
        while (handler4_flag == 0);
        exit(104);
    } else {
        // parent
        sleep(10);
        sigkill(pid, SIGUSR0, 0);
        int ret;
        wait(0, &ret);
        assert_eq(ret, 104);
    }
}

static volatile int handler5_cnt = 0;
void handler5(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR0);
    static volatile int nonreentrace = 0;
    assert(!nonreentrace);    // non-reentrance
    nonreentrace = 1;
    sleep(5);
    sleep(5);
    if (handler5_cnt < 5){
        fprintf(1, "handler5: sending SIGUSR0 to self\n");
        sigkill(getpid(), SIGUSR0, 0);
    }
    sleep(5);
    sleep(5);
    fprintf(1, "handler5 triggered\n");
    nonreentrace = 0;
    handler5_cnt++;
}

// signal handler itself should not be reentrant.
//  when the signal handler is called for SIGUSR0, it should block all SIGUSR0.
//  after the signal handler returns, the signal should be unblocked.
//   then, the signal handler should be called again. (5 times)
// set handler for SIGUSR0, kernel should block it from re-entrance.
//测试信号处理程序的递归或重复调用
void basic5(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = handler5,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR0, &sa, 0);
        while (handler5_cnt < 5);
        exit(105);
    } else {
        // parent
        sleep(10);
        sigkill(pid, SIGUSR0, 0);
        int ret;
        wait(0, &ret);
        assert_eq(ret, 105);
    }
}

volatile int handler6_flag = 0;
void handler6(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR0);
    handler6_flag = 1;
    fprintf(1, "handler6 triggered due to %d\n", signo);
    sleep(30);
    assert(handler6_flag == 2);
    handler6_flag = 3;
}

void handler6_2(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR1);
    assert(handler6_flag == 1);
    handler6_flag = 2;
    fprintf(1, "handler6_2 triggered due to %d\n", signo);
}

// signal handler can be nested.
//测试嵌套调用功能，即在一个信号处理时，能否接受并处理另一个不同类型的信号
void basic6(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = handler6,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR0, &sa, 0);
        sigaction_t sa2 = {
            .sa_sigaction = handler6_2,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa2.sa_mask);
        sigaction(SIGUSR1, &sa2, 0);
        while (handler6_flag != 3);
        exit(106);
    } else {
        // parent
        sleep(10);
        sigkill(pid, SIGUSR0, 0);
        sleep(5);
        sigkill(pid, SIGUSR1, 0);
        sleep(5);
        int ret;
        wait(0, &ret);
        assert_eq(ret, 106);
    }
}

volatile int handler7_flag = 0;
void handler7(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR0);
    handler7_flag = 1;
    fprintf(1, "handler7 triggered due to %d\n", signo);
    sleep(30);
    sigset_t pending;
    sigpending(&pending);
    assert_eq(pending, sigmask(SIGUSR1));
    assert(handler7_flag == 1); // handler7 should not interrupted by SIGUSR1 (handler7_2)
    handler7_flag = 2;
}

void handler7_2(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR1);
    printf("handler7_flag = %d\n", handler7_flag);
    assert(handler7_flag == 2);
    handler7_flag = 3;
    fprintf(1, "handler7_2 triggered due to %d\n", signo);
}

// signal handler can be nested.
//测试信号屏蔽和信号排队功能，关注在处理一个信号的时候如何显示屏蔽特定的信号。
void basic7(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = handler7,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGUSR1); // block SIGUSR1 when handling SIGUSR0
        sigaction(SIGUSR0, &sa, 0);

        sigaction_t sa2 = {
            .sa_sigaction = handler7_2,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa2.sa_mask);
        sigaction(SIGUSR1, &sa2, 0);

        while (handler7_flag != 3);
        exit(107);
    } else {
        // parent
        sleep(10);
        sigkill(pid, SIGUSR0, 0);
        sleep(5);
        sigkill(pid, SIGUSR1, 0);
        sleep(5);
        int ret;
        wait(0, &ret);
        assert_eq(ret, 107);
    }
}

// SIG_IGN and SIG_DFL
void basic8(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = SIG_IGN,
            .sa_restorer  = NULL,
        };
        sigaction(SIGUSR0, &sa, 0);
        sigkill(getpid(), SIGUSR0, 0); // should have no effect

        sigaction_t sa2 = {
            .sa_sigaction = SIG_DFL,
            .sa_restorer  = NULL,
        };
        sigaction(SIGUSR1, &sa2, 0);
        sigkill(getpid(), SIGUSR1, 0); // should terminate the process

        exit(1);
    } else {
        // parent
        // Removed:
        // sigkill(pid, SIGUSR0, 0);
        int ret;
        wait(0, &ret);
        assert(ret == -10 - SIGUSR1); // child terminated by SIGUSR1
    }
}


// Base Checkpoint 2: SIGKILL

void handler10(int signo, siginfo_t* info, void* ctx2) {
    exit(2);
}

// child process is killed by signal: SIGKILL, which cannot be handled, ignored and blocked.
void basic10(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = handler10,
            .sa_restorer  = NULL,
        };
        sigaction(SIGKILL, &sa, 0); 
        // set handler for SIGKILL, which should not be called
        while (1);
        exit(1);
    } else {
        // parent
        sleep(20);
        sigkill(pid, SIGKILL, 0);
        int ret;
        wait(0, &ret);
        assert(ret == -10 - SIGKILL);
    }
}

// child process is killed by signal: SIGKILL, which cannot be handled, ignored and blocked.
void basic11(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGKILL);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        // set handler for SIGKILL, which should not be called
        while (1);
        exit(1);
    } else {
        // parent
        sleep(20);
        sigkill(pid, SIGKILL, 0);
        int ret;
        wait(0, &ret);
        assert(ret == -10 - SIGKILL);
    }
}

// Base Checkpoint 3: signals under fork & exec

void basic20(char *s) {
    // our modification does not affect our parent process.
    // because `run` method in the testsuite will do fork for us.

    sigaction_t sa = {
        .sa_sigaction = SIG_IGN,
        .sa_restorer  = NULL,
    };
    sigaction(SIGUSR0, &sa, 0);
    // ignore SIGUSR0.

    int pid = fork();
    if (pid == 0) {
        // child
        sigkill(getpid(), SIGUSR0, 0); 
        // should have no effect, because parent ignores it.
        exit(1);
    } else {
        // parent
        int ret;
        wait(0, &ret);
        assert(ret == 1); // child should not be terminated by SIGUSR0
    }
}

volatile int alarm_flag = 0;

void handler31(int signo, siginfo_t* info, void* ctx) {
    assert(signo == SIGALRM);
    alarm_flag = 1;
    fprintf(1, "SIGALRM received at alarm_flag = %d\n", alarm_flag);
    sigreturn();
}

void basic31(char* s) {
    int pid = fork();
    if (pid == 0) {
        // child
        sigaction_t sa = {
            .sa_sigaction = handler31,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM, &sa, 0);

        // 第一次设置：3秒后触发
        unsigned int remain = alarm(3);
        assert_eq(remain, 0); // 应该没有之前的 alarm

        // 等待4秒，alarm 应该触发
        for (int i = 0; i < 4; i++) sleep(100);
        assert_eq(alarm_flag, 1);

        // 第二次设置：5秒后触发
        alarm_flag = 0;
        remain = alarm(5);
        assert_eq(remain, 0); // 上一个 alarm 已完成，返回0

        // 等待2秒后取消
        sleep(200);
        remain = alarm(0);
        assert(remain >= 2 && remain <= 5); // 应该有剩余2~5秒

        // 再等3秒，SIGALRM 不应触发
        sleep(300);
        assert_eq(alarm_flag, 0); // 未触发信号

        exit(202);
    } else {
        // parent
        int ret;
        wait(0, &ret);
        assert_eq(ret, 202);
    }
}


volatile int stop_cont_flag = 0;

void handler_cont(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGCONT);
    fprintf(1, "[PID %d] Waiting: stop_cont_flag = %d (expecting 1)\n", getpid(), stop_cont_flag);
    assert(stop_cont_flag == 1);
    fprintf(1, "SIGCONT handler triggered\n");
    stop_cont_flag = -1;
    
}
void handler34(int signo, siginfo_t* info, void* ctx2) {
    assert(signo == SIGUSR0);
    fprintf(1, "[PID %d] Waiting: stop_cont_flag = %d \n", getpid(), stop_cont_flag);
    fprintf(1, "handler34 triggered due to %d boooooom!!\n", signo);
    stop_cont_flag = stop_cont_flag+1;
}

// Test SIGSTOP and SIGCONT functionality
// Tests that SIGSTOP pauses process execution and SIGCONT resumes it
void basic34(char* s) {
    int pid = fork();
    if (pid == 0) {
        // Child process
        // Register handler for SIGCONT
        sigaction_t sa = {
            .sa_sigaction = handler_cont,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGCONT, &sa, 0);

        sigaction_t sa1 = {
            .sa_sigaction = handler34,
            .sa_restorer  = sigreturn,
        };
        sigemptyset(&sa1.sa_mask);
        sigaction(SIGUSR0, &sa1, 0);

        // Signal parent that we're ready
        stop_cont_flag = 1;

        // Loop until handler_cont sets flag to 2
        fprintf(1, "Child process running, waiting for signals\n");
        
        while (stop_cont_flag != -1) {
            // Keep printing to show process is running
            if (stop_cont_flag == 1) {
                fprintf(1, "Child process start running\n");
                sleep(10);
            } 
        }
        
        exit(200);
    } else {
        // Parent process
        fprintf(1, "Parent waiting for child to initialize\n");
        
        // Wait for child to set flag
        sleep(50);
        
        fprintf(1, "Parent sending SIGSTOP to child\n");
        // Send SIGSTOP to child process
        sigkill(pid, SIGSTOP, 0);
        sleep(5);
        // Check if process is actually stopped
        // We can verify this by trying to send a regular signal and seeing if it's processed
        fprintf(1, "Sending test signal to stopped process\n");
        sigkill(pid, SIGUSR0, 0);
        sleep(5);
        
        // If child was truly stopped, stop_cont_flag should still be 1
        
        fprintf(1, "Parent sending SIGCONT to child\n");
        // Resume the child process
        sigkill(pid, SIGCONT, 0);
        
        // Wait for child to complete
        int ret;
        wait(0, &ret);
        assert_eq(ret, 200);
        
        fprintf(1, "SIGSTOP/SIGCONT test passed\n");
    }
}