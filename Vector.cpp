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
#include "Vector.h"

#ifdef WIN32
#define ASSIGN_PTR(dest,val)  InterlockedExchangePointer((volatile PVOID*)&dest,val)
#else
#define ASSIGN_PTR(dest,val)  dest=val
#endif

Vector::Vector()
{
	TRACE("Vector::Vector - start")
	for(int cnt=0;cnt<VECBLKSIZE;cnt++)
		ASSIGN_PTR(itsArray[cnt],0);
	TRACE("Vector::Vector - end")
}

Vector::~Vector()
{
	TRACE("Vector::~Vector - start")
	for(int cnt=0;cnt<VECBLKSIZE;cnt++)
	{
		if(itsArray[cnt]!=0)
			delete [] itsArray[cnt];
	}
	TRACE("Vector::~Vector - end")
}

void Vector::set(unsigned short thePosition,void* theObject)
{
	TRACE("Vector::set - start")
	unsigned short high=thePosition >> 8;
	unsigned short low=thePosition & 0xff;
	void** block;
	
	if(itsArray[high]==0)
	{
		block=new void*[VECBLKSIZE];
		ASSIGN_PTR(itsArray[high],block);		
		for(int cnt=0;cnt<VECBLKSIZE;cnt++)
			ASSIGN_PTR(block[cnt],0);
	}
	else
	{	
		block=(void**)itsArray[high];
	}

	ASSIGN_PTR(block[low],theObject);	
	TRACE("Vector::set - end")
}

void* Vector::at(unsigned short thePosition)
{
	TRACE("Vector::at - start")
	unsigned short high=thePosition >> 8;
	unsigned short low=thePosition & 0xff;

	if(itsArray[high]!=0)
	{
		void** block=(void**)itsArray[high];
		TRACE("Vector::at - end")
		return block[low];	
	}
	
	TRACE("Vector::at - Not found")
	return 0;	
}

void* Vector::unset(unsigned short thePosition)
{
	TRACE("Vector::unset - start")
	unsigned short high=thePosition >> 8;
	unsigned short low=thePosition & 0xff;
	void* obj=0;

	if(itsArray[high]!=0)
	{
		void** block=(void**)itsArray[high];
		obj=block[low];
		ASSIGN_PTR(block[low],0);	
	}
	
	TRACE("Vector::unset - end")
	return obj;	
}


