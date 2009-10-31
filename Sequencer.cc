#include "Sequencer.h"
#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

static const unsigned int entryWindowHeight = 130 + 6; //size plus padding
static const unsigned int smallEntryWindowHeight = 40 + 6; //size plus padding

SequencerEntry::SequencerEntry(const Glib::ustring &entryGlade, boost::function<void (Gtk::Viewport *current, Gtk::Viewport *next)> doSwapEntryBoxCallback_)
{
    uiXml = Gtk::Builder::create_from_string(entryGlade);
    uiXml->get_widget("entryViewport", mainWindow);
    uiXml->get_widget("smallEntryViewport", smallWindow);

    mainWindow->get_parent()->remove(*mainWindow);
    smallWindow->get_parent()->remove(*smallWindow);

    isFullBox = false;
    doSwapEntryBoxCallback = doSwapEntryBoxCallback_;
}//constructor

Gtk::Widget *SequencerEntry::getHookWidget()
{
//    return mainWindow;
    return smallWindow;    
}//getHookWidget

Sequencer::Sequencer(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_)
{
    entryGlade = entryGlade_;
    parentWidget = parentWidget_;

    tmpLabel.set_text("");
//    tmpLabel.set_size_request(-1, 1000);
    tmpLabel.show();

//    tmpLabelBox.pack_start(tmpLabel, true, true, 0);
//    tmpLabelBox.show();
    
    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));
}//constructor

void Sequencer::addEntry()
{
    boost::function<void (Gtk::Viewport *current, Gtk::Viewport *next)> doSwapEntryBoxCallback =
        boost::lambda::bind(boost::mem_fn(&Sequencer::doSwapEntryBox), boost::lambda::var(this), boost::lambda::_1, boost::lambda::_2);

    boost::shared_ptr<SequencerEntry> newEntry(new SequencerEntry(entryGlade, doSwapEntryBoxCallback));
    entries.push_back(newEntry);

    Gtk::Widget *entryHookWidget = newEntry->getHookWidget();

//    parentWidget->set_size_request(-1, 20 * parentWidget->children().size());
//    parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));
    parentWidget->children().insert(parentWidget->children().begin(), Gtk::Box_Helpers::Element(*entryHookWidget));

    int height = parentWidget->get_height();
    int labelHeight = height - (parentWidget->children().size() - 1) * smallEntryWindowHeight;
    labelHeight = std::max(labelHeight, -1);
    tmpLabel.set_size_request(-1, labelHeight);
}//addEntry

void Sequencer::deleteEntry(unsigned int index)
{
}//deleteEntry

void Sequencer::doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next)
{

}//doSwapEntryBox


