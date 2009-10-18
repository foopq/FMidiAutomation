#include "FMidiAutomationData.h"
#include "boost/serialization/map.hpp" 

namespace
{


}//anonymous namespace


void FMidiAutomationData::addTempoChange(int tick, boost::shared_ptr<Tempo> tempo)
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

template void FMidiAutomationData::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void FMidiAutomationData::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);



