/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

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
#include "Data/SequencerEntry.h"
#include "Animation.h"
#include "SequencerEntryBlockUI.h"


SequencerEntryBlockUI::SequencerEntryBlockUI(std::shared_ptr<SequencerEntryBlock> baseEntryBlock_, std::shared_ptr<SequencerEntryUI> owningEntry_)
{
    baseEntryBlock = baseEntryBlock_;
    owningEntry = owningEntry_;

    valuesPerPixel = std::numeric_limits<double>::max();
    offsetY = 0;

    curPointerTick = baseEntryBlock->getStartTick();
    leftMarkerTick = -1;
    rightMarkerTick = -1;
}//constructor

std::shared_ptr<SequencerEntryBlockUI> SequencerEntryBlockUI::deepClone(std::shared_ptr<SequencerEntryUI> newOwningEntry)
{
    std::shared_ptr<SequencerEntryBlockUI> clone(new SequencerEntryBlockUI(baseEntryBlock, newOwningEntry));

    clone->valuesPerPixel = valuesPerPixel;
    clone->offsetY = offsetY;

    clone->curPointerTick = curPointerTick;
    clone->leftMarkerTick = leftMarkerTick;
    clone->rightMarkerTick = rightMarkerTick;

    return clone;
}//deepClone

std::shared_ptr<SequencerEntryUI> SequencerEntryBlockUI::getOwningEntry()
{
    return owningEntry.lock();
}//getOwningEntry

/*
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
*/

double SequencerEntryBlockUI::getValuesPerPixel()
{
    return valuesPerPixel;
}//getValuesPerPixel

double SequencerEntryBlockUI::getOffsetY()
{
    return offsetY;
}//getOffsetY

void SequencerEntryBlockUI::setValuesPerPixel(double valuesPerPixel_)
{
    valuesPerPixel = valuesPerPixel_;
}//setValuesPerPixel

void SequencerEntryBlockUI::setOffsetY(double offsetY_)
{
    offsetY = offsetY_;
}//setOffsetY

void SequencerEntryBlockUI::setUITickPositions(int main, int left, int right)
{
    curPointerTick = main;
    leftMarkerTick = left;
    rightMarkerTick = right;
}//setUITickPositions

std::tuple<int, int, int> SequencerEntryBlockUI::getUITickPositions()
{
    return std::make_tuple(curPointerTick, leftMarkerTick, rightMarkerTick);
}//getUITickPositions

std::shared_ptr<SequencerEntryBlock> SequencerEntryBlockUI::getBaseEntryBlock() const
{
    return baseEntryBlock;
}//getBaseEntryBlock

KeySelectedType SequencerEntryBlockUI::getSelectedState(std::shared_ptr<Keyframe> keyframe)
{
    if (keyframeSelectionStates.find(keyframe) == keyframeSelectionStates.end()) {
        keyframeSelectionStates[keyframe] = KeySelectedType::NotSelected;
    }//if

    return keyframeSelectionStates[keyframe];
}//getSelectedState

void SequencerEntryBlockUI::setSelectedState(std::shared_ptr<Keyframe> keyframe, KeySelectedType state)
{
    keyframeSelectionStates[keyframe] = state;
}//setSelectedState

void SequencerEntryBlockUI::doSave(boost::archive::xml_oarchive &outputArchive)
{
    int SequencerEntryBlockUIVersion = 1;
    outputArchive & BOOST_SERIALIZATION_NVP(SequencerEntryBlockUIVersion);

    outputArchive & BOOST_SERIALIZATION_NVP(valuesPerPixel);
    outputArchive & BOOST_SERIALIZATION_NVP(offsetY);
    outputArchive & BOOST_SERIALIZATION_NVP(curPointerTick);
    outputArchive & BOOST_SERIALIZATION_NVP(leftMarkerTick);
    outputArchive & BOOST_SERIALIZATION_NVP(rightMarkerTick);
}//doSave

void SequencerEntryBlockUI::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    int SequencerEntryBlockUIVersion = -1;
    inputArchive & BOOST_SERIALIZATION_NVP(SequencerEntryBlockUIVersion);

    inputArchive & BOOST_SERIALIZATION_NVP(valuesPerPixel);
    inputArchive & BOOST_SERIALIZATION_NVP(offsetY);
    inputArchive & BOOST_SERIALIZATION_NVP(curPointerTick);
    inputArchive & BOOST_SERIALIZATION_NVP(leftMarkerTick);
    inputArchive & BOOST_SERIALIZATION_NVP(rightMarkerTick);
}//doLoad



void SequencerEntryBlockUI::renderCurves(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
    baseEntryBlock->getCurve()->render(context, graphState, areaWidth, areaHeight, shared_from_this());
    baseEntryBlock->getSecondaryCurve()->render(context, graphState, areaWidth, areaHeight, shared_from_this());
}//renderCurves

/*
template<class Archive>
void SequencerEntryBlockUI::serialize(Archive &ar, const unsigned int version)
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
*/


template<class Archive>
void SequencerEntryBlockSelectionInfo::serialize(Archive &ar, const unsigned int version)
{
//    ar & BOOST_SERIALIZATION_NVP(entry);
//    ar & BOOST_SERIALIZATION_NVP(entryBlock);

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


template void SequencerEntryBlockSelectionInfo::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void SequencerEntryBlockSelectionInfo::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);


