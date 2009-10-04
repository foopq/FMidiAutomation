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
    std::map<int, boost::shared_ptr<Tempo> > tempoChanges; //int index is tick value (>= 0)

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;

};//FMidiAutomationData

BOOST_CLASS_VERSION(FMidiAutomationData, 1);

#endif
