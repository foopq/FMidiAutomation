/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include <gtkmm.h>
#include <libglademm.h>
#include "Command_Sequencer.h"
#include "Data/FMidiAutomationData.h"
#include "Data/Sequencer.h"
#include "UI/SequencerUI.h"
#include "Data/SequencerEntry.h"
#include "UI/SequencerEntryUI.h"
#include "Globals.h"
#include "FMidiAutomationMainWindow.h"


//AddSequencerEntryCommand
AddSequencerEntryCommand::AddSequencerEntryCommand(bool useDefaults_, FMidiAutomationMainWindow *window) : Command("Add Sequencer Entry", window)/*{{{*/
{
    useDefaults = useDefaults_;
}//constructor

AddSequencerEntryCommand::~AddSequencerEntryCommand()
{
    //Nothing
}//destructor

void AddSequencerEntryCommand::doAction()
{
    Globals &globals = Globals::Instance();

    if (entry == nullptr) {
        std::shared_ptr<SequencerEntry> newEntry(new SequencerEntry);
        globals.projectData.getSequencer()->addEntry(newEntry); //-1, useDefaults);
        entry = window->getSequencer()->addEntry(-1, useDefaults, newEntry);
    } else {
        globals.projectData.getSequencer()->addEntry(entry->getBaseEntry());
        window->getSequencer()->addEntry(-1, entry);
    }//if
}//doAction

void AddSequencerEntryCommand::undoAction()
{
    Globals &globals = Globals::Instance();

    globals.projectData.getSequencer()->deleteEntry(entry->getBaseEntry());
    window->getSequencer()->deleteEntry(entry);
}//undoAction/*}}}*/

//DeleteSequencerEntryCommand
DeleteSequencerEntryCommand::DeleteSequencerEntryCommand(std::shared_ptr<SequencerEntryUI> entry_, FMidiAutomationMainWindow *window) 
                                                            : Command("Delete Sequencer Entry", window)/*{{{*/
{
    entry = entry_;
    entryIndex = entry->getIndex();
}//constructor

DeleteSequencerEntryCommand::~DeleteSequencerEntryCommand()
{
    //Nothing
}//destructor

void DeleteSequencerEntryCommand::doAction()
{
    Globals &globals = Globals::Instance();

    globals.projectData.getSequencer()->deleteEntry(entry->getBaseEntry());
    window->getSequencer()->deleteEntry(entry);
}//doAction

void DeleteSequencerEntryCommand::undoAction()
{
    Globals &globals = Globals::Instance();

    globals.projectData.getSequencer()->addEntry(entry->getBaseEntry());
    window->getSequencer()->addEntry(entryIndex, entry);
}//undoAction/*}}}*/

//SequencerEntryUpCommand
SequencerEntryUpCommand::SequencerEntryUpCommand(std::shared_ptr<SequencerEntryUI> entry_, FMidiAutomationMainWindow *window) 
                                                        : Command("Sequencer Entry Up", window)/*{{{*/
{
    entry = entry_;
    origIndex = entry->getIndex();
}//constructor

SequencerEntryUpCommand::~SequencerEntryUpCommand()
{
    //Nothing
}//destructor

void SequencerEntryUpCommand::doAction()
{
    window->getSequencer()->deleteEntry(entry);
    window->getSequencer()->addEntry(origIndex - 1, entry);
}//doAction

void SequencerEntryUpCommand::undoAction()
{
    window->getSequencer()->deleteEntry(entry);
    window->getSequencer()->addEntry(origIndex, entry);
}//undoAction/*}}}*/

//SequencerEntryDownCommand
SequencerEntryDownCommand::SequencerEntryDownCommand(std::shared_ptr<SequencerEntryUI> entry_, FMidiAutomationMainWindow *window) 
                                                        : Command("Sequencer Entry Down", window)/*{{{*/
{
    entry = entry_;
    origIndex = entry->getIndex();
}//constructor

SequencerEntryDownCommand::~SequencerEntryDownCommand()
{
    //Nothing
}//destructor

void SequencerEntryDownCommand::doAction()
{
    window->getSequencer()->deleteEntry(entry);
    window->getSequencer()->addEntry(origIndex + 1, entry);
}//doAction

void SequencerEntryDownCommand::undoAction()
{
    window->getSequencer()->deleteEntry(entry);
    window->getSequencer()->addEntry(origIndex, entry);
}//undoAction/*}}}*/

#if 0
//AddSequencerEntryBlockCommand
AddSequencerEntryBlockCommand::AddSequencerEntryBlockCommand(std::shared_ptr<SequencerEntryUI> entry_, std::shared_ptr<SequencerEntryBlock> entryBlock_,
                                                                FMidiAutomationMainWindow *window) : Command("Add Sequencer Entry Block", window)/*{{{*/
{
    entry = entry_;
    entryBlock = entryBlock_;
    entryBlockUI.reset(new SequencerEntryBlockUI(entryBlock, entry_));

    std::cout << "add entry block: " << entryBlock->getStartTick() << " - " << *entryBlock->getRawStartTick() << std::endl;
}//constructor

AddSequencerEntryBlockCommand::~AddSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void AddSequencerEntryBlockCommand::doAction()
{
    entry->getBaseEntry()->addEntryBlock(entryBlock);
    entry->addEntryBlock(entryBlockUI);
}//doAction

void AddSequencerEntryBlockCommand::undoAction()
{
    entry->getBaseEntry()->removeEntryBlock(entryBlock);
    entry->removeEntryBlock(entryBlockUI);
}//undoAction/*}}}*/
#endif

//AddSequencerEntryBlocksCommand
AddSequencerEntryBlocksCommand::AddSequencerEntryBlocksCommand(
        std::vector<std::pair<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<SequencerEntryBlockUI>>> &entryBlocks_,
        FMidiAutomationMainWindow *window) : Command("Add Sequencer Entry Blocks", window)/*{{{*/
{
    entryBlocks.swap(entryBlocks_);
}//constructor

AddSequencerEntryBlocksCommand::~AddSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void AddSequencerEntryBlocksCommand::doAction()
{
    for (auto entryBlockIter : entryBlocks) {
        entryBlockIter.first->getBaseEntry()->addEntryBlock(entryBlockIter.second->getBaseEntryBlock());
        entryBlockIter.first->addEntryBlock(entryBlockIter.second);
    }//foreach
}//doAction

void AddSequencerEntryBlocksCommand::undoAction()
{
    for (auto entryBlockIter : entryBlocks) {
        entryBlockIter.first->getBaseEntry()->removeEntryBlock(entryBlockIter.second->getBaseEntryBlock());
        entryBlockIter.first->removeEntryBlock(entryBlockIter.second);
    }//foreach
}//undoAction/*}}}*/

#if 0
//DeleteSequencerEntryBlockCommand
DeleteSequencerEntryBlockCommand::DeleteSequencerEntryBlockCommand(std::shared_ptr<SequencerEntryBlockUI> entryBlock_, FMidiAutomationMainWindow *window) 
                                                                        : Command("Delete Sequencer Entry Block", window)/*{{{*/
{
    entryBlockUI = entryBlock_;
    entryBlock = entryBlockUI->getBaseEntryBlock();
    entry = entryBlockUI->getOwningEntry();
}//constructor

DeleteSequencerEntryBlockCommand::~DeleteSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void DeleteSequencerEntryBlockCommand::doAction()
{
    entry->getBaseEntry()->removeEntryBlock(entryBlock);
    entry->removeEntryBlock(entryBlockUI);
}//doAction

void DeleteSequencerEntryBlockCommand::undoAction()
{
    entry->getBaseEntry()->addEntryBlock(entryBlock);
    entry->addEntryBlock(entryBlockUI);
}//undoAction/*}}}*/
#endif

//DeleteSequencerEntryBlocksCommand
DeleteSequencerEntryBlocksCommand::DeleteSequencerEntryBlocksCommand(std::vector<std::shared_ptr<SequencerEntryBlockUI>> entryBlocks_,
                                                                        FMidiAutomationMainWindow *window) : Command("Delete Sequencer Entry Blocks", window)/*{{{*/
{
    entryBlocks = entryBlocks_;
}//constructor

DeleteSequencerEntryBlocksCommand::~DeleteSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void DeleteSequencerEntryBlocksCommand::doAction()
{
    for (auto entryBlock : entryBlocks) {
        entryBlock->getOwningEntry()->getBaseEntry()->removeEntryBlock(entryBlock->getBaseEntryBlock());
        entryBlock->getOwningEntry()->removeEntryBlock(entryBlock);
    }//for
}//doAction

void DeleteSequencerEntryBlocksCommand::undoAction()
{
    for (auto entryBlock : entryBlocks) {
        entryBlock->getOwningEntry()->getBaseEntry()->addEntryBlock(entryBlock->getBaseEntryBlock());
        entryBlock->getOwningEntry()->addEntryBlock(entryBlock);
    }//for
}//undoAction/*}}}*/

//ChangeSequencerEntryBlockPropertiesCommand
ChangeSequencerEntryBlockPropertiesCommand::ChangeSequencerEntryBlockPropertiesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock_, Glib::ustring newTitle_,
                                                                                        FMidiAutomationMainWindow *window) : Command("Change Sequencer Entry Block Properties", window)/*{{{*/
{
    entryBlock = entryBlock_;
    prevTitle = newTitle_;
}//constructor

ChangeSequencerEntryBlockPropertiesCommand::~ChangeSequencerEntryBlockPropertiesCommand()
{
    //Nothing
}//destructor

void ChangeSequencerEntryBlockPropertiesCommand::doAction()
{
    Glib::ustring curTitle = entryBlock->getTitle();
    entryBlock->setTitle(prevTitle);
    prevTitle = curTitle;
}//doAction

void ChangeSequencerEntryBlockPropertiesCommand::undoAction()
{
    doAction();
}//undoAction/*}}}*/

//MoveSequencerEntryBlockCommand
MoveSequencerEntryBlockCommand::MoveSequencerEntryBlockCommand(/*{{{*/
                                                                std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks_,
                                                                std::map<std::shared_ptr<SequencerEntryBlockUI>, int> entryOriginalStartTicks_,
                                                                std::map<std::shared_ptr<SequencerEntryBlockUI>, int> entryNewStartTicks_,
                                                                FMidiAutomationMainWindow *window) : Command("Move Sequencer Entry Block", window)
{
    entryBlocks = entryBlocks_;

    entryOriginalStartTicks = entryOriginalStartTicks_;
    entryNewStartTicks = entryNewStartTicks_;

std::cout << "move entry block: " << std::endl;

//for (std::map<std::shared_ptr<SequencerEntryBlock>, int>::iterator mapIter = entryOriginalStartTicks.begin(); mapIter != entryOriginalStartTicks.end(); ++mapIter) {
//    std::cout << "entryOriginalStartTicks: " << mapIter->second << std::endl;
//}//for

//for (std::map<std::shared_ptr<SequencerEntryBlock>, int>::iterator mapIter = entryNewStartTicks.begin(); mapIter != entryNewStartTicks.end(); ++mapIter) {
//    std::cout << "entryNewStartTicks: " << mapIter->second << std::endl;
//}//for

}//constructor

MoveSequencerEntryBlockCommand::~MoveSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void MoveSequencerEntryBlockCommand::doAction()
{
    std::cout << "MoveSequencerEntryBlockCommand::doAction()" << std::endl;

    for (auto blockIter : entryBlocks) {
        std::shared_ptr<SequencerEntryBlock> entryBlock = blockIter.second->getBaseEntryBlock();
        entryBlock->moveBlock(entryNewStartTicks[blockIter.second]);
    }//for
}//doAction

void MoveSequencerEntryBlockCommand::undoAction()
{
    std::cout << "MoveSequencerEntryBlockCommand::undoAction()" << std::endl;

    for (auto blockIter : entryBlocks) {
        std::shared_ptr<SequencerEntryBlock> entryBlock = blockIter.second->getBaseEntryBlock();
        entryBlock->moveBlock(entryOriginalStartTicks[blockIter.second]);
    }//for
}//undoAction/*}}}*/

//ChangeSequencerEntryPropertiesCommand
ChangeSequencerEntryPropertiesCommand::ChangeSequencerEntryPropertiesCommand(std::shared_ptr<SequencerEntry> entry_, std::shared_ptr<SequencerEntryImpl> origImpl_,
                                                                                std::shared_ptr<SequencerEntryImpl> newImpl_,
                                                                                FMidiAutomationMainWindow *window) : Command("Change Sequencer Entry Properties", window)/*{{{*/
{
    entry = entry_;
    origImpl = origImpl_;
    newImpl = newImpl_;
}//constructor

ChangeSequencerEntryPropertiesCommand::ChangeSequencerEntryPropertiesCommand::~ChangeSequencerEntryPropertiesCommand()
{
    //Nothing
}//destructor

void ChangeSequencerEntryPropertiesCommand::ChangeSequencerEntryPropertiesCommand::doAction()
{
    entry->setNewDataImpl(newImpl);
}//doAction

void ChangeSequencerEntryPropertiesCommand::ChangeSequencerEntryPropertiesCommand::undoAction()
{
    entry->setNewDataImpl(origImpl);
}//undoAction/*}}}*/

MergeSequencerEntryBlocksCommand::MergeSequencerEntryBlocksCommand(
                                        std::vector<std::shared_ptr<SequencerEntryBlockUI> > &origEntryBlocks_,
                                        std::vector<std::shared_ptr<SequencerEntryBlockUI> > &replacementEntryBlocks_,
                                        FMidiAutomationMainWindow *window) 
                                        : Command("Merge Sequencer Entry Blocks", window)
{
    origEntryBlocks.swap(origEntryBlocks_);
    replacementEntryBlocks.swap(replacementEntryBlocks_);

    std::cout << "MergeSequencerEntryBlocksCommand: " << origEntryBlocks.size() << " - " << replacementEntryBlocks.size() << std::endl;
}//constructor

MergeSequencerEntryBlocksCommand::~MergeSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void MergeSequencerEntryBlocksCommand::doAction()
{
    for (auto entryBlockIter : origEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->removeEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->removeEntryBlock(entryBlockIter);
    }//foreach

    for (auto entryBlockIter : replacementEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->addEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->addEntryBlock(entryBlockIter);
    }//foreach
}//doAction

void MergeSequencerEntryBlocksCommand::undoAction()
{
    for (auto entryBlockIter : replacementEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->removeEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->removeEntryBlock(entryBlockIter);
    }//foreach

    for (auto entryBlockIter : origEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->addEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->addEntryBlock(entryBlockIter);
    }//foreach
}//undoAction

SplitSequencerEntryBlocksCommand::SplitSequencerEntryBlocksCommand(
                                        std::set<std::shared_ptr<SequencerEntryBlockUI> > &origEntryBlocks_,
                                        std::vector<std::shared_ptr<SequencerEntryBlockUI> > &replacementEntryBlocks_,
                                        FMidiAutomationMainWindow *window) 
                                        : Command("Split Sequencer Entry Blocks", window)
{
    origEntryBlocks.swap(origEntryBlocks_);
    replacementEntryBlocks.swap(replacementEntryBlocks_);
}//constructor

SplitSequencerEntryBlocksCommand::~SplitSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void SplitSequencerEntryBlocksCommand::doAction()
{
    for (auto entryBlockIter : origEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->removeEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->removeEntryBlock(entryBlockIter);
    }//foreach

    for (auto entryBlockIter : replacementEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->addEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->addEntryBlock(entryBlockIter);
    }//foreach
}//doAction

void SplitSequencerEntryBlocksCommand::undoAction()
{
    for (auto entryBlockIter : replacementEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->removeEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->removeEntryBlock(entryBlockIter);
    }//foreach

    for (auto entryBlockIter : origEntryBlocks) {
        entryBlockIter->getOwningEntry()->getBaseEntry()->addEntryBlock(entryBlockIter->getBaseEntryBlock());
        entryBlockIter->getOwningEntry()->addEntryBlock(entryBlockIter);
    }//foreach
}//undoAction




