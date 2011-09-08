/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __PROCESSRECORDEDMIDI_H
#define __PROCESSRECORDEDMIDI_H

#include <vector>
#include <memory>

enum class MidiTokenType : char
{
    None,
    CC, 
    UnknownToken
};//MidiTokenType

struct MidiToken
{
    unsigned int curFrame;
    MidiTokenType type;
    unsigned int channel;
    unsigned char controller;
    unsigned char value;
    //unsigned char data2;
    //std::vector<unsigned char> sysexData;
};//MidiToken

enum class PortStreamTokenizerState : char
{
    Idle,
    TokenWaiting,
    CC_Controller,
    CC_Value,
    Sysex,
    IgnoredOneData,
    IgnoredTwoData,

    UnknownTokenizerState
};//PortStreamTokenizerState

class PortStreamTokenizer
{
    std::shared_ptr<MidiToken> curToken;
    PortStreamTokenizerState state;
    std::vector<unsigned char> queuedData;
    unsigned int readHeadPosition;

    bool processNextToken();

public:
    PortStreamTokenizer();

    void addDataToStream(unsigned char *start, unsigned int length);
    bool isTokenAvailable();
    std::shared_ptr<MidiToken> getNextToken();
};//PortStreamTokenizer


#endif


