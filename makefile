
LIB=-L/usr/local/lib -L/usr/X11R6/lib -lX11 
INC=-I. -I /usr/X11R6/include -I /usr/local/include 

x11grid: x11grid.a main.o
	g++ -I. x11grid.o main.o -o x11grid $(LIB) $(INC) -w

x11grid.a: x11grid.o   $(INCS)
	ar -r -s x11grid.a x11grid.o

x11grid.o: x11grid.cpp  $(INCS)  x11grid.h x11methods.h
	g++ -D BSD -c  -pthread -lstdc++ x11grid.cpp ${INC} 

main.o: main.cpp  $(INCS)  x11grid.h x11methods.h
	g++ -D BSD -c  -pthread -lstdc++ main.cpp ${INC} 

clean:
	rm x11grid
	rm *.o
	rm *.a

