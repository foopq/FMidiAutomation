/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#include <fstream>
#include "FMidiAutomationData.h"
#include "FMidiAutomationMainWindow.h"
#include "boost/serialization/map.hpp" 
#include "GraphState.h"
#include "SerializationHelper.h"
#include "Tempo.h"
#include "Sequencer.h"
#include "fmaipair.h"

namespace
{

Glib::ustring readEntryGlade()
{
    std::ifstream inputStream("FMidiAutomationEntry.glade");
    assert(inputStream.good());
    if (false == inputStream.good()) {
        return "";
    }//if

    Glib::ustring retString;
    std::string line;
    while (std::getline(inputStream,line)) {
        retString += line;
    }//while

    return retString;
}//readEntryGlade

}//anonymous namespace

FMidiAutomationData::FMidiAutomationData()
{
    sequencer.reset(new Sequencer);
    addTempoChange(0U, std::shared_ptr<Tempo>(new Tempo(12000, 4, 4)));
    entryGlade = readEntryGlade();
}//constructor

FMidiAutomationData::~FMidiAutomationData()
{
    //Nothing
}//destructor

std::shared_ptr<Sequencer> FMidiAutomationData::getSequencer()
{
    return sequencer;
}//getSequencer

Glib::ustring &FMidiAutomationData::getEntryGlade()
{
    return entryGlade;
}//getEntryGlade

void FMidiAutomationData::addTempoChange(int tick, std::shared_ptr<Tempo> tempo)
{
    tempoChanges.insert(std::make_pair(tick, tempo));
    updateTempoChangesUIData(getTempoChanges());
}//addTempoChange

void FMidiAutomationData::removeTempoChange(int tick)
{
    tempoChanges.erase(tempoChanges.find(tick));
    updateTempoChangesUIData(getTempoChanges());
}//removeTempoChange

bool FMidiAutomationData::HasTempoChangeAtTick(int tick)
{
    return (tempoChanges.find(tick) != tempoChanges.end());
}//HasTempoChangeAtTick

std::shared_ptr<Tempo> FMidiAutomationData::getTempoChangeAtTick(int tick)
{
    if (tempoChanges.find(tick) != tempoChanges.end()) {
        return tempoChanges[tick];
    } else {
        return std::shared_ptr<Tempo>();
    }//if
}//getTempoChangeAtTick

fmaipair<FMidiAutomationData::TempoChangesIter, FMidiAutomationData::TempoChangesIter> FMidiAutomationData::getTempoChanges()
{
    return fmai_make_pair(tempoChanges.begin(), tempoChanges.end());
}//getTempoChanges

FMidiAutomationData::TempoChangesIter FMidiAutomationData::getTempoChangesLowerBound(int tick)
{
    return tempoChanges.lower_bound(tick);
}//getTempoChangesLowerBound

/*
template<class Archive>
void FMidiAutomationData::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(tempoChanges);
    ar & BOOST_SERIALIZATION_NVP(sequencer);
}//serialize
*/

void FMidiAutomationData::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    int FMidiAutomationDataVersion = 1; 

    inputArchive & BOOST_SERIALIZATION_NVP(FMidiAutomationDataVersion);
    inputArchive & BOOST_SERIALIZATION_NVP(tempoChanges);
    sequencer->doLoad(inputArchive);
}//doLoad

void FMidiAutomationData::doSave(boost::archive::xml_oarchive &outputArchive)
{
    int FMidiAutomationDataVersion = 1;

    outputArchive & BOOST_SERIALIZATION_NVP(FMidiAutomationDataVersion);
    outputArchive & BOOST_SERIALIZATION_NVP(tempoChanges);
    sequencer->doSave(outputArchive);
}//doSave

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

    ar & BOOST_SERIALIZATION_NVP(didMoveKey);
    ar & BOOST_SERIALIZATION_NVP(didMoveKeyOutTangent);
    ar & BOOST_SERIALIZATION_NVP(didMoveKeyInTangent);

    ar & BOOST_SERIALIZATION_NVP(displayMode);
    ar & BOOST_SERIALIZATION_NVP(lastSequencerPointerTick);
    ar & BOOST_SERIALIZATION_NVP(lastSequencerLeftPointerTick);
    ar & BOOST_SERIALIZATION_NVP(lastSequencerRightPointerTick);

    ar & BOOST_SERIALIZATION_NVP(doingRubberBanding);
    ar & BOOST_SERIALIZATION_NVP(insertMode);
}//serialize

//template void FMidiAutomationData::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void GraphState::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

//template void FMidiAutomationData::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
template void GraphState::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);




