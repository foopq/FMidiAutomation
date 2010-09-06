#ifndef __PROCESSRECORDEDMIDI_H
#define __PROCESSRECORDEDMIDI_H

#include <vector>

enum MidiTokenType
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

enum PortStreamTokenizerState
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
    MidiToken curToken;
    PortStreamTokenizerState state;
    std::vector<unsigned char> queuedData;
    unsigned int readHeadPosition;

    bool processNextToken();

public:
    PortStreamTokenizer();

    void addDataToStream(unsigned char *start, unsigned int length);
    bool isTokenAvailable();
    MidiToken getNextToken();
};//PortStreamTokenizer


#endif


