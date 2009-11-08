

#ifndef __ENTRYBLOCKPROPERTIES_H
#define __ENTRYBLOCKPROPERTIES_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

class SequencerEntryBlock;

struct EntryBlockProperties
{
    bool wasChanged;
    Glib::ustring newTitle;
    Gdk::Color newColour;

    EntryBlockProperties(Glib::RefPtr<Gtk::Builder> uiXml, boost::shared_ptr<SequencerEntryBlock> entryBlock);
};//EntryBlockProperties

#endif
