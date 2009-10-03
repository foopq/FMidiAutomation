#include "FMidiAutomationData.h"
#include "boost/serialization/map.hpp" 

Tempo::Tempo(unsigned int bpm_, unsigned int beatsPerBar_, unsigned int barSubDivisions_)
{
    bpm = bpm_;
    beatsPerBar = beatsPerBar_;
    barSubDivisions = barSubDivisions_;
}//constructor

template<class Archive>
void Tempo::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(bpm);
    ar & BOOST_SERIALIZATION_NVP(beatsPerBar);
    ar & BOOST_SERIALIZATION_NVP(barSubDivisions);
}//serialize


template<class Archive>
void FMidiAutomationData::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(tempoChanges);
}//serialize

template void FMidiAutomationData::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void Tempo::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);


template void FMidiAutomationData::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
template void Tempo::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);



