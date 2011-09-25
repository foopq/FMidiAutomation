/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include <gtkmm.h>
#include <libglademm.h>
#include "Command_CurveEditor.h"
#include "Data/FMidiAutomationData.h"
#include "Data/SequencerEntry.h"
#include "Animation.h"
#include "Globals.h"
#include "FMidiAutomationMainWindow.h"


//AddKeyframesCommand
AddKeyframesCommand::AddKeyframesCommand(std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock_, 
                                            std::map<int, std::shared_ptr<Keyframe> > &origKeyframes, int newTick,
                                            FMidiAutomationMainWindow *window) : Command("Add Keyframe", window, CommandFilter::CurveEditorOnly)
{
    if (origKeyframes.empty() == true) {
        return;
    }//if

    std::shared_ptr<Keyframe> firstKeyframe = origKeyframes.begin()->second;
    int firstKeyTick = firstKeyframe->tick;

    currentlySelectedEntryBlock = currentlySelectedEntryBlock_;
    assert(currentlySelectedEntryBlock != nullptr);

    keyframes.clear();
    for (std::map<int, std::shared_ptr<Keyframe> >::const_iterator keyIter = origKeyframes.begin(); keyIter != origKeyframes.end(); ++keyIter) {
        std::shared_ptr<Keyframe> origKeyframe = keyIter->second;
        std::shared_ptr<Keyframe> newKeyframe(new Keyframe);

        *newKeyframe = *origKeyframe;
        newKeyframe->tick = origKeyframe->tick - firstKeyTick + newTick;
        //newKeyframe->setSelectedState(KeySelectedType::NotSelected);

        keyframes.insert(std::make_pair(newKeyframe->tick, newKeyframe));
    }//for
}//constructor

AddKeyframesCommand::AddKeyframesCommand(std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock_, int curMouseUnderTick_, int curMouseUnderValue_,
                                            FMidiAutomationMainWindow *window) : Command("Add Keyframe", window, CommandFilter::CurveEditorOnly)
{
    currentlySelectedEntryBlock = currentlySelectedEntryBlock_;
    //curMouseUnderTick = curMouseUnderTick_;
    //curMouseUnderValue = curMouseUnderValue_;

    assert(currentlySelectedEntryBlock != nullptr);
    //if ((currentlySelectedEntryBlock == nullptr) || (mainWindow->getGraphState().displayMode != DisplayMode::Curve)) {
    //    return;
    //}//if

    std::shared_ptr<Keyframe> newKeyframe(new Keyframe);

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
    for (std::map<int, std::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        std::shared_ptr<Keyframe> keyframe = keyIter->second;
        currentlySelectedEntryBlock->getCurve()->addKey(keyframe);
    }//for
}//doAction

void AddKeyframesCommand::undoAction()
{
    for (std::map<int, std::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        std::shared_ptr<Keyframe> keyframe = keyIter->second;
        currentlySelectedEntryBlock->getCurve()->deleteKey(keyframe);
    }//for
}//undoAction

//DeleteKeyframesCommand
DeleteKeyframesCommand::DeleteKeyframesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock_, 
                                                std::map<int, std::shared_ptr<Keyframe> > keyframes_,
                                                FMidiAutomationMainWindow *window) : Command("Delete Keyframe", window, CommandFilter::CurveEditorOnly)
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
    for (std::map<int, std::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        std::shared_ptr<Keyframe> keyframe = keyIter->second;
        entryBlock->getCurve()->deleteKey(keyframe);
    }//for
}//doAction

void DeleteKeyframesCommand::undoAction()
{
    for (std::map<int, std::shared_ptr<Keyframe> >::const_iterator keyIter = keyframes.begin(); keyIter != keyframes.end(); ++keyIter) {
        std::shared_ptr<Keyframe> keyframe = keyIter->second;
        entryBlock->getCurve()->addKey(keyframe);
    }//for
}//undoAction

//MoveKeyframesCommand
MoveKeyframesCommand::MoveKeyframesCommand(std::shared_ptr<SequencerEntryBlock> entryBlock_, 
                                            std::vector<std::shared_ptr<KeyInfo> > &keyframes_,
                                            FMidiAutomationMainWindow *window) : Command("Move Keyframe", window, CommandFilter::CurveEditorOnly)
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
    for (std::shared_ptr<KeyInfo> keyframe : keyframes) {
        entryBlock->getCurve()->deleteKey(keyframe->keyframe);

        std::swap(keyframe->keyframe->tick, keyframe->movingKeyOrigTick);
        std::swap(keyframe->keyframe->value, keyframe->movingKeyOrigValue);

        entryBlock->getCurve()->addKey(keyframe->keyframe);
    }//foreach
}//doAction

void MoveKeyframesCommand::undoAction()
{
    doAction();
}//undoAction



