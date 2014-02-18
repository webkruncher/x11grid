
#include "x11grid.h"
using namespace X11Methods;
#include <math.h>

struct TestPattern;
struct CustomRow;
struct CustomColumn;
struct CustomCell;
struct TestStructure : X11Grid::DefaultStructure
{
		typedef TestPattern GridType;
		typedef CustomRow RowType;
		typedef CustomColumn ColumnType;
		typedef CustomCell CellType;
};

	struct TestRect : X11Methods::Rect
	{
		TestRect() {}
		TestRect(const int ulx,const int uly,const int brx,const int bry) 
			: Rect(ulx,uly,brx,bry) {}
		TestRect(const TestRect& a) : X11Methods::Rect(a) {}
		TestRect& operator=(const TestRect& a) { X11Methods::Rect::operator=(a); }
		virtual operator XPoint& () 
		{
			if (xpoints) delete[] xpoints;
			xpoints=new XPoint[4];	
			xpoints[0].x=first.first;
			xpoints[0].y=first.second;
			xpoints[1].x=second.first;
			xpoints[1].y=first.second;
			xpoints[2].x=second.first;
			xpoints[2].y=second.second;
			xpoints[3].x=first.first;
			xpoints[3].y=second.second;
			return *xpoints;
		}
	};

struct CustomCell : X11Grid::Cell
{
		CustomCell(X11Grid::GridBase& _grid,const int _x,const int _y,unsigned long background)
			: X11Grid::Cell(_grid,_x,_y,background) {}
		virtual void operator()(Pixmap& bitmap)
			{ X11Grid::Cell::operator()(bitmap); }
};

struct CustomColumn : X11Grid::Column<TestStructure>
{
		CustomColumn(X11Grid::GridBase& _grid,const int _position) : X11Grid::Column<TestStructure>(_grid,_position) {}
		virtual bool update(const unsigned long updateloop,const unsigned long updaterate) 
			{ return X11Grid::Column<TestStructure>::update(updateloop,updaterate); }
};

struct CustomRow : X11Grid::Row<TestStructure>
{
		CustomRow(X11Grid::GridBase& _grid) : X11Grid::Row<TestStructure>(_grid) {}
		virtual void update(const unsigned long updateloop,const unsigned long updaterate) 
			{ X11Grid::Row<TestStructure>::update(updateloop,updaterate); }
};

struct Bubble : X11Grid::Card
{
	Bubble(X11Grid::GridBase& _grid,string _text) : grid(_grid), Card(_grid), text(_text) {}
	virtual void cover(Display* display,GC& gc,Pixmap& bitmap,unsigned long color,X11Methods::InvalidBase& _invalid,const int X,const int Y) 
	{
		TestRect r(X-50,Y-20,X+50,Y+20);	
		XPoint& points(r);
		XSetForeground(display,gc,color);
		XFillPolygon(display,bitmap,  gc,&points, 4, Complex, CoordModeOrigin);
		InvalidArea<TestRect>& invalid(static_cast<InvalidArea<TestRect>&>(_invalid));
		invalid.insert(r);
	}

	virtual void operator()(Pixmap& bitmap,const int x,const int y,Display* display,GC& gc,X11Methods::InvalidBase& _invalid)
	{
		TestRect r(X-50,Y-20,X+50,Y+20);	
		XPoint& points(r);
		XSetForeground(display,gc,0X0080FF);
		XFillPolygon(display,bitmap,  gc,&points, 4, Complex, CoordModeOrigin);
		XSetForeground(display,gc,0X8800FF);
		stringstream ss; ss<<id<<") "<<text;
		XDrawString(display,bitmap,gc,X-40,Y,ss.str().c_str(),ss.str().size());
		InvalidArea<TestRect>& invalid(static_cast<InvalidArea<TestRect>&>(_invalid));
		invalid.insert(r);
	}
	void operator()(int x,int y)
	{
			Point b(X,Y);
			grid[b]-=this;
			Point p(x,y);
			grid[p]+=this;
			X=x; Y=y;
	}
	private:
	X11Grid::GridBase& grid;
	string text;
	int X,Y;
};

struct ColorCurve
{
	ColorCurve(X11Grid::GridBase& _grid,double _sx,double _sy) : grid(_grid),sx(_sx), sy(_sy),t(0),c(0),alive(true) {}
	void operator()()
	{
		if (alive) if (c>=0XFF) {c=0XFF;return;}
		if (!alive) if (t<150) t=150;
		t++; 
		double T((t*1000)/300);
		if (T>1000) return;
		if (T<500) c=(log(T)*50);
		if (T>500) c=255-((log(T-500)*50));
		if (c>255) c=255;
		if (c<0) c=0;
		double y=-c;
		double range(0XFF-0X70);
		c=((c/0XFF)*range)+0X70;
		Point p(sx+T,y+sy);
		const unsigned long C(*this);
		grid[p]=((C<<8) | C);
	}
	operator unsigned long (){return floor(c);}
	void operator = (bool b){alive=b;}
	private:
	X11Grid::GridBase& grid;
	const double sx,sy;
	double t,c;
	bool alive;
};

  inline void reseed()
  {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC,&tp);
    tp.tv_nsec; srand(tp.tv_nsec);
  }


struct TestPattern : X11Grid::Grid<TestStructure>
{
	TestPattern(Display* _display,GC& _gc,const int _ScreenWidth, const int _ScreenHeight,const unsigned long bkcolor)
		: X11Grid::Grid<TestStructure>(_display,_gc,_ScreenWidth,_ScreenHeight,bkcolor), color(0), cx(900), cy(50), r(3),c(0),
		Root(*this,"Root Node"), Dummy(*this,"Dummy"),ping(400,300),side(false),dir(false), flip(false), limit(120), step(4),
		curve(*this,50,600),updateloop(0)
	{ 
		Root(ping.first,ping.second);
		Dummy(ping.first,ping.second);
		for (int z=0;z<5;z++)
		{
			reseed();
			unsigned long color(rand());
			int accross(ScreenWidth);
			int down(ScreenHeight);
			X11Grid::TestPatternGenerator g(accross,down);
			X11Grid::PatternBase& p(g);
			for (X11Grid::PatternBase::iterator pit=p.begin();pit!=p.end();pit++)
			{
				Point a(pit->first,pit->second);
				grid[a]=color;
			}
			usleep(1e3);
		}
	}
	virtual operator InvalidBase& () {return invalid;}
	protected:
	Bubble Root,Dummy;
	ColorCurve curve;
	void operator()(Pixmap& bitmap) 
	{ 
		//paint.clear();
		stringstream ss;
		stringstream ssupdates; ssupdates<<"Update:"<<updateloop;
		stringstream pingpong,sscolor; 
		pingpong<<"dir:"<<dir;
		pingpong<<" side:"<<side;
		pingpong<<" pong:"<<setw(5)<<pong.first<<","<<setw(5)<<pong.second;
		sscolor<<" update: "<<updateloop<<" color:"<<(unsigned long)curve;
		ss<<setw(20)<<left<<ssupdates.str();
		ss<<setw(40)<<left<<pingpong.str();
		ss<<setw(40)<<left<<sscolor.str();
		ss<<(*this);
		XSetForeground(display,gc,0X2222);
		XFillRectangle(display,bitmap,gc,10,100,ScreenWidth-160,40);
		XSetForeground(display,gc,0X7F7F7F);
		XDrawString(display,bitmap,gc,20,120,ss.str().c_str(),ss.str().size());
		X11Grid::Grid<TestStructure>::operator()(bitmap);
	}
	bool side,dir,flip; const int limit,step;
	pair<int,int> ping,pong;
	int updateloop;
	virtual void update() 
	{
		curve();
		if (updateloop==200) curve=false;
		TestStructure::RowType& grid(*this);
		if ((!pong.first) && (!pong.second)) if (flip) {side=!side; flip=false;} else flip=true;
		if ( (abs(pong.first)>limit) || (abs(pong.second)>limit) ) dir=!dir;
		if (side) pong.first+=((dir)?step:-step); else pong.second+=((dir)?step:-step);
		Dummy(ping.first+pong.first,ping.second+pong.second);
		{
				if (!color) color=rand()%0XFF;
				c+=0.1;
				double radius(r);
				for (int j=0;j<2;j++)
				{
					radius+=30;
					Point a(cx+(cos(c)*radius),cy+(sin(c)*radius));
					grid[a]=color;
					tests.push_back(a);
					color<<=1;
				}
		}
		while (tests.size()>60)
		{
			Point& p(tests.front());
			grid[p].remove();
			tests.pop_front();
		}
		TestStructure::RowType::update(updateloop,50);
		++updateloop;
	}
	private:
	unsigned long color;
	double r,c;
	int cx,cy;
	InvalidArea<TestRect> invalid;
	deque<Point> tests;
	void operator()(const unsigned long color,Pixmap&  bitmap,const int x,const int y)
	{
		TestRect r(x-2,y-2,x+2,y+2);	
		XPoint& points(r);
		XSetForeground(display,gc,color);
		XFillPolygon(display,bitmap,  gc,&points, 4, Complex, CoordModeOrigin);
		InvalidBase& _invalidbase(*this);
		InvalidArea<TestRect>& invalid(static_cast<InvalidArea<TestRect>&>(_invalidbase));
		invalid.insert(r);
	}
};


int main(int argc,char** argv)
{
	KeyMap keys;
	return X11Grid::x11main<TestStructure>(argc,argv,keys,0X333333);
}
