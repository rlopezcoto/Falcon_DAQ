FLASHCAM_INC= -I/root/fadc/fc250b-2.0-server/Lib 
FLASHCAM_LIB=/root/fadc/fc250b-2.0-server/Lib/*.a /root/fadc/fc250b-2.0-server/Lib/pzpsa.a 
ROOT_INC = $(shell root-config --cflags)
ROOT_LIB = $(shell root-config --libs)

XCDFROOT=/root/XCDF-3.0.0/XCDF/install/
XCDFLIB= -L$(XCDFROOT)/lib -lxcdf
XCDFINC= -I$(XCDFROOT)/include

OBJECTS= ChargeTimeExtraction.o

all: $(OBJECTS) f2f spe rf

ChargeTimeExtraction.o: ../include/ChargeTimeExtraction.h ../src/ChargeTimeExtraction.c
	g++ -c ../src/ChargeTimeExtraction.c -o ChargeTimeExtraction.o $(FLASHCAM_INC) $(ROOT_INC) -I../include

f2f: ../src_exe/Flash2Falcon.c
	g++ ../src_exe/Flash2Falcon.c $(OBJECTS) -o Flash2Falcon $(FLASHCAM_INC) $(FLASHCAM_LIB) $(ROOT_INC) $(ROOT_LIB) -I$(XCDFROOT)/include -L$(XCDFROOT)/lib -lxcdf  -I../include

spe: ../src_exe/single_pe.c
	g++ ../src_exe/single_pe.c $(OBJECTS) -o single_pe $(FLASHCAM_INC) $(FLASHCAM_LIB) $(ROOT_INC) $(ROOT_LIB) -I$(XCDFROOT)/include  -I../include

rf: ../src_exe/readFalconXCDF.cc	
	g++ ../src_exe/readFalconXCDF.cc -o readFalconXCDF  -lz -lm $(XCDFLIB) $(XCDFINC)

install:
	cp single_pe ../install/bin
	cp Flash2Falcon ../install/bin
	cp readFalconXCDF ../istall/bin

clean:
	rm *o single_pe Flash2Falcon  
