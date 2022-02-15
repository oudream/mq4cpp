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

#ifndef __GARBAGECOLLECTOR__
#define __GARBAGECOLLECTOR__

#include "LinkedList.h"
#include "Thread.h"

class GarbageCollector : protected LinkedList, protected Thread
{
public:	
	GarbageCollector(const char* theName) : Thread(theName) {};
			
	~GarbageCollector() 
	{
		wait();
		free();
		release();
	};

	void add(void* theObj)
	{
		wait();
		push(theObj);
		release();
	};

	// ++ v1.1
	void flush()
	{
		wait();
		free();
		release();
	};
};

#endif
