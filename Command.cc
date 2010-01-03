#include <gtkmm.h>
#include <libglademm.h>
#include "Command.h"
#include "Tempo.h"
#include "FMidiAutomationMainWindow.h"
#include "FMidiAutomationData.h"
#include "Sequencer.h"
#include "Animation.h"

extern FMidiAutomationMainWindow *mainWindow;

CommandManager &CommandManager::Instance()
{
    static CommandManager manager;
    return manager;
}//Instance

void CommandManager::setTitleStar(boost::function<void (void)> titleStarFunc_)
{
    titleStarFunc = titleStarFunc_;
}//setTitleStar

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

    mainWindow->queue_draw();
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

    mainWindow->queue_draw();
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

    titleStarFunc();

    mainWindow->queue_draw();
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

AddSequencerEntryCommand::AddSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer_, bool useDefaults_)
{
    sequencer = sequencer_;
    useDefaults = useDefaults_;
}//constructor

AddSequencerEntryCommand::~AddSequencerEntryCommand()
{
    //Nothing
}//destructor

void AddSequencerEntryCommand::doAction()
{
    if (entry == NULL) {
        entry = sequencer->addEntry(-1, useDefaults);
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

SequencerEntryUpCommand::SequencerEntryUpCommand(boost::shared_ptr<Sequencer> sequencer_, boost::shared_ptr<SequencerEntry> entry_)
{
    sequencer = sequencer_;
    entry = entry_;
    origIndex = entry->getIndex();
}//constructor

SequencerEntryUpCommand::~SequencerEntryUpCommand()
{
    //Nothing
}//destructor

void SequencerEntryUpCommand::doAction()
{
    sequencer->deleteEntry(entry);
    sequencer->addEntry(entry, origIndex - 1);
}//doAction

void SequencerEntryUpCommand::undoAction()
{
    sequencer->deleteEntry(entry);
    sequencer->addEntry(entry, origIndex);
}//undoAction

SequencerEntryDownCommand::SequencerEntryDownCommand(boost::shared_ptr<Sequencer> sequencer_, boost::shared_ptr<SequencerEntry> entry_)
{
    sequencer = sequencer_;
    entry = entry_;
    origIndex = entry->getIndex();
}//constructor

SequencerEntryDownCommand::~SequencerEntryDownCommand()
{
    //Nothing
}//destructor

void SequencerEntryDownCommand::doAction()
{
    sequencer->deleteEntry(entry);
    sequencer->addEntry(entry, origIndex + 1);
}//doAction

void SequencerEntryDownCommand::undoAction()
{
    sequencer->deleteEntry(entry);
    sequencer->addEntry(entry, origIndex);
}//undoAction

AddSequencerEntryBlockCommand::AddSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntry> entry_, boost::shared_ptr<SequencerEntryBlock> entryBlock_)
{
    entry = entry_;
    entryBlock = entryBlock_;
}//constructor

AddSequencerEntryBlockCommand::~AddSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void AddSequencerEntryBlockCommand::doAction()
{
    entry->addEntryBlock(entryBlock->getStartTick(), entryBlock);
}//doAction

void AddSequencerEntryBlockCommand::undoAction()
{
    entry->removeEntryBlock(entryBlock);
}//undoAction

DeleteSequencerEntryBlockCommand::DeleteSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_)
{
    entryBlock = entryBlock_;
    entry = entryBlock->getOwningEntry();
}//constructor

DeleteSequencerEntryBlockCommand::~DeleteSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void DeleteSequencerEntryBlockCommand::doAction()
{
    entry->removeEntryBlock(entryBlock);
}//doAction

void DeleteSequencerEntryBlockCommand::undoAction()
{
    entry->addEntryBlock(entryBlock->getStartTick(), entryBlock);
}//undoAction

ChangeSequencerEntryBlockPropertiesCommand::ChangeSequencerEntryBlockPropertiesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, Glib::ustring newTitle_)
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
}//undoAction

MoveSequencerEntryBlockCommand::MoveSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, int origTick_, int newTick_)
{
    entryBlock = entryBlock_;
    origTick = origTick_;
    newTick = newTick_;
}//constructor

MoveSequencerEntryBlockCommand::~MoveSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void MoveSequencerEntryBlockCommand::doAction()
{
    entryBlock->moveBlock(newTick);
}//doAction

void MoveSequencerEntryBlockCommand::undoAction()
{
    entryBlock->moveBlock(origTick);
}//undoAction

ChangeSequencerEntryPropertiesCommand::ChangeSequencerEntryPropertiesCommand(boost::shared_ptr<SequencerEntry> entry_, boost::shared_ptr<SequencerEntryImpl> origImpl_, boost::shared_ptr<SequencerEntryImpl> newImpl_)
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
}//undoAction

AddKeyframeCommand::AddKeyframeCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock_, int curMouseUnderTick_, int curMouseUnderValue_)
{
    currentlySelectedEntryBlock = currentlySelectedEntryBlock_;
    curMouseUnderTick = curMouseUnderTick_;
    curMouseUnderValue = curMouseUnderValue_;
}//constructor

AddKeyframeCommand::~AddKeyframeCommand()
{
    //Nothing
}//destructor

void AddKeyframeCommand::doAction()
{
    assert(currentlySelectedEntryBlock != NULL);
    //if ((currentlySelectedEntryBlock == NULL) || (mainWindow->getGraphState().displayMode != DisplayMode::Curve)) {
    //    return;
    //}//if

    boost::shared_ptr<Keyframe> newKeyframe(new Keyframe);

    newKeyframe->tick = curMouseUnderTick;
    newKeyframe->value = curMouseUnderValue;

    boost::shared_ptr<Animation> curve = currentlySelectedEntryBlock->getCurve();
    curve->addKey(newKeyframe);
}//doAction

void AddKeyframeCommand::undoAction()
{
    currentlySelectedEntryBlock->getCurve()->deleteKey(curMouseUnderTick);

    mainWindow->queue_draw();
}//undoAction

DeleteKeyframeCommand::DeleteKeyframeCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, boost::shared_ptr<Keyframe> keyframe_)
{
    entryBlock = entryBlock_;
    keyframe = keyframe_;
}//constructor

DeleteKeyframeCommand::~DeleteKeyframeCommand()
{
    //Nothing
}//destructor

void DeleteKeyframeCommand::doAction()
{
    entryBlock->getCurve()->deleteKey(keyframe);
}//doAction

void DeleteKeyframeCommand::undoAction()
{
    entryBlock->getCurve()->addKey(keyframe);
}//undoAction

MoveKeyframeCommand::MoveKeyframeCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, boost::shared_ptr<Keyframe> keyframe_, int movingKeyOrigTick_, double movingKeyOrigValue_)
{
    entryBlock = entryBlock_;
    keyframe = keyframe_;
    movingKeyOrigTick = movingKeyOrigTick_;
    movingKeyOrigValue = movingKeyOrigValue_;
}//constructor

MoveKeyframeCommand::~MoveKeyframeCommand()
{
    //Nothing
}//destructor

void MoveKeyframeCommand::doAction()
{
    entryBlock->getCurve()->deleteKey(keyframe);

    std::swap(keyframe->tick, movingKeyOrigTick);
    std::swap(keyframe->value, movingKeyOrigValue);

    entryBlock->getCurve()->addKey(keyframe);
}//doAction

void MoveKeyframeCommand::undoAction()
{
    doAction();
}//undoAction



