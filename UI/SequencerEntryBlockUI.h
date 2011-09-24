/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __SEQUENCERENTRYBLOCKUI_H
#define __SEQUENCERENTRYBLOCKUI_H

#include <gtkmm.h>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>
#include <unordered_map>

struct GraphState;
class SequencerEntryUI;
class SequencerEntryBlockUI;
class SequencerEntryBlock;
class Animation;
struct Keyframe;
enum class KeySelectedType : char;

struct SequencerEntryBlockSelectionInfo
{
    std::weak_ptr<SequencerEntryUI> entry;
    std::shared_ptr<SequencerEntryBlockUI> entryBlock;
    Gdk::Rectangle drawnArea;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntryBlockSelectionInfo

class SequencerEntryBlockUI : public std::enable_shared_from_this<SequencerEntryBlockUI>
{
    std::shared_ptr<SequencerEntryBlock> baseEntryBlock;
    std::weak_ptr<SequencerEntryUI> owningEntry;

    //UI properties
    double valuesPerPixel;
    double offsetY;
    int curPointerTick;
    int leftMarkerTick;
    int rightMarkerTick;

    std::unordered_map<std::shared_ptr<Keyframe>, KeySelectedType> keyframeSelectionStates;

public:    
    SequencerEntryBlockUI(std::shared_ptr<SequencerEntryBlock> baseEntryBlock, std::shared_ptr<SequencerEntryUI> entry);

    double getValuesPerPixel();
    double getOffsetY();
    void setValuesPerPixel(double valuesPerPixel);
    void setOffsetY(double offsetY);

    std::shared_ptr<SequencerEntryBlock> getBaseEntryBlock() const;
    std::shared_ptr<SequencerEntryUI> getOwningEntry();

    void setUITickPositions(int main, int left, int right);
    std::tuple<int, int, int> getUITickPositions();

    KeySelectedType getSelectedState(std::shared_ptr<Keyframe> keyframe);
    void setSelectedState(std::shared_ptr<Keyframe> keyframe, KeySelectedType state);

    std::shared_ptr<SequencerEntryBlockUI> deepClone(std::shared_ptr<SequencerEntryUI> newOwningEntry);
//    std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > deepCloneSplit(int tick);

    void renderCurves(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight);

    void doSave(boost::archive::xml_oarchive &outputArchive);
    void doLoad(boost::archive::xml_iarchive &inputArchive);

//    template<class Archive> void serialize(Archive &ar, const unsigned int version);
//    friend class boost::serialization::access;
};//SequencerEntryBlockUI




#endif
