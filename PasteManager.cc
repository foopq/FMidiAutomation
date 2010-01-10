#include "PasteManager.h"
#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include "Command.h"

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

void PasteManager::setNewCommand(boost::shared_ptr<PasteCommand> command_)
{
    command = command_;
    menuPaste->set_sensitive(true);

    if (false == pasteOnly) {
        menuPasteInstance->set_sensitive(true);
    }//if
}//setNewCommand

PasteSequencerEntryBlockCommand::PasteSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock_)
{
    entryBlock = entryBlock_;
}//constructor

PasteSequencerEntryBlockCommand::~PasteSequencerEntryBlockCommand()
{
    //Nothing
}//destructor

void PasteSequencerEntryBlockCommand::doPaste()
{
    Globals &globals = Globals::Instance();
    
    if (globals.graphState->displayMode != DisplayMode::Sequencer) {
        return;
    }//if

    boost::shared_ptr<SequencerEntry> selectedEntry = globals.sequencer->getSelectedEntry();
    if (selectedEntry != NULL) {
        if (selectedEntry->getEntryBlock(globals.graphState->curPointerTick) != NULL) {
            return;
        }//if

        boost::shared_ptr<SequencerEntryBlock> newentryBlock(new SequencerEntryBlock(selectedEntry, globals.graphState->curPointerTick, boost::shared_ptr<SequencerEntryBlock>()));
        newentryBlock->setTitle(entryBlock->getTitle());        

        newentryBlock->cloneCurves(entryBlock);
        boost::shared_ptr<Command> addSequencerEntryBlockCommand(new AddSequencerEntryBlockCommand(selectedEntry, newentryBlock));
        CommandManager::Instance().setNewCommand(addSequencerEntryBlockCommand);
    }//if
}//doPaste

void PasteSequencerEntryBlockCommand::doPasteInstance()
{
    Globals &globals = Globals::Instance();

    if (globals.graphState->displayMode != DisplayMode::Sequencer) {
        return;
    }//if
    
    boost::shared_ptr<SequencerEntry> selectedEntry = globals.sequencer->getSelectedEntry();
    if (selectedEntry != NULL) {
        if (selectedEntry->getEntryBlock(globals.graphState->curPointerTick) != NULL) {
            return;
        }//if

        boost::shared_ptr<SequencerEntryBlock> newentryBlock(new SequencerEntryBlock(selectedEntry, globals.graphState->curPointerTick, entryBlock));
        newentryBlock->setTitle(entryBlock->getTitle() + " (Instance)");

        //FIXME: and clone the curves, etc...
        boost::shared_ptr<Command> addSequencerEntryBlockCommand(new AddSequencerEntryBlockCommand(selectedEntry, newentryBlock));
        CommandManager::Instance().setNewCommand(addSequencerEntryBlockCommand);
    }//if
}//doPasteInstance


