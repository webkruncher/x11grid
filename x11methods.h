#ifndef __X11_METHODS_H__
#define __X11_METHODS_H__

namespace X11Methods
{
	using namespace std;

	struct ApplicationBase {};

	struct Point : pair<int,int> 
	{
		Point() {}
		Point(const int a,const int b) : pair<int,int>(a,b) {}
		Point(const Point& a) : pair<int,int>(a.first,a.second) {}
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
		virtual ~Rect() {if (xpoints) delete xpoints;}
		friend ostream& operator<<(ostream&,const Rect&);
		virtual ostream& operator<<(ostream& o) const { o<<first<<"/"<<second; return o;}
		void clear(){first.clear();second.clear();}
		bool operator<(Rect& r){return true;}
		operator XPoint& ()
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
		private:
		XPoint* xpoints;
	};
	inline ostream& operator<<(ostream& o,const Rect& b){return b.operator<<(o);}

	// wip
	//inline bool operator<(const Rect&_x,const Rect& y)
	//	{ return (__x.first < __y.first || (!(__y.first < __x.first) && __x.second < __y.second)); }


	struct InvalidArea : set<Rect>
	{
		void reduce()
		{
			// wip
			//cout<<"Reduce"<<endl;
			for (iterator it=begin();it!=end();it++)
			{
				const Rect& r(*it);
				//cout<<r<<endl;
			}
			//cout<<"----------------"<<endl;
		}
		void Fill(Display* display,Pixmap& bitmap,Window& window,GC& gc)
		{
			static unsigned long c(10); c++;
			for (iterator it=begin();it!=end();it++)
			{
				const Rect& r(*it);
				int x(r.first.first);
				int y(r.first.second);
				int w(r.second.first-x);
				int h(r.second.second-y);
				XSetForeground(display,gc,c);
				XFillRectangle(display,bitmap,gc,x,y,w,h);
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
		virtual bool operator()(KeyMap&) {return true;}
		virtual void operator()(Pixmap& bitmap) = 0;
		virtual void update() = 0; 
		InvalidArea invalid;
		protected:
		Display* display;
		GC& gc;
		const int ScreenWidth,ScreenHeight;
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
			} else {
				XSetForeground(display,gc,0X3333);
				//XFillRectangle(display,bitmap,gc,0,0,ScreenWidth,ScreenHeight);

				for (InvalidArea::iterator it=canvas.invalid.begin();it!=canvas.invalid.end();it++)
				{
					const Rect& r(*it);
					int x(r.first.first);
					int y(r.first.second);
					int w(r.second.first-x);
					int h(r.second.second-y);
					XFillRectangle(display,bitmap,gc,x,y,w,h);
				}

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
		public:
		operator Window& () { return window;}
		operator Display* () { return display;}
		operator Canvas& () { return canvas; }
		Application(const int _screen,Display* _display,Window& _window,GC& _gc,XImage* _image,Canvas& _canvas,KeyMap& _keys,const int _ScreenWidth,const int _ScreenHeight)
			: screen(_screen),display(_display),window(_window),gc(_gc),image(_image),canvas(_canvas),keys(_keys),ScreenWidth(_ScreenWidth),ScreenHeight(_ScreenHeight),buffers(canvas) {}
		virtual void operator()(int argc,char** argv)
		{
			canvas(*this,argc,argv);
			if (!display) return;
			const long long started(when());
			buffers(screen,display,window,gc,image,ScreenWidth,ScreenHeight);
			long long next(0),unext(0);
			while (true) 
			{
				Buffer& buffer(buffers);
				Pixmap& bitmap(buffer);
				if (when(started)>next) 
				{
					canvas(bitmap);
					canvas.invalid.reduce();
					draw(bitmap);
					next=(when(started)+1e1);
				}
				if (when(started)>unext) 
				{
					update();
					unext=(when(started)+1e2);
				}
				if (!events(bitmap)) return ;
				usleep(1e3);
				cout.flush();
			}
		}
		protected:
		virtual void update() { }
		Canvas& canvas;
		KeyMap& keys;
		virtual void draw(Pixmap& bitmap) 
		{ 
			for (InvalidArea::iterator it=canvas.invalid.begin();it!=canvas.invalid.end();it++)
			{
				const Rect& r(*it);
				int x(r.first.first);
				int y(r.first.second);
				int w(r.second.first-x);
				int h(r.second.second-y);
				XCopyArea(display,bitmap,window,gc,x,y,w,h,x,y); 
			}
			//canvas.invalid.Fill(display,bitmap,window,gc);
			//XCopyArea(display,bitmap,window,gc,0,0,ScreenWidth,ScreenHeight,0,0); 
			canvas.invalid.clear();
		}	
		virtual bool events(Pixmap& bitmap,KeyMap& keys) 
		{ 
			XEvent& e(keys);
			if (!e.type) return true;
			cout<<"Ev>"<<e.type<<"  "<<Expose<<" - ";
			if (e.type==NoExpose) 
			{
				cout<<"NoExpose"; cout.flush();
			}
			if (e.type==GraphicsExpose) 
			{
				cout<<"GraphicsExpose"; cout.flush();
			}
			if (e.type==Expose) 
			{
				cout<<"Expose"; cout.flush();
				canvas(bitmap);
				canvas.invalid.clear();
				canvas.invalid.insert(Rect(0,0,ScreenWidth,ScreenHeight));
				draw(bitmap); 
			}
			return true; 
		}
		virtual bool events(Pixmap& bitmap)
		{
			XEvent e;
			if (XPending(display)) 
			{ 
				XNextEvent(display,&e); 
				if (e.type==KeyPress) keys=e; else keys.clear();
				bool ret(events(bitmap,keys)); 
				XSync(display,True);
				keys.clear();
				return ret;
			} 
			return true;
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
			if (e.type==KeyPress) cout<<"k,"<<e.xkey.keycode<<","<<e.xkey.state<<". ";
			if (e.type==ButtonPress) cout<<"b,"<<e.xbutton.x<<","<<e.xbutton.y<<". ";
			cout.flush();
			bool r(canvas(keys));
			Application::events(bitmap,keys);
			return r;
		}
	};

} //X11Methods
#endif //__X11_METHODS_H__

