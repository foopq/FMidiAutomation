/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __ANIMATION_H
#define __ANIMATION_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>
#include <boost/function.hpp>
#include <boost/serialization/version.hpp>
#include "FMidiAutomationMainWindow.h"

class SequencerEntryBlock;
class SequencerEntry;

namespace CurveType {
enum CurveType
{
    Init,
    Step,
    Linear,
    Bezier
};//CurveType
}//namespace CurveType

namespace KeySelectedType {
enum KeySelectedType
{
    NotSelected,
    Key,
    InTangent,
    OutTangent
};//KeySelectedType
}//namespace KeySelectedType

struct Keyframe
{
    Keyframe();

    int tick;
    double value;
    double inTangent[2];
    double outTangent[2];
    CurveType::CurveType curveType;

    int drawnStartX;
    int drawnStartY;
    int drawnOutX;
    int drawnOutY;
    int drawnInX;
    int drawnInY;

    //bool isSelected;
    KeySelectedType::KeySelectedType getSelectedState();
    void setSelectedState(KeySelectedType::KeySelectedType state);

    std::shared_ptr<Keyframe> deepClone();

private:
    KeySelectedType::KeySelectedType selectedState;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//Keyframe

class Animation : public std::enable_shared_from_this<Animation>
{
    std::shared_ptr<Animation> instanceOf;
    std::map<int, std::shared_ptr<Keyframe> > keyframes; //XXX: We use the ordered map properties.. reconsider if ever changing to a hash map
    int *startTick;

    Animation() {}

    void absorbCurve(std::shared_ptr<Animation> otherAnim);

public:
    Animation(SequencerEntryBlock *owningEntryBlock, std::shared_ptr<Animation> instanceOf);
    ~Animation();

    std::shared_ptr<Animation> deepClone();
    std::pair<std::shared_ptr<Animation>, std::shared_ptr<Animation> > deepCloneSplit(int offset, SequencerEntryBlock *owningEntryBlock1, 
                                                                                        SequencerEntryBlock *owningEntryBlock2);

    void addKey(std::shared_ptr<Keyframe> keyframe);
    //void deleteKey(int tick);
    void deleteKey(std::shared_ptr<Keyframe> keyframe);
    int getNumKeyframes() const;
    std::shared_ptr<Keyframe> getKeyframe(unsigned int index);
    std::shared_ptr<Keyframe> getKeyframeAtTick(int tick);

    std::shared_ptr<Keyframe> getPrevKeyframe(std::shared_ptr<Keyframe> keyframe);
    std::shared_ptr<Keyframe> getNextKeyframe(std::shared_ptr<Keyframe> keyframe);

    void mergeOtherAnimation(std::shared_ptr<Animation> otherAnim, InsertMode insertMode);

    double sample(int tick);

    void render(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight);
//    std::pair<int, SelectedEntity> getSelection(int tick, double value);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
    friend class SequencerEntryBlock;
};//Animation

void drawAnimation(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, 
                    std::vector<int> &verticalPixelTickValues, std::vector<float> &horizontalPixelValues, std::shared_ptr<Animation> animation);


BOOST_CLASS_VERSION(Animation, 1);
BOOST_CLASS_VERSION(Keyframe, 1);


#endif


