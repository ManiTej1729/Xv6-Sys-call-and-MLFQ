#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}

uint64
sys_getSysCount(void)
{
  int mask;
  argint(0, &mask);

  // macro to syscall names mapping
  char *syscall_names[] = {
    "sys_fork",
    "sys_exit",
    "sys_wait",
    "sys_pipe",
    "sys_read",
    "sys_kill",
    "sys_exec",
    "sys_fstat",
    "sys_chdir",
    "sys_dup",
    "sys_getpid",
    "sys_sbrk",
    "sys_sleep",
    "sys_uptime",
    "sys_open",
    "sys_write",
    "sys_mknod",
    "sys_unlink",
    "sys_link",
    "sys_mkdir",
    "sys_close",
    "sys_waitx",
    "sys_updateMask",
    "sys_getSysCount",
    "sys_sigalarm",
    "sys_sigreturn",
    "sys_settickets"
  };

  struct proc *p = myproc();
  printf("Process with ID %d used the system call \"%s\" %d time(s)\n", p->pid, syscall_names[mask - 1] + 4, p->count);
  return 0;
}

uint64
sys_updateMask(void)
{
  int mask;
  argint(0, &mask);

  struct proc *p = myproc();
  p->mask = mask;
  // printf("print mask %d\n", p->mask);
  return 0;
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 fn_ptr;

  argint(0, &interval);
  argaddr(1, &fn_ptr);

  struct proc *p = myproc();
  p->interval_duration = interval;
  p->timeleft = interval;
  p->inside = 0;
  p->handler = (void (*)())fn_ptr;

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  memmove(p->trapframe, p->curr_state, sizeof(struct trapframe));
  kfree(p->curr_state);
  p->inside = 0;
  return p->trapframe->a0;
}

uint64
sys_settickets(void) {
  int tickets;
  argint(0, &tickets);

  struct proc *p = myproc();
  p->num_tickets = tickets;
  return 0;
}
