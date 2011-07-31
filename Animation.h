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
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
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

    boost::shared_ptr<Keyframe> deepClone();

private:
    KeySelectedType::KeySelectedType selectedState;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//Keyframe

class Animation
{
    boost::shared_ptr<Animation> instanceOf;
    std::map<int, boost::shared_ptr<Keyframe> > keyframes;
    int *startTick;

    Animation() {}

    void absorbCurve(boost::shared_ptr<Animation> otherAnim);

public:
    Animation(SequencerEntryBlock *owningEntryBlock, boost::shared_ptr<Animation> instanceOf);
    ~Animation();

    boost::shared_ptr<Animation> deepClone();

    void addKey(boost::shared_ptr<Keyframe> keyframe);
    //void deleteKey(int tick);
    void deleteKey(boost::shared_ptr<Keyframe> keyframe);
    int getNumKeyframes() const;
    boost::shared_ptr<Keyframe> getKeyframe(unsigned int index);
    boost::shared_ptr<Keyframe> getKeyframeAtTick(int tick);

    boost::shared_ptr<Keyframe> getNextKeyframe(boost::shared_ptr<Keyframe> keyframe);

    double sample(int tick);

    void render(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight);
//    std::pair<int, SelectedEntity> getSelection(int tick, double value);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
    friend class SequencerEntryBlock;
};//Animation

void drawAnimation(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, 
                    std::vector<int> &verticalPixelTickValues, std::vector<float> &horizontalPixelValues, boost::shared_ptr<Animation> animation);


BOOST_CLASS_VERSION(Animation, 1);
BOOST_CLASS_VERSION(Keyframe, 1);


#endif


