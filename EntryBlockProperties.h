

#ifndef __ENTRYBLOCKPROPERTIES_H
#define __ENTRYBLOCKPROPERTIES_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

class SequencerEntryBlock;

struct EntryBlockProperties
{
    Glib::ustring newTitle;

    EntryBlockProperties(Glib::RefPtr<Gtk::Builder> uiXml, boost::shared_ptr<SequencerEntryBlock> entryBlock);
};//EntryBlockProperties

#endif
