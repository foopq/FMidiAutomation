/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "PasteManager.h"
#include "Data/SequencerEntry.h"
#include "UI/SequencerEntryBlockUI.h"
#include "UI/SequencerEntryUI.h"
#include "Command_Sequencer.h"
#include "Command_CurveEditor.h"
#include "Animation.h"
#include "Globals.h"
#include "GraphState.h"
#include "FMidiAutomationMainWindow.h"

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

void PasteManager::doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    if (command != nullptr) {
        command->doPaste(contextData, targetWindow);
    }//if
}//doPaste

void PasteManager::doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    if (command != nullptr) {
        command->doPasteInstance(contextData, targetWindow);
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

PasteSequencerEntryBlocksCommand::PasteSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks_)
{
    entryBlocks = entryBlocks_;
}//constructor

PasteSequencerEntryBlocksCommand::~PasteSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void PasteSequencerEntryBlocksCommand::doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
std::cout << "PasteSequencerEntryBlocksCommand::doPaste" << std::endl;

    if (targetWindow->getGraphState().displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    if (entryBlocks.empty() == true) {
        return;
    }//if

    std::shared_ptr<SequencerEntryUI> targetSequencerEntry = boost::any_cast<std::shared_ptr<SequencerEntryUI> >(contextData);

    int firstEntryOrigStartTick = entryBlocks.begin()->second->getBaseEntryBlock()->getStartTick();
    int tickOffset = targetWindow->getGraphState().curPointerTick - firstEntryOrigStartTick;

    std::vector<std::pair<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<SequencerEntryBlockUI>>> newEntryBlocks;
    newEntryBlocks.reserve(entryBlocks.size());

    for (auto entryIter : entryBlocks) {
        std::shared_ptr<SequencerEntryUI> selectedEntry = entryIter.second->getOwningEntry();

        if (targetSequencerEntry != nullptr) {
            selectedEntry = targetSequencerEntry;
        }//if        

        if (selectedEntry == nullptr) {
            continue;
        }//if

        int startTick = entryIter.second->getBaseEntryBlock()->getStartTick() + tickOffset;

        std::shared_ptr<SequencerEntryBlock> newEntryBlock;
        newEntryBlock = entryIter.second->getBaseEntryBlock()->deepClone(selectedEntry->getBaseEntry(), startTick);
        newEntryBlock->setTitle(entryIter.second->getBaseEntryBlock()->getTitle());

        std::shared_ptr<SequencerEntryBlockUI> newEntryBlockUI(new SequencerEntryBlockUI(newEntryBlock, selectedEntry));

        newEntryBlocks.push_back(std::make_pair(selectedEntry, newEntryBlockUI));
    }//for

    std::shared_ptr<Command> addSequencerEntryBlocksCommand(new AddSequencerEntryBlocksCommand(newEntryBlocks, targetWindow));
    CommandManager::Instance().setNewCommand(addSequencerEntryBlocksCommand, true);
}//doPaste

void PasteSequencerEntryBlocksCommand::doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    if (targetWindow->getGraphState().displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    if (entryBlocks.empty() == true) {
        return;
    }//if

    std::shared_ptr<SequencerEntryUI> targetSequencerEntry = boost::any_cast<std::shared_ptr<SequencerEntryUI> >(contextData);

    int firstEntryOrigStartTick = entryBlocks.begin()->second->getBaseEntryBlock()->getStartTick();
    int tickOffset = targetWindow->getGraphState().curPointerTick - firstEntryOrigStartTick;

    std::vector<std::pair<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<SequencerEntryBlockUI>>> newEntryBlocks;
    newEntryBlocks.reserve(entryBlocks.size());

    for (auto entryIter : entryBlocks) {
        std::shared_ptr<SequencerEntryUI> selectedEntry = entryIter.second->getOwningEntry();

        if (targetSequencerEntry != nullptr) {
            selectedEntry = targetSequencerEntry;
        }//if        

        if (selectedEntry == nullptr) {
            continue;
        }//if

        int startTick = entryIter.second->getBaseEntryBlock()->getStartTick() + tickOffset;

        std::shared_ptr<SequencerEntryBlock> newEntryBlock;
        newEntryBlock.reset(new SequencerEntryBlock(selectedEntry->getBaseEntry(), startTick, entryIter.second->getBaseEntryBlock()));
        newEntryBlock->setTitle(entryIter.second->getBaseEntryBlock()->getTitle() + " (Instance)");
        newEntryBlock->cloneCurves(entryIter.second->getBaseEntryBlock());

        std::shared_ptr<SequencerEntryBlockUI> newEntryBlockUI(new SequencerEntryBlockUI(newEntryBlock, selectedEntry));

        newEntryBlocks.push_back(std::make_pair(selectedEntry, newEntryBlockUI));
    }//for

    std::shared_ptr<Command> addSequencerEntryBlocksCommand(new AddSequencerEntryBlocksCommand(newEntryBlocks, targetWindow));
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

void PasteSequencerKeyframesCommand::doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    if (targetWindow->getGraphState().displayMode != DisplayMode::Curve) {
        return;
    }//if

    std::shared_ptr<SequencerEntryBlockUI> currentlySelectedEntryBlock = targetWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();
    if (currentlySelectedEntryBlock == nullptr) {
        return;
    }//if

    int tick = targetWindow->getGraphState().curPointerTick;

    //if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(tick) != nullptr) {
    //    return;
    //}//if

    std::shared_ptr<Command> addKeyframesCommand(new AddKeyframesCommand(currentlySelectedEntryBlock->getBaseEntryBlock(), keyframes, tick - currentlySelectedEntryBlock->getBaseEntryBlock()->getStartTick(), targetWindow));
    CommandManager::Instance().setNewCommand(addKeyframesCommand, true);
}//doPaste

void PasteSequencerKeyframesCommand::doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    doPaste(std::shared_ptr<SequencerEntry>(), targetWindow);
}//doPasteInstance


