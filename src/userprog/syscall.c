#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/kernel/stdio.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "process.h"
#include "pagedir.h"
#include "filesys/file.h"
#ifndef ARGLEN
#define ARGLEN 5
#endif

typedef int pid_t;
int success_flag = 0;

static void syscall_handler (struct intr_frame *);
static void is_valid_ptr (const void *ptr);
static void is_valid_ptr_single (const void *ptr);
void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
bool is_in_valid_page (const void * ptr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // printf ("system call!\n");
  // thread_exit ();
  //first check if f->esp is a valid pointer)
  is_valid_ptr (f->esp);
  // printf ("%x\n", f->esp);
  //cast f->esp into an int*, then dereference it for the SYS_CODE

  int *p = f->esp;
  // printf ("%x\n", *p);
  int argv[ARGLEN];  
  // printf ("hhh\n");
  switch(*p)
  {
    case SYS_HALT:
    {
      halt ();
      //Implement syscall HALT
      break;
    }
    case SYS_EXIT:
    {
      // printf("---exit---\n");
      //Implement syscall EXIT
      read_args (1, argv, f);
      // thread_exit ();
      exit (argv[0]);
      break;
    }
    case SYS_EXEC:
    {
      // printf("---exec---\n");
      read_args (1, argv, f);
      is_valid_ptr ((void *) *(++p));
      f->eax = exec (argv[0]);
      break;
    }
    case SYS_WAIT:
    {
      // printf("---wait---\n");
      read_args (1, argv, f);
      //printf("---READ: %d---\n", argv[0]);
      //printf("---SYS: %d---\n", *(p + 1));
      f->eax = wait (argv[0]);
      break;
    }
    case SYS_CREATE:
    {
      // printf("---create---\n");
      read_args (2, argv, f);
      f->eax = create (argv[0], argv[1]);
      break;
    }
    case SYS_REMOVE:
    {
      // printf("---remove---\n");
      read_args (1, argv, f);
      f->eax = remove (argv[0]);
      break;
    }
    case SYS_OPEN:
    {
      // printf("---open---\n");
      read_args (1, argv, f);
      // printf("---open read done---\n");
      f->eax = open (argv[0]);
      break;
    }    
    case SYS_FILESIZE:
    {
      // printf("---filesize---\n");
      read_args (1, argv, f);
      f->eax = filesize (argv[0]);
      break;
    }
    case SYS_READ:
    {
      // printf("---read---\n");
      read_args (3, argv, f);
      f->eax = read (argv[0], argv[1], argv[2]);
      break;
    }
    case SYS_WRITE:
    {
      // printf("---write---\n");
      read_args (3, argv, f);
      // int fd = *((int*)f->esp + 1);
      // void* buffer = (void*)(*((int*)f->esp + 2));
      // unsigned size = *((unsigned*)f->esp + 3);
      f->eax = write (argv[0], argv[1], argv[2]);
      break;
    }
    case SYS_SEEK:
    {
      // printf("---seek---\n");
      read_args (2, argv, f);
      seek(argv[0], argv[1]);
      break;
    }
    case SYS_TELL:
    {
      // printf("---tell---\n");
      read_args (1, argv, f);
      f->eax = tell (argv[0]);
      break;
    }
    case SYS_CLOSE:
    {
      // printf("---close---\n");
      read_args (1, argv, f);
      close (argv[0]);
      break;
    }
  }
}

void 
read_args (int n, int *argv, struct intr_frame *f)
{
  int i;
  int *ptr;
  /*if (*(int*)f->esp == SYS_WAIT) {
    hex_dump((uintptr_t)f->esp, f->esp, sizeof(char)*16, true);
  }*/
  // printf ("---Sum: %d---\n", n);
  //hex_dump((uintptr_t)f->esp, f->esp, sizeof(char)*48, true);
  for (i = 0; i < n; ++i) 
  {
    ptr = ((int *)f->esp + 1 + i);
    // printf ("%s\n", *ptr);
    is_valid_ptr ((void *)ptr);
    // printf ("---%x---\n", *ptr);
    argv[i] = *ptr;
    // if (*(int*)f->esp == SYS_WAIT)
      //printf ("argv[%d]---%s---\n", i, argv[i]);
  }
  // printf ("%d\n", n);
}

static void 
is_valid_ptr (const void *ptr)
{
  // printf ("---address: %p---\n", ptr);
  is_valid_ptr_single ((int *)ptr);
  // printf ("---the pointer is: %p\n", ptr);
  for (int i = 0; i < 4; ++i)
    is_valid_ptr_single ((char *)ptr + i);
}

static void 
is_valid_ptr_single (const void *ptr)
{
  // printf ("---address: %p---\n", ptr);
  if (ptr == NULL || !is_user_vaddr (ptr) || !is_user_vaddr_above (ptr))
  {
    exit (-1);
  } 
  // printf ("---Before ID: %d---\n", thread_current ()->tid);
  void *ptr_check = pagedir_get_page (thread_current ()->pagedir, ptr);
  if (!ptr_check)
  {
    exit (-1);
  }
}

void 
halt ()
{
  shutdown_power_off ();
}

void 
exit (int status)
{
  // printf("---in exit %d---\n", thread_current()->tid);
  struct thread *cur = thread_current ();
  // printf ("jjj\n");
  // printf ("%s: exit(%d)\n", cur->name, status);
  struct thread *p = thread_find (cur->parentId);
  //printf("---%s %d\n", cur->tid, cur->exit_status);
  if (p != NULL)
  {
    // printf ("---%d is not null---\n", p->tid);
    struct list_elem *e;
    // int len = 0;
    // // struct list_elem *e;
    // for (e = list_begin (&p->children); e != list_end (&p->children); e = list_next (e))
    // {
    //   len++;
    // }
    // printf ("---the length of %d children is %d---\n", p->tid, len);
    struct child_process *child;
    for (e = list_begin (&p->children); e != list_tail (&p->children); e = list_next (e))
    {
      child = list_entry (e, struct child_process, childelem);
      // printf ("---child %d---\n", child->tid);
      if (child->tid == cur->tid)
      {
        lock_acquire (&cur->child_lock);
        child->exited = true;
        child->exit_status = status;
        lock_release (&cur->child_lock);
        // printf ("---%d: %d %d---\n", child->tid, child->exit_status, status);
        break;
      }
    }
  }
  // printf ("finish exit \n");
  printf ("%s: exit(%d)\n", cur->name, status);
  thread_exit ();
  
}

pid_t 
exec (const char *file)
{
  printf("---in exec---\n");
  if (!file)
    exit (-1);
  is_in_valid_page (file);
  // printf ("---exe id: %d---\n", thread_current ()->tid);
  pid_t pid = -1;
  // lock_acquire (&filesys_lock);
  printf ("------ load -------\n");
  pid = process_execute (file);
  // printf ("------- finish : %d -----\n", pid);
  // lock_release (&filesys_lock);
  // printf ("---%d---\n", pid);
  return pid;
}

int 
wait (pid_t child_tid)
{
  // printf("---in wait---\n");
  // printf ("---wait id: %d---\n", child_tid);
  int res = 0;
  res = process_wait (child_tid);
  return res;
}

bool 
create (const char *file, unsigned initial_size)
{
  if (!file)
    exit (-1);
  is_in_valid_page (file);
  bool flag;
  lock_acquire (&filesys_lock);
  // printf ("begin create\n");
  flag = filesys_create (file, initial_size);
  //printf ("---1 Done---\n");
  lock_release (&filesys_lock);
  //printf ("---2 Done---\n");
  return flag;
}

bool
is_in_valid_page (const void * ptr)
{
  if (!is_user_vaddr (ptr))
    exit (-1);
  void *ptr_check = pagedir_get_page (thread_current ()->pagedir, ptr);
  if (!ptr_check)
    exit (-1);
}

bool 
remove (const char *file)
{
  bool flag;
  lock_acquire (&filesys_lock);
  flag = filesys_remove (file);
  lock_release (&filesys_lock);
  return flag;
}

int 
open (const char *file)
{
  // printf ("cnm\n");
  if (!file)
    exit (-1);
  is_in_valid_page (file);
  lock_acquire (&filesys_lock);
  struct file *f = filesys_open (file);
  // printf ("%s, %s\n", file, thread_current()->name);
  if (!strcmp (file, thread_current ()->name)){
      // printf ("equal\n");
      file_deny_write (f);
  }
    
  if (!f)
  {
    lock_release (&filesys_lock);
    return -1;
  }
  int fd = add_file (f);
  lock_release (&filesys_lock);
  // printf ("released\n");
  return fd;
}

int 
filesize (int fd)
{
  lock_acquire (&filesys_lock);
  struct file* f = get_file (fd);
  if (!f)
  {
    lock_release (&filesys_lock);
    return -1;
  }
  int size = file_length (f);
  lock_release (&filesys_lock);
  return size;
}

int 
read (int fd, void *buffer, unsigned length)
{
  if (!buffer)
    exit (-1);
  is_valid_ptr (buffer);
  is_in_valid_page (buffer);
  char *buffer_cpy = (char*) buffer; 
  is_in_valid_page (buffer);
  if (fd == STDOUT_FILENO)
  {
    return length;
  }
  lock_acquire (&filesys_lock);
  struct file *f = get_file (fd);
  if (!f)
  {
    lock_release (&filesys_lock);
    return -1;
  }
  int size = file_read (f, buffer, length);
  lock_release (&filesys_lock);
  return size;
}

int 
write (int fd, const void *buffer, unsigned size)
{
  // printf ("----write-----\n");
  if (!buffer)
    exit (-1);
  for (int i = 0; i < size; i++) {
    is_valid_ptr (buffer + i);
  }
  // printf ("fd: %d\n", fd);
  // printf ("1\n");
  is_valid_ptr (buffer);
  // printf ("2\n");
  is_in_valid_page (buffer);
  // printf ("3\n");
  
  if (fd == STDOUT_FILENO)
  {
    // printf ("wsm\n");
    // printf ("size: %d\n", size);
    putbuf (buffer, size);
    // lock_release (&filesys_lock);
    // printf ("nmd\n");
    return size;
  }
  if (fd == STDIN_FILENO)
  {
    // lock_release (&filesys_lock);
    return -1;
  }
  lock_acquire (&filesys_lock);
  // printf ("-----1------\n");
  struct file *f = get_file (fd);
  // printf ("-----2------\n");
  if (!f)
  {
    lock_release(&filesys_lock);
    return -1;
  }
  if (is_deny_write(f))
  {
    lock_release (&filesys_lock);
    // printf("----------------\n");
    return 0;
  }
  int length = file_write (f, buffer, size);
  lock_release (&filesys_lock);
  // printf ("-----------%d---------------\n", length);
  return length;
}

void 
seek (int fd, unsigned position) 
{
  lock_acquire (&filesys_lock);
  struct File *file = get_file (fd);
  if (!file) 
  {
    lock_release (&filesys_lock);
    return -1;
  }
  file_seek (file, position);
  lock_release (&filesys_lock);
}

unsigned 
tell (int fd) 
{
  lock_acquire (&filesys_lock);
  struct File *file = get_file (fd);
  if (!file)
  {
    lock_release (&filesys_lock);
    return -1;
  }
  off_t size = file_tell (file);
  return size;
}

void 
close (int fd) 
{
  lock_acquire (&filesys_lock);
  bool res = close_file (fd);
  if (!res)
  {
    lock_release (&filesys_lock);
    exit(-1);
  }
  lock_release (&filesys_lock);
}


