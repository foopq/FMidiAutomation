#include "Sequencer.h"
#include <iostream>

SequencerEntry::SequencerEntry(const Glib::ustring &entryGlade)
{
    uiXml = Gtk::Builder::create_from_string(entryGlade);
    uiXml->get_widget("entryViewport", mainWindow);

    mainWindow->get_parent()->remove(*mainWindow);
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

//std::cout << "1" << std::endl;
//    entryHookWidget->reparent(*parentWidget);


//std::cout << "2" << std::endl;
    parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));

//std::cout << "3" << std::endl;

//    parentWidget->set_size_request(-1, 20 * parentWidget->children().size());
//std::cout << "4" << std::endl;
}//addEntry

void Sequencer::deleteEntry(unsigned int index)
{
}//deleteEntry



