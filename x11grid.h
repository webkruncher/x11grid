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
#include <X11/Xatom.h>



#include "keystrokes.h"
#include "x11methods.h"
namespace X11Grid
{
	using namespace std;
	using namespace X11Methods;

	struct Card
	{
		Card(const unsigned long _id) : id(_id) {}
		virtual void operator()(Pixmap& bitmap,const int x,const int y,Display* display,GC& gc,X11Methods::InvalidBase& invalid) = 0;
		operator const unsigned long (){return id;}
		virtual void cover(Display*,GC&,Pixmap&,unsigned long,X11Methods::InvalidBase& invalid,const int Y,const int Y) = 0;
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
		//virtual bool operator()(XEvent&,KeyMap&) {return true;}
		virtual bool events(Pixmap& bitmap,KeyMap& keys) {return true;}
	};
	inline ostream& operator<<(ostream& o,GridBase& b){return b.operator<<(o);}

	struct Cell 
	{
		Cell(GridBase& _grid,const int _x,const int _y,const unsigned long _background)
			: grid(_grid), X(_x), Y(_y),color(0),background(_background),deactivate(false),active(true) {}
		void operator=(unsigned long _color){color=_color;}
		void remove(){deactivate=true;}
		virtual bool update(const unsigned long,const unsigned long) { return !active; }
		virtual void operator()(Pixmap& bitmap)
		{ 
			if (deactivate) {color=background; active=false;}
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
			grid.cover(c,background,X,Y);
		}
		protected:				
		GridBase& grid;
		const int X,Y;
		unsigned long color,background;
		bool deactivate,active;
		map<unsigned long,Card*> cards;
	};

	template <typename DS>
		struct Column : map<int,typename DS::CellType>
	{
		Column(GridBase& _grid,const int _position) : grid(_grid),X(_position) {}
		Cell& operator[](Point& p)
		{
			typename DS::ColumnType::iterator found(this->find(p.second));
			if (found!=this->end()) return found->second;
			insert(make_pair<int,typename DS::CellType>(p.second,typename DS::CellType(grid,p.first,p.second,0XFFFF00)));
			typename DS::ColumnType::iterator it(this->find(p.second));
			if (it==this->end()) throw runtime_error("Cannot create column");
			return it->second;
		}
		virtual bool update(const unsigned long updateloop,const unsigned long updaterate)
		{
			if (this->empty()) return true;
			for (typename DS::ColumnType::iterator it=this->begin();it!=this->end();it++) 
				if (it->second.update(updateloop,updaterate)) erase(it);
			if (this->empty()) return true;
			return false;
		}
		virtual void operator()(Pixmap& bitmap)
			{ for (typename DS::ColumnType::iterator it=this->begin();it!=this->end();it++) it->second(bitmap); }
		protected:
		GridBase& grid;
		const int X;
	};

	template <typename DS>
		struct Row : map<int,typename DS::ColumnType>
	{
		Row(GridBase& _grid) : grid(_grid) {}
		virtual void update(const unsigned long updateloop,const unsigned long updaterate)
		{
			//grid.clear();
			for (typename DS::RowType::iterator it=this->begin();it!=this->end();it++) 
				if (it->second.update(updateloop,updaterate)) this->erase(it);
		}
		virtual void operator()(Pixmap& bitmap)
			{ for (typename DS::RowType::iterator it=this->begin();it!=this->end();it++) it->second(bitmap); }
		Cell& operator[](Point& p)
		{
			typename DS::RowType::iterator found(this->find(p.first));
			if (found==this->end()) insert(make_pair<int,typename DS::ColumnType>(p.first,typename DS::ColumnType(grid,p.first)));
			typename DS::RowType::iterator it(this->find(p.first));
			if (it==this->end()) throw runtime_error("Cannot create row");
			return it->second[p];
		}
		protected:
		GridBase& grid;
	};

	template <typename DS>
		struct Grid : Canvas, DS::RowType, GridBase
	{
		Grid(Display* _display,GC& _gc,const int _ScreenWidth, const int _ScreenHeight,const unsigned long _bkcolor)
			: Canvas(_display,_gc,_ScreenWidth,_ScreenHeight), DS::RowType(static_cast<GridBase&>(*this)),
				updateloop(0),bkcolor(_bkcolor) {}
		virtual Cell& operator[](Point& p) { return DS::RowType::operator[](p); }
		protected:
		const unsigned long bkcolor;
		virtual void update() { }
		virtual void operator()(Pixmap& bitmap)
		{ 
			InvalidBase& _invalid(*this);
			for (vector<CardCover>::iterator coverit=coverup.begin();coverit!=coverup.end();coverit++)
			{
				CardCover& p(*coverit);
				p.card->cover(display,gc,bitmap,p.color,_invalid,p.x,p.y);
			}
			coverup.clear();
			DS::RowType::operator()(bitmap);
		}
		unsigned long updateloop;
		virtual void operator()(const unsigned long color,Pixmap&  bitmap,const int x,const int y) {}
		virtual int operator()(Card& card,Pixmap& bitmap,const int x,const int y)
		{ 
			InvalidBase& _invalid(*this);
			card(bitmap,x,y,display,gc,_invalid); 
		}
		virtual operator InvalidBase& () = 0;
		private:
		virtual void cover(Card* c,unsigned long color,const int x,const int y)
		{
			CardCover cover(c,color,x,y);
			coverup.push_back(cover);
		} 
		virtual bool events(Pixmap& bitmap,KeyMap& keys) {return true;}
		//virtual bool operator()(XEvent&,KeyMap&) {return true;}
		vector<CardCover> coverup;
	};


	struct DefaultStructure
	{
		typedef Program ProgramType;
		typedef Grid<DefaultStructure> GridType;
		typedef Column<DefaultStructure> ColumnType;
		typedef Row<ColumnType> RowType;
		typedef Cell CellType;
	};

	inline void GetScreenSize(Display* display,int& width, int& height)
	{
		 Screen* pscr(DefaultScreenOfDisplay(display));
		 if (!pscr) return;
		 width=pscr->width;
		 height=pscr->height;
	}

	struct stringmap : public map<string,string> { };
	class CmdLine : public stringmap
	{
	public:
		int Argc()
		{
			if (!argc) Build();
			return argc;
		}
	
		char** Argv()
		{
			if (!argv) Build();
			return argv;
		}

		CmdLine() : programname("utility"),argv(NULL),argc(0),ownerofargs(false) {}
		CmdLine(int _argc,char** _argv,string _progname="utility") 
			: programname(_progname), argv(_argv),argc(_argc),ownerofargs(false)
		{
			for (int i=1;i<argc;i++)
			{
				string arg=argv[i];
				bool trip=false;
				int nexti = i+1;
				if (arg.find("-")==0)
				{
					if (nexti<argc)
					{
						string next = argv[nexti];
						if (next.find("-")!=0)
						{
							(*this)[arg]=next;
							trip=true;
						}
					}
				}
				if (trip) i=nexti;
				else (*this)[arg]="";
			}
		}

		virtual ~CmdLine() { if (ownerofargs) if (argv) free(argv); }
		ostream& operator<<(ostream& o)
		{
			for(iterator it=begin();it!=end();it++)
				o<<it->first<<":"<<it->second<<endl;
			return o;
		}

		bool exists(string name)
		{
			for(iterator it=begin();it!=end();it++) if (it->first==name) return true;
			return false;
		}

		string programname; // user settable
		private: int argc; char **argv; string argbuffer; bool ownerofargs;
		void Build()
		{
			if (argc) return; if (argv) return;
			ownerofargs=true;
			size_t bytes(0);
			int items(0);
			for (iterator it=begin();it!=end();it++) 
			{
				bytes+=it->first.size(); 
				items++; 
				if (it->second.size()) {bytes+=it->second.size()+1; items++;} 
			}
			argbuffer.resize(bytes);
			argv=(char**)malloc(sizeof(char*)*items);
			if (!argv) throw string("Cannot allocate argv double pointer");
			int i(0);
			for (iterator it=begin();it!=end();it++)
			{
				argv[i++]=&argbuffer[argbuffer.size()];
				if (it->first.size()) argbuffer+=it->first;
				if (it->second.size()) 
				{
					argbuffer+="\0"; 
					argv[i++]=&argbuffer[argbuffer.size()];
					argbuffer+=it->second;
				}
			}
			argc=i;
		}
	};

	inline ostream& operator<<(ostream& o,CmdLine& l){return l.operator<<(o);}

	inline Window ScreenRoot(Screen* screen,Screen* tmp=0,Window root=0)
	{
		if (screen!=tmp) 
		{
			Display *display(DisplayOfScreen(screen));
			int i;
			Window ret,parent,*children(NULL);
			unsigned int nchildren;
			root = RootWindowOfScreen(screen);
			Atom swm(XInternAtom(display,"__SWM_VROOT",False));
			if (XQueryTree(display, root, &ret, &parent, &children, &nchildren)) 
			{
				for (i = 0;i<nchildren;i++) 
				{
					Atom type; int format; unsigned long num, after; Window *subroot(NULL);
					if (XGetWindowProperty(display,children[i],swm,0,1,False,XA_WINDOW,&type,&format,&num,&after,(unsigned char **)&subroot)==Success&&subroot) 
						{root=*subroot; break;}
				}
				if (children) XFree((char *)children);
			}
			tmp=screen;
		}
		return root;
	}

	template <typename DS>
		inline int x11main(int argc,char** argv,KeyMap& keys,unsigned long bkcolor)
	{
		XSizeHints displayarea;
		Display *display;//(XOpenDisplay(""));
		display = XOpenDisplay (getenv ("DISPLAY"));
		int screen(DefaultScreen(display));
		const unsigned long background(bkcolor);
		const unsigned long foreground(bkcolor);

		displayarea.x = 000;
		displayarea.y = 000;
		GetScreenSize(display,displayarea.width,displayarea.height);
		//cout<<"Area:"<<displayarea.width<<"x"<<displayarea.height<<endl;
		displayarea.flags = PPosition | PSize;

		CmdLine cmdline(argc,argv,"life");

		XSetWindowAttributes attributes;
		Window window,parent(0);
		GC gc;
		unsigned int Class(CopyFromParent);
		int depth(CopyFromParent);
		unsigned int border(0);
		unsigned long valuemask(CopyFromParent);
		if (!cmdline.exists("-root"))
		{
			window=XCreateSimpleWindow(display,DefaultRootWindow(display), 
				displayarea.x,displayarea.y,displayarea.width,displayarea.height,5,foreground,background);
			gc=(XCreateGC(display,window,0,0));
			XSetBackground(display,gc,background);
			XSetForeground(display,gc,foreground);
			XSelectInput(display,window,ButtonPressMask|KeyPressMask|ExposureMask);
			XMapRaised(display,window);
		} else {

			long long int wid(0);
			if (cmdline.exists("-window-id"))
			{
				string sswid(cmdline["-window-id"]); sswid.erase(0,2);
				char* pEnd; wid = strtoll ((char*)sswid.c_str(), &pEnd, 16);
			}
				parent=wid; 
				if (parent)
					window=XCreateWindow(display, parent,
						displayarea.x,displayarea.y, displayarea.width,displayarea.height, 
						border, depth, Class, CopyFromParent, valuemask, &attributes);
				else window=ScreenRoot(DefaultScreenOfDisplay(display));
				gc=XCreateGC(display,window,0,0);
				if (wid) XMapRaised(display,window);
		}


		XSetForeground(display,gc,bkcolor);
		XFillRectangle(display,window,gc, displayarea.x,displayarea.y, displayarea.width,displayarea.height);

		stringstream except;
		try
		{
			typename DS::GridType canvas(display,gc,displayarea.width, displayarea.height,bkcolor);
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


	struct ProximityRectangle : X11Methods::Rect
	{
		ProximityRectangle() : x(0), y(0), proxi(false),discard(false) {} 
		ProximityRectangle(const int _x,const int _y) : x(_x), y(_y), proxi(true),discard(false) {}
		ProximityRectangle(const int _x,const int _y,const int ulx,const int uly,const int brx,const int bry) 
			: x(_x), y(_y), X11Methods::Rect(ulx,uly,brx,bry), proxi(false),discard(false) {}
		ProximityRectangle(const ProximityRectangle& a) : x(a.x),y(a.y), X11Methods::Rect(a),proxi(false),discard(false) {}
		ProximityRectangle& operator=(const ProximityRectangle& a) { x=a.x; y=a.y; X11Methods::Rect::operator=(a);proxi=false;discard=false; }
		virtual ~ProximityRectangle() { for (vector<Rect*>::iterator it=subs.begin();it!=subs.end();it++) delete (*it); }
		void operator()(const ProximityRectangle& e) 
		{ 
				//subs.push_back(new ProximityRectangle(r));	
				ProximityRectangle& r(*this);
					if (e.first.first<r.first.first) r.first.first=e.first.first;
					if (e.first.second<r.first.second) r.first.second=e.first.second;
					if (e.second.first>r.second.first) r.second.first=e.second.first;
					if (e.second.second>r.second.second) r.second.second=e.second.second;
		}
		void zero() { first.first=first.second=0; second.first=second.second=100; }
		virtual operator XPoint& ()
		{
			if (xpoints) delete[] xpoints;
			xpoints=new XPoint[4];	
			ProximityRectangle r(*this);
			if (!subs.empty()) 
			{
				for (vector<Rect*>::iterator it=subs.begin();it!=subs.end();it++)
				{
					Rect& e(**it);
					if (e.first.first<r.first.first) r.first.first=e.first.first;
					if (e.first.second<r.first.second) r.first.second=e.first.second;
					if (e.second.first>r.second.first) r.second.first=e.second.first;
					if (e.second.second>r.second.second) r.second.second=e.second.second;
				}
			}
			xpoints[0].x=r.first.first;
			xpoints[0].y=r.first.second;
			xpoints[1].x=r.second.first;
			xpoints[1].y=r.first.second;
			xpoints[2].x=r.second.first;
			xpoints[2].y=r.second.second;
			xpoints[3].x=r.first.first;
			xpoints[3].y=r.second.second;
//cout<<"P:"<<r.first<<"x"<<r.second<<endl;
			return *xpoints;
		}

		bool lessthan(ProximityRectangle& p)  
		{
			if ( !proxi ) return X11Methods::Rect::operator<(p); //const_cast<ProximityRectangle&>(p));
			if(x<p.x) return true;
			if (y<p.y) return true;
			return false;
		}
		operator const vector<Point>& () const
		{
				neighbors.push_back(Point(x-1,y));
				neighbors.push_back(Point(x-1,y+1));
				neighbors.push_back(Point(x-1,y-1));
				neighbors.push_back(Point(x+1,y));
				neighbors.push_back(Point(x+1,y+1));
				neighbors.push_back(Point(x+1,y-1));
				neighbors.push_back(Point(x,y+1));
				neighbors.push_back(Point(x,y-1));
				return neighbors;
		}
		private:
		mutable vector<Point> neighbors;
		int x,y;
		bool proxi;
		mutable bool discard;
		vector<Rect*> subs;
	};
	inline bool operator<(const ProximityRectangle& a,const ProximityRectangle& b)
	{
		return const_cast<ProximityRectangle&>(b).lessthan(const_cast<ProximityRectangle&>(a));
	}

	struct InvalidGrid : InvalidArea<ProximityRectangle>
	{
		InvalidGrid() : color(0XF){}
		void insert(const int x,const int y,ProximityRectangle r) { X11Methods::InvalidArea<ProximityRectangle>::insert(r); }
		private:
		unsigned long color;
		iterator find(const Point& p) 
		{ 
			ProximityRectangle tmp(p.first,p.second);
			return set<ProximityRectangle>::find(tmp);
		}
		//virtual void Show(Display* display,Pixmap& bitmap,Window& window,GC& gc) { return; }
		virtual void expose() 
		{
			clear();
			ProximityRectangle i(0,0,0,0,1024,768);//(x-(CW/2)),(y-(CH/2)),(x+(CW/2)),(y+(CH/2)));	
			insert(0,0,i);
		}
		virtual void reduce()
		{
			vector<ProximityRectangle> killset;
			for (iterator it=begin();it!=end();it++)
			{
				ProximityRectangle& r(const_cast<ProximityRectangle&>(*it));
				const vector<Point>& p(r);;
				for (vector<Point>::const_iterator rit=p.begin();rit!=p.end();rit++)
				{
					const Point& P(*rit);
					iterator found(find(P));
					if (found!=it)
						if (found!=end()) 
						{ 
							r(*found);
							erase(found);
						}
				}
			}
		}
	};


	struct TestPatternGenerator;
	struct PatternBase : vector<pair<double,double> >
	{
		friend struct TestPatternGenerator;
		static PatternBase* generate(const int w,const int h);
		protected:
		PatternBase(){}
		virtual void operator()(const int x,const int y) = 0;
		void push(const double x,const double y){push_back(make_pair<double,double>(x,y));}
	};

	struct TestPatternGenerator 
	{
		TestPatternGenerator(const int _w,const int _h) : w(_w),h(_h),p(NULL) {}
		virtual ~TestPatternGenerator() {if (p) delete p;}
		operator PatternBase& ()
		{
			if (p) delete p;
			p=PatternBase::generate(w,h);
			return *p;
		}
		private:
		const int w,h;
		PatternBase* p;
	};





} // X11Grid
#endif  //KRUNCH_X11_GRID_H


