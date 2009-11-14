
#ifndef __PASTEMANAGER_H
#define __PASTEMANAGER_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <stack>
#include <gtkmm.h>

struct Tempo;
struct FMidiAutomationData;
class Sequencer;
class SequencerEntry;
class SequencerEntryBlock;

struct PasteCommand
{
    virtual void doPaste() = 0;
    virtual void doPasteInstance() = 0;
};//PasteCommand

class PasteManager
{
    Gtk::ImageMenuItem *menuPaste;
    Gtk::ImageMenuItem *menuPasteInstance;
    bool pasteOnly;

    boost::shared_ptr<PasteCommand> command;

public:
    static PasteManager &Instance();

    PasteManager();

    void setMenuItems(Gtk::ImageMenuItem *menuPaste, Gtk::ImageMenuItem *menuPasteInstance);
    void setPasteOnly(bool pasteOnly);

    void doPaste();
    void doPasteInstance();
    void setNewCommand(boost::shared_ptr<PasteCommand> command);
};//PasteManager

struct PasteSequencerEntryBlockCommand : public PasteCommand
{
    boost::shared_ptr<SequencerEntryBlock> entryBlock;

    PasteSequencerEntryBlockCommand(boost::shared_ptr<SequencerEntryBlock> entryBlock);
    ~PasteSequencerEntryBlockCommand();

    void doPaste();
    void doPasteInstance();
};//PasteSequencerEntryBlockCommand

#endif

