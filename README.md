CS 162 Group Repository
=======================

This repository contains code for CS 162 group projects.


## Design for Project 1: User Programs

### Design for wait() syscall.
wait must fail and return -1 immediately if any of the following conditions are true:

1. pid does not refer to a direct child of the calling process.  
So we need a way to find childs! Maybe can store child's pids in pcb.  

```c
/*process.c:*/
25 struct process {
26   /* Owned by process.c. */
27   uint32_t* pagedir;          /* Page directory. */
28   char process_name[16];      /* Name of the main thread */
29   struct thread* main_thread; /* Pointer to main thread */
     pid_t* childs;              // <- maybe append child's pid here when call process_execute() 
30 }
```


2. The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.  
I think the lock(or disable intr) is what we need.
