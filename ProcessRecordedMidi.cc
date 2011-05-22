#include "ProcessRecordedMidi.h"
#include "FMidiAutomationMainWindow.h"
#include "jack.h"
#include "Sequencer.h"
#include "Animation.h"
#include "Command.h"
#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <vector>
#include <deque>
#include <boost/foreach.hpp>

namespace
{



}//anonymous namespace


PortStreamTokenizer::PortStreamTokenizer()
{
    curToken.type = None;
    readHeadPosition = 0;
    state = Idle;
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
        case Idle:
            {
                if ((nextByte & 0xf0) == 0xf0) {
                    switch (nextByte) {
                        case 0xf0:
                            state = Sysex;
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
                            state = IgnoredTwoData;
                            return true;
                            break;

                        case 0xf3: //song select
                            state = IgnoredOneData;
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
                        state = IgnoredTwoData;
                        break;
                    case 0x0c: //prog change +1
                    case 0x0d: //aftertouch +1
                        state = IgnoredOneData;
                        break;

                    case 0x0b: //cc
                        curToken.channel = nextByte & 0x0f;
                        state = CC_Controller;
                        break;
                }//switch
            }//Idle
            break;

        case TokenWaiting:
            return false;
            break;

        case CC_Controller:
            curToken.controller = nextByte;
            state = CC_Value;
            break;

        case CC_Value:
            curToken.type = CC;
            curToken.value = nextByte;
            state = TokenWaiting;
            break;

        case Sysex:
            if (0xf7 == nextByte) {
                state = Idle;
            }//if
            break;

        case IgnoredOneData:
            state = Idle;
            break;

        case IgnoredTwoData:
            state = IgnoredOneData;
            break;

        case UnknownTokenizerState:
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

    if (state == TokenWaiting) {
        return true;
    } else {
        return false;
    }//if
}//isTokenAvailable

MidiToken PortStreamTokenizer::getNextToken()
{
    MidiToken retToken = curToken;
    curToken.type = None;
    state = Idle;

    return retToken;
}//getNextToken




void FMidiAutomationMainWindow::processRecordedMidi()
{
    std::cout << "UNDO STEP FOR PROCESSRECORDEDMIDI" << std::endl;

    JackSingleton &jackSingleton = JackSingleton::Instance();

    typedef std::pair<jack_port_t *, boost::shared_ptr<PortStreamTokenizer> > StreamTokenizersPair;
    std::map<jack_port_t *, boost::shared_ptr<PortStreamTokenizer> > streamTokenizers;

    typedef std::pair<jack_port_t *, boost::shared_ptr<SequencerEntry> > EntryMapPair;
    std::multimap<jack_port_t *, boost::shared_ptr<SequencerEntry> > entryMap;
    Globals &globals = Globals::Instance();

    typedef std::pair<boost::shared_ptr<SequencerEntry>, int> EntryPairType;
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

        boost::shared_ptr<PortStreamTokenizer> tokenizer(new PortStreamTokenizer);
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
        boost::shared_ptr<PortStreamTokenizer> tokenizer = streamTokenizers[header.port];
        tokenizer->addDataToStream(&recordBuffer[header.bufferPos], header.length);

        bool hadToken = false;
        do {
            hadToken = false;
            BOOST_FOREACH (StreamTokenizersPair curPair, streamTokenizers) {
                if (curPair.second->isTokenAvailable() == true) {
                    hadToken = true;
                    MidiToken token = curPair.second->getNextToken();
                    token.curFrame = header.curFrame;

                    BOOST_FOREACH (EntryMapPair entryMapPair, entryMap.equal_range(header.port)) {
                        entryMapPair.second->addRecordToken(token);
                    }//foreach
                }//if
            }//foreach
        } while (hadToken == true);
    }//foreach

    std::cout << "2" << std::endl;

/*    
    std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap = globals.sequencer->getEntryMap();
    gdk_threads_enter();
    globals.sequencer->cloneEntryMap();
    gdk_threads_leave();

    std::cout << "2.5" << std::endl;

    BOOST_FOREACH (EntryPairType entry, globals.sequencer->getEntryPair()) {
        entry.first->commitRecordedTokens();
    }//foreach

    std::cout << "3" << std::endl;

    std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap = globals.sequencer->getEntryMap();

    boost::shared_ptr<Command> processRecordedMidiCommand(new ProcessRecordedMidiCommand(origEntryMap, newEntryMap));
    CommandManager::Instance().setNewCommand(processRecordedMidiCommand, false);

    std::cout << "almost out processRecordMidi" << std::endl;
*/
    queuedUIThreadOperation = UIThreadOperation::finishProcessRecordedMidiOp;

    graphDrawingArea->queue_draw();

////!!!! Unset selected entry block
////!!!! Switch select selected entry
}//processRecordedMidi

void FMidiAutomationMainWindow::finishProcessRecordedMidi()
{
    std::cout << "finishProcessRecordedMidi" << std::endl;

    Globals &globals = Globals::Instance();
    typedef std::pair<boost::shared_ptr<SequencerEntry>, int> EntryPairType;

    std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap = globals.sequencer->getEntryMap();
    globals.sequencer->cloneEntryMap();

    std::cout << "2.5" << std::endl;

    BOOST_FOREACH (EntryPairType entry, globals.sequencer->getEntryPair()) {
        entry.first->commitRecordedTokens();
    }//foreach

    std::cout << "3" << std::endl;

    std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap = globals.sequencer->getEntryMap();

    boost::shared_ptr<Command> processRecordedMidiCommand(new ProcessRecordedMidiCommand(origEntryMap, newEntryMap));
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

    int startTick = recordTokenBuffer[0].curFrame;
    int lastTickTime = startTick;

    std::deque<boost::shared_ptr<SequencerEntryBlock> > newEntryBlocks;

    boost::shared_ptr<SequencerEntryBlock> entryBlock(new SequencerEntryBlock(shared_from_this(), startTick, boost::shared_ptr<SequencerEntryBlock>()));
    addEntryBlock(startTick, entryBlock);
    newEntryBlocks.push_back(entryBlock);

    boost::shared_ptr<Animation> animCurve = entryBlock->getCurve();

    BOOST_FOREACH (MidiToken &token, recordTokenBuffer) {
        boost::shared_ptr<Keyframe> keyframe(new Keyframe);

        keyframe->tick = token.curFrame - startTick;
        keyframe->value = token.value;
        keyframe->curveType = CurveType::Step;

        if ((keyframe->tick - lastTickTime) > separationTickTime) {
            entryBlock.reset(new SequencerEntryBlock(shared_from_this(), keyframe->tick, boost::shared_ptr<SequencerEntryBlock>()));
            addEntryBlock(keyframe->tick, entryBlock);
            animCurve = entryBlock->getCurve();
            newEntryBlocks.push_back(entryBlock);
        }//if

        animCurve->addKey(keyframe);
    }//foreach

    EntryBlockMergePolicy::EntryBlockMergePolicy mergePolicy = EntryBlockMergePolicy::Merge;
    mergeEntryBlockLists(shared_from_this(), newEntryBlocks, mergePolicy);
}//commitRecordedTokens

void SequencerEntry::mergeEntryBlockLists(boost::shared_ptr<SequencerEntry> entry, std::deque<boost::shared_ptr<SequencerEntryBlock> > &newEntryBlocks, 
                                            EntryBlockMergePolicy::EntryBlockMergePolicy mergePolicy)
{
    if (newEntryBlocks.empty() == true) {
        //Probably can't happen..
        return;
    }//if

    std::set<boost::shared_ptr<SequencerEntryBlock> > newEntryBlocksSet;
    BOOST_FOREACH (boost::shared_ptr<SequencerEntryBlock> entryBlock, newEntryBlocks) {
        newEntryBlocksSet.insert(entryBlock);
    }//foreach

    std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator entryBlockIter;
    std::deque<boost::shared_ptr<SequencerEntryBlock> > oldEntryBlocks;
    for (entryBlockIter = entryBlocks.begin(); entryBlockIter != entryBlocks.end(); ++entryBlockIter) {
        if (newEntryBlocksSet.find(entryBlockIter->second) == newEntryBlocksSet.end()) {
            oldEntryBlocks.push_back(entryBlockIter->second);
        }//if
    }//for

    while (oldEntryBlocks.empty() == false) {
        boost::shared_ptr<SequencerEntryBlock> oldEntryBlock = oldEntryBlocks.front();
        boost::shared_ptr<SequencerEntryBlock> newEntryBlock = newEntryBlocks.front();

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
        boost::shared_ptr<SequencerEntryBlock> mergedBlock;
        mergedBlock = mergeEntryBlocks(oldEntryBlock, newEntryBlock, mergePolicy);

        oldEntryBlocks.pop_front();
        newEntryBlocks.pop_front();

        oldEntryBlocks.push_front(mergedBlock);
    }//while
}//void

boost::shared_ptr<SequencerEntryBlock> SequencerEntry::mergeEntryBlocks(boost::shared_ptr<SequencerEntryBlock> oldEntryBlock, boost::shared_ptr<SequencerEntryBlock> newEntryBlock,
                                                                           EntryBlockMergePolicy::EntryBlockMergePolicy mergePolicy)
{
    boost::shared_ptr<Animation> oldCurve = oldEntryBlock->getCurve();
    boost::shared_ptr<Animation> oldSecondaryCurve = oldEntryBlock->getSecondaryCurve();
    boost::shared_ptr<Animation> newCurve = newEntryBlock->getCurve();
    boost::shared_ptr<Animation> newSecondaryCurve = newEntryBlock->getSecondaryCurve();

    int newCurveStartTick = newEntryBlock->getStartTick();
    int newCurveEndTick = newCurveStartTick + newEntryBlock->getDuration();

    int startTick = std::min(oldEntryBlock->getStartTick(), newEntryBlock->getStartTick());

    boost::shared_ptr<SequencerEntryBlock> merged(new SequencerEntryBlock(shared_from_this(), startTick, boost::shared_ptr<SequencerEntryBlock>()));

    boost::shared_ptr<Animation> mergedCurve = merged->getCurve();
    boost::shared_ptr<Animation> mergedSecondaryCurve = merged->getSecondaryCurve();    

    int oldCurveNumKeys = oldCurve->getNumKeyframes();
    for (int index = 0; index < oldCurveNumKeys; ++index) {
        boost::shared_ptr<Keyframe> curKey = oldCurve->getKeyframe(index);

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
        boost::shared_ptr<Keyframe> curKey = newCurve->getKeyframe(index);
        mergedCurve->addKey(curKey->deepClone());
    }//for

    int oldSecondaryCurveNumKeys = oldSecondaryCurve->getNumKeyframes();
    for (int index = 0; index < oldSecondaryCurveNumKeys; ++index) {
        boost::shared_ptr<Keyframe> curKey = oldSecondaryCurve->getKeyframe(index);

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
        boost::shared_ptr<Keyframe> curKey = newSecondaryCurve->getKeyframe(index);
        mergedSecondaryCurve->addKey(curKey->deepClone());
    }//for

    removeEntryBlock(oldEntryBlock);
    removeEntryBlock(newEntryBlock);

    addEntryBlock(startTick, merged);

    return merged;
}//mergeEntryBlocks

