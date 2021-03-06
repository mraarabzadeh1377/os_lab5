#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void)
{
  return fork();
}

int sys_exit(void)
{
  exit();
  return 0; // not reached
}

int sys_wait(void)
{
  return wait();
}

int sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void)
{
  return myproc()->pid;
}

int sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_inc_num(void)
{
  int num;
  if (argint(0, &num) < 0)
    return -1;
  return inc_num(num);
}

int sys_invoked_systemcall(void)
{
  int pid;
  if (argint(0, &pid) < 0)
    return -1;
  invoked_systemcall(pid);
  return 1;
}

int sys_sort_systemcall(void)
{
  int pid;
  if (argint(0, &pid) < 0)
    return -1;
  sort_systemcall(pid);
  return 1;
}

int sys_get_count(void)
{
  int pid;
  int sysnum;
  if (argint(0, &pid) < 0)
    return -1;
  if (argint(1, &sysnum) < 0)
    return -1;
  get_count(pid, sysnum);
  return 1;
}

int sys_log_systemcall(void)
{
  log_systemcall();
  return 1;
}

int sys_ticketlock_init(void)
{
  ticketlock_init();
  return 1;
}

int sys_ticketlock_test(void)
{
  ticketlock_test();
  return 1;
}

int sys_rwlock_init(void)
{
  rwlock_init();
  return 1;
}

int sys_rwlock_test(void)
{
  int num;
  if (argint(0, &num) < 0)
    return -1;

  rwlock_test(num);
  return 1;
}
int sys_addProcessToPriorityQueue(void)
{
  int num, pid;
  if (argint(0, &pid) < 0)
    return -1;
  if (argint(1, &num) < 0)
    return -1;
  addProcessToPriorityQueue(pid, num);
  return 1;
}

int sys_addProcessToFCFSQueue(void)
{
  int pid;
  if (argint(0, &pid) < 0)
    return -1;
  addProcessToFCFSQueue(pid);
  return 1;
}
int sys_addProcessToLattaryQueue(void)
{
  int num, pid;
  if (argint(0, &pid) < 0)
    return -1;
  if (argint(1, &num) < 0)
    return -1;
  cprintf("in wrapper:%d\n", num);
  addProcessToLattaryQueue(pid, num);
  return 1;
}
int sys_changeProcessQueue(void)
{
  int num, pid, qstate;
  if (argint(0, &qstate) < 0)
    return -1;
  if (argint(1, &pid) < 0)
    return -1;
  if (argint(2, &num) < 0)
    return -1;
  changeProcessQueue(qstate, pid, num);
  return 1;
}
int sys_print_all_proc(void)
{
  print_all_proc();
  return 0;
}

int sys_shm_open(void)
{
  int id, count, flag;
  if (argint(0, &id) < 0)
    return -1;
  if (argint(1, &count) < 0)
    return -1;
  if (argint(2, &flag) < 0)
    return -1;
  return shm_open(id, count, flag);
}
int sys_shm_close(void)
{
  int id;
  if (argint(0, &id) < 0)
    return -1;
  return shm_close(id);
}
int sys_shm_attach(void)
{
  int id;
  if (argint(0, &id) < 0)
    return -1;
  shm_attach(id);
  return 1;
}