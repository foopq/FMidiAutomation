
#ifndef __SEQUENCER_H
#define __SEQUENCER_H

#include <gtkmm.h>
#include <vector>
#include <boost/shared_ptr.hpp>

class SequencerEntry
{
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Viewport *mainWindow;

public:
    SequencerEntry(const Glib::ustring &entryGlade);

    Gtk::Widget *getHookWidget();
};//SequencerEntry

class Sequencer
{
    std::vector<boost::shared_ptr<SequencerEntry> > entries;
    Glib::ustring entryGlade;
    Gtk::VBox *parentWidget;

public:
    Sequencer(const Glib::ustring &entryGlade, Gtk::VBox *parentWidget);

    void addEntry();
    void deleteEntry(unsigned int index);
};//Sequencer

#endif

