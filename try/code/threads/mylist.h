// mylist.h 
//	Data structures to manage LISP-like lists.  
//
//      As in LISP, a list can contain any type of data structure
//	as an item on the list: thread control blocks, 
//	pending interrupts, etc.  That is why each item is a "void *",
//	or in other words, a "pointers to anything".
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef MYLIST_H
#define MYLIST_H

#include "copyright.h"
#include "utility.h"

// The following class defines a "list element" -- which is
// used to keep track of one item on a list.  It is equivalent to a
// LISP cell, with a "car" ("next") pointing to the next element on the list,
// and a "cdr" ("item") pointing to the item on the list.
//
// Internal data structures kept public so that List operations can
// access them directly.
template  <class T>
class MyListElement {
   public:
 MyListElement(T itemPtr, int sortKey);	// initialize a list element
  
  void setKey(int key);
  
     MyListElement *next;		// next element on list, 
				// NULL if this is the last
     int key;		    	// priority, for a sorted list
     T item; 	    	// pointer to item on the list
     
};

// The following class defines a "list" -- a singly linked list of
// list elements, each of which points to a single item on the list.
//
// By using the "Sorted" functions, the list can be kept in sorted
// in increasing order by "key" in ListElement.
template< class T>
class MyList {
  public:
    MyList();			// initialize the list
    ~MyList();			// de-allocate the list

    void Prepend(T item); 	// Put item at the beginning of the list
    void Append(T item); 	// Put item at the end of the list
    T Remove(); 	 	// Take item off the front of the list

    void Mapcar(VoidFunctionPtr func);	// Apply "func" to every element 
					// on the list
    bool IsEmpty();		// is the list empty? 
    

    // Routines to put/get items on/off list in order (sorted by key)
    void SortedInsert(T item, int sortKey);	// Put item into list
    T SortedRemove(int *keyPtr); 	  	// Remove first item from list
    T getFirst();
    T Peek();
    T SortedPeek(int *keyPtr);
  private:
    MyListElement<T> *first;  	// Head of the list, NULL if list is empty
    MyListElement<T> *last;		// Last element of list
};


// list.cc 
//
//     	Routines to manage a singly-linked list of "things".
//
// 	A "ListElement" is allocated for each item to be put on the
//	list; it is de-allocated when the item is removed. This means
//      we don't need to keep a "next" pointer in every object we
//      want to put on a list.
// 
//     	NOTE: Mutual exclusion must be provided by the caller.
//  	If you want a synchronized list, you must use the routines 
//	in synchlist.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.



//----------------------------------------------------------------------
// ListElement::ListElement
// 	Initialize a list element, so it can be added somewhere on a list.
//
//	"itemPtr" is the item to be put on the list.  It can be a pointer
//		to anything.
//	"sortKey" is the priority of the item, if any.
//----------------------------------------------------------------------
template <class T>
MyListElement<T>::MyListElement(T itemPtr, int sortKey)
{
     item = itemPtr;
     key = sortKey;
     next = NULL;	// assume we'll put it at the end of the list 
}

//----------------------------------------------------------------------
// List::List
//	Initialize a list, empty to start with.
//	Elements can now be added to the list.
//----------------------------------------------------------------------
template <class T>
MyList<T>::MyList()
{ 
    first = last = NULL; 
}

//----------------------------------------------------------------------
// List::~List
//	Prepare a list for deallocation.  If the list still contains any 
//	ListElements, de-allocate them.  However, note that we do *not*
//	de-allocate the "items" on the list -- this module allocates
//	and de-allocates the ListElements to keep track of each item,
//	but a given item may be on multiple lists, so we can't
//	de-allocate them here.
//----------------------------------------------------------------------
template <class T>
MyList<T>::~MyList()
{ 
    while (Remove() != NULL)
	;	 // delete all the list elements
}

//----------------------------------------------------------------------
// List::Append
//      Append an "item" to the end of the list.
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the end.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------
template <class T>
void
MyList<T>::Append(T item)
{
  MyListElement<T> *element = new MyListElement<T>(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it after last
	last->next = element;
	last = element;
    }
}

//----------------------------------------------------------------------
// List::Prepend
//      Put an "item" on the front of the list.
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the beginning.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------
template <class T>
void
MyList<T>::Prepend(T item)
{
  MyListElement<T> *element = new MyListElement<T>(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it before first
	element->next = first;
	first = element;
    }
}

//----------------------------------------------------------------------
// List::Remove
//      Remove the first "item" from the front of the list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//----------------------------------------------------------------------
template <class T>
T
MyList<T>::Remove()
{
    return SortedRemove(NULL);  // Same as SortedRemove, but ignore the key
}

template <class T>
T
MyList<T>::Peek()
{
  return SortedPeek(NULL); //Same as SortedPeek, but ignore the key
}
//----------------------------------------------------------------------
// List::Mapcar
//	Apply a function to each item on the list, by walking through  
//	the list, one element at a time.
//
//	Unlike LISP, this mapcar does not return anything!
//
//	"func" is the procedure to apply to each element of the list.
//----------------------------------------------------------------------
template <class T>
void
MyList<T>::Mapcar(VoidFunctionPtr func)
{
  for (MyListElement<T> *ptr = first; ptr != NULL; ptr = ptr->next) {
       DEBUG('l', "In mapcar, about to invoke %x(%x)\n", func, ptr->item);
       (*func)((int)ptr->item);
    }
}

//----------------------------------------------------------------------
// List::IsEmpty
//      Returns TRUE if the list is empty (has no items).
//----------------------------------------------------------------------
template <class T>
bool
MyList<T>::IsEmpty() 
{ 
    if (first == NULL)
        return TRUE;
    else
	return FALSE; 
}

//----------------------------------------------------------------------
// List::SortedInsert
//      Insert an "item" into a list, so that the list elements are
//	sorted in increasing order by "sortKey".
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, walk through the list, one element at a time,
//	to find where the new item should be placed.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//	"sortKey" is the priority of the item.
//----------------------------------------------------------------------
template <class T>
void
MyList<T>::SortedInsert( T item, int sortKey)///adapted
{
 MyListElement<T> *element = new MyListElement<T>(item, sortKey);
  MyListElement<T> *ptr;		// keep track

    if (IsEmpty()) {	// if list is empty, put
        first = element;
        last = element;
    } else if (sortKey > first->key) {	
		// item goes on front of list
	element->next = first;
	first = element;
    } else {		// look for first elt in list bigger than item
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (sortKey > ptr->next->key) {
		element->next = ptr->next;
	        ptr->next = element;
		return;
	    }
	}
	last->next = element;		// item goes at end of list
	last = element;
    }
}

//----------------------------------------------------------------------
// List::SortedRemove
//      Remove the first "item" from the front of a sorted list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//	Sets *keyPtr to the priority value of the removed item
//	(this is needed by interrupt.cc, for instance).
//
//	"keyPtr" is a pointer to the location in which to store the 
//		priority of the removed item.
//----------------------------------------------------------------------
template <class T>
T
MyList<T>::SortedRemove(int *keyPtr)
{
  MyListElement<T> *element = first;
    T thing;

    if (IsEmpty()) 
	return NULL;

    thing = first->item;
    if (first == last) {	// list had one item, now has none 
        first = NULL;
	last = NULL;
    } else {
        first = element->next;
    }
    if (keyPtr != NULL)
        *keyPtr = element->key;
    delete element;
    return thing;
}

template <class T>
T
MyList<T>::SortedPeek(int *keyPtr)
{
  MyListElement<T> *element = first;
  T thing;
  if(this->isEmpty())
    return NULL;
      thing = first->item;
  if(keyPtr!=NULL)
    *keyPtr = element->key;
  return thing;
}
// List::getFirst()
//{
//return (void*)this->first->item;
//}
template <class T>
void MyListElement<T>::setKey(int k)
{
  this->key = k;
}


#endif // MYLIST_H
