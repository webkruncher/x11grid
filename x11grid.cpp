/*
* Copyright (c) Jack M. Thompson WebKruncher.com, exexml.com
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the WebKruncher nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Jack M. Thompson ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Jack M. Thompson BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "x11grid.h"
#include <math.h>

using namespace X11Grid;

#define NPATTERNS 2 

struct PatternX : PatternBase
{
	friend struct PatternBase;
	private: PatternX(){}
	virtual void operator ()(const int w,const int h) 
	{
		const int t( (rand()%h)-(rand()%h));
		for (int j=(w/4);j<((w/4)*3);j++) push((j/2)+t,(j)+t);
		for (int j=(w/4);j<((w/4)*3);j++) push((j/2)+t,(w-j)+t);
	}
};

struct Circles : PatternBase
{
	friend struct PatternBase;
	private: Circles(){}
	virtual void operator ()(const int w,const int h) 
	{
		const double cx(w/2);
		const double cy(h/2);
		double n(0);
		double r((rand()%20)+5);
		while (n<100)
		{
			push(cx+(sin(n)*r),cy+(cos(n))*r);
			n+=0.8;
		}
	}
};

struct Sine : PatternBase
{
	friend struct PatternBase;
	private: Sine(){}
	virtual void operator ()(const int w,const int h) 
	{
		const double cx(w/2);
		for (int cy=0;cy<h;cy+=(h/5))
		{
			double n(0);
			double r((rand()%4)+1);
			while (n<w)
			{
				push(n,cy+(sin(n))*r);
				push(n,cy+(cos(n))*r);
				n+=2;
			}
		}
	}
};

PatternBase* PatternBase::generate(const int w,const int h)
{
	PatternBase* p(NULL);
	#if 1
		const int r(rand()%NPATTERNS);
	#else
		const int r(NPATTERNS-1);
	#endif
	switch(r)
	{
		case 0:p=new PatternX; break;
		case 1:p=new Circles; break;
		case 2:p=new Sine; break;
		default: throw string("Invalid pattern index");
	}
	(*p)(w,h);
	return p;
}

