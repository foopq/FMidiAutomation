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

PasteCommand::PasteCommand(const Glib::ustring &pasteStr_)
{
    pasteStr = pasteStr_;
}//constructor

PasteManager &PasteManager::Instance()
{
    static PasteManager manager;
    return manager;
}//Instance

PasteManager::PasteManager()
{
    //Nothing
}//constructor

void PasteManager::registerMenuItems(FMidiAutomationMainWindow *window, Gtk::ImageMenuItem *menuPaste, Gtk::ImageMenuItem *menuPasteInstance,
                                       Gtk::MenuItem *pasteBlocksToEntry, Gtk::MenuItem *pasteInstanceBlocksToEntry)
{
    menuPasteMap[window] = menuPaste;
    menuPasteInstanceMap[window] = menuPasteInstance;
    pasteBlocksToEntryMap[window] = pasteBlocksToEntry;
    pasteInstanceBlocksToEntryMap[window] = pasteInstanceBlocksToEntry;
}//registerMenuItems

void PasteManager::unregisterMenuItems(FMidiAutomationMainWindow *window)
{
std::cout << "^^^^ PasteManager::unregisterMenuItems" << std::endl;

    auto mapIter = menuPasteMap.find(window);
    if (mapIter != menuPasteMap.end()) {
        menuPasteMap.erase(mapIter);
    }//if

    mapIter = menuPasteInstanceMap.find(window);
    if (mapIter != menuPasteInstanceMap.end()) {
        menuPasteInstanceMap.erase(mapIter);
    }//if

    auto mapIter2 = pasteBlocksToEntryMap.find(window);
    if (mapIter2 != pasteBlocksToEntryMap.end()) {
        pasteBlocksToEntryMap.erase(mapIter2);
    }//if

    mapIter2 = pasteInstanceBlocksToEntryMap.find(window);
    if (mapIter2 != pasteInstanceBlocksToEntryMap.end()) {
        pasteInstanceBlocksToEntryMap.erase(mapIter2);
    }//if
}//unregisterMenuItems

void PasteManager::doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    if (targetWindow->IsInSequencer() == true) {
        if (sequencerPasteCommand != nullptr) {
            sequencerPasteCommand->doPaste(contextData, targetWindow);
        }//if
    } else {
        if (curveEditorPasteCommand != nullptr) {
            curveEditorPasteCommand->doPaste(contextData, targetWindow);
        }//if
    }//if
}//doPaste

void PasteManager::doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow)
{
    if (targetWindow->IsInSequencer() == true) {
        if (sequencerPasteCommand != nullptr) {
            sequencerPasteCommand->doPasteInstance(contextData, targetWindow);
        }//if
    } else {
        if (curveEditorPasteCommand != nullptr) {
            curveEditorPasteCommand->doPasteInstance(contextData, targetWindow);
        }//if
    }//if
}//doPasteInstance

void PasteManager::clearCommands()
{
    sequencerPasteCommand.reset();
    curveEditorPasteCommand.reset();
    updateMenus();
}//clearCommands

void PasteManager::updateMenus()
{
    for (auto mapIter : menuPasteMap) {
        FMidiAutomationMainWindow *window = mapIter.first;

        if (window->IsInSequencer() == true) {
            if (sequencerPasteCommand != nullptr) {
                menuPasteMap[window]->set_sensitive(true);
                menuPasteMap[window]->set_label("Paste " + sequencerPasteCommand->pasteStr);

                menuPasteInstanceMap[window]->set_sensitive(true);
                menuPasteInstanceMap[window]->set_label("Paste Instance of " + sequencerPasteCommand->pasteStr);
                menuPasteInstanceMap[window]->set_visible(true);

                pasteBlocksToEntryMap[window]->set_sensitive(true);
                pasteBlocksToEntryMap[window]->set_label("Paste to Entry " + sequencerPasteCommand->pasteStr);
                pasteBlocksToEntryMap[window]->set_visible(true);

                pasteInstanceBlocksToEntryMap[window]->set_sensitive(true);
                pasteInstanceBlocksToEntryMap[window]->set_label("Paste Instances to Entry " + sequencerPasteCommand->pasteStr);
                pasteInstanceBlocksToEntryMap[window]->set_visible(true);
            } else {
                menuPasteMap[window]->set_sensitive(false);
                menuPasteMap[window]->set_label("Paste");
                menuPasteInstanceMap[window]->set_visible(true);

                menuPasteInstanceMap[window]->set_sensitive(false);
                menuPasteInstanceMap[window]->set_label("Paste Instance");

                pasteBlocksToEntryMap[window]->set_sensitive(false);
                pasteBlocksToEntryMap[window]->set_label("Paste to Entry");
                pasteBlocksToEntryMap[window]->set_visible(true);

                pasteInstanceBlocksToEntryMap[window]->set_sensitive(false);
                pasteInstanceBlocksToEntryMap[window]->set_label("Paste Instances to Entry");
                pasteInstanceBlocksToEntryMap[window]->set_visible(true);
            }//if
        } else {
            if (curveEditorPasteCommand != nullptr) {
                menuPasteMap[window]->set_sensitive(true);
                menuPasteMap[window]->set_label("Paste " + curveEditorPasteCommand->pasteStr);
            } else {
                menuPasteMap[window]->set_sensitive(false);
                menuPasteMap[window]->set_label("Paste");
            }//if

            menuPasteInstanceMap[window]->set_sensitive(false);
            menuPasteInstanceMap[window]->set_label("");
            menuPasteInstanceMap[window]->set_visible(false);

            pasteBlocksToEntryMap[window]->set_sensitive(false);
            pasteBlocksToEntryMap[window]->set_label("");
            pasteBlocksToEntryMap[window]->set_visible(false);

            pasteInstanceBlocksToEntryMap[window]->set_sensitive(false);
            pasteInstanceBlocksToEntryMap[window]->set_label("");
            pasteInstanceBlocksToEntryMap[window]->set_visible(false);
        }//if
    }//for
}//updateMenus


void PasteManager::setNewCommand(FMidiAutomationMainWindow *window, std::shared_ptr<PasteCommand> command_)
{
    if (window->IsInSequencer() == true) {
        sequencerPasteCommand = command_;
    } else {
        curveEditorPasteCommand = command_;
    }//if

    updateMenus();
}//setNewCommand

PasteSequencerEntryBlocksCommand::PasteSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks_)
            : PasteCommand("Sequencer Entry Blocks")
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
            : PasteCommand("Keyframes")
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


