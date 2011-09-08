/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/



#ifndef __SEQUENCER_H
#define __SEQUENCER_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>
#include "SequencerEntryBlock.h"
//#include "ProcessRecordedMidi.h"
//#include "VectorStreambuf.h"

class Sequencer;
struct GraphState;
class SequencerEntry;
class SequencerEntryBlock;
class FMidiAutomationMainWindow;
class Animation;
struct Keyframe;
struct ProcessRecordedMidiCommand;
class EntryBlockSelectionState;


class Sequencer : public std::enable_shared_from_this<Sequencer>
{
    FMidiAutomationMainWindow *mainWindow;
    std::map<std::shared_ptr<SequencerEntry>, int > entries; //int is abs height
    Glib::ustring entryGlade;
    Gtk::VBox *parentWidget;
    Gtk::Label tmpLabel;
    Gtk::VBox tmpLabelBox;
    SequencerEntry *selectedEntry;
    std::shared_ptr<SequencerEntryBlock> selectedEntryBlock;
    std::vector<SequencerEntryBlockSelectionInfo> selectionInfos;

    void adjustEntryIndices();

////    Sequencer() {} //For serialization

protected:
    void setEntryMap(std::map<std::shared_ptr<SequencerEntry>, int > entryMap);

public:
    Sequencer(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);
    void doInit(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);

    std::shared_ptr<SequencerEntry> addEntry(int index, bool useDefaults);
    void addEntry(std::shared_ptr<SequencerEntry> entry, int index);
    void deleteEntry(std::shared_ptr<SequencerEntry> entry);

    std::shared_ptr<SequencerEntryBlock> getSelectedEntryBlock() const;
    std::shared_ptr<SequencerEntryBlock> getSelectedEntryBlock(int x, int y, bool setSelection); //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
    void updateSelectedEntryBlocksInRange(EntryBlockSelectionState &entryBlockSelectionState,
                                            gdouble mousePressDownX, gdouble mousePressDownY, gdouble xPos, gdouble yPos,
                                            int areaWidth, int areaHeight);
    void clearSelectedEntryBlock();

    unsigned int getEntryIndex(std::shared_ptr<SequencerEntry> entry);
    std::shared_ptr<SequencerEntry> getSelectedEntry();
    std::pair<std::map<std::shared_ptr<SequencerEntry>, int >::const_iterator, std::map<std::shared_ptr<SequencerEntry>, int >::const_iterator> getEntryPair() const;
    unsigned int getNumEntries() const;
 
    void doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next);
    void notifySelected(SequencerEntry *selectedEntry);
    void notifyOnScroll(double pos);

    void updateEntryFocus(unsigned int y);
    void editSequencerEntryProperties(std::shared_ptr<SequencerEntry> entry, bool createUpdatePoint);

    void drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, 
                        unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues);

    void doLoad(boost::archive::xml_iarchive &inputArchive);
    void doSave(boost::archive::xml_oarchive &outputArchive);

    //Public because we need to do this when we resize the window
    void adjustFillerHeight();

    //For midi processing
    void cloneEntryMap();
    std::map<std::shared_ptr<SequencerEntry>, int > getEntryMap();    

//    std::shared_ptr<VectorStreambuf> serializeEntryMap();
//    void deserializeEntryMap(std::shared_ptr<VectorStreambuf> streambuf);

    friend struct ProcessRecordedMidiCommand;
};//Sequencer



BOOST_CLASS_VERSION(Sequencer, 1);


#endif

