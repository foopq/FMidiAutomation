/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __SEQUENCERENTRY_H
#define __SEQUENCERENTRY_H

#include <gtkmm.h>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <tuple>
#include <jack/jack.h>
#include "SequencerEntryBlock.h"
#include "fmaipair.h"

class Sequencer;
class SequencerEntryBlock;
struct MidiToken;
class EntryBlockSelectionState;

enum class ControlType : char
{
    CC,
    RPN,
};//ControlType

enum class EntryBlockMergePolicy : char
{
    Replace,
    Merge,  
    Join    //include first keyframes up to start of second keyframes
};//EntryBlockMergePolicy

struct SequencerEntryImpl
{
    SequencerEntryImpl();
    ~SequencerEntryImpl();

    std::shared_ptr<SequencerEntryImpl> clone();
    bool operator==(SequencerEntryImpl &other);

    ControlType controllerType;
    unsigned char msb;
    unsigned char lsb;
    unsigned char channel;

    //UI specific
    Glib::ustring title;    
    int minValue;
    int maxValue;
    bool sevenBit;
    bool useBothMSBandLSB; //implied true if sevenBit is true

    bool recordMode;
    bool soloMode;
    bool muteMode;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntryImpl

class SequencerEntry : public std::enable_shared_from_this<SequencerEntry>
{
    std::shared_ptr<SequencerEntryImpl> impl;
    std::map<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks;
    std::set<jack_port_t *> inputPorts;
    std::set<jack_port_t *> outputPorts;
    std::vector<std::shared_ptr<MidiToken> > recordTokenBuffer;

    void mergeEntryBlockLists(std::shared_ptr<SequencerEntry> entry, std::deque<std::shared_ptr<SequencerEntryBlock> > &newEntryBlocks, 
                              EntryBlockMergePolicy mergePolicy);

    std::shared_ptr<SequencerEntryBlock> mergeEntryBlocks(std::shared_ptr<SequencerEntryBlock> oldEntryBlock, std::shared_ptr<SequencerEntryBlock> newEntryBlock,
                                                             EntryBlockMergePolicy mergePolicy);

public:
    SequencerEntry();
    ~SequencerEntry();

    double sample(int tick);
    unsigned char sampleChar(int tick);

    void setRecordMode(bool mode);
    void setSoloMode(bool mode);
    void setMuteMode(bool mode);
    bool getRecordMode();
    bool getSoloMode();
    bool getMuteMode();

    const std::shared_ptr<SequencerEntryImpl> getImpl();
    std::shared_ptr<SequencerEntryImpl> getImplClone();
    void setNewDataImpl(std::shared_ptr<SequencerEntryImpl> impl);

    void addEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock);
    void removeEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock);
    std::shared_ptr<SequencerEntryBlock> getEntryBlock(int tick);
    fmaipair<decltype(entryBlocks.begin()), decltype(entryBlocks.end())> getEntryBlocksPair();
    std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > splitEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock, int tick);

    std::set<jack_port_t *> getInputPorts() const;
    std::set<jack_port_t *> getOutputPorts() const;
    void setInputPorts(std::set<jack_port_t *> ports);
    void setOutputPorts(std::set<jack_port_t *> ports);

    void clearRecordTokenBuffer();
    void addRecordToken(std::shared_ptr<MidiToken> token);
    void commitRecordedTokens();

    Glib::ustring getTitle() const;
    void setTitle(Glib::ustring);

    std::shared_ptr<SequencerEntry> deepClone();

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//SequencerEntry


BOOST_CLASS_VERSION(SequencerEntryImpl, 1);
BOOST_CLASS_VERSION(SequencerEntry, 1);


#endif


