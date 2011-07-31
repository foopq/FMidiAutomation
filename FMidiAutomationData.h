/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __FMIDIAUTOMATIONDATA_H
#define __FMIDIAUTOMATIONDATA_H

#include <gtkmm.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include "Tempo.h"

struct FMidiAutomationData
{
    Glib::ustring entryGlade;
    std::map<int, boost::shared_ptr<Tempo> > tempoChanges; //int index is tick value (>= 0)

    void addTempoChange(int tick, boost::shared_ptr<Tempo> tempo);
    void removeTempoChange(int tick);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//FMidiAutomationData

BOOST_CLASS_VERSION(FMidiAutomationData, 1);

#endif
