#include "ksignal.h"

#include <defs.h>
#include <proc.h>
#include <trap.h>

/**
 * @brief init the signal struct inside a PCB.
 * 
 * @param p 
 * @return int 
 */

 extern struct proc *proc[]; 

int siginit(struct proc *p) {
    // init p->signal
    for (int i = 1; i < _NSIG; i++) {
        p->signal.sa[i].sa_sigaction = SIG_DFL;
        p->signal.sa[i].sa_mask = 0;
    }

    // Set the signal mask to empty (no blocked signals)
    p->signal.sigmask = 0;

    // Initialize the pending signal set to empty
    p->signal.sigpending = 0;

    p->stopped = 0;  

     return 0;
}

int siginit_fork(struct proc *parent, struct proc *child) {
    // copy parent's sigactions and signal mask
    // but clear all pending signals
    for (int i = 1; i < _NSIG; i++) {
        child->signal.sa[i] = parent->signal.sa[i];
    }

    child->signal.sigmask = parent->signal.sigmask;
    child->signal.sigpending = 0;  // Clear pending signals for the child
    child->stopped = 0;
     return 0;
}

int siginit_exec(struct proc *p) {
    // inherit signal mask and pending signals.
    // but reset all sigactions (except ignored) to default.

    sigset_t oldmask = p->signal.sigmask;
    
    for (int i = 1; i < _NSIG; i++) {
        if (p->signal.sa[i].sa_sigaction != SIG_IGN) {
            p->signal.sa[i].sa_sigaction = SIG_DFL;
            p->signal.sa[i].sa_mask = 0;
        }
    }
    p->signal.sigpending = 0;

     return 0;
}

static void save_context(struct proc *p, struct ucontext *uc) {
    struct trapframe *tf = p->trapframe;
    
    // Save general purpose registers
    uc->uc_mcontext.regs[0] = tf->ra;   // ra
    uc->uc_mcontext.regs[1] = tf->sp;   // sp
    uc->uc_mcontext.regs[2] = tf->gp;   // gp
    uc->uc_mcontext.regs[3] = tf->tp;   // tp
    uc->uc_mcontext.regs[4] = tf->t0;   // t0
    uc->uc_mcontext.regs[5] = tf->t1;   // t1
    uc->uc_mcontext.regs[6] = tf->t2;   // t2
    uc->uc_mcontext.regs[7] = tf->s0;   // s0
    uc->uc_mcontext.regs[8] = tf->s1;   // s1
    uc->uc_mcontext.regs[9] = tf->a0;   // a0
    uc->uc_mcontext.regs[10] = tf->a1;  // a1
    uc->uc_mcontext.regs[11] = tf->a2;  // a2
    uc->uc_mcontext.regs[12] = tf->a3;  // a3
    uc->uc_mcontext.regs[13] = tf->a4;  // a4
    uc->uc_mcontext.regs[14] = tf->a5;  // a5
    uc->uc_mcontext.regs[15] = tf->a6;  // a6
    uc->uc_mcontext.regs[16] = tf->a7;  // a7
    uc->uc_mcontext.regs[17] = tf->s2;  // s2
    uc->uc_mcontext.regs[18] = tf->s3;  // s3
    uc->uc_mcontext.regs[19] = tf->s4;  // s4
    uc->uc_mcontext.regs[20] = tf->s5;  // s5
    uc->uc_mcontext.regs[21] = tf->s6;  // s6
    uc->uc_mcontext.regs[22] = tf->s7;  // s7
    uc->uc_mcontext.regs[23] = tf->s8;  // s8
    uc->uc_mcontext.regs[24] = tf->s9;  // s9
    uc->uc_mcontext.regs[25] = tf->s10; // s10
    uc->uc_mcontext.regs[26] = tf->s11; // s11
    uc->uc_mcontext.regs[27] = tf->t3;  // t3
    uc->uc_mcontext.regs[28] = tf->t4;  // t4
    uc->uc_mcontext.regs[29] = tf->t5;  // t5
    uc->uc_mcontext.regs[30] = tf->t6;  // t6
    
    // Save program counter
    uc->uc_mcontext.epc = tf->epc;
    // Save signal mask
    uc->uc_sigmask = p->signal.sigmask;
}

static void restore_context(struct proc *p, struct ucontext *uc) {
    struct trapframe *tf = p->trapframe;
    
    // Restore general purpose registers
    tf->ra = uc->uc_mcontext.regs[0];   // ra
    tf->sp = uc->uc_mcontext.regs[1];   // sp
    tf->gp = uc->uc_mcontext.regs[2];   // gp
    tf->tp = uc->uc_mcontext.regs[3];   // tp
    tf->t0 = uc->uc_mcontext.regs[4];   // t0
    tf->t1 = uc->uc_mcontext.regs[5];   // t1
    tf->t2 = uc->uc_mcontext.regs[6];   // t2
    tf->s0 = uc->uc_mcontext.regs[7];   // s0
    tf->s1 = uc->uc_mcontext.regs[8];   // s1
    tf->a0 = uc->uc_mcontext.regs[9];   // a0
    tf->a1 = uc->uc_mcontext.regs[10];  // a1
    tf->a2 = uc->uc_mcontext.regs[11];  // a2
    tf->a3 = uc->uc_mcontext.regs[12];  // a3
    tf->a4 = uc->uc_mcontext.regs[13];  // a4
    tf->a5 = uc->uc_mcontext.regs[14];  // a5
    tf->a6 = uc->uc_mcontext.regs[15];  // a6
    tf->a7 = uc->uc_mcontext.regs[16];  // a7
    tf->s2 = uc->uc_mcontext.regs[17];  // s2
    tf->s3 = uc->uc_mcontext.regs[18];  // s3
    tf->s4 = uc->uc_mcontext.regs[19];  // s4
    tf->s5 = uc->uc_mcontext.regs[20];  // s5
    tf->s6 = uc->uc_mcontext.regs[21];  // s6
    tf->s7 = uc->uc_mcontext.regs[22];  // s7
    tf->s8 = uc->uc_mcontext.regs[23];  // s8
    tf->s9 = uc->uc_mcontext.regs[24];  // s9
    tf->s10 = uc->uc_mcontext.regs[25]; // s10
    tf->s11 = uc->uc_mcontext.regs[26]; // s11
    tf->t3 = uc->uc_mcontext.regs[27];  // t3
    tf->t4 = uc->uc_mcontext.regs[28];  // t4
    tf->t5 = uc->uc_mcontext.regs[29];  // t5
    tf->t6 = uc->uc_mcontext.regs[30];  // t6

    // Restore program counter
    tf->epc = uc->uc_mcontext.epc;
    // Restore signal mask
    p->signal.sigmask = uc->uc_sigmask;
}

int do_signal(void) {
    struct proc *p = curr_proc();
    sigset_t initial_mask = p->signal.sigmask;
    if (!p->signal.sigpending)
        return 0;

    // 处理 SIGCONT 信号
    if (p->signal.sigpending & (1ULL << SIGCONT)) {
        // 如果进程当前被停止，则恢复它
        if (p->stopped) {
            p->stopped = 0;
        }     
    }
    //若进程stopped，则直接返回
    if (p->stopped || !p->signal.sigpending) {
        return 0;
    }
    
    // Check each signal
    for (int i = 1; i < _NSIG; i++) {
        if ((p->signal.sigpending & (1ULL << i)) && !(p->signal.sigmask & (1ULL << i))) {
            // SIGKILL cannot be blocked or ignored
            if (i == SIGKILL) {
                setkilled(p, -10 - SIGKILL);  // Set exit code to -10 - SIGKILL
                return -1;
            }

            if (i == SIGSTOP) {
            // 设置进程为停止状态
            p->stopped = 1;
            
            // 清除挂起的 SIGSTOP
            p->signal.sigpending &= ~(1ULL << SIGSTOP);
            // 返回 0，让进程停止但不终止
            return 0;
         }

            // Check if signal is blocked
            if (!(p->signal.sigmask & (1ULL << i))) {
                // Handle the signal
                if (p->signal.sa[i].sa_sigaction == SIG_DFL) {
                    // Default handling
                    // if (i == SIGSTOP) {
                    //     // SIGSTOP cannot be caught or ignored
                    //     p->killed = 1;
                    //     return -1;
                    // }
                    // For other signals with SIG_DFL, terminate the process
                    setkilled(p, -10 - i);  // Set exit code to -10 - signal_number
                    return -1;
                } else if (p->signal.sa[i].sa_sigaction == SIG_IGN) {
                    // Ignore the signal
                    p->signal.sigpending &= ~(1ULL << i);
                } else {
                    //如果信号有用户定义的处理程序，设置上下文并跳转到处理程序
                    // User-defined handler
                    // Calculate new stack pointer
                    p->signal.sigpending &= ~(1ULL << i);
                    uint64 new_sp = (p->trapframe->sp - (sizeof(struct ucontext) + sizeof(siginfo_t))) & ~0xF;
                    if (!IS_USER_VA(new_sp) || !IS_USER_VA(new_sp + sizeof(struct ucontext) + sizeof(siginfo_t))) {
                        return -1;
                    }                    
                    // Acquire memory lock before accessing user memory
                    acquire(&p->mm->lock);
                    
                    struct ucontext kuc;
                    kuc.uc_sigmask = initial_mask;  // 使用保存的初始掩码

                    // Save current context
                    save_context(p, &kuc);

                    // Copy to user-space
                    if (copy_to_user(p->mm, (uint64)(new_sp + sizeof(siginfo_t)), (char *)&kuc, sizeof(struct ucontext)) < 0) {
                        release(&p->mm->lock);
                        return -1;
                    }

                    // Create siginfo_t
                    siginfo_t tmp_si = {0};
                    tmp_si.si_signo = i;
                    if (copy_to_user(p->mm, new_sp, (char *)&tmp_si, sizeof(siginfo_t)) < 0) {
                        release(&p->mm->lock);
                        return -1;
                    }

                    // Set up trapframe for signal handler
                    p->trapframe->a0 = i;                    // First argument: signo
                    p->trapframe->a1 = new_sp;         // Second argument: siginfo_t pointer
                    p->trapframe->a2 = new_sp + sizeof(siginfo_t);           // Third argument: ucontext pointer
                    p->trapframe->ra = (uint64)p->signal.sa[i].sa_restorer; // Return to sigreturn
                    p->trapframe->sp = new_sp;               // New stack pointer
                    p->trapframe->epc = (uint64)p->signal.sa[i].sa_sigaction; // Jump to signal handler
                    
                    // Update signal mask - block the current signal and any signals specified in sa_mask
                    p->signal.sigmask |= (1ULL << (i)) | p->signal.sa[i].sa_mask;;
                    // Clear pending signal
                    //  p->signal.sigpending &= ~(1ULL << i);
                    
                    // Release memory lock after we're done with user memory
                    release(&p->mm->lock);
                }
            }
        }
    }
     return 0;
}

// syscall handlers:
//  sys_* functions are called by syscall.c
//  sys_sigaction 用于设置或获取指定信号的处理方式，注册信号处理程序的功能
int sys_sigaction(int signo, const sigaction_t __user *act, sigaction_t __user *oldact) {
    if (signo <= 0 || signo >= _NSIG) {
        return -1;  // Invalid signal number
    }

    // SIGKILL and SIGSTOP cannot be caught or ignored
    if (signo == SIGKILL || signo == SIGSTOP) {
        return -1;
    }

    struct proc *p = curr_proc();
    
    if (oldact) {
        // Copy the old action to user-space
        acquire(&p->mm->lock);
        int ok = copy_to_user(p->mm, (uint64)oldact, (char *)&p->signal.sa[signo], sizeof(sigaction_t));
        release(&p->mm->lock);
        if (ok < 0) return -1;
    }

    if (act) {
        // Copy the new action from user-space
        sigaction_t tmp;
        acquire(&p->mm->lock);
        int ok = copy_from_user(p->mm, (char *)&tmp, (uint64)act, sizeof(sigaction_t));
        release(&p->mm->lock);
        if (ok < 0) return -1;

        // 检查地址是否合法
        if (!IS_USER_VA((uint64)tmp.sa_sigaction)) {
            return -1;
        }

        p->signal.sa[signo] = tmp;
    }

     return 0;
}

int sys_sigreturn() {
    struct proc *p = curr_proc();
    //计算上下文在用户栈中的位置
    struct ucontext *uc = (struct ucontext *)(p->trapframe->sp + sizeof(siginfo_t)); 
    // Restore context，将trapframe的内容保存到到kuc中
    struct ucontext kuc;
    // Acquire memory lock before accessing user memory
    acquire(&p->mm->lock);
    if (copy_from_user(p->mm, (char *)&kuc, (uint64)(p->trapframe->sp + sizeof(siginfo_t)), sizeof(struct ucontext)) < 0) {
        release(&p->mm->lock);
        return -1;
    }
    restore_context(p, &kuc);
    if (!IS_USER_VA(p->trapframe->epc)) {
        release(&p->mm->lock);
        return -1;
    }
    
    p->signal.sigmask = kuc.uc_sigmask;

    
    // Release memory lock after we're done with user memory
    release(&p->mm->lock);

     return 0;
}

int sys_sigprocmask(int how, const sigset_t __user *set, sigset_t __user *oldset) {
    struct proc *p = curr_proc();
    printf("sigprocmask: Current mask: %x\n", p->signal.sigmask);
    if (oldset) {
        // Copy the old mask to user-space
        acquire(&p->mm->lock);
        int ok = copy_to_user(p->mm, (uint64)oldset, (char *)&p->signal.sigmask, sizeof(sigset_t));
        release(&p->mm->lock);
        if (ok < 0) return -1;
    }

    if (set) {
        sigset_t new_mask;
        acquire(&p->mm->lock);
        int ok = copy_from_user(p->mm, (char *)&new_mask, (uint64)set, sizeof(sigset_t));
        release(&p->mm->lock);
        if (ok < 0) return -1;

        // SIGKILL and SIGSTOP cannot be blocked
        new_mask &= ~((1ULL << SIGKILL) | (1ULL << SIGSTOP));
        printf("sigprocmask: Setting new mask: %x (how: %d)\n", new_mask, how);

        switch (how) {
            case SIG_BLOCK:
                p->signal.sigmask |= new_mask;  // Block the specified signals
                break;
            case SIG_UNBLOCK:
                p->signal.sigmask &= ~new_mask;  // Unblock the specified signals
                break;
            case SIG_SETMASK:
                p->signal.sigmask = new_mask;  // Set the signal mask to the specified mask
                break;
            default:
                return -1;  // Invalid action
        }
        printf("sigprocmask: Final mask: %x\n", p->signal.sigmask);
    }

     return 0;
}

int sys_sigpending(sigset_t __user *set) {
    struct proc *p = curr_proc();
    
    // Copy pending signals to user-space
    acquire(&p->mm->lock);
    int ok = copy_to_user(p->mm, (uint64)set, (char *)&p->signal.sigpending, sizeof(sigset_t));
    release(&p->mm->lock);
    if (ok < 0) return -1;
    
    return 0;

}

int sys_sigkill(int pid, int signo, int code) {
    if (signo <= 0 || signo >= _NSIG) {
        return -1;  // Invalid signal number
    }

    struct proc *target = NULL;
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = pool[i];
        if (p == NULL) continue;
        acquire(&p->lock);
        if (p->pid == pid) {
            target = p;
            break;
        }
        release(&p->lock);
    }

    if (!target) {
        return -1;  // Process not found
    }

     if (target->stopped) {
        //只能添加sigcont
        if (signo == SIGCONT) {
            target->signal.sigpending |= (1ULL << signo);
        } 
    } else {
        // 如果进程未停止，所有信号都正常添加到挂起集
        target->signal.sigpending |= (1ULL << signo);
    }
    release(&target->lock);

     return 0;
}