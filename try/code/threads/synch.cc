// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!


//-----------------------------------------------------//
//This is the implementation of Lock
//we added locker as the private variable
//-----------------------------------------//
Lock::Lock(char* debugName)
{
  name=debugName;
  locked = new Semaphore(name,1);
  //wait_queue = new List();
  holder = NULL;
}

Lock::~Lock() 
{
    //delete waitlock_queue;
}
//----------------------------------------//
//Acquire a lock ,if not ,busy waiting
//-----------------------//
void Lock::Acquire()
{
  IntStatus  oldLevel = interrupt->SetLevel(IntOff);
  locked->P();
  holder = currentThread;

  (void)interrupt->SetLevel(oldLevel);

}
void Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(currentThread == holder);
    holder = NULL;
    locked->V();
    (void)interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
    if(holder == currentThread)return TRUE;
    else return FALSE;
}
//------------------------------------------------------//
//----------------------------------------------//
//this is the condition implementation part
//
//---------------------------------------------//
Condition::Condition(char* debugName) 
{
    name = debugName;
    lock = NULL;
    wait_cond_queue = new List();
}
Condition::~Condition() 
{delete wait_cond_queue; }

void Condition::Wait(Lock* conditionLock) 
{ 
    //ASSERT(FALSE); 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    if(wait_cond_queue->IsEmpty())
    {
        lock = conditionLock;
    }
    ASSERT(lock == conditionLock);
    wait_cond_queue->Append(currentThread);                             //当前线程添加到该条件变量的等待队列中
    conditionLock->Release();                                        //释放当前锁，使得其他线程可以进入临界区
    currentThread->Sleep();                                            //当前线程进入等待状态
    conditionLock->Acquire();                                         //当其他线程发送一个信号后，当前线程wake up，并获得锁
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock) 
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!wait_cond_queue->IsEmpty())
    {

        ASSERT(lock == conditionLock);
        nextThread = (Thread*)wait_cond_queue->Remove();
        scheduler->ReadyToRun(nextThread);
    }
    (void*)interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock) 
{
    Thread* nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    while(!wait_cond_queue->IsEmpty())
    {
        Signal(conditionLock);
    }
    (void*)interrupt->SetLevel(oldLevel);
 }



//------------------------------
//The next is the Read_Write Lock Part.
//
//-------------------------------------------------

RWLock::RWLock()
{
    pendingReaders = 0;
    pendingWriters = 0;
    activeReaders = 0;
    mutex = new Lock("mutex");
    conReader = new Condition("conReader");
    conWriter = new Condition("conWriter");
}
RWLock::~RWLock()
{
    delete mutex;delete conReader;delete conWriter;
}
void RWLock::readlock()
{
    mutex->Acquire();
    pendingReaders++;
     printf("reader %s %d is trying to read \n",currentThread->getName(),currentThread->getThreadID());
  
    while(activeReaders < 0)conReader->Wait(mutex);
    pendingReaders--;
    activeReaders++;
    mutex->Release();
}

void RWLock::writelock()
{
    mutex->Acquire();
    pendingWriters++;
     printf("writer %s %d is trying to write  \n",currentThread->getName(),currentThread->getThreadID());

    while(activeReaders!=0)conWriter->Wait(mutex);
    pendingWriters--;
    activeReaders = -1;
    mutex->Release();
}

void RWLock::unlock()
{
    mutex->Acquire();
    if(activeReaders < 0)
    {
        activeReaders = 0;
        printf("writer %s %d is leaving the critical region\n",currentThread->getName(),currentThread->getThreadID());
        if(pendingReaders > 0)conReader->Broadcast(mutex);
        else if(pendingReaders == 0 && pendingWriters > 0)conWriter->Signal(mutex);
    }
    else if(activeReaders >0)
    {
        activeReaders--;
         printf("reader %s %d is leaving the critical region\n",currentThread->getName(),currentThread->getThreadID());
        if(pendingReaders==0 && activeReaders ==0)conWriter->Signal(mutex);
    }
    mutex->Release();
}

//--------------------------------------------------
//Barrier definetion
//
//
//--------------------------------------------------------
int total = 0;
int memnum[20]={0};
int memid[20] = {0};
int cnt = 0;
void BarrierProcess(int num)
{
    if(num == -1)
    {
         printf("total %d threads reached at the Barrier-------\n",cnt);
         printf("the threads put all the numbers:");
         printf("%d",memnum[0]);
        for(int i = 1;i< cnt;i++)
        {
            printf("+%d",memnum[i]);
        }
        printf("=%d\n",total);
        cnt = 0;for(int i = 0;i<20;i++)
        {
            memnum[i]= 0;
            memid[i]=0;
        }
        total = 0;
    }
    
    else
    {

        total+=num;
        memid[cnt]=currentThread->getThreadID();
        memnum[cnt] = num;
        cnt ++;
    }
}
Barrier::Barrier()
{
    barrierNum = 0;
    remaining = barrierNum;
    processFunction = NULL;
    cond = new Condition("condition");
    lock = new Lock("locker");
}

void Barrier::enterBarrier()
{
    lock->Acquire();
    remaining--;
    while(remaining>0)cond->Wait(lock);
    
    if(processFunction!=NULL)
    {
        (*processFunction)(-1);
    }
    processFunction = NULL;
    cond->Broadcast(lock);
    lock->Release();

}
