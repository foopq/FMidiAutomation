/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/



#ifndef __PASTEMANAGER_H
#define __PASTEMANAGER_H

#include <memory>
#include <functional>
#include <boost/any.hpp>
#include <stack>
#include <gtkmm.h>

struct Tempo;
struct FMidiAutomationData;
class Sequencer;
class SequencerEntry;
class SequencerEntryBlock;
class SequencerEntryBlockUI;
struct Keyframe;
class FMidiAutomationMainWindow;

struct PasteCommand
{
    Glib::ustring pasteStr;

    PasteCommand(const Glib::ustring &pasteStr);

    virtual void doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow) = 0;
    virtual void doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow) = 0;
};//PasteCommand

class PasteManager
{
    std::map<FMidiAutomationMainWindow *, Gtk::ImageMenuItem *> menuPasteMap;
    std::map<FMidiAutomationMainWindow *, Gtk::ImageMenuItem *> menuPasteInstanceMap;
    std::map<FMidiAutomationMainWindow *, Gtk::MenuItem *> pasteBlocksToEntryMap;
    std::map<FMidiAutomationMainWindow *, Gtk::MenuItem *> pasteInstanceBlocksToEntryMap;

    std::shared_ptr<PasteCommand> sequencerPasteCommand;
    std::shared_ptr<PasteCommand> curveEditorPasteCommand;

public:
    static PasteManager &Instance();

    PasteManager();

    void registerMenuItems(FMidiAutomationMainWindow *window, Gtk::ImageMenuItem *menuPaste, Gtk::ImageMenuItem *menuPasteInstance,
                            Gtk::MenuItem *pasteBlocksToEntry, Gtk::MenuItem *pasteInstanceBlocksToEntry);
    void unregisterMenuItems(FMidiAutomationMainWindow *window);
    void clearCommands();
    void updateMenus();

    void doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow);
    void doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow);
    void setNewCommand(FMidiAutomationMainWindow *window, std::shared_ptr<PasteCommand> command);
};//PasteManager

struct PasteSequencerEntryBlocksCommand : public PasteCommand
{
    PasteSequencerEntryBlocksCommand(std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks); //FIXME: This shouldn't be a copy, but a reference!
    ~PasteSequencerEntryBlocksCommand();

    void doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow);
    void doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow);

private:    
    std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > entryBlocks;
};//PasteSequencerEntryBlocksCommand

struct PasteSequencerKeyframesCommand : public PasteCommand
{
    PasteSequencerKeyframesCommand(std::map<int, std::shared_ptr<Keyframe> > keyframes); //FIXME: This shouldn't be a copy, but a reference!
    ~PasteSequencerKeyframesCommand();

    void doPaste(boost::any contextData, FMidiAutomationMainWindow *targetWindow);
    void doPasteInstance(boost::any contextData, FMidiAutomationMainWindow *targetWindow);

private:
    std::map<int, std::shared_ptr<Keyframe> > keyframes;
};//PasteSequencerKeyframesCommand


#endif

