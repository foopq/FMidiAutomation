/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __SEQUENCERENTRYUI_H
#define __SEQUENCERENTRYUI_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>
#include <jack/jack.h>
#include "SequencerEntryBlockUI.h"
#include "fmaipair.h"

class SequencerUI;
class SequencerEntryBlockUI;
class SequencerEntry;
struct MidiToken;
class EntryBlockSelectionState;

enum class EntryBlockMergePolicy : char;

class SequencerEntryUI : public std::enable_shared_from_this<SequencerEntryUI>
{
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Viewport *mainWindow;
    Gtk::Viewport *smallWindow;
    Gtk::Frame *largeFrame;
    Gtk::Viewport *smallFrame;
    Gtk::CheckButton *activeCheckButton;
    bool isFullBox;
    int curIndex;
    std::map<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks;
    std::shared_ptr<SequencerEntry> baseEntry;
    std::weak_ptr<SequencerUI> owningSequencer;

    unsigned int relativeStartY; //for UI
    unsigned int relativeEndY; //for UI

    void handleSwitchPressed();
    bool handleKeyEntryOnLargeTitleEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSmallTitleEntryBox(GdkEventKey *event);
    bool mouseButtonPressed(GdkEventButton *event);
    bool handleEntryFocus(GdkEventFocus*);

    void handleRecPressed();
    void handleRecSmPressed();
    void handleSoloPressed();
    void handleSoloSmPressed();
    void handleMutePressed();
    void handleMuteSmPressed();

    void mergeEntryBlockLists(std::shared_ptr<SequencerEntryUI> entry, std::deque<std::shared_ptr<SequencerEntryBlockUI> > &newEntryBlocks, 
                              EntryBlockMergePolicy mergePolicy);

    std::shared_ptr<SequencerEntryBlockUI> mergeEntryBlocks(std::shared_ptr<SequencerEntryBlockUI> oldEntryBlock, std::shared_ptr<SequencerEntryBlockUI> newEntryBlock,
                                                             EntryBlockMergePolicy mergePolicy);

protected:
    void setBaseEntry(std::shared_ptr<SequencerEntry> baseEntry);

public:
    SequencerEntryUI(const Glib::ustring &entryGlade, unsigned int entryNum, std::shared_ptr<SequencerEntry> baseEntry, std::shared_ptr<SequencerUI> owningSequencer);
    ~SequencerEntryUI();

    std::shared_ptr<SequencerEntry> getBaseEntry();
    fmaipair<decltype(entryBlocks.begin()), decltype(entryBlocks.end())> getEntryBlocksPair();

    void setThemeColours();

    void setIndex(unsigned int index);
    unsigned int getIndex();
    void deselect();
    void select();

    Gtk::Widget *getHookWidget();
    bool IsFullBox() const;
    Glib::ustring getTitle() const;
    void setTitle(Glib::ustring);

    void setLabelColour(Gdk::Color colour);

    void setUIBounds(unsigned int relativeStartY, unsigned int relativeEndY);
    std::pair<unsigned int, unsigned int> getUIBounds();

    void addEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock);
    void removeEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock);
//    std::pair<std::shared_ptr<SequencerEntryBlockUI>, std::shared_ptr<SequencerEntryBlockUI> > splitEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock, int tick);

    void setFocus();

    std::shared_ptr<SequencerEntryUI> deepClone(const Glib::ustring &entryGlade);

    void drawEntryBoxes(Cairo::RefPtr<Cairo::Context> context, std::vector<int> &verticalPixelTickValues, int relativeStartY, int relativeEndY, 
                            std::vector<SequencerEntryBlockSelectionInfo> &selectionInfo, 
                            EntryBlockSelectionState &entryBlockSelectionState);

    void doSave(boost::archive::xml_oarchive &outputArchive);
    void doLoad(boost::archive::xml_iarchive &inputArchive);

    friend struct ProcessRecordedMidiCommand;
};//SequencerEntry



#endif


