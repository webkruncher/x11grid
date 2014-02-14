
#include "x11grid.h"

namespace X11Grid
{
		void Grid::operator()(Pixmap& bitmap) 
		{ 
			for (vector<CardCover>::iterator coverit=coverup.begin();coverit!=coverup.end();coverit++)
			{
				CardCover& p(*coverit);
				p.card->cover(display,gc,bitmap,p.color,invalid,p.x,p.y);
			}
			coverup.clear();
			invalid.insert(Rect(10,100,ScreenWidth-160,140));	
			Row::operator()(bitmap); 
		}

		int Grid::operator()(Card& card,Pixmap&  bitmap,const int x,const int y)
			{ card(bitmap,x,y,display,gc,invalid); }

		void Grid::operator()(const unsigned long color,Pixmap&  bitmap,const int x,const int y)
		{
			int cw(10); int ch(10);
			XPoint points[4]; 
			const int X(x);
			const int Y(y);
			points[0].x=X;
			points[0].y=Y;
			points[1].x=X+cw;
			points[1].y=Y;
			points[2].x=X+cw;
			points[2].y=Y+ch;
			points[3].x=X;
			points[3].y=Y+ch;
			XSetForeground(display,gc,color);
			XFillPolygon(display,bitmap,  gc,points, 4, Complex, CoordModeOrigin);
			invalid.insert(Rect(X,Y,X+cw,Y+ch));	
		}

		int Grid::operator()(const int x,const int y) 
		{
			int n(0);
			vector<Point> p;
			p.push_back(Point(x-1,y));
			p.push_back(Point(x-1,y+1));
			p.push_back(Point(x-1,y-1));
			p.push_back(Point(x+1,y));
			p.push_back(Point(x+1,y+1));
			p.push_back(Point(x+1,y-1));
			p.push_back(Point(x,y+1));
			p.push_back(Point(x,y-1));
			return n;
		}

} // X11Grid

