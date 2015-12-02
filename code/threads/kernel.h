// kernel.h
//	Global variables for the Nachos kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

// Record --------------------------------------------------------
// 2015/10/4 : define OpenFile(char *filename) 
// 2015/10/4 : define WriteToFileId(char *buffer, int size, OpenFileId id) 
// 2015/10/4 : define CloseFileId(OpenFileId id)
// 2015/10/4 : define ReadFromFileId(char *buffer, int size, OpenFileId id) 
// 2015/10/13: modify PrintInt flow again
// 2015/12/02: modify Exec to take priority as arg
// end Record ----------------------------------------------------

#ifndef KERNEL_H
#define KERNEL_H

#include "copyright.h"
#include "debug.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "alarm.h"
#include "filesys.h"
#include "machine.h"

class PostOfficeInput;
class PostOfficeOutput;
class SynchConsoleInput;
class SynchConsoleOutput;
class SynchDisk;



class Kernel {
  public:
    Kernel(int argc, char **argv);
    				// Interpret command line arguments
    ~Kernel();		        // deallocate the kernel
    
    void Initialize(); 		// initialize the kernel -- separated
				// from constructor because 
				// refers to "kernel" as a global
	void ExecAll();
	int Exec(char* name, int priority);
    void ThreadSelfTest();	// self test of threads and synchronization
	
    void ConsoleTest();         // interactive console self test
    void NetworkTest();         // interactive 2-machine network test
	Thread* getThread(int threadID){return t[threadID];}    
	
	int CreateFile(char* filename); // fileSystem call
    OpenFileId OpenFile(char *filename);
    int WriteToFileId(char *buffer, int size, OpenFileId id);
    int ReadFromFileId(char *buffer, int size, OpenFileId id);
    int CloseFileId(OpenFileId id);
// These are public for notational convenience; really, 
// they're global variables used everywhere.

    Thread *currentThread;	// the thread holding the CPU
    Scheduler *scheduler;	// the ready list
    Interrupt *interrupt;	// interrupt status
    Statistics *stats;		// performance metrics
    Alarm *alarm;		// the software alarm clock    
    Machine *machine;           // the simulated CPU
    SynchConsoleInput *synchConsoleIn;
    SynchConsoleOutput *synchConsoleOut;
    SynchDisk *synchDisk;
    FileSystem *fileSystem;     
    PostOfficeInput *postOfficeIn;
    PostOfficeOutput *postOfficeOut;

    int hostName;               // machine identifier
    void PrintInt(int number);
  private:

	Thread* t[10];
	char*   execfile[10];
    int execpriority[10];
	int execfileNum;
	int threadNum;
    bool randomSlice;		// enable pseudo-random time slicing
    bool debugUserProg;         // single step user program
    double reliability;         // likelihood messages are dropped
    char *consoleIn;            // file to read console input from
    char *consoleOut;           // file to send console output to
#ifndef FILESYS_STUB
    bool formatFlag;          // format the disk if this is true
#endif
};


#endif // KERNEL_H


