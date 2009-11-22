

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
