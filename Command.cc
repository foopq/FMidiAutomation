#include <gtkmm.h>
#include <libglademm.h>
#include "Command.h"
#include "Tempo.h"
#include "FMidiAutomationMainWindow.h"
#include "FMidiAutomationData.h"
#include "Sequencer.h"
#include "Animation.h"
#include <boost/foreach.hpp>

extern FMidiAutomationMainWindow *mainWindow;

Command::Command(Glib::ustring commandStr_)/*{{{*/
{
    commandStr = commandStr_;
}//constructor/*}}}*/

CommandManager &CommandManager::Instance()/*{{{*/
{
    static CommandManager manager;
    return manager;
}//Instance/*}}}*/

void CommandManager::setTitleStar(boost::function<void (void)> titleStarFunc_)/*{{{*/
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

    boost::shared_ptr<Command> command = redoStack.top();
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

    mainWindow->queue_draw();
}//doRedo/*}}}*/

void CommandManager::doUndo()/*{{{*/
{
    if (undoStack.empty() == true) {
        return;
    }//if

    boost::shared_ptr<Command> command = undoStack.top();
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

    mainWindow->queue_draw();
}//doUndo/*}}}*/

void CommandManager::setNewCommand(boost::shared_ptr<Command> command, bool applyCommand)/*{{{*/
{
    while (redoStack.empty() == false) {
        redoStack.pop();
    }//while

    menuRedo->set_sensitive(false);

    undoStack.push(command);
    menuUndo->set_sensitive(true);
    menuUndo->set_label("Undo " + command->commandStr);

    if (true == applyCommand) {
        command->doAction();
    }//if

    titleStarFunc();

    mainWindow->queue_draw();
}//setNewcommand/*}}}*/

//UpdateTempoChangeCommand
UpdateTempoChangeCommand::UpdateTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar,/*{{{*/
                                                    unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData_) : Command("Update Tempo Change")
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
AddTempoChangeCommand::AddTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int tick_,/*{{{*/
                                                boost::shared_ptr<FMidiAutomationData> datas_,
                                                boost::function<void (void)> updateTempoChangesUIData_) : Command("Add Tempo Change")
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
}//undoAction/*}}}*/

//DeleteTempoChangeCommand
DeleteTempoChangeCommand::DeleteTempoChangeCommand(unsigned int tick_,/*{{{*/
                                                    boost::shared_ptr<FMidiAutomationData> datas_,
                                                    boost::function<void (void)> updateTempoChangesUIData_) : Command("Delete Tempo Change")
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
}//undoAction/*}}}*/

//AddSequencerEntryCommand
AddSequencerEntryCommand::AddSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer_, bool useDefaults_) : Command("Add Sequencer Entry")/*{{{*/
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
}//undoAction/*}}}*/

//DeleteSequencerEntryCommand
DeleteSequencerEntryCommand::DeleteSequencerEntryCommand(boost::shared_ptr<Sequencer> sequencer_, boost::shared_ptr<SequencerEntry> entry_) : Command("Delete Sequencer Entry")/*{{{*/
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
}//undoAction/*}}}*/

//SequencerEntryUpCommand
SequencerEntryUpCommand::SequencerEntryUpCommand(boost::shared_ptr<Sequencer> sequencer_, boost::shared_ptr<SequencerEntry> entry_) : Command("Sequencer Entry Up")/*{{{*/
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
}//undoAction/*}}}*/

//SequencerEntryDownCommand
SequencerEntryDownCommand::SequencerEntryDownCommand(boost::shared_ptr<Sequencer> sequencer_, boost::shared_ptr<SequencerEntry> entry_) : Command("Sequencer Entry Down")/*{{{*/
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
}//undoAction/*}}}*/

//AddSequencerEntryBlockCommand
AddSequencerEntryBlockCommand::AddSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntry> entry_, boost::shared_ptr<SequencerEntryBlock> entryBlock_) : Command("Add Sequencer Entry Block")/*{{{*/
{
    entry = entry_;
    entryBlock = entryBlock_;


    std::cout << "add entry block: " << entryBlock->getStartTick() << " - " << *entryBlock->getRawStartTick() << std::endl;
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
}//undoAction/*}}}*/

//DeleteSequencerEntryBlockCommand
DeleteSequencerEntryBlockCommand::DeleteSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_) : Command("Delete Sequencer Entry Block")/*{{{*/
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
}//undoAction/*}}}*/

//DeleteSequencerEntryBlocksCommand
DeleteSequencerEntryBlocksCommand::DeleteSequencerEntryBlocksCommand(std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > &entryBlocks_) : Command("Delete Sequencer Entry Blocks")/*{{{*/
{
    entryBlocks = entryBlocks_;
}//constructor

DeleteSequencerEntryBlocksCommand::~DeleteSequencerEntryBlocksCommand()
{
    //Nothing
}//destructor

void DeleteSequencerEntryBlocksCommand::doAction()
{
    for (std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator blockIter = entryBlocks.begin(); blockIter != entryBlocks.end(); ++blockIter) {
        boost::shared_ptr<SequencerEntryBlock> entryBlock = blockIter->second;
        boost::shared_ptr<SequencerEntry> entry = entryBlock->getOwningEntry();

        entry->removeEntryBlock(entryBlock);
    }//for
}//doAction

void DeleteSequencerEntryBlocksCommand::undoAction()
{
    for (std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator blockIter = entryBlocks.begin(); blockIter != entryBlocks.end(); ++blockIter) {
        boost::shared_ptr<SequencerEntryBlock> entryBlock = blockIter->second;
        boost::shared_ptr<SequencerEntry> entry = entryBlock->getOwningEntry();

        entry->addEntryBlock(entryBlock->getStartTick(), entryBlock);
    }//for
}//undoAction/*}}}*/

//ChangeSequencerEntryBlockPropertiesCommand
ChangeSequencerEntryBlockPropertiesCommand::ChangeSequencerEntryBlockPropertiesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, Glib::ustring newTitle_) : Command("Change Sequencer Entry Block Properties")/*{{{*/
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
                                                                std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > &entryBlocks_,
                                                                std::map<boost::shared_ptr<SequencerEntryBlock>, int> &entryOriginalStartTicks_,
                                                                std::map<boost::shared_ptr<SequencerEntryBlock>, int> &entryNewStartTicks_) : Command("Move Sequencer Entry Block")
{
    entryBlocks = entryBlocks_;

    entryOriginalStartTicks = entryOriginalStartTicks_;
    entryNewStartTicks = entryNewStartTicks_;

std::cout << "move entry block: " << std::endl;

for (std::map<boost::shared_ptr<SequencerEntryBlock>, int>::iterator mapIter = entryOriginalStartTicks.begin(); mapIter != entryOriginalStartTicks.end(); ++mapIter) {
    std::cout << "entryOriginalStartTicks: " << mapIter->second << std::endl;
}//for

for (std::map<boost::shared_ptr<SequencerEntryBlock>, int>::iterator mapIter = entryNewStartTicks.begin(); mapIter != entryNewStartTicks.end(); ++mapIter) {
    std::cout << "entryNewStartTicks: " << mapIter->second << std::endl;
}//for

}//constructor

MoveSequencerEntryBlockCommand::~MoveSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void MoveSequencerEntryBlockCommand::doAction()
{
    std::cout << "MoveSequencerEntryBlockCommand::doAction()" << std::endl;

    for (std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator blockIter = entryBlocks.begin(); blockIter != entryBlocks.end(); ++blockIter) {
        boost::shared_ptr<SequencerEntryBlock> entryBlock = blockIter->second;
        entryBlock->moveBlock(entryNewStartTicks[entryBlock]);
    }//for
}//doAction

void MoveSequencerEntryBlockCommand::undoAction()
{
    std::cout << "MoveSequencerEntryBlockCommand::undoAction()" << std::endl;

    for (std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator blockIter = entryBlocks.begin(); blockIter != entryBlocks.end(); ++blockIter) {
        boost::shared_ptr<SequencerEntryBlock> entryBlock = blockIter->second;
        entryBlock->moveBlock(entryOriginalStartTicks[entryBlock]);
    }//for
}//undoAction/*}}}*/

//ChangeSequencerEntryPropertiesCommand
ChangeSequencerEntryPropertiesCommand::ChangeSequencerEntryPropertiesCommand(boost::shared_ptr<SequencerEntry> entry_, boost::shared_ptr<SequencerEntryImpl> origImpl_, boost::shared_ptr<SequencerEntryImpl> newImpl_) : Command("Change Sequencer Entry Properties")/*{{{*/
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

//AddKeyframesCommand
AddKeyframesCommand::AddKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock_, /*{{{*/
                                            std::multimap<int, boost::shared_ptr<Keyframe> > &origKeyframes, int newTick) : Command("Add Keyframe")
{
    //XXX: I suspect we need to sort out an offset from newTick to the first key and then offset the rest based on that..

    currentlySelectedEntryBlock = currentlySelectedEntryBlock_;
    assert(currentlySelectedEntryBlock != NULL);

    keyframes.clear();
    for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = origKeyframes.begin(); keyIter != origKeyframes.end(); ++keyIter) {
        boost::shared_ptr<Keyframe> origKeyframe = keyIter->second;
        boost::shared_ptr<Keyframe> newKeyframe(new Keyframe);

        *newKeyframe = *origKeyframe;
        newKeyframe->tick = newTick;
        newKeyframe->setSelectedState(KeySelectedType::NotSelected);

        keyframes.insert(std::make_pair(newKeyframe->tick, newKeyframe));
    }//for
}//constructor

AddKeyframesCommand::AddKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock_, int curMouseUnderTick_, int curMouseUnderValue_) : Command("Add Keyframe")
{
    currentlySelectedEntryBlock = currentlySelectedEntryBlock_;
    //curMouseUnderTick = curMouseUnderTick_;
    //curMouseUnderValue = curMouseUnderValue_;

    assert(currentlySelectedEntryBlock != NULL);
    //if ((currentlySelectedEntryBlock == NULL) || (mainWindow->getGraphState().displayMode != DisplayMode::Curve)) {
    //    return;
    //}//if

    boost::shared_ptr<Keyframe> newKeyframe(new Keyframe);

    newKeyframe->tick = curMouseUnderTick_;
    newKeyframe->value = curMouseUnderValue_;

    keyframes.clear();
    keyframes.insert(std::make_pair(newKeyframe->tick, newKeyframe));
}//constructor

AddKeyframesCommand::~AddKeyframesCommand()
{
    //Nothing
}//destructor

void AddKeyframesCommand::doAction()
{
    for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        boost::shared_ptr<Keyframe> keyframe = keyIter->second;
        currentlySelectedEntryBlock->getCurve()->addKey(keyframe);
    }//for
}//doAction

void AddKeyframesCommand::undoAction()
{
    for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        boost::shared_ptr<Keyframe> keyframe = keyIter->second;
        currentlySelectedEntryBlock->getCurve()->deleteKey(keyframe);
    }//for
}//undoAction/*}}}*/

//DeleteKeyframesCommand
DeleteKeyframesCommand::DeleteKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, /*{{{*/
                                                std::multimap<int, boost::shared_ptr<Keyframe> > &keyframes_) : Command("Delete Keyframe")
{
    entryBlock = entryBlock_;
    keyframes = keyframes_;
}//constructor

DeleteKeyframesCommand::~DeleteKeyframesCommand()
{
    //Nothing
}//destructor

void DeleteKeyframesCommand::doAction()
{
    for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        boost::shared_ptr<Keyframe> keyframe = keyIter->second;
        entryBlock->getCurve()->deleteKey(keyframe);
    }//for
}//doAction

void DeleteKeyframesCommand::undoAction()
{
    for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        boost::shared_ptr<Keyframe> keyframe = keyIter->second;
        entryBlock->getCurve()->addKey(keyframe);
    }//for
}//undoAction/*}}}*/

//MoveKeyframesCommand
MoveKeyframesCommand::MoveKeyframesCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_, /*{{{*/
                                            std::vector<boost::shared_ptr<KeyInfo> > &keyframes_) : Command("Move Keyframe")
{
    entryBlock = entryBlock_;
    keyframes = keyframes_;
}//constructor

MoveKeyframesCommand::~MoveKeyframesCommand()
{
    //Nothing
}//destructor

void MoveKeyframesCommand::doAction()
{
    BOOST_FOREACH (boost::shared_ptr<KeyInfo> keyframe, keyframes) {
        entryBlock->getCurve()->deleteKey(keyframe->keyframe);

        std::swap(keyframe->keyframe->tick, keyframe->movingKeyOrigTick);
        std::swap(keyframe->keyframe->value, keyframe->movingKeyOrigValue);

        entryBlock->getCurve()->addKey(keyframe->keyframe);
    }//foreach
}//doAction

void MoveKeyframesCommand::undoAction()
{
    doAction();
}//undoAction/*}}}*/

//ProcessRecordedMidiCommand
ProcessRecordedMidiCommand::ProcessRecordedMidiCommand(std::map<boost::shared_ptr<SequencerEntry>, int > origEntryMap_, std::map<boost::shared_ptr<SequencerEntry>, int > newEntryMap_) : Command("Process Recorded Midi")/*{{{*/
{
    origEntryMap = origEntryMap_;
    newEntryMap = newEntryMap_;
}//constructor

ProcessRecordedMidiCommand::~ProcessRecordedMidiCommand()
{
    //Nothing
}//destructor

void ProcessRecordedMidiCommand::doAction()
{
    Globals &globals = Globals::Instance();
    globals.sequencer->setEntryMap(newEntryMap);
}//doAction

void ProcessRecordedMidiCommand::undoAction()
{
    Globals &globals = Globals::Instance();
    globals.sequencer->setEntryMap(origEntryMap);
}//undoAction/*}}}*/


