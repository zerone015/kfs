#include "signal.h"
#include "proc.h"

static const uint8_t trampoline_code[] = {
    0xB8, 0x77, 0x00, 0x00, 0x00,   // mov eax, 119
    0xCD, 0x80                      // int 0x80
};

static void sigstop_handle(struct task_struct *target)
{
    signal_pending_clear(target, SIGCONT);
    signal_pending_set(target, SIGSTOP);
    wake_up(target);
}

static void sigcont_handle(struct task_struct *target)
{
    signal_pending_clear(target, SIGSTOP);
    if (target->state == PROCESS_STOPPED) {
        __wake_up(target);
        caught_signal_send(target->parent, SIGCHLD);
    }
    if (signal_is_caught(target, SIGCONT))
        signal_pending_set(target, SIGCONT);
}

static void sigkill_handle(struct task_struct *target)
{
    signal_pending_set(target, SIGKILL);
    if (!runnable_state(target))
        __wake_up(target);
}

static void do_stop(void)
{
    current->state = PROCESS_STOPPED;
    caught_signal_send(current->parent, SIGCHLD);
    yield();
}

static void do_default_signal(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGILL:
    case SIGABRT:
    case SIGFPE:
    case SIGKILL:        
    case SIGSEGV:
    case SIGTERM:
        do_exit(sig);
        break;
    case SIGSTOP:
        do_stop();
    }
}

static void prepare_signal_frame_setup(struct signal_frame *sf) 
{
    uint32_t *pte = pte_from_va(sf);
    uintptr_t addr = (uintptr_t)sf;

    if (is_rdwr_cow(*pte))
        cow_handle(pte, addr);

    uintptr_t end_addr = addr + sizeof(struct signal_frame);
    if (align_down(addr, PAGE_SIZE) != align_down(end_addr, PAGE_SIZE)) {
        if (is_rdwr_cow(*(pte + 1)))
            cow_handle(pte + 1, end_addr);
    }
}

static void signal_frame_setup(struct signal_frame *sf, struct ucontext *ucontext, int sig)
{
    sf->eax = ucontext->eax;
    sf->ecx = ucontext->ecx;
    sf->edx = ucontext->edx;
    sf->ebx = ucontext->ebx;
    sf->esi = ucontext->esi;
    sf->edi = ucontext->edi;
    sf->esp = ucontext->esp;
    sf->ebp = ucontext->ebp;
    sf->eip = ucontext->eip;
    sf->eflags = ucontext->eflags;
    memcpy(sf->trampoline, trampoline_code, sizeof(trampoline_code));
    sf->arg = sig;
    sf->ret = (uint32_t)sf->trampoline;
}

static void do_caught_signal(int sig, sighandler_t sighandler, struct ucontext *ucontext)
{
    struct signal_frame *sf; 
    
    sf = (struct signal_frame *)(ucontext->esp - sizeof(struct signal_frame));
    
    prepare_signal_frame_setup(sf);
    signal_frame_setup(sf, ucontext, sig);

    ucontext->esp = (uint32_t)sf;
    ucontext->eip = (uint32_t)sighandler;
    
    current_signal_set(current, sig);
    signal_pending_clear(current, sig);
}

void do_signal(uint32_t sig_pending, struct ucontext *ucontext)
{
    int sig = pick_signal(sig_pending);
    sighandler_t sighandler = sighandler_lookup(sig);
    if (sighandler == SIG_DFL)
        do_default_signal(sig);
    else
        do_caught_signal(sig, sighandler, ucontext);
}

void signal_send(struct task_struct *target, int sig)
{
    switch (sig) {
    case SIGSTOP:
        sigstop_handle(target);
        break;
    case SIGCONT:
        sigcont_handle(target);
        break;
    case SIGKILL:
        sigkill_handle(target);
        break;
    default:
        if (signal_sendable(target, sig))
            __signal_send(target, sig);
    }
}

int unmasked_signal_pending(void)
{
    if (current_signal_empty(current))
        return current->sig_pending;
    return current->sig_pending & ~(1 << current->current_signal);
}