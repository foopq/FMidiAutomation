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
#include <set>
#include <deque>
#include <jack/jack.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include "ProcessRecordedMidi.h"
#include "VectorStreambuf.h"
#include "FMidiAutomationMainWindow.h" //FIXME: I don't like including this here.. it was pulled in because we needed declaration for EntryBlockSelectionState

class Sequencer;
struct GraphState;
class SequencerEntry;
class SequencerEntryBlock;
class FMidiAutomationMainWindow;
class Animation;
struct Keyframe;
struct ProcessRecordedMidiCommand;

namespace EntryBlockMergePolicy
{
enum EntryBlockMergePolicy
{
    Replace,
    Merge,  
    Join    //include first keyframes up to start of second keyframes
};//EntryBlockMergePolicy
}//namespace EntryBlockMergePolicy

struct SequencerEntryBlockSelectionInfo
{
    SequencerEntry *entry;
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    Gdk::Rectangle drawnArea;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntryBlockSelectionInfo

class SequencerEntryBlock : public boost::enable_shared_from_this<SequencerEntryBlock>
{
    boost::weak_ptr<SequencerEntry> owningEntry;
    Glib::ustring title;
    int startTick;
    boost::shared_ptr<SequencerEntryBlock> instanceOf;
    //int duration; //in ticks, or unused if instanceOf isn't NULL
    boost::shared_ptr<Animation> curve;
    boost::shared_ptr<Animation> secondaryCurve;

    //UI properties
    double valuesPerPixel;
    double offsetY;

    SequencerEntryBlock() {} //For serialization

protected:
    void setInstanceOf(boost::shared_ptr<SequencerEntryBlock> instanceOf);

public:    
    SequencerEntryBlock(boost::shared_ptr<SequencerEntry> owningEntry, int startTick, boost::shared_ptr<SequencerEntryBlock> instanceOf);

    void cloneCurves(boost::shared_ptr<SequencerEntryBlock> entryBlock);

    void moveBlock(int startTick);
//    void setDuration(int duration);
    void setTitle(const Glib::ustring &title);

    double getValuesPerPixel();
    double getOffsetY();
    void setValuesPerPixel(double valuesPerPixel);
    void setOffsetY(double offsetY);

    int getStartTick() const;
    int getDuration() const;
    Glib::ustring getTitle() const;
    boost::shared_ptr<SequencerEntryBlock> getInstanceOf() const;

    boost::shared_ptr<SequencerEntry> getOwningEntry() const;

    boost::shared_ptr<Animation> getCurve();
    boost::shared_ptr<Animation> getSecondaryCurve();

    boost::shared_ptr<Keyframe> getNextKeyframe(boost::shared_ptr<Keyframe> keyframe);

    int *getRawStartTick();

    boost::shared_ptr<SequencerEntryBlock> deepClone();

    void renderCurves(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
    friend class SequencerEntry;
};//SequencerEntryBlock

struct SequencerEntryImpl
{
    enum ControlType
    {
        CC,
        RPN,
    };//ControlType

    SequencerEntryImpl();
    ~SequencerEntryImpl();

    boost::shared_ptr<SequencerEntryImpl> clone();
    bool operator==(SequencerEntryImpl &other);

    SequencerEntryImpl::ControlType controllerType;
    unsigned char msb;
    unsigned char lsb;
    unsigned char channel;

    bool recordMode;
    bool soloMode;
    bool muteMode;

    //UI specific
    Glib::ustring title;    
    int minValue;
    int maxValue;
    bool sevenBit;
    bool useBothMSBandLSB; //implied true if sevenBit is true

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntryImpl

class SequencerEntry : public boost::enable_shared_from_this<SequencerEntry>
{
    boost::shared_ptr<SequencerEntryImpl> impl;

    Sequencer *sequencer;
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Viewport *mainWindow;
    Gtk::Viewport *smallWindow;
    Gtk::Frame *largeFrame;
//    Gtk::Frame *smallFrame;
    Gtk::Viewport *smallFrame;
    Gtk::CheckButton *activeCheckButton;
    bool isFullBox;
    int curIndex;
    std::map<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks;

    std::set<jack_port_t *> inputPorts;
    std::set<jack_port_t *> outputPorts;

    std::vector<MidiToken> recordTokenBuffer;

    bool inHandler;
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

    void mergeEntryBlockLists(boost::shared_ptr<SequencerEntry> entry, std::deque<boost::shared_ptr<SequencerEntryBlock> > &newEntryBlocks, 
                              EntryBlockMergePolicy::EntryBlockMergePolicy mergePolicy);

    boost::shared_ptr<SequencerEntryBlock> mergeEntryBlocks(boost::shared_ptr<SequencerEntryBlock> oldEntryBlock, boost::shared_ptr<SequencerEntryBlock> newEntryBlock,
                                                             EntryBlockMergePolicy::EntryBlockMergePolicy mergePolicy);

    SequencerEntry() {} //For serialization and clone

public:
    SequencerEntry(const Glib::ustring &entryGlade, Sequencer *sequencer, unsigned int entryNum);
    void doInit(const Glib::ustring &entryGlade, Sequencer *sequencer, unsigned int entryNum);
    ~SequencerEntry();

    double sample(int tick);
    unsigned char sampleChar(int tick);

    const boost::shared_ptr<SequencerEntryImpl> getImpl();
    boost::shared_ptr<SequencerEntryImpl> getImplClone();
    void setNewDataImpl(boost::shared_ptr<SequencerEntryImpl> impl);

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

    void addEntryBlock(int, boost::shared_ptr<SequencerEntryBlock> entryBlock);
    void removeEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock);
    boost::shared_ptr<SequencerEntryBlock> getEntryBlock(int tick);
    std::pair<boost::shared_ptr<SequencerEntryBlock>, boost::shared_ptr<SequencerEntryBlock> > splitEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock, int tick);

    std::set<jack_port_t *> getInputPorts() const;
    std::set<jack_port_t *> getOutputPorts() const;
    void setInputPorts(std::set<jack_port_t *> ports);
    void setOutputPorts(std::set<jack_port_t *> ports);

    void clearRecordTokenBuffer();
    void addRecordToken(MidiToken &token);
    void commitRecordedTokens();

    boost::shared_ptr<SequencerEntry> deepClone();

    void drawEntryBoxes(Cairo::RefPtr<Cairo::Context> context, std::vector<int> &verticalPixelTickValues, int relativeStartY, int relativeEndY, 
                            std::vector<SequencerEntryBlockSelectionInfo> &selectionInfo, 
                            EntryBlockSelectionState &entryBlockSelectionState);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntry

class Sequencer
{
    FMidiAutomationMainWindow *mainWindow;
    std::map<boost::shared_ptr<SequencerEntry>, int > entries; //int is abs height
    Glib::ustring entryGlade;
    Gtk::VBox *parentWidget;
    Gtk::Label tmpLabel;
    Gtk::VBox tmpLabelBox;
    SequencerEntry *selectedEntry;
    boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock;
    std::vector<SequencerEntryBlockSelectionInfo> selectionInfos;

    void adjustEntryIndices();

////    Sequencer() {} //For serialization

protected:
    void setEntryMap(std::map<boost::shared_ptr<SequencerEntry>, int > entryMap);

public:
    Sequencer(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);
    void doInit(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);

    boost::shared_ptr<SequencerEntry> addEntry(int index, bool useDefaults);
    void addEntry(boost::shared_ptr<SequencerEntry> entry, int index);
    void deleteEntry(boost::shared_ptr<SequencerEntry> entry);

    boost::shared_ptr<SequencerEntryBlock> getSelectedEntryBlock() const;
    boost::shared_ptr<SequencerEntryBlock> getSelectedEntryBlock(int x, int y, bool setSelection); //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
    void updateSelectedEntryBlocksInRange(EntryBlockSelectionState &entryBlockSelectionState,
                                            gdouble mousePressDownX, gdouble mousePressDownY, gdouble xPos, gdouble yPos,
                                            int areaWidth, int areaHeight);
    void clearSelectedEntryBlock();

    unsigned int getEntryIndex(boost::shared_ptr<SequencerEntry> entry);
    boost::shared_ptr<SequencerEntry> getSelectedEntry();
    std::pair<std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator, std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator> getEntryPair() const;
    unsigned int getNumEntries() const;
 
    void doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next);
    void notifySelected(SequencerEntry *selectedEntry);
    void notifyOnScroll(double pos);

    void editSequencerEntryProperties(boost::shared_ptr<SequencerEntry> entry, bool createUpdatePoint);

    void drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues);

    void doLoad(boost::archive::xml_iarchive &inputArchive);
    void doSave(boost::archive::xml_oarchive &outputArchive);

    //Public because we need to do this when we resize the window
    void adjustFillerHeight();

    //For midi processing
    void cloneEntryMap();
    std::map<boost::shared_ptr<SequencerEntry>, int > getEntryMap();    

//    boost::shared_ptr<VectorStreambuf> serializeEntryMap();
//    void deserializeEntryMap(boost::shared_ptr<VectorStreambuf> streambuf);

    friend struct ProcessRecordedMidiCommand;
};//Sequencer



BOOST_CLASS_VERSION(Sequencer, 1);
BOOST_CLASS_VERSION(SequencerEntryImpl, 1);
BOOST_CLASS_VERSION(SequencerEntry, 1);
BOOST_CLASS_VERSION(SequencerEntryBlock, 1);
BOOST_CLASS_VERSION(SequencerEntryBlockSelectionInfo, 1);


#endif

