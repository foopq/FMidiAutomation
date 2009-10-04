# Makefile

# Compiler name
CC=g++
MAKEDEPEND=gcc -M $(CXXFLAGS) -o $*.d $<

# Libraries to be included
LDLIBS=`pkg-config jack alsa gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 --libs` -lboost_filesystem -lboost_serialization -lboost_thread

# Flagas
CXXFLAGS=-Wall -g `pkg-config jack alsa gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 --cflags` -I.

# Variables
SRCS = main.cc FMidiAutomationGraph.cc FMidiAutomationMainWindow.cc FMidiAutomationData.cc Tempo.cc
OBJS = $(SRCS:.cc=.o)

#Application name
FMidiAutomation: $(OBJS)
	$(CC) $(OBJS) $(LDLIBS)  -o FMidiAutomation

%.o : %.c
	cp $*.d $*.P; \
        sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
            -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
        rm -f $*.d
	$(COMPILE.c) -MD -o $@ $<

%.P : %.cc
	$(MAKEDEPEND)
	@sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' < $*.d > $@; \
	rm -f $*.d; [ -s $@ ] || rm -f $@

include $(SRCS:.cc=.P)


##-include $(SRCS:.cc=.P)

clean:
	rm -f *.o *.P FMidiAutomationMainWindow

PHONY: clean

