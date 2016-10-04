# Project Name
PRJNAME=noise-maker

# Filepaths
SRCDIR=src
INCDIR=inc
LIBDIR=lib
BINDIR=../Executable
INSTALL=/usr/bin

# Toolchain
CC=g++

# Find source files
C_FILES := $(shell find -L $(SRCDIR) -name '*.cpp')
O_FILES := $(C_FILES:.cpp=.o)

# Find header dirs
INC=$(shell find -L $(INCDIR) -name '*.h' -exec dirname {} \; | uniq)
INC+=$(shell find -L $(LIBDIR) -name '*.h' -exec dirname {} \; | uniq)
INCLUDES=$(INC:%=-I%)

# Compiler Flags
PFLAGS=-pthread
LDFLAGS:=
CFLAGS=
EFLAGS=-fno-inline -Wall -Werror -O0
LFLAGS=-lm -lrt -std=c++0x -lorchestration -lpthread -lextend

# Compiling Process
all: $(PRJNAME) clean

$(PRJNAME): $(O_FILES)
	$(CC) $(PFLAGS) $(LDFLAGS) $(INCLUDES) $(CFLAGS) -o $(BINDIR)/$@ $^ $(EFLAGS) $(LFLAGS)

%.o: %.cpp
	$(CC) $(PFLAGS) -c $(INCLUDES) -o $@ $< $(LFLAGS)

clean:
	rm -f $(SRCDIR)/*.o

cleanall:
	rm -f $(SRCDIR)/*.o
	rm -f $(BINDIR)/*
install:
	cp $(BINDIR)/$(PRJNAME) $(INSTALL)/