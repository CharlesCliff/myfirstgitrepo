// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "synchtest.cc"
// testnum is set in main.cc
int testnum = 1;


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
void SimpleThread(int which);
void
SimpleThread2(int id)
{
  Thread* t[6];
  for(int i = 0; i<6; i++)
    {
      t[i]=new Thread( "fork thread");
      printf("Creating thread %d\n",t[i]->getThreadID());
      t[i]->Fork(SimpleThread,t[i]->getThreadID());
    }
   
  return;
}

void
SimpleThread3(int which)
{
    int num;
     
    
    for (num = 0; num < 3; num++) {
      printf("*** thread %d name \"%s\" looped %d times %d priority\n", currentThread->getThreadID(),currentThread->getName(), num,currentThread->getPriority());
      
      currentThread->Yield();
    }
     
    return;
}

//---------------------------------------------------------------//
void
SimpleThread(int which)
{
    int num;
     
    
    for (num = 0; num < 3; num++) {
      printf("*** thread %d name \"%s\" looped %d times %d priority\n", currentThread->getThreadID(),currentThread->getName(), num,currentThread->getPriority());
      
	if(num == 1)
	  {
	    Thread *t4 = new Thread("newly thread 4");
      t4->setPriority(10+num);
      t4->Fork(SimpleThread3,t4->getThreadID());
	  }
      
      currentThread->Yield();
    }
     
    return;
}


//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}



//-------------------------------------------------------------------
//ThreadTest2
//     test 128 thread to see if the thread boundary sets right
//-------------------------------------------------------------------
void ThreadTest2()
{
  //---------------------------------------------------------//
  Thread *t1 = new Thread("forked thread1");
  Thread *t2 = new Thread("forked thread2");
  Thread *t3 = new Thread("forked thread3");
  // t1->setPriority(80);
  //t2->setPriority(20);
  //t3->setPriority(50);
  t1->Fork(SimpleThread,t1->getThreadID());
  t2->Fork(SimpleThread,t2->getThreadID());
  t3->Fork(SimpleThread,t3->getThreadID());
  SimpleThread(0);
  //----------------------------------------------------------------//
 /*
  Thread* t[6];
  for(int i = 0; i<6; i++)
    {
      t[i]=new Thread( "fork thread");
      printf("Creating thread %d\n",t[i]->getThreadID());
      t[i]->Fork(SimpleThread2,t[i]->getThreadID());
    }

  SimpleThread2(0);

  *///----------------------------///
  ///TestStatus
  printf("next is the test of command TS(test status),it can be bold or plain ");
  printf("Please give the command:\n");
  while(true)
    {
  char command[10]= {0};
  scanf("%s",command);
  // printf("success command!\n");
  printf("the input was %s\n",command);

  if((command[0]=='T'||command[0]=='t')&&(command[1]=='S'||command[1]=='s')&&command[2]=='\0')
    { ThreadStatus();break;}
  else
    {
      printf("the command is not defined. Please try again\n"); 
      continue;
    }
    }
  
  return;
}

//--------------------------------------------------------------------
//SCHED_RR test
void OneTickTock();
void
ScheduleRoundRobin(int which)
{
  for(int i = 0;i<10 ;i++)
    {
      printf("tick for %s %d  %d  times, left %d\n",currentThread->getName(),currentThread->getThreadID(),i,currentThread->getTimeLeft());
      OneTickTock();
    }
}
  void
ThreadScheduleTest()
{
  Thread* t1 = new Thread("forking RR_thread1");
  Thread* t2 = new Thread("forking RR_thread2");
  
  t1->Fork(ScheduleRoundRobin,t1->getThreadID());
  t2->Fork(ScheduleRoundRobin,t2->getThreadID());
  ScheduleRoundRobin(0);
}




//----------------------------------------------------------------------------

//----------------------------------------------------------------------
// ThreadTest
//  Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
  ThreadTest1();
  break;
    case 2:
      ThreadScheduleTest(); // ThreadTest2();
      break;
      case 3:
      SemTest();
      break;
      case 4:
      CondTest();
      break;
      case 5:
      ReadWriteTest();
      break;
      case 6:
      BarrierTest();
      break;
    default:
  printf("No test specified.\n");
  break;
    }
}


void OneTickTock()
{
  interrupt->SetLevel(IntOff);
  interrupt->SetLevel(IntOn);
  return;
}
