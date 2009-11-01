#include <gtkmm.h>
#include <libglademm.h>
#include "Command.h"
#include "Tempo.h"
#include "FMidiAutomationMainWindow.h"
#include "FMidiAutomationData.h"
#include "Sequencer.h"

CommandManager &CommandManager::Instance()
{
    static CommandManager manager;
    return manager;
}//Instance

void CommandManager::setMenuItems(Gtk::ImageMenuItem *menuUndo_, Gtk::ImageMenuItem *menuRedo_)
{
    menuUndo = menuUndo_;
    menuRedo = menuRedo_;

    menuUndo->set_sensitive(false);
    menuRedo->set_sensitive(false);
}//setMenuItems

void CommandManager::doRedo()
{
    if (redoStack.empty() == true) {
        return;
    }//if

    boost::shared_ptr<Command> command = redoStack.top();
    redoStack.pop();

    if (redoStack.empty() == true) {
        menuRedo->set_sensitive(false);
    }//if

    undoStack.push(command);
    menuUndo->set_sensitive(true);

    command->doAction();    
}//doRedo

void CommandManager::doUndo()
{
    if (undoStack.empty() == true) {
        return;
    }//if

    boost::shared_ptr<Command> command = undoStack.top();
    undoStack.pop();

    if (undoStack.empty() == true) {
        menuUndo->set_sensitive(false);
    }//if

    redoStack.push(command);
    menuRedo->set_sensitive(true);

    command->undoAction();
}//doUndo

void CommandManager::setNewCommand(boost::shared_ptr<Command> command)
{
    while (redoStack.empty() == false) {
        redoStack.pop();
    }//while

    menuRedo->set_sensitive(false);

    undoStack.push(command);
    menuUndo->set_sensitive(true);

    command->doAction();
}//setNewcommand

//UpdateTempoChangeCommand
UpdateTempoChangeCommand::UpdateTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                                    unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData_)
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
AddTempoChangeCommand::AddTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int tick_, 
                                                boost::shared_ptr<FMidiAutomationData> datas_,
                                                boost::function<void (void)> updateTempoChangesUIData_)
{
    datas = datas_;
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
    assert(datas->tempoChanges.find(tick) == datas->tempoChanges.end());
    datas->addTempoChange(tick, tempo);
    updateTempoChangesUIData();
}//doAction

void AddTempoChangeCommand::undoAction()
{
    assert(datas->tempoChanges.find(tick) != datas->tempoChanges.end());
    datas->removeTempoChange(tick);
    updateTempoChangesUIData();
}//undoAction

//DeleteTempoChangeCommand
DeleteTempoChangeCommand::DeleteTempoChangeCommand(unsigned int tick_, 
                                                    boost::shared_ptr<FMidiAutomationData> datas_,
                                                    boost::function<void (void)> updateTempoChangesUIData_)
{
    datas = datas_;
    tick = tick_;
    updateTempoChangesUIData = updateTempoChangesUIData_;
}//constructor

DeleteTempoChangeCommand::~DeleteTempoChangeCommand()
{
    //Nothing
}//destructor

void DeleteTempoChangeCommand::doAction()
{
    assert(datas->tempoChanges.find(tick) != datas->tempoChanges.end());

    tempo = datas->tempoChanges[tick];

    datas->removeTempoChange(tick);

    updateTempoChangesUIData();
}//doAction

void DeleteTempoChangeCommand::undoAction()
{
    assert(datas->tempoChanges.find(tick) == datas->tempoChanges.end());
    datas->addTempoChange(tick, tempo);
    updateTempoChangesUIData();
}//undoAction

AddSequencerEntryCommand::AddSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer_)
{
    sequencer = sequencer_;
}//constructor

AddSequencerEntryCommand::~AddSequencerEntryCommand()
{
    //Nothing
}//destructor

void AddSequencerEntryCommand::doAction()
{
    if (entry == NULL) {
        entry = sequencer->addEntry(-1);
    } else {
        sequencer->addEntry(entry, -1);
    }//if
}//doAction

void AddSequencerEntryCommand::undoAction()
{
    sequencer->deleteEntry(entry);
}//undoAction

DeleteSequencerEntryCommand::DeleteSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer_, boost::shared_ptr<SequencerEntry> entry_)
{
    sequencer = sequencer_;
    entry = entry_;
}//constructor

DeleteSequencerEntryCommand::~DeleteSequencerEntryCommand()
{
    //Nothing
}//destructor

void DeleteSequencerEntryCommand::doAction()
{
    entryIndex = sequencer->getEntryIndex(entry);
    sequencer->deleteEntry(entry);
}//doAction

void DeleteSequencerEntryCommand::undoAction()
{
    sequencer->addEntry(entry, entryIndex);
}//undoAction



