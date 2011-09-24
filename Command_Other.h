/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __COMMAND_OTHER_H
#define __COMMAND_OTHER_H

#include <memory>
#include <boost/function.hpp>
#include <stack>
#include "Data/Sequencer.h"

struct Tempo;
class FMidiAutomationMainWindow;

struct Command
{
    Command(Glib::ustring commandStr, FMidiAutomationMainWindow *window);
    virtual ~Command() {};

    Glib::ustring commandStr;
    FMidiAutomationMainWindow *window;

    virtual void doAction() = 0;
    virtual void undoAction() = 0;
};//Command

class CommandManager
{
    std::stack<std::shared_ptr<Command> > undoStack;
    std::stack<std::shared_ptr<Command> > redoStack;
    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    boost::function<void (void)> titleStarFunc;

public:
    static CommandManager &Instance();

    void setTitleStar(boost::function<void (void)> titleStarFunc);

    void setMenuItems(Gtk::ImageMenuItem *menuUndo, Gtk::ImageMenuItem *menuRedo);

    void doRedo();
    void doUndo();
    void setNewCommand(std::shared_ptr<Command> command, bool applyCommand);
};//CommandManager

struct AddTempoChangeCommand : public Command
{
    AddTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int tick_, boost::function<void (void)> updateTempoChangesUIData,
                            FMidiAutomationMainWindow *window);
    virtual ~AddTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;
};//AddTempoChangeCommand

struct DeleteTempoChangeCommand : public Command
{
    DeleteTempoChangeCommand(unsigned int tick_, boost::function<void (void)> updateTempoChangesUIData, FMidiAutomationMainWindow *window);
    virtual ~DeleteTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;
};//DeleteTempoChangeCommand

struct UpdateTempoChangeCommand : public Command
{
    UpdateTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData, FMidiAutomationMainWindow *window);
    virtual ~UpdateTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    boost::function<void (void)> updateTempoChangesUIData;
    
    unsigned int old_bpm; //times 100
    unsigned int old_beatsPerBar;
    unsigned int old_barSubDivisions;   
};//AddTempoChangeCommand

struct ProcessRecordedMidiCommand : public Command
{
    ProcessRecordedMidiCommand(decltype(Sequencer::entries) &origEntryMap, decltype(Sequencer::entries) &newEntryMap, FMidiAutomationMainWindow *window);
    virtual ~ProcessRecordedMidiCommand();

    void doAction();
    void undoAction();

private:
    decltype(Sequencer::entries) origEntryMap;
    decltype(Sequencer::entries) newEntryMap;

    std::map<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntry>> oldNewMap;
    std::map<std::shared_ptr<SequencerEntry>, std::shared_ptr<SequencerEntry>> newOldMap;
};//ProcessRecordedMidiCommand

#endif
