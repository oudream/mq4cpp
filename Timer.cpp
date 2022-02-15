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
#include "Logger.h"
#include "Timer.h"
#include <time.h>

Timer* Timer::itsDefaultTimer=NULL;

Wakeup::Wakeup(Wakeup& o) :Message("Wakeup") // ++ v1.5
{
	TRACE("Wakeup constructor")
	itsTargetQueue=o.itsTargetQueue; 
	memcpy(&itsTimeVal,&o.itsTimeVal,sizeof(_TIMEVAL));
	memcpy(&itsWakeTime,&o.itsWakeTime,sizeof(_TIMEDIFF));
	itsRepeatFlag=o.itsRepeatFlag;
	itsSender=o.itsSender;
}

Wakeup::Wakeup(MessageQueue* theQueue,long ms,bool repeat)
		  :Message("Wakeup"), itsTargetQueue(theQueue->getID()), itsRepeatFlag(repeat)
{
	TRACE("Wakeup constructor")
#if WIN32
	ftime(&itsTimeVal);
	itsWakeTime.time=ms/1000;
	itsWakeTime.millitm=(unsigned)ms%1000;
#else
	gettimeofday(&itsTimeVal, NULL);
	TRACE("Current time=" << itsTimeVal.tv_sec << "." << itsTimeVal.tv_usec)

	itsWakeTime.tv_sec=ms/1000;
	itsWakeTime.tv_usec=(ms - (itsWakeTime.tv_sec * 1000))*1000;
	TRACE("Delta wake time=" << itsWakeTime.tv_sec << "." << itsWakeTime.tv_usec)
#endif
}
	
Wakeup::~Wakeup() 
{
	TRACE("Wakeup destructor")
}

Message* Wakeup::clone() // ++ v1.5
{
	return new Wakeup(*this);	
}

void Wakeup::setTime()
{
#if WIN32
	ftime(&itsTimeVal);
#else
	gettimeofday(&itsTimeVal, NULL);
	TRACE("Current time=" << itsTimeVal.tv_sec << "." << itsTimeVal.tv_usec)
#endif
}

bool Wakeup::isExpired()
{
#if WIN32
	timeb aTimeVal;
	timeb aDeltaTime;	
	ftime(&aTimeVal);
	aDeltaTime.time = aTimeVal.time - itsTimeVal.time;

	if(aTimeVal.millitm >= itsTimeVal.millitm)
	{
		aDeltaTime.millitm = aTimeVal.millitm - itsTimeVal.millitm;
	}
	else
	{
		--aDeltaTime.time;
		aDeltaTime.millitm = aTimeVal.millitm +1000 - itsTimeVal.millitm;
	}

	TRACE("Current time=" << itsTimeVal.time << "." << itsTimeVal.millitm)
	TRACE("Time=" << aTimeVal.time << "." << aTimeVal.millitm)
	TRACE("Delta time=" << aDeltaTime.time << "." << aDeltaTime.millitm)

	if((aDeltaTime.time    == itsWakeTime.time   ) ?
	   (aDeltaTime.millitm >= itsWakeTime.millitm) :
	   (aDeltaTime.time    >= itsWakeTime.time   ) )
	{
		return true;
	}

#else

	timeval aTimeVal;
	timeval aDeltaTime;
	gettimeofday(&aTimeVal, NULL);
    
	aDeltaTime.tv_sec = aTimeVal.tv_sec - itsTimeVal.tv_sec;
	aDeltaTime.tv_usec = aTimeVal.tv_usec - itsTimeVal.tv_usec;
	if (aDeltaTime.tv_usec < 0) 
	{
		--aDeltaTime.tv_sec;
		aDeltaTime.tv_usec += 1000000;
	}

	TRACE("Current time=" << itsTimeVal.tv_sec << "." << itsTimeVal.tv_usec)
	TRACE("Delta time=" << aDeltaTime.tv_sec << "." << aDeltaTime.tv_usec)

	if((aDeltaTime.tv_sec  == itsWakeTime.tv_sec ) ?
	   (aDeltaTime.tv_usec >= itsWakeTime.tv_usec) :
	   (aDeltaTime.tv_sec  >= itsWakeTime.tv_sec ) )
	{
		return true;
	}

#endif

	return false;
}

void Wakeup::toStream(ostream& theStream)
{
	char aTimeString[40];
	time_t aTime=time(NULL);
	strftime(aTimeString,sizeof(aTimeString),"%Y-%m-%d %H:%M:%S", localtime(&aTime));		
#if WIN32
	theStream << aTimeString << ": Timer wakeup=" << (unsigned long)itsTimeVal.time 
	          << "." << (unsigned long)itsTimeVal.millitm << endl;
#else
	theStream << aTimeString << ": Timer wakeup=" << (unsigned long)itsTimeVal.tv_sec 
	          << "." << (unsigned long)itsTimeVal.tv_usec << endl;
#endif
}

// ++ v1.1
unsigned long Timer::time() 
{
	_TIMEVAL aTimeVal;	
#if WIN32
	ftime(&aTimeVal);
	return aTimeVal.time;
#else
	gettimeofday(&aTimeVal, NULL);
	return aTimeVal.tv_sec;
#endif
}

_TIMEVAL Timer::timeExt() 
{
	_TIMEVAL aTimeVal;	
#if WIN32
	ftime(&aTimeVal);
	return aTimeVal;
#else
	gettimeofday(&aTimeVal, NULL);
	return aTimeVal;
#endif
}

long Timer::subtractMillisecs(_TIMEVAL* x,_TIMEVAL* y)
{
#ifdef WIN32
	long elapse_milli=y->millitm - x->millitm;
	long elapse_sec=y->time - x->time;
#else
	long elapse_milli=(y->tv_usec - x->tv_usec)/1000L;
	long elapse_sec=y->tv_sec - x->tv_sec;
#endif

	if (elapse_milli < 0)
	{
	    elapse_milli += 1000;
	    --elapse_sec;
	}

	if(elapse_sec < 0)
		elapse_milli=-elapse_milli;
	
	long res=elapse_sec * 1000L + elapse_milli;
	TRACE("sec=" << elapse_sec << " milli=" << elapse_milli << " res=" << res )

	return res;
}

Timer::Timer() 
      :Thread("DefaultTimer")
{
	TRACE("Default Timer constructor")
	start();
}

Timer::Timer(const char* theTimerName)
	  :Thread(theTimerName)
{
	TRACE("Timer constructor")
	start();
}
	
Timer::~Timer() 
{	
	TRACE("Timer destructor")
	stop(false);
	free();
}

void Timer::schedule(Wakeup* theWakeup)
{
	TRACE("Timer::schedule - start")

	if(isShuttingDown())
	{
		if(theWakeup->repeat()==false)
			delete theWakeup;
		TRACE("Timer::schedule: Action aborted on shutdown")
		return;
	}
	
	try 
	{
		wait();		
		push(theWakeup);
		release();
	}
	catch(Exception& ex) 
	{
		release();
		onException(ex);
	}
	
	TRACE("Timer::schedule - end")		
}

#ifndef WIN32 
#define TESTCANCEL \
	if(itsRunningFlag==false) break; \
	pthread_testcancel(); 
#else
#define TESTCANCEL \
	if(itsRunningFlag==false) break; 
#endif

void Timer::run() 
{
	TRACE("Timer::run - start")
	
	while(true)
	{		
		try 
		{												
			TESTCANCEL

			wait();
			TRACE("Timer::run - executing all wakeup")

			forEach(); // Iterate all elements in queue

			TRACE("Timer::run - wakeup done")
			release();	

			TESTCANCEL
				
			sleep(10); // v1.7 - wait 10 ms
		}
		catch(Exception& ex) 
		{
			release();
			onException(ex);
		}
		catch(...)
		{
			release();
			TRACE("Timer::run : Unhandled exception")
		}
	}
	
	TRACE("Timer::run - end")		
}

bool Timer::onIteration(LinkedElement* theElement)
{
	TRACE("Timer::onIteration - start")

	if(itsRunningFlag==false || isShuttingDown())
		return false;			

	try
	{
		Wakeup* aWakeup=(Wakeup*)theElement->getObject();
		
		if(aWakeup->isExpired())
		{
			aWakeup->setTime();
			Message* aNewWakeup=aWakeup->clone(); //++ v1.5
			aNewWakeup->setSender(0); //++ v1.5
			
			TRACE("Wakeup detected")			
			MessageQueue::post(aWakeup->getQueue(),aNewWakeup); //v1.5
			TRACE("Wakeup sent")
			
			if(aWakeup->repeat()==false)
			{
				theElement->remove();
				delete theElement;
				itsElementCount--; //++ v1.5
				delete aWakeup;
				TRACE("Wakeup removed")
			}
		}
	}
	catch(Exception& exc)
	{
		onException(exc);	
	}
	
	TRACE("Timer::onIteration - end")
	return true;
}

void Timer::onException(Exception& ex)
{
	CRITICAL(ex.getMessage().c_str())
}

void Timer::postToDefaultTimer(Wakeup* theMessage)
{
	TRACE("Timer::postToDefaultTimer - start")
	
	if(itsDefaultTimer==NULL)
		itsDefaultTimer=new Timer();
	
	if(theMessage)
		itsDefaultTimer->schedule(theMessage);

	TRACE("Timer::postToDefaultTimer - end")
}	

void Timer::waitForCompletion()
{ 
	TRACE("Timer::waitForCompletion - start")
	if(itsDefaultTimer!=NULL)
	{	
		delete itsDefaultTimer;
		itsDefaultTimer=NULL;
	}
	TRACE("Timer::waitForCompletion - end")
}

