# Makefile

# Compiler name
CC=g++

# Libraries to be included
LDLIBS=`pkg-config jack alsa gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 --libs` -lboost_filesystem -lboost_serialization -lboost_thread

# Flagas
CXXFLAGS=-Wall -g `pkg-config jack alsa gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 --cflags` -I.

# Variables
OBJS = main.o FMidiAutomationGraph.o FMidiAutomationMainWindow.o FMidiAutomationData.o

#Application name
FMidiAutomation: $(OBJS)
	$(CC) $(OBJS) $(LDLIBS)  -o FMidiAutomation

%.o : %.c
	$(COMPILE.c) -MD -o $@ $<
	@cp $*.d $*.P; \
        sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
            -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
        rm -f $*.d


clean:
	rm -f *.o FMidiAutomationMainWindow

PHONY: clean

