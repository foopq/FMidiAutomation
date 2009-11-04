
#ifndef __SEQUENCER_H
#define __SEQUENCER_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

class Sequencer;

class SequencerEntry
{
    Sequencer *sequencer;
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Viewport *mainWindow;
    Gtk::Viewport *smallWindow;
    bool isFullBox;
    int curIndex;

    bool inHandler;
    void handleSwitchPressed();
    bool handleKeyEntryOnLargeTitleEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSmallTitleEntryBox(GdkEventKey *event);
    bool mouseButtonPressed(GdkEventButton *event);

public:
    SequencerEntry(const Glib::ustring &entryGlade, Sequencer *sequencer, unsigned int entryNum);

    void setIndex(unsigned int index);
    unsigned int getIndex();
    void deselect();

    Gtk::Widget *getHookWidget();
    bool IsFullBox() const;
};//SequencerEntry

class Sequencer
{
    std::map<boost::shared_ptr<SequencerEntry>, int > entries; //int is abs height
    Glib::ustring entryGlade;
    Gtk::VBox *parentWidget;
    Gtk::Label tmpLabel;
    Gtk::VBox tmpLabelBox;
    SequencerEntry *selectedEntry;

    void adjustFillerHeight();
    void adjustEntryIndices();

public:
    Sequencer(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget);

    boost::shared_ptr<SequencerEntry> addEntry(int index);
    void addEntry(boost::shared_ptr<SequencerEntry> entry, int index);
    void deleteEntry(boost::shared_ptr<SequencerEntry> entry);

    unsigned int getEntryIndex(boost::shared_ptr<SequencerEntry> entry);
    boost::shared_ptr<SequencerEntry> getSelectedEntry();
    unsigned int getNumEntries() const;
 
    void doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next);
    void notifySelected(SequencerEntry *selectedEntry);
    void notifyOnScroll(double pos);
};//Sequencer

#endif

