/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/




#ifndef __ENTRYBLOCKPROPERTIES_H
#define __ENTRYBLOCKPROPERTIES_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>

class SequencerEntryBlock;

struct EntryBlockProperties
{
    bool wasChanged;
    Glib::ustring newTitle;
    Gdk::Color newColour;

    EntryBlockProperties(Glib::RefPtr<Gtk::Builder> uiXml, std::shared_ptr<SequencerEntryBlock> entryBlock);
};//EntryBlockProperties

#endif
