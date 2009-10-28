#include "Sequencer.h"

SequencerEntry::SequencerEntry(const Glib::ustring &entryGlade)
{
    uiXml = Gtk::Builder::create_from_string(entryGlade);
    uiXml->get_widget("entryViewport", mainWindow);
}//constructor

Gtk::Widget *SequencerEntry::getHookWidget()
{
    return mainWindow;
}//getHookWidget

Sequencer::Sequencer(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_)
{
    entryGlade = entryGlade_;
    parentWidget = parentWidget_;
}//constructor

void Sequencer::addEntry()
{
    boost::shared_ptr<SequencerEntry> newEntry(new SequencerEntry(entryGlade));
    entries.push_back(newEntry);

    Gtk::Widget *entryHookWidget = newEntry->getHookWidget();
    entryHookWidget->reparent(*parentWidget);

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));

    parentWidget->set_size_request(-1, 300 * parentWidget->children().size());
}//addEntry

void Sequencer::deleteEntry(unsigned int index)
{
}//deleteEntry



