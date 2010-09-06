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
#include <boost/foreach.hpp>


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

    int startTick = recordTokenBuffer[0].curFrame;

    boost::shared_ptr<SequencerEntryBlock> entryBlock(new SequencerEntryBlock(shared_from_this(), startTick, boost::shared_ptr<SequencerEntryBlock>()));
    addEntryBlock(startTick, entryBlock);

    boost::shared_ptr<Animation> animCurve = entryBlock->getCurve();

    BOOST_FOREACH (MidiToken &token, recordTokenBuffer) {
        boost::shared_ptr<Keyframe> keyframe(new Keyframe);

        keyframe->tick = token.curFrame - startTick;
        keyframe->value = token.value;
        keyframe->curveType = CurveType::Step;

        animCurve->addKey(keyframe);
    }//foreach
}//commitRecordedTokens



