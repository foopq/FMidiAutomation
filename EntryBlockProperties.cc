#include "EntryBlockProperties.h"
#include "Sequencer.h"
#include <iostream>

EntryBlockProperties::EntryBlockProperties(Glib::RefPtr<Gtk::Builder> uiXml, boost::shared_ptr<SequencerEntryBlock> entryBlock)
{
    Gtk::Dialog *mainDialog;
    uiXml->get_widget("entryBoxProperties", mainDialog);

    Glib::ustring title = entryBlock->getTitle();

    Gtk::Entry *entry;
    uiXml->get_widget("entryBoxLabelEntry", entry);
    entry->set_text(title);

    int result = mainDialog->run();
    if (result == 0) {
        Glib::ustring newEntryTitle = entry->get_text();
        if ((newEntryTitle.empty() == false) && (newEntryTitle != entryBlock->getTitle())) {
            newTitle = newEntryTitle;
        }//if
    }//if

    mainDialog->hide();
}//constructor


