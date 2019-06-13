///////////////////////////////////////////////////////////////////////////////////////
/// \file gutil.h
/// \brief GUTIL LIBRARY (Fully portable version)
///
/// This library is provided as a component of the ecosystem modelling platform
/// LPJ-GUESS. It combines the functionality of the XTRING and BENUTIL libraries
/// These components are documented separately by commenting in this header file
/// and in the source code file gutil.cpp.
///
/// FULL PORTABILITY VERSION: tested and should work in any Unix, Linux or Windows
/// environment.
///
/// Enquiries to: Joe Siltberg, Lund University: joe.siltberg@nateko.lu.se
/// All rights reserved, copyright retained by the author.
///
/// \author Ben Smith, University of Lund
/// $Date: 2013-07-30 14:52:19 +0200 (Tue, 30 Jul 2013) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef GUTIL_H
#define GUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

void fail();

/// XTRING class - pointer-free manipulation of character strings in C++.
/** Declare new xstring objects using one of the following forms:
 *
 *  \code
 *     xtring s; // equivalent to xtring s=""
 *     xtring s="initial text";
 *     xtring s='c';
 *     xtring s(INITSIZE); // reserves space for at least INITSIZE+1 characters
 *  \endcode
 *
 *  You can also cast a string literal to an xtring the usual way:
 *
 *  \code
 *     (xtring)"Cast to a xtring"
 *  \endcode
 *
 *  In general, xtring objects can be used in place of standard C char* strings
 *  without explicit casting, e.g.
 *
 *  \code
 *     char copy[100];
 *     xtring original="text";
 *     strcpy(copy,original);
 *  \endcode
 *
 *  However, xtring objects must be explicitly casted to char* when specified as
 *  arguments in calls to functions with an ellipsis argument, e.g.
 *
 *  \code
 *     xtring name="Ben"
 *     printf("My name is: %s",(char*)name);
 *  \endcode
 *
 *  Casting to char* is useful also if you (unwisely?) choose to write directly to
 *  the internal string buffer of the xtring object:
 *
 *  \code
 *     xtring name(100);
 *     char* pbuffer=(char*)name;
 *     strcpy(pbuffer,"Ben");
 *  \endcode
 *
 *  Note that some of the member functions of xtring can cause the size and memory
 *  position of the internal buffer to change.
 *
 *  \section ops Operators
 *
 *  Operator functionality is described mainly by code examples. The examples below
 *  assume the following data types for variables:
 *
 *  \code
 *     xtring x1,x2,x3;
 *     char* s;
 *     char c;
 *     unsigned long n;
 *  \endcode
 *
 *  Assignment operator:
 *
 *  \code
 *     x1=x2; x1=s; x1=c;
 *  \endcode
 *
 *  Concatenation (appends a char* string, xtring or character to the end of an
 *  xtring string):
 *
 *  \code
 *     x1=x2+x3; x1=x2+s; x1=x2+c;
 *     x1+=x2;   x1+=s;   x1+=c;
 *  \endcode
 *
 *  Concatenation of, for example, two char* strings, or a xtring to the end of
 *  a char* string, is possible by casting one of the operands to an xtring:
 *
 *  \code
 *     x1=(xtring)"first"+"second";
 *     x1=(xtring)"before"+x2;
 *  \endcode
 *
 *  Reference to character as array element:
 *
 *  \code
 *     c=x1[n]; x1[n]=c;
 *  \endcode
 *
 *  The size of the internal string buffer is expanded if necessary to ensure
 *  that the specified index is valid (points to a character position within the
 *  internal string buffer). However, the string itself is not expanded (i.e.
 *  the position of the trailing null byte, signifying the end of the string, is
 *  not changed.
 *
 *  Comparison operators:
 *
 *  \code
 *     x1==x2 && x2!=x3 || x1<s || x1>c || x2<=x3 || x3>=x1
 *  \endcode
 *
 *  Note that the left hand operand must be a xtring (or casted to xtring)
 *
 *  \author Ben Smith, University of Lund
 */
class xtring {

	 // MEMBER VARIABLES

private:
	 char *ptext;
	 unsigned long nxegment;
	 char *buf;

	 // MEMBER FUNCTIONS

private:
	 void init();
	 void resize(unsigned long nchar);
	 void expand(unsigned long nchar);

public:
	 xtring(const xtring& s);
	 xtring();
	 xtring(char* text);
	 xtring(const char* inittext);
	 xtring(char c);
	 xtring(unsigned long n);
	 xtring(int n);
	 xtring(long n);
	 xtring(unsigned int n);
	 ~xtring();
	 operator char*();
	 operator const char*() const;

	 /// Adjusts memory allocation
	 /** Expands or contracts the memory allocation to the current xtring object
	  *  to accomodate a string at least n characters in length (not including
	  *  the trailing null byte). The currently stored string may be copied to a
	  *  new location in memory but is not deleted. This function may be useful
	  *  if you intend to write directly to the internal string buffer, whose
	  *  address is returned by casting the xtring object to char*.
	  */
	 void reserve(unsigned long n);

	 /// Length of string
	 /** Returns length of current string in characters
	  *  (not including trailing null character).
	  */
	 unsigned long len();

	 /// Converts to upper case
	 /** Returns a new xtring equivalent to the current one, but with
	  *  lower-case alphabetics 'a'-'z' converted to upper case.
	  */
	 xtring upper();

	 /// Converts to lower case
	 /** Returns a new xtring equivalent to the current one, but with
	  *  upper-case alphabetics 'A'-'Z' converted to lower case.
	  */
	 xtring lower();

	 /// Returns only printable characters
	 /** Returns a new xtring equivalent to the current one, but with
	  *  non-printable characters (ASCII code 0-31) removed.
	  */
	 xtring printable();

	 /// Returns a prefix of the string
	 /** Returns a new xtring consisting of the leftmost n characters of
	  *  the current xtring. A null xtring ("") is returned if n<=0; if n>length
	  *  of the current xtring, an identical copy of the current xtring is
	  *  returned.
	  */
	 xtring left(unsigned long n);

	 /// Returns a substring
	 /** Returns a new xtring consisting of the rightmost portion of the current
	  *  xtring, starting at character number s (on 0 base). If s<0 an identical
	  *  copy of the current xtring is returned; if s>=length of the current
	  *  xtring, a null xtring is returned.
	  */
	 xtring mid(unsigned long s);

	 /// Returns a substring
	 /** Returns a new xtring consisting of up to n characters, starting at
	  *  character number s of the current xtring. If s<0, the new string starts
	  *  at character 0 of the current xtring; if s>=length of the current
	  *  xtring, a null xtring is returned.
	  */
	 xtring mid(unsigned long s,unsigned long n);

	 /// Returns a postfix of the string
	 /** Returns a new xtring consisting of the rightmost n characters of the
	  *  current xtring. A null string is returned if n<=0; if n>length of the
	  *  current xtring, an identical copy is returned.
	  */
	 xtring right(unsigned long n);

	 /// Search for a substring
	 /** Returns the position of the (first character of the) specified character
	  *  string if it occurs as a substring of the current xtring. If there are
	  *  several occurences, the position of the leftmost occurence is returned.
	  *  Returns -1 if string not found.
	  */
	 long find(const char* s);

	 /// Search for a character
	 /** Returns the position of the (first character of the) specified character
	  *  if it occurs in the current xtring. If there are several occurences, the
	  *  position of the leftmost occurence is returned.
	  *  Returns -1 if character not found.
	  */
	 long find(char c);

	 /// Search for one of several characters
	 /** Returns the position in this xtring of the first (leftmost) occurrence
	  *  any character forming part of the string pointed to by s. Returns -1 if
	  *  there are no occurrences.
	  */
	 long findoneof(const char* s);

	 /// Search for character which isn't one of a given sequence of characters
	 /** Returns the position in this xtring of the first (leftmost) character
	  *  NOT forming part of the string pointed to by s. Returns -1 if no such
	  *  character is found.
	  */
	 long findnotoneof(const char* s);

	 /// Convert to a number
	 /** Returns the numerical value of the current xtring, if it is a valid
	  *  representation of a double precision floating point number in C++. Call
	  *  function isnum() to test whether the returned value is meaningful.
	  */
	 double num();

	 /// Checks if the string represents a number
	 /** Returns 1 if the current xtring is a valid representation of a double
	  *  precision floating point number in C++, 0 otherwise.
	  */
	 char isnum();

	 /// String formatting
	 /** A printf-style function for writing formatted data to this xtring object.
	  *  Equivalent to sprintf in the standard C library (stdio.h).
	  */
	 void printf(const char* fmt,...);

	 xtring& operator=(xtring& s);
	 xtring& operator=(const char* s);
	 xtring& operator=(char c);
	 xtring operator+(xtring& s2);
	 xtring operator+(const char* s2);
	 xtring operator+(char c);
	 xtring& operator+=(xtring& s2);
	 xtring& operator+=(const char* s2);
	 xtring& operator+=(char c);
	 char& operator[](unsigned long n);
	 char& operator[](int n);
	 char& operator[](unsigned int n);
	 char& operator[](long n);
	 bool operator==(xtring& s2) const;
	 bool operator==(char* s2) const;
	 bool operator==(char c) const;
	 bool operator==(const char* s2) const;
	 bool operator!=(xtring& s2) const;
	 bool operator!=(char* s2) const;
	 bool operator!=(char c) const;
	 bool operator!=(const char* s2) const;
	 bool operator<(xtring& s2) const;
	 bool operator<(char* s2) const;
	 bool operator<(char c) const;
	 bool operator<(const char* s2) const;
	 bool operator>(xtring& s2) const;
	 bool operator>(char* s2) const;
	 bool operator>(char c) const;
	 bool operator>(const char* s2) const;
	 bool operator<=(xtring& s2) const;
	 bool operator<=(char* s2) const;
	 bool operator<=(char c) const;
	 bool operator<=(const char* s2) const;
	 bool operator>=(xtring& s2) const;
	 bool operator>=(char* s2) const;
	 bool operator>=(char c) const;
	 bool operator>=(const char* s2) const;
};


///////////////////////////////////////////////////////////////////////////////////////
// COLLECTION CLASS TEMPLATES FROM THE BENUTIL LIBRARY


const int SEGSIZE=16;


/// List Array of objects
/** Use this template to produce collection classes of any objects with a valid default
 *  constructor.
 *
 *  Declare a type using this template as either:
 *
 *  \code
 *    typedef ListArray<MyObjectType> MyCollectionClass;
 *  \endcode
 *
 *  or (for collection classes containing additional members)
 *
 *  \code
 *    class myclass : public ListArray<MyObjectType>
 *    { ... };
 *  \endcode
 *
 *  Either of the above declarations will produce a class including the public functions
 *  and member variables included in this class.
 *
 *  This functionality implies that it is possible to loop sequentially through all
 *  items in the list array using code like the following:
 *
 *  \code
 *    mycollection.firstobj();
 *    while (mycollection.isobj) {
 *      MyObjectType& obj=mycollection.getobj(); // NB: '&' required unless query only
 *      // query or modify object obj here
 *      mycollection.nextobj();
 *    }
 *  \endcode
 *
 *  or alternatively:
 *
 *  \code
 *    for (i=0;i<mycollection.nobj;i++) {
 *      MyObjectType& obj=mycollection[i];
 *      // query or modify object obj here
 *    }
 *  \endcode
 */
template<class tdata> class ListArray {

	 class Item {

	 public:
		  Item* pnext;
		  Item* pprev;
		  tdata object;
	 };


private:
	 Item** array;
	 Item* pfirstitem;
	 Item* plastitem;
	 Item* pthisitem;
	 unsigned int nseg;
	 unsigned int thisobj;

public:
	 /// Whether the internal object pointer points to an object
	 /** This variable (NB: not a function) is true whenever the internal object
	  *  pointer points to a MyObjectType object, false otherwise (including
	  *  when the list array is empty).
	  */
	 bool isobj;

	 /// The number of objects currently stored in the list array.
	 unsigned int nobj;

public:
	 ListArray() {
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
	 }

	 /// Clears the entire list array, releasing dynamic memory.
	 void killall() {
		  Item* pitem=pfirstitem;
		  Item* pnext;
		  while (pitem) {
				pnext=pitem->pnext;
				delete pitem;
				pitem=pnext;
		  }
		  if (nseg) delete[] array;
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
	 }

	 ~ListArray() {
		  killall();
	 }

	 /// Clears list array (if not empty)
	 /** Clears the array (if not empty) and fills it with nitem MyObjectType
	  *  objects.
	  */
	 void initarray(unsigned int nitem) {
		  unsigned int i;
		  killall();
		  if (nitem<=0) return;
		  nseg=nitem%SEGSIZE;
		  if (nseg*SEGSIZE<nitem) nseg++;
		  array=new Item*[nseg*SEGSIZE];
		  if (!array) fail();
		  for (i=0;i<nitem;i++) createobj();
		  pthisitem=0;
		  thisobj=0;
	 }

	 /// Creates a new object of type MyObjectType and returns a reference to it.
	 tdata& createobj() {
		  Item* pitem=new Item;
		  if (!pitem) fail();
		  pitem->pprev=plastitem;
		  pitem->pnext=NULL;
		  if (plastitem) plastitem->pnext=pitem;
		  else pfirstitem=pitem;
		  plastitem=pitem;
		  pthisitem=pitem;
		  isobj=true;
		  thisobj=nobj++;
		  if (nobj>nseg*SEGSIZE) {
				Item** newarray=new Item*[(nseg+1)*SEGSIZE];
				if (!newarray) fail();
				if (nseg++) {
					 unsigned int i;
					 for (i=0;i<nobj-1;i++)
						  newarray[i]=array[i];
					 delete[] array;
				}
				array=newarray;
		  }
		  array[nobj-1]=pitem;
		  return pitem->object;
	 }

	 /// Go to the first object
	 /** Causes the internal object pointer (pthisitem) to point to the first
	  *  MyObjectType object in the list array. Returns false if the list array
	  *  is empty.
	  */
	 bool firstobj() {
		  if (pfirstitem) {
				pthisitem=pfirstitem;
				isobj=true;
				thisobj=0;
				return true;
		  }
		  return false;
	 }

	 /// Go to the next object
	 /** Causes the internal object pointer to point to the next MyObjectType
	  *  object in the list array. Returns false if the last item has already
	  *  been reached.
	  */
	 bool nextobj() {
		  if (pthisitem) {
				pthisitem=pthisitem->pnext;
				thisobj++;
		  }
		  if (pthisitem) return true;
		  isobj=false;
		  return false;
	 }

	 /// Returns current object
	 /** Returns a reference to the object currently pointed to by the internal
	  *  object pointer. Do not call this function unless the pointer is
	  *  pointing to a valid object (isobj=true).
	  */
	 tdata& getobj() {
		  return pthisitem->object;
	 }

	 /// Returns i'th object
	 /** Overload of the array brackets operator which returns a reference to
	  *  the i'th MyObjectType item in the list array. Do not call unless the
	  *  internal object pointer is pointing to a valid object. Note that this
	  *  function does NOT affect the value of the internal pointer.
	  */
	 tdata& operator[](unsigned int i) {
		  return array[i]->object;
	 }

	 /// Removes the object currently pointed to by the internal pointer.
	 void killobj() {
		  unsigned int i;
		  if (!pthisitem) return;
		  if (pthisitem==pfirstitem) pfirstitem=pthisitem->pnext;
		  if (pthisitem==plastitem) plastitem=pthisitem->pprev;
		  Item* pnextitem=pthisitem->pnext;
		  if (pthisitem->pprev) pthisitem->pprev->pnext=pthisitem->pnext;
		  if (pthisitem->pnext) pthisitem->pnext->pprev=pthisitem->pprev;
		  delete pthisitem;
		  pthisitem=pnextitem;
		  if (!pthisitem) isobj=false;
		  for (i=thisobj+1;i<nobj;i++)
				array[i-1]=array[i];
		  nobj--;
	 }
};


/// List Array of objects with id member and no reference members
/** Use this template to produce collection classes for objects similar to:
 *
 *  \code
 *    class MyObjectType {
 *    public:
 *      unsigned int id;
 *      // Other member data and functions
 *    };
 *  \endcode
 *
 *  Declare a type using this template as either:
 *
 *  \code
 *    typedef ListArray_id<MyObjectType> MyCollectionClass;
 *  \endcode
 *
 *  or (for collection classes containing additional members)
 *
 *  \code
 *    class myclass : public ListArray_id<MyObjectType>
 *    { ... };
 *  \endcode
 *
 *  Either of the above declarations will produce a class including the public functions
 *  and member variables included in this class.
 *
 *  This functionality implies that it is possible to loop sequentially through all
 *  items in the list array using code like the following:
 *
 *  \code
 *    mycollection.firstobj();
 *    while (mycollection.isobj) {
 *      MyObjectType& obj=mycollection.getobj(); // NB: '&' required unless query only
 *      // query or modify object obj here
 *      mycollection.nextobj();
 *    }
 *  \endcode
 *
 *  or alternatively:
 *
 *  \code
 *    for (i=0;i<mycollection.nobj;i++) {
 *      MyObjectType& obj=mycollection[i];
 *      // query or modify object obj here
 *    }
 *  \endcode
 */
template<class tdata> class ListArray_id {

	 class Item {

	 public:
		  Item* pnext;
		  Item* pprev;
		  tdata object;
	 };


private:
	 Item** array;
	 Item* pfirstitem;
	 Item* plastitem;
	 Item* pthisitem;
	 unsigned int nseg;
	 unsigned int id;
	 unsigned int thisobj;

public:
	 /// Whether the internal object pointer points to an object
	 /** This variable (NB: not a function) is true whenever the internal
	  *  object pointer points to a MyObjectType object, false otherwise
	  *  (including when the list array is empty).
	  */
	 bool isobj;

	 /// The number of objects currently stored in the list array.
	 unsigned int nobj;

public:
	 ListArray_id() {
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 /// Clears the entire list array, releasing dynamic memory.
	 /** Resets the id counter to 0.
	  */
	 void killall() {
		  Item* pitem=pfirstitem;
		  Item* pnext;
		  while (pitem) {
				pnext=pitem->pnext;
				delete pitem;
				pitem=pnext;
		  }
		  if (nseg) delete[] array;
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 ~ListArray_id() {
		  killall();
	 }

	 /// Clears list array (if not empty)
	 /** Clears list array (if not empty), resets id counter to 0 and fills list
	  *  array with nitem MyObjectType objects.
	  */
	 void initarray(unsigned int nitem) {
		  unsigned int i;
		  killall();
		  if (nitem<=0) return;
		  nseg=nitem%SEGSIZE;
		  if (nseg*SEGSIZE<nitem) nseg++;
		  array=new Item*[nseg*SEGSIZE];
		  if (!array) fail();
		  for (i=0;i<nitem;i++) createobj();
		  pthisitem=0;
		  thisobj=0;
	 }

	 /// Creates a new object of type MyObjectType and returns a reference to it.
	 /** The id member of the new object is set to the value of a counter
	  *  which has initial value 0 and is incremented (by 1) on each subsequent
	  *  call to createobj.
	  */
	 tdata& createobj() {
		  Item* pitem=new Item;
		  if (!pitem) fail();
		  pitem->object.id=id++;
		  pitem->pprev=plastitem;
		  pitem->pnext=NULL;
		  if (plastitem) plastitem->pnext=pitem;
		  else pfirstitem=pitem;
		  plastitem=pitem;
		  pthisitem=pitem;
		  isobj=true;
		  thisobj=nobj++;
		  if (nobj>nseg*SEGSIZE) {
				Item** newarray=new Item*[(nseg+1)*SEGSIZE];
				if (!newarray) fail();
				if (nseg++) {
					 unsigned int i;
					 for (i=0;i<nobj-1;i++)
						  newarray[i]=array[i];
					 delete[] array;
				}
				array=newarray;
		  }
		  array[nobj-1]=pitem;
		  return pitem->object;
	 }

	 /// Go to the first object
	 /** Causes the internal object pointer (pthisitem) to point to the first
	  *  MyObjectType object in the list array. Returns false if the list array
	  *  is empty.
	  */
	 bool firstobj() {
		  if (pfirstitem) {
				pthisitem=pfirstitem;
				isobj=true;
				thisobj=0;
				return true;
		  }
		  return false;
	 }

	 /// Go to the next object
	 /** Causes the internal object pointer to point to the next MyObjectType
	  *  object in the list array. Returns false if the last item has already
	  *  been reached.
	  */
	 bool nextobj() {
		  if (pthisitem) {
				pthisitem=pthisitem->pnext;
				thisobj++;
		  }
		  if (pthisitem) return true;
		  isobj=false;
		  return false;
	 }

	 /// Returns current object
	 /** Returns a reference to the object currently pointed to by the internal
	  *  object pointer. Do not call this function unless the pointer is
	  *  pointing to a valid object (isobj=true).
	  */
	 tdata& getobj() {
		  return pthisitem->object;
	 }

	 /// Returns i'th object
	 /** Overload of the array brackets operator which returns a reference to
	  *  the i'th MyObjectType item in the list array. Do not call unless the
	  *  internal object pointer is pointing to a valid object. Note that this
	  *  function does NOT affect the value of the internal pointer.
	  */
	 tdata& operator[](unsigned int i) {
		  return array[i]->object;
	 }

	 /// Removes the object currently pointed to by the internal pointer.
	 void killobj() {
		  unsigned int i;
		  if (!pthisitem) return;
		  if (pthisitem==pfirstitem) pfirstitem=pthisitem->pnext;
		  if (pthisitem==plastitem) plastitem=pthisitem->pprev;
		  Item* pnextitem=pthisitem->pnext;
		  if (pthisitem->pprev) pthisitem->pprev->pnext=pthisitem->pnext;
		  if (pthisitem->pnext) pthisitem->pnext->pprev=pthisitem->pprev;
		  delete pthisitem;
		  pthisitem=pnextitem;
		  if (!pthisitem) isobj=false;
		  for (i=thisobj+1;i<nobj;i++)
				array[i-1]=array[i];
		  nobj--;
	 }
};


/// List Array of objects with id member plus one extra member to initialise
/** Use this template to produce collection classes for objects similar to:
 *
 *  \code
 *    class MyObjectType {
 *    private:
 *      unsigned int id;
 *      MyRefType& refmember; // typically a reference member, but may be any type
 *    public:
 *      MyObjectType(unsigned int i,MyRefType& r):id(i),refmember(r) {}
 *        // constructor with declaration like this MUST exist
 *      // Other member data and functions
 *    };
 *  \endcode
 *
 *  Declare a type using this template as either:
 *
 *  \code
 *    typedef ListArray_idin1<MyObjectType,MyRefType> MyCollectionClass;
 *  \endcode
 *
 *  or (for collection classes containing additional members)
 *
 *  \code
 *    class myclass : ListArray_idin1<MyObjectType,MyRefType>
 *    { ... };
 *  \endcode
 *
 *  Either of the above declarations will produce a class including the public functions
 *  and member variables included in this class.
 *
 *  This functionality implies that it is possible to loop sequentially through all
 *  items in the list array using code like the following:
 *
 *  \code
 *    mycollection.firstobj();
 *    while (mycollection.isobj) {
 *      MyObjectType& obj=mycollection.getobj(); // NB: '&' required unless query only
 *      // query or modify object obj here
 *      mycollection.nextobj();
 *    }
 *  \endcode
 *
 *  or alternatively:
 *
 *  \code
 *    for (i=0;i<mycollection.nobj;i++) {
 *      MyObjectType& obj=mycollection[i];
 *      // query or modify object obj here
 *    }
 *  \endcode
 */
template<class tdata,class tref> class ListArray_idin1 {

	 class Item {

	 public:
		  Item* pnext;
		  Item* pprev;
		  tdata object;

		  Item(unsigned int id,tref& ref) : object(id,ref) {}
	 };


private:
	 Item** array;
	 Item* pfirstitem;
	 Item* plastitem;
	 Item* pthisitem;
	 unsigned int nseg;
	 unsigned int id;
	 unsigned int thisobj;

public:
	 /// Whether the internal object pointer points to an object
	 /** This variable (NB: not a function) is true whenever the internal object
	  *  pointer points to a MyObjectType object, false otherwise (including
	  *  when the list array is empty).
	  */
	 bool isobj;

	 /// The number of objects currently stored in the list array.
	 unsigned int nobj;

public:
	 ListArray_idin1() {
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 /// Clears the entire list array, releases dynamic memory.
	 /** Resets the id counter to 0.
	  */
	 void killall() {
		  Item* pitem=pfirstitem;
		  Item* pnext;
		  while (pitem) {
				pnext=pitem->pnext;
				delete pitem;
				pitem=pnext;
		  }
		  if (nseg) delete[] array;
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 ~ListArray_idin1() {
		  killall();
	 }

	 /// Creates a new object of type MyObjectType and returns a reference to it.
	 /** The constructor function MyObjectType(unsigned int,MyRefType&) is
	  *  called, with the unsigned int argument set to the value of a counter
	  *  which has initial value 0 and is incremented (by 1) on each subsequent
	  *  call to createobj; ref is sent as the MyRefType& argument to the
	  *  constructor function.
	  */
	 tdata& createobj(tref& ref) {
		  Item* pitem=new Item(id++,ref);
		  if (!pitem) fail();
		  pitem->pprev=plastitem;
		  pitem->pnext=NULL;
		  if (plastitem) plastitem->pnext=pitem;
		  else pfirstitem=pitem;
		  plastitem=pitem;
		  pthisitem=pitem;
		  isobj=true;
		  thisobj=nobj++;
		  if (nobj>nseg*SEGSIZE) {
				Item** newarray=new Item*[(nseg+1)*SEGSIZE];
				if (!newarray) fail();
				if (nseg++) {
					 unsigned int i;
					 for (i=0;i<nobj-1;i++)
						  newarray[i]=array[i];
					 delete[] array;
				}
				array=newarray;
		  }
		  array[nobj-1]=pitem;
		  return pitem->object;
	 }

	 /// Go to the first object
	 /** Causes the internal object pointer (pthisitem) to point to the first
	  *  MyObjectType object in the list array. Returns false if the list array
	  *  is empty.
	  */
	 bool firstobj() {
		  if (pfirstitem) {
				pthisitem=pfirstitem;
				isobj=true;
				thisobj=0;
				return true;
		  }
		  return false;
	 }

	 /// Go to the next object
	 /** Causes the internal object pointer to point to the next MyObjectType
	  *  object in the list array. Returns false if the last item has already
	  *  been reached.
	  */
	 bool nextobj() {
		  if (pthisitem) {
				pthisitem=pthisitem->pnext;
				thisobj++;
		  }
		  if (pthisitem) return true;
		  isobj=false;
		  return false;
	 }


	 /// Returns current object
	 /** Returns a reference to the object currently pointed to by the internal
	  *  object pointer. Do not call this function unless the pointer is
	  *  pointing to a valid object (isobj=true).
	  */
	 tdata& getobj() {
		  return pthisitem->object;
	 }

	 /// Returns i'th object
	 /** Overload of the array brackets operator which returns a reference to
	  *  the i'th MyObjectType item in the list array. Do not call unless the
	  *  internal object pointer is pointing to a valid object. Note that this
	  *  function does NOT affect the value of the internal pointer.
	  */
	 tdata& operator[](unsigned int i) {
		  return array[i]->object;
	 }

	 /// Removes the object currently pointed to by the internal pointer.
	 void killobj() {
		  unsigned int i;
		  if (!pthisitem) return;
		  if (pthisitem==pfirstitem) pfirstitem=pthisitem->pnext;
		  if (pthisitem==plastitem) plastitem=pthisitem->pprev;
		  Item* pnextitem=pthisitem->pnext;
		  if (pthisitem->pprev) pthisitem->pprev->pnext=pthisitem->pnext;
		  if (pthisitem->pnext) pthisitem->pnext->pprev=pthisitem->pprev;
		  delete pthisitem;
		  pthisitem=pnextitem;
		  if (!pthisitem) isobj=false;
		  for (i=thisobj+1;i<nobj;i++)
				array[i-1]=array[i];
		  nobj--;
	 }
};


/// List Array of objects with id member plus two extra members to initialise
/** Use this template to produce collection classes for objects similar to:
 *
 *  \code
 *    class MyObjectType {
 *    private:
 *      unsigned int id;
 *      MyRefType1& refmember1; // typically reference members,
 *      MyRefType2& refmember2; // but may be any type
 *    public:
 *      MyObjectType(unsigned int i,MyRefType1& r1,MyRefType2& r2):id(i),
 *                   refmember1(r1),refmember2(r2) {}
 *        // constructor with declaration like this MUST exist
 *      // Other member data and functions
 *    };
 *  \endcode
 *
 *  Declare a type using this template as either:
 *
 *  \code
 *    typedef ListArray_idin2<MyObjectType,MyRefType1,MyRefType2>
 *      MyCollectionClass;
 *  \endcode
 *
 *  or (for collection classes containing additional members)
 *
 *  \code
 *    class myclass :
 *      public ListArray_idin2<MyObjectType,MyRefType1,MyRefType2>
 *      { ... };
 *  \endcode
 *
 *  Either of the above declarations will produce a class including the public functions
 *  and member variables included in this class.
 *
 *  This functionality implies that it is possible to loop sequentially through all
 *  items in the list array using code like the following:
 *
 *  \code
 *    mycollection.firstobj();
 *    while (mycollection.isobj) {
 *      MyObjectType& obj=mycollection.getobj(); // NB: '&' required unless query only
 *      // query or modify object obj here
 *      mycollection.nextobj();
 *    }
 *  \endcode
 *
 *  or alternatively:
 *
 *  \code
 *    for (i=0;i<mycollection.nobj;i++) {
 *      MyObjectType& obj=mycollection[i];
 *      // query or modify object obj here
 *   }
 *  \endcode
 */
template<class tdata,class tref1,class tref2> class ListArray_idin2 {

	 class Item {

	 public:
		  Item* pnext;
		  Item* pprev;
		  tdata object;

		  Item(unsigned int id,tref1& ref1,tref2& ref2) :
					 object(id,ref1,ref2) {}
	 };


private:
	 Item** array;
	 Item* pfirstitem;
	 Item* plastitem;
	 Item* pthisitem;
	 unsigned int nseg;
	 unsigned int id;
	 unsigned int thisobj;

public:
	 /// Whether the internal object pointer points to an object
	 /** This variable (NB: not a function) is true whenever the internal object
	  *  pointer points to a MyObjectType object, false otherwise (including
	  *  when the list array is empty).
	  */
	 bool isobj;

	 /// The number of objects currently stored in the list array.
	 unsigned int nobj;

public:
	 ListArray_idin2() {
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 /// Clears the entire list array, releases dynamic memory.
	 /** Resets the id counter to 0.
	  */
	 void killall() {
		  Item* pitem=pfirstitem;
		  Item* pnext;
		  while (pitem) {
				pnext=pitem->pnext;
				delete pitem;
				pitem=pnext;
		  }
		  if (nseg) delete[] array;
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 ~ListArray_idin2() {
		  killall();
	 }

	 /// Creates a new object of type MyObjectType and returns a reference to it.
	 /** The constructor function MyObjectType(unsigned int,MyRefType1&,MyRefType2&)
	  *  is called, with the unsigned int argument set to the value of a counter
	  *  which has initial value 0 and is incremented (by 1) on each subsequent
	  *  call to createobj; ref1 and ref2 are sent as the MyRefType& arguments
	  *  to the constructor function.
	  */
	 tdata& createobj(tref1& ref1,tref2& ref2) {
		  Item* pitem=new Item(id++,ref1,ref2);
		  if (!pitem) fail();
		  pitem->pprev=plastitem;
		  pitem->pnext=NULL;
		  if (plastitem) plastitem->pnext=pitem;
		  else pfirstitem=pitem;
		  plastitem=pitem;
		  pthisitem=pitem;
		  isobj=true;
		  thisobj=nobj++;
		  if (nobj>nseg*SEGSIZE) {
				Item** newarray=new Item*[(nseg+1)*SEGSIZE];
				if (!newarray) fail();
				if (nseg++) {
					 unsigned int i;
					 for (i=0;i<nobj-1;i++)
						  newarray[i]=array[i];
					 delete[] array;
				}
				array=newarray;
		  }
		  array[nobj-1]=pitem;
		  return pitem->object;
	 }

	 /// Returns the first object
	 /** Causes the internal object pointer (pthisitem) to point to the first
	  *  MyObjectType object in the list array. Returns false if the list array
	  *  is empty.
	  */
	 bool firstobj() {
		  if (pfirstitem) {
				pthisitem=pfirstitem;
				isobj=true;
				thisobj=0;
				return true;
		  }
		  return false;
	 }

	 /// Returns the next object
	 /** Causes the internal object pointer to point to the next MyObjectType
	  *  object in the list array. Returns false if the last item has already
	  * been reached.
	  */
	 bool nextobj() {
		  if (pthisitem) {
				pthisitem=pthisitem->pnext;
				thisobj++;
		  }
		  if (pthisitem) return true;
		  isobj=false;
		  return false;
	 }

	 /// Returns current object
	 /** Returns a reference to the object currently pointed to by the internal
	  *  object pointer. Do not call this function unless the pointer is
	  *  pointing to a valid object (isobj=true).
	  */
	 tdata& getobj() {
		  return pthisitem->object;
	 }

	 /// Returns i'th object
	 /** Overload of the array brackets operator which returns a reference to
	  *  the i'th MyObjectType item in the list array. Do not call unless the
	  *  internal object pointer is pointing to a valid object. Note that this
	  *  function does NOT affect the value of the internal pointer.
	  */
	 tdata& operator[](unsigned int i) {
		  return array[i]->object;
	 }

	 /// Removes the object currently pointed to by the internal pointer.
	 void killobj() {
		  unsigned int i;
		  if (!pthisitem) return;
		  if (pthisitem==pfirstitem) pfirstitem=pthisitem->pnext;
		  if (pthisitem==plastitem) plastitem=pthisitem->pprev;
		  Item* pnextitem=pthisitem->pnext;
		  if (pthisitem->pprev) pthisitem->pprev->pnext=pthisitem->pnext;
		  if (pthisitem->pnext) pthisitem->pnext->pprev=pthisitem->pprev;
		  delete pthisitem;
		  pthisitem=pnextitem;
		  if (!pthisitem) isobj=false;
		  for (i=thisobj+1;i<nobj;i++)
				array[i-1]=array[i];
		  nobj--;
	 }
};


/// List Array of objects with id member plus three extra members to initialise
/** Use this template to produce collection classes for objects similar to:
 *
 *  \code
 *    class MyObjectType {
 *    private:
 *      unsigned int id;
 *      MyRefType1& refmember1; // typically reference members,
 *      MyRefType2& refmember2; // but may be any type
 *      MyRefType3& refmember3;
 *    public:
 *      MyObjectType(unsigned int i,MyRefType1& r1,MyRefType2& r2):id(i),
 *                   refmember1(r1),refmember2(r2),refmember3(r3) {}
 *        // constructor with declaration like this MUST exist
 *      // Other member data and functions
 *    };
 *  \endcode
 *
 *  Declare a type using this template as either:
 *
 *  \code
 *    typedef ListArray_idin3<MyObjectType,MyRefType1,MyRefType2,MyRefType3>
 *      MyCollectionClass;
 *  \endcode
 *
 *  or (for collection classes containing additional members)
 *
 *  \code
 *    class myclass :
 *      public ListArray_idin3<MyObjectType,MyRefType1,MyRefType2,MyRefType3>
 *      { ... };
 *  \endcode
 *
 *  Either of the above declarations will produce a class including the public functions
 *  and member variables included in this class.
 *
 *  This functionality implies that it is possible to loop sequentially through all
 *  items in the list array using code like the following:
 *
 *  \code
 *    mycollection.firstobj();
 *    while (mycollection.isobj) {
 *      MyObjectType& obj=mycollection.getobj(); // NB: '&' required unless query only
 *      // query or modify object obj here
 *      mycollection.nextobj();
 *    }
 *  \endcode
 *
 *  or alternatively:
 *
 *  \code
 *    for (i=0;i<mycollection.nobj;i++) {
 *      MyObjectType& obj=mycollection[i];
 *      // query or modify object obj here
 *   }
 *  \endcode
 */
template<class tdata,class tref1,class tref2,class tref3> class ListArray_idin3 {

	 class Item {

	 public:
		  Item* pnext;
		  Item* pprev;
		  tdata object;

		  Item(unsigned int id,tref1& ref1,tref2& ref2,tref3& ref3) :
					 object(id,ref1,ref2,ref3) {}
	 };


private:
	 Item** array;
	 Item* pfirstitem;
	 Item* plastitem;
	 Item* pthisitem;
	 unsigned int nseg;
	 unsigned int id;
	 unsigned int thisobj;

public:
	 /// Whether the internal object pointer points to an object
	 /** This variable (NB: not a function) is true whenever the internal object
	  *  pointer points to a MyObjectType object, false otherwise (including
	  *  when the list array is empty).
	  */
	 bool isobj;

	 /// The number of objects currently stored in the list array.
	 unsigned int nobj;

public:
	 ListArray_idin3() {
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 /// Clears the entire list array, releases dynamic memory.
	 /** Resets the id counter to 0.
	  */
	 void killall() {
		  Item* pitem=pfirstitem;
		  Item* pnext;
		  while (pitem) {
				pnext=pitem->pnext;
				delete pitem;
				pitem=pnext;
		  }
		  if (nseg) delete[] array;
		  pfirstitem=NULL;
		  plastitem=NULL;
		  pthisitem=NULL;
		  isobj=false;
		  nobj=0;
		  nseg=0;
		  id=0;
	 }

	 ~ListArray_idin3() {
		  killall();
	 }

	 /// Creates a new object of type MyObjectType and returns a reference to it.
	 /** The constructor function MyObjectType(unsigned int,MyRefType1&,MyRefType2&, MyRefType3&)
	  *  is called, with the unsigned int argument set to the value of a counter
	  *  which has initial value 0 and is incremented (by 1) on each subsequent
	  *  call to createobj; ref1, ref2 and ref3 are sent as the MyRefType&
	  *  arguments to the constructor function.
	  */
	 tdata& createobj(tref1& ref1,tref2& ref2,tref3& ref3) {
		  Item* pitem=new Item(id++,ref1,ref2,ref3);
		  if (!pitem) fail();
		  pitem->pprev=plastitem;
		  pitem->pnext=NULL;
		  if (plastitem) plastitem->pnext=pitem;
		  else pfirstitem=pitem;
		  plastitem=pitem;
		  pthisitem=pitem;
		  isobj=true;
		  thisobj=nobj++;
		  if (nobj>nseg*SEGSIZE) {
				Item** newarray=new Item*[(nseg+1)*SEGSIZE];
				if (!newarray) fail();
				if (nseg++) {
					 unsigned int i;
					 for (i=0;i<nobj-1;i++)
						  newarray[i]=array[i];
					 delete[] array;
				}
				array=newarray;
		  }
		  array[nobj-1]=pitem;
		  return pitem->object;
	 }

	 /// Go to the first object
	 /** Causes the internal object pointer (pthisitem) to point to the first
	  *  MyObjectType object in the list array. Returns false if the list array
	  *  is empty.
	  */
	 bool firstobj() {
		  if (pfirstitem) {
				pthisitem=pfirstitem;
				isobj=true;
				thisobj=0;
				return true;
		  }
		  return false;
	 }

	 /// Go to the next object
	 /** Causes the internal object pointer to point to the next MyObjectType
	  *  object in the list array. Returns false if the last item has already
	  *  been reached.
	  */
	 bool nextobj() {
		  if (pthisitem) {
				pthisitem=pthisitem->pnext;
				thisobj++;
		  }
		  if (pthisitem) return true;
		  isobj=false;
		  return false;
	 }

	 /// Returns current object
	 /** Returns a reference to the object currently pointed to by the internal
	  *  object pointer. Do not call this function unless the pointer is
	  *  pointing to a valid object (isobj=true).
	  */
	 tdata& getobj() {
		  return pthisitem->object;
	 }

	 /// Returns i'th object
	 /** Overload of the array brackets operator which returns a reference to
	  *  the i'th MyObjectType item in the list array. Do not call unless the
	  *  internal object pointer is pointing to a valid object. Note that this
	  *  function does NOT affect the value of the internal pointer.
	  */
	 tdata& operator[](unsigned int i) {
		  return array[i]->object;
	 }

	 /// Removes the object currently pointed to by the internal pointer.
	 void killobj() {
		  unsigned int i;
		  if (!pthisitem) return;
		  if (pthisitem==pfirstitem) pfirstitem=pthisitem->pnext;
		  if (pthisitem==plastitem) plastitem=pthisitem->pprev;
		  Item* pnextitem=pthisitem->pnext;
		  if (pthisitem->pprev) pthisitem->pprev->pnext=pthisitem->pnext;
		  if (pthisitem->pnext) pthisitem->pnext->pprev=pthisitem->pprev;
		  delete pthisitem;
		  pthisitem=pnextitem;
		  if (!pthisitem) isobj=false;
		  for (i=thisobj+1;i<nobj;i++)
				array[i-1]=array[i];
		  nobj--;
	 }
};


/// Functionality for relating runtime "progress" to real time
/** The computer model for which gutil was developed can sometimes take many
 *  hours to complete a simulation. It is desirable for users to obtain an
 *  ongoing estimate of progress and remaining simulation time. This class
 *  provides the necessary functionality.
 *
 *  Sample program:
 *
 *  \code
 *  void slow(int i) {
 *    // Do some number crunching
 *  }
 *
 *  int main() {
 *    const int STEPS=1e10;
 *    const int REPORT=1e9;
 *    int i;
 *
 *    Timer t;
 *    t.init();
 *
 *    t.settimer();
 *
 *    for (i=0;i<STEPS;i++) {
 *      t.setprogress(double(i)/double(STEPS));
 *
 *      if (!(i%REPORT)) {
 *        printf("Elapsed time: %s\n",t.elapsed.str);
 *        printf("Remaining time: %s\n",t.remaining.str);
 *      }
 *
 *      slow(i);
 *    }
 *
 *    return 0;
 *  }
 *  \endcode
 */
class Timer {
private:
	 long m_clock;
	 long lastclock;
	 double start_t;
	 double finish_t;
	 unsigned long timebase;
	 double progress;
public:
	 struct {
		  int hours;          ///< elapsed full hours since timer set
		  int minutes;        ///< elapsed full minutes past the hour since timer set
		  int seconds;        ///< elapsed full seconds past the minute since timer set
		  int milliseconds;   ///< elapsed milliseconds past the second since timer set
		  double time;
		  char str[12];       ///< elapsed time in the form "hh:mm:ss"
	 } elapsed;

	 struct {
		  int hours;          ///< full hours remaining to "finished" status
		  int minutes;        ///< full minutes past the hour remaining
		  int seconds;        ///< full seconds past the minute remaining
		  int milliseconds;   ///< milliseconds past the second remaining
		  double time;
		  char str[12];       ///< remaining time in the form "hh:mm:ss"
	 } remaining;

private:
	 void print() {
		  if (elapsed.minutes>59 || elapsed.seconds>59 || elapsed.hours>9999) strcpy(elapsed.str,"#time_err#");
		  else sprintf(elapsed.str,"%d%d:%d%d:%d%d",
							elapsed.hours/10,elapsed.hours%10,elapsed.minutes/10,
							elapsed.minutes%10,elapsed.seconds/10,elapsed.seconds%10);
		  if (remaining.minutes>59 || remaining.seconds>59 || remaining.hours>9999) strcpy(remaining.str,"#time_err#");
		  sprintf(remaining.str,"%d%d:%d%d:%d%d",
					 remaining.hours/10,remaining.hours%10,remaining.minutes/10,
					 remaining.minutes%10,remaining.seconds/10,remaining.seconds%10);
	 }

public:
	 /// Returns time since start of process in milliseconds
	 long timemilli() {
		  long newclock=(long)((double)clock()/(double)CLOCKS_PER_SEC*1000.0);
		  if (newclock>=lastclock) m_clock+=newclock-lastclock;
		  lastclock=newclock;
		  return m_clock;
	 }

	 /// Initialises the timer, called by the constructor
	 void init() {
		  m_clock=0;
		  lastclock=(long)((double)clock()/(double)CLOCKS_PER_SEC*1000.0);
		  timebase=timemilli();
		  settimer(3600.0);
	 }

	 Timer() {
		  init();
	 }

	 void update() {
		  unsigned long time=timemilli();
		  elapsed.time=(double)(time-timebase-start_t)/1000.0;
		  elapsed.hours=(int)(elapsed.time/3600.0);
		  elapsed.minutes=(int)(elapsed.time-elapsed.hours*3600)/60;
		  elapsed.seconds=(int)(elapsed.time-elapsed.hours*3600-
										elapsed.minutes*60);
		  elapsed.milliseconds=(int)(elapsed.time-elapsed.hours*3600-elapsed.minutes*60-
				elapsed.seconds);
		  remaining.time=(double)(finish_t-time+timebase)/1000.0;
		  if (remaining.time<0.0) remaining.time=0.0;
		  remaining.hours=(int)(remaining.time/3600.0);
		  remaining.minutes=(int)(remaining.time-remaining.hours*3600)/60;
		  remaining.seconds=(int)(remaining.time-remaining.hours*3600-
										  remaining.minutes*60);
		  remaining.milliseconds=(int)(remaining.time-remaining.hours*3600-
				remaining.minutes*60-remaining.seconds);
		  progress=elapsed.time*1000.0/(finish_t-start_t);
		  print();
	 }

	 /// Commences timing
	 /**
	  *  \param duration gives number of seconds to "finished" status
	  */
	 void settimer(double duration) {
		  unsigned long time=timemilli();
		  start_t=time-timebase;
		  finish_t=start_t+duration*1000.0;
		  update();
	 }

	 /// Sets one hour to "finished" status
	 void settimer() {
		  settimer(3600.0);
	 }

	 /// Specifies fractional progress (in range 0-1) towards "finished" status
	 /** Modifies time remaining to "finished"
	  */
	 void setprogress(double p) {
		  if (p>1.0) p=1.0;
		  update();
		  if (p>1.0e-10) finish_t=start_t+elapsed.time*1000.0/p;
		  update();
	 }

	 /// Returns fractional progress (in range 0-1) towards "finished" status
	 double getprogress() {
		  update();
		  return progress;
	 }
};


/// Writes date and time in standard Unix format to an xtring argument
/** e.g. "Tue Nov 06 10:17:47 2001"
 */
void unixtime(xtring& result);


/// Reads text according to FORTRAN-style format specification
/** Function synopsis: bool readfor(FILE* strm, xtring format, ...)
 *
 *  Reads in ASCII text data from input stream strm, according to a FORTRAN-style
 *  format specification given in the string format. Addresses of the variables to be
 *  assigned to are listed, in order of assignment, in the ellipsis (...) argument list
 *  of the function. By default, any characters remaining on the current line are
 *  discarded when the statement terminates (this behaviour can be overridden by a $
 *  specifier in the format string - see below). The function returns true if all
 *  specified values could be read in and assigned, or false if an end-of-file
 *  condition prevented some values from being read in and assigned.
 *
 *  Version modified 2005-09-13
 *  The only change is that "a" format now reads only as far as next white space
 *  character (or comma, or separator specified in format string), instead of end of
 *  line as previously. The read to end of line specify "a#" in format string.
 *
 *  Example: The following code opens for reading a text file called "climate.txt",
 *  reads in two values of type double, one integer, and 12 further values of type
 *  double, and assigns these values, respectively, to variables lon, lat, elev, and
 *  the 12 elements of array mdata. Note that the arguments following the format
 *  string are addresses of the variables to be assigned to - use the '&' prefix for
 *  simple variables; omit the '&' if specifying an array name (a pointer).
 *
 *  \code
 *  #include <stdio.h>
 *  #include <gutil.h>
 *
 *  FILE* in;
 *  double lon,lat;
 *  int elev;
 *  double mdata[12];
 *
 *  in=fopen("climate.txt","rt");
 *  if (!readfor(in,"f6.1,f5.1,i4,12f5.1",&lon,&lat,&elev,mdata))
 *     printf("Warning: not all values could be read in\n");
 *  \endcode
 *
 *  \section format Format specifiers
 *
 *  A subset of the format specifiers defined for input in the FORTRAN-77 language are
 *  supported by function readfor. Note that there are some differences in the way
 *  these are implemented compared to FORTRAN. Specifier fields in the format string
 *  are normally separated by commas. Each instance of an F, I or A specifier is
 *  assumedto correspond to one argument in the ellipsis argument list. Specifier
 *  syntax is case-insensitive and spaces and tabs in the format string are ignored.
 *
 *  Important: a value (other than 1) for n (number of items), if specified as part of
 *  an F, I or A specification, assumes the values read are to be assigned to
 *  consecutive elements of an array, whose starting address is given by a single
 *  argument in the argument list. Do not use this feature to assign multiple values
 *  with the same input format to multiple consective arguments.
 *
 *  \subsection float Floating point specifier: nFw.dEe
 *
 *    Reads one or more floating point numbers, assigning each value to a variable of
 *    type double.
 *
 *    - n - the number of items to read in; if n is not specified, one value is assumed.
 *          See cautionary note above.
 *    - w - the number of characters to read in for each item; if not specified,
 *          characters are read up to the next space, tab, end-of-line, or the next
 *          instance on the current line of a separator character appearing immediately
 *          after the specifier in the format string (see below).
 *    - d - the number of digits in the fractional part of the number; if a decimal point
 *          is encountered, this overrides the specified value. If d is omitted and no
 *          decimal point is encountered, the input string is interpreted as a whole
 *          number.
 *    - e - the number of digits in the exponent part of the number; if the character 'E'
 *          or 'e' is encountered, this overrides the specified value. If e is omitted,
 *          the exponent 0 (i.e., 10^0 = 1) is assumed.
 *
 *    Examples:
 *
 *    \verbatim
      Input stream: 1234 5.67
      Format spec    Strings read    Values assigned
      F              "1234"          1234.0
      F3             "123"           123.0
      F3.1           "123"           12.3
      F4.2E1         "1234"          1.23E+04
      2F4.2          "1234", " 5.6"  12.34, 5.6
      \endverbatim
 *
 *  \subsection integer Integer specifier: nIw
 *
 *    Reads one or more integers, assigning each value to a variable of type int.
 *
 *    - n -  the number of items to read in; if n is not specified, one value is assumed.
 *           See cautionary note above.
 *    - w -  the number of characters to read in for each item; if not specified,
 *           characters are read up to the next space, tab, end-of-line, or the next
 *           instance on the current line of a separator character appearing immediately
 *           after the specifier in the format string (see below).
 *
 *    Examples:
 *
 *    \verbatim
      Input stream: 123 4567
      Format spec    Strings read    Values assigned
      I              "123"           123
      I2             "12"            12
      2I4            "123 ", "4567"  123, 4567
      \endverbatim
 *  \subsection char Character string specifier: nAw
 *
 *    Reads one or more character strings, assigning each to a variable of type xtring.
 *
 *    - n -  the number of items to read in; if n is not specified, one value is assumed.
 *
 *    - w -  the number of characters to read for each item; if not specified, characters
 *           are read up to the next white space character, comma, or the next instance of
 *           a separator character appearing immediately after the specifier in the format
 *           string (see below).
 *
 *    Specify # after specified to read to end of line (see below)
 *
 *    Examples:
 *
 *    \verbatim
      Input stream: BERT HIGGINS
      Format spec    Strings read and assigned
      A              "BERT"
      A#             "BERT HIGGINS"
      A2             "BE"
      2A6            "BERT H", "IGGINS"
      \endverbatim
 *
 *  \subsection pos Position specifier: nX
 *
 *    Advances one or more characters on the input stream.
 *
 *    - n - The number of characters to read and discard; if n is omitted, a single
 *          character is read in and discarded.
 *
 *  \subsection eol End-of-line specifier: /
 *
 *    Advances to the end of the current line on the input stream. Input continues from
 *    the start of the next line.
 *
 *  \subsection suppress Suppress line feed specifier: $
 *
 *    If given as the last significant character in the format string, suppresses
 *    reading and discarding of the remainder of the current line on the input stream.
 *
 *  \subsection readeol Read to end of line specifier: #
 *
 *    Continues reading to end of line (intended for use with character string (A)
 *    specifier).
 *
 *  \subsection sep Separator character specifier:
 *
 *    Any character other than a space, tab or comma, that cannot be interpreted as part
 *    of one of the specifiers above, is interpreted as a separator character, and
 *    causes input up to and including the first instance of the specified separator
 *    character on the current line. If given immediately following a variable-width F,
 *    I or A specification, input continues, for each item, until an instance of the
 *    specified separator character is encountered, or the end of the current line is
 *    reached (otherwise input continues until a space, tab or the end of the line is
 *    reached). If the separator character follows a fixed-width F, I or A specification,
 *    or forms a separate specification field, the characters read are discarded.
 *
 *    Examples:
 *
 *    \verbatim
      Input stream: 123; Hello World!; 3.142
      Format spec    Strings read                     Values assigned
      I;A;F          "123", " Hello World!", "3.142"  123, " Hello World!", 3.142
      F2.1;;I2,A     "12", " 3", ".142"               1.2, 3, ".142"
		\endverbatim
 *
 */
bool readfor(FILE* in, const char* fmt, ...);


/// Used by functions with variable number of arguments to print to a string
/** Function synopsis: void formatf(xtring& output, xtring& format, void* parglist)
 *
 * Translates a printf-style format string (format) and a set of "ellipsis" arguments
 * whose starting address is given by parglist to an output xtring (output)
 */
void formatf(xtring& output,char* format,va_list& v);


/// Checks if a file exists
/**
 * \returns true if the specified filename exists and can be opened for
 *               reading, otherwise false
 */
bool fileexists(const xtring& filename);

#endif // GUTIL_H
