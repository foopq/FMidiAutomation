/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include "SequencerEntry.h"
#include "FMidiAutomationCurveEditor.h"
#include "Animation.h"
#include "Command.h"
#include "GraphState.h"
#include "FMidiAutomationData.h"
#include "Tempo.h"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

namespace
{

//This is a copy... any edits need to also go into all the copies    
void handleGraphTimeScroll(gdouble xPos, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaWidth)
{
    gdouble offsetX = -(xPos - mousePressDownX);
//    gdouble offsetY = -(event->y - mousePressDownY);

    if ((offsetX < 0) && (graphState.zeroithTickPixel != std::numeric_limits<int>::max()) && (graphState.zeroithTickPixel >= (drawingAreaWidth/2))) {       
        return;
    }//if

    gdouble curOffset = graphState.offsetX;
    graphState.offsetX = graphState.baseOffsetX + offsetX;

    int tickCountStart = 0 * graphState.ticksPerPixel + graphState.offsetX * graphState.ticksPerPixel;
    int tickCountEnd = drawingAreaWidth * graphState.ticksPerPixel + graphState.offsetX * graphState.ticksPerPixel;

    if ((tickCountStart < 0) && (tickCountEnd > 0)) {
        int tickCountMiddle = (drawingAreaWidth / 2) * graphState.ticksPerPixel + graphState.offsetX * graphState.ticksPerPixel;
        if (tickCountMiddle < 0) {
            graphState.offsetX = curOffset;
        }//if
    }//if
}//handleGraphTimeScroll

bool handleGraphValueScroll(gdouble yPos, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaHeight)
{
    gdouble offsetY = yPos - mousePressDownY;

    double newOffset = graphState.baseOffsetY + offsetY;
    double medianValue = (drawingAreaHeight-60) / 2.0 * graphState.valuesPerPixel + newOffset * graphState.valuesPerPixel;

    const std::shared_ptr<SequencerEntryImpl> entryImpl = graphState.entryBlockSelectionState.GetFirstEntryBlock()->getOwningEntry()->getImpl();
    double minValue = entryImpl->minValue;
    double maxValue = entryImpl->maxValue;

    if ((medianValue < minValue) || (medianValue > maxValue)) {
        return false;
    }//if

    graphState.offsetY = graphState.baseOffsetY + offsetY;
    return true;
}//handleGraphValueScroll

void handleKeyScroll(gdouble xPos, gdouble yPos, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaWidth, int drawingAreaHeight)
{
    if (graphState.keyframeSelectionState.HasSelected() == false) {
        //Should be impossible
        return;
    }//if

    int eventX = std::max(0, (int)xPos);
    eventX = std::min(eventX, drawingAreaWidth-1);
    int eventY = std::max(60, (int)yPos);
    eventY = std::min(eventY, drawingAreaHeight);
    eventY -= 60;

    //int newTick = graphState.verticalPixelTickValues[eventX];
    //if (graphState.zeroithTickPixel != std::numeric_limits<int>::max()) {
    //    newTick = std::max(newTick, graphState.verticalPixelTickValues[graphState.zeroithTickPixel+1]);
    //}//if
    //double newValue = graphState.horizontalPixelValues[eventY];

    //newValue = std::max((int)newValue, graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl()->minValue);
    //newValue = std::min((int)newValue, graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl()->maxValue);

/*    
    //This checks to ensure we don't blow away another key when moving the current one??
    if ( ((newTick - graphState.getCurrentlySelectedEntryBlock()->getStartTick()) != graphState.currentlySelectedKeyframes.begin()->second->tick) && 
         (graphState.getCurrentlySelectedEntryBlock()->getCurve()->getKeyframeAtTick(newTick) != NULL) ) {
        return;
    }//if
*/    

    int curX = xPos;
    curX = std::max(0, curX);
    curX = std::min(curX, drawingAreaWidth-1);
    int diffTick = graphState.verticalPixelTickValues[curX] - graphState.verticalPixelTickValues[mousePressDownX];

    int curY = yPos;
    curY = std::max(60, curY);
    curY = std::min(curY, drawingAreaHeight-1);
    double diffValue = graphState.horizontalPixelValues[curY] - graphState.horizontalPixelValues[mousePressDownY];

//int tickOffset = (newTick - graphState.getCurrentlySelectedEntryBlock()->getStartTick()) - graphState.currentlySelectedKeyframes.begin()->second->tick;
//double valueOffset = newValue - graphState.currentlySelectedKeyframes.begin()->second->value;

//std::cout << "diffTick: " << diffTick << " - tickOffset: " << tickOffset << std::endl;
//std::cout << "diffValue: " << diffValue << " - valueOffset: " << valueOffset << std::endl;

    std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = graphState.entryBlockSelectionState.GetFirstEntryBlock();

    //Prevent dragging first key before start of entry block
    if (graphState.keyframeSelectionState.GetOrigTick(graphState.keyframeSelectionState.GetFirstKeyframe()) + diffTick + currentlySelectedEntryBlock->getStartTick() < currentlySelectedEntryBlock->getStartTick()) {
        std::cout << "exit 1: " << graphState.keyframeSelectionState.GetOrigTick(graphState.keyframeSelectionState.GetFirstKeyframe()) << " - " 
                                << graphState.keyframeSelectionState.GetOrigTick(graphState.keyframeSelectionState.GetFirstKeyframe()) + diffTick << std::endl;
        return;
    }//if

    std::map<int, std::shared_ptr<Keyframe> > updatedCurrentlySelectedKeyframes;
    BOOST_FOREACH (auto keyIter, graphState.keyframeSelectionState.GetCurrentlySelectedKeyframes()) {
        std::shared_ptr<Keyframe> curKeyframe = keyIter.second;

        int newTick = graphState.keyframeSelectionState.GetOrigTick(curKeyframe) + diffTick;

        double newValue = graphState.keyframeSelectionState.GetOrigValue(curKeyframe) + diffValue;
        newValue = std::max<double>(newValue, currentlySelectedEntryBlock->getOwningEntry()->getImpl()->minValue);
        newValue = std::min<double>(newValue, currentlySelectedEntryBlock->getOwningEntry()->getImpl()->maxValue);

        //if ( ((newTick - graphState.getCurrentlySelectedEntryBlock()->getStartTick()) != graphState.currentlySelectedKeyframes.begin()->second->tick) && 
        //    (graphState.getCurrentlySelectedEntryBlock()->getCurve()->getKeyframeAtTick(newTick) != NULL) ) {
        //    std::cout << "exit 2" << std::endl;
        //    continue;
        //}//if

        //curKeyframe->tick += tickOffset;
        //curKeyframe->value += valueOffset;

        //std::cout << "curTick: " << curKeyframe->tick << std::endl;

        //This checks to ensure we don't blow away another key when moving the current one
        if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(newTick + currentlySelectedEntryBlock->getStartTick()) == NULL) {
            currentlySelectedEntryBlock->getCurve()->deleteKey(curKeyframe);
            curKeyframe->tick = newTick;
            currentlySelectedEntryBlock->getCurve()->addKey(curKeyframe);

            //std::cout << " - newTick: " << curKeyframe->tick << std::endl;
        }//if

        curKeyframe->value = newValue;

        updatedCurrentlySelectedKeyframes[curKeyframe->tick] = curKeyframe;

        graphState.didMoveKey = true;
        graphState.didMoveKeyOutTangent = true;
        graphState.didMoveKeyInTangent = true;
    }//for

    graphState.keyframeSelectionState.SetCurrentlySelectedKeyframes(updatedCurrentlySelectedKeyframes);

//std::cout << std::endl << std::endl;    
}//handleKeyScroll

void handleKeyTangentScroll(gdouble xPos, gdouble yPos, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaWidth, int drawingAreaHeight)
{
    if (graphState.keyframeSelectionState.GetNumSelected() != 1) {
        return;
    }//if

    int eventX = std::max(0, (int)xPos);
    eventX = std::min(eventX, drawingAreaWidth-1);
    int eventY = std::max(60, (int)yPos);
    eventY = std::min(eventY, drawingAreaHeight);
    eventY -= 60;

    int newTick = graphState.verticalPixelTickValues[eventX] - graphState.entryBlockSelectionState.GetFirstEntryBlock()->getStartTick();
    double newValue = graphState.horizontalPixelValues[eventY];

    std::shared_ptr<Keyframe> curKeyframe = graphState.keyframeSelectionState.GetFirstKeyframe();

    if (SelectedEntity::InTangent == graphState.selectedEntity) {
        graphState.didMoveKeyInTangent = true;

        curKeyframe->inTangent[0] = curKeyframe->tick - newTick;
        curKeyframe->inTangent[1] = newValue - curKeyframe->value;
    } else {
        graphState.didMoveKeyOutTangent = true;

        curKeyframe->outTangent[0] = newTick - curKeyframe->tick;
        curKeyframe->outTangent[1] = newValue - curKeyframe->value;
    }//if
}//handleKeyTangentScroll



}//anonymous namespace

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasLMBPress()
{
    unsetAllCurveFrames();

    std::shared_ptr<Keyframe> selectedKey = curveEditor->getKeySelection(*graphState, mousePressDownX, mousePressDownY, ctrlCurrentlyPressed);

std::cout << "num selected keys: " << graphState->keyframeSelectionState.GetNumSelected() << " - " << ctrlCurrentlyPressed << std::endl;

    std::shared_ptr<Keyframe> firstKey;
    if (graphState->keyframeSelectionState.HasSelected() == true) {
        firstKey = graphState->keyframeSelectionState.GetFirstKeyframe();
    }//if

    curveEditor->setKeyUIValues(uiXml, firstKey);

    if (selectedKey != NULL) {
        graphState->didMoveKey = false;

        BOOST_FOREACH (auto keyIter, graphState->keyframeSelectionState.GetCurrentlySelectedKeyframes()) {
            graphState->keyframeSelectionState.AddOrigKeyframe(keyIter.second);
        }//for

        menuCopy->set_sensitive(true);
        menuCut->set_sensitive(true);
    } else {
        menuCopy->set_sensitive(false);
        menuCut->set_sensitive(false);
        graphState->doingRubberBanding = true;
    }//if

    //Essentially clear the selection state of the tempo changes
    (void)checkForTempoSelection(-100, datas->tempoChanges);

    if (true == graphState->doingRubberBanding) {
        graphState->keyframeSelectionState.ResetRubberbandingSelection();
    }//if
}//handleCurveEditorMainCanvasLMBPress

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasMMBPress()
{
    //Nothing
}//handleCurveEditorMainCanvasMMBPress

bool FMidiAutomationMainWindow::handleCurveEditorMainCanvasRMBPress(gdouble xPos, guint button, guint32 time)
{
    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("ContextMenu", "Context Menu"));

    Glib::ustring ui_info = "<ui><popup name='PopupMenu'></popup></ui>";

    curveEditor->getKeySelection(*graphState, mousePressDownX, mousePressDownY, ctrlCurrentlyPressed);

    std::shared_ptr<Keyframe> firstKeyframe;
    if (graphState->keyframeSelectionState.HasSelected() == true) {
        firstKeyframe = graphState->keyframeSelectionState.GetFirstKeyframe();
    }//if

    curveEditor->setKeyUIValues(uiXml, firstKeyframe);

    if (firstKeyframe == NULL) {
        graphState->selectedEntity = SelectedEntity::Nobody;

        std::string menuStr = "Add Keyframe";
        std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = graphState->entryBlockSelectionState.GetFirstEntryBlock();

        int curMouseUnderTick = graphState->verticalPixelTickValues[xPos];

        if (xPos > graphState->zeroithTickPixel) { 
            if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(curMouseUnderTick) != NULL) {
                menuStr = "Keyframe exists at this tick";
            }//if

            m_refActionGroup->add(Gtk::Action::create("ContextAddKeyframe", menuStr.c_str()), sigc::mem_fun(curveEditor.get(), &CurveEditor::handleAddKeyframe));
            ui_info =
                "<ui>"
                "  <popup name='PopupMenu'>"
                "    <menuitem action='ContextAddKeyframe'/>"
                "  </popup>"
                "</ui>";
        } else {
            return false;
        }//if
    } else {
        graphState->selectedEntity = SelectedEntity::KeyValue;

        m_refActionGroup->add(Gtk::Action::create("ContextDeleteKeyframe", "Delete Keyframes"), sigc::mem_fun(curveEditor.get(), &CurveEditor::handleDeleteKeyframes));
        m_refActionGroup->add(Gtk::Action::create("ContextResetTangents", "Reset Tangents"), sigc::mem_fun(curveEditor.get(), &CurveEditor::handleResetTangents));
        ui_info =
            "<ui>"
            "  <popup name='PopupMenu'>"
            "    <menuitem action='ContextResetTangents'/>"
            "    <separator/>"
            "    <menuitem action='ContextDeleteKeyframe'/>"
            "  </popup>"
            "</ui>";
    }//if

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);

    mainWindow->add_accel_group(m_refUIManager->get_accel_group());

    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try {
        m_refUIManager->add_ui_from_string(ui_info);
    } catch(const Glib::Error& ex) {
        std::cerr << "building menus failed: " <<  ex.what();
    } 
    #else
    std::auto_ptr<Glib::Error> ex;
    m_refUIManager->add_ui_from_string(ui_info, ex);
    if(ex.get()) {
        std::cerr << "building menus failed: " <<  ex->what();
    }
    #endif //GLIBMM_EXCEPTIONS_ENABLED

    m_pMenuPopup = dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu"));
    if(m_pMenuPopup != NULL) {
        m_pMenuPopup->show_all_children();
        m_pMenuPopup->popup(button, time);
    } else {
        g_warning("menu not found");
    }//if

    return true;
}//handleCurveEditorMainCanvasRMBPress

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasLMBRelease()
{
    graphState->doingRubberBanding = false;

    if (graphState->selectedEntity == SelectedEntity::KeyValue) {
        if (true == graphState->didMoveKey) {                        
            std::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = graphState->entryBlockSelectionState.GetFirstEntryBlock();

            //Move key back to where it was
            std::vector<std::shared_ptr<MoveKeyframesCommand::KeyInfo> > keyInfos;
            keyInfos.reserve(graphState->keyframeSelectionState.GetNumSelected());

            BOOST_FOREACH (auto keyIter, graphState->keyframeSelectionState.GetCurrentlySelectedKeyframes()) {
                std::shared_ptr<Keyframe> curKeyframe = keyIter.second;

                std::shared_ptr<MoveKeyframesCommand::KeyInfo> keyInfo(new MoveKeyframesCommand::KeyInfo);
                keyInfo->keyframe = curKeyframe;
                keyInfo->movingKeyOrigTick = curKeyframe->tick;
                keyInfo->movingKeyOrigValue = curKeyframe->value;
                keyInfos.push_back(keyInfo);

                currentlySelectedEntryBlock->getCurve()->deleteKey(curKeyframe);
                curKeyframe->tick = graphState->keyframeSelectionState.GetOrigTick(curKeyframe);
                curKeyframe->value = graphState->keyframeSelectionState.GetOrigValue(curKeyframe);
                currentlySelectedEntryBlock->getCurve()->addKey(curKeyframe);
            }//foreach

            std::shared_ptr<Command> moveKeyframeCommand(new MoveKeyframesCommand(currentlySelectedEntryBlock, keyInfos));
            CommandManager::Instance().setNewCommand(moveKeyframeCommand, true);
        }//if
    }//if

    if (graphState->keyframeSelectionState.HasSelected() == true) {
        menuCopy->set_sensitive(true);
        menuCut->set_sensitive(true);
    } else {
        menuCopy->set_sensitive(false);
        menuCut->set_sensitive(false);
    }//if
}//handleCurveEditorMainCanvasLMBRelease

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasMMBRelease()
{
    //Nothing
}//handleCurveEditorMainCanvasMMBRelease

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasRMBRelease()
{
    //Nothing
}//handleCurveEditorMainCanvasRMBRelease

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasMouseMove(gdouble xPos, gdouble yPos)
{
    if (true == shiftCurrentlyPressed) {
        //We are scrolling the canvas
        gdouble curOffsetX = graphState->offsetX;

        handleGraphTimeScroll(xPos, *graphState, mousePressDownX, mousePressDownY, drawingAreaWidth);
        bool didScroll = handleGraphValueScroll(yPos, *graphState, mousePressDownX, mousePressDownY, drawingAreaHeight);

        if ((graphState->offsetX != curOffsetX) || (true == didScroll)) {
            graphState->refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphState->refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
        }//if
    } else {
        std::shared_ptr<Keyframe> firstKeyframe;
        if (graphState->keyframeSelectionState.HasSelected() == true) {
            firstKeyframe = graphState->keyframeSelectionState.GetFirstKeyframe();
        }//if

        if (graphState->selectedEntity == SelectedEntity::KeyValue) {
            handleKeyScroll(xPos, yPos, *graphState, mousePressDownX, mousePressDownY, drawingAreaWidth, drawingAreaHeight);
            curveEditor->setKeyUIValues(uiXml, firstKeyframe);
        }//if

        else if ((graphState->selectedEntity == SelectedEntity::InTangent) || (graphState->selectedEntity == SelectedEntity::OutTangent)) {
            handleKeyTangentScroll(xPos, yPos, *graphState, mousePressDownX, mousePressDownY, drawingAreaWidth, drawingAreaHeight);
            curveEditor->setKeyUIValues(uiXml, firstKeyframe);
        }//if
    }//if

    if (true == graphState->doingRubberBanding) {
        curveEditor->updateSelectedKeyframesInRange(graphState->keyframeSelectionState, mousePressDownX, mousePressDownY, xPos, yPos,
                                                    drawingAreaWidth, drawingAreaHeight);
    }//if
}//handleSequencerCurveEditorMouseMove

