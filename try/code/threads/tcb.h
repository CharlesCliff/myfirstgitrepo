///This is the thread control block which manages the whole thread exist in the system
///we handle the Thread in a list which is somewhat like the readyList in scheduler
///
#include "list.h"
#include "thread.h"
class Tcb
{
 private:
  List *tcbList;
 public:
  Tcb();
  ~Tcb();
  void FindNextThreaID();
  void FreeThread(int ThreadID);
  
}
