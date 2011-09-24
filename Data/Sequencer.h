/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/



#ifndef __SEQUENCER_H
#define __SEQUENCER_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>
#include "fmaipair.h"

class SequencerEntry;
struct ProcessRecordedMidiCommand;


class Sequencer : public std::enable_shared_from_this<Sequencer>
{
    std::vector<std::shared_ptr<SequencerEntry> > entries;

protected:
    void setEntryMap(decltype(Sequencer::entries) &entryMap);

public:
    typedef decltype(Sequencer::entries) SequencerEntriesType;

    Sequencer();

    void addEntry(std::shared_ptr<SequencerEntry> entry);
    void deleteEntry(std::shared_ptr<SequencerEntry> entry);

    fmaipair<decltype(entries.begin()), decltype(entries.end())> getEntryPair();
    unsigned int getNumEntries() const;
 
    void doLoad(boost::archive::xml_iarchive &inputArchive);
    void doSave(boost::archive::xml_oarchive &outputArchive);

    //For midi processing
    void cloneEntryMap();
    decltype(entries) getEntryMap();    

//    std::shared_ptr<VectorStreambuf> serializeEntryMap();
//    void deserializeEntryMap(std::shared_ptr<VectorStreambuf> streambuf);

    friend struct ProcessRecordedMidiCommand;
};//Sequencer



BOOST_CLASS_VERSION(Sequencer, 1);


#endif

