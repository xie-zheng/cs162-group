#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "lib/kernel/console.h"
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }


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


/* ----- hanlders for syscall ----- */

/* release the resources and exit */
static void sys_exit(int status UNUSED) {
  process_exit();
}

/* 
 * wait must fail and return -1 immediately if any of the following conditions are true:
 *
 * 1. pid does not refer to a direct child of the calling process. 
 *    pid is a direct child of the calling process if and only if 
 *    the calling process received pid as a return value from a successful call to exec. 
 *    Note that children are not inherited: if A spawns child B and B spawns child process C,
 *    then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail. 
 *    Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
 *
 * 2. The process that calls wait has already called wait on pid.
 *    That is, a process may wait for any given child at most once.
 */
int wait (pid_t pid) {
  // cond1
  return -1;
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
