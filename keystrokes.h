#ifndef X_KRUNCHER_KEYSTROKES_H
#define X_KRUNCHER_KEYSTROKES_H

namespace X11Methods
{
	using namespace std;
	class KeyMap
	{
		public:
		enum Prepesition {none,home,up,down,left,right,in,out};
		enum KState {normal,shift=1,ctrl=4,alt=8};
		KeyMap() : quit(false), prepi(none),kstate(normal) {}
		void clear(){e.type=0;e.xkey.keycode=0;kstate=normal;quit=false;}
		operator KState () { return kstate; }
		operator bool () { return quit; }
		virtual void operator=(XEvent& _e)
		{
			e=_e;
			((int&)kstate)=e.xkey.state;
			switch (e.xkey.keycode) 
			{ 
				case  98: prepi=up; 	break;
				case 104: prepi=down; 	break;
				case 100: prepi=left; 	break;
				case 102: prepi=right; 	break;
				case  99: prepi=in; 	break;
				case 105: prepi=out; 	break;
				case  60: prepi=home;	break;
				case  24: quit=true;	break;
				default: break; //cout<<">"<<e.xkey.keycode<<"<"<<endl; 
			}
		}
		operator Prepesition () { return prepi; }
		operator XEvent& () { return e;}
		protected:
		XEvent e;
		bool quit;
		Prepesition  prepi;
		KState kstate;
	};
} // X11Methods
#endif //X_KRUNCHER_KEYSTROKES_H

