/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __COMMAND_OTHER_H
#define __COMMAND_OTHER_H

#include <memory>
#include <functional>
#include <map>
#include <queue>
#include "Data/Sequencer.h"

struct Tempo;
class FMidiAutomationMainWindow;
enum class WindowMode : char;

enum class CommandFilter : char
{
    SequencerOnly,
    CurveEditorOnly,
    Both,
    BothMainWindowOnly
};//ComandFilter


struct Command
{
    Command(Glib::ustring commandStr, FMidiAutomationMainWindow *window, CommandFilter commandFilter);
    virtual ~Command() {};

    Glib::ustring commandStr;
    FMidiAutomationMainWindow *window;
    CommandFilter commandFilter;

    virtual void doAction() = 0;
    virtual void undoAction() = 0;
};//Command

struct OrderedCommandPairComparator : public std::binary_function<std::pair<int, std::shared_ptr<Command>> &, 
                                                                  std::pair<int, std::shared_ptr<Command>> &, bool>
{
    bool operator()(const std::pair<int, std::shared_ptr<Command>> &a, const std::pair<int, std::shared_ptr<Command>> &b) const
    {
        return a.first < b.first;
    }//operator()
};//OrderedCommandPairComparator

class CommandQueue
{
    typedef std::priority_queue<std::pair<int, std::shared_ptr<Command>>, std::vector<std::pair<int, std::shared_ptr<Command>>>, OrderedCommandPairComparator> CommandQueueType;

    CommandQueueType sequencerQueue;
    CommandQueueType curveEditorQueue;
    CommandQueueType bothQueue;
    CommandQueueType bothMainWindowOnlyQueue;

    int commandCount;

public:
    CommandQueue();

    std::shared_ptr<Command> getNextCommand(FMidiAutomationMainWindow *window);
    std::shared_ptr<Command> getNextCommandNoRemove(FMidiAutomationMainWindow *window);

    void clear();
    void addNewCommand(std::shared_ptr<Command> command);
};//CommandQueue

class CommandManager
{
    CommandQueue undoStack;
    CommandQueue redoStack;
    std::map<FMidiAutomationMainWindow *, Gtk::ImageMenuItem *> undoMenuMap;
    std::map<FMidiAutomationMainWindow *, Gtk::ImageMenuItem *> redoMenuMap;
    std::function<void (void)> titleStarFunc;

public:
    static CommandManager &Instance();

    void setTitleStar(std::function<void (void)> titleStarFunc);
    void updateUndoRedoMenus();
    void clearCommands();

    void registerMenuItems(FMidiAutomationMainWindow *window, Gtk::ImageMenuItem *menuUndo, Gtk::ImageMenuItem *menuRedo);
    void unregisterMenuItems(FMidiAutomationMainWindow *window);

    void doRedo(FMidiAutomationMainWindow *window);
    void doUndo(FMidiAutomationMainWindow *window);
    void setNewCommand(std::shared_ptr<Command> command, bool applyCommand);
};//CommandManager

struct AddTempoChangeCommand : public Command
{
    AddTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int tick_, std::function<void (void)> updateTempoChangesUIData,
                            FMidiAutomationMainWindow *window);
    virtual ~AddTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    unsigned int tick;
    std::function<void (void)> updateTempoChangesUIData;
};//AddTempoChangeCommand

struct DeleteTempoChangeCommand : public Command
{
    DeleteTempoChangeCommand(unsigned int tick_, std::function<void (void)> updateTempoChangesUIData, FMidiAutomationMainWindow *window);
    virtual ~DeleteTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    unsigned int tick;
    std::function<void (void)> updateTempoChangesUIData;
};//DeleteTempoChangeCommand

struct UpdateTempoChangeCommand : public Command
{
    UpdateTempoChangeCommand(std::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                unsigned int new_barSubDivisions, std::function<void (void)> updateTempoChangesUIData, FMidiAutomationMainWindow *window);
    virtual ~UpdateTempoChangeCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<Tempo> tempo;
    std::function<void (void)> updateTempoChangesUIData;
    
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
