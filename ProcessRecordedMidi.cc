/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "ProcessRecordedMidi.h"
#include "jack.h"
#include "Data/Sequencer.h"
#include "Data/SequencerEntry.h"
#include "UI/SequencerUI.h"
#include "Animation.h"
#include "Command_Other.h"
#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <vector>
#include <deque>
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

    for (auto entry : globals.projectData.getSequencer()->getEntryPair()) {
        std::set<jack_port_t *> ports = entry->getInputPorts();
        for (jack_port_t *port : ports) {
            entryMap.insert(std::make_pair(port, entry));
        }//foreach

        entry->clearRecordTokenBuffer();
    }//foreach

    std::vector<std::string> inputPorts = jackSingleton.getInputPorts();

    for (std::string inputPort : inputPorts) {
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

    for (MidiInputInfoHeader header : bufferHeaders) {
        std::shared_ptr<PortStreamTokenizer> tokenizer = streamTokenizers[header.port];
        tokenizer->addDataToStream(&recordBuffer[header.bufferPos], header.length);

        bool hadToken = false;
        do {
            hadToken = false;
            for (StreamTokenizersPair curPair : streamTokenizers) {
                if (curPair.second->isTokenAvailable() == true) {
                    hadToken = true;
                    std::shared_ptr<MidiToken> token = curPair.second->getNextToken();
                    token->curFrame = header.curFrame;

                    for (EntryMapPair entryMapPair : fmaipair<decltype(entryMap.begin()), decltype(entryMap.end())>(entryMap.equal_range(header.port))) {
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

    for (EntryPairType entry : globals.sequencer->getEntryPair()) {
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

    auto origEntryMap = globals.projectData.getSequencer()->getEntryMap();
    std::map<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntry>> oldNewEntryMap = globals.projectData.getSequencer()->cloneEntryMap();

    std::cout << "2.5" << std::endl;

    for (auto entry : globals.projectData.getSequencer()->getEntryPair()) {
        entry->commitRecordedTokens();
    }//foreach

    std::cout << "3" << std::endl;

    sequencer->cloneEntryMap(oldNewEntryMap);

    auto newEntryMap = globals.projectData.getSequencer()->getEntryMap();

    std::shared_ptr<Command> processRecordedMidiCommand(new ProcessRecordedMidiCommand(origEntryMap, newEntryMap, this));
    CommandManager::Instance().setNewCommand(processRecordedMidiCommand, false);

    std::cout << "almost out processRecordMidi" << std::endl;
}//finishProcessRecordedMidi


