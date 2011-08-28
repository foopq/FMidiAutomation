/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __COMMAND_H
#define __COMMAND_H

#include <boost/shared_ptr.hpp>
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

    Glib::ustring commandStr;

    virtual void doAction() = 0;
    virtual void undoAction() = 0;
};//Command

class CommandManager
{
    std::stack<boost::shared_ptr<Command> > undoStack;
    std::stack<boost::shared_ptr<Command> > redoStack;
    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    boost::function<void (void)> titleStarFunc;

public:
    static CommandManager &Instance();

    void setTitleStar(boost::function<void (void)> titleStarFunc);

    void setMenuItems(Gtk::ImageMenuItem *menuUndo, Gtk::ImageMenuItem *menuRedo);

    void doRedo();
    void doUndo();
    void setNewCommand(boost::shared_ptr<Command> command, bool applyCommand);
};//CommandManager

struct ChangeSequencerEntryPropertiesCommand : public Command
{
    ChangeSequencerEntryPropertiesCommand(boost::shared_ptr<SequencerEntry> entry, boost::shared_ptr<SequencerEntryImpl> origImpl, boost::shared_ptr<SequencerEntryImpl> newImpl);
    ~ChangeSequencerEntryPropertiesCommand();

    void doAction();
    void undoAction();

private:    
    boost::shared_ptr<SequencerEntry> entry;
    boost::shared_ptr<SequencerEntryImpl> origImpl;
    boost::shared_ptr<SequencerEntryImpl> newImpl;
};//ChangeSequencerEntryPropertiesCommand

struct MoveSequencerEntryBlockCommand : public Command
{
    MoveSequencerEntryBlockCommand(std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks,
                                    std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryOriginalStartTicks, //FIXME: This should be a reference
                                    std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryNewStartTicks);

    ~MoveSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryOriginalStartTicks;
    std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryNewStartTicks;
    std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks;
};//MoveSequencerEntryBlockCommand

struct ChangeSequencerEntryBlockPropertiesCommand : public Command
{
    ChangeSequencerEntryBlockPropertiesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock, Glib::ustring newTitle);
    ~ChangeSequencerEntryBlockPropertiesCommand();

    void doAction();
    void undoAction();

private:    
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    Glib::ustring prevTitle;
};//ChangeSequencerEntryBlockPropertiesCommand

struct AddSequencerEntryBlockCommand : public Command
{
    AddSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntry> entry, boost::shared_ptr<SequencerEntryBlock> entryBlock);
    ~AddSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<SequencerEntry> entry;
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
};//AddSequencerEntryBlockCommand

struct AddSequencerEntryBlocksCommand : public Command
{
    AddSequencerEntryBlocksCommand(std::vector<std::pair<boost::shared_ptr<SequencerEntry>, boost::shared_ptr<SequencerEntryBlock>>> &entryBlocks_);
    ~AddSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:
    std::vector<std::pair<boost::shared_ptr<SequencerEntry>, boost::shared_ptr<SequencerEntryBlock>>> entryBlocks;
};//AddSequencerEntryBlocksCommand

struct DeleteSequencerEntryBlocksCommand : public Command
{
    //FIXME: This shouldn't be a copy, but a reference!
    DeleteSequencerEntryBlocksCommand(std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks);
    ~DeleteSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private: 
    std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks;
};//DeleteSequencerEntryBlocksCommand

struct DeleteSequencerEntryBlockCommand : public Command
{
    DeleteSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock);
    ~DeleteSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<SequencerEntry> entry;
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
};//DeleteSequencerEntryBlockCommand

struct SequencerEntryUpCommand : public Command
{
    SequencerEntryUpCommand(boost::shared_ptr<Sequencer> sequencer, boost::shared_ptr<SequencerEntry> entry);
    ~SequencerEntryUpCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    unsigned int origIndex;
};//SequencerEntryUpCommand

struct SequencerEntryDownCommand : public Command
{
    SequencerEntryDownCommand(boost::shared_ptr<Sequencer> sequencer, boost::shared_ptr<SequencerEntry> entry);
    ~SequencerEntryDownCommand();

    void doAction();
    void undoAction();

private: 
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    unsigned int origIndex;
};//SequencerEntryDownCommand

struct AddSequencerEntryCommand : public Command
{
    AddSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer, bool useDefaults);
    ~AddSequencerEntryCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    bool useDefaults;
};//AddSequencerEntryCommand

struct DeleteSequencerEntryCommand : public Command
{
    DeleteSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer, boost::shared_ptr<SequencerEntry> entry);
    ~DeleteSequencerEntryCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    unsigned int entryIndex;
};//DeleteSequencerEntryCommand

struct AddTempoChangeCommand : public Command
{
    AddTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int tick_, boost::shared_ptr<FMidiAutomationData> datas_,
                            boost::function<void (void)> updateTempoChangesUIData);
    ~AddTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<FMidiAutomationData> datas;
    boost::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;
};//AddTempoChangeCommand

struct DeleteTempoChangeCommand : public Command
{
    DeleteTempoChangeCommand(unsigned int tick_, boost::shared_ptr<FMidiAutomationData> datas_, boost::function<void (void)> updateTempoChangesUIData);
    ~DeleteTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<FMidiAutomationData> datas;
    boost::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;
};//DeleteTempoChangeCommand

struct UpdateTempoChangeCommand : public Command
{
    UpdateTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData);
    ~UpdateTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<Tempo> tempo;
    boost::function<void (void)> updateTempoChangesUIData;
    
    unsigned int old_bpm; //times 100
    unsigned int old_beatsPerBar;
    unsigned int old_barSubDivisions;   
};//AddTempoChangeCommand

struct AddKeyframesCommand : public Command
{
    AddKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, int curMouseUnderTick, int curMouseUnderValue);
    AddKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, std::map<int, boost::shared_ptr<Keyframe> > &origKeyframes, int newTick);
    ~AddKeyframesCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock;
    std::map<int, boost::shared_ptr<Keyframe> > keyframes;
};//AddKeyframesCommand

struct DeleteKeyframesCommand : public Command
{
    DeleteKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock, std::map<int, boost::shared_ptr<Keyframe> > keyframes);
    ~DeleteKeyframesCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    std::map<int, boost::shared_ptr<Keyframe> > keyframes;
};//DeleteKeyframesCommand

struct MoveKeyframesCommand : public Command
{
    struct KeyInfo
    {
        boost::shared_ptr<Keyframe> keyframe;
        int movingKeyOrigTick;
        double movingKeyOrigValue;
    };//KeyInfo

    MoveKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock, std::vector<boost::shared_ptr<KeyInfo> > &keyframes);
    ~MoveKeyframesCommand();

    void doAction();
    void undoAction();

private:
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    std::vector<boost::shared_ptr<KeyInfo> > keyframes;
};//MoveKeyframesCommand

struct ProcessRecordedMidiCommand : public Command
{
    ProcessRecordedMidiCommand(std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap, std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap);
    ~ProcessRecordedMidiCommand();

    void doAction();
    void undoAction();

private:
    std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap;
    std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap;
};//ProcessRecordedMidiCommand


#endif
