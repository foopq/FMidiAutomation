/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __SEQUENCERENTRYBLOCK_H
#define __SEQUENCERENTRYBLOCK_H

#include <gtkmm.h>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>


struct GraphState;
class SequencerEntry;
class SequencerEntryBlock;
class Animation;
struct Keyframe;


struct SequencerEntryBlockSelectionInfo
{
    std::weak_ptr<SequencerEntry> entry;
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    Gdk::Rectangle drawnArea;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntryBlockSelectionInfo

class SequencerEntryBlock : public std::enable_shared_from_this<SequencerEntryBlock>
{
    std::weak_ptr<SequencerEntry> owningEntry;
    Glib::ustring title;
    int startTick;
    std::shared_ptr<SequencerEntryBlock> instanceOf;
    //int duration; //in ticks, or unused if instanceOf isn't NULL
    std::shared_ptr<Animation> curve;
    std::shared_ptr<Animation> secondaryCurve;

    //UI properties
    double valuesPerPixel;
    double offsetY;
    int curPointerTick;
    int leftMarkerTick;
    int rightMarkerTick;

    SequencerEntryBlock() {} //For serialization

protected:
    void setInstanceOf(std::shared_ptr<SequencerEntryBlock> instanceOf);

public:    
    SequencerEntryBlock(std::shared_ptr<SequencerEntry> owningEntry, int startTick, std::shared_ptr<SequencerEntryBlock> instanceOf);

    void cloneCurves(std::shared_ptr<SequencerEntryBlock> entryBlock);

    void moveBlock(int startTick);
//    void setDuration(int duration);
    void setTitle(const Glib::ustring &title);

    double getValuesPerPixel();
    double getOffsetY();
    void setValuesPerPixel(double valuesPerPixel);
    void setOffsetY(double offsetY);

    int getStartTick() const;
    int getDuration() const;
    Glib::ustring getTitle() const;
    std::shared_ptr<SequencerEntryBlock> getInstanceOf() const;

    std::shared_ptr<SequencerEntry> getOwningEntry() const;

    std::shared_ptr<Animation> getCurve();
    std::shared_ptr<Animation> getSecondaryCurve();

    std::shared_ptr<Keyframe> getNextKeyframe(std::shared_ptr<Keyframe> keyframe);

    int *getRawStartTick();

    void setUITickPositions(int main, int left, int right);
    std::tuple<int, int, int> getUITickPositions();

    std::shared_ptr<SequencerEntryBlock> deepClone();
    std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > deepCloneSplit(int tick);

    void renderCurves(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
    friend class SequencerEntry;
};//SequencerEntryBlock


BOOST_CLASS_VERSION(SequencerEntryBlock, 1);
BOOST_CLASS_VERSION(SequencerEntryBlockSelectionInfo, 1);


#endif
