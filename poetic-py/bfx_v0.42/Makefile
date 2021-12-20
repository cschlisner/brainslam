CC=g++
CFLAGS=-c -Wall --std=c++0x -O3
SOURCES=preprocessor/ppparser.cc preprocessor/ppparse.cc preprocessor/scanner/pplex.cc \
	compiler/ccparser.cc compiler/ccparse.cc compiler/init.cc compiler/scanner/cclex.cc \
	main.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=bfx

all:	$(SOURCES) $(EXECUTABLE)

$(EXECUTABLE):	$(OBJECTS)
	$(CC) $(OBJECTS) -o $@
	
.cc.o:
	$(CC) $(CFLAGS) $< -o $@
