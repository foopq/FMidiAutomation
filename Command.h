/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __COMMAND_H
#define __COMMAND_H

#include <memory>
#include <boost/function.hpp>
#include <stack>

struct Tempo;
struct FMidiAutomationData;
class Sequencer;
class SequencerEntry;
class SequencerEntryBlock;
struct SequencerEntryImpl;
struct Keyframe;
class CommandManager;

struct Command
{
    Command(Glib::ustring commandStr);
    virtual ~Command() {};

    Glib::ustring commandStr;

    virtual void doAction() = 0;
    virtual void undoAction() = 0;
};//Command

class CommandManager
{
    std::stack<std::shared_ptr<Command> > undoStack;
    std::stack<std::shared_ptr<Command> > redoStack;
    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    boost::function<void (void)> titleStarFunc;

public:
    static CommandManager &Instance();

    void setTitleStar(boost::function<void (void)> titleStarFunc);

    void setMenuItems(Gtk::ImageMenuItem *menuUndo, Gtk::ImageMenuItem *menuRedo);

    void doRedo();
    void doUndo();
    void setNewCommand(std::shared_ptr<Command> command, bool applyCommand);
};//CommandManager

struct ChangeSequencerEntryPropertiesCommand : public Command
{
    ChangeSequencerEntryPropertiesCommand(std::shared_ptr<SequencerEntry> entry, std::shared_ptr<SequencerEntryImpl> origImpl, std::shared_ptr<SequencerEntryImpl> newImpl);
    virtual ~ChangeSequencerEntryPropertiesCommand();

    void doAction();
    void undoAction();

private:    
    std::shared_ptr<SequencerEntry> entry;
    std::shared_ptr<SequencerEntryImpl> origImpl;
    std::shared_ptr<SequencerEntryImpl> newImpl;
};//ChangeSequencerEntryPropertiesCommand

struct MoveSequencerEntryBlockCommand : public Command
{
    MoveSequencerEntryBlockCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks,
                                    std::map<std::shared_ptr<SequencerEntryBlock>, int> entryOriginalStartTicks, //FIXME: This should be a reference
                                    std::map<std::shared_ptr<SequencerEntryBlock>, int> entryNewStartTicks);

    virtual ~MoveSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    std::map<std::shared_ptr<SequencerEntryBlock>, int> entryOriginalStartTicks;
    std::map<std::shared_ptr<SequencerEntryBlock>, int> entryNewStartTicks;
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks;
};//MoveSequencerEntryBlockCommand

struct ChangeSequencerEntryBlockPropertiesCommand : public Command
{
    ChangeSequencerEntryBlockPropertiesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock, Glib::ustring newTitle);
    virtual ~ChangeSequencerEntryBlockPropertiesCommand();

    void doAction();
    void undoAction();

private:    
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    Glib::ustring prevTitle;
};//ChangeSequencerEntryBlockPropertiesCommand

struct AddSequencerEntryBlockCommand : public Command
{
    AddSequencerEntryBlockCommand(std::shared_ptr<SequencerEntry> entry, std::shared_ptr<SequencerEntryBlock> entryBlock);
    virtual ~AddSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntry> entry;
    std::shared_ptr<SequencerEntryBlock> entryBlock;
};//AddSequencerEntryBlockCommand

struct AddSequencerEntryBlocksCommand : public Command
{
    AddSequencerEntryBlocksCommand(std::vector<std::pair<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntryBlock>>> &entryBlocks_);
    virtual ~AddSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:
    std::vector<std::pair<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntryBlock>>> entryBlocks;
};//AddSequencerEntryBlocksCommand

struct DeleteSequencerEntryBlocksCommand : public Command
{
    //FIXME: This shouldn't be a copy, but a reference!
    DeleteSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks);
    virtual ~DeleteSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private: 
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks;
};//DeleteSequencerEntryBlocksCommand

struct DeleteSequencerEntryBlockCommand : public Command
{
    DeleteSequencerEntryBlockCommand(std::shared_ptr<SequencerEntryBlock> entryBlock);
    virtual ~DeleteSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntry> entry;
    std::shared_ptr<SequencerEntryBlock> entryBlock;
};//DeleteSequencerEntryBlockCommand

struct SequencerEntryUpCommand : public Command
{
    SequencerEntryUpCommand(std::shared_ptr<Sequencer> sequencer, std::shared_ptr<SequencerEntry> entry);
    virtual ~SequencerEntryUpCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Sequencer> sequencer;
    std::shared_ptr<SequencerEntry> entry;
    unsigned int origIndex;
};//SequencerEntryUpCommand

struct SequencerEntryDownCommand : public Command
{
    SequencerEntryDownCommand(std::shared_ptr<Sequencer> sequencer, std::shared_ptr<SequencerEntry> entry);
    virtual ~SequencerEntryDownCommand();

    void doAction();
    void undoAction();

private: 
    std::shared_ptr<Sequencer> sequencer;
    std::shared_ptr<SequencerEntry> entry;
    unsigned int origIndex;
};//SequencerEntryDownCommand

struct AddSequencerEntryCommand : public Command
{
    AddSequencerEntryCommand(std::shared_ptr<Sequencer> sequencer, bool useDefaults);
    virtual ~AddSequencerEntryCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Sequencer> sequencer;
    std::shared_ptr<SequencerEntry> entry;
    bool useDefaults;
};//AddSequencerEntryCommand

struct DeleteSequencerEntryCommand : public Command
{
    DeleteSequencerEntryCommand(std::shared_ptr<Sequencer> sequencer, std::shared_ptr<SequencerEntry> entry);
    virtual ~DeleteSequencerEntryCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Sequencer> sequencer;
    std::shared_ptr<SequencerEntry> entry;
    unsigned int entryIndex;
};//DeleteSequencerEntryCommand

struct AddTempoChangeCommand : public Command
{
    AddTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int tick_, std::shared_ptr<FMidiAutomationData> datas_,
                            boost::function<void (void)> updateTempoChangesUIData);
    virtual ~AddTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<FMidiAutomationData> datas;
    std::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;
};//AddTempoChangeCommand

struct DeleteTempoChangeCommand : public Command
{
    DeleteTempoChangeCommand(unsigned int tick_, std::shared_ptr<FMidiAutomationData> datas_, boost::function<void (void)> updateTempoChangesUIData);
    virtual ~DeleteTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<FMidiAutomationData> datas;
    std::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;
};//DeleteTempoChangeCommand

struct UpdateTempoChangeCommand : public Command
{
    UpdateTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData);
    virtual ~UpdateTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    boost::function<void (void)> updateTempoChangesUIData;
    
    unsigned int old_bpm; //times 100
    unsigned int old_beatsPerBar;
    unsigned int old_barSubDivisions;   
};//AddTempoChangeCommand

struct AddKeyframesCommand : public Command
{
    AddKeyframesCommand(std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, int curMouseUnderTick, int curMouseUnderValue);
    AddKeyframesCommand(std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, std::map<int, std::shared_ptr<Keyframe> > &origKeyframes, int newTick);
    virtual ~AddKeyframesCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock;
    std::map<int, std::shared_ptr<Keyframe> > keyframes;
};//AddKeyframesCommand

struct DeleteKeyframesCommand : public Command
{
    DeleteKeyframesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock, std::map<int, std::shared_ptr<Keyframe> > keyframes);
    virtual ~DeleteKeyframesCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    std::map<int, std::shared_ptr<Keyframe> > keyframes;
};//DeleteKeyframesCommand

struct MoveKeyframesCommand : public Command
{
    struct KeyInfo
    {
        std::shared_ptr<Keyframe> keyframe;
        int movingKeyOrigTick;
        double movingKeyOrigValue;
    };//KeyInfo

    MoveKeyframesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock, std::vector<std::shared_ptr<KeyInfo> > &keyframes);
    virtual ~MoveKeyframesCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    std::vector<std::shared_ptr<KeyInfo> > keyframes;
};//MoveKeyframesCommand

struct ProcessRecordedMidiCommand : public Command
{
    ProcessRecordedMidiCommand(std::map<std::shared_ptr<SequencerEntry>, int > origEntryMap, std::map<std::shared_ptr<SequencerEntry>, int > newEntryMap);
    virtual ~ProcessRecordedMidiCommand();

    void doAction();
    void undoAction();

private:
    std::map<std::shared_ptr<SequencerEntry>, int > origEntryMap;
    std::map<std::shared_ptr<SequencerEntry>, int > newEntryMap;
};//ProcessRecordedMidiCommand

struct MergeSequencerEntryBlocksCommand : public Command
{
    MergeSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlock> > &origEntryBlocks,
                                        std::multimap<int, std::shared_ptr<SequencerEntryBlock> > &replacementEntryBlocks);
    virtual ~MergeSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:    
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > origEntryBlocks;
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > replacementEntryBlocks;
};//MergeSequencerEntryBlocksCommand

struct SplitSequencerEntryBlocksCommand : public Command
{
    SplitSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlock> > &origEntryBlocks,
                                        std::multimap<int, std::shared_ptr<SequencerEntryBlock> > &replacementEntryBlocks);
    virtual ~SplitSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:    
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > origEntryBlocks;
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > replacementEntryBlocks;
};//SplitSequencerEntryBlocksCommand

#endif
