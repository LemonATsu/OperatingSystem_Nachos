// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
#define AGING 10
#define REINSERT(LIST, t) LIST ## _ReadyList->Remove(t); LIST ## _ReadyList->Insert(t)
#define REAPPEND(LIST, t) LIST ## _ReadyList->Remove(t); LIST ## _ReadyList->Append(t)

int SJFCompare(Thread *a, Thread *b) 
{
    double ta = a->getBurstTime(),
        tb = b->getBurstTime();

    if (ta == tb)
        return 0;
    return ta > tb ? 1 : -1;
}

int PJCompare(Thread *a, Thread *b)
{
    int pa = a->getPriority(),
        pb = a->getPriority();

    if(pa == pb)
        return 0;
    return pa > pb ? -1 : 1;
}



//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    intHandler = new SchedulerIntHandler();
    readyList = new List<Thread *>; 
    SJF_ReadyList = new SortedList<Thread *>(SJFCompare);
    PJ_ReadyList = new SortedList<Thread *>(PJCompare);
    RR_ReadyList = new List<Thread *>;
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
    delete SJF_ReadyList;
    delete PJ_ReadyList;
    delete RR_ReadyList;
} 

//----------------------------------------------------------------------
// modified at 2015/12/02
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    //DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
    int currentTime = kernel->stats->totalTicks;
    int p = thread->getPriority();
    thread->setStatus(READY);
    thread->setReadyTime(currentTime);
    
    // Insert :: put item into list by order
    // Append :: put item at tail of list
    if(p >= 100) {
        // L1 queue
        InsertToQueue(thread, 1);
        // Preemptive
        if(thread->getBurstTime() < kernel->currentThread->getBurstTime()) {
            intHandler->Schedule(1);
        }
    } else if (p >= 50) {
        // L2 queue
        InsertToQueue(thread, 2);
    } else {
        // L3 queue
        InsertToQueue(thread, 3);
    }
}

//----------------------------------------------------------------------
// modified at 2015/12/02
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    int currentTime = kernel->stats->totalTicks;
    Thread* t = NULL;
    Aging(SJF_ReadyList);
    Aging(PJ_ReadyList);
    Aging(RR_ReadyList);

    if(!(SJF_ReadyList->IsEmpty())) {

        t = SJF_ReadyList->RemoveFront();
        RemoveLog(currentTime, t->getID(), 1);
    
    } else if(!(PJ_ReadyList->IsEmpty())) {
    
        t = PJ_ReadyList->RemoveFront();
        RemoveLog(currentTime, t->getID(), 2);
    
    } else if(!RR_ReadyList->IsEmpty()) {
        
        t = RR_ReadyList->RemoveFront();
        RemoveLog(currentTime, t->getID(), 3);

    }

    return t;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    // predict next burst time.
    int currentTime = kernel->stats->totalTicks;
    int executionTime = currentTime - oldThread->getStartTime();
    double newburst = executionTime / 2 + oldThread->getBurstTime() / 2;
    oldThread->setBurstTime(newburst);
    nextThread->setStartTime(currentTime); // set StartTime
    
    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    SwitchLog(currentTime, nextThread->getID(), oldThread->getID(), executionTime); 
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

//----------------------------------------------------------------------
// Scheduler::Aging
// 	Check all the thread in list, if they wait more than 1500 ticks,
//	increase their priority.
//----------------------------------------------------------------------
void
Scheduler::Aging(List<Thread *> *list)
{
    ListIterator<Thread *> *iterator = new ListIterator<Thread *>(list);
    for(; !(iterator->IsDone()); iterator->Next()) {
        int currentTime = kernel->stats->totalTicks;
        Thread* t = iterator->Item();
        
        if((currentTime - t->getReadyTime()) >= 1500) {
            int old = t->getPriority();
            t->Aging(AGING);
            t->setReadyTime(currentTime); // reset time ticks.
            PriorityChangeLog(currentTime, t->getID(), old, t->getPriority());
            CheckAndMove(t, old);
        }       
    }
}


//----------------------------------------------------------------------
// Scheduler::InsertToQueue
// Insert to a specific list and output the insertion infomation.
//----------------------------------------------------------------------
void
Scheduler::InsertToQueue(Thread* t, int level)
{
    int currentTime = kernel->stats->totalTicks;
    if(level == 1) {
        SJF_ReadyList->Insert(t);
    } else if(level == 2) {
        PJ_ReadyList->Insert(t);
    } else if(level == 3) {
        RR_ReadyList->Append(t);
    }
    InsertLog(currentTime, t->getID(), level);
}

//----------------------------------------------------------------------
// Scheduler::RemoveFromQueue
// Remove from a specific list and output the remove infomation.
//----------------------------------------------------------------------
void
Scheduler::RemoveFromQueue(Thread* t, int level)
{
    int currentTime = kernel->stats->totalTicks;
    if(level == 1) {
        SJF_ReadyList->Remove(t);
    } else if(level == 2) {
        PJ_ReadyList->Remove(t);
    } else if(level == 3) {
        RR_ReadyList->Remove(t);
    }
    RemoveLog(currentTime, t->getID(), level);
}

//----------------------------------------------------------------------
// Scheduler::CheckAndMove
// Check if a thread need to move to other queue after changing priority
//----------------------------------------------------------------------
void 
Scheduler::CheckAndMove(Thread* t, int oldPriority)
{
    int p = t->getPriority();
    
    if(p >= 100) {
        if(oldPriority < 50) {
            // t is in RR originally.
            RemoveFromQueue(t, 3);
        } else if(oldPriority < 100){
            // t is in PJ.
            RemoveFromQueue(t, 2);
        } else {
            // re-insert it to renew its position.
            REINSERT(SJF, t);
            return;
        }
        InsertToQueue(t, 1);
    } else if(p >= 50) {
        if(oldPriority < 50) {
            // if origin t is in RR
            RemoveFromQueue(t, 3);
        } else if(oldPriority > 99) {
            // origin t is in SJF
            RemoveFromQueue(t, 1);
        } else {
            REINSERT(PJ, t);
            return;
        }
        InsertToQueue(t, 2);
    } else {
        if(oldPriority > 99) {
            // origin is in SJF, but now move to RR
            RemoveFromQueue(t, 1);
        } else if(oldPriority > 49) {
            // origin is in PJ, but now move to RR
            RemoveFromQueue(t, 2);
        } else {
            REAPPEND(RR, t);
            return;
        }
        InsertToQueue(t, 3);
    }
}

//----------------------------------------------------------------------
// Scheduler::InsertLog
// Obvious.
//----------------------------------------------------------------------
void
Scheduler::InsertLog(int time, int tid, int level) 
{
    cout << "Tick " << time << ": Thread " << tid << " is inserted into queue L" << level << endl;
}

//----------------------------------------------------------------------
// Scheduler::RemoveLog
// Very obvious.
//----------------------------------------------------------------------
void
Scheduler::RemoveLog(int time, int tid, int level)
{
    cout << "Tick " << time << ": Thread " << tid << " is removed from queue L" << level << endl;
}

//----------------------------------------------------------------------
// Scheduler::SwitchLog
// Too obvious.
//----------------------------------------------------------------------
void
Scheduler::SwitchLog(int time, int nid, int pid, int executed) 
{
    cout << "Tick " << time << ": Thread " << nid << " is now selected for execution" << endl;
    cout << "Tick " << time << ": Thread " << pid << " is replaced, and it has executed " << executed << " ticks" << endl;
}

//----------------------------------------------------------------------
// Scheduler::PriorityChangeLog
// I AM FABULOUS
//----------------------------------------------------------------------
void
Scheduler::PriorityChangeLog(int time, int tid, int old, int now)
{
    cout << "Tick " << time << ": Thread " << tid << " changes its priority from "<< old << " to " << now << endl;
}

void
SchedulerIntHandler::CallBack()
{
    kernel->interrupt->YieldOnReturn();
}
void
SchedulerIntHandler::Schedule(int time)
{
    kernel->interrupt->Schedule(this, time, TimerInt);
}

