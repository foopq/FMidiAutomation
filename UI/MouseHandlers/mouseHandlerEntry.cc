/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include "FMidiAutomationCurveEditor.h"
#include "Animation.h"
#include "Command.h"
#include <boost/lexical_cast.hpp>

namespace
{

enum MouseRegion
{
    FrameRegion,
    MainCanvas,
    TickMarkerRegion,
    LeftValueRegion
};//MouseRegion

enum MouseButton
{
    LeftButton = 1,
    MiddleButton = 2,
    RightButton = 3
};//MouseButton

MouseRegion determineMouseRegion(int x, int y, DisplayMode::DisplayMode displayMode)
{
    if (y <= 30) {
        return FrameRegion;
    }//if

    if (y <= 60) {
        return TickMarkerRegion;
    }//if

    if (x > 60) {
        return MainCanvas;
    }//if

    switch (displayMode) {        
        case DisplayMode::Curve:
            return LeftValueRegion;

        case DisplayMode::Sequencer:
        default:
            return MainCanvas;
    }//switch
}//determineMouseRegion

bool handleGraphValueZoom(GdkScrollDirection direction, GraphState &graphState, int drawingAreaHeight)
{
    bool changed = true;
    //double curValuesPerPixel = graphState.valuesPerPixel;

    const boost::shared_ptr<SequencerEntryImpl> entryImpl = graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl();
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

    static guint32 lastHandledTime = 0; //XXX: This is safe to do, right? Only one thread ever gets here?

    if ((event->time - lastHandledTime) < 100) {
        return false;
    } else {
        lastHandledTime = event->time;
    }//if

    if (true == ctrlCurrentlyPressed) {
        int curTicksPerPixel = graphState.ticksPerPixel;

        handleGraphTimeZoom(event->direction, graphState, drawingAreaWidth);

        if (curTicksPerPixel != graphState.ticksPerPixel) {
            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
        }//if
    } else {
        if (DisplayMode::Curve == graphState.displayMode) {
            bool ret = handleGraphValueZoom(event->direction, graphState, drawingAreaHeight);

            if (true == ret) {
                graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
                graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
            }//if
        }//if
    }//if

    graphDrawingArea->queue_draw();

    return true;
}//handleScroll/*}}}*/

bool FMidiAutomationMainWindow::mouseButtonPressed(GdkEventButton *event)/*{{{*/
{
    bool retVal = true;

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
        if (LeftButton == event->button) {
            graphState.selectedEntity = Nobody;
            leftMouseCurrentlyPressed = true;
        }//if

        mousePressDownX = event->x;
        mousePressDownY = event->y;
    }//if

    MouseRegion mouseRegion = determineMouseRegion(event->x, event->y, graphState.displayMode);

    switch (graphState.displayMode) {        
        case DisplayMode::Curve:
            switch (mouseRegion) {
                case FrameRegion:
                    switch (event->button) {
                        case LeftButton: handleCurveEditorFrameRegionLMBPress(); break;
                        case MiddleButton: handleCurveEditorFrameRegionMMBPress(); break;
                        case RightButton: handleCurveEditorFrameRegionRMBPress(); break;
                    }//switch
                    break;

                case MainCanvas:
                    switch (event->button) {
                        case LeftButton: 
                            switch (event->type) {
                                case GDK_BUTTON_PRESS:
                                    graphState.inMotion = true;
                                    graphState.baseOffsetX = graphState.offsetX;
                                    graphState.baseOffsetY = graphState.offsetY;

                                    handleCurveEditorMainCanvasLMBPress();
                                    break;

                                default: break;
                            }//switch
                            break;
                            
                        case MiddleButton: handleCurveEditorMainCanvasMMBPress(); break;
                        case RightButton: retVal = handleCurveEditorMainCanvasRMBPress(event->x, event->button, event->time); break;
                    }//switch
                    break;

                case TickMarkerRegion:
                    switch (event->button) {
                        case LeftButton: handleSequencerTickMarkerRegionLMBPress(event->x); break; //Same case as for sequencer
                        case MiddleButton: handleCurveEditorTickMarkerRegionMMBPress(); break;
                        case RightButton: handleCurveEditorTickMarkerRegionRMBPress(); break;
                    }//switch
                    break;

                case LeftValueRegion:
                    switch (event->button) {
                        case LeftButton: handleCurveEditorLeftValueRegionLMBPress(); break;
                        case MiddleButton: handleCurveEditorLeftValueRegionMMBPress(); break;
                        case RightButton: handleCurveEditorLeftValueRegionRMBPress(); break; 
                    }//switch
                    break;
            }//switch
            break;

        case DisplayMode::Sequencer:
            switch (mouseRegion) {
                case FrameRegion:
                    switch (event->button) {
                        case LeftButton: handleSequencerFrameRegionLMBPress(); break;
                        case MiddleButton: handleSequencerFrameRegionMMBPress(); break;
                        case RightButton: handleSequencerFrameRegionRMBPress(); break;
                    }//switch
                    break;

                case MainCanvas:
                    switch (event->button) {
                        case LeftButton: 
                            switch (event->type) {
                                case GDK_BUTTON_PRESS:
                                    graphState.inMotion = true;
                                    graphState.baseOffsetX = graphState.offsetX;
                                    graphState.baseOffsetY = graphState.offsetY;

                                    handleSequencerMainCanvasLMBPress();
                                    break;

                                case GDK_2BUTTON_PRESS:
                                    if (SequencerEntrySelection == graphState.selectedEntity) {
                                        //Double click on a sequencer entry block means we open up the curve editor for it
                                        handleCurveButtonPressed();
                                    }//if
                                    break;

                                default: break;
                            }//switch
                            break;

                        case MiddleButton: handleSequencerMainCanvasMMBPress(); break;
                        case RightButton: handleSequencerMainCanvasRMBPress(event->button, event->time); break;
                    }//switch
                    break;

                case TickMarkerRegion:
                    switch (event->button) {
                        case LeftButton: handleSequencerTickMarkerRegionLMBPress(event->x); break;
                        case MiddleButton: handleSequencerTickMarkerRegionMMBPress(); break;
                        case RightButton: handleSequencerTickMarkerRegionRMBPress(); break;
                    }//switch
                    break;

                case LeftValueRegion:
                    assert(mouseRegion != LeftValueRegion);
                    break;
            }//switch
            break;
    }//switch

    graphDrawingArea->queue_draw();
    return retVal;
}//mouseButtonPressed/*}}}*/

bool FMidiAutomationMainWindow::mouseButtonReleased(GdkEventButton *event)/*{{{*/
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

    if (LeftButton == event->button) {
        leftMouseCurrentlyPressed = false;
        graphState.inMotion = false;
    }//if

    MouseRegion mouseRegion = determineMouseRegion(event->x, event->y, graphState.displayMode);

    switch (graphState.displayMode) {        
        case DisplayMode::Curve:
            switch (mouseRegion) {
                case FrameRegion:
                    switch (event->button) {
                        case LeftButton: handleCurveEditorFrameRegionLMBRelease(); break;
                        case MiddleButton: handleCurveEditorFrameRegionMMBRelease(); break;
                        case RightButton: handleCurveEditorFrameRegionRMBRelease(); break;
                    }//switch
                    break;

                case MainCanvas:
                    switch (event->button) {
                        case LeftButton: handleCurveEditorMainCanvasLMBRelease(); break;
                        case MiddleButton: handleCurveEditorMainCanvasMMBRelease(); break;
                        case RightButton: handleCurveEditorMainCanvasRMBRelease(); break;
                    }//switch
                    break;

                case TickMarkerRegion:
                    switch (event->button) {
                        case LeftButton: handleSequencerTickMarkerRegionLMBRelease(event->x, event->y); break; //Same as sequencer path
                        case MiddleButton: handleCurveEditorTickMarkerRegionMMBRelease(); break;
                        case RightButton: handleSequencerTickMarkerRegionRMBRelease(event->x, event->y); break; //Same as sequencer path
                    }//switch
                    break;

                case LeftValueRegion:
                    switch (event->button) {
                        case LeftButton: handleCurveEditorLeftValueRegionLMBRelease(); break;
                        case MiddleButton: handleCurveEditorLeftValueRegionMMBRelease(); break;
                        case RightButton: handleCurveEditorLeftValueRegionRMBRelease(); break; 
                    }//switch
                    break;
            }//switch
            break;

        case DisplayMode::Sequencer:
            switch (mouseRegion) {
                case FrameRegion:
                    switch (event->button) {
                        case LeftButton: handleSequencerFrameRegionLMBRelease(); break;
                        case MiddleButton: handleSequencerFrameRegionMMBRelease(); break;
                        case RightButton: handleSequencerFrameRegionRMBRelease(); break;
                    }//switch
                    break;

                case MainCanvas:
                    switch (event->button) {
                        case LeftButton: handleSequencerMainCanvasLMBRelease(event->x); break;
                        case MiddleButton: handleSequencerMainCanvasMMBRelease(); break;
                        case RightButton: handleSequencerMainCanvasRMBRelease(); break;
                    }//switch
                    break;

                case TickMarkerRegion:
                    switch (event->button) {
                        case LeftButton: handleSequencerTickMarkerRegionLMBRelease(event->x, event->y); break;
                        case MiddleButton: handleSequencerTickMarkerRegionMMBRelease(); break;
                        case RightButton: handleSequencerTickMarkerRegionRMBRelease(event->x, event->y); break;
                    }//switch
                    break;

                case LeftValueRegion:
                    assert(mouseRegion != LeftValueRegion);
                    break;
            }//switch
            break;
    }//switch

    graphDrawingArea->queue_draw();
    return true;
}//mouseButtonReleased/*}}}*/

bool FMidiAutomationMainWindow::mouseMoved(GdkEventMotion *event)/*{{{*/
{
    graphState.curMousePosX = event->x;
    graphState.curMousePosY = event->y;

    MouseRegion mouseRegion = determineMouseRegion(event->x, event->y, graphState.displayMode);

    if (false == leftMouseCurrentlyPressed) {
        if (graphState.displayMode == DisplayMode::Curve) {
            int tick = 0;
            int value = 0;

            if (MainCanvas == mouseRegion) {
                tick = graphState.verticalPixelTickValues[graphState.curMousePosX];
                positionTickEntry->set_text(boost::lexical_cast<Glib::ustring>(tick));
                value = (int)(graphState.horizontalPixelValues[graphState.curMousePosY - MainCanvasOffsetY] + 0.5);
                positionValueEntry->set_text(boost::lexical_cast<Glib::ustring>(value));
            }//if

            curveEditor->setUnderMouseTickValue(tick, value);
        }//if
        return false;
    }//if

    static guint32 lastHandledTime = 0; //XXX: This is safe to do, right? Only one thread ever gets here?

    if ((event->time - lastHandledTime) < 20) {
        return false;
    } else {
        lastHandledTime = event->time;
    }//if

    switch (graphState.displayMode) {        
        case DisplayMode::Curve:
            switch (mouseRegion) {
                case FrameRegion: handleCurveEditorFrameRegionMouseMove(); break;
                case MainCanvas: handleCurveEditorMainCanvasMouseMove(event->x, event->y); break;
                case TickMarkerRegion: handleCurveEditorTickMarkerRegionMouseMove(); break;
                case LeftValueRegion: handleCurveEditorLeftValueRegionMouseMove(); break;
                break;
            }//switch
            break;

        case DisplayMode::Sequencer:
            switch (mouseRegion) {
                case FrameRegion: handleSequencerFrameRegionMouseMove(); break;
                case MainCanvas: handleSequencerMainCanvasMouseMove(event->x); break;
                case TickMarkerRegion: handleSequencerTickMarkerRegionMouseMove(); break;
                case LeftValueRegion: assert(mouseRegion != LeftValueRegion); break;
            }//switch
            break;
    }//switch

    //We want to be a little more flexible with the tick bars, so we don't consider what region the mouse is in when they're selected
    if (false == ctrlCurrentlyPressed) {
        if (graphState.selectedEntity == PointerTickBar) {
            if ((event->x >= 0) && (event->x < drawingAreaWidth)) {
                updateCursorTick(graphState.verticalPixelTickValues[event->x], true);
            }//if
        }//if

        else if (graphState.selectedEntity == LeftTickBar) {
            if ((event->x >= 0) && (event->x < drawingAreaWidth) && ((graphState.rightMarkerTick == -1) || (graphState.verticalPixelTickValues[event->x] < graphState.rightMarkerTick))) {
                graphState.leftMarkerTick = graphState.verticalPixelTickValues[event->x];
                graphState.leftMarkerTick = std::max(graphState.leftMarkerTick, 0);
                leftTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.leftMarkerTick));
                graphDrawingArea->queue_draw();
            }//if
        }//if

        else if (graphState.selectedEntity == RightTickBar) {
            if ((event->x >= 0) && (event->x < drawingAreaWidth) && ((graphState.leftMarkerTick == -1) || (graphState.verticalPixelTickValues[event->x] > graphState.leftMarkerTick))) {
                graphState.rightMarkerTick = graphState.verticalPixelTickValues[event->x];
                graphState.rightMarkerTick = std::max(graphState.rightMarkerTick, 0);
                rightTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.rightMarkerTick));
                graphDrawingArea->queue_draw();
            }//if
        }//if
    }//if

    graphDrawingArea->queue_draw();
    return true;
}//mouseMoved/*}}}*/





