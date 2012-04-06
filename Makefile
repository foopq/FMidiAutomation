# Makefile

TARGET=FMidiAutomation

# Compiler name
CXX=/usr/bin/g++
CXX2=/usr/bin/g++
MAKEDEPEND=gcc -M $(CXXFLAGS) -o $*.d $<

# Libraries to be included
LDLIBS=`pkg-config jack alsa flowcanvas gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 libgnomecanvasmm-2.6 --libs` -lboost_filesystem-mt -lboost_serialization-mt -lboost_thread-mt -ltcmalloc

# Flagas
CXXFLAGS=-Wall -g `pkg-config jack alsa flowcanvas gtkmm-2.4 pangomm-1.4 libglademm-2.4 gdkmm-2.4 libxml++-2.6 libgnomecanvasmm-2.6 --cflags` -I. -std=c++0x

# Variables
SRCS = main.cc WindowManager.cc \
	   Data/FMidiAutomationData.cc Data/Sequencer.cc Data/SequencerEntry.cc Data/SequencerEntryBlock.cc \
	   UI/SequencerUI.cc UI/SequencerEntryUI.cc UI/SequencerEntryBlockUI.cc \
	   UI/MouseHandlers/mouseHandlerEntry.cc \
	   UI/MouseHandlers/Sequencer/mouseHandler_Sequencer_FrameRegion.cc \
	   UI/MouseHandlers/Sequencer/mouseHandler_Sequencer_MainCanvas.cc \
	   UI/MouseHandlers/Sequencer/mouseHandler_Sequencer_TickMarkerRegion.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_FrameRegion.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_LeftValueRegion.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_MainCanvas.cc \
	   UI/MouseHandlers/CurveEditor/mouseHandler_CurveEditor_TickMarkerRegion.cc \
       FMidiAutomationGraph.cc FMidiAutomationMainWindow.cc Tempo.cc jack.cc EntryBlockProperties.cc \
       PasteManager.cc EntryProperties.cc FMidiAutomationCurveEditor.cc Animation.cc jackPortDialog.cc ProcessRecordedMidi.cc \
	   SerializationHelper.cc Config.cc Command_CurveEditor.cc Command_Sequencer.cc Command_Other.cc


OBJS = $(SRCS:.cc=.o)
DEPS = $(SRCS:.cc=.depends)

#Application name
FMidiAutomation: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.depends: %.cc
	$(CXX2) -M $(CXXFLAGS) $< > $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

PHONY: clean

