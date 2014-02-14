
#ifndef KRUNCH_X11_GRID_H
#define KRUNCH_X11_GRID_H
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vector>
#include <stdexcept>
#include <iomanip.h>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <iostream>
#include <deque>
#include <utility>

#include "keystrokes.h"
#include "x11methods.h"
namespace X11Grid
{
	using namespace std;
	using namespace X11Methods;

	struct Card
	{
		Card(const unsigned long _id) : id(_id) {}
		virtual void operator()(Pixmap& bitmap,const int x,const int y,Display* display,GC& gc,X11Methods::InvalidArea& invalid) = 0;
		operator const unsigned long (){return id;}
		virtual void cover(Display*,GC&,Pixmap&,unsigned long,X11Methods::InvalidArea& invalid,const int Y,const int Y) = 0;
		protected:
		const unsigned long id;
	};

	struct CardCover
	{
		CardCover(Card* _card,const unsigned long _color,const int _x,const int _y) :
			card(_card),color(_color),x(_x),y(_y) {}
		CardCover(const CardCover& c) : card(c.card),color(c.color), x(c.x),y(c.y) {}
		void operator=(const CardCover& c) {card=c.card; color=c.color; x=c.x; y=c.y; }
		Card* card; 
		unsigned long color;
		int x,y;
	};

	struct Cell;
	struct GridBase : map<string,int>
	{
		GridBase() : nextid(0) {}
		operator const unsigned long () { return ++nextid; }
		virtual void operator()(const unsigned long color,Pixmap&  bitmap,const int x,const int y) = 0;
		virtual int operator()(Card&,Pixmap&,const int x,const int y) = 0;
		virtual Cell& operator[](Point& p) = 0;
		friend ostream& operator<<(ostream&,GridBase&);
		virtual ostream& operator<<(ostream& o) { for (iterator it=begin();it!=end();it++) o<<it->first<<":"<<setw(8)<<it->second<<" "; return o;}
		virtual void cover(Card*,unsigned long color,const int x,const int y) = 0;
		private:
		unsigned long nextid;
	};
	inline ostream& operator<<(ostream& o,GridBase& b){return b.operator<<(o);}

	struct Cell 
	{
		Cell(GridBase& _grid,const int _x,const int _y)
			: grid(_grid), X(_x), Y(_y),color(0X00FF00),deactivate(false),active(true) {}
		void operator=(unsigned long _color){color=_color;}
		void remove(){deactivate=true;}
		bool update() { return !active; }
		virtual void operator()(Pixmap& bitmap)
		{ 
			if (deactivate) {color=0X3333; active=false;}
			if (!cards.empty()) 
			{
				for (map<unsigned long,Card*>::iterator cit=cards.begin();cit!=cards.end();cit++) 
					grid(*cit->second,bitmap,X,Y);
			} else grid(color,bitmap,X,Y);
		}
		virtual void operator+=(Card* c)
		{
			if (!c) return;
			const unsigned long id(*c);
			cards[id]=c;
		}
		virtual void operator-=(Card* c)
		{
			if (!c) return;
			const unsigned long id(*c);
			map<unsigned long,Card*>::iterator it(cards.find(id));
			if (it==cards.end()) return;
			cards.erase(it);
			if (cards.empty()) active=false;
			grid.cover(c,0X3333,X,Y);
		}
		private:				
		GridBase& grid;
		const int X,Y;
		unsigned long color;
		bool deactivate,active;
		map<unsigned long,Card*> cards;
	};

	struct Column : map<int,Cell>
	{
		Column(GridBase& _grid,const int _position) : grid(_grid),X(_position) {}
		Cell& operator[](Point& p)
		{
			iterator found(find(p.second));
			if (found!=end()) return found->second;
			insert(make_pair<int,Cell>(p.second,Cell(grid,p.first,p.second)));
			iterator it(find(p.second));
			if (it==end()) throw runtime_error("Cannot create column");
			return it->second;
		}
		bool update()
		{
			if (empty()) return true;
			for (iterator it=begin();it!=end();it++) 
				if (it->second.update()) erase(it);
			if (empty()) return true;
			return false;
		}
		virtual void operator()(Pixmap& bitmap)
			{ for (iterator it=begin();it!=end();it++) it->second(bitmap); }
		private:
		GridBase& grid;
		const int X;
	};

	template <typename DS>
		struct Row : map<int,Column>
	{
		Row(GridBase& _grid) : grid(_grid) {}
		virtual void update()
		{
			grid.clear();
			for (iterator it=begin();it!=end();it++) 
				if (it->second.update()) erase(it);
		}
		virtual void operator()(Pixmap& bitmap)
			{ for (iterator it=begin();it!=end();it++) it->second(bitmap); }
		Cell& operator[](Point& p)
		{
			iterator found(find(p.first));
			if (found==end()) insert(make_pair<int,Column>(p.first,Column(grid,p.first)));
			iterator it(find(p.first));
			if (it==end()) throw runtime_error("Cannot create row");
			return it->second[p];
		}
		private:
		GridBase& grid;
	};

	template <typename DS>
		struct Grid : Canvas, DS::RowType, GridBase
	{
		Grid(Display* _display,GC& _gc,const int _ScreenWidth, const int _ScreenHeight)
			: Canvas(_display,_gc,_ScreenWidth,_ScreenHeight), DS::RowType(static_cast<GridBase&>(*this)),
				updateloop(0) {}
		protected:
		virtual void update() { }
		virtual void operator()(Pixmap& bitmap)
		{ 
			for (vector<CardCover>::iterator coverit=coverup.begin();coverit!=coverup.end();coverit++)
			{
				CardCover& p(*coverit);
				p.card->cover(display,gc,bitmap,p.color,invalid,p.x,p.y);
			}
			coverup.clear();
			invalid.insert(Rect(10,100,ScreenWidth-160,140));	
			DS::RowType::operator()(bitmap); 
		}
		unsigned long updateloop;
		Rect paint;
		virtual void operator()(const unsigned long color,Pixmap&  bitmap,const int x,const int y) {}
		virtual int operator()(Card& card,Pixmap& bitmap,const int x,const int y)
			{ card(bitmap,x,y,display,gc,invalid); }
		private:
		virtual Cell& operator[](Point& p) { return DS::RowType::operator[](p); }
		virtual void cover(Card* c,unsigned long color,const int x,const int y)
		{
			CardCover cover(c,color,x,y);
			coverup.push_back(cover);
		} 
		vector<CardCover> coverup;
	};


	struct DefaultStructure
	{
		typedef Program ProgramType;
		typedef Grid<DefaultStructure> GridType;
		typedef Row<Column> RowType;
	};


	template <typename DS>
		inline int x11main(int argc,char** argv,KeyMap& keys)
	{
		XSizeHints displayarea;
		Display *display(XOpenDisplay(""));
		int screen(DefaultScreen(display));
		const unsigned long mybackground(0X3333);
		const unsigned long myforeground(0X3333);

		displayarea.x = 000;
		displayarea.y = 000;
		displayarea.width = 1280;
		displayarea.height = 1024;
		displayarea.flags = PPosition | PSize;

		Window window(XCreateSimpleWindow(display,DefaultRootWindow(display), displayarea.x,displayarea.y,displayarea.width,displayarea.height,5,myforeground,mybackground));
		GC gc(XCreateGC(display,window,0,0));
		XSetBackground(display,gc,mybackground);
		XSetForeground(display,gc,myforeground);
		XSelectInput(display,window,ButtonPressMask|KeyPressMask|ExposureMask);
		XMapRaised(display,window);

		stringstream except;
		try
		{
			typename DS::GridType canvas(display,gc,displayarea.width, displayarea.height);
			typename DS::ProgramType program(screen,display,window,gc,NULL,canvas,keys,displayarea.width,displayarea.height);
			program(argc,argv);
		}
		catch(runtime_error& e){except<<"runtime error:"<<e.what();}
		catch(...){except<<"unknown error";}
		if (!except.str().empty()) cout<<except.str()<<endl;

		XFreeGC(display,gc);
		XDestroyWindow(display,window);
		XCloseDisplay(display);
		return 0;
	}
} // X11Grid
#endif  //KRUNCH_X11_GRID_H


