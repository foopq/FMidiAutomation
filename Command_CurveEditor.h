/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __COMMAND_CURVEEDITOR_H
#define __COMMAND_CURVEEDITOR_H

#include <memory>
#include <functional>
#include <stack>
#include "Data/Sequencer.h"
#include "Command_Other.h"

class SequencerEntryBlock;
struct Keyframe;
class FMidiAutomationMainWindow;

struct AddKeyframesCommand : public Command
{
    AddKeyframesCommand(std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, int curMouseUnderTick, int curMouseUnderValue,
                            FMidiAutomationMainWindow *window);
    AddKeyframesCommand(std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock, std::map<int, std::shared_ptr<Keyframe> > &origKeyframes, int newTick,
                            FMidiAutomationMainWindow *window);
    virtual ~AddKeyframesCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock;
    std::map<int, std::shared_ptr<Keyframe> > keyframes;
};//AddKeyframesCommand

struct DeleteKeyframesCommand : public Command
{
    DeleteKeyframesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock, std::map<int, std::shared_ptr<Keyframe> > keyframes, FMidiAutomationMainWindow *window);
    virtual ~DeleteKeyframesCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    std::map<int, std::shared_ptr<Keyframe> > keyframes;
};//DeleteKeyframesCommand

struct MoveKeyframesCommand : public Command
{
    struct KeyInfo
    {
        std::shared_ptr<Keyframe> keyframe;
        int movingKeyOrigTick;
        double movingKeyOrigValue;
    };//KeyInfo

    MoveKeyframesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock, std::vector<std::shared_ptr<KeyInfo> > &keyframes, FMidiAutomationMainWindow *window);
    virtual ~MoveKeyframesCommand();

    void doAction();
    void undoAction();

private:
    std::shared_ptr<SequencerEntryBlock> entryBlock;
    std::vector<std::shared_ptr<KeyInfo> > keyframes;
};//MoveKeyframesCommand

#endif
