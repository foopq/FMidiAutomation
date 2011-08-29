# Makefile

TARGET=FMidiAutomation

# Compiler name
CXX=/bin/g++
CXX2=/usr/bin/g++
MAKEDEPEND=gcc -M $(CXXFLAGS) -o $*.d $<

# Libraries to be included
LDLIBS=`pkg-config jack alsa gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 libgnomecanvasmm-2.6 --libs` -lboost_filesystem-mt -lboost_serialization-mt -lboost_thread-mt -lflowcanvas -ltcmalloc

# Flagas
CXXFLAGS=-Wall -g `pkg-config jack alsa gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 libgnomecanvasmm-2.6 --cflags` -I. -std=c++0x

# Variables
SRCS = main.cc FMidiAutomationGraph.cc FMidiAutomationMainWindow.cc FMidiAutomationData.cc Tempo.cc Command.cc jack.cc Sequencer.cc EntryBlockProperties.cc \
       PasteManager.cc EntryProperties.cc FMidiAutomationCurveEditor.cc Animation.cc jackPortDialog.cc ProcessRecordedMidi.cc \
	   SerializationHelper.cc Config.cc \
	   UI/MouseHandlers/mouseHandlerEntry.cc \
	   UI/MouseHandlers/Sequencer/mouseHandler_Sequencer_FrameRegion.cc \
	   UI/MouseHandlers/Sequencer/mouseHandler_Sequencer_MainCanvas.cc \
	   UI/MouseHandlers/Sequencer/mouseHandler_Sequencer_TickMarkerRegion.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_FrameRegion.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_LeftValueRegion.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_MainCanvas.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_TickMarkerRegion.cc

OBJS = $(SRCS:.cc=.o)
DEPS = $(SRCS:.cc=.depends)

#Application name
FMidiAutomation: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDLIBS) $(OBJS) -o $(TARGET)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.depends: %.cc
	$(CXX2) -M $(CXXFLAGS) $< > $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

PHONY: clean

