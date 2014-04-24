// thread.cc 
//	Routines to manage threads.  There are four main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef	// this is put at the top of the
					// execution stack, for detecting 
					// stack overflows

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------

Thread::Thread(char* threadName)
{
    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif
    //
    threadID = -1;
    for(int i = 0; i < MaxThreadNum; i++)
      {
	if(tid[i] == 0)
	  {
	    threadID = i;
	    tid[i] = 1;
	    break;
	  }
	else continue;
      }
    priority = NORM_PRIORITY;
    interrupted = false;
    joinList = new List;
    timeslice = InitialSlice;
    timeleft = timeslice;
    level = 0;
    usrID = threadID;
    threadnum++;
    threadPort[threadnum] = this;
   // DEBUG('t',"Create thread name = %s, ID = %d\n",name,threadID);
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    //added by ChenYang,2014.03.01
    freeThreadID(this->threadID);
    threadPort[threadnum] = NULL;
    threadnum--;
    
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single integer argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------
//Changed in 2014.03.01 by ChenYang
void 
Thread::Fork(VoidFunctionPtr func, int arg)
{
  /// this->threadID = findFreeThreadID();//this is the part that I added
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
	  name, (int) func, arg);
    ///    ASSERT(this->threadID != -1);
    if(this->threadID == -1)
      {
	threadnum--;
	
	printf("Failed to forking thread ,the tid_port is full\n");
	//DEBUG('t',"Failed to forking thread for the tid port is full\n");
	delete this;
	return;
      }
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts
    
    ////----debug information-----------------------------------------------------------------///
    Thread* oldthread = currentThread;
   // printf("The value of ShouldISwitch is %d------------- \n ",scheduler->ShouldISwitch(currentThread,this));
    if(scheduler->ShouldISwitch(currentThread ,this))
      {
	// printf("reached here!");
		//printf("The Thread %s was preempted by thread %s with priority %d\n",oldthread->getName(),this->getName(),this->getPriority());
	currentThread->Yield();
	//	scheduler->ReadyToRun(oldthread);

	//scheduler->Run(this);
      }
    //--------------------------------------------------------------------------//
    
   				// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow 
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT((int) *stack == (int) STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we don't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we set "threadToBeDestroyed", 
//	so that Scheduler::Run() will call the destructor, once we're
//	running in the context of a different thread.
//
// 	NOTE: we disable interrupts, so that we don't get a time slice 
//	between setting threadToBeDestroyed, and going to sleep.
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\", %d\n", getName(),this->getThreadID());
    
    threadToBeDestroyed = currentThread;
    Sleep();					// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::Yield
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    
    DEBUG('t', "Yielding thread \"%s\"\n", getName());

    //-----this is the RR part-------------------------//
    if(scheduler->getPolicy() == SCHED_RR)
      {
	int t = stats->totalTicks-scheduler->getlastSwitchTime();
	scheduler->setlastSwitchTime(stats->totalTicks);
	nextThread = scheduler->FindNextToRun();
	this->setTimeLeft(timeleft-t/10);
	if(nextThread!=NULL)
	  {
	    scheduler->ReadyToRun(this);
	    scheduler->Run(nextThread);
	  }
	
      }
    //----------------------------------------------------//
    
    // int tmp = this->getPriority() - (stats->totalTicks - scheduler->getLastScheduleTime());
    
    // this->setPriority(tmp);
    ///---------------------------------------------------------------////
    //this is the priority part---------//
    else if(scheduler->getPolicy() == SCHED_PRIO_P)
      {
    int tmppriority=this->getPriority();
    this->setPriority(tmppriority-this->MAX_PRIORITY+this->getThreadID());
    // scheduler->Print();printf("\n");
    nextThread = scheduler->FindNextToRun();
    //-----------------------------------------------------------------------------------//
    /*  ////this is the debugging part------------
    printf("The next to run is ");
    if(nextThread!=NULL)
      {	
    nextThread->Print();
      }
    elss
      
	printf("NULL");
    
    printf("\n");
    
////-----------------------------------------------------------------------------------///
*/
    
     if (nextThread != NULL) {
       
     
      int nextPriority = nextThread->getPriority();
      if(nextPriority >=(this->getPriority()))
	{
	  
	scheduler->ReadyToRun(this);
	scheduler->Run(nextThread);
	}
      else
	{
	  scheduler->ReadyToRun(nextThread);
	}
     }
      }
    ///-----------------------------------------/////
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread is blocked
//	waiting on a synchronization variable (Semaphore, Lock, or Condition).
//	Eventually, some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
Thread::Sleep ()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
	interrupt->Idle();	// no one to run, wait for an interrupt
        
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	Dummy functions because C++ does not allow a pointer to a member
//	function.  So in order to do this, we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState] = (int) ThreadRoot;
    machineState[StartupPCState] = (int) InterruptEnable;
    machineState[InitialPCState] = (int) func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, userRegisters[i]);
}
#endif

//----------------------------------------------------------------------
// Thread::setPriority
// Sets thread priority. Note that this method causes a context switch
// if the thread is in the ready queue and the new priority of the thread 
// is the highest in the ready queue. Note that the thread (whose priority
// is being set) need not be in the ready queue. It might just have been
// created, might be blocked, suspended etc.
//----------------------------------------------------------------------
//void Thread::setPriority(int prior) {
//  ASSERT( false );
//}  

//----------------------------------------------------------------------
// NEW
// Interrupts this thread. 
//----------------------------------------------------------------------
void Thread::Interrupt() {
  interrupted = true;
}

//----------------------------------------------------------------------
// NEW
// Tests if the current thread has been interrupted. 
//----------------------------------------------------------------------
bool Thread::Interrupted() {
  bool result = interrupted;
  interrupted = false;
  return result;
}

//------------------------------------------------------------------------
//  NEW
//  NOTE: Only other threads may do a join on this thread.
//
//------------------------------------------------------------------------
void
Thread::Join()
{
    Thread *nextThread;

    ASSERT (this != currentThread);
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    currentThread->setStatus(BLOCKED);
    joinList->Append(currentThread);

    while ((nextThread = scheduler->FindNextToRun()) == NULL)
      interrupt->Idle();       // No thread to run, wait for an interrupt,
                               // the only way to enabled threads to execute
    scheduler->Run(nextThread);

    (void) interrupt->SetLevel(oldLevel);
}

//------------------------------------------------------------------------
//  Thread::Suspend()
//      
//------------------------------------------------------------------------
void Thread::Suspend() {
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  
  printf("Student's suspend routine called\n");
  //  ASSERT(this != currentThread);

  DEBUG('t', "Thread \"%s\" suspending thread \"%s\"\n", currentThread->getName(), getName());

  // Inform the scheduler which maintains the ready and suspended 
  // list of threads to suspend the thread.
  scheduler->Suspend(this);

  (void) interrupt->SetLevel(oldLevel);
}
 
//------------------------------------------------------------------------
//  NEW
//  NOTE: Only other threads may suspend this thread.
//
//------------------------------------------------------------------------
void Thread::Resume() {
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  
  ASSERT(this != currentThread);

  DEBUG('t', "Thread \"%s\" resuming thread \"%s\"\n", currentThread->getName(), getName());

  // Inform the scheduler which maintains the ready and suspended 
  // list of threads to suspend the thread.
  scheduler->Resume(this);

  (void) interrupt->SetLevel(oldLevel);  
}
