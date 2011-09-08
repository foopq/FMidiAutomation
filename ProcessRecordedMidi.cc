/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "ProcessRecordedMidi.h"
#include "jack.h"
#include "Sequencer.h"
#include "SequencerEntry.h"
#include "Animation.h"
#include "Command.h"
#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <vector>
#include <deque>
#include <boost/foreach.hpp>
#include "Globals.h"
#include "FMidiAutomationMainWindow.h"

namespace
{



}//anonymous namespace


PortStreamTokenizer::PortStreamTokenizer()
{
    curToken = std::make_shared<MidiToken>();
    curToken->type = MidiTokenType::None;
    readHeadPosition = 0;
    state = PortStreamTokenizerState::Idle;
}//constructor

void PortStreamTokenizer::addDataToStream(unsigned char *start, unsigned int length)
{
    queuedData.insert(queuedData.end(), start, start+length);
}//addDataToStream

bool PortStreamTokenizer::processNextToken()
{
    if (readHeadPosition >= queuedData.size()) {
        readHeadPosition = 0;
        queuedData.clear();
        return false;
    }//if

    unsigned char nextByte = queuedData[readHeadPosition];
    readHeadPosition++;

    switch (state) {
        case PortStreamTokenizerState::Idle:
            {
                if ((nextByte & 0xf0) == 0xf0) {
                    switch (nextByte) {
                        case 0xf0:
                            state = PortStreamTokenizerState::Sysex;
                            return true;
                            break;

                        case 0xf1: //undefined
                        case 0xf4: //undefined
                        case 0xf5: //undefined
                        case 0xf6: //tune request
                        case 0xf7: //end of sysex -- this is bad if we got this here
                        case 0xf8: //clock
                        case 0xf9: //undefined
                        case 0xfa: //start
                        case 0xfb: //continue
                        case 0xfc: //stop
                        case 0xfd: //undefined
                        case 0xfe: //active sensing
                        case 0xff: //reset
                            return true;
                            break;

                        case 0xf2: //song position pointer
                            state = PortStreamTokenizerState::IgnoredTwoData;
                            return true;
                            break;

                        case 0xf3: //song select
                            state = PortStreamTokenizerState::IgnoredOneData;
                            return true;
                            break;
                    }//nextByte
                }//if

                unsigned char command = nextByte >> 4;
                switch (command) {
                    case 0x08: //note off +2
                    case 0x09: //note on +2
                    case 0x0a: //aftertouch +2
                    case 0x0e: //pitch wheel +2
                        state = PortStreamTokenizerState::IgnoredTwoData;
                        break;
                    case 0x0c: //prog change +1
                    case 0x0d: //aftertouch +1
                        state = PortStreamTokenizerState::IgnoredOneData;
                        break;

                    case 0x0b: //cc
                        curToken->channel = nextByte & 0x0f;
                        state = PortStreamTokenizerState::CC_Controller;
                        break;
                }//switch
            }//Idle
            break;

        case PortStreamTokenizerState::TokenWaiting:
            return false;
            break;

        case PortStreamTokenizerState::CC_Controller:
            curToken->controller = nextByte;
            state = PortStreamTokenizerState::CC_Value;
            break;

        case PortStreamTokenizerState::CC_Value:
            curToken->type = MidiTokenType::CC;
            curToken->value = nextByte;
            state = PortStreamTokenizerState::TokenWaiting;
            break;

        case PortStreamTokenizerState::Sysex:
            if (0xf7 == nextByte) {
                state = PortStreamTokenizerState::Idle;
            }//if
            break;

        case PortStreamTokenizerState::IgnoredOneData:
            state = PortStreamTokenizerState::Idle;
            break;

        case PortStreamTokenizerState::IgnoredTwoData:
            state = PortStreamTokenizerState::IgnoredOneData;
            break;

        case PortStreamTokenizerState::UnknownTokenizerState:
            std::cerr << "Invalid tokenizer state!" << std::endl;
            break;
    }//switch

    return true;
}//processNextToken

bool PortStreamTokenizer::isTokenAvailable()
{
    while (processNextToken() == true) {
        //Nothing
    }//while

    if (state == PortStreamTokenizerState::TokenWaiting) {
        return true;
    } else {
        return false;
    }//if
}//isTokenAvailable

std::shared_ptr<MidiToken> PortStreamTokenizer::getNextToken()
{
    std::shared_ptr<MidiToken> retToken = curToken;
    curToken->type = MidiTokenType::None;
    state = PortStreamTokenizerState::Idle;

    return retToken;
}//getNextToken

void FMidiAutomationMainWindow::processRecordedMidi()
{
    std::cout << "UNDO STEP FOR PROCESSRECORDEDMIDI" << std::endl;

    JackSingleton &jackSingleton = JackSingleton::Instance();

    typedef std::pair<jack_port_t *, std::shared_ptr<PortStreamTokenizer> > StreamTokenizersPair;
    std::map<jack_port_t *, std::shared_ptr<PortStreamTokenizer> > streamTokenizers;

    typedef std::pair<jack_port_t *, std::shared_ptr<SequencerEntry> > EntryMapPair;
    std::multimap<jack_port_t *, std::shared_ptr<SequencerEntry> > entryMap;
    Globals &globals = Globals::Instance();

    typedef std::pair<std::shared_ptr<SequencerEntry>, int> EntryPairType;
    BOOST_FOREACH (EntryPairType entry, globals.sequencer->getEntryPair()) {
        std::set<jack_port_t *> ports = entry.first->getInputPorts();
        BOOST_FOREACH (jack_port_t *port, ports) {
            entryMap.insert(std::make_pair(port, entry.first));
        }//foreach

        entry.first->clearRecordTokenBuffer();
    }//foreach

    std::vector<std::string> inputPorts = jackSingleton.getInputPorts();

    BOOST_FOREACH (std::string inputPort, inputPorts) {
        jack_port_t *jackPort = jackSingleton.getInputPort(inputPort);

        std::shared_ptr<PortStreamTokenizer> tokenizer(new PortStreamTokenizer);
        streamTokenizers[jackPort] = tokenizer;
    }//foreach

    std::vector<unsigned char> &recordBuffer = jackSingleton.getRecordBuffer();
    std::vector<MidiInputInfoHeader> &bufferHeaders = jackSingleton.getMidiRecordBufferHeaders();

    if (recordBuffer.empty() == true) {
        std::cout << "Nothing recorded" << std::endl;
        return;
    }//if

    std::cout << "recordBuffer: " << recordBuffer.size() << std::endl;
    std::cout << "recordBufferHeaders: " << bufferHeaders.size() << std::endl;

    BOOST_FOREACH (MidiInputInfoHeader header, bufferHeaders) {
        std::shared_ptr<PortStreamTokenizer> tokenizer = streamTokenizers[header.port];
        tokenizer->addDataToStream(&recordBuffer[header.bufferPos], header.length);

        bool hadToken = false;
        do {
            hadToken = false;
            BOOST_FOREACH (StreamTokenizersPair curPair, streamTokenizers) {
                if (curPair.second->isTokenAvailable() == true) {
                    hadToken = true;
                    std::shared_ptr<MidiToken> token = curPair.second->getNextToken();
                    token->curFrame = header.curFrame;

                    BOOST_FOREACH (EntryMapPair entryMapPair, entryMap.equal_range(header.port)) {
                        entryMapPair.second->addRecordToken(token);
                    }//foreach
                }//if
            }//foreach
        } while (hadToken == true);
    }//foreach

    std::cout << "2" << std::endl;

/*    
    std::map<std::shared_ptr<SequencerEntry>, int > origEntryMap = globals.sequencer->getEntryMap();
    gdk_threads_enter();
    globals.sequencer->cloneEntryMap();
    gdk_threads_leave();

    std::cout << "2.5" << std::endl;

    BOOST_FOREACH (EntryPairType entry, globals.sequencer->getEntryPair()) {
        entry.first->commitRecordedTokens();
    }//foreach

    std::cout << "3" << std::endl;

    std::map<std::shared_ptr<SequencerEntry>, int > newEntryMap = globals.sequencer->getEntryMap();

    std::shared_ptr<Command> processRecordedMidiCommand(new ProcessRecordedMidiCommand(origEntryMap, newEntryMap));
    CommandManager::Instance().setNewCommand(processRecordedMidiCommand, false);

    std::cout << "almost out processRecordMidi" << std::endl;
*/
    queuedUIThreadOperation = UIThreadOperation::finishProcessRecordedMidiOp;

    queue_draw();

////!!!! Unset selected entry block
////!!!! Switch select selected entry
}//processRecordedMidi

void FMidiAutomationMainWindow::finishProcessRecordedMidi()
{
    std::cout << "finishProcessRecordedMidi" << std::endl;

    Globals &globals = Globals::Instance();
    typedef std::pair<std::shared_ptr<SequencerEntry>, int> EntryPairType;

    std::map<std::shared_ptr<SequencerEntry>, int > origEntryMap = globals.sequencer->getEntryMap();
    globals.sequencer->cloneEntryMap();

    std::cout << "2.5" << std::endl;

    BOOST_FOREACH (EntryPairType entry, globals.sequencer->getEntryPair()) {
        entry.first->commitRecordedTokens();
    }//foreach

    std::cout << "3" << std::endl;

    std::map<std::shared_ptr<SequencerEntry>, int > newEntryMap = globals.sequencer->getEntryMap();

    std::shared_ptr<Command> processRecordedMidiCommand(new ProcessRecordedMidiCommand(origEntryMap, newEntryMap));
    CommandManager::Instance().setNewCommand(processRecordedMidiCommand, false);

    std::cout << "almost out processRecordMidi" << std::endl;
}//finishProcessRecordedMidi

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
    addEntryBlock(startTick, entryBlock);
    newEntryBlocks.push_back(entryBlock);

    std::shared_ptr<Animation> animCurve = entryBlock->getCurve();

    BOOST_FOREACH (std::shared_ptr<MidiToken> token, recordTokenBuffer) {
        std::shared_ptr<Keyframe> keyframe(new Keyframe);

        keyframe->tick = token->curFrame - startTick;
        keyframe->value = token->value;
        keyframe->curveType = CurveType::Step;

        if ((keyframe->tick - lastTickTime) > separationTickTime) {
            entryBlock.reset(new SequencerEntryBlock(shared_from_this(), keyframe->tick, std::shared_ptr<SequencerEntryBlock>()));
            addEntryBlock(keyframe->tick, entryBlock);
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
    BOOST_FOREACH (std::shared_ptr<SequencerEntryBlock> entryBlock, newEntryBlocks) {
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

    addEntryBlock(startTick, merged);

    return merged;
}//mergeEntryBlocks

