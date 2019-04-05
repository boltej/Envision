//-------------------------------------------------------------------
// PROGRAM: TDLIST.HPP
//
// PURPOSE: TDList - Doubly link list of template objects
//
// COLLABORATORS:
//-------------------------------------------------------------------

#pragma once

#include "libs.h"

#include "Report.h"
#include "CONTAINR.HPP"



template < class T >
struct TDListNode                // structure of a node in the list
   {
   TDListNode< T > *pPrev;       // pointer to previous element
   TDListNode< T > *pNext;       // pointer to next element
   T data;                       // container object
   };


//------------------ TDList: Doubly linked list --------------------//

template < class T >
class TDList : public Container
   {
   protected:
      TDListNode< T > *pHead;          // pointer to the first node (head)
      TDListNode< T > *pTail;          // pointer to the last node (tail)
      TDListNode< T > *pCurrent;       // pointer to the current node

      void Copy( TDList & dl );   // duplication method

   public:
      //-- constructor --//
      TDList( void );

      //-- copy constructor --//
      TDList( TDList *dl );

      //-- destructor --//
      ~TDList( void );

      //-- accessors --//
      T  operator [] ( UINT pos ) { return Get( pos ); }
      T  operator () ( UINT pos ) { return Get( pos ); }

      T  Get( UINT pos );

      //-- assignment operator (sets target TDList = argument TDList) --//
      void operator = ( TDList &dl );

      //-- increment operator (move towards tail in target TDList) --//
      void operator ++ () { GoToNext(); }

      //-- decrement operator (move towards head in target TDList) --//
      void operator -- () { GoToPrev(); }

      //-- append operator (append a list to another list) --//
      void operator += ( TDList & dl );

      //-- insert an item in the list --//
      int Insert( T insertItem, T beforeItem );
      int Insert( T insertItem );

      //-- append an item to the end of a list (return error code) --//
      //-- 'current' doesn't change                                --//
      int Append( T item );

      //-- examine the current item (return item) --//
      T Examine( void ) { return ( pCurrent ? pCurrent->data : NULL ); }
      T Get    ( void ) { return ( pCurrent ? pCurrent->data : NULL ); }

      //-- replace an item in the list --//
      int Replace( T newItem, T oldItem );

      //-- delete an item from the list (set 'Current' to former next --//
      //-- item unless tail deleted, then to previous item).          --//
      int Delete( T item );
      int Delete( void );      // delete current item

      //-- empty the list (remove all items) --//
      void Empty( void );

      //-- move around in list --//
      TDListNode< T > *GoToHead( void ) { return pCurrent = pHead; }
      TDListNode< T > *GoToTail( void ) { return pCurrent = pTail; }
      TDListNode< T > *GoToItem( T _data );

      void GoToNext( void );     // same as ++
      void GoToPrev( void );     // same as --

      TDListNode< T > *GetHead(void) { return pHead; }

      int IsHead ( void );
      int IsTail ( void );
      int IsEmpty( void );

      TDListNode< T > *GetCurrent( void ) { return pCurrent; }
      TDListNode< T > *SetCurrent( TDListNode< T > *pNewCurrent )
         {
         TDListNode< T > *temp = pCurrent;
         pCurrent = pNewCurrent;
         return temp;
         }
   };



//--------------------------- TDList methods ----------------------//


//-- TDList::Copy()--------------------------------------------------
//
//-- internal copy duplication method
//
//-- Arguments:
//      dl = source list to be copied
//
//-- Return Value:
//      none
//-------------------------------------------------------------------

template < class T >
void TDList< T >::Copy( TDList< T > & dl )
   {
   pHead = NULL;
   pTail = NULL;
   TDListNode< T > *sNode = dl.pHead;       // get access to source list

   while ( sNode != NULL )                  // loop until end of source list
      {
      if (pHead == NULL)                    // empty target list?
         {
         pTail = new TDListNode< T >;       // add item to pTail
         if (pTail == NULL)                 // allocation error?
            {
             Report::ErrorMsg( "Allocation Error!" );
            return;
            }
         pHead = pCurrent = pTail;          // make pHead pTail
         pTail->pPrev     = NULL;           // set up pTail structure
         pTail->pNext     = NULL;
         pTail->data      = sNode->data;
         }
      else                                   // not start of target list?
         {
         pTail->pNext = new TDListNode< T >; // allocate new target node
                                             // and set pNext pointer
         if ( pTail->pNext == NULL )         // allocation error?
            {
             Report::ErrorMsg( "Allocation Error!" );
            return;
            }
         pTail->pNext->pNext = NULL;         // indicate end of list (tmp)
         pTail->pNext->pPrev = pTail;        // point to prev last element
         pTail->pNext->data  = sNode->data;  // retrieve data ptr from src

         pTail = pTail->pNext;               // increment pTail pointer
         }
      sNode = sNode->pNext;                  // move to next node of src
      }
   }


//-- TDList::TDList()------------------------------------------------
//
//-- constructors
//-------------------------------------------------------------------

//--- constructor ---//
template < class T >
TDList< T >::TDList( void )
   : Container()
   { pHead = pTail = pCurrent = NULL; }

//--- copy constructor ---//
template < class T >
TDList< T >::TDList( TDList< T > *dl )
   : Container()
   {
   if ( ! dl )
      {
      pHead = pTail = pCurrent = NULL;
      return;
      }

   length = dl->length;
   Copy( *dl );
   pCurrent = pHead;
   }

//--- destructor ---//
template < class T >
TDList< T >::~TDList( void ) { Empty(); }       // empty list (delete nodes)


//-- TDList::operator -----------------------------------------------
//
//-- various operators
//-------------------------------------------------------------------


template < class T >
T TDList< T >::Get( UINT pos )
   {
   GoToHead();

   for ( UINT i=0; i < pos; i++ )
      GoToNext();

   return Examine();
   }


//--- assignment operator ---//
template < class T >
void TDList< T >::operator = ( TDList< T > & dl )
   {
   Empty();
   length = dl.length;
   Copy( dl );
   pCurrent = pHead;
   }


//--- list append operator ---//
template < class T >
void TDList< T >::operator += ( TDList< T > & dl )
   {
   //void *item;

   TDListNode< T > *sNode = dl.GetHead();     // get access to source list

   while ( sNode != NULL )      // loop until end of source list
      {
      if ( pHead == NULL )                    // Empty target list?
         {
         pTail = new TDListNode< T >;         // add item to pTail
         if ( pTail == NULL )                 // allocation error?
            {
             Report::ErrorMsg( "Allocation Error!" );
            return;
            }
         pHead = pCurrent = pTail;            // make pHead pTail
         pTail->pPrev     = NULL;             // set up pTail structure
         pTail->pNext     = NULL;
         pTail->data     = sNode->data;
         }
      else                                    // not empty of target list?
         {
         //-- pTail->pNext is the (new) pTail node of the target list
         pTail->pNext = new TDListNode< T >;  // allocate new target node
         if ( pTail->pNext == NULL )          // allocation error?
            {
             Report::ErrorMsg( "Allocation Error!" );
            return;
            }
         pTail->pNext->pNext = NULL;          // indicate end of list (tmp)
         pTail->pNext->pPrev = pTail;         // point to prev last element
         pTail->pNext->data  = sNode->data;   // retrieve data ptr from src list
         pTail = pTail->pNext;                // increment pTail pointer
         }
      length++;                               // increment lengther
      sNode = sNode->pNext;                   // move to next node of src
      }
   return;
   }


//-- TDList::Append()------------------------------------------------
//
//-- append an item to the end of the list
//
//-- Arguments:
//      item = item to be added
//
//-- Return Value:
//      number of items in updated list
//-------------------------------------------------------------------

template < class T >
int TDList< T >::Append( T _data )
   {
   TDListNode< T > *_pCurrent = pCurrent;

   TDListNode< T > *newNode = new TDListNode< T >;  // allocate new node

   if ( newNode == NULL )               // allocation error?
      {
      Report::ErrorMsg( _T("Allocation Error!") );
      return length;
      }

   GoToTail();                          // go to end of list

   if ( pHead == NULL )                 // empty list?
      {
      pHead = pTail = pCurrent = newNode; // take care of object members
      newNode->pPrev = NULL;              // and newNode
      }
   else                                 // not empty
      {
      pTail = newNode;                  // fix pTail pointer
      newNode->pPrev = pCurrent;
      pCurrent->pNext = newNode;        // old end points to new node
      }

   newNode->pNext = NULL;
   newNode->data = _data;

   if ( _pCurrent != NULL )
      pCurrent = _pCurrent;    // restore original location

   length++;

   return length;
   }


//-- TDList::Insert()------------------------------------------------
//
//-- Insert a pointer to an item into the list
//
//-- Arguments:
//      pInsertItem = item to be inserted
//      pBeforeItem = item to which the inserted item is to be
//                    insert before
//
//-- Return Value:
//      number of items in list after insertion
//
//-- Comments:
//      1) pCurrent pointer is unchanged
//-------------------------------------------------------------------

template < class T >
int TDList< T >::Insert( T insertItem, T beforeItem )
   {
   if ( GoToItem( beforeItem ) == NULL ) // then move to before item
       return length;

   return Insert( insertItem );
   }


//-- insert before current position --//
template < class T >
int TDList< T >::Insert( T insertItem )
   {
   TDListNode< T > *newNode;

   newNode = new TDListNode< T >;       // allocate TDListNode storage
   if ( newNode == NULL )               // allocation error?
      {
       Report::ErrorMsg( "Allocation Error!" );
      return length;
      }

   newNode->pPrev = NULL;
   newNode->pNext = NULL;
   newNode->data = insertItem;

   if ( pHead == NULL )                 // is it an empty list?
      pHead = pTail = pCurrent = newNode;

   else if ( pCurrent->pPrev == NULL )  // insert at pHead?
      {
      pCurrent->pPrev = newNode;
      newNode->pNext = pCurrent;
      pHead = newNode;
      }

   else                                 // insert in body
      {
      pCurrent->pPrev->pNext = newNode;
      newNode->pPrev  =pCurrent->pPrev;
      pCurrent->pPrev = newNode;
      newNode->pNext  = pCurrent;
      }

   length++;
   return length;
   }



//-- TDList::Replace() ----------------------------------------------
//
//-- Replaces an item in the list
//
//-- Arguments:
//      pNewItem = new item
//      pOldItem = item to replace
//
//-- Return Value:
//      CO_FAILURE = item not replaced
//      CO_SUCCESS = item replaced
//
//-- Comments:
//      1) pCurrent is set to the replaced item
//-------------------------------------------------------------------

template < class T >
int TDList< T >::Replace( T newItem, T oldItem )
   {
   //TDListNode< T > *temp;

   if ( GoToItem( oldItem ) == NULL )  // then move to specified item
       return CO_FAILURE;

   //-- at proper node, now replace data ptr --//
   pCurrent->data = newItem;
   return CO_SUCCESS;
   }



//-- TDList::Delete() -----------------------------------------------
//
//-- Deletes an item in the list
//
//-- Arguments:
//      pItem = (optional) void pointer to item to be deleted
//              default = current item
//
//-- Return Value:
//      CO_FAILURE = item not deleted
//      CO_SUCCESS = item deleted
//
//-- Comments:
//      1) pCurrent is not changed unless it points the deleted item,
//         in which case it is set to the next item
//-------------------------------------------------------------------

template < class T >
int TDList< T >::Delete( T _data )
   {
   TDListNode< T > *_pCurrent = pCurrent;

   if ( GoToItem( _data ) == NULL )  // then move to specified item
      {
      pCurrent = _pCurrent;
      return CO_FAILURE;
      }

   //-- are we deleting the current item? --//
   if ( pCurrent == _pCurrent )
      _pCurrent = pCurrent->pNext;

   int success = Delete();  // delete the item at the current ptr

   // restore current ptr to initial state ( or next node if current deleted)
   if ( _pCurrent == NULL ) // tail of list?
      GoToTail();
   else
      pCurrent = _pCurrent;

   return success;
   }


//-- delete current item --//
template < class T >
int TDList< T >::Delete( void )
   {
   TDListNode< T > *temp;

   if ( length == 0 )
      return CO_FAILURE;

   //--- single item list?? ---//
   if ( pHead == pTail )
      {
      temp = pHead = pTail = NULL;
      }

   //--- is it the pHead? ---//
   else if ( IsHead() )
      {
      pCurrent->pNext->pPrev = NULL;    // set 2nd node pPrev to NULL
      temp = pHead = pCurrent->pNext;
      }

   //--- is it the pTail ---//
   else if ( IsTail() )
      {
      pCurrent->pPrev->pNext = NULL;
      temp = pTail = pCurrent->pPrev;
      }

   //--- neither Head nor Tail ---//
   else
      {
      pCurrent->pPrev->pNext = pCurrent->pNext;
      pCurrent->pNext->pPrev = pCurrent->pPrev;
      temp = pCurrent->pNext;
      }

   delete pCurrent;                       // deallocate deleted item
   pCurrent = temp;                       // reset pCurrent item ptr
   length--;                              // decrement length

   return CO_SUCCESS;
   }


//-- TDList::Empty() ------------------------------------------------
//
//-- Empties the list
//
//-- Comments: Doesnt clear what the pointer points to, only
//             the list node
//-------------------------------------------------------------------

template < class T >
void TDList< T >::Empty( void )
   {
   TDListNode< T > *temp, *hold;
   temp = pHead;
   while ( temp != NULL )
      {
      hold = temp->pNext;
      delete temp;
      temp = hold;
      }

   pHead = pTail = pCurrent = NULL;
   length = 0;
   }


//-- TDList::GoToItem() ---------------------------------------------
//
//-- Moves to the specified item
//
//-- Arguments:
//      pItem = void pointer to item to move to
//
//-- Return Value:
//      pointer to the found node
//
//-- Comments: none
//-------------------------------------------------------------------


template < class T >
TDListNode< T > *TDList< T >::GoToItem( T _data )
   {
   TDListNode< T > *temp;               // declare ptr to a TDList Node

   temp = pHead;                        // set it to the pHead of the list

   while ( temp != NULL )               // while not at end of list
      {
      if ( temp->data == _data )        // if the item has been found
         {
         pCurrent = temp;               // make it pCurrent
         return pCurrent;
         }
      else                              // not found, so keep trying
         temp = temp->pNext;            // move to pNext node
      }
      return NULL;                      // Item not found
   }



//-- TDList::GoToNext() ---------------------------------------------
//
//-- Move towards end of list
//
//-- Arguments: none
//
//-- Return Value: none
//
//-- Comments: none
//-------------------------------------------------------------------

template < class T >
void TDList< T >::GoToNext( void )
   {
   if ( (pCurrent != NULL) && (pCurrent->pNext != NULL) )
      pCurrent = pCurrent->pNext;
   }


//-- TDList::GoToPrev() ---------------------------------------------
//
//-- Move towards front of list
//
//-- Arguments: none
//
//-- Return Value: none
//
//-- Comments: none
//-------------------------------------------------------------------

template < class T >
void TDList< T >::GoToPrev( void )
   {
   if ( (pCurrent != NULL) && (pCurrent->pPrev != NULL) )
      pCurrent = pCurrent->pPrev;
   }


//-- TDList::IsHead() -----------------------------------------------
//
//-- test for current = head of list
//
//-- Arguments: none
//
//-- Return Value:
//      CO_FAILURE = not head of list
//      CO_SUCCESS = head of list
//
//-- Comments: none
//-------------------------------------------------------------------

template < class T >
int TDList< T >::IsHead( void )
   {
   if ( pCurrent->pPrev == NULL ) return CO_SUCCESS;
   else return CO_FAILURE;
   }


//-- TDList::IsTail() -----------------------------------------------
//
//-- test for current = tail of list
//
//-- Arguments: none
//
//-- Return Value:
//      CO_FAILURE = not tail of list
//      CO_SUCCESS = tail of list
//
//-- Comments: none
//-------------------------------------------------------------------

template < class T >
int TDList< T >::IsTail( void )
   {
   if ( length == 0 )
      return CO_SUCCESS;

   if ( pCurrent->pNext == NULL )
      return CO_SUCCESS;
   else
      return CO_FAILURE;
   }


//-- TDList::IsEmpty() ----------------------------------------------
//
//-- test for empty list
//
//-- Arguments: none
//
//-- Return Value:
//      CO_FAILURE = list not empty
//      CO_SUCCESS = list empty
//
//-- Comments: none
//-------------------------------------------------------------------

template < class T >
int TDList< T >::IsEmpty( void )
   {
   if ( pTail == NULL )
      return CO_SUCCESS;
   else
      return CO_FAILURE;
   }
