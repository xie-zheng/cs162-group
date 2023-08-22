#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "lib/kernel/console.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

/* since pintos' file system is not thread-safe */
static struct lock filesys_lock;

static void syscall_handler(struct intr_frame*);
void syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

/* Process Control Syscalls */
static void sys_exit(int status UNUSED);
int sys_wait (pid_t pid);

/* File Operation Syscalls */
bool sys_create (const char *file, unsigned initial_size);
bool sys_remove (const char *file);
int sys_open (const char *file);
int sys_filesize (int fd);
int sys_read (int fd, void *buffer, unsigned size);
int sys_write (int fd, const void *buffer, unsigned size);
void sys_seek (int fd, unsigned position);
unsigned sys_tell(int fd);
void sys_close (int fd);


/* ----- handler user virtual memory ----- */

/* verify the validity of a user-provided pointer. */
static inline bool verify_vaddr(const uint8_t* vaddr) { return !(vaddr == NULL || pagedir_get_page(thread_current()->pcb->pagedir, vaddr) == NULL); }

/* verify an array of user-provided pointers. */
static void verify_multiple(const uint8_t* vaddr, size_t cnt) {
  for (size_t i = 0; i < cnt; i++)
    if (!verify_vaddr(vaddr + i))
        sys_exit(-1);
}

static void verify_str(const char* ptr) {
  while (verify_vaddr((uint8_t*)ptr))
    if (*ptr++ == '\0')
      return;
  sys_exit(-1);
}


/* ----- hanlders for syscall(process) ----- */

/* release the resources and exit */
static void sys_exit(int status UNUSED) {
  process_exit();
}

/* wait must fail and return -1 immediately if any of the following conditions are true:
 *
 * 1. pid does not refer to a direct child of the calling process. 
 *    pid is a direct child of the calling process if and only if 
 *    the calling process received pid as a return value from a successful call to exec. 
 *    Note that children are not inherited: if A spawns child B and B spawns child process C,
 *    then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail. 
 *    Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
 *
 * 2. The process that calls wait has already called wait on pid.
 *    That is, a process may wait for any given child at most once. */
int sys_wait (pid_t pid) {
  // cond1
  return -1;
}


/* ----- handlers for syscall(filesys) ----- */

bool sys_create(const char *file, unsigned initial_size) {
  bool result = false;
  lock_acquire(&filesys_lock);
  result = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return result;
}

bool sys_remove(const char *file) {
  bool result = false;
  lock_acquire(&filesys_lock);
  result = filesys_remove(file);
  lock_release(&filesys_lock);
  return result;
}

int sys_open(const char* file) {
  int result = -1;
  lock_acquire(&filesys_lock);
  struct file* f = filesys_open(file);
  struct process* p = thread_current()->pcb;
  if (f != NULL) {
    result = p->next_fd++;
    p->fds[result] = f;
  }
  lock_release(&filesys_lock);

  return result;
}

int sys_filesize(int fd) {
  int size = 0;
  lock_acquire(&filesys_lock);
  struct process* p = thread_current()->pcb;
  // TODO: validify the fd
  //
  // assume for that fd is valid
  // what about 0, 1(stdin stdout)?
  size = file_length(p->fds[fd]);
  lock_release(&filesys_lock);

  return size;
}

int sys_read(int fd, void *buffer, unsigned size) {
  int result = 0;
  lock_acquire(&filesys_lock);
  struct process* p = thread_current()->pcb;
  struct file* f = p->fds[fd];
  result = file_read(f, buffer, size);
  lock_release(&filesys_lock);

  return result;
}

int sys_write(int fd, const void *buffer, unsigned size) {
  int result = 0;
  lock_acquire(&filesys_lock);
  struct process* p = thread_current()->pcb;
  struct file* f = p->fds[fd];
  result = file_write(f, buffer, size);
  lock_release(&filesys_lock);

  return result;
}

void sys_seek(int fd, unsigned position) {
  lock_acquire(&filesys_lock);
  struct process* p = thread_current()->pcb;
  struct file* f = p->fds[fd];
  file_seek(f, position);
  lock_release(&filesys_lock);
}

unsigned sys_tell(int fd) {
  unsigned result;
  lock_acquire(&filesys_lock);
  struct process* p = thread_current()->pcb;
  struct file* f = p->fds[fd];
  result = file_tell(f);
  lock_release(&filesys_lock);
  return result;
}

/* ----- syscall dispatcher ----- */

static void syscall_handler(struct intr_frame* f UNUSED) {
  uint32_t* args = ((uint32_t*)f->esp);
  verify_multiple((uint8_t*)args, 4);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  switch(args[0]) {
    case SYS_HALT:
      shutdown_power_off();
      break;

    case SYS_EXIT:
      f->eax = args[1];
      printf("%s: exit(%d)\n", thread_current()->pcb->process_name, args[1]);
      process_exit();
      break;
    
    case SYS_EXEC:
      verify_str((char*)args[1]);
      f->eax = process_execute((char*)args[1]);
      break;

    case SYS_WAIT:
      process_wait(args[1]);
      break;

    case SYS_PRACTICE:
      f->eax = args[1] + 1;
      break;
  
    case SYS_WRITE:
        /* fd 1 is console. */
        switch(args[1]) {
          case 1:         
            putbuf((char*)args[2], args[3]);
            break;
          default:
            break;
        }

    default:
      break;
  }
}
