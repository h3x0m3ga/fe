# change application name here (executable output name)
TARGET=fe
# compiler
CC=gcc
# debug
DEBUG=-g
# optimisation
OPT=-O2
# warnings
WARN=-Wall

PTHREAD=-pthread

CCFLAGS=$(DEBUG) $(OPT) $(WARN) $(PTHREAD) -pipe

# Added webkit2gtk-4.0 for building with webkit2gtk
GTKLIB=`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0`

# linker
LD=gcc
LDFLAGS=$(PTHREAD) $(GTKLIB) -export-dynamic

OBJS=main.o

all: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(LDFLAGS)
main.o: main.c
	$(CC) -c $(CCFLAGS) main.c $(GTKLIB) -o main.o
clean:
	rm -f *.o $(TARGET)
