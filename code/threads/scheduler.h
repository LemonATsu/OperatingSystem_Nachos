// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

// 15/12/01: add 3 level queue 


#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();		// Initialize list of ready threads 
    ~Scheduler();		// De-allocate ready list

    void ReadyToRun(Thread* thread);	
    				// Thread can be dispatched.
    Thread* FindNextToRun();	// Dequeue first thread on the ready 
				// list, if any, and return thread.
    void Run(Thread* nextThread, bool finishing);
    				// Cause nextThread to start running
    void CheckToBeDestroyed();// Check if thread that had been
    				// running needs to be deleted
    void Print();		// Print contents of ready list
    void CheckAndMove(Thread* t, int oldPriority);    
    // SelfTest for scheduler is implemented in class Thread
   
    void Aging(List<Thread *> *list);
    void InsertLog(int time, int tid, int level);
    void RemoveLog(int time, int tid, int level);
    void SwitchLog(int time, int nid, int pid, int executed);
    void PriorityChangeLog(int time, int tid, int old, int now);
  private:
    void InsertToQueue(Thread* t, int level);
    void RemoveFromQueue(Thread* t, int level);
    List<Thread *> *readyList;  // queue of threads that are ready to run,
				// but not running

    SortedList<Thread *> *SJF_ReadyList;
    SortedList<Thread *> *PJ_ReadyList;
    List<Thread *> *RR_ReadyList;

    Thread *toBeDestroyed;	// finishing thread to be destroyed
    				// by the next thread that runs
};

#endif // SCHEDULER_H
