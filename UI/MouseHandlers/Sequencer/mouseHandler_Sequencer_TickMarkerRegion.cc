#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include "FMidiAutomationCurveEditor.h"
#include "Animation.h"
#include "Command.h"
#include <boost/lexical_cast.hpp>


void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionLMBPress(gdouble xPos)
{
    if (false == ctrlCurrentlyPressed) {
        if ((graphState.leftMarkerTickXPixel >= 0) && (abs(xPos - graphState.leftMarkerTickXPixel) <= 5)) {
            graphState.selectedEntity = LeftTickBar;
        }//if
        else if ((graphState.rightMarkerTickXPixel >= 0) && (abs(xPos - graphState.rightMarkerTickXPixel) <= 5)) {
            graphState.selectedEntity = RightTickBar;
        }//if
        else if (abs(xPos - graphState.curPointerTickXPixel) <= 5) {
            graphState.selectedEntity = PointerTickBar;
        }//if
        else if (checkForTempoSelection(xPos, datas->tempoChanges) == true) {
            graphState.selectedEntity = TempoChange;
            handleBPMFrameClickBase();
            updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);

            if (graphState.displayMode == DisplayMode::Sequencer) {
                sequencer->clearSelectedEntryBlock();
                graphState.currentlySelectedEntryBlocks.clear();
            }//if
        }//if

        else {
            //Essentially clear the selection state of the tempo changes
            (void)checkForTempoSelection(-100, datas->tempoChanges);
        }//if
    }//if
}//handleSequencerTickMarkerRegionLMBPress

void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionMMBPress()
{
    //Nothing
}//handleSequencerTickMarkerRegionMMBPress

void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionRMBPress()
{
    //Nothing
}//handleSequencerTickMarkerRegionRMBPress

void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionLMBRelease(gdouble xPos, gdouble yPos)
{
    if ((yPos == mousePressDownY) && (abs(xPos -mousePressDownX) <= 5)) {
        if (false == ctrlCurrentlyPressed) {
            if (graphState.selectedEntity != TempoChange) {
                updateCursorTick(graphState.verticalPixelTickValues[xPos], true);
            }//if
        } else {
            if ((graphState.rightMarkerTick == -1) || (graphState.rightMarkerTick > graphState.verticalPixelTickValues[xPos])) {
                graphState.leftMarkerTick = graphState.verticalPixelTickValues[xPos];
                graphState.leftMarkerTick = std::max(graphState.leftMarkerTick, 0);
                leftTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.leftMarkerTick));
            }//if
        }//if
    }//if
}//handleSequencerTickMarkerRegionLMBRelease

void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionMMBRelease()
{
    //Nothing
}//handleSequencerTickMarkerRegionMMBRelease

void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionRMBRelease(gdouble xPos, gdouble yPos)
{
    if ((yPos == mousePressDownY) && (abs(xPos - mousePressDownX) <= 5)) {
        if ((graphState.leftMarkerTick == -1) || (graphState.leftMarkerTick < graphState.verticalPixelTickValues[xPos])) {
            graphState.rightMarkerTick = graphState.verticalPixelTickValues[xPos];
            graphState.rightMarkerTick = std::max(graphState.rightMarkerTick, 0);
            rightTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.rightMarkerTick));
        }//if
    }//if
}//handleSequencerTickMarkerRegionRMBRelease

void FMidiAutomationMainWindow::handleSequencerTickMarkerRegionMouseMove()
{
    //Nothing
}//handleSequencerTickMarkerRegionMouseMove


