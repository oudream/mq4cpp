///////////////////////////////////////////////////////////////////////////////
// MQ4CPP - Message queuing for C++
// Copyright (C) 2004-2007  Riccardo Pompeo (Italy)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef __LINKEDLIST__
#define __LINKEDLIST__

#ifdef WIN32
#include <windows.h>
#endif

class LinkedElement
{
protected:
	void* itsObject;
	LinkedElement* itsPreviousElement;
	LinkedElement* itsNextElement;

public:
	LinkedElement(void* theObject);
	virtual ~LinkedElement();
	virtual void* getObject();
	virtual LinkedElement* getPrevious();
	virtual LinkedElement* getNext();
	virtual void setPrevious(LinkedElement* theElement);
	virtual void setNext(LinkedElement* theElement);
	virtual void insert(LinkedElement* thePreviousElement,LinkedElement* theNextElement);
	virtual void insertBefore(LinkedElement* theElement);
	virtual void insertAfter(LinkedElement* theElement);
	virtual void append(LinkedElement* theElement);
	virtual void remove();	
};

class LinkedList : protected LinkedElement
{
protected:

#ifdef WIN32
	LONG volatile itsElementCount;
#else
	int itsElementCount;
#endif

	virtual bool onIteration(LinkedElement* theElement);	
	virtual void deleteObject(void* theObject)=0; //++ v1.5

public:
	LinkedList();
	virtual ~LinkedList();
	virtual int elements() { return itsElementCount; };
	virtual void push(void* theObject);
	virtual bool isEmpty();
	virtual void* pop();
	virtual void free();
	virtual void forEach(bool theFromLastFlag=false);
};

#endif

