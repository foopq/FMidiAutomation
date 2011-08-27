/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "PasteManager.h"
#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include "Command.h"
#include "Animation.h"

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
    }//if
}//setPasteOnly

void PasteManager::setMenuItems(Gtk::ImageMenuItem *menuPaste_, Gtk::ImageMenuItem *menuPasteInstance_)
{
    menuPaste = menuPaste_;
    menuPasteInstance = menuPasteInstance_;
}//setMenuItems

void PasteManager::doPaste()
{
    if (command != NULL) {
        command->doPaste();
    }//if
}//doPaste

void PasteManager::doPasteInstance()
{
    if (command != NULL) {
        command->doPasteInstance();
    }//if
}//doPasteInstance

void PasteManager::clearCommand()
{
    command.reset();
}//clearcommand

void PasteManager::setNewCommand(boost::shared_ptr<PasteCommand> command_)
{
    command = command_;
    menuPaste->set_sensitive(true);

    if (false == pasteOnly) {
        menuPasteInstance->set_sensitive(true);
    }//if
}//setNewCommand

PasteSequencerEntryBlocksCommand::PasteSequencerEntryBlocksCommand(std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > entryBlocks_)
{
    entryBlocks = entryBlocks_;
}//constructor

PasteSequencerEntryBlocksCommand::~PasteSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void PasteSequencerEntryBlocksCommand::doPaste()
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    if (entryBlocks.empty() == true) {
        return;
    }//if

    int firstEntryOrigStartTick = entryBlocks.begin()->second->getStartTick();
    int tickOffset = globals.graphState->curPointerTick - firstEntryOrigStartTick;

    for (std::multimap<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator entryIter = entryBlocks.begin(); entryIter != entryBlocks.end(); ++entryIter) {
        //boost::shared_ptr<SequencerEntry> selectedEntry = globals.sequencer->getSelectedEntry();
        boost::shared_ptr<SequencerEntry> selectedEntry = entryIter->second->getOwningEntry();

        if (selectedEntry == NULL) {
            continue;
        }//if

        int startTick = entryIter->second->getStartTick() + tickOffset;

        boost::shared_ptr<SequencerEntryBlock> newEntryBlock(new SequencerEntryBlock(selectedEntry, startTick, boost::shared_ptr<SequencerEntryBlock>()));
        newEntryBlock->setTitle(entryIter->second->getTitle());

        newEntryBlock->cloneCurves(entryIter->second);
        boost::shared_ptr<Command> addSequencerEntryBlockCommand(new AddSequencerEntryBlockCommand(selectedEntry, newEntryBlock));
        CommandManager::Instance().setNewCommand(addSequencerEntryBlockCommand, true);
    }//for
}//doPaste

void PasteSequencerEntryBlocksCommand::doPasteInstance()
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    if (entryBlocks.empty() == true) {
        return;
    }//if

    int firstEntryOrigStartTick = entryBlocks.begin()->second->getStartTick();
    int tickOffset = globals.graphState->curPointerTick - firstEntryOrigStartTick;

    for (std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator entryIter = entryBlocks.begin(); entryIter != entryBlocks.end(); ++entryIter) {
        //boost::shared_ptr<SequencerEntry> selectedEntry = globals.sequencer->getSelectedEntry();
        boost::shared_ptr<SequencerEntry> selectedEntry = entryIter->second->getOwningEntry();

        if (selectedEntry == NULL) {
            continue;
        }//if

        int startTick = entryIter->second->getStartTick() + tickOffset;

        boost::shared_ptr<SequencerEntryBlock> newEntryBlock(new SequencerEntryBlock(selectedEntry, startTick, entryIter->second));
        newEntryBlock->setTitle(entryIter->second->getTitle() + " (Instance)");

        //newEntryBlock->cloneCurves(entryIter->second); ?? do this here?
        //FIXME: and clone the curves, etc... -- does this still make sense?
        boost::shared_ptr<Command> addSequencerEntryBlockCommand(new AddSequencerEntryBlockCommand(selectedEntry, newEntryBlock));
        CommandManager::Instance().setNewCommand(addSequencerEntryBlockCommand, true);
    }//for
}//doPasteInstance

PasteSequencerKeyframesCommand::PasteSequencerKeyframesCommand(std::map<int, boost::shared_ptr<Keyframe> > &keyframes_)
{
    keyframes = keyframes_;
}//constructor

PasteSequencerKeyframesCommand::~PasteSequencerKeyframesCommand()
{
    //Nothing
}//destructor

void PasteSequencerKeyframesCommand::doPaste()
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Curve) {
        return;
    }//if

    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = globals.graphState->entryBlockSelectionState.GetFirstEntryBlock();
    if (currentlySelectedEntryBlock == NULL) {
        return;
    }//if

    int tick = globals.graphState->curPointerTick;

    //if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(tick) != NULL) {
    //    return;
    //}//if

    boost::shared_ptr<Command> addKeyframesCommand(new AddKeyframesCommand(currentlySelectedEntryBlock, keyframes, tick - currentlySelectedEntryBlock->getStartTick()));
    CommandManager::Instance().setNewCommand(addKeyframesCommand, true);
}//doPaste

void PasteSequencerKeyframesCommand::doPasteInstance()
{
    doPaste();
}//doPasteInstance


