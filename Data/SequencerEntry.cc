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
#include "../Globals.h"
#include "../ProcessRecordedMidi.h"


namespace
{

void setThemeColours(Gtk::Widget *widget)
{
    Globals &globals = Globals::Instance();

    Gdk::Color fgColour;
    Gdk::Color bgColour;
    Gdk::Color editBoxBgColour;
    Gdk::Color textColour;
    Gdk::Color darkTextColour;
    Gdk::Color black;

    black.set_rgb(0, 0, 0);

    if (true == globals.darkTheme) {
        fgColour.set_rgb(52429, 42429, 52429);
        bgColour.set_rgb(10000, 10000, 10000);
        editBoxBgColour.set_rgb(25000, 25000, 25000);
        textColour.set_rgb(55982, 55982, 55982);
        darkTextColour.set_rgb(45982, 45982, 45982);
    }//if

    Gtk::Viewport *viewport = dynamic_cast<Gtk::Viewport *>(widget);
    if (viewport != nullptr) {
        viewport->modify_bg(Gtk::STATE_NORMAL, bgColour);
        viewport->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::Label *label = dynamic_cast<Gtk::Label *>(widget);
    if (label != nullptr) {
        label->modify_fg(Gtk::STATE_NORMAL, darkTextColour);
    }//if

    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(widget);
    if (entry != nullptr) {
        entry->modify_base(Gtk::STATE_NORMAL, bgColour);
        entry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
        entry->modify_bg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::Frame *frame = dynamic_cast<Gtk::Frame *>(widget);
    if (frame != nullptr) {
        frame->modify_bg(Gtk::STATE_NORMAL, black);
    }//if

    Gtk::Button *button = dynamic_cast<Gtk::Button *>(widget);
    if (button != nullptr) {
        button->modify_bg(Gtk::STATE_NORMAL, bgColour);
    }//if

    Gtk::Table *table = dynamic_cast<Gtk::Table *>(widget);
    if (table != nullptr) {        
        table->modify_bg(Gtk::STATE_NORMAL, bgColour);
        table->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::EventBox *eventBox = dynamic_cast<Gtk::EventBox *>(widget);
    if (eventBox != nullptr) {
        eventBox->modify_bg(Gtk::STATE_NORMAL, bgColour);
        eventBox->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::ComboBox *comboBox = dynamic_cast<Gtk::ComboBox *>(widget);
    if (comboBox != nullptr) {
        comboBox->modify_bg(Gtk::STATE_NORMAL, bgColour);
        comboBox->modify_text(Gtk::STATE_NORMAL, bgColour);
        comboBox->modify_base(Gtk::STATE_NORMAL, bgColour);
        comboBox->modify_fg(Gtk::STATE_NORMAL, bgColour);

//        comboBox->get_column(0);

        /*
        Gtk::TreeModel::Children children = comboBox->get_model()->children();
        for (Gtk::TreeRow row : children) {

        }//foreach
        */
    }//if

    /*
    Gtk::Alignment *alignment = dynamic_cast<Gtk::Alignment *>(widget);
    if (alignment != nullptr) {
        alignment->modify_bg(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_fg(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_base(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_text(Gtk::STATE_NORMAL, bgColour);
    }//if
    */

    Gtk::CellRendererText *cellRendererText = dynamic_cast<Gtk::CellRendererText *>(widget);
    if (cellRendererText != nullptr) {
        std::cout << "crt" << std::endl;
    }//if

    Gtk::Container *container = dynamic_cast<Gtk::Container *>(widget);
    if (container != nullptr) {
        Glib::ListHandle<Gtk::Widget *> children = container->get_children();
        for (Gtk::Widget *childWidget : children) {
            ::setThemeColours(childWidget);
        }//forach
    }//if
}//setThemeColours

}//anonymous namespace


SequencerEntryImpl::SequencerEntryImpl()
{
    controllerType = ControlType::CC;
    channel = 16;
    msb = 7;
    lsb = 0;

    //UI specific
    minValue = 0;
    maxValue = 127;
    sevenBit = true;
    useBothMSBandLSB = false; //implied true if sevenBit is true

    recordMode = false;
    soloMode = false;
    muteMode = false;
}//constructor

SequencerEntryImpl::~SequencerEntryImpl()
{
    //Nothing
}//destructor

std::shared_ptr<SequencerEntryImpl> SequencerEntryImpl::clone()
{
    std::shared_ptr<SequencerEntryImpl> retVal(new SequencerEntryImpl);
    *retVal = *this;
    return retVal;
}//clone

bool SequencerEntryImpl::operator==(SequencerEntryImpl &other)
{
    bool diff = false;

    diff |= this->controllerType != other.controllerType;
    diff |= this->msb != other.msb;
    diff |= this->lsb != other.lsb;
    diff |= this->minValue != other.minValue;
    diff |= this->maxValue != other.maxValue;
    diff |= this->sevenBit != other.sevenBit;
    diff |= this->useBothMSBandLSB != other.useBothMSBandLSB;
    diff |= this->channel != other.channel;
    diff |= this->title != other.title;
    diff |= this->recordMode != other.recordMode;
    diff |= this->soloMode != other.soloMode;
    diff |= this->muteMode != other.muteMode;

    return !diff;
}//operator==

SequencerEntry::SequencerEntry()
{
    impl.reset(new SequencerEntryImpl);
}//constructor

SequencerEntry::~SequencerEntry()
{
    //Nothing
}//destructor

std::shared_ptr<SequencerEntry> SequencerEntry::deepClone()
{
    std::shared_ptr<SequencerEntry> clone(new SequencerEntry);

    clone->impl.reset(new SequencerEntryImpl);
    *clone->impl = *impl;
 
    std::map<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > oldNewMap;

    for(std::map<int, std::shared_ptr<SequencerEntryBlock> >::const_iterator mapIter = entryBlocks.begin(); mapIter != entryBlocks.end(); ++mapIter) {
        std::shared_ptr<SequencerEntryBlock> entryBlockClone = mapIter->second->deepClone(mapIter->second->getOwningEntry(), mapIter->second->getStartTick());
        clone->entryBlocks[mapIter->first] = entryBlockClone;

        oldNewMap[mapIter->second] = entryBlockClone;
    }//for

    for (std::map<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> >::const_iterator mapIter = oldNewMap.begin(); mapIter != oldNewMap.end(); ++mapIter) {
        if (mapIter->second->getInstanceOf() != nullptr) {
            std::shared_ptr<SequencerEntryBlock> entryBlockClone = oldNewMap[mapIter->second->getInstanceOf()];
            assert(entryBlockClone != nullptr);
            mapIter->second->setInstanceOf(entryBlockClone);
        }//if
    }//for

    clone->inputPorts = inputPorts;
    clone->outputPorts = outputPorts;

    clone->recordTokenBuffer = recordTokenBuffer;

    return clone;
}//deepClone

std::shared_ptr<SequencerEntryImpl> SequencerEntry::getImplClone()
{
    return impl->clone();
}//getImplClone

const std::shared_ptr<SequencerEntryImpl> SequencerEntry::getImpl()
{
    return impl;
}//getImpl

void SequencerEntry::setNewDataImpl(std::shared_ptr<SequencerEntryImpl> impl_)
{
    impl = impl_;
}//setNewDataImpl

void SequencerEntry::setRecordMode(bool mode)
{
    impl->recordMode = mode;
}//setRecordMode

void SequencerEntry::setSoloMode(bool mode)
{
    impl->soloMode = mode;
}//setSoloMode

void SequencerEntry::setMuteMode(bool mode)
{
    impl->muteMode = mode;
}//setMuteMode

bool SequencerEntry::getRecordMode()
{
    return impl->recordMode;
}//getRecordMode

bool SequencerEntry::getSoloMode()
{
    return impl->soloMode;
}//getSoloMode

bool SequencerEntry::getMuteMode()
{
    return impl->muteMode;
}//getMuteMode

void SequencerEntry::addEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    removeEntryBlock(entryBlock);
    entryBlocks[entryBlock->getStartTick()] = entryBlock;
}//addEntryBlock

void SequencerEntry::removeEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    if (entryBlocks.find(entryBlock->getStartTick()) != entryBlocks.end()) {
        entryBlocks.erase(entryBlocks.find(entryBlock->getStartTick()));
    } else {
    }//if
}//removeEntryBlock

template<class Archive>
void SequencerEntry::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(impl);
    ar & BOOST_SERIALIZATION_NVP(entryBlocks);

    std::vector<std::string> inputPortsStr;
    std::vector<std::string> outputPortsStr;

    JackSingleton &jackSingleton = JackSingleton::Instance();

    for (jack_port_t *port : inputPorts) {
        std::string portName = jackSingleton.getInputPortName(port);
//        std::cout << "IN1: " << portName << std::endl;

        assert(portName.empty() == false);
        inputPortsStr.push_back(portName);
    }//foreach

    for (jack_port_t *port : outputPorts) {
        std::string portName = jackSingleton.getOutputPortName(port);
//        std::cout << "OUT1: " << portName << std::endl;

        assert(portName.empty() == false);
        outputPortsStr.push_back(portName);
    }//foreach

    ar & BOOST_SERIALIZATION_NVP(inputPortsStr);
    ar & BOOST_SERIALIZATION_NVP(outputPortsStr);

    inputPorts.clear();
    outputPorts.clear();

    for (std::string portStr : inputPortsStr) {
        jack_port_t *port = jackSingleton.getInputPort(portStr);
        inputPorts.insert(port);

//        std::cout << "IN2: " << portStr << " - " << port << std::endl;
    }//foreach

    for (std::string portStr : outputPortsStr) {
        jack_port_t *port = jackSingleton.getOutputPort(portStr);
        outputPorts.insert(port);

//        std::cout << "OUT2: " << portStr << " - " << port << std::endl;
    }//foreach

//    std::cout << "SE serialize: " << isFullBox << std::endl;
}//serialize

template<class Archive>
void SequencerEntryImpl::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(controllerType);
    ar & BOOST_SERIALIZATION_NVP(msb);
    ar & BOOST_SERIALIZATION_NVP(lsb);
    ar & BOOST_SERIALIZATION_NVP(channel);
    ar & BOOST_SERIALIZATION_NVP(minValue);
    ar & BOOST_SERIALIZATION_NVP(maxValue);
    ar & BOOST_SERIALIZATION_NVP(sevenBit);
    ar & BOOST_SERIALIZATION_NVP(useBothMSBandLSB);

    ar & BOOST_SERIALIZATION_NVP(recordMode);
    ar & BOOST_SERIALIZATION_NVP(soloMode);
    ar & BOOST_SERIALIZATION_NVP(muteMode);

    std::string titleStr = Glib::locale_from_utf8(title);
    ar & BOOST_SERIALIZATION_NVP(titleStr);
    title = titleStr;

    std::cout << "minValue: " << minValue << std::endl;
    std::cout << "maxValue: " << maxValue << std::endl;

//    std::cout << "TITLE: " << title << std::endl;
}//serialize

std::shared_ptr<SequencerEntryBlock> SequencerEntry::getEntryBlock(int tick)
{
    if (entryBlocks.find(tick) != entryBlocks.end()) {
        return entryBlocks[tick];
    } else {
        return std::shared_ptr<SequencerEntryBlock>();
    }//if
}//getEntryBlock

std::set<jack_port_t *> SequencerEntry::getInputPorts() const
{
    return inputPorts;
}//getInputPorts

std::set<jack_port_t *> SequencerEntry::getOutputPorts() const
{
    return outputPorts;
}//getOutputPorts

void SequencerEntry::setInputPorts(std::set<jack_port_t *> ports)
{
    inputPorts = ports;
}//setInputPorts

void SequencerEntry::setOutputPorts(std::set<jack_port_t *> ports)
{
    outputPorts = ports;
}//setOutputPorts

double SequencerEntry::sample(int tick)
{
    if (entryBlocks.empty() == true) {
        return 0;
    }//if

    std::map<int, std::shared_ptr<SequencerEntryBlock> >::iterator entryBlockIter = entryBlocks.upper_bound(tick);
    if (entryBlockIter != entryBlocks.begin()) {
        entryBlockIter--;
    }//if

    double val = entryBlockIter->second->getCurve()->sample(tick);

    val = std::min(val, (double)impl->maxValue);
    val = std::max(val, (double)impl->minValue);

    return val;
}//sample

unsigned char SequencerEntry::sampleChar(int tick)
{
    double value = sample(tick);
    value -= impl->minValue;
    value /= (double)(impl->maxValue - impl->minValue);

    if (true == impl->sevenBit) {
        value *= 127.0 + 0.5;
    } else {        
        value *= 255.0 + 0.5;
    }//if

    return (unsigned char)value;
}//sampleChar

void SequencerEntry::clearRecordTokenBuffer()
{
    std::cout << "clearRecordTokenBuffer" << std::endl;
    recordTokenBuffer.clear();
}//clearRecordTokenBuffer

void SequencerEntry::addRecordToken(std::shared_ptr<MidiToken> token)
{
    if (impl->recordMode == false) {
        std::cout << "out 1" << std::endl;
        return;
    }//if

    if ((token->type == MidiTokenType::CC) && (impl->controllerType != ControlType::CC)) {
        std::cout << "out 2" << std::endl;
        return;
    }//if

    if ((impl->channel != 16) && (impl->channel != token->channel)) {
        std::cout << "out 3" << std::endl;
        return;
    }//if

    if ((token->type == MidiTokenType::CC) && (impl->msb != token->controller)) {
        std::cout << "out 4" << std::endl;
        return;
    }//if

    std::cout << "add token" << std::endl;
    recordTokenBuffer.push_back(token);
}//addRecordToken

std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > SequencerEntry::splitEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock, int tick)
{
    if ((tick <= entryBlock->getStartTick()) || (tick >= (entryBlock->getStartTick() + entryBlock->getDuration()))) {
        return std::make_pair(entryBlock, entryBlock);
    }//if

    std::shared_ptr<Animation> curve = entryBlock->getCurve();
    std::shared_ptr<Animation> secondaryCurve = entryBlock->getSecondaryCurve();

    std::shared_ptr<SequencerEntryBlock> firstBlock(new SequencerEntryBlock(shared_from_this(), entryBlock->getStartTick(), std::shared_ptr<SequencerEntryBlock>()));
    std::shared_ptr<Animation> newCurve = firstBlock->getCurve();
    std::shared_ptr<Animation> newSecondaryCurve = firstBlock->getSecondaryCurve();    

    int curveNumKeys = curve->getNumKeyframes();
    int index = 0;
    for (index = 0; index < curveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = curve->getKeyframe(index);
        if (curKey->tick + entryBlock->getStartTick() < tick) {
            newCurve->addKey(curKey->deepClone());
        } else {
            break;
        }//if
    }//for

    int secondaryCurveNumKeys = secondaryCurve->getNumKeyframes();
    int secondaryIndex = 0;
    for (secondaryIndex = 0; secondaryIndex < secondaryCurveNumKeys; ++secondaryIndex) {
        std::shared_ptr<Keyframe> curKey = secondaryCurve->getKeyframe(index);
        if (curKey->tick + entryBlock->getStartTick() < tick) {
            newSecondaryCurve->addKey(curKey->deepClone());
        } else {
            break;
        }//if
    }//for

    int secondStartTick = secondaryCurve->getKeyframe(index)->tick + entryBlock->getStartTick();
    std::shared_ptr<SequencerEntryBlock> secondBlock(new SequencerEntryBlock(shared_from_this(), secondStartTick, std::shared_ptr<SequencerEntryBlock>()));
    newCurve = secondBlock->getCurve();
    newSecondaryCurve = secondBlock->getSecondaryCurve();    

    for (/*nothing*/; index < curveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = curve->getKeyframe(index);
        std::shared_ptr<Keyframe> keyClone = curKey->deepClone();

        keyClone->tick -= secondStartTick;
        newCurve->addKey(keyClone);
    }//for

    for (/*nothing*/; secondaryIndex < secondaryCurveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = secondaryCurve->getKeyframe(index);
        std::shared_ptr<Keyframe> keyClone = curKey->deepClone();

        keyClone->tick -= secondStartTick;
        newSecondaryCurve->addKey(keyClone);
    }//for

    removeEntryBlock(entryBlock);
    addEntryBlock(firstBlock);
    addEntryBlock(secondBlock);

    return std::make_pair(firstBlock, secondBlock);
}//splitEntryBlock

Glib::ustring SequencerEntry::getTitle() const
{
    return impl->title;
}//getTitle

void SequencerEntry::setTitle(Glib::ustring title)
{
    if (title.empty() == false) {
        impl->title = title;
    }//if
}//setTitle


////////////////////////////////////////////////

void SequencerEntry::commitRecordedTokens()
{
    if (recordTokenBuffer.empty() == true) {
        std::cout << "commitRecodedTokens early exit" << std::endl;
        return;
    }//if

    std::cout << "commitRecodedTokens" << std::endl;

    static const int separationTickTime = 2000;

    int startTick = recordTokenBuffer[0]->curFrame;
    int lastTickTime = startTick;

    std::deque<std::shared_ptr<SequencerEntryBlock> > newEntryBlocks;

    std::shared_ptr<SequencerEntryBlock> entryBlock(new SequencerEntryBlock(shared_from_this(), startTick, std::shared_ptr<SequencerEntryBlock>()));
    addEntryBlock(entryBlock);
    newEntryBlocks.push_back(entryBlock);

    std::shared_ptr<Animation> animCurve = entryBlock->getCurve();

    for (std::shared_ptr<MidiToken> token : recordTokenBuffer) {
        std::shared_ptr<Keyframe> keyframe(new Keyframe);

        keyframe->tick = token->curFrame - startTick;
        keyframe->value = token->value;
        keyframe->curveType = CurveType::Step;

        if ((keyframe->tick - lastTickTime) > separationTickTime) {
            entryBlock.reset(new SequencerEntryBlock(shared_from_this(), keyframe->tick, std::shared_ptr<SequencerEntryBlock>()));
            addEntryBlock(entryBlock);
            animCurve = entryBlock->getCurve();
            newEntryBlocks.push_back(entryBlock);
        }//if

        animCurve->addKey(keyframe);
    }//foreach

    EntryBlockMergePolicy mergePolicy = EntryBlockMergePolicy::Merge;
    mergeEntryBlockLists(shared_from_this(), newEntryBlocks, mergePolicy);
}//commitRecordedTokens

void SequencerEntry::mergeEntryBlockLists(std::shared_ptr<SequencerEntry> entry, std::deque<std::shared_ptr<SequencerEntryBlock> > &newEntryBlocks, 
                                            EntryBlockMergePolicy mergePolicy)
{
    if (newEntryBlocks.empty() == true) {
        //Probably can't happen..
        return;
    }//if

    std::set<std::shared_ptr<SequencerEntryBlock> > newEntryBlocksSet;
    for (std::shared_ptr<SequencerEntryBlock> entryBlock : newEntryBlocks) {
        newEntryBlocksSet.insert(entryBlock);
    }//foreach

    std::map<int, std::shared_ptr<SequencerEntryBlock> >::const_iterator entryBlockIter;
    std::deque<std::shared_ptr<SequencerEntryBlock> > oldEntryBlocks;
    for (entryBlockIter = entryBlocks.begin(); entryBlockIter != entryBlocks.end(); ++entryBlockIter) {
        if (newEntryBlocksSet.find(entryBlockIter->second) == newEntryBlocksSet.end()) {
            oldEntryBlocks.push_back(entryBlockIter->second);
        }//if
    }//for

    while (oldEntryBlocks.empty() == false) {
        std::shared_ptr<SequencerEntryBlock> oldEntryBlock = oldEntryBlocks.front();
        std::shared_ptr<SequencerEntryBlock> newEntryBlock = newEntryBlocks.front();

        int oldEndTick = oldEntryBlock->getStartTick() + oldEntryBlock->getDuration();
        int newEndTick = newEntryBlock->getStartTick() + newEntryBlock->getDuration();

        //they don't overlap, old before new
        if ((oldEntryBlock->getStartTick() < newEntryBlock->getStartTick()) && (oldEndTick < newEntryBlock->getStartTick())) {
            oldEntryBlocks.pop_front();
            continue;
        }//if

        //they don't overlap, new before old
        if ((newEntryBlock->getStartTick() < oldEntryBlock->getStartTick()) && (newEndTick < oldEntryBlock->getStartTick())) {
            newEntryBlocks.pop_front();
            continue;
        }//if

        //otherwise new is appened to old or within old or vice-versa
        std::shared_ptr<SequencerEntryBlock> mergedBlock;
        mergedBlock = mergeEntryBlocks(oldEntryBlock, newEntryBlock, mergePolicy);

        oldEntryBlocks.pop_front();
        newEntryBlocks.pop_front();

        oldEntryBlocks.push_front(mergedBlock);
    }//while
}//void

std::shared_ptr<SequencerEntryBlock> SequencerEntry::mergeEntryBlocks(std::shared_ptr<SequencerEntryBlock> oldEntryBlock, std::shared_ptr<SequencerEntryBlock> newEntryBlock,
                                                                           EntryBlockMergePolicy mergePolicy)
{
    std::shared_ptr<Animation> oldCurve = oldEntryBlock->getCurve();
    std::shared_ptr<Animation> oldSecondaryCurve = oldEntryBlock->getSecondaryCurve();
    std::shared_ptr<Animation> newCurve = newEntryBlock->getCurve();
    std::shared_ptr<Animation> newSecondaryCurve = newEntryBlock->getSecondaryCurve();

    int newCurveStartTick = newEntryBlock->getStartTick();
    int newCurveEndTick = newCurveStartTick + newEntryBlock->getDuration();

    int startTick = std::min(oldEntryBlock->getStartTick(), newEntryBlock->getStartTick());

    std::shared_ptr<SequencerEntryBlock> merged(new SequencerEntryBlock(shared_from_this(), startTick, std::shared_ptr<SequencerEntryBlock>()));

    std::shared_ptr<Animation> mergedCurve = merged->getCurve();
    std::shared_ptr<Animation> mergedSecondaryCurve = merged->getSecondaryCurve();    

    int oldCurveNumKeys = oldCurve->getNumKeyframes();
    for (int index = 0; index < oldCurveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = oldCurve->getKeyframe(index);

        switch (mergePolicy) {
            case EntryBlockMergePolicy::Merge:
                mergedCurve->addKey(curKey->deepClone());
                break;

            case EntryBlockMergePolicy::Replace:
                {
                int absTick = curKey->tick + oldEntryBlock->getStartTick();
                if ((absTick < newCurveStartTick) || (absTick > newCurveEndTick)) {
                    mergedCurve->addKey(curKey->deepClone());
                }//if
                }
                break;

            case EntryBlockMergePolicy::Join:    //include first keyframes up to start of second keyframes
                {
                int absTick = curKey->tick + oldEntryBlock->getStartTick();
                if (absTick < newCurveStartTick) {
                    mergedCurve->addKey(curKey->deepClone());
                }//if
                }
                break;
        }//switch
    }//for

    int newCurveNumKeys = newCurve->getNumKeyframes();
    for (int index = 0; index < newCurveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = newCurve->getKeyframe(index);
        mergedCurve->addKey(curKey->deepClone());
    }//for

    int oldSecondaryCurveNumKeys = oldSecondaryCurve->getNumKeyframes();
    for (int index = 0; index < oldSecondaryCurveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = oldSecondaryCurve->getKeyframe(index);

        switch (mergePolicy) {
            case EntryBlockMergePolicy::Merge:
                mergedSecondaryCurve->addKey(curKey->deepClone());
                break;

            case EntryBlockMergePolicy::Replace:
                {
                int absTick = curKey->tick + oldEntryBlock->getStartTick();
                if ((absTick < newCurveStartTick) || (absTick > newCurveEndTick)) {
                    mergedSecondaryCurve->addKey(curKey->deepClone());
                }//if
                }
                break;

             case EntryBlockMergePolicy::Join:    //include first keyframes up to start of second keyframes
                {
                int absTick = curKey->tick + oldEntryBlock->getStartTick();
                if (absTick < newCurveStartTick) {
                    mergedSecondaryCurve->addKey(curKey->deepClone());
                }//if
                }
                break;
        }//switch
    }//for

    int newSecondaryCurveNumKeys = newSecondaryCurve->getNumKeyframes();
    for (int index = 0; index < newSecondaryCurveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = newSecondaryCurve->getKeyframe(index);
        mergedSecondaryCurve->addKey(curKey->deepClone());
    }//for

    removeEntryBlock(oldEntryBlock);
    removeEntryBlock(newEntryBlock);

    addEntryBlock(merged);

    return merged;
}//mergeEntryBlocks



template void SequencerEntry::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void SequencerEntryImpl::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void SequencerEntry::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
template void SequencerEntryImpl::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);

