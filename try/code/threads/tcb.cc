//This is the tcb define part which gives the concrete definition and implement of the tcb part
#include "system.h"
#include "tcb.h"
#include "copyright.h"
#include "thread.h"

Tcb::Tcb()
{
  tcbList = new List;
}


Tcb::~Tcb()
{
  delete tcbList;
}

void Tcb::FindNextThreadID()
{
  
}
