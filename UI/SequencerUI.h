/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/



#ifndef __SEQUENCERUI_H
#define __SEQUENCERUI_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>
#include "Data/SequencerEntryBlock.h"
#include "SequencerEntryBlockUI.h"
#include "fmaipair.h"

class Sequencer;
struct GraphState;
class SequencerEntryUI;
class SequencerEntryBlockUI;
class FMidiAutomationMainWindow;
class Animation;
struct Keyframe;
struct ProcessRecordedMidiCommand;
class EntryBlockSelectionState;


class SequencerUI : public std::enable_shared_from_this<SequencerUI>
{
    FMidiAutomationMainWindow *mainWindow; //Note: Needs to be a raw pointer, sadly
    std::map<std::shared_ptr<SequencerEntryUI>, int> entries; //int is abs height
    Glib::ustring entryGlade;
    Gtk::VBox *parentWidget;
    Gtk::Label tmpLabel;
    Gtk::VBox tmpLabelBox;
    std::shared_ptr<SequencerEntryUI> selectedEntry;
    std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock;
    std::vector<SequencerEntryBlockSelectionInfo> selectionInfos;

    void adjustEntryIndices();

protected:
    void setEntryMap(std::map<std::shared_ptr<SequencerEntryUI>, int > entryMap);

public:
    SequencerUI(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);
    void doInit(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);

    void deepCloneEntries(std::shared_ptr<SequencerUI> other);

    std::shared_ptr<SequencerEntryUI> addEntry(int index, bool useDefaults, std::shared_ptr<SequencerEntry> origEntry);
    void addEntry(int index, std::shared_ptr<SequencerEntryUI> entry);
    void deleteEntry(std::shared_ptr<SequencerEntryUI> entry);

    std::shared_ptr<SequencerEntryBlockUI> getSelectedEntryBlock() const;
    std::shared_ptr<SequencerEntryBlockUI> getSelectedEntryBlock(int x, int y, bool setSelection); //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
    void updateSelectedEntryBlocksInRange(EntryBlockSelectionState &entryBlockSelectionState,
                                            gdouble mousePressDownX, gdouble mousePressDownY, gdouble xPos, gdouble yPos,
                                            int areaWidth, int areaHeight);
    void clearSelectedEntryBlock();

    unsigned int getEntryIndex(std::shared_ptr<SequencerEntryUI> entry);
    std::shared_ptr<SequencerEntryUI> getSelectedEntry();
    fmaipair<decltype(entries.begin()), decltype(entries.end())> getEntryPair();
    unsigned int getNumEntries() const;
 
    void doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next);
    void notifySelected(std::shared_ptr<SequencerEntryUI> selectedEntry);
    void notifyOnScroll(double pos);

    void updateEntryFocus(unsigned int y);
    void editSequencerEntryProperties(std::shared_ptr<SequencerEntryUI> entry, bool createUpdatePoint);

    void drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, 
                        unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues);

    //Public because we need to do this when we resize the window
    void adjustFillerHeight();

//    void cloneEntryMap();
//    std::map<std::shared_ptr<SequencerEntry>, int > getEntryMap();    

    void doSave(boost::archive::xml_oarchive &outputArchive);
    void doLoad(boost::archive::xml_iarchive &inputArchive);
};//SequencerUI





#endif

