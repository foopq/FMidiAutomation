/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#include "Sequencer.h"
#include "SequencerEntry.h"
#include "Animation.h"
#include "jack.h"
#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "SerializationHelper.h"
#include "Globals.h"
#include "GraphState.h"
#include "FMidiAutomationMainWindow.h"


SequencerEntryBlock::SequencerEntryBlock(std::shared_ptr<SequencerEntry> owningEntry_, int startTick_, std::shared_ptr<SequencerEntryBlock> instanceOf_)
{
    startTick_ = std::max(startTick_, 0);

    startTick = startTick_;
    instanceOf = instanceOf_;
//    duration = 200;

    owningEntry = owningEntry_;

    valuesPerPixel = std::numeric_limits<double>::max();
    offsetY = 0;

    if (instanceOf == NULL) {
        curve.reset(new Animation(this, std::shared_ptr<Animation>()));
        secondaryCurve.reset(new Animation(this, std::shared_ptr<Animation>()));
    } else {
        curve.reset(new Animation(this, instanceOf->curve));
        secondaryCurve.reset(new Animation(this, instanceOf->secondaryCurve));
    }//if

    curPointerTick = startTick;
    leftMarkerTick = -1;
    rightMarkerTick = -1;
}//constructor

std::shared_ptr<SequencerEntryBlock> SequencerEntryBlock::deepClone()
{
    std::shared_ptr<SequencerEntryBlock> clone(new SequencerEntryBlock);

    clone->owningEntry = owningEntry;
    clone->title = title;
    clone->startTick = startTick;

    //clone->std::shared_ptr<SequencerEntryBlock> instanceOf;
    //int duration; //in ticks, or unused if instanceOf isn't NULL
    
    clone->curve = curve->deepClone();
    clone->secondaryCurve = secondaryCurve->deepClone();

    clone->valuesPerPixel = valuesPerPixel;
    clone->offsetY = offsetY;

    return clone;
}//deepClone

std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > SequencerEntryBlock::deepCloneSplit(int tick)
{
    std::shared_ptr<SequencerEntryBlock> clone1(new SequencerEntryBlock);
    std::shared_ptr<SequencerEntryBlock> clone2(new SequencerEntryBlock);

    int animOffset = tick - startTick;

    clone1->owningEntry = owningEntry;
    clone1->title = title + " Split";
    clone1->startTick = startTick;
    clone1->valuesPerPixel = valuesPerPixel;
    clone1->offsetY = offsetY;

    clone2->owningEntry = owningEntry;
    clone2->title = title + " Split";
    clone2->startTick = tick;
    clone2->valuesPerPixel = valuesPerPixel;
    clone2->offsetY = offsetY;

    auto animationClonePair = curve->deepCloneSplit(animOffset, clone1.get(), clone2.get());
    std::shared_ptr<Animation> animationClone1A = animationClonePair.first;
    std::shared_ptr<Animation> animationClone1B = animationClonePair.second;

    animationClonePair = secondaryCurve->deepCloneSplit(animOffset, clone1.get(), clone2.get());
    std::shared_ptr<Animation> animationClone2A = animationClonePair.first;
    std::shared_ptr<Animation> animationClone2B = animationClonePair.second;

    clone1->curve = animationClone1A;
    clone1->secondaryCurve = animationClone2A;

    clone2->curve = animationClone1B;
    clone2->secondaryCurve = animationClone2B;
   
    return std::make_pair(clone1, clone2);
}//deepCloneSplit

void SequencerEntryBlock::setInstanceOf(std::shared_ptr<SequencerEntryBlock> instanceOf_)
{
    instanceOf = instanceOf_;

    curve.reset(new Animation(this, instanceOf->curve));
    secondaryCurve.reset(new Animation(this, instanceOf->secondaryCurve));
}//setInstanceOf

double SequencerEntryBlock::getValuesPerPixel()
{
    return valuesPerPixel;
}//getValuesPerPixel

double SequencerEntryBlock::getOffsetY()
{
    return offsetY;
}//getOffsetY

void SequencerEntryBlock::setValuesPerPixel(double valuesPerPixel_)
{
    valuesPerPixel = valuesPerPixel_;
}//setValuesPerPixel

void SequencerEntryBlock::setOffsetY(double offsetY_)
{
    offsetY = offsetY_;
}//setOffsetY

void SequencerEntryBlock::moveBlock(int startTick_)
{
    std::shared_ptr<SequencerEntry> owningEntry_ = owningEntry.lock();
    if (owningEntry_ == NULL) {
        return;
    }//if

    if (owningEntry_->getEntryBlock(startTick_) != NULL) {
        return;
    }//if

    owningEntry_->removeEntryBlock(shared_from_this());

    startTick_ = std::max(startTick_, 0);
    std::cout << "move block from: " << startTick << " to " << startTick_ << std::endl;

    startTick = startTick_;
    owningEntry_->addEntryBlock(startTick, shared_from_this());

    if (curPointerTick < startTick) {
        curPointerTick = startTick;
    }//if

    if (leftMarkerTick < startTick) {
        leftMarkerTick = -1;
    }//if

    if (rightMarkerTick < startTick) {
        rightMarkerTick = -1;
    }//if
}//moveBlock

std::shared_ptr<Keyframe> SequencerEntryBlock::getNextKeyframe(std::shared_ptr<Keyframe> keyframe)
{
    std::shared_ptr<Keyframe> afterFirst = curve->getNextKeyframe(keyframe);

    if (afterFirst != NULL) {
        return afterFirst;
    }//if

    std::shared_ptr<Keyframe> afterSecond = secondaryCurve->getNextKeyframe(keyframe);

    if (afterSecond != NULL) {
        return afterSecond;
    }//if

    return std::shared_ptr<Keyframe>();
}//getNextKeyframe

//void SequencerEntryBlock::setDuration(int duration_)
//{
//    duration = duration_;
//}//setDuration

void SequencerEntryBlock::cloneCurves(std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    curve->absorbCurve(entryBlock->curve);
    secondaryCurve->absorbCurve(entryBlock->secondaryCurve);
}//cloneCurves

void SequencerEntryBlock::setTitle(const Glib::ustring &title_)
{
    title = title_;
}//setTitle

int SequencerEntryBlock::getStartTick() const
{
    return startTick;
}//getStartTick

int *SequencerEntryBlock::getRawStartTick()
{
    return &startTick;
}//getRawStartTick

int SequencerEntryBlock::getDuration() const
{
    if (instanceOf == NULL) {
        int duration = 0;

        if (curve != NULL) {
            int numKeys = curve->getNumKeyframes();
            if (numKeys > 0) {
                std::shared_ptr<Keyframe> lastKey = curve->getKeyframe(numKeys-1);

                duration = std::max(duration, lastKey->tick);
            }//if
        }//if

        if (secondaryCurve != NULL) {
            int numKeys = secondaryCurve->getNumKeyframes();
            if (numKeys > 0) {
                std::shared_ptr<Keyframe> lastKey = secondaryCurve->getKeyframe(numKeys-1);

                duration = std::max(duration, lastKey->tick);
            }//if
        }//if

        return duration;
    } else {
        return instanceOf->getDuration();
    }//if
}//getDuration

Glib::ustring SequencerEntryBlock::getTitle() const
{
    return title;
}//getTitle

void SequencerEntryBlock::setUITickPositions(int main, int left, int right)
{
    curPointerTick = main;
    leftMarkerTick = left;
    rightMarkerTick = right;
}//setUITickPositions

std::tuple<int, int, int> SequencerEntryBlock::getUITickPositions()
{
    return std::make_tuple(curPointerTick, leftMarkerTick, rightMarkerTick);
}//getUITickPositions

std::shared_ptr<SequencerEntryBlock> SequencerEntryBlock::getInstanceOf() const
{
    return instanceOf;
}//getInstanceOf

std::shared_ptr<SequencerEntry> SequencerEntryBlock::getOwningEntry() const
{
    return owningEntry.lock();
}//getOwningEntry

std::shared_ptr<Animation> SequencerEntryBlock::getCurve()
{
   return curve;
}//getCurve

std::shared_ptr<Animation> SequencerEntryBlock::getSecondaryCurve()
{
    return secondaryCurve;
}//getSecondaryCurve

void SequencerEntryBlock::renderCurves(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
    curve->render(context, graphState, areaWidth, areaHeight);
    secondaryCurve->render(context, graphState, areaWidth, areaHeight);
}//renderCurves

template<class Archive>
void SequencerEntryBlock::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(owningEntry);
    ar & BOOST_SERIALIZATION_NVP(startTick);
    ar & BOOST_SERIALIZATION_NVP(instanceOf);
//    ar & BOOST_SERIALIZATION_NVP(duration);
    ar & BOOST_SERIALIZATION_NVP(valuesPerPixel);
    ar & BOOST_SERIALIZATION_NVP(offsetY);
    ar & BOOST_SERIALIZATION_NVP(curPointerTick);
    ar & BOOST_SERIALIZATION_NVP(leftMarkerTick);
    ar & BOOST_SERIALIZATION_NVP(rightMarkerTick);

    std::string titleStr = Glib::locale_from_utf8(title);
    ar & BOOST_SERIALIZATION_NVP(titleStr);
    title = titleStr;

    ar & BOOST_SERIALIZATION_NVP(curve);
    ar & BOOST_SERIALIZATION_NVP(secondaryCurve);

    curve->startTick = &startTick;
    secondaryCurve->startTick = &startTick;
}//serialize

template<class Archive>
void SequencerEntryBlockSelectionInfo::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(entry);
    ar & BOOST_SERIALIZATION_NVP(entryBlock);

    int x = drawnArea.get_x();
    int y = drawnArea.get_y();
    int width = drawnArea.get_width();
    int height = drawnArea.get_height();

    ar & BOOST_SERIALIZATION_NVP(x);
    ar & BOOST_SERIALIZATION_NVP(y);
    ar & BOOST_SERIALIZATION_NVP(width);
    ar & BOOST_SERIALIZATION_NVP(height);

    drawnArea.set_x(x);
    drawnArea.set_y(y);
    drawnArea.set_width(width);
    drawnArea.set_height(height);
}//serialize


template void SequencerEntryBlock::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void SequencerEntryBlockSelectionInfo::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void SequencerEntryBlock::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
template void SequencerEntryBlockSelectionInfo::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);

