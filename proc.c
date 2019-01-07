// ticketlock
struct ticketlock ticketlockTest;
int count = 0;

// rwlock
struct rwlock rwlockTest;
int rwCounter = 0;
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "ticketlock.h"
#include "rwlock.h"
#include "date.h"
#define MAX_QUEUES_LENGTH 20
#define MOST_PRIORITY 10

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

struct proc *priorityQueue[MAX_QUEUES_LENGTH];
struct proc *fcfsQueue[MAX_QUEUES_LENGTH];
struct proc *lattaryQueue[MAX_QUEUES_LENGTH];

int priorityQueueIterator = 0;
int fcfsQueueIterator = 0;
int lattaryQueueIterator = 0;

int proc_create_time = 0;
int lattaryBase = 0;
unsigned long timer = 0;
unsigned long randstate = 1;
unsigned int rand()
{
  randstate = randstate * 1664525 + 1013904223;
  return randstate;
}

struct proc *priorityScheduler(void)
{
  struct proc *p = 0;
  priorityQueueIterator = priorityQueueIterator % MAX_QUEUES_LENGTH;

  int most_priority = MOST_PRIORITY;

  // cprintf("priorityScheduler\n");
  int j = 0;
  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    if (priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH])
    {
      if ((priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH]->priority < most_priority) && (priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH]->state == RUNNABLE))
      {
        // cprintf("in if %s  %d  %d \n", priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH]->name, priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH]->pid, priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH]->state);
        p = priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH];
        most_priority = priorityQueue[(i + priorityQueueIterator) % MAX_QUEUES_LENGTH]->priority;
        j = (i + priorityQueueIterator) % MAX_QUEUES_LENGTH;
        // p->priority = p->priority + 1;
      }
    }
    // cprintf("in for %d\n", i);
  }
  // cprintf(" out for \n");
  priorityQueueIterator = j + 1;
  return p;
}
struct proc *fcfsScheduler(void)
{
  struct proc *p = 0;
  int min_create_time = proc_create_time;
  // cprintf("priorityScheduler\n");
  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    if (fcfsQueue[i])
    {
      if ((fcfsQueue[i]->state == RUNNABLE) && (fcfsQueue[i]->createdAt < min_create_time))
      {
        // cprintf("fcfsScheduler\n");
        p = fcfsQueue[i];
        min_create_time = p->createdAt;
      }
    }
    // cprintf("in for %d\n", i);
  }
  // cprintf(" out for \n");
  return p;
}
int lattaryLength = 0;
struct proc *lattaryScheduler()
{
  lattaryLength = 0;
  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    if (lattaryQueue[i])
    {
      if (lattaryQueue[i]->state == RUNNABLE)
      {
        lattaryLength += lattaryQueue[i]->lattary.length;
      }
    }
  }

  if (lattaryLength)
  {
    // cprintf("lattaryScheduler lenght %d\n ", lattaryLength);
    int lattaryRand = rand() % lattaryLength;
    for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
    {
      if (lattaryQueue[i])
      {
        if (lattaryQueue[i]->state == RUNNABLE)
        {
          lattaryRand -= lattaryQueue[i]->lattary.length;
          if (lattaryRand <= 0)
          {
            // cprintf("lattaryScheduler done\n");
            return lattaryQueue[i];
          }
        }
      }
    }
  }
  // cprintf("lattaryScheduler failed\n");
  return 0;
}

int addProcessToPriorityQueue(int procId, int prio)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {

    if (p->pid == procId)
    {
      break;
    }
  }
  p->priority = prio;
  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {

    if (priorityQueue[i] == 0 || priorityQueue[i]->state == UNUSED)
    {
      priorityQueue[i] = p;
      p->procQu = PRIORITY;
      return 1;
    }
  }
  return 0;
}

int addProcessToFCFSQueue(int procId)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {

    if (p->pid == procId)
    {
      break;
    }
  }
  p->createdAt = proc_create_time++;
  cprintf("%d\t%d\n", p->createdAt, p->timer);

  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    if (fcfsQueue[i] == 0 || fcfsQueue[i]->state == UNUSED)
    {
      fcfsQueue[i] = p;
      p->procQu = FCFS;
      return 1;
    }
  }
  return 0;
}
int addProcessToLattaryQueue(int procId, int length)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {

    if (p->pid == procId)
    {
      break;
    }
  }
  p->lattary.base = lattaryBase;
  p->lattary.length = length;
  lattaryBase += length;

  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    if (lattaryQueue[i] == 0 || lattaryQueue[i]->state == UNUSED)
    {
      lattaryQueue[i] = p;
      p->procQu = LATTARY;
      return 1;
    }
  }
  return 0;
}

int removeFromQueue(int procId, struct proc *queue[MAX_QUEUES_LENGTH])
{
  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    if (queue[i] != 0 && queue[i]->pid == procId)
    {
      queue[i] = 0;
      return 1;
    }
  }
  return 0;
}

int changeProcessQueue(int q, int procId, int prioOrlength)
{
  struct proc *p;
  // cprintf("in change queue:%d\n", prioOrlength);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {

    if (p->pid == procId)
    {
      break;
    }
  }

  if (q == p->procQu)
  {
    if (q == PRIORITY)
    {
      p->priority = prioOrlength;
    }
    else if (q == LATTARY)
    {
      lattaryBase -= p->lattary.length;
      p->lattary.length = prioOrlength;
      lattaryBase += p->lattary.length;
    }
    return 1;
  }

  if (p->procQu == FCFS)
  {
    removeFromQueue(procId, fcfsQueue);
  }
  else if (p->procQu == PRIORITY)
  {
    removeFromQueue(procId, priorityQueue);
  }
  else if (p->procQu == LATTARY)
  {
    removeFromQueue(procId, lattaryQueue);
  }

  if (q == FCFS)
  {
    return addProcessToFCFSQueue(procId);
  }
  else if (q == PRIORITY)
  {
    return addProcessToPriorityQueue(procId, prioOrlength);
  }
  else if (q == LATTARY)
  {
    return addProcessToLattaryQueue(procId, prioOrlength);
    // cprintf("in change line: %d\n", q);
  }
  return 0;
}

void pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int cpuid()
{
  return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i)
  {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0)
  {
    p->state = UNUSED;
    return 0;
  }
  p->timer = timer;
  // if (addProcessToLattaryQueue(p->pid, 10))
  // {
  //   cprintf("added to PriorityQueue  in allocproc \n");
  // }
  // else
  // {
  //   cprintf("not added to PriorityQueue  in allocproc \n");
  // }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  for (int i = 0; i < MAX_QUEUES_LENGTH; i++)
  {
    priorityQueue[i] = 0;
  }

  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if (n > 0)
  {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  else if (n < 0)
  {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++)
  {
    if (curproc->ofile[fd])
    {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curproc)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for (;;)
  {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&ptable.lock))
    panic("sched ptable.lock");
  if (mycpu()->ncli != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (readeflags() & FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  acquire(&ptable.lock); //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first)
  {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  if (lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if (lk != &ptable.lock)
  {                        //DOC: sleeplock0
    acquire(&ptable.lock); //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock)
  { //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

void ticketlockSleep(void *chan)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  acquire(&ptable.lock);

  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;
  release(&ptable.lock);
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void wakeup1(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

void wakeup2(void *chan, int pid)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan && p->pid == pid)
    {
      p->state = RUNNABLE;
      break;
    }
}

void wakeupByPid(void *chan, int pid)
{
  acquire(&ptable.lock);
  wakeup2(chan, pid);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [EMBRYO] "embryo",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if (p->state == SLEEPING)
    {
      getcallerpcs((uint *)p->context->ebp + 2, pc);
      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

//our systemcall

int inc_num(int a)
{
  cprintf(" %d ", a + 1);
  return a + 1;
}

void invoked_systemcall(int pid)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->pid == pid)
      goto pri;
  cprintf("pid not found\n");
  return;
pri:
  for (int i = 0; i < 27; ++i)
  {
    if (p->systemcalls[i])
    {
      cprintf("id: %d \n", p->systemcalls[i]->id);
      cprintf("name: %s ", p->systemcalls[i]->name);
      cprintf("call num: %d \n", p->systemcalls[i]->number_of_call);

      struct systemcall_instance *si = p->systemcalls[i]->instances;
      for (int j = 0; j < p->systemcalls[i]->number_of_call; j++)
      {
        cprintf(" time: %d/%d/%d  %d:%d:%d \n", si->time->year, si->time->month, si->time->day, si->time->hour, si->time->minute, si->time->second);
        for (int k = 0; k < p->systemcalls[i]->parameter_number; k++)
        {
          cprintf(" %s ", p->systemcalls[i]->arg_type[k]);
          if (!strncmp(p->systemcalls[i]->arg_type[k], "int*", 4) || !strncmp(p->systemcalls[i]->arg_type[k], "struct stat*", 12) || !strncmp(p->systemcalls[i]->arg_type[k], "char**", 6))
          {
            //cprintf(" %s ", si->arg_value[k]->pointer_val[0]);
          }
          else if (!strncmp(p->systemcalls[i]->arg_type[k], "int", 3) || !strncmp(p->systemcalls[i]->arg_type[k], "short", 5) || !strncmp(p->systemcalls[i]->arg_type[k], "fd", 2))
          {
            cprintf(" %d ", si->arg_value[k]->int_val);
          }
          else if (!strncmp(p->systemcalls[i]->arg_type[k], "char*", 5))
          {
            cprintf(" %s ", si->arg_value[k]->chars_val);
          }
        }
        cprintf("\n");
        si = si->next;
      }
      cprintf("\n");
    }
  }
}

void sort_systemcall(int pid)
{
  invoked_systemcall(pid);
}

void get_count(int pid, int sysnum)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->pid == pid)
      goto write;
  cprintf("pid not found\n");
  return;
write:
  if (p->systemcalls[sysnum])
  {
    cprintf("proccess id is:%d  systemcall number is:%d  number of use is:%d \n", p->pid, sysnum,
            p->systemcalls[sysnum]->number_of_call);
  }
  else
  {
    cprintf("this systemcall not found for this process\n");
  }
}

void log_systemcall(void)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == RUNNING || p->state == SLEEPING)
    {
      cprintf("process name:%s\n", p->name);
      for (int i = 0; i < 27; ++i)
      {
        if (p->systemcalls[i])
        {
          cprintf("id: %d \n", p->systemcalls[i]->id);
          cprintf("name: %s ", p->systemcalls[i]->name);
          cprintf("call num: %d \n", p->systemcalls[i]->number_of_call);

          struct systemcall_instance *si = p->systemcalls[i]->instances;
          for (int j = 0; j < p->systemcalls[i]->number_of_call; j++)
          {
            cprintf(" time: %d/%d/%d  %d:%d:%d \n", si->time->year, si->time->month, si->time->day, si->time->hour, si->time->minute, si->time->second);
            for (int k = 0; k < p->systemcalls[i]->parameter_number; k++)
            {
              cprintf(" %s ", p->systemcalls[i]->arg_type[k]);
              if (!strncmp(p->systemcalls[i]->arg_type[k], "int*", 4) || !strncmp(p->systemcalls[i]->arg_type[k], "struct stat*", 12) || !strncmp(p->systemcalls[i]->arg_type[k], "char**", 6))
              {
                //cprintf(" %s ", si->arg_value[k]->pointer_val[0]);
              }
              else if (!strncmp(p->systemcalls[i]->arg_type[k], "int", 3) || !strncmp(p->systemcalls[i]->arg_type[k], "short", 5) || !strncmp(p->systemcalls[i]->arg_type[k], "fd", 2))
              {
                cprintf(" %d ", si->arg_value[k]->int_val);
              }
              else if (!strncmp(p->systemcalls[i]->arg_type[k], "char*", 5))
              {
                cprintf(" %s ", si->arg_value[k]->chars_val);
              }
            }
            cprintf("\n");
            si = si->next;
          }
          cprintf("\n");
        }
      }
    }
}

void ticketlock_init(void)
{
  initTicketlock(&ticketlockTest, "ticketLock test");
}

void delay(int amount)
{
  uint initial_ticks, current_ticks;

  acquire(&tickslock);
  initial_ticks = ticks;
  release(&tickslock);

  while (1)
  {
    acquire(&tickslock);
    current_ticks = ticks;
    release(&tickslock);

    if (current_ticks - initial_ticks > amount)
      break;
  }
}

void ticketlock_test(void)
{
  acquireTicket(&ticketlockTest);
  delay(100);
  count++;
  cprintf(" pid: %d   count: %d\n", myproc()->pid, count);
  releaseTicket(&ticketlockTest);
}

void rwlock_init(void)
{
  initRwlock(&rwlockTest, "rwlock test");
}

void rwlock_test(int mode)
{
  rwWait(&rwlockTest, mode);
  delay(80);
  if (!mode)
  {
    cprintf("reading => pid: %d   counter: %d \n", myproc()->pid, rwCounter);
  }
  else
  {
    cprintf("writing => pid: %d   counter: %d \n", myproc()->pid, rwCounter);
    rwCounter++;
  }
  rwSignal(&rwlockTest);
}

void print_all_proc(void)
{
  struct proc *p;
  char situation[10];
  char qstate[10];
  cprintf("id\tname\tstate\tpriority createtime\tlattary_ticket\tscheduling_queue\n\n");
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state != UNUSED)
    {
      if (p->state == RUNNABLE)
        strncpy(situation, "RUNNABL", 7);
      if (p->state == RUNNING)
        strncpy(situation, "RUNNING", 7);
      if (p->state == SLEEPING)
        strncpy(situation, "SLEEPIN", 7);
      if (p->state == ZOMBIE)
        strncpy(situation, "ZOMBIES", 7);
      if (p->state == EMBRYO)
        strncpy(situation, "EMBRYIO", 7);
      situation[7] = '\0';
      if (p->procQu == FCFS)
        strncpy(qstate, "FCFS", 4);
      if (p->procQu == PRIORITY)
        strncpy(qstate, "PRIO", 4);
      if (p->procQu == LATTARY)
        strncpy(qstate, "LATT", 4);
      qstate[4] = '\0';
      cprintf("%d\t%s\t%s\t%d\t%d\t%d\t%s\n", p->pid, p->name, situation, p->priority, p->createdAt, p->lattary.length, qstate);
    }
  }
  cprintf("\n\tend of showing\n");
}

//