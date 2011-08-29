/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationData.h"
#include "FMidiAutomationMainWindow.h"
#include "boost/serialization/map.hpp" 

namespace
{


}//anonymous namespace


void FMidiAutomationData::addTempoChange(int tick, std::shared_ptr<Tempo> tempo)
{
    tempoChanges.insert(std::make_pair(tick, tempo));
    updateTempoChangesUIData(tempoChanges);
}//addTempoChange

void FMidiAutomationData::removeTempoChange(int tick)
{
    tempoChanges.erase(tempoChanges.find(tick));
    updateTempoChangesUIData(tempoChanges);
}//removeTempoChange

template<class Archive>
void FMidiAutomationData::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(tempoChanges);
}//serialize

template<class Archive> 
void GraphState::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(baseOffsetX);
    ar & BOOST_SERIALIZATION_NVP(baseOffsetY);
    ar & BOOST_SERIALIZATION_NVP(inMotion);
    ar & BOOST_SERIALIZATION_NVP(zeroithTickPixel);
    ar & BOOST_SERIALIZATION_NVP(offsetX);
    ar & BOOST_SERIALIZATION_NVP(offsetY);
    ar & BOOST_SERIALIZATION_NVP(barsSubdivisionAmount);
    ar & BOOST_SERIALIZATION_NVP(ticksPerPixel);
    ar & BOOST_SERIALIZATION_NVP(valuesPerPixel);
    ar & BOOST_SERIALIZATION_NVP(curPointerTick);
    ar & BOOST_SERIALIZATION_NVP(curPointerTickXPixel);
    ar & BOOST_SERIALIZATION_NVP(leftMarkerTick);
    ar & BOOST_SERIALIZATION_NVP(rightMarkerTick);
    ar & BOOST_SERIALIZATION_NVP(leftMarkerTickXPixel);
    ar & BOOST_SERIALIZATION_NVP(rightMarkerTickXPixel);
    //ar & BOOST_SERIALIZATION_NVP(currentlySelectedEntryOriginalStartTicks); -- not serialized
    //ar & BOOST_SERIALIZATION_NVP(currentlySelectedEntryBlocks); -- not serialized
    ar & BOOST_SERIALIZATION_NVP(lastSequencerPointerTick);
}//serialize

template void FMidiAutomationData::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void GraphState::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void FMidiAutomationData::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
template void GraphState::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);




