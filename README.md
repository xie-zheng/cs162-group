CS 162 Group Repository
=======================

This repository contains code for CS 162 group projects.


## Design for Project 1: User Programs

### Design for wait() syscall.
wait must fail and return -1 immediately if any of the following conditions are true:

1. pid does not refer to a direct child of the calling process.

So we need a way to find childrens!


2. The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.

I think the lock is what we need.
