/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __FMIDIAUTOMATIONDATA_H
#define __FMIDIAUTOMATIONDATA_H

#include <gtkmm.h>
#include <memory>
#include <vector>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include "fmaipair.h"

struct Tempo;
class Sequencer;

class FMidiAutomationData
{
    Glib::ustring entryGlade;
    std::map<int, std::shared_ptr<Tempo> > tempoChanges; //int index is tick value (>= 0)
    std::shared_ptr<Sequencer> sequencer;

public:    
    typedef decltype(tempoChanges.begin()) TempoChangesIter;

    FMidiAutomationData();
    ~FMidiAutomationData();

    std::shared_ptr<Sequencer> getSequencer();

    Glib::ustring &getEntryGlade();

    bool HasTempoChangeAtTick(int tick);
    void addTempoChange(int tick, std::shared_ptr<Tempo> tempo);
    void removeTempoChange(int tick);
    std::shared_ptr<Tempo> getTempoChangeAtTick(int tick);
    fmaipair<TempoChangesIter, TempoChangesIter> getTempoChanges();
    FMidiAutomationData::TempoChangesIter getTempoChangesLowerBound(int tick);

    //template<class Archive> void serialize(Archive &ar, const unsigned int version);
    //friend class boost::serialization::access;
    
    void doLoad(boost::archive::xml_iarchive &inputArchive);
    void doSave(boost::archive::xml_oarchive &outputArchive);
};//FMidiAutomationData

BOOST_CLASS_VERSION(FMidiAutomationData, 1);


#endif
