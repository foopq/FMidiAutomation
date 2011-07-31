/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/




#ifndef __ENTRYPROPERTIES_H
#define __ENTRYPROPERTIES_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

class SequencerEntry;
struct SequencerEntryImpl;

struct EntryProperties
{
    bool wasChanged;
    boost::shared_ptr<SequencerEntryImpl> origImpl;
    boost::shared_ptr<SequencerEntryImpl> newImpl;
    Glib::RefPtr<Gtk::Builder> uiXml;

    EntryProperties(Glib::RefPtr<Gtk::Builder> uiXml, boost::shared_ptr<SequencerEntry> entry, bool hideCancelButton);

private:
    void ccControllerValueChanged();
    void lsbValueChanged();
};//EntryProperties

#endif
