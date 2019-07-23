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
#ifndef __X11_METHODS_H__
#define __X11_METHODS_H__
#include <X11/cursorfont.h>

namespace X11Methods
{
	using namespace std;

	void DebugEvent( XEvent& );
	struct ApplicationBase {};

	struct Point : pair<int,int> 
	{
		Point() {}
		Point(const int a,const int b) : pair<int,int>(a,b) {}
		Point(const Point& a) : pair<int,int>(a.first,a.second) {}
		bool operator<(const Point& n)
		{
			const pair<int,int>& a(*this);
			const pair<int,int>& b(n);
			return a<b;
		}
		friend ostream& operator<<(ostream&,const Point&);
		virtual ostream& operator<<(ostream& o) const { o<<first<<"x"<<second; return o;}
		void clear(){first=0;second=0;}
	};
	inline ostream& operator<<(ostream& o,const Point& b){return b.operator<<(o);}

	struct Rect : pair<Point,Point>
	{
		Rect() : xpoints(NULL) {}
		Rect(const int ulx,const int uly,const int brx,const int bry) 
			: pair<Point,Point>(make_pair<Point,Point>(Point(ulx,uly),Point(brx,bry))),xpoints(NULL) { }
		Rect(const Rect& a) : pair<Point,Point>(a),xpoints(NULL) {}
		virtual Rect& operator=(const Rect& a) { pair<Point,Point>::operator=(a); /* don't copy points */ }
		virtual ~Rect() { if (xpoints) delete xpoints; }
		friend ostream& operator<<(ostream&,const Rect&);
		virtual ostream& operator<<(ostream& o) const { o<<first<<"/"<<second; return o;}
		void clear(){first.clear();second.clear();}
		bool operator<(const Rect& r)
		{
			if (first<r.first) return true;
			if (second<r.second) return true;
			return false;
		}
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
		protected:
		mutable XPoint* xpoints;
	};
	inline ostream& operator<<(ostream& o,const Rect& b){return b.operator<<(o);}


	struct InvalidBase
	{
		InvalidBase() : trace(false) {}
		virtual void Fill(Display* display,Pixmap& bitmap,GC& gc) = 0;
		virtual void Show(Display* display,Pixmap& bitmap,Window& window,GC& gc) 
			{ if (trace) Trace(display,bitmap,window,gc,0XFF); }
		virtual void Trace(Display* display,Pixmap& bitmap,Window& window,GC& gc,const unsigned long) = 0;
		virtual void Draw(Display*,Pixmap&,Window&,GC&) = 0;
		virtual void reduce() = 0;
		virtual void expose() {}
		virtual void clear() = 0;
		void SetTrace(bool t){trace=t;}
		protected: bool trace;
	};

	template <typename R>
		struct InvalidArea : InvalidBase, set<R>
	{
		virtual void clear() { set<R>::clear(); }
		virtual void insert(R r) {set<R>::insert(r); }
		virtual void expand(R r) 
		{
			if (this->empty()) {set<R>::insert(r);  return;}
			typename set<R>::reverse_iterator last(this->rbegin());
			if (this->size()>1) throw string("Cannot expand an invalid area with more than one entry");
			const R& e(*last);
			if (r.first.first>e.first.first) r.first.first=e.first.first;
			if (r.first.second>e.first.second) r.first.second=e.first.second;
			if (r.second.first<e.second.first) r.second.first=e.second.first;
			if (r.second.second<e.second.second) r.second.second=e.second.second;
			erase(e); insert(r); 
		}
		virtual void reduce() { }
		virtual void Draw(Display* display,Pixmap& bitmap,Window& window,GC& gc) 
		{
			for (typename set<R>::iterator it=this->begin();it!=this->end();it++)
			{
				R& r(const_cast<R&>(*it));
				XPoint& points(r);
				int x(r.first.first);
				int y(r.first.second);
				int w(r.second.first-x);
				int h(r.second.second-y);
				XCopyArea(display,bitmap,window,gc,x,y,w,h,x,y); 
			}
		}

		virtual void Fill(Display* display,Pixmap& bitmap,GC& gc)
		{
			XSetForeground(display,gc,0XFFFF);
			for (typename set<R>::iterator it=this->begin();it!=this->end();it++)
			{
				R& r(const_cast<R&>(*it));
				XPoint& points(r);
				int x(r.first.first-1);
				int y(r.first.second-1);
				int w(r.second.first-x+2);
				int h(r.second.second-y+2);
				XFillRectangle(display,bitmap,gc,x,y,w,h);
			}
		}
		virtual void Trace(Display* display,Pixmap& bitmap,Window& window,GC& gc,unsigned long color)
		{
			for (typename set<R>::iterator it=this->begin();it!=this->end();it++)
			{
				color<<=4; if (color==0) color=0XFF;
				XSetForeground(display,gc,color);
				R& r(const_cast<R&>(*it));
				XPoint& points(r);
				int x(r.first.first-1);
				int y(r.first.second-1);
				int w(r.second.first-x+2);
				int h(r.second.second-y+2);
				XDrawLines(display, bitmap, gc, &points, 4, CoordModeOrigin);
				//XFillRectangle(display,bitmap,gc,x,y,w,h);
				XCopyArea(display,bitmap,window,gc,x,y,w,h,x,y); 
			}
		}
	};

	class Canvas 
	{
		friend class Application;
		public:
		virtual void operator()(ApplicationBase&,int argc,char** argv) {}
		Canvas(Display* _display,GC& _gc,const int _ScreenWidth, const int _ScreenHeight)
			: display(_display),gc(_gc),ScreenWidth(_ScreenWidth),ScreenHeight(_ScreenHeight) { }
		virtual bool operator()(XEvent&,KeyMap&) //{cout<<"Event:"<<endl; cout.flush(); return true;}
			{return true;}
		virtual bool operator()(KeyMap&) {return true;}
		virtual void operator()(Pixmap& bitmap) = 0;
		virtual void update() = 0; 
		virtual operator InvalidBase& () = 0;
		protected:
		Display* display;
		GC& gc;
		const int ScreenWidth,ScreenHeight;
		private:
	};

	class Buffer 
	{
		friend class Application;
		Buffer(const int _screen,Display* _display,Window& window,GC& _gc,Canvas& _canvas,XImage* _image,const int _ScreenWidth,const int _ScreenHeight)
			: screen(_screen),display(_display),gc(_gc),image(_image),ScreenWidth(_ScreenWidth),ScreenHeight(_ScreenHeight),
			bitmap(XCreatePixmap(_display,window,_ScreenWidth,_ScreenHeight,DefaultDepth(_display, DefaultScreen(_display)))),canvas(_canvas) { }
		~Buffer() { XFreePixmap(display,bitmap); }
		Pixmap bitmap;
		const int screen;
		Display* display;
		GC& gc;
		Canvas& canvas;
		XImage* image;
		const int ScreenWidth,ScreenHeight;
		public: operator Pixmap& ()
		{
			if (image) 
			{
				int x(0); int y(0); const int w(image->width); const int h(image->height);
				const int i((ScreenWidth/w)+1);
				const int j((ScreenHeight/h)+1);
				for (int ii=0;ii<i;ii++)
					for (int jj=0;jj<j;jj++)
						{ x=ii*w; y=jj*h; XPutImage(display,bitmap,gc,image,0,0,x,y,w,h); }
//			} else {
//				XSetForeground(display,gc,0XFFFFFF);
//				InvalidBase& invalid(canvas);
//				invalid.Fill(display,bitmap,gc);
			}
			return bitmap;
		}
	};

	class Application ;
	ostream& operator<<(ostream&,Application&); 

	class Application : public ApplicationBase
	{
		friend ostream& operator<<(ostream&,Application&); 
		ostream& operator<<(ostream& o) { o<<"App->Screen:"<<screen<<", Display"<<display<<" Screen Dimensions:"<<ScreenWidth<<" x "<<ScreenHeight<<endl; return o;	}
		Cursor cursor;
		bool Focused;
		public:
		operator Window& () { return window;}
		operator Display* () { return display;}
		operator Canvas& () { return canvas; }
		Application(const int _screen,Display* _display,Window& _window,GC& _gc,XImage* _image,Canvas& _canvas,KeyMap& _keys,const int _ScreenWidth,const int _ScreenHeight)
			: Focused(true), screen(_screen),display(_display),window(_window),gc(_gc),image(_image),canvas(_canvas),keys(_keys),ScreenWidth(_ScreenWidth),ScreenHeight(_ScreenHeight),buffers(canvas) 
		{
			cursor = XCreateFontCursor(display, XC_arrow);
		}
		virtual void operator()(int argc,char** argv)
		{
			const long long started(when());
			buffers(screen,display,window,gc,image,ScreenWidth,ScreenHeight);
			long long next(0),unext(0);
			while (true) 
			{
				//if (when(started)>next) 
				{
					Buffer& buffer(buffers);
					Pixmap& bitmap(buffer);
					canvas(*this,argc,argv);
					if (!display) return;
					canvas(bitmap);
					InvalidBase& invalid(canvas);
					draw(bitmap);
					if (!events(bitmap)) return ;
					next=(when(started)+1e2);
				}
				//if (when(started)>unext) 
				{
					update();
					unext=(when(started)+1e2);
				}
				usleep(1e2);
			}
		}
		protected:
		virtual void update() { }
		Canvas& canvas;
		KeyMap& keys;
		virtual void draw(Pixmap& bitmap) 
		{ 
			InvalidBase& invalid(canvas);
			invalid.reduce();
			invalid.Draw(display,bitmap,window,gc);
			invalid.Show(display,bitmap,window,gc);
			invalid.clear();
		}	

		virtual bool events(Pixmap& bitmap,KeyMap& keys) 
		{ 
			XEvent& e(keys);
			if (!e.type) return true;
			cout<<"Ev>"<<e.type<<"  "<<Expose<<" - ";
			if (e.type==NoExpose) 
			{
				//cout<<"NoExpose"; cout.flush();
			}
			if (e.type==GraphicsExpose) 
			{
				cout<<"GraphicsExpose"; cout.flush();
			}
			if (e.type==Expose) 
			{
				InvalidBase& invalid(canvas);
				cout<<"Expose"; cout.flush();
				invalid.expose();
			}
			return true; 
		}

		virtual bool events(Pixmap& bitmap)
		{

			XWindowAttributes attr;
			XButtonEvent start;
			if (!XPending(display)) return true;

			bool ret(true);
			XEvent e;
			XNextEvent(display,&e);
			DebugEvent( e );
			keys.clear();
			if (Focused) 
			{
					if (e.type==KeyPress) keys=e; 
					if (!canvas(e,keys)) return false;
			}
			{

				if (e.xany.type == LeaveNotify)
				{
					XUngrabPointer(display, CurrentTime);
					Focused=false;
				}

				if (e.xany.type == EnterNotify)
				{
					int grabbed(XGrabPointer(display, window , False, 
						ButtonPressMask|ButtonReleaseMask|EnterWindowMask|LeaveWindowMask|PointerMotionMask|PointerMotionHintMask|Button1MotionMask|Button2MotionMask|Button3MotionMask|Button4MotionMask|Button5MotionMask|ButtonMotionMask
						, GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime));
					Focused=true;
				}

				if (e.xany.type == FocusOut)
				{
					XUngrabPointer(display, CurrentTime);
					Focused=false;
				}

				if (e.xany.type == FocusIn)
				{
					int grabbed(XGrabPointer(display, window , False, 
						ButtonPressMask|ButtonReleaseMask|EnterWindowMask|LeaveWindowMask|PointerMotionMask|PointerMotionHintMask|Button1MotionMask|Button2MotionMask|Button3MotionMask|Button4MotionMask|Button5MotionMask|ButtonMotionMask
						, GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime));
					Focused=true;
				}

				if (!Focused) return true;

			}
			return ret;
		}

		protected:
		const long long when(const long long offset=0)
		{
			struct timespec tp;
			clock_gettime(CLOCK_MONOTONIC,&tp);
			long long now(tp.tv_sec);
			if (!offset) return now;
			now-=offset;
			now*=1e9;
			now+=tp.tv_nsec;
			return now;
		}
		const int screen;
		Display* display;
		Window& window;
		GC& gc;
		XImage* image;
		const int ScreenWidth,ScreenHeight;

		private:
		class ScreenBuffers : pair<Buffer*,Buffer*>
		{
			bool toggle;
			public: 
			ScreenBuffers(Canvas& _canvas) : canvas(_canvas) {}
			virtual ~ScreenBuffers(){delete first; delete second;}
			void operator()(const int screen,Display* display,Window& window,GC& gc,XImage* image,const int ScreenWidth,const int ScreenHeight)
			{
				first=new Buffer(screen,display,window,gc,canvas,image,ScreenWidth,ScreenHeight); 
				second=new Buffer(screen,display,window,gc,canvas,image,ScreenWidth,ScreenHeight); 
			}
			operator Buffer& ()
			{
				toggle!=toggle; 
				return ((toggle)?*first:*second);
			}
			private: Canvas& canvas;
		} ; 
		protected: ScreenBuffers buffers;
	};
	inline ostream& operator<<(ostream& o,Application& a){return a.operator<<(o);} 
	struct Program : Application
	{
		Program(const int screen,Display* display,Window& window,GC& gc,XImage* image,Canvas& canvas,KeyMap& keys,const int ScreenWidth,const int ScreenHeight)
			: Application(screen,display,window,gc,image,canvas,keys,ScreenWidth,ScreenHeight) {}
		virtual void update() { canvas.update(); }
		operator Application& () { return *this; }
		virtual bool events(Pixmap& bitmap,KeyMap& keys) 
		{ 
			XEvent& e(keys);
			//if (e.type&KeyPress) 

cout<<"k,"<<e.xkey.keycode<<","<<e.xkey.state<<". ";

			cout.flush();
			bool r(canvas(keys));
			Application::events(bitmap,keys);
			return r;
		}
	};

	inline void DebugEvent( XEvent& e )
	{
		switch ( e.type )
		{ 
			case KeyPress: cout << "KeyPress" << endl; break;
			case KeyRelease: cout << "KeyRelease" << endl; break;
			case ButtonPress: cout << "ButtonPress" << endl; break;
			case ButtonRelease: cout << "ButtonRelease" << endl; break;
			case MotionNotify: cout << "MotionNotify" << endl; break;
			case EnterNotify: cout << "EnterNotify" << endl; break;
			case LeaveNotify: cout << "LeaveNotify" << endl; break;
			case FocusIn: cout << "FocusIn" << endl; break;
			case FocusOut: cout << "FocusOut" << endl; break;
			case KeymapNotify: cout << "KeymapNotify" << endl; break;
			case Expose: cout << "Expose" << endl; break;
			case GraphicsExpose: cout << "GraphicsExpose" << endl; break;
			case NoExpose: break; //cout << "NoExpose" << endl; break;
			case VisibilityNotify: cout << "VisibilityNotify" << endl; break;
			case CreateNotify: cout << "CreateNotify" << endl; break;
			case DestroyNotify: cout << "DestroyNotify" << endl; break;
			case UnmapNotify: cout << "UnmapNotify" << endl; break;
			case MapNotify: cout << "MapNotify" << endl; break;
			case MapRequest: cout << "MapRequest" << endl; break;
			case ReparentNotify: cout << "ReparentNotify" << endl; break;
			case ConfigureNotify: cout << "ConfigureNotify" << endl; break;
			case ConfigureRequest: cout << "ConfigureRequest" << endl; break;
			case GravityNotify: cout << "GravityNotify" << endl; break;
			case ResizeRequest: cout << "ResizeRequest" << endl; break;
			case CirculateNotify: cout << "CirculateNotify" << endl; break;
			case CirculateRequest: cout << "CirculateRequest" << endl; break;
			case PropertyNotify: cout << "PropertyNotify" << endl; break;
			case SelectionClear: cout << "SelectionClear" << endl; break;
			case SelectionRequest: cout << "SelectionRequest" << endl; break;
			case SelectionNotify: cout << "SelectionNotify" << endl; break;
			case ColormapNotify: cout << "ColormapNotify" << endl; break;
			case ClientMessage: cout << "ClientMessage" << endl; break;
			case MappingNotify: cout << "MappingNotify" << endl; break;
			case GenericEvent: cout << "GenericEvent" << endl; break;
			case LASTEvent: cout << "LASTEvent" << endl; break;
		}
		cout.flush();

	}	
} //X11Methods
#endif //__X11_METHODS_H__

