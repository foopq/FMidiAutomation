/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "EntryBlockProperties.h"
#include "Data/SequencerEntryBlock.h"
#include <iostream>

EntryBlockProperties::EntryBlockProperties(Glib::RefPtr<Gtk::Builder> uiXml, std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    Gtk::Dialog *mainDialog;
    uiXml->get_widget("entryBoxProperties", mainDialog);

    Glib::ustring title = entryBlock->getTitle();

    Gtk::Entry *entry;
    uiXml->get_widget("entryBoxLabelEntry", entry);
    entry->set_text(title);

    wasChanged = false;
    int result = mainDialog->run();
    if (result == 0) {
        Glib::ustring newEntryTitle = entry->get_text();

        if ((newEntryTitle.empty() == false) && (newEntryTitle != entryBlock->getTitle())) {
            newTitle = newEntryTitle;
            wasChanged = true;
        }//if
    }//if

    mainDialog->hide();
}//constructor


