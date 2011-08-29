/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/



#ifndef __PASTEMANAGER_H
#define __PASTEMANAGER_H

#include <memory>
#include <boost/function.hpp>
#include <stack>
#include <gtkmm.h>

struct Tempo;
struct FMidiAutomationData;
class Sequencer;
class SequencerEntry;
class SequencerEntryBlock;
struct Keyframe;

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

    std::shared_ptr<PasteCommand> command;

public:
    static PasteManager &Instance();

    PasteManager();

    void setMenuItems(Gtk::ImageMenuItem *menuPaste, Gtk::ImageMenuItem *menuPasteInstance);
    void setPasteOnly(bool pasteOnly);
    void clearCommand();

    void doPaste();
    void doPasteInstance();
    void setNewCommand(std::shared_ptr<PasteCommand> command);
};//PasteManager

struct PasteSequencerEntryBlocksCommand : public PasteCommand
{
    PasteSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks); //FIXME: This shouldn't be a copy, but a reference!
    ~PasteSequencerEntryBlocksCommand();

    void doPaste();
    void doPasteInstance();

private:    
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > entryBlocks;
};//PasteSequencerEntryBlocksCommand

struct PasteSequencerKeyframesCommand : public PasteCommand
{
    PasteSequencerKeyframesCommand(std::map<int, std::shared_ptr<Keyframe> > keyframes); //FIXME: This shouldn't be a copy, but a reference!
    ~PasteSequencerKeyframesCommand();

    void doPaste();
    void doPasteInstance();

private:
    std::map<int, std::shared_ptr<Keyframe> > keyframes;
};//PasteSequencerKeyframesCommand


#endif

