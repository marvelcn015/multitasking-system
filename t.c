/*********** A Multitasking System ************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NPROC 10
#define SSIZE 1024

#define FREE 0
#define READY 1
#define SLEEP 2
#define ZOMBIE 3
#define RUNNING 4

typedef struct proc
{
  struct proc *next;
  int *saved_sp;

  int pid;
  int ppid;
  int status;
  int priority;
  int event;
  int exitCode;

  struct proc *child;
  struct proc *sibling;
  struct proc *parent;

  int kstack[SSIZE];
} PROC;

PROC proc[NPROC];
PROC *freeList;
PROC *sleepList;
PROC *readyQueue;
PROC *running;

int findCmd(char *);
void tswitch(void);
int do_switch(void);
int do_fork(void);
int do_ps(void);
int do_exit(void);
int kexit(int value);
int kfork(int (*func)(void));
int body(void);
int do_sleep(void);
int do_wakeup(void);
int do_wait(void);
int do_shutdown(void);
int ksleep(int event);
int kwakeup(int event);
int kwait(int *status);
void printProcessTree(PROC *node, int depth);

/****************** queue.c file ********************/
int enqueue(PROC **queue, PROC *p)
{
  PROC *q = *queue;
  if (q == 0 || p->priority > q->priority)
  {
    *queue = p;
    p->next = q;
  }
  else
  {
    while (q->next && p->priority <= q->next->priority)
      q = q->next;
    p->next = q->next;
    q->next = p;
  }
  return 0;
}

// remove and return first PROC in queue
PROC *dequeue(PROC **queue)
{
  PROC *p = *queue;
  if (p)
    *queue = (*queue)->next;
  return p;
}

void printList(char *name, PROC *p)
{
  printf("%s = ", name);
  while (p)
  {
    printf("[%d %d]->", p->pid, p->priority);
    p = p->next;
  }
  printf("NULL\n");
}

char *cmds[] = {"ps", "fork", "switch", "exit", "sleep", "wakeup", "wait", "shutdown", 0};

int (*fptr[])() = {do_ps, do_fork, do_switch, do_exit, do_sleep, do_wakeup, do_wait, do_shutdown};

/*******************************************************
kfork() creates a child process; returns child pid.
When scheduled to run, child PROC resumes to func();
********************************************************/
int kfork(int (*func)(void))
{
  PROC *p;
  int i;
  // Get a proc from freeList for child process
  p = dequeue(&freeList);
  if (!p)
  {
    printf("no more proc\n");
    return (-1);
  }

  p->child = 0;
  p->sibling = 0;
  p->parent = running;
  // Insert p to END of running's child list
  if (running->child == 0)
    running->child = p;
  else
  {
    PROC *q = running->child;
    while (q->sibling)
      q = q->sibling;
    q->sibling = p;
  }

  // Initialize the new proc and its stack
  p->status = READY;
  p->priority = 1; // for ALL PROCs except P0
  p->ppid = running->pid;
  p->parent = running;

  // kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
  for (i = 1; i < 10; i++)
    p->kstack[SSIZE - i] = 0;

  p->kstack[SSIZE - 1] = (int)func;
  p->saved_sp = &(p->kstack[SSIZE - 9]);

  enqueue(&readyQueue, p);

  return p->pid;
}

int kexit(int value)
{
  running->exitCode = value;
  running->status = ZOMBIE;
  running->priority = 0;

  // Transfer all children to process 1
  if (running->child != 0)
  {
    PROC *p1_child = proc[1].child;

    if (p1_child == 0)
    {
      proc[1].child = running->child;
    }
    else
    {
      // Find the last child of process 1
      while (p1_child->sibling != 0)
      {
        p1_child = p1_child->sibling;
      }
      p1_child->sibling = running->child;
    }

    // Set all children's parent to process 1
    PROC *child = running->child;
    while (child != 0)
    {
      child->parent = &proc[1];
      child->ppid = 1;
      child = child->sibling;
    }

    running->child = 0;
  }

  // Wake up parent if it's sleeping
  if (running->parent->status == SLEEP)
  {
    kwakeup(running->parent->pid);
  }

  tswitch();
}

int do_fork(void)
{
  int child = kfork(body);
  if (child < 0)
    printf("kfork failed\n");
  else
  {
    printf("proc %d kforked a child = %d\n", running->pid, child);
    printList("readyQueue", readyQueue);
  }
  return child;
}

int do_switch(void)
{
  printf("proc %d switch task\n", running->pid);
  tswitch();
  printf("proc %d resume\n", running->pid);
}

int do_shutdown(void)
{
  // Only P1 can execute shutdown
  if (running->pid != 1)
  {
    printf("Only P1 can shutdown the system\n");
    return -1;
  }

  // Clear ready queue
  while (readyQueue)
  {
    PROC *p = dequeue(&readyQueue);
    p->status = FREE;
    enqueue(&freeList, p);
  }

  // Clear sleep queue
  while (sleepList)
  {
    PROC *p = dequeue(&sleepList);
    p->status = FREE;
    enqueue(&freeList, p);
  }

  printf("System shutting down...\n");
  exit(0);
}

char *pstatus[] = {"FREE   ", "READY  ", "SLEEP  ", "ZOMBIE ", "RUNNING"};

void printProcessTree(PROC *node, int depth)
{
  printf("\033[0;32;34m");
  if (node == NULL)
    return;
  for (int i = 0; i < depth; i++)
  {
    printf("    ");
  }
  if (node->pid == 0)
    printf("P%d: %s\n", node->pid,
           node == running ? "RUNNING" : pstatus[node->status]);
  else
    printf("└─ P%d: %s\n", node->pid,
           node == running ? "RUNNING" : pstatus[node->status]);
  printProcessTree(node->child, depth + 1);
  printProcessTree(node->sibling, depth);
  printf("\033[0m");
}

int do_exit(void)
{
  int exitCode;

  // Process 1 never dies
  if (running->pid == 1)
  {
    printf("P1 never dies\n");
    return -1;
  }

  printf("enter an exitCode value: ");
  scanf("%d", &exitCode);
  // Clear input buffer
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
  {
  }

  kexit(exitCode);
  return 0;
}

int do_ps(void)
{
  int i;
  PROC *p;
  printf("pid   ppid    status    event\n");
  printf("-----------------------------\n");
  for (i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    printf("%2d%7d", p->pid, p->ppid);
    if (p == running)
      printf("%13s", pstatus[4]);
    else
      printf("%13s", pstatus[p->status]);
    printf("%6d\n", p->event);
  }
}

int do_sleep(void)
{
  // Process 1 refuses to sleep by command
  if (running->pid == 1)
  {
    printf("P1 refuses to sleep\n");
    return -1;
  }

  int event;
  printf("enter an event value: ");
  scanf("%d", &event);
  // Clear input buffer
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
  {
  }

  ksleep(event);
  return 0;
}

int do_wakeup(void)
{
  int event;
  printf("enter an event value: ");
  scanf("%d", &event);
  // Clear input buffer
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
  {
  }

  kwakeup(event);
  return 0;
}

int do_wait(void)
{
  int exitCode;
  int pid = kwait(&exitCode);

  if (pid < 0)
  {
    printf("proc %d has no child\n", running->pid);
  }
  else if (pid > 0)
  {
    printf("proc %d waited for a ZOMBIE child %d exitCode=%d\n",
           running->pid, pid, exitCode);
  }
  return pid;
}

int ksleep(int event)
{
  // Event value must be greater than 0
  if (event <= 0)
  {
    printf("event must be greater than 0\n");
    return -1;
  }

  running->event = event;
  running->status = SLEEP;

  enqueue(&sleepList, running);
  tswitch();
  return 0;
}

int kwakeup(int event)
{
  // Find and wake up processes with matching event
  PROC *p = sleepList;
  PROC *prev = sleepList;
  bool found = false;

  while (p != 0)
  {
    if (p->event == event)
    {
      PROC *temp = p;

      if (p == sleepList)
      {
        // Case 1: Found process is head of sleep list
        sleepList = p->next;
        p = sleepList;
        prev = sleepList;
      }
      else
      {
        // Case 2: Found process is not head of sleep list
        prev->next = p->next;
        p = p->next;
      }

      // Move found process to ready queue
      temp->next = 0;
      enqueue(&readyQueue, temp);
      temp->event = 0;
      temp->status = READY;
      found = true;
      printf("kwakeup: wake up proc %d\n", temp->pid);
    }
    else
    {
      prev = p;
      p = p->next;
    }
  }

  if (!found)
  {
    printf("kwakeup: no process with event %d\n", event);
  }
  return 0;
}

int kwait(int *status)
{
  // Return -1 if no children
  if (running->child == 0)
  {
    return -1;
  }

  // Search for ZOMBIE child process
  PROC *p = running->child;
  PROC *prev = running->child;

  while (p != 0)
  {
    if (p->status == ZOMBIE)
    {
      int pid = p->pid;
      *status = p->exitCode;
      p->status = FREE;

      // Remove from child list
      if (p == running->child)
      {
        // Case 1: Found process is first child
        running->child = p->sibling;
      }
      else
      {
        // Case 2: Found process is not first child
        prev->sibling = p->sibling;
      }

      // Clean up the process
      p->parent = 0;
      p->sibling = 0;
      p->ppid = 0;
      p->next = 0;
      enqueue(&freeList, p);
      return pid;
    }
    prev = p;
    p = p->sibling;
  }

  // No ZOMBIE child found, sleep and wait
  ksleep(running->pid);
  return 0;
}

int body(void)
{
  char command[64];
  printf("proc %d start from body()\n", running->pid);
  while (1)
  {
    printf("***************************************\n");
    printf("proc %d running: Parent=%d  child = ", running->pid, running->ppid);

    PROC *p = running->child;
    while (p != 0)
    {
      printf("[%d %s]->", p->pid, pstatus[p->status]);
      p = p->sibling;
    }
    puts("NULL");

    printList("freeList ", freeList);
    printList("sleepList", sleepList);
    printList("readQueue", readyQueue);

    // Display process tree
    printf("\n--- Process Tree ---\n");
    printProcessTree(&proc[0], 0);
    printf("\n");

    printf("P%d$ [ps|fork|switch|exit|sleep|wakeup|wait]: ", running->pid);
    fgets(command, 64, stdin);
    command[strlen(command) - 1] = 0;

    int index = findCmd(command);
    if (index >= 0)
    {
      fptr[index]();
    }
    else
    {
      puts("invalid command");
    }
  }
}

int init()
{
  int i;
  for (i = 0; i < NPROC; i++)
  {
    proc[i].pid = i;
    proc[i].status = FREE;
    proc[i].priority = 0;
    proc[i].next = (PROC *)&proc[(i + 1)];
  }
  proc[NPROC - 1].next = 0;

  freeList = &proc[0];
  sleepList = 0;
  readyQueue = 0;

  // Create P0 as the initial running process
  running = dequeue(&freeList);
  running->ppid = 0;
  running->status = READY;
  running->priority = 0;
  running->parent = running;
  printList("freeList", freeList);
  printf("init complete: P0 running\n");
}

/*************** main() ***************/
int main()
{
  printf("\nWelcome to UTCS Multitasking System\n");
  init();
  printf("P0 fork P1\n");
  kfork(body);

  while (1)
  {
    if (readyQueue)
    {
      printf("P0: switch task\n");
      tswitch();
    }
  }
}

/*********** scheduler *************/
int scheduler()
{
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
    enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);
}

int findCmd(char *command)
{
  int i = 0;
  while (cmds[i])
  {
    if (strcmp(command, cmds[i]) == 0)
      return i;
    i++;
  }
  return -1;
}