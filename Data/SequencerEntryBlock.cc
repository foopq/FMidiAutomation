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
#include <boost/lexical_cast.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "SerializationHelper.h"


SequencerEntryBlock::SequencerEntryBlock(std::shared_ptr<SequencerEntry> owningEntry_, int startTick_, std::shared_ptr<SequencerEntryBlock> instanceOf_)
{
    startTick_ = std::max(startTick_, 0);

    startTick = startTick_;
    instanceOf = instanceOf_;
//    duration = 200;

    owningEntry = owningEntry_;

    if (instanceOf == nullptr) {
        curve.reset(new Animation(this, std::shared_ptr<Animation>()));
        secondaryCurve.reset(new Animation(this, std::shared_ptr<Animation>()));
    } else {
        curve.reset(new Animation(this, instanceOf->curve));
        secondaryCurve.reset(new Animation(this, instanceOf->secondaryCurve));
    }//if
}//constructor

std::shared_ptr<SequencerEntryBlock> SequencerEntryBlock::deepClone(std::shared_ptr<SequencerEntry> owningEntry_, int startTick_)
{
std::cout << "deepClone: " << startTick_ << " - " << startTick << std::endl;

    std::shared_ptr<SequencerEntryBlock> clone(new SequencerEntryBlock);

    if (owningEntry_ != nullptr) {
        clone->owningEntry = owningEntry_;
    } else {
        clone->owningEntry = this->owningEntry;
    }//if

    clone->title = title;
    clone->startTick = startTick_;

    //clone->std::shared_ptr<SequencerEntryBlock> instanceOf;
    //int duration; //in ticks, or unused if instanceOf isn't nullptr
    
    clone->curve = curve->deepClone(clone->getRawStartTick());
    clone->secondaryCurve = secondaryCurve->deepClone(clone->getRawStartTick());

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

    clone2->owningEntry = owningEntry;
    clone2->title = title + " Split";
    clone2->startTick = tick;

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

void SequencerEntryBlock::moveBlock(int startTick_)
{
    std::shared_ptr<SequencerEntry> owningEntry_ = owningEntry.lock();
    if (owningEntry_ == nullptr) {
        return;
    }//if

    if (owningEntry_->getEntryBlock(startTick_) != nullptr) {
        return;
    }//if

    owningEntry_->removeEntryBlock(shared_from_this());

    startTick_ = std::max(startTick_, 0);
    std::cout << "move block from: " << startTick << " to " << startTick_ << std::endl;

    startTick = startTick_;
    owningEntry_->addEntryBlock(shared_from_this());

    //FIXME: This needs to be propagated to UI counterpart(s)
    /*
    if (curPointerTick < startTick) {
        curPointerTick = startTick;
    }//if

    if (leftMarkerTick < startTick) {
        leftMarkerTick = -1;
    }//if

    if (rightMarkerTick < startTick) {
        rightMarkerTick = -1;
    }//if
    */
}//moveBlock

std::shared_ptr<Keyframe> SequencerEntryBlock::getNextKeyframe(std::shared_ptr<Keyframe> keyframe)
{
    std::shared_ptr<Keyframe> afterFirst = curve->getNextKeyframe(keyframe);

    if (afterFirst != nullptr) {
        return afterFirst;
    }//if

    std::shared_ptr<Keyframe> afterSecond = secondaryCurve->getNextKeyframe(keyframe);

    if (afterSecond != nullptr) {
        return afterSecond;
    }//if

    return std::shared_ptr<Keyframe>();
}//getNextKeyframe

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
    if (instanceOf == nullptr) {
        int duration = 0;

        if (curve != nullptr) {
            int numKeys = curve->getNumKeyframes();
            if (numKeys > 0) {
                std::shared_ptr<Keyframe> lastKey = curve->getKeyframe(numKeys-1);

                duration = std::max(duration, lastKey->tick);
            }//if
        }//if

        if (secondaryCurve != nullptr) {
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

template<class Archive>
void SequencerEntryBlock::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(owningEntry);
    ar & BOOST_SERIALIZATION_NVP(startTick);
    ar & BOOST_SERIALIZATION_NVP(instanceOf);
//    ar & BOOST_SERIALIZATION_NVP(duration);

    std::string titleStr = Glib::locale_from_utf8(title);
    ar & BOOST_SERIALIZATION_NVP(titleStr);
    title = titleStr;

    ar & BOOST_SERIALIZATION_NVP(curve);
    ar & BOOST_SERIALIZATION_NVP(secondaryCurve);

    curve->startTick = &startTick;
    secondaryCurve->startTick = &startTick;
}//serialize


template void SequencerEntryBlock::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void SequencerEntryBlock::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);

