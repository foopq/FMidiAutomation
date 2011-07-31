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
    boost::shared_ptr<SequencerEntry> entry;
    boost::shared_ptr<SequencerEntryImpl> origImpl;
    boost::shared_ptr<SequencerEntryImpl> newImpl;

    ChangeSequencerEntryPropertiesCommand(boost::shared_ptr<SequencerEntry> entry, boost::shared_ptr<SequencerEntryImpl> origImpl, boost::shared_ptr<SequencerEntryImpl> newImpl);
    ~ChangeSequencerEntryPropertiesCommand();

    void doAction();
    void undoAction();
};//ChangeSequencerEntryPropertiesCommand

struct MoveSequencerEntryBlockCommand : public Command
{
    std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryOriginalStartTicks;
    std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryNewStartTicks;
    std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks;

    MoveSequencerEntryBlockCommand(std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > &entryBlocks,
                                    std::map<boost::shared_ptr<SequencerEntryBlock>, int> &entryOriginalStartTicks,
                                    std::map<boost::shared_ptr<SequencerEntryBlock>, int> &entryNewStartTicks);

    ~MoveSequencerEntryBlockCommand();

    void doAction();
    void undoAction();
};//MoveSequencerEntryBlockCommand

struct ChangeSequencerEntryBlockPropertiesCommand : public Command
{
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    Glib::ustring prevTitle;

    ChangeSequencerEntryBlockPropertiesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock, Glib::ustring newTitle);
    ~ChangeSequencerEntryBlockPropertiesCommand();

    void doAction();
    void undoAction();
};//ChangeSequencerEntryBlockPropertiesCommand

struct AddSequencerEntryBlockCommand : public Command
{
    boost::shared_ptr<SequencerEntry> entry;
    boost::shared_ptr<SequencerEntryBlock> entryBlock;

    AddSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntry> entry, boost::shared_ptr<SequencerEntryBlock> entryBlock);
    ~AddSequencerEntryBlockCommand();

    void doAction();
    void undoAction();
};//AddSequencerEntryBlockCommand

struct DeleteSequencerEntryBlocksCommand : public Command
{
    std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks;

    DeleteSequencerEntryBlocksCommand(std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > &entryBlocks);
    ~DeleteSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();
};//DeleteSequencerEntryBlocksCommand

struct DeleteSequencerEntryBlockCommand : public Command
{
    boost::shared_ptr<SequencerEntry> entry;
    boost::shared_ptr<SequencerEntryBlock> entryBlock;

    DeleteSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock);
    ~DeleteSequencerEntryBlockCommand();

    void doAction();
    void undoAction();
};//DeleteSequencerEntryBlockCommand

struct SequencerEntryUpCommand : public Command
{
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    unsigned int origIndex;

    SequencerEntryUpCommand(boost::shared_ptr<Sequencer> sequencer, boost::shared_ptr<SequencerEntry> entry);
    ~SequencerEntryUpCommand();

    void doAction();
    void undoAction();
};//SequencerEntryUpCommand

struct SequencerEntryDownCommand : public Command
{
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    unsigned int origIndex;

    SequencerEntryDownCommand(boost::shared_ptr<Sequencer> sequencer, boost::shared_ptr<SequencerEntry> entry);
    ~SequencerEntryDownCommand();

    void doAction();
    void undoAction();
};//SequencerEntryDownCommand

struct AddSequencerEntryCommand : public Command
{
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    bool useDefaults;

    AddSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer, bool useDefaults);
    ~AddSequencerEntryCommand();

    void doAction();
    void undoAction();
};//AddSequencerEntryCommand

struct DeleteSequencerEntryCommand : public Command
{
    boost::shared_ptr<Sequencer> sequencer;
    boost::shared_ptr<SequencerEntry> entry;
    unsigned int entryIndex;

    DeleteSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer, boost::shared_ptr<SequencerEntry> entry);
    ~DeleteSequencerEntryCommand();

    void doAction();
    void undoAction();
};//DeleteSequencerEntryCommand

struct AddTempoChangeCommand : public Command
{
    boost::shared_ptr<FMidiAutomationData> datas;
    boost::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;

    AddTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int tick_, boost::shared_ptr<FMidiAutomationData> datas_,
                            boost::function<void (void)> updateTempoChangesUIData);
    ~AddTempoChangeCommand();

    void doAction();
    void undoAction();
};//AddTempoChangeCommand

struct DeleteTempoChangeCommand : public Command
{
    boost::shared_ptr<FMidiAutomationData> datas;
    boost::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;

    DeleteTempoChangeCommand(unsigned int tick_, boost::shared_ptr<FMidiAutomationData> datas_, boost::function<void (void)> updateTempoChangesUIData);
    ~DeleteTempoChangeCommand();

    void doAction();
    void undoAction();
};//DeleteTempoChangeCommand

struct UpdateTempoChangeCommand : public Command
{
    boost::shared_ptr<Tempo> tempo;
    boost::function<void (void)> updateTempoChangesUIData;
    
    unsigned int old_bpm; //times 100
    unsigned int old_beatsPerBar;
    unsigned int old_barSubDivisions;   

    UpdateTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData);
    ~UpdateTempoChangeCommand();

    void doAction();
    void undoAction();
};//AddTempoChangeCommand

struct AddKeyframesCommand : public Command
{
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock;
    std::multimap<int, boost::shared_ptr<Keyframe> > keyframes;

    AddKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, int curMouseUnderTick, int curMouseUnderValue);
    AddKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, std::multimap<int, boost::shared_ptr<Keyframe> > &origKeyframes, int newTick);
    ~AddKeyframesCommand();

    void doAction();
    void undoAction();
};//AddKeyframesCommand

struct DeleteKeyframesCommand : public Command
{
    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    std::multimap<int, boost::shared_ptr<Keyframe> > keyframes;

    DeleteKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock, std::multimap<int, boost::shared_ptr<Keyframe> > &keyframes);
    ~DeleteKeyframesCommand();

    void doAction();
    void undoAction();
};//DeleteKeyframesCommand

struct MoveKeyframesCommand : public Command
{
    struct KeyInfo
    {
        boost::shared_ptr<Keyframe> keyframe;
        int movingKeyOrigTick;
        double movingKeyOrigValue;
    };//KeyInfo

    boost::shared_ptr<SequencerEntryBlock> entryBlock;
    std::vector<boost::shared_ptr<KeyInfo> > keyframes;

    MoveKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock, std::vector<boost::shared_ptr<KeyInfo> > &keyframes);
    ~MoveKeyframesCommand();

    void doAction();
    void undoAction();
};//MoveKeyframesCommand

struct ProcessRecordedMidiCommand : public Command
{
    std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap;
    std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap;

    ProcessRecordedMidiCommand(std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap, std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap);
    ~ProcessRecordedMidiCommand();

    void doAction();
    void undoAction();
};//ProcessRecordedMidiCommand


#endif
