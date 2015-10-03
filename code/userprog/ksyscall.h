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

void SysPrintInt(int number)
{
    // sign = 1 : negative
    int sign = number >= 0 ? 0 : 1; 
    int len = 0, copy_len = 0;
    char *str = new char[sizeof(int) * 8 + 2]; // parse Int to in a reverse string,
    char *rev = new char[sizeof(int) * 8 + 2]; // reverse the string back to normal int format.
    
    // 0, print it directly
    if(number == 0) {
        kernel->synchConsoleOut->PrintString("0\n\0", 3);
        return; 
    }
    number = number < 0 ? -number : number;
    
    // itoa, but the result is a reverse int string.
    while(number > 0) {
        str[len++] = (number % 10) + '0';
        number /=10;
    }


    // check if it is negative
    if(sign)
        rev[copy_len++] = '-';

    // reverse it back to normal string
    for(len = len - 1; len >= 0; len--) {
        rev[copy_len++] = str[len];
    }

    // add newline and terminate character
    rev[copy_len++] = '\n';
    rev[copy_len++] = '\0';

    // console part
    kernel->synchConsoleOut->PrintString(rev, copy_len);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
