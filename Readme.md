# CS162: Operating Systems and System Programming

This repository contains the course materials, hws, and projects for CS162. The course covers fundamental concepts in operating systems,
including process management, memory management, file systems, and concurrency.

## Course Structure
* Four Fundamental OS Concepts
  * Thread: Execution Context
  * Address Space
  * Process: an instance of a running program
  * Dual mode operation / Protection 
* Abstractions
  * Threads and Processes
  * High-Level IO and Low-Level IO
  * Pipes and Sockets
* Synchronization
  * Locks, Semaphores, Monitors(Readers/Writers Monitor example)
  * Hardware atomicity primitives
  * Producer-Consumer with a Bounded Buffer
* Scheduling
  * Round Robin Scheduling
  * Shortest Job First Scheduling / Shortest Remaining Time First Scheduling
  * Multilevel Feedback Queue Scheduling
  * Lottery Scheduling
  * Real-Time Scheduling
  * Linux CFS Scheduler: fair fraction of CPU
  * Scheduling goals: 
    * Minimize response time
    * Maximize throughput
    * Fairness
    * Predictability
  * Deadlock and Starvation
* Memory 
  * Segment Mapping, Page Tables, Multi-level Page Tables, Inverted Page Tables
  * Locality and Cache Organization
  * TLB, Demand Paging, Page Fault, Replacement Policies
  * Clock Algorithm, Nth-Chance clock Algorithm, Second-Chance List Algorithm
  * Working Set Model, Thrashing
* IO 
  * IO controllers and Device Drivers
  * Notification mechanisms: Polling, Interrupts
  * DMA
  * Disk Performance: Seek time, Rotational latency, Transfer time

File System
  * Queuing Latency and Queuing Theory
  * Inode and Naming 
  * File allocation table (FAT) Scheme
  * 4.2 BSD Fast File System: Multi-level inode header to describe files
  * NTFS 
  * map file or anonymous segment to memory(`mmap()`) and Buffer Cache
  * Durability and Consistency
    * RAID
    * Copy on Write
    * Transactional and Journaling File Systems
    * End-to-End arguments 
 * Distributed Decision Making
   * Two-Phase Commit Protocol
   * Byzantine General's Problem
 * Networking
   * Internet Protocol (IP)
   * DNS
   * TCP and UDP
 * Remote Procedure Call (RPC)
 * Distributed File Systems
   * NFS
   * AFS
   * Key-Value Store
     * Chord: Distributed Lookup (Directory) Service
    