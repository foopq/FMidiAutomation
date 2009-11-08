
#ifndef __SEQUENCER_H
#define __SEQUENCER_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

class Sequencer;
struct GraphState;
class SequencerEntry;
class SequencerEntryBlock;
class FMidiAutomationMainWindow;

struct SequencerEntryBlockSelectionInfo
{
    SequencerEntry *entry;
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    Gdk::Rectangle drawnArea;
};//SequencerEntryBlockSelectionInfo

class SequencerEntryBlock
{
    Glib::ustring title;
    int startTick;
    boost::shared_ptr<SequencerEntryBlock> instanceOf;
    int duration; //in ticks, or unused if instanceOf isn't NULL
    //boost::shared_ptr<SequencerEntryCurce> curve;

public:    
    SequencerEntryBlock(int startTick, boost::shared_ptr<SequencerEntryBlock> instanceOf);

    void moveBlock(int startTick);
    void setDuration(int duration);
    void setTitle(const Glib::ustring &title);

    int getStartTick() const;
    int getDuration() const;
    Glib::ustring getTitle() const;
    boost::shared_ptr<SequencerEntryBlock> getInstanceOf() const;
};//SequencerEntryBlock

class SequencerEntry
{
    Sequencer *sequencer;
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Viewport *mainWindow;
    Gtk::Viewport *smallWindow;
    Gtk::Frame *largeFrame;
    Gtk::Frame *smallFrame;
    bool isFullBox;
    int curIndex;
    std::map<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks;

    bool inHandler;
    void handleSwitchPressed();
    bool handleKeyEntryOnLargeTitleEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSmallTitleEntryBox(GdkEventKey *event);
    bool mouseButtonPressed(GdkEventButton *event);

public:
    SequencerEntry(const Glib::ustring &entryGlade, Sequencer *sequencer, unsigned int entryNum);
    ~SequencerEntry();
    void setThemeColours();

    void setIndex(unsigned int index);
    unsigned int getIndex();
    void deselect();
    void select();

    Gtk::Widget *getHookWidget();
    bool IsFullBox() const;
    Glib::ustring getTitle() const;

    void addEntryBlock(int, boost::shared_ptr<SequencerEntryBlock> entryBlock);
    void removeEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock);

    void drawEntryBoxes(Cairo::RefPtr<Cairo::Context> context, std::vector<int> &verticalPixelTickValues, int relativeStartY, int relativeEndY, std::vector<SequencerEntryBlockSelectionInfo> &selectionInfo,
                            boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock);

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

    void adjustFillerHeight();
    void adjustEntryIndices();

public:
    Sequencer(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget, FMidiAutomationMainWindow *mainWindow);

    boost::shared_ptr<SequencerEntry> addEntry(int index);
    void addEntry(boost::shared_ptr<SequencerEntry> entry, int index);
    void deleteEntry(boost::shared_ptr<SequencerEntry> entry);

    boost::shared_ptr<SequencerEntryBlock> getSelectedEntryBlock() const;
    boost::shared_ptr<SequencerEntryBlock> getSelectedEntryBlock(int x, int y, bool setSelection); //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
    // ... MULTISELECT???
    void clearSelectedEntryBlock();

    unsigned int getEntryIndex(boost::shared_ptr<SequencerEntry> entry);
    boost::shared_ptr<SequencerEntry> getSelectedEntry();
    unsigned int getNumEntries() const;
 
    void doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next);
    void notifySelected(SequencerEntry *selectedEntry);
    void notifyOnScroll(double pos);

    void drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues);
};//Sequencer

#endif

