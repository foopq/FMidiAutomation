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
//#include "Data/FMidiAutomationData.h"
#include "Data/Sequencer.h"
#include "UI/SequencerEntryUI.h"
#include "UI/SequencerUI.h"
#include "Animation.h"
#include "Globals.h"
#include "FMidiAutomationMainWindow.h"

Command::Command(Glib::ustring commandStr_, FMidiAutomationMainWindow *window_)/*{{{*/
{
    window = window_;
    commandStr = commandStr_;
}//constructor/*}}}*/

CommandManager &CommandManager::Instance()/*{{{*/
{
    static CommandManager manager;
    return manager;
}//Instance/*}}}*/

void CommandManager::setTitleStar(std::function<void (void)> titleStarFunc_)/*{{{*/
{
    titleStarFunc = titleStarFunc_;
}//setTitleStar/*}}}*/

void CommandManager::setMenuItems(Gtk::ImageMenuItem *menuUndo_, Gtk::ImageMenuItem *menuRedo_)/*{{{*/
{
    menuUndo = menuUndo_;
    menuRedo = menuRedo_;

    menuUndo->set_label("Undo");
    menuRedo->set_label("Redo");

    menuUndo->set_sensitive(false);
    menuRedo->set_sensitive(false);
}//setMenuItems/*}}}*/

void CommandManager::doRedo()/*{{{*/
{
    if (redoStack.empty() == true) {
        return;
    }//if

    std::shared_ptr<Command> command = redoStack.top();
    redoStack.pop();

    if (redoStack.empty() == true) {
        menuRedo->set_label("Redo");
        menuRedo->set_sensitive(false);
    } else {
        menuRedo->set_label("Redo " + redoStack.top()->commandStr);
    }//if

    undoStack.push(command);
    menuUndo->set_sensitive(true);
    menuUndo->set_label("Undo " + command->commandStr);

    command->doAction();

    command->window->queue_draw();
}//doRedo/*}}}*/

void CommandManager::doUndo()/*{{{*/
{
    if (undoStack.empty() == true) {
        return;
    }//if

    std::shared_ptr<Command> command = undoStack.top();
    undoStack.pop();

    if (undoStack.empty() == true) {
        menuUndo->set_label("Undo");
        menuUndo->set_sensitive(false);
    } else {
        menuUndo->set_label("Undo " + undoStack.top()->commandStr);
    }//if

    redoStack.push(command);
    menuRedo->set_sensitive(true);
    menuRedo->set_label("Redo " + command->commandStr);

    command->undoAction();

    command->window->queue_draw();
}//doUndo/*}}}*/

void CommandManager::setNewCommand(std::shared_ptr<Command> command, bool applyCommand)/*{{{*/
{
    while (redoStack.empty() == false) {
        redoStack.pop();
    }//while

    menuRedo->set_label("Redo");
    menuRedo->set_sensitive(false);

    undoStack.push(command);
    menuUndo->set_sensitive(true);
    menuUndo->set_label("Undo " + command->commandStr);

    if (true == applyCommand) {
        command->doAction();
    }//if

    titleStarFunc();

    command->window->queue_draw();
}//setNewcommand/*}}}*/

//UpdateTempoChangeCommand
UpdateTempoChangeCommand::UpdateTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar,/*{{{*/
                                                    unsigned int new_barSubDivisions, std::function<void (void)> updateTempoChangesUIData_,
                                                    FMidiAutomationMainWindow *window) : Command("Update Tempo Change", window)
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
}//undoAction/*}}}*/

//AddTempoChangeCommand
AddTempoChangeCommand::AddTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int tick_,/*{{{*/
                                                std::function<void (void)> updateTempoChangesUIData_, FMidiAutomationMainWindow *window) 
                                                : Command("Add Tempo Change", window)
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
}//undoAction/*}}}*/

//DeleteTempoChangeCommand
DeleteTempoChangeCommand::DeleteTempoChangeCommand(unsigned int tick_,/*{{{*/
                                                    std::function<void (void)> updateTempoChangesUIData_, FMidiAutomationMainWindow *window) 
                                                    : Command("Delete Tempo Change", window)
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
}//undoAction/*}}}*/

//ProcessRecordedMidiCommand
ProcessRecordedMidiCommand::ProcessRecordedMidiCommand(decltype(Sequencer::entries) &origEntryMap_, decltype(Sequencer::entries) &newEntryMap_,
                                                        FMidiAutomationMainWindow *window) : Command("Process Recorded Midi", window)/*{{{*/
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
}//undoAction/*}}}*/



