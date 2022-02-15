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

#define SILENT
#include "Trace.h"
#include "LinkedList.h"

#ifdef WIN32
#define ASSIGN_LONG(dest,val) InterlockedExchange(&dest,val)
#define ASSIGN_PTR(dest,val)  InterlockedExchangePointer((volatile PVOID*)&dest,val)
#define INCREMENT_LONG(dest)  InterlockedIncrement(&dest)
#define DECREMENT_LONG(dest)  InterlockedDecrement(&dest)
#else
#define ASSIGN_LONG(dest,val) dest=val
#define ASSIGN_PTR(dest,val)  dest=val
#define INCREMENT_LONG(dest)  dest++
#define DECREMENT_LONG(dest)  dest--
#endif

LinkedElement::LinkedElement(void* theObject)
			 : itsObject(theObject)

{
	ASSIGN_PTR(itsPreviousElement,this);
	ASSIGN_PTR(itsNextElement,this);
}

LinkedElement::~LinkedElement()
{
}

void* LinkedElement::getObject()
{
	return itsObject;
}

LinkedElement* LinkedElement::getNext()
{
	return itsNextElement;
}

LinkedElement* LinkedElement::getPrevious()
{
	return itsPreviousElement;
}

void LinkedElement::setPrevious(LinkedElement* theElement)
{
	ASSIGN_PTR(itsPreviousElement,theElement);
}

void LinkedElement::setNext(LinkedElement* theElement)
{
	ASSIGN_PTR(itsNextElement,theElement);
}

void LinkedElement::insert(LinkedElement* thePreviousElement,LinkedElement* theNextElement)
{
	ASSIGN_PTR(itsPreviousElement,thePreviousElement);
	ASSIGN_PTR(itsNextElement,theNextElement);
	thePreviousElement->setNext(this);
	theNextElement->setPrevious(this);
}

void LinkedElement::insertBefore(LinkedElement* theElement)
{
	theElement->insert(itsPreviousElement,this);
}

void LinkedElement::insertAfter(LinkedElement* theElement)
{
	theElement->insert(this,itsNextElement);
}

void LinkedElement::append(LinkedElement* theElement)
{
	ASSIGN_PTR(itsNextElement,theElement);
	theElement->setPrevious(this);
}

void LinkedElement::remove()
{
	itsPreviousElement->setNext(itsNextElement);
	itsNextElement->setPrevious(itsPreviousElement);
	ASSIGN_PTR(itsPreviousElement,this);
	ASSIGN_PTR(itsNextElement,this);
}

LinkedList::LinkedList() : LinkedElement(0)
{
	TRACE("LinkedList constructor")
	ASSIGN_LONG(itsElementCount,0);
}

LinkedList::~LinkedList()
{
	TRACE("LinkedList destructor")
}

void LinkedList::push(void* theObject)
{
	TRACE("LinkedList::push - start")
	LinkedElement* anElement=new LinkedElement(theObject);
	if(itsElementCount==0)
	{
		anElement->append(this);
		append(anElement);
		TRACE("Create circular link between LinkedList and LinkedElement")		
	}
	else
	{
		anElement->insert(this,getNext());
		TRACE("LinkedElement inserted in the circular list")	
	}

	INCREMENT_LONG(itsElementCount);
	TRACE("LinkedList::push - end")
}

bool LinkedList::isEmpty()
{
	if(itsElementCount==0)
		return true;
	
	return false;	
}

void* LinkedList::pop()
{
	TRACE("LinkedList::pop - start")
	void* anObject=0;
	
	if(itsElementCount>=1)
	{
		LinkedElement* anElement=getPrevious();
		anObject=anElement->getObject();
		anElement->remove();
		delete anElement;
		DECREMENT_LONG(itsElementCount);
	}

	TRACE("LinkedList::pop - end")
	return anObject;
}

void LinkedList::free()
{
	TRACE("LinkedList::free - start")
	while(itsElementCount>0)
	{
		LinkedElement* anElement=getNext();
		void* anObject=anElement->getObject();
		anElement->remove();		
		delete anElement;
		deleteObject(anObject);
		DECREMENT_LONG(itsElementCount);
	}
	TRACE("LinkedList::free - end")
}

void LinkedList::forEach(bool theFromLastFlag)
{
	TRACE("LinkedList::forEach - start")
	if(itsElementCount==0)
		return;

	LinkedElement* anElement;

	if(theFromLastFlag==true)
		anElement=getNext();
	else
		anElement=getPrevious();
	
	int max=itsElementCount;
	for(int cnt=0;cnt < max; cnt++)
	{
		LinkedElement* aNextElement;
		
		if(theFromLastFlag==true)
			aNextElement=anElement->getNext();
		else
			aNextElement=anElement->getPrevious();

		if(onIteration(anElement)==false)
			break;
			
		anElement=aNextElement;
	}
	TRACE("LinkedList::forEach - end")
}

bool LinkedList::onIteration(LinkedElement* theElement)
{
	return false;	
}

