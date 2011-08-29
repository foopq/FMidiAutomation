#include <list>
#include "SerializationHelper.h"
#include <boost/foreach.hpp>

namespace
{
    std::list<SharedPtrMapSingletonBase *> registeredSharedPtrMapSingletonBases;
}//anonymous namespace

void RegisterSharedPtrMapSingletonBase(SharedPtrMapSingletonBase *base)
{
    registeredSharedPtrMapSingletonBases.push_back(base);
}//RegisterSharedPtrMapSingletonBase

void ResetSharedPtrMapSingletonMaps()
{
    BOOST_FOREACH (auto mapIter, registeredSharedPtrMapSingletonBases) {
        mapIter->ResetSharedPtrMapSingleton();
    }//foreach
}//ResetSharedPtrMapSingletonMaps

template <typename T>
SharedPtrMapSingleton<T> &SharedPtrMapSingleton<T>::Instance()
{
    static SharedPtrMapSingleton<T> instance;
    return instance;
}//Instance

template <typename T>
void SharedPtrMapSingleton<T>::ResetSharedPtrMapSingleton()
{
    SharedPtrMapSingleton<T> &instance = Instance();
    instance.ptrMap.clear();
}//ResetSharedPtrMapSingleton

struct Tempo;
template SharedPtrMapSingleton<Tempo> &SharedPtrMapSingleton<Tempo>::Instance();
template void SharedPtrMapSingleton<Tempo>::ResetSharedPtrMapSingleton();

struct FMidiAutomationData;
template SharedPtrMapSingleton<FMidiAutomationData> &SharedPtrMapSingleton<FMidiAutomationData>::Instance();
template void SharedPtrMapSingleton<FMidiAutomationData>::ResetSharedPtrMapSingleton();

class SequencerEntryBlock;
template SharedPtrMapSingleton<SequencerEntryBlock> &SharedPtrMapSingleton<SequencerEntryBlock>::Instance();
template void SharedPtrMapSingleton<SequencerEntryBlock>::ResetSharedPtrMapSingleton();

class Animation;
template SharedPtrMapSingleton<Animation> &SharedPtrMapSingleton<Animation>::Instance();
template void SharedPtrMapSingleton<Animation>::ResetSharedPtrMapSingleton();

struct Keyframe;
template SharedPtrMapSingleton<Keyframe> &SharedPtrMapSingleton<Keyframe>::Instance();
template void SharedPtrMapSingleton<Keyframe>::ResetSharedPtrMapSingleton();

class SequencerEntry;
template SharedPtrMapSingleton<SequencerEntry> &SharedPtrMapSingleton<SequencerEntry>::Instance();
template void SharedPtrMapSingleton<SequencerEntry>::ResetSharedPtrMapSingleton();

struct SequencerEntryImpl;
template SharedPtrMapSingleton<SequencerEntryImpl> &SharedPtrMapSingleton<SequencerEntryImpl>::Instance();
template void SharedPtrMapSingleton<SequencerEntryImpl>::ResetSharedPtrMapSingleton();






