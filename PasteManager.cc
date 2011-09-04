/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "PasteManager.h"
#include "Sequencer.h"
#include "Command.h"
#include "Animation.h"
#include <boost/foreach.hpp>
#include "Globals.h"
#include "GraphState.h"

PasteManager &PasteManager::Instance()
{
    static PasteManager manager;
    return manager;
}//Instance

PasteManager::PasteManager()
{
    pasteOnly = false;
}//constructor

void PasteManager::setPasteOnly(bool pasteOnly_)
{
    pasteOnly = pasteOnly_;
    if (true == pasteOnly) {
        menuPasteInstance->set_sensitive(false);
        pasteInstanceBlocksToEntry->set_sensitive(false);
    }//if
}//setPasteOnly

void PasteManager::setMenuItems(Gtk::ImageMenuItem *menuPaste_, Gtk::ImageMenuItem *menuPasteInstance_,
                                Gtk::MenuItem *pasteBlocksToEntry_, Gtk::MenuItem *pasteInstanceBlocksToEntry_)
{
    menuPaste = menuPaste_;
    menuPasteInstance = menuPasteInstance_;
    pasteBlocksToEntry = pasteBlocksToEntry_;
    pasteInstanceBlocksToEntry = pasteInstanceBlocksToEntry_;
}//setMenuItems

void PasteManager::doPaste(boost::any contextData)
{
    if (command != NULL) {
        command->doPaste(contextData);
    }//if
}//doPaste

void PasteManager::doPasteInstance(boost::any contextData)
{
    if (command != NULL) {
        command->doPasteInstance(contextData);
    }//if
}//doPasteInstance

void PasteManager::clearCommand()
{
    command.reset();
}//clearcommand

void PasteManager::setNewCommand(std::shared_ptr<PasteCommand> command_)
{
    command = command_;
    menuPaste->set_sensitive(true);
    pasteBlocksToEntry->set_sensitive(true);

    if (false == pasteOnly) {
        menuPasteInstance->set_sensitive(true);
        pasteInstanceBlocksToEntry->set_sensitive(true);
    }//if
}//setNewCommand

PasteSequencerEntryBlocksCommand::PasteSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks_)
{
    entryBlocks = entryBlocks_;

std::cout << "PasteSequencerEntryBlocksCommand: " << entryBlocks.size() << std::endl;
}//constructor

PasteSequencerEntryBlocksCommand::~PasteSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void PasteSequencerEntryBlocksCommand::doPaste(boost::any contextData)
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    if (entryBlocks.empty() == true) {
        return;
    }//if

    std::shared_ptr<SequencerEntry> targetSequencerEntry = boost::any_cast<std::shared_ptr<SequencerEntry> >(contextData);

    int firstEntryOrigStartTick = entryBlocks.begin()->second->getStartTick();
    int tickOffset = globals.graphState->curPointerTick - firstEntryOrigStartTick;

    std::vector<std::pair<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntryBlock>>> newEntryBlocks;
    newEntryBlocks.reserve(entryBlocks.size());

std::cout << "PasteSequencerEntryBlocksCommand::doPaste " << entryBlocks.size() << std::endl;

    BOOST_FOREACH (auto entryIter, entryBlocks) {
        std::shared_ptr<SequencerEntry> selectedEntry = entryIter.second->getOwningEntry();

std::cout << "PasteSequencerEntryBlocksCommand 1" << std::endl;

        if (targetSequencerEntry != NULL) {
            selectedEntry = targetSequencerEntry;
        }//if        

        if (selectedEntry == NULL) {
            continue;
        }//if

std::cout << "PasteSequencerEntryBlocksCommand 2: " << selectedEntry->getTitle() << std::endl;

        int startTick = entryIter.second->getStartTick() + tickOffset;

        std::shared_ptr<SequencerEntryBlock> newEntryBlock(new SequencerEntryBlock(selectedEntry, startTick, entryIter.second));
        newEntryBlock->setTitle(entryIter.second->getTitle());
        newEntryBlock->cloneCurves(entryIter.second);

        newEntryBlocks.push_back(std::make_pair(selectedEntry, newEntryBlock));
    }//for

    std::shared_ptr<Command> addSequencerEntryBlocksCommand(new AddSequencerEntryBlocksCommand(newEntryBlocks));
    CommandManager::Instance().setNewCommand(addSequencerEntryBlocksCommand, true);
}//doPaste

void PasteSequencerEntryBlocksCommand::doPasteInstance(boost::any contextData)
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    if (entryBlocks.empty() == true) {
        return;
    }//if

    std::shared_ptr<SequencerEntry> targetSequencerEntry = boost::any_cast<std::shared_ptr<SequencerEntry> >(contextData);

    int firstEntryOrigStartTick = entryBlocks.begin()->second->getStartTick();
    int tickOffset = globals.graphState->curPointerTick - firstEntryOrigStartTick;

    std::vector<std::pair<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntryBlock>>> newEntryBlocks;
    newEntryBlocks.reserve(entryBlocks.size());

    for (std::map<int, std::shared_ptr<SequencerEntryBlock> >::const_iterator entryIter = entryBlocks.begin(); entryIter != entryBlocks.end(); ++entryIter) {
        std::shared_ptr<SequencerEntry> selectedEntry = entryIter->second->getOwningEntry();

        if (targetSequencerEntry != NULL) {
            selectedEntry = targetSequencerEntry;
        }//if

        if (selectedEntry == NULL) {
            continue;
        }//if

        int startTick = entryIter->second->getStartTick() + tickOffset;

        std::shared_ptr<SequencerEntryBlock> newEntryBlock(new SequencerEntryBlock(selectedEntry, startTick, entryIter->second));
        newEntryBlock->setTitle(entryIter->second->getTitle() + " (Instance)");

        newEntryBlocks.push_back(std::make_pair(selectedEntry, newEntryBlock));
    }//for

    std::shared_ptr<Command> addSequencerEntryBlocksCommand(new AddSequencerEntryBlocksCommand(newEntryBlocks));
    CommandManager::Instance().setNewCommand(addSequencerEntryBlocksCommand, true);
}//doPasteInstance

PasteSequencerKeyframesCommand::PasteSequencerKeyframesCommand(std::map<int, std::shared_ptr<Keyframe> > keyframes_)
{
    keyframes = keyframes_;
}//constructor

PasteSequencerKeyframesCommand::~PasteSequencerKeyframesCommand()
{
    //Nothing
}//destructor

void PasteSequencerKeyframesCommand::doPaste(boost::any contextData)
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Curve) {
        return;
    }//if

    std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = globals.graphState->entryBlockSelectionState.GetFirstEntryBlock();
    if (currentlySelectedEntryBlock == NULL) {
        return;
    }//if

    int tick = globals.graphState->curPointerTick;

    //if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(tick) != NULL) {
    //    return;
    //}//if

    std::shared_ptr<Command> addKeyframesCommand(new AddKeyframesCommand(currentlySelectedEntryBlock, keyframes, tick - currentlySelectedEntryBlock->getStartTick()));
    CommandManager::Instance().setNewCommand(addKeyframesCommand, true);
}//doPaste

void PasteSequencerKeyframesCommand::doPasteInstance(boost::any contextData)
{
    doPaste(std::shared_ptr<SequencerEntry>());
}//doPasteInstance


