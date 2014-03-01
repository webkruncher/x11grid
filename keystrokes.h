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

