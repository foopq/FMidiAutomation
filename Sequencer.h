
#ifndef __SEQUENCER_H
#define __SEQUENCER_H

#include <gtkmm.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

class SequencerEntry
{
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Viewport *mainWindow;
    Gtk::Viewport *smallWindow;
    bool isFullBox;
    boost::function<void (Gtk::Viewport *current, Gtk::Viewport *next)> doSwapEntryBoxCallback;

public:
    SequencerEntry(const Glib::ustring &entryGlade, boost::function<void (Gtk::Viewport *current, Gtk::Viewport *next)> doSwapEntryBoxCallback);

    Gtk::Widget *getHookWidget();
};//SequencerEntry

class Sequencer
{
    std::vector<boost::shared_ptr<SequencerEntry> > entries;
    Glib::ustring entryGlade;
    Gtk::VBox *parentWidget;
    Gtk::Label tmpLabel;
    Gtk::VBox tmpLabelBox;

public:
    Sequencer(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget);
    void doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next);

    void addEntry();
    void deleteEntry(unsigned int index);
};//Sequencer

#endif

