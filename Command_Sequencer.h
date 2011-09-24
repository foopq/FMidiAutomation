/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __COMMAND_SEQUENCER_H
#define __COMMAND_SEQUENCER_H

#include <memory>
#include <boost/function.hpp>
#include <stack>
#include "Data/Sequencer.h"
#include "Command_Other.h"

class SequencerEntry;
class SequencerEntryUI;
class SequencerEntryBlock;
class SequencerEntryBlockUI;
struct SequencerEntryImpl;
class FMidiAutomationMainWindow;

struct ChangeSequencerEntryPropertiesCommand : public Command
{
    ChangeSequencerEntryPropertiesCommand(std::shared_ptr<SequencerEntry> entry, std::shared_ptr<SequencerEntryImpl> origImpl, std::shared_ptr<SequencerEntryImpl> newImpl, FMidiAutomationMainWindow *window);
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
    MoveSequencerEntryBlockCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks,
                                    std::map<std::shared_ptr<SequencerEntryBlockUI>, int> entryOriginalStartTicks, //FIXME: This should be a reference
                                    std::map<std::shared_ptr<SequencerEntryBlockUI>, int> entryNewStartTicks,
                                    FMidiAutomationMainWindow *window);

    virtual ~MoveSequencerEntryBlockCommand();

    void doAction();
    void undoAction();

private:
    std::map<std::shared_ptr<SequencerEntryBlockUI>, int> entryOriginalStartTicks;
    std::map<std::shared_ptr<SequencerEntryBlockUI>, int> entryNewStartTicks;
    std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks;
};//MoveSequencerEntryBlockCommand

struct ChangeSequencerEntryBlockPropertiesCommand : public Command
{
    ChangeSequencerEntryBlockPropertiesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock, Glib::ustring newTitle,
                                                FMidiAutomationMainWindow *window);
    virtual ~ChangeSequencerEntryBlockPropertiesCommand();

    void doAction();
    void undoAction();

private:    
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    Glib::ustring prevTitle;
};//ChangeSequencerEntryBlockPropertiesCommand

struct AddSequencerEntryBlocksCommand : public Command
{
    AddSequencerEntryBlocksCommand(std::vector<std::pair<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<SequencerEntryBlockUI>>> &entryBlocks_,
                                        FMidiAutomationMainWindow *window);
    virtual ~AddSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:
    std::vector<std::pair<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<SequencerEntryBlockUI>>> entryBlocks;
};//AddSequencerEntryBlocksCommand

struct DeleteSequencerEntryBlocksCommand : public Command
{
    //FIXME: This shouldn't be a copy, but a reference!
    DeleteSequencerEntryBlocksCommand(std::vector<std::shared_ptr<SequencerEntryBlockUI>> entryBlocks, FMidiAutomationMainWindow *window);
    virtual ~DeleteSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private: 
    std::vector<std::shared_ptr<SequencerEntryBlockUI>> entryBlocks;
};//DeleteSequencerEntryBlocksCommand

struct SequencerEntryUpCommand : public Command
{
    SequencerEntryUpCommand(std::shared_ptr<SequencerEntryUI> entry, FMidiAutomationMainWindow *window);
    virtual ~SequencerEntryUpCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryUI> entry;
    unsigned int origIndex;
};//SequencerEntryUpCommand

struct SequencerEntryDownCommand : public Command
{
    SequencerEntryDownCommand(std::shared_ptr<SequencerEntryUI> entry, FMidiAutomationMainWindow *window);
    virtual ~SequencerEntryDownCommand();

    void doAction();
    void undoAction();

private: 
    std::shared_ptr<SequencerEntryUI> entry;
    unsigned int origIndex;
};//SequencerEntryDownCommand

struct AddSequencerEntryCommand : public Command
{
    AddSequencerEntryCommand(bool useDefaults, FMidiAutomationMainWindow *window);
    virtual ~AddSequencerEntryCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryUI> entry;
    bool useDefaults;
};//AddSequencerEntryCommand

struct DeleteSequencerEntryCommand : public Command
{
    DeleteSequencerEntryCommand(std::shared_ptr<SequencerEntryUI> entry, FMidiAutomationMainWindow *window);
    virtual ~DeleteSequencerEntryCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryUI> entry;
    unsigned int entryIndex;
};//DeleteSequencerEntryCommand

struct MergeSequencerEntryBlocksCommand : public Command
{
    MergeSequencerEntryBlocksCommand(std::vector<std::shared_ptr<SequencerEntryBlockUI> > &origEntryBlocks,
                                        std::vector<std::shared_ptr<SequencerEntryBlockUI> > &replacementEntryBlocks,
                                        FMidiAutomationMainWindow *window);
    virtual ~MergeSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:    
    std::vector<std::shared_ptr<SequencerEntryBlockUI> > origEntryBlocks;
    std::vector<std::shared_ptr<SequencerEntryBlockUI> > replacementEntryBlocks;
};//MergeSequencerEntryBlocksCommand

struct SplitSequencerEntryBlocksCommand : public Command
{
    SplitSequencerEntryBlocksCommand(std::set<std::shared_ptr<SequencerEntryBlockUI> > &origEntryBlocks,
                                        std::vector<std::shared_ptr<SequencerEntryBlockUI> > &replacementEntryBlocks,
                                        FMidiAutomationMainWindow *window);
    virtual ~SplitSequencerEntryBlocksCommand();

    void doAction();
    void undoAction();

private:    
    std::set<std::shared_ptr<SequencerEntryBlockUI> > origEntryBlocks;
    std::vector<std::shared_ptr<SequencerEntryBlockUI> > replacementEntryBlocks;
};//SplitSequencerEntryBlocksCommand

#endif
