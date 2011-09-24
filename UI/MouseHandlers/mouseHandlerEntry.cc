/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationMainWindow.h"
#include "Data/Sequencer.h"
#include "UI/SequencerUI.h"
#include "Data/SequencerEntry.h"
#include "UI/SequencerEntryBlockUI.h"
#include "FMidiAutomationCurveEditor.h"
#include "Animation.h"
#include <boost/lexical_cast.hpp>
#include "GraphState.h"

namespace
{

enum class MouseRegion : char
{
    FrameRegion,
    MainCanvas,
    TickMarkerRegion,
    LeftValueRegion
};//MouseRegion

enum class MouseButton : char
{
    LeftButton = 1,
    MiddleButton = 2,
    RightButton = 3
};//MouseButton

MouseRegion determineMouseRegion(int x, int y, DisplayMode displayMode)
{
    if (y <= 30) {
        return MouseRegion::FrameRegion;
    }//if

    if (y <= 60) {
        return MouseRegion::TickMarkerRegion;
    }//if

    if (x > 60) {
        return MouseRegion::MainCanvas;
    }//if

    switch (displayMode) {        
        case DisplayMode::Curve:
            return MouseRegion::LeftValueRegion;

        case DisplayMode::Sequencer:
        default:
            return MouseRegion::MainCanvas;
    }//switch
}//determineMouseRegion

bool handleGraphValueZoom(GdkScrollDirection direction, GraphState &graphState, int drawingAreaHeight)
{
    bool changed = true;
    //double curValuesPerPixel = graphState->valuesPerPixel;

    const std::shared_ptr<SequencerEntryImpl> entryImpl = graphState.entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->getOwningEntry()->getImpl();
    int minValue = entryImpl->minValue;
    int maxValue = entryImpl->maxValue;

    int delta = maxValue - minValue;
    double origValuesPerPixel = (double)delta / (double)(drawingAreaHeight - 60);
    origValuesPerPixel *= 1.05;

    if (direction == GDK_SCROLL_UP) { //zoom in
        if ((graphState.horizontalPixelValues[0] - graphState.horizontalPixelValues[drawingAreaHeight-60-1]) < 10) {
            changed = false;
        } else {
            graphState.valuesPerPixel *= 0.8;
        }//if
    } else {
        if ((graphState.horizontalPixelValues[0] - graphState.horizontalPixelValues[drawingAreaHeight-60-1]) > delta * 2) {
            changed = false;
        } else {
            graphState.valuesPerPixel *= 1.2;
        }//if
    }//if

    if (true == changed) {
        double medianValue = graphState.horizontalPixelValues[graphState.horizontalPixelValues.size() / 2];
        graphState.setOffsetCenteredOnValue(medianValue, drawingAreaHeight);
    }//if

    return changed;
}//handleGraphValueZoom

void handleGraphTimeZoom(GdkScrollDirection direction, GraphState &graphState, int drawingAreaWidth)
{
    //This is a kinda brain-dead way of doing things..
    const boost::array<int, 40> scrollLevels = 
               {{ -50, -48, -46, -44, -42, -40, -38, -36, -34, -32, -30, -28, -26, -24, -20, -16, -8, -4, -2, 1,
                  2, 4, 8, 16, 32, 50, 70, 80, 90, 95, 100, 110, 118, 128, 140, 150, 170, 200, 250, 300
                }};

    int curPos = 20;
    for (int pos = 0; pos < (int)scrollLevels.size(); ++pos) {
        if (scrollLevels[pos] == graphState.ticksPerPixel) {
            curPos = pos;
            break;
        }//if
    }//if

    if (direction == GDK_SCROLL_UP) { //zoom in
        curPos = std::min(curPos + 1, ((int)scrollLevels.size()) - 1);
    } else {
        curPos = std::max(curPos - 1, 0);
    }//if

    if (graphState.ticksPerPixel != scrollLevels[curPos]) {
        graphState.ticksPerPixel = scrollLevels[curPos];

        int medianTickValue = graphState.verticalPixelTickValues[drawingAreaWidth / 2];
        graphState.setOffsetCenteredOnTick(medianTickValue, drawingAreaWidth);
    }//if
}//handleGraphTimeZoom

MouseButton getMouseButton(guint button)
{
    switch (button) {
        default:
        case 1:
            return MouseButton::LeftButton;

        case 2:
            return MouseButton::MiddleButton;

        case 3:
            return MouseButton::RightButton;
    }//switch
}//getMouseButton

}//anonymous namespace

bool FMidiAutomationMainWindow::handleScroll(GdkEventScroll *event)/*{{{*/
{
    //Good idea to sync up the bucky bits
    if (event->state & GDK_CONTROL_MASK) {
        ctrlCurrentlyPressed = true;
    } else {
        ctrlCurrentlyPressed = false;
    }//if

    if (event->state & GDK_SHIFT_MASK) {
        shiftCurrentlyPressed = true;
    } else {
        shiftCurrentlyPressed = false;
    }//if

    if (event->state & GDK_MOD1_MASK) {
        altCurrentlyPressed = true;
    } else {
        altCurrentlyPressed = false;
    }//if

    if (true == shiftCurrentlyPressed) {
        int curTicksPerPixel = graphState->ticksPerPixel;

        handleGraphTimeZoom(event->direction, *graphState, drawingAreaWidth);

        if (curTicksPerPixel != graphState->ticksPerPixel) {
            graphState->refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphState->refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
        }//if
    } else {
        if (DisplayMode::Curve == graphState->displayMode) {
            bool ret = handleGraphValueZoom(event->direction, *graphState, drawingAreaHeight);

            if (true == ret) {
                graphState->refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
                graphState->refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
            }//if
        }//if
    }//if

    if ((event->time - lastHandledTime) >= 60) {
        lastHandledTime = event->time;
        queue_draw();
    }//if

    return true;
}//handleScroll/*}}}*/

bool FMidiAutomationMainWindow::mouseButtonPressed(GdkEventButton *event)/*{{{*/
{
    bool retVal = true;
    MouseButton eventButton = getMouseButton(event->button);

    //Good idea to sync up the bucky bits
    if (event->state & GDK_CONTROL_MASK) {
        ctrlCurrentlyPressed = true;
    } else {
        ctrlCurrentlyPressed = false;
    }//if

    if (event->state & GDK_SHIFT_MASK) {
        shiftCurrentlyPressed = true;
    } else {
        shiftCurrentlyPressed = false;
    }//if

    if (event->state & GDK_MOD1_MASK) {
        altCurrentlyPressed = true;
    } else {
        altCurrentlyPressed = false;
    }//if

    if (event->type == GDK_BUTTON_PRESS) {
        if (MouseButton::LeftButton == eventButton) {
            graphState->selectedEntity = SelectedEntity::Nobody;
            leftMouseCurrentlyPressed = true;
        }//if

        mousePressDownX = event->x;
        mousePressDownY = event->y;
    }//if

    MouseRegion mouseRegion = determineMouseRegion(event->x, event->y, graphState->displayMode);

    switch (graphState->displayMode) {        
        case DisplayMode::Curve:
            switch (mouseRegion) {
                case MouseRegion::FrameRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleCurveEditorFrameRegionLMBPress(); break;
                        case MouseButton::MiddleButton: handleCurveEditorFrameRegionMMBPress(); break;
                        case MouseButton::RightButton: handleCurveEditorFrameRegionRMBPress(); break;
                    }//switch
                    break;

                case MouseRegion::MainCanvas:
                    switch (eventButton) {
                        case MouseButton::LeftButton: 
                            switch (event->type) {
                                case GDK_BUTTON_PRESS:
                                    graphState->inMotion = true;
                                    graphState->baseOffsetX = graphState->offsetX;
                                    graphState->baseOffsetY = graphState->offsetY;

                                    handleCurveEditorMainCanvasLMBPress();
                                    break;

                                default: break;
                            }//switch
                            break;
                            
                        case MouseButton::MiddleButton: handleCurveEditorMainCanvasMMBPress(); break;
                        case MouseButton::RightButton: retVal = handleCurveEditorMainCanvasRMBPress(event->x, event->button, event->time); break;
                    }//switch
                    break;

                case MouseRegion::TickMarkerRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerTickMarkerRegionLMBPress(event->x); break; //Same case as for sequencer
                        case MouseButton::MiddleButton: handleCurveEditorTickMarkerRegionMMBPress(); break;
                        case MouseButton::RightButton: handleCurveEditorTickMarkerRegionRMBPress(); break;
                    }//switch
                    break;

                case MouseRegion::LeftValueRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleCurveEditorLeftValueRegionLMBPress(); break;
                        case MouseButton::MiddleButton: handleCurveEditorLeftValueRegionMMBPress(); break;
                        case MouseButton::RightButton: handleCurveEditorLeftValueRegionRMBPress(); break; 
                    }//switch
                    break;
            }//switch
            break;

        case DisplayMode::Sequencer:
            sequencer->updateEntryFocus(event->y);

            switch (mouseRegion) {
                case MouseRegion::FrameRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerFrameRegionLMBPress(); break;
                        case MouseButton::MiddleButton: handleSequencerFrameRegionMMBPress(); break;
                        case MouseButton::RightButton: handleSequencerFrameRegionRMBPress(); break;
                    }//switch
                    break;

                case MouseRegion::MainCanvas:
                    switch (eventButton) {
                        case MouseButton::LeftButton: 
                            switch (event->type) {
                                case GDK_BUTTON_PRESS:
                                    graphState->inMotion = true;
                                    graphState->baseOffsetX = graphState->offsetX;
                                    graphState->baseOffsetY = graphState->offsetY;

                                    handleSequencerMainCanvasLMBPress();
                                    break;

                                case GDK_2BUTTON_PRESS:
                                    if (SelectedEntity::SequencerEntrySelection == graphState->selectedEntity) {
                                        //Double click on a sequencer entry block means we open up the curve editor for it
                                        handleCurveButtonPressed();
                                    }//if
                                    break;

                                default: break;
                            }//switch
                            break;

                        case MouseButton::MiddleButton: handleSequencerMainCanvasMMBPress(); break;
                        case MouseButton::RightButton: handleSequencerMainCanvasRMBPress(event->button, event->time); break;
                    }//switch
                    break;

                case MouseRegion::TickMarkerRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerTickMarkerRegionLMBPress(event->x); break;
                        case MouseButton::MiddleButton: handleSequencerTickMarkerRegionMMBPress(); break;
                        case MouseButton::RightButton: handleSequencerTickMarkerRegionRMBPress(); break;
                    }//switch
                    break;

                case MouseRegion::LeftValueRegion:
                    assert(mouseRegion != MouseRegion::LeftValueRegion);
                    break;
            }//switch
            break;
    }//switch

    queue_draw();
    return retVal;
}//mouseButtonPressed/*}}}*/

bool FMidiAutomationMainWindow::mouseButtonReleased(GdkEventButton *event)/*{{{*/
{
    MouseButton eventButton = getMouseButton(event->button);

    mousePressReleaseX = event->x;
    mousePressReleaseY = event->y;

    //Good idea to sync up the bucky bits
    if (event->state & GDK_CONTROL_MASK) {
        ctrlCurrentlyPressed = true;
    } else {
        ctrlCurrentlyPressed = false;
    }//if

    if (event->state & GDK_SHIFT_MASK) {
        shiftCurrentlyPressed = true;
    } else {
        shiftCurrentlyPressed = false;
    }//if

    if (event->state & GDK_MOD1_MASK) {
        altCurrentlyPressed = true;
    } else {
        altCurrentlyPressed = false;
    }//if

    if (MouseButton::LeftButton == eventButton) {
        leftMouseCurrentlyPressed = false;
        graphState->inMotion = false;
    }//if

    MouseRegion mouseRegion = determineMouseRegion(event->x, event->y, graphState->displayMode);

    switch (graphState->displayMode) {        
        case DisplayMode::Curve:
            switch (mouseRegion) {
                case MouseRegion::FrameRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleCurveEditorFrameRegionLMBRelease(); break;
                        case MouseButton::MiddleButton: handleCurveEditorFrameRegionMMBRelease(); break;
                        case MouseButton::RightButton: handleCurveEditorFrameRegionRMBRelease(); break;
                    }//switch
                    break;

                case MouseRegion::MainCanvas:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleCurveEditorMainCanvasLMBRelease(); break;
                        case MouseButton::MiddleButton: handleCurveEditorMainCanvasMMBRelease(); break;
                        case MouseButton::RightButton: handleCurveEditorMainCanvasRMBRelease(); break;
                    }//switch
                    break;

                case MouseRegion::TickMarkerRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerTickMarkerRegionLMBRelease(event->x, event->y); break; //Same as sequencer path
                        case MouseButton::MiddleButton: handleCurveEditorTickMarkerRegionMMBRelease(); break;
                        case MouseButton::RightButton: handleSequencerTickMarkerRegionRMBRelease(event->x, event->y); break; //Same as sequencer path
                    }//switch
                    break;

                case MouseRegion::LeftValueRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleCurveEditorLeftValueRegionLMBRelease(); break;
                        case MouseButton::MiddleButton: handleCurveEditorLeftValueRegionMMBRelease(); break;
                        case MouseButton::RightButton: handleCurveEditorLeftValueRegionRMBRelease(); break; 
                    }//switch
                    break;
            }//switch
            break;

        case DisplayMode::Sequencer:
            switch (mouseRegion) {
                case MouseRegion::FrameRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerFrameRegionLMBRelease(); break;
                        case MouseButton::MiddleButton: handleSequencerFrameRegionMMBRelease(); break;
                        case MouseButton::RightButton: handleSequencerFrameRegionRMBRelease(); break;
                    }//switch
                    break;

                case MouseRegion::MainCanvas:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerMainCanvasLMBRelease(event->x); break;
                        case MouseButton::MiddleButton: handleSequencerMainCanvasMMBRelease(); break;
                        case MouseButton::RightButton: handleSequencerMainCanvasRMBRelease(); break;
                    }//switch
                    break;

                case MouseRegion::TickMarkerRegion:
                    switch (eventButton) {
                        case MouseButton::LeftButton: handleSequencerTickMarkerRegionLMBRelease(event->x, event->y); break;
                        case MouseButton::MiddleButton: handleSequencerTickMarkerRegionMMBRelease(); break;
                        case MouseButton::RightButton: handleSequencerTickMarkerRegionRMBRelease(event->x, event->y); break;
                    }//switch
                    break;

                case MouseRegion::LeftValueRegion:
                    assert(mouseRegion != MouseRegion::LeftValueRegion);
                    break;
            }//switch
            break;
    }//switch

    queue_draw();
    return true;
}//mouseButtonReleased/*}}}*/

bool FMidiAutomationMainWindow::mouseMoved(GdkEventMotion *event)/*{{{*/
{
    graphState->curMousePosX = event->x;
    graphState->curMousePosY = event->y;

    if (true == shiftCurrentlyPressed) {
        graphState->doingRubberBanding = false;
    }//if

    MouseRegion mouseRegion = determineMouseRegion(event->x, event->y, graphState->displayMode);

    if (false == leftMouseCurrentlyPressed) {
        if (graphState->displayMode == DisplayMode::Curve) {
            int tick = 0;
            int value = 0;

            if (MouseRegion::MainCanvas == mouseRegion) {
                tick = graphState->verticalPixelTickValues[graphState->curMousePosX];
                positionTickEntry->set_text(boost::lexical_cast<Glib::ustring>(tick));
                value = (int)(graphState->horizontalPixelValues[graphState->curMousePosY - MainCanvasOffsetY] + 0.5);
                positionValueEntry->set_text(boost::lexical_cast<Glib::ustring>(value));
            }//if

            curveEditor->setUnderMouseTickValue(tick, value);
        }//if
        return false;
    }//if

    switch (graphState->displayMode) {        
        case DisplayMode::Curve:
            switch (mouseRegion) {
                case MouseRegion::FrameRegion: handleCurveEditorFrameRegionMouseMove(); break;
                case MouseRegion::MainCanvas: handleCurveEditorMainCanvasMouseMove(event->x, event->y); break;
                case MouseRegion::TickMarkerRegion: handleCurveEditorTickMarkerRegionMouseMove(); break;
                case MouseRegion::LeftValueRegion: handleCurveEditorLeftValueRegionMouseMove(); break;
                break;
            }//switch
            break;

        case DisplayMode::Sequencer:
            switch (mouseRegion) {
                case MouseRegion::FrameRegion: handleSequencerFrameRegionMouseMove(); break;
                case MouseRegion::MainCanvas: handleSequencerMainCanvasMouseMove(event->x, event->y); break;
                case MouseRegion::TickMarkerRegion: handleSequencerTickMarkerRegionMouseMove(); break;
                case MouseRegion::LeftValueRegion: assert(mouseRegion != MouseRegion::LeftValueRegion); break;
            }//switch
            break;
    }//switch

    //We want to be a little more flexible with the tick bars, so we don't consider what region the mouse is in when they're selected
    if (false == ctrlCurrentlyPressed) {
        if (graphState->selectedEntity == SelectedEntity::PointerTickBar) {
            if ((event->x >= 0) && (event->x < drawingAreaWidth)) {
                updateCursorTick(graphState->verticalPixelTickValues[event->x], true);
            }//if
        }//if

        else if (graphState->selectedEntity == SelectedEntity::LeftTickBar) {
            if ((event->x >= 0) && (event->x < drawingAreaWidth) && ((graphState->rightMarkerTick == -1) || (graphState->verticalPixelTickValues[event->x] < graphState->rightMarkerTick))) {
                graphState->leftMarkerTick = graphState->verticalPixelTickValues[event->x];
                graphState->leftMarkerTick = std::max(graphState->leftMarkerTick, 0);
                leftTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState->leftMarkerTick));
                queue_draw();
            }//if
        }//if

        else if (graphState->selectedEntity == SelectedEntity::RightTickBar) {
            if ((event->x >= 0) && (event->x < drawingAreaWidth) && ((graphState->leftMarkerTick == -1) || (graphState->verticalPixelTickValues[event->x] > graphState->leftMarkerTick))) {
                graphState->rightMarkerTick = graphState->verticalPixelTickValues[event->x];
                graphState->rightMarkerTick = std::max(graphState->rightMarkerTick, 0);
                rightTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState->rightMarkerTick));
                queue_draw();
            }//if
        }//if
    }//if

    if ((event->time - lastHandledTime) >= 20) {
        lastHandledTime = event->time;
        queue_draw();
    }//if

    return true;
}//mouseMoved/*}}}*/





