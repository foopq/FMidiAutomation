#ifndef __COMMAND_H
#define __COMMAND_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <stack>

struct Tempo;
struct FMidiAutomationData;

struct Command
{
    virtual void doAction() = 0;
    virtual void undoAction() = 0;
};//Command

class CommandManager
{
    std::stack<boost::shared_ptr<Command> > undoStack;
    std::stack<boost::shared_ptr<Command> > redoStack;
    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;

public:
    static CommandManager &Instance();

    void setMenuItems(Gtk::ImageMenuItem *menuUndo, Gtk::ImageMenuItem *menuRedo);

    void doRedo();
    void doUndo();
    void setNewCommand(boost::shared_ptr<Command> command);
};//CommandManager

struct AddTempoChangeCommand : public Command
{
    boost::shared_ptr<FMidiAutomationData> datas;
    boost::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;

    AddTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int tick_, boost::shared_ptr<FMidiAutomationData> datas_,
                            boost::function<void (void)> updateTempoChangesUIData);
    ~AddTempoChangeCommand();

    void doAction();
    void undoAction();
};//AddTempoChangeCommand

struct DeleteTempoChangeCommand : public Command
{
    boost::shared_ptr<FMidiAutomationData> datas;
    boost::shared_ptr<Tempo> tempo;
    unsigned int tick;
    boost::function<void (void)> updateTempoChangesUIData;

    DeleteTempoChangeCommand(unsigned int tick_, boost::shared_ptr<FMidiAutomationData> datas_, boost::function<void (void)> updateTempoChangesUIData);
    ~DeleteTempoChangeCommand();

    void doAction();
    void undoAction();
};//DeleteTempoChangeCommand

struct UpdateTempoChangeCommand : public Command
{
    boost::shared_ptr<Tempo> tempo;
    boost::function<void (void)> updateTempoChangesUIData;
    
    unsigned int old_bpm; //times 100
    unsigned int old_beatsPerBar;
    unsigned int old_barSubDivisions;   

    UpdateTempoChangeCommand(boost::shared_ptr<Tempo> tempo_, unsigned int new_bpm, unsigned int new_beatsPerBar, 
                                unsigned int new_barSubDivisions, boost::function<void (void)> updateTempoChangesUIData);
    ~UpdateTempoChangeCommand();

    void doAction();
    void undoAction();
};//AddTempoChangeCommand

#endif