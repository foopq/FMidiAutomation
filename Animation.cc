#include "Animation.h"
#include "Sequencer.h"
#include <boost/foreach.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

namespace
{

double calculateBezierRatio(double start, double outTan, double inTan, double end, double tick)
{
    const double epsilon = 0.0001;

    if ((tick-epsilon) < start) {
        return 0;
    }//if

    if ((tick+epsilon) > end) {
        return 1;
    }//if

    if (outTan < start) {
        outTan = start;
    }//if

    if (inTan > end) {
        inTan = end;
    }//if

    double ratio = 0.5;
    double step = ratio * 0.5;
    for (int iter = 0; iter < 25; ++iter) {
        double oneMinusRatio = 1 - ratio;

        //Solve for parametric form of bezier
        double curVal =  (start * oneMinusRatio * oneMinusRatio * oneMinusRatio) + (3 * outTan * oneMinusRatio * oneMinusRatio * ratio) +
                         (3 * inTan * oneMinusRatio * ratio * ratio) + (end * ratio * ratio * ratio);

        if (fabs((curVal - tick) / tick) < epsilon) {
            return ratio;
        }//if

        if (curVal < tick) {
            ratio += step;
        } else {
            ratio -= step;
        }//if

        step *= 0.5;
    }//for

    //Shouldn't get here, but if we do, the ratio should be close enough
    return ratio;
}//calculateBezierRatio

double doBezierInterpolation(boost::shared_ptr<Keyframe> beforeKeyframe, boost::shared_ptr<Keyframe> afterKeyframe, int tick)
{
    double ratio =  calculateBezierRatio(beforeKeyframe->tick, beforeKeyframe->outTangent[0] + beforeKeyframe->tick,
                                            afterKeyframe->tick - afterKeyframe->inTangent[0], afterKeyframe->tick, (double)tick);
            
    double oneMinusRatio = 1 - ratio;

    //Solve for parametric form of bezier
    double resultVal =  (beforeKeyframe->value * oneMinusRatio * oneMinusRatio * oneMinusRatio) + (3 * (beforeKeyframe->outTangent[1] + beforeKeyframe->value) * oneMinusRatio * oneMinusRatio * ratio) +
                        (3 * (afterKeyframe->value - afterKeyframe->inTangent[1]) * oneMinusRatio * ratio * ratio) + (afterKeyframe->value * ratio * ratio * ratio);

    return resultVal;
}//doLinearInterpolation

double doLinearInterpolation(boost::shared_ptr<Keyframe> beforeKeyframe, boost::shared_ptr<Keyframe> afterKeyframe, int tick)
{
    double ratio =  ((double)(tick - beforeKeyframe->tick)) / ((double)(afterKeyframe->tick - beforeKeyframe->tick));
    double resultVal = beforeKeyframe->value + (afterKeyframe->value - beforeKeyframe->value) * ratio;

    return resultVal;
}//doLinearInterpolation

double doStepInterpolation(boost::shared_ptr<Keyframe> beforeKeyframe, boost::shared_ptr<Keyframe> afterKeyframe, int tick)
{
    double resultVal = beforeKeyframe->value;

    return resultVal;
}//doStepInterpolation

}//anonymous namespace

Keyframe::Keyframe()
{
    tick = 0;
    value = 0;
    inTangent[0] = std::numeric_limits<int>::min();
    inTangent[1] = std::numeric_limits<int>::min();
    outTangent[0] = std::numeric_limits<int>::min();
    outTangent[1] = std::numeric_limits<int>::min();
    curveType = CurveType::Init;
    selectedState = KeySelectedType::NotSelected;
};//constructor

boost::shared_ptr<Keyframe> Keyframe::deepClone()
{
    boost::shared_ptr<Keyframe> clone(new Keyframe);
    *clone = *this;

    return clone;
}//deepClone

Animation::Animation(SequencerEntryBlock *owningEntryBlock_, boost::shared_ptr<Animation> instanceOf_)
{
    startTick = owningEntryBlock_->getRawStartTick();
    instanceOf = instanceOf_;
}//constructor

Animation::~Animation()
{
    //Nothing
}//destructor

boost::shared_ptr<Animation> Animation::deepClone()
{
    boost::shared_ptr<Animation> clone(new Animation);

    for(std::map<int, boost::shared_ptr<Keyframe> >::const_iterator mapIter = keyframes.begin(); mapIter != keyframes.end(); ++mapIter) {
        clone->keyframes[mapIter->first] = mapIter->second->deepClone();
    }//for

    clone->startTick = startTick;

    return clone;
}//deepClone

void Animation::absorbCurve(boost::shared_ptr<Animation> otherAnim)
{
    this->keyframes = otherAnim->keyframes;
}//absorbCurve

boost::shared_ptr<Keyframe> Animation::getNextKeyframe(boost::shared_ptr<Keyframe> keyframe)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

    std::map<int, boost::shared_ptr<Keyframe> >::iterator keyIter = curKeyframes->find(keyframe->tick);
    if (keyIter == curKeyframes->end()) {
        return boost::shared_ptr<Keyframe>();
    }//if

    std::map<int, boost::shared_ptr<Keyframe> >::iterator nextKeyIter = keyIter;
    ++nextKeyIter;

    if (nextKeyIter == curKeyframes->end()) {
        return boost::shared_ptr<Keyframe>();
    }//if

    return nextKeyIter->second;
}//getNextKeyframe

void Animation::addKey(boost::shared_ptr<Keyframe> keyframe)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

    if (curKeyframes->find(keyframe->tick) == curKeyframes->end()) {
        (*curKeyframes)[keyframe->tick] = keyframe;
        std::map<int, boost::shared_ptr<Keyframe> >::iterator keyIter = curKeyframes->find(keyframe->tick);
        std::map<int, boost::shared_ptr<Keyframe> >::iterator nextKeyIter = keyIter;
        std::map<int, boost::shared_ptr<Keyframe> >::iterator prevKeyIter = keyIter;
        if (curKeyframes->begin() == prevKeyIter) {
            prevKeyIter = curKeyframes->end();
        } else {
            prevKeyIter--;
        }//if
        ++nextKeyIter;

        if (CurveType::Init == keyframe->curveType) {
            if (prevKeyIter != curKeyframes->end()) {
                keyframe->curveType = prevKeyIter->second->curveType;

                if (prevKeyIter->second->curveType == CurveType::Bezier) {
                    int prevTickDiff = (keyframe->tick - prevKeyIter->second->tick) / 3;
                    keyframe->inTangent[0] = prevTickDiff;
                    keyframe->inTangent[1] = 0;

                    if (prevKeyIter->second->outTangent[0] == std::numeric_limits<int>::min()) {
                        prevKeyIter->second->outTangent[0] = prevTickDiff;
                        prevKeyIter->second->outTangent[1] = 0;
                    }//if

                    if (nextKeyIter != curKeyframes->end()) {
                        int tickDiff = (nextKeyIter->second->tick - keyframe->tick) / 3;
                        keyframe->outTangent[0] = tickDiff;
                        keyframe->outTangent[1] = 0;

                        if (nextKeyIter->second->inTangent[0] == std::numeric_limits<int>::min()) {
                            nextKeyIter->second->inTangent[0] = tickDiff;
                            nextKeyIter->second->inTangent[1] = 0;
                        }//if
                    }//if
                }//if
            } else {
                keyframe->curveType = CurveType::Linear;
            }//if
        }//if
//        std::cout << "addKey: " << keyframe->tick << "  --  " << curKeyframes->size() << std::endl;
    }//if
}//addKey

void Animation::deleteKey(boost::shared_ptr<Keyframe> keyframe)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

//    std::cout << "deleteKey1: " << keyframe->tick << std::endl;

//    deleteKey(keyframe->tick);

    if (curKeyframes->find(keyframe->tick) != curKeyframes->end()) {
        curKeyframes->erase(curKeyframes->find(keyframe->tick));
    }//if
}//deleteKey

/*
void Animation::deleteKey(int tick)
{
    tick -= *startTick;

    if (keyframes.find(tick) != keyframes.end()) {
        std::cout << "deleteKey2: " << tick << std::endl;

        keyframes.erase(keyframes.find(tick));
    }//if
    else {
std::cout << "deleteKey miss: " << tick << std::endl;
asm ("int $0x03;");
    }
}//deleteKey
*/

int Animation::getNumKeyframes() const
{
    if (instanceOf == NULL) {
        return keyframes.size();
    } else {
        return instanceOf->keyframes.size();
    }//if
}//getNumKeyframes

boost::shared_ptr<Keyframe> Animation::getKeyframe(unsigned int index)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

    index = std::min(index, (unsigned int)curKeyframes->size());
    std::map<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = curKeyframes->begin();
    std::advance(keyIter, index);
    return keyIter->second;
}//getKeyframe

boost::shared_ptr<Keyframe> Animation::getKeyframeAtTick(int tick)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

    tick -= *startTick;

    std::map<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = curKeyframes->find(tick);
    if (keyIter != curKeyframes->end()) {
        return keyIter->second;
    } else {
        return boost::shared_ptr<Keyframe>();
    }//if
}//getKeyframeAtTick

template<class Archive>
void Keyframe::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(tick);
    ar & BOOST_SERIALIZATION_NVP(value);
    ar & BOOST_SERIALIZATION_NVP(curveType);
    ar & BOOST_SERIALIZATION_NVP(inTangent);
    ar & BOOST_SERIALIZATION_NVP(outTangent);
}//serialize

template<class Archive>
void Animation::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(keyframes);
}//serialize

double Animation::sample(int tick)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

    tick -= *startTick;

    if (curKeyframes->empty() == true) {
        return 0;
    }//if

    if (curKeyframes->size() == 1) {
        return curKeyframes->begin()->second->value;
    }//if

    std::map<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = curKeyframes->upper_bound(tick);

    if (keyIter == curKeyframes->end()) {
        --keyIter;
        return keyIter->second->value;
    }//if

    if (keyIter == curKeyframes->begin()) {
        return keyIter->second->value;
    }//if

    std::map<int, boost::shared_ptr<Keyframe> >::const_iterator beforeKeyIter = keyIter;
    --beforeKeyIter;

    switch (beforeKeyIter->second->curveType) {
        case CurveType::Bezier:
            return doBezierInterpolation(beforeKeyIter->second, keyIter->second, tick);

        case CurveType::Linear:
            return doLinearInterpolation(beforeKeyIter->second, keyIter->second, tick);
        case CurveType::Step:
            return doStepInterpolation(beforeKeyIter->second, keyIter->second, tick);
        default:
            break;
    }//switch

    return 0;
}//sample


template void Animation::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
//template void Keyframe::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void Animation::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
//template void Keyframe::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);


