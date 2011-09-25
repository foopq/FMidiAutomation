/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include <gtkmm.h>
#include <libglademm.h>
#include "Command_Other.h"
#include "Tempo.h"
#include "Data/Sequencer.h"
#include "UI/SequencerEntryUI.h"
#include "UI/SequencerUI.h"
#include "Animation.h"
#include "Globals.h"
#include "FMidiAutomationMainWindow.h"

Command::Command(Glib::ustring commandStr_, FMidiAutomationMainWindow *window_, CommandFilter commandFilter_)
{
    window = window_;
    commandStr = commandStr_;
    commandFilter = commandFilter_;
}//constructor

CommandQueue::CommandQueue()
{
    commandCount = 0;
}//constructor

std::shared_ptr<Command> CommandQueue::getNextCommand(FMidiAutomationMainWindow *window)
{
    std::shared_ptr<Command> command = getNextCommandNoRemove(window);
    if (command != nullptr) {
        if ((bothQueue.empty() == false) && (bothQueue.top().second == command)) {
            bothQueue.pop();
        }//if

        else if ((bothMainWindowOnlyQueue.empty() == false) && (bothMainWindowOnlyQueue.top().second == command)) {
            bothMainWindowOnlyQueue.pop();
        }//if

        else if ((sequencerQueue.empty() == false) && (sequencerQueue.top().second == command)) {
            sequencerQueue.pop();
        }//if

        else if ((curveEditorQueue.empty() == false) && (curveEditorQueue.top().second == command)) {
            curveEditorQueue.pop();
        }//if
    }//if

    return command;
}//getNextCommand

std::shared_ptr<Command> CommandQueue::getNextCommandNoRemove(FMidiAutomationMainWindow *window)
{
    WindowMode windowMode = window->getWindowMode();
    std::pair<int, std::shared_ptr<Command>> topCommand = std::make_pair(std::numeric_limits<int>::min(), std::shared_ptr<Command>());

    std::pair<int, std::shared_ptr<Command>> tmpCommand;

    if (bothQueue.empty() == false) {
        tmpCommand = bothQueue.top();
        if (tmpCommand.first > topCommand.first) {
            topCommand = tmpCommand;
        }//if
    }//if

    switch (windowMode) {
        case WindowMode::MainWindow:
            if (bothMainWindowOnlyQueue.empty() == false) {
                tmpCommand = bothMainWindowOnlyQueue.top();
                if (tmpCommand.first > topCommand.first) {
                    topCommand = tmpCommand;
                }//if
            }//if

            if (window->IsInSequencer() == true) {
                if (sequencerQueue.empty() == false) {
                    tmpCommand = sequencerQueue.top();
                    if (tmpCommand.first > topCommand.first) {
                        topCommand = tmpCommand;
                    }//if
                }//if
            } else {
                if (curveEditorQueue.empty() == false) {
                    tmpCommand = curveEditorQueue.top();
                    if (tmpCommand.first > topCommand.first) {
                        topCommand = tmpCommand;
                    }//if
                }//if
            }//if
            break;

        case WindowMode::CurveEditorOnly:
            //assert(window->IsInSequencer() == false); -- I want this assertion in, but sadly it fails during the creation of a new window (not loaded).. could be refactored I suppose..
            if (curveEditorQueue.empty() == false) {
                tmpCommand = curveEditorQueue.top();
                if (tmpCommand.first > topCommand.first) {
                    topCommand = tmpCommand;
                }//if
            }//if
            break;
    }//switch

    return topCommand.second;
}//getNextCommandNoRemove

void CommandQueue::clear()
{
    sequencerQueue = CommandQueueType();
    curveEditorQueue = CommandQueueType();
    bothQueue = CommandQueueType();
    bothMainWindowOnlyQueue = CommandQueueType();
}//clear

void CommandQueue::addNewCommand(std::shared_ptr<Command> command)
{
    commandCount++;
    std::pair<int, std::shared_ptr<Command>> newCommand = std::make_pair(commandCount, command);

    switch (command->commandFilter) {
        case CommandFilter::SequencerOnly:
            sequencerQueue.push(newCommand);
            break;
        case CommandFilter::CurveEditorOnly:
            curveEditorQueue.push(newCommand);
            break;
        case CommandFilter::Both:
            bothQueue.push(newCommand);
            break;
        case CommandFilter::BothMainWindowOnly:
            bothMainWindowOnlyQueue.push(newCommand);
            break;
    }//switch
}//addNewCommand

CommandManager &CommandManager::Instance()
{
    static CommandManager manager;
    return manager;
}//Instance

void CommandManager::setTitleStar(std::function<void (void)> titleStarFunc_)
{
    titleStarFunc = titleStarFunc_;
}//setTitleStar

void CommandManager::registerMenuItems(FMidiAutomationMainWindow *window, Gtk::ImageMenuItem *menuUndo, Gtk::ImageMenuItem *menuRedo)
{
    undoMenuMap[window] = menuUndo;
    redoMenuMap[window] = menuRedo;

std::cout << "%%%%%% registerMenuItems: " << undoMenuMap.size() << std::endl;

    updateUndoRedoMenus();
}//registerMenuItems

void CommandManager::unregisterMenuItems(FMidiAutomationMainWindow *window)
{
    auto mapIter = undoMenuMap.find(window);
    if (mapIter != undoMenuMap.end()) {
        undoMenuMap.erase(mapIter);
    }//if

    mapIter = redoMenuMap.find(window);
    if (mapIter != redoMenuMap.end()) {
        redoMenuMap.erase(mapIter);
    }//if
}//unregisterMenuItems

void CommandManager::updateUndoRedoMenus()
{
    for (auto mapIter : undoMenuMap) {
        std::shared_ptr<Command> nextCommand = undoStack.getNextCommandNoRemove(mapIter.first);

        Gtk::ImageMenuItem *menuItem = mapIter.second;
        if (nextCommand != nullptr) {
            menuItem->set_sensitive(true);
            menuItem->set_label("Undo " + nextCommand->commandStr);
        } else {
            menuItem->set_label("Undo");
            menuItem->set_sensitive(false);
        }//if
    }//for

    for (auto mapIter : redoMenuMap) {
        std::shared_ptr<Command> nextCommand = redoStack.getNextCommandNoRemove(mapIter.first);

        Gtk::ImageMenuItem *menuItem = mapIter.second;
        if (nextCommand != nullptr) {
            menuItem->set_sensitive(true);
            menuItem->set_label("Redo " + nextCommand->commandStr);
        } else {
            menuItem->set_label("Redo");
            menuItem->set_sensitive(false);
        }//if
    }//for
}//updateUndoRedoMenus

void CommandManager::doRedo(FMidiAutomationMainWindow *window)
{
    std::shared_ptr<Command> nextCommand = redoStack.getNextCommand(window);

    if (nextCommand == nullptr) {
        return;
    }//if

    nextCommand->doAction();
    undoStack.addNewCommand(nextCommand);

    for (auto mapIter : undoMenuMap) {
        mapIter.first->queue_draw();
    }//for

    updateUndoRedoMenus();
}//doRedo

void CommandManager::doUndo(FMidiAutomationMainWindow *window)
{
    std::shared_ptr<Command> nextCommand = undoStack.getNextCommand(window);

    if (nextCommand == nullptr) {
        return;
    }//if

    nextCommand->undoAction();
    redoStack.addNewCommand(nextCommand);

    for (auto mapIter : undoMenuMap) {
        mapIter.first->queue_draw();
    }//for

    updateUndoRedoMenus();
}//doUndo

void CommandManager::setNewCommand(std::shared_ptr<Command> command, bool applyCommand)
{
    redoStack.clear();

    undoStack.addNewCommand(command);

    if (true == applyCommand) {
        command->doAction();
    }//if

    titleStarFunc();
    updateUndoRedoMenus();

    command->window->queue_draw();
}//setNewCommand

//UpdateTempoChangeCommand
UpdateTempoChangeCommand::UpdateTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar,
                                                    unsigned int new_barSubDivisions, std::function<void (void)> updateTempoChangesUIData_,
                                                    FMidiAutomationMainWindow *window) : Command("Update Tempo Change", window, CommandFilter::Both)
{
    old_bpm = new_bpm;
    old_beatsPerBar = new_beatsPerBar;
    old_barSubDivisions = new_barSubDivisions;
    tempo = tempo_;
    updateTempoChangesUIData = updateTempoChangesUIData_;
}//constructor

UpdateTempoChangeCommand::~UpdateTempoChangeCommand()
{
    //Nothing
}//destructor

void UpdateTempoChangeCommand::doAction()
{
    std::swap(tempo->bpm, old_bpm);
    std::swap(tempo->beatsPerBar, old_beatsPerBar);
    std::swap(tempo->barSubDivisions, old_barSubDivisions);

    updateTempoChangesUIData();
}//doAction

void UpdateTempoChangeCommand::undoAction()
{
    doAction();
}//undoAction

//AddTempoChangeCommand
AddTempoChangeCommand::AddTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int tick_,
                                                std::function<void (void)> updateTempoChangesUIData_, FMidiAutomationMainWindow *window) 
                                                : Command("Add Tempo Change", window, CommandFilter::Both)
{
    tempo = tempo_;
    tick = tick_;
    updateTempoChangesUIData = updateTempoChangesUIData_;
}//constructor

AddTempoChangeCommand::~AddTempoChangeCommand()
{
    //Nothing
}//destructor

void AddTempoChangeCommand::doAction()
{
    Globals &globals = Globals::Instance();

    //assert(datas->tempoChanges.find(tick) == datas->tempoChanges.end());
    assert(globals.projectData.HasTempoChangeAtTick(tick) == false);
    globals.projectData.addTempoChange(tick, tempo);
    updateTempoChangesUIData();
}//doAction

void AddTempoChangeCommand::undoAction()
{
    Globals &globals = Globals::Instance();

    //assert(datas->tempoChanges.find(tick) != datas->tempoChanges.end());
    assert(globals.projectData.HasTempoChangeAtTick(tick) == true);
    globals.projectData.removeTempoChange(tick);
    updateTempoChangesUIData();
}//undoAction

//DeleteTempoChangeCommand
DeleteTempoChangeCommand::DeleteTempoChangeCommand(unsigned int tick_,
                                                    std::function<void (void)> updateTempoChangesUIData_, FMidiAutomationMainWindow *window) 
                                                    : Command("Delete Tempo Change", window, CommandFilter::Both)
{
    tick = tick_;
    updateTempoChangesUIData = updateTempoChangesUIData_;
}//constructor

DeleteTempoChangeCommand::~DeleteTempoChangeCommand()
{
    //Nothing
}//destructor

void DeleteTempoChangeCommand::doAction()
{
    Globals &globals = Globals::Instance();

    //assert(datas->tempoChanges.find(tick) != datas->tempoChanges.end());
    assert(globals.projectData.HasTempoChangeAtTick(tick) == true);
    tempo = globals.projectData.getTempoChangeAtTick(tick);
    globals.projectData.removeTempoChange(tick);
    updateTempoChangesUIData();
}//doAction

void DeleteTempoChangeCommand::undoAction()
{
    Globals &globals = Globals::Instance();

    //assert(datas->tempoChanges.find(tick) == datas->tempoChanges.end());
    assert(globals.projectData.HasTempoChangeAtTick(tick) == false);
    globals.projectData.addTempoChange(tick, tempo);
    updateTempoChangesUIData();
}//undoAction

//ProcessRecordedMidiCommand
ProcessRecordedMidiCommand::ProcessRecordedMidiCommand(decltype(Sequencer::entries) &origEntryMap_, decltype(Sequencer::entries) &newEntryMap_,
                                                        FMidiAutomationMainWindow *window) : Command("Process Recorded Midi", window, CommandFilter::BothMainWindowOnly)
{
    origEntryMap.swap(origEntryMap_);
    newEntryMap.swap(newEntryMap_);

    assert(origEntryMap.size() == newEntryMap.size());

    for (unsigned int pos = 0; pos < origEntryMap.size(); ++pos) {
        oldNewMap[origEntryMap[pos]] = newEntryMap[pos];
        newOldMap[newEntryMap[pos]] = origEntryMap[pos];
    }//for
}//constructor

ProcessRecordedMidiCommand::~ProcessRecordedMidiCommand()
{
    //Nothing
}//destructor

void ProcessRecordedMidiCommand::doAction()
{
    Globals &globals = Globals::Instance();
    globals.projectData.getSequencer()->setEntryMap(newEntryMap);

    for (auto entryIter : window->getSequencer()->getEntryPair()) {
        assert(oldNewMap[entryIter.first->getBaseEntry()] != nullptr);
        entryIter.first->setBaseEntry(oldNewMap[entryIter.first->getBaseEntry()]);
    }//for
}//doAction

void ProcessRecordedMidiCommand::undoAction()
{
    Globals &globals = Globals::Instance();
    globals.projectData.getSequencer()->setEntryMap(origEntryMap);

    for (auto entryIter : window->getSequencer()->getEntryPair()) {
        assert(newOldMap[entryIter.first->getBaseEntry()] != nullptr);
        entryIter.first->setBaseEntry(newOldMap[entryIter.first->getBaseEntry()]);
    }//for
}//undoAction



