CC = g++
CL_INC = $(AMDAPPSDKROOT)/include
CPPFLAGS = -I. -std=c++0x -g -O3 -I$(CL_INC)  -D_GNU_SOURCE 
LIBS = -L$(AMDAPPSDKROOT)/lib/x86_64 -lOpenCL -lrt -lpthread
 
DEPS = InputFlags.h \
	   OpenCLHelper.h \

OBJ = Main.o \
	  InputFlags.o \
	  OpenCLHelper.o \
	  PCIeBandwidthTest.o

%.o: %.cpp $(DEPS) 
	$(CC) -c -o $@ $< $(CPPFLAGS)

PCIeBandwidth: $(OBJ) 
	$(CC) $(CPPFLAGS) -o $@ $^ $(LIBS)

clean:
	rm *.o PCIeBandwidth
