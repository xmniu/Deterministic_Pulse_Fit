# example1

#LIBS = `root-config --glibs` -L/usr/X11R6/lib -lm -ldl -lstdc++
LIBS = -L/usr/X11R6/lib -lm -ldl -lstdc++

#OPTCOMP =  +K0
CXXFLAGS = -O3 --exceptions -I$(ROOTSYS)/include
# -pg -ggdb
OBJECTS = example1.o readparameters.o

all: $(OBJECTS)
	gcc $(OPTCOMP) $(CXXFLAGS) $(LIBS) -o example1 -g $(OBJECTS) 

example1.o: example1.cpp example1.hpp readparameters.hpp readparameters.cpp 
	gcc $(OPTCOMP) $(CXXFLAGS) -c example1.cpp

readparameters.o: readparameters.hpp readparameters.cpp 
	gcc $(OPTCOMP) $(CXXFLAGS) -c readparameters.cpp

clean:
	rm -f *.o
	rm -f example1
	rm -rf ti_files
	rm -f *~;
