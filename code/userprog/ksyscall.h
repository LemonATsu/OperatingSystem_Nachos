/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

// Record --------------------------------------------------------
// 2015/10/1 : Implement SysPrintInt() to do console int output.
// 2015/10/4 : Implement SysOpen() 
// 2015/10/4 : Implement SysWrite() 
// 2015/10/4 : Implement SysClose() 
// 2015/10/4 : Implement SysRead() 
// 2015/10/8 : modify PrintInt flow
// end Record ----------------------------------------------------

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"

#include "synchconsole.h"



void SysHalt()
{
  kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}

OpenFileId SysOpen(char *filename)
{
    return kernel->interrupt->OpenFile(filename);
}

int SysWrite(char *buffer, int size, OpenFileId id) 
{
    return kernel->interrupt->WriteToFileId(buffer, size, id);
}

int SysRead(char *buffer, int size, OpenFileId id)
{
    return kernel->interrupt->ReadFromFileId(buffer, size, id);
}

int SysClose(OpenFileId id)
{
    return kernel->interrupt->CloseFileId(id);
}

void SysPrintInt(int number)
{
    kernel->interrupt->PrintInt(number);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
