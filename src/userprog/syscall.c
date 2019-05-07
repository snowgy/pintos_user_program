#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/kernel/stdio.h"
#ifndef ARGLEN
#define ARGLEN 5
#endif


static void syscall_handler (struct intr_frame *);
static bool is_valid_ptr (const void *ptr);
int write (int fd, const void* buffer, unsigned size);
void exit (int status);

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
  if (!is_valid_ptr (f->esp))
  {
    exit (-1);
  }
  //cast f->esp into an int*, then dereference it for the SYS_CODE

  int argv[ARGLEN];

  switch(*(int*)f->esp)
  {
    case SYS_HALT:
    {
      //Implement syscall HALT
      break;
    }
    case SYS_EXIT:
    {
      //Implement syscall EXIT
      read_args (1, argv, f);
      exit (argv[0]);
      break;
    }
    case SYS_EXEC:
    {
      read_args (1, argv, f);
      break;
    }
    case SYS_WAIT:
    {
      read_args (1, argv, f);
      break;
    }
    case SYS_CREATE:
    {
      read_args (2, argv, f);
      break;
    }
    case SYS_REMOVE:
    {
      read_args (1, argv, f);
      break;
    }
    case SYS_OPEN:
    {
      read_args (1, argv, f);
      break;
    }    
    case SYS_FILESIZE:
    {
      read_args (1, argv, f);
      break;
    }
    case SYS_READ:
    {
      read_args (3, argv, f);
      break;
    }
    case SYS_WRITE:
    {
      read_args (3, argv, f);
      // int fd = *((int*)f->esp + 1);
      // void* buffer = (void*)(*((int*)f->esp + 2));
      // unsigned size = *((unsigned*)f->esp + 3);
      f->eax = write (argv[0], argv[1], argv[2]);
      break;
    } 
    case SYS_SEEK:
    {
      read_args (2, argv, f);
      break;
    }
    case SYS_TELL:
    {
      read_args (1, argv, f);
      break;
    }
    case SYS_CLOSE:
    {
      read_args (1, argv, f);
      break;
    }
  }
}

void read_args (int n, int *argv, struct intr_frame *f)
{
  int i;
  for (i = 0; i < n; ++i) 
  {
    argv[i] = *((int *)f->esp + 1 + i);
  }
}

static bool is_valid_ptr (const void *ptr)
{
  return is_user_vaddr (ptr) && is_user_vaddr_above (ptr);
}

void exit (int status)
{
  printf ("%s: exit(%d)\n", thread_current ()->name, status);
  thread_exit ();
}

int write (int fd, const void* buffer, unsigned size)
{
  if (fd == STDOUT_FILENO)
  {
    putbuf (buffer, size);
    return;
  }
}
