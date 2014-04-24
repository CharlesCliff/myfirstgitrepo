//------------------------------------------------------------
//This is the test of synchonization
//
//Read_Write lock test is here and barrier test is here
//
//
//
//------------------------------------------


#include "synch.h"
#include "thread.h"
#include "system.h"
//--------------------------------------------------------------------
//synchtest part
// producer-consumer test using semaphore and  lock and condition
//
//semtest part
//--------------------------------------------------------------

#define N 10
Semaphore *sem_mutex;
Semaphore *sem_full;
Semaphore *sem_empty;
int full,empty;
void produce_item(){};
void consume_item(){};
void insert_item(){full++;empty--;}
void remove_item(){full--;empty++;}
void SemTest();

void Producer_sem(int which)
{
  for(int i = 0;i < 2;i++)
  {
    produce_item();
    sem_empty->P();
    sem_mutex->P();
    insert_item();
    printf("producer %d ,full is %d , empty is %d\n",currentThread->getThreadID(),full,empty );
    sem_mutex->V();
    sem_full->V();
    currentThread->Yield();
  }
}

void Consumer_sem(int which)
{
  for(int i = 0; i < 23 ;i++)
  {
    sem_full->P();
    sem_mutex->P();
    remove_item();
    printf("consumer %d ,full is %d , empty is %d time %d \n",currentThread->getThreadID(),full,empty,i );
    sem_mutex->V();
    sem_empty->V();
    consume_item();
    currentThread->Yield();
  }
}
void SemTest()
{
	printf("This is the semaphore test of producer-consumer problem!*******\n");
  sem_empty = new Semaphore("EMPTY",10);
  sem_full = new Semaphore("FULL",0);
  sem_mutex = new Semaphore("MUTEX",1);
  full = 0;
  empty = 10;
  Thread* t1[10];
  for(int i = 0;i < 10;i++)
  {
    t1[i] = new Thread("producer");
  }
  
  Thread *t2 = new Thread("consumer");
  Thread *t3 = new Thread("consumer");
  t2->Fork(Consumer_sem,t2->getThreadID());
  t3->Fork(Consumer_sem,t3->getThreadID());
  for(int i = 0; i < 10; i++)
  {
    t1[i]->Fork(Producer_sem,t1[i]->getThreadID());
  }
  
}
//-----------------------------------------------------------------------------//
//
//
//
//
//-----------------------------------------------------------------------//
#define MAX 15
Lock* cond_mutex;
Condition* prodc;
Condition* consc;
int buffer = 0;
#define FullBuffer 10
void Producer_cond(int which)
{
  for(int i = 1; i< MAX/7+1;i++)
  {
    cond_mutex->Acquire();
    while(buffer == FullBuffer)prodc->Wait(cond_mutex);
    buffer++;
    printf("producer %d produce %d  buffer is*********%d\n",currentThread->getThreadID(),i,buffer);
    consc->Signal(cond_mutex);
    cond_mutex->Release();
    currentThread->Yield();
  }
}
void Consumer_cond(int which)
{
  for(int i = 1; i< MAX*2;i++)
  {
    cond_mutex->Acquire();
    while(buffer == 0)consc->Wait(cond_mutex);
    buffer--;
    printf("consumer %d consume %d buffer is---------- %d\n",currentThread->getThreadID(),i,buffer);
    prodc->Signal(cond_mutex);
    cond_mutex->Release();
    currentThread->Yield();
  }
}

void CondTest()
{
	printf("This is the Condition Test  of the producer-consumer problem!-----\n");
  cond_mutex = new Lock("cond_mutex");
  prodc = new Condition("produce condition");
  consc = new Condition("consume condition");
  Thread* t1[10];
  for(int i = 0;i < 10;i++)
  {
    t1[i] = new Thread("producer");
  }
  
  Thread *t2 = new Thread("consumer");
  Thread *t3 = new Thread("consumer");
  t2->Fork(Consumer_cond,t2->getThreadID());
  t3->Fork(Consumer_cond,t3->getThreadID());
  for(int i = 0; i < 10; i++)
  {
    t1[i]->Fork(Producer_cond,t1[i]->getThreadID());
  }
  
}
//---------------------------------------------------------------------//
//
//
//
//--------------------------------------------------------------------//
RWLock* rwlock;
char* name;
void Reader(int which)
{
	for(int i = 0; i< 2 ; i++)
	{
	rwlock->readlock();

	printf("Reader %s %d entered the critical section reading---------  %s\n",currentThread->getName(),
		currentThread->getThreadID(),name);
	currentThread->Yield();
	rwlock->unlock();
	currentThread->Yield();
	}
}

void Writer(int which)
{
	for( int i = 0; i < 2;i++)
	{
	rwlock->writelock();
	name = currentThread->getName();
	printf("Writer %s %d entered the critical section writing---------- %s\n",currentThread->getName(),
		currentThread->getThreadID(),currentThread->getName());
	currentThread->Yield();
	rwlock->unlock();
	currentThread->Yield();
	}

}

void ReadWriteTest()
{
	printf("The Read-Write Lock Test Begins~~~~~~~~~~~\n");
	rwlock = new RWLock();
	name = "Hello I am Mr.Winner!";
	Thread* r1 = new Thread("Reader_1");
	Thread* r2 = new Thread("Reader_2");
	Thread* w1 = new Thread("Writer_1");
	Thread* w2 = new Thread("Writer_2");
	Thread* r3 = new Thread("Reader_3");
	Thread* r4 = new Thread("Reader_4");
	Thread* r5 = new Thread("Reader_5");
	Thread* w3 = new Thread("Writer_3");
	Thread* w4 = new Thread("Writer_4");
	Thread* w5 = new Thread("Writer_5");
	Thread* w6 = new Thread("Writer_6");
	w1->Fork(Writer,w1->getThreadID());
	w2->Fork(Writer,w2->getThreadID());
	w3->Fork(Writer,w3->getThreadID());
	//w4->Fork(Writer,w4->getThreadID());
	//w5->Fork(Writer,w5->getThreadID());
	//w6->Fork(Writer,w6->getThreadID());

	r1->Fork(Reader,r1->getThreadID());
	r2->Fork(Reader,r2->getThreadID());
	r3->Fork(Reader,r3->getThreadID());
	//r4->Fork(Reader,r4->getThreadID());
	//r5->Fork(Reader,r5->getThreadID());
	
}

//--------------------------------------------------------------------//
//
//
//
//-------------------------------------------------------------------//
Barrier *barrier;
extern void BarrierProcess(int num);
void BarrierFunction(int which)
{
	printf("thread_%d is putting number %d\n",currentThread->getThreadID(),2*which);
	BarrierProcess(2*which);
	barrier->enterBarrier();
	printf("thread_%d crossed the barrier\n");
}


void BarrierTest()
{
	printf("The Barrier Test started !+++++++++++++++++++++\n");
	Thread *b[9];
	barrier = new Barrier();
	barrier->setProcessFunction((VoidFunctionPtr)BarrierProcess);
	barrier->setBarrierNum(9);
	for(int i = 0;i< 9;i++)
	{
		b[i] = new Thread("BarrierThread");
		
	}
	for(int i = 0; i < 9 ; i ++)
	{
		b[i]->Fork(BarrierFunction,b[i]->getThreadID());
	}
}
