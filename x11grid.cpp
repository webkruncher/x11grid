

#include "x11grid.h"
#include <math.h>

using namespace X11Grid;

#define NPATTERNS 3 

struct TestPattern : PatternBase
{
	friend struct PatternBase;
	private: TestPattern(){}
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
		double r((rand()%10)+5);
		while (n<360)
		{
			push(cx+(sin(n)*r),cy+(cos(n))*r);
			n+=0.2;
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
				n+=0.8;
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
		case 0:p=new TestPattern; break;
		case 1:p=new Circles; break;
		case 2:p=new Sine; break;
		default: throw string("Invalid pattern index");
	}
	(*p)(w,h);
	return p;
}

