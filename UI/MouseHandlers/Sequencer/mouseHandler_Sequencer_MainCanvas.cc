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

}//anonymous namespace


void FMidiAutomationMainWindow::handleSequencerMainCanvasLMBPress()
{
    if (false == ctrlCurrentlyPressed) {
        unsetAllCurveFrames();

        boost::shared_ptr<SequencerEntryBlock> entryBlock = sequencer->getSelectedEntryBlock(mousePressDownX, mousePressDownY, true);
        if (entryBlock == NULL) {
            sequencer->clearSelectedEntryBlock();

            menuCopy->set_sensitive(false);
            menuCut->set_sensitive(false);

            graphState.currentlySelectedEntryBlocks.clear();
            graphState.currentlySelectedEntryOriginalStartTicks.clear();
        } else {
            graphState.selectedEntity = SequencerEntrySelection;

            if (graphState.currentlySelectedEntryOriginalStartTicks.find(entryBlock) == graphState.currentlySelectedEntryOriginalStartTicks.end()) {
                graphState.currentlySelectedEntryOriginalStartTicks.clear();
                graphState.currentlySelectedEntryOriginalStartTicks[entryBlock]  = entryBlock->getStartTick();
                graphState.currentlySelectedEntryBlocks.clear();
                graphState.currentlySelectedEntryBlocks.insert(std::make_pair(entryBlock->getStartTick(), entryBlock));

                positionTickEntry->set_text(boost::lexical_cast<Glib::ustring>(entryBlock->getStartTick()));

                menuCopy->set_sensitive(true);
                menuCut->set_sensitive(true);
            }//if
        }//if

        //Essentially clear the selection state of the tempo changes
        (void)checkForTempoSelection(-100, datas->tempoChanges);
    } else {
        //Left click while control is held
        boost::shared_ptr<SequencerEntryBlock> entryBlock = sequencer->getSelectedEntryBlock(mousePressDownX, mousePressDownY, true);

        if (entryBlock != NULL) {
            if (graphState.currentlySelectedEntryOriginalStartTicks.find(entryBlock) == graphState.currentlySelectedEntryOriginalStartTicks.end()) {
                graphState.currentlySelectedEntryOriginalStartTicks[entryBlock] = entryBlock->getStartTick();
                graphState.currentlySelectedEntryBlocks.insert(std::make_pair(entryBlock->getStartTick(), entryBlock));
            } else {
                std::map<boost::shared_ptr<SequencerEntryBlock>, int>::iterator tickIter = graphState.currentlySelectedEntryOriginalStartTicks.find(entryBlock);
                std::multimap<int, boost::shared_ptr<SequencerEntryBlock> >::iterator entryIter = graphState.currentlySelectedEntryBlocks.find(entryBlock->getStartTick());

                assert(tickIter != graphState.currentlySelectedEntryOriginalStartTicks.end());
                assert(entryIter != graphState.currentlySelectedEntryBlocks.end());

                graphState.currentlySelectedEntryOriginalStartTicks.erase(tickIter);
                graphState.currentlySelectedEntryBlocks.erase(entryIter);
            }//if
        }//if
    }//if
}//handleSequencerMainCanvasLMBPress

void FMidiAutomationMainWindow::handleSequencerMainCanvasMMBPress()
{
    //Nothing
}//handleSequencerMainCanvasMMBPress

void FMidiAutomationMainWindow::handleSequencerMainCanvasRMBPress(guint button, guint32 time)
{
    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("ContextMenu", "Context Menu"));

    Glib::ustring ui_info = "<ui><popup name='PopupMenu'></popup></ui>";
    boost::shared_ptr<SequencerEntryBlock> entryBlock = sequencer->getSelectedEntryBlock(mousePressDownX, mousePressDownY, true);

    if (entryBlock != NULL) {
        if (graphState.currentlySelectedEntryOriginalStartTicks.find(entryBlock) == graphState.currentlySelectedEntryOriginalStartTicks.end()) {
            std::cout << "adding new entry block 2" << std::endl;

            graphState.currentlySelectedEntryOriginalStartTicks[entryBlock] = entryBlock->getStartTick();
            graphState.currentlySelectedEntryBlocks.insert(std::make_pair(entryBlock->getStartTick(), entryBlock));
        }//if

        //Context menu to delete entry
        
        m_refActionGroup->add(Gtk::Action::create("ContextDelete", "Delete Entry Block"), sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleDeleteSequencerEntryBlock));
        if (graphState.currentlySelectedEntryBlocks.size() > 1) {
            m_refActionGroup->add(Gtk::Action::create("ContextDelete", "Delete All Selected Entry Blocks"), sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleDeleteSequencerEntryBlocks));
        }//if
        m_refActionGroup->add(Gtk::Action::create("ContextProperties", "Entry Block Properties"), sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleSequencerEntryProperties));
        m_refActionGroup->add(Gtk::Action::create("ContextCurve", "Entry Block Curve"), sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleSequencerEntryCurve));
        ui_info =
            "<ui>"
            "  <popup name='PopupMenu'>"
            "    <menuitem action='ContextCurve'/>"
            "    <menuitem action='ContextProperties'/>"
            "    <menuitem action='ContextDelete'/>"
            "  </popup>"
            "</ui>";

        graphDrawingArea->queue_draw();
    } else {
        if (sequencer->getSelectedEntry() != NULL) {
            //Context menu to add entry
            m_refActionGroup->add(Gtk::Action::create("ContextAdd", "Add Entry Block"), sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleAddSequencerEntryBlock));
            ui_info =
                "<ui>"
                "  <popup name='PopupMenu'>"
                "    <menuitem action='ContextAdd'/>"
                "  </popup>"
                "</ui>";
        }//if
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
}//handleSequencerMainCanvasRMBPress

void FMidiAutomationMainWindow::handleSequencerMainCanvasLMBRelease(gdouble xPos)
{
    if ((graphState.selectedEntity == SequencerEntrySelection) && (mousePressDownX != xPos)) {
        /*  -- It almost makes sense to fix up the times on the start ticks within the maps.. but it seems the house of cards need it to be this way.
               The correct start tick value is on the entry block. One day I should refactor this to make more sense.
        graphState.currentlySelectedEntryBlocks.clear();
        std::map<boost::shared_ptr<SequencerEntryBlock>, int> tmpCurrentlySelectedEntryOriginalStartTicks;
        for (std::map<boost::shared_ptr<SequencerEntryBlock>, int>::iterator tickIter = graphState.currentlySelectedEntryOriginalStartTicks.begin();
                tickIter != graphState.currentlySelectedEntryOriginalStartTicks.end(); ++tickIter) {
            graphState.currentlySelectedEntryBlocks.insert(std::make_pair(tickIter->first->getStartTick(), tickIter->first));
            tmpCurrentlySelectedEntryOriginalStartTicks[tickIter->first] = tickIter->first->getStartTick();                
        }//for

        graphState.currentlySelectedEntryOriginalStartTicks.swap(tmpCurrentlySelectedEntryOriginalStartTicks);

        for (std::map<boost::shared_ptr<SequencerEntryBlock>, int>::iterator tickIter = graphState.currentlySelectedEntryOriginalStartTicks.begin();
                tickIter != graphState.currentlySelectedEntryOriginalStartTicks.end(); ++tickIter) {
            std::cout << "1handleSequencerMainCanvasLMBRelease: " << tickIter->first->getStartTick() << " - " << tickIter->second << std::endl;
        }//for
        */

        std::map<boost::shared_ptr<SequencerEntryBlock>, int> entryNewStartTicks;
        for (std::multimap<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator blockIter = graphState.currentlySelectedEntryBlocks.begin(); 
                    blockIter != graphState.currentlySelectedEntryBlocks.end(); ++blockIter) {
            boost::shared_ptr<SequencerEntryBlock> entryBlock = blockIter->second;

            entryNewStartTicks[entryBlock] = entryBlock->getStartTick();

            //std::cout << "2handleSequencerMainCanvasLMBRelease: " << blockIter->first << std::endl << std::endl;
        }//for

//      graphState.currentlySelectedEntryBlocks.clear();
//      for (std::map<boost::shared_ptr<SequencerEntryBlock>, int>::const_iterator blockIter = entryNewStartTicks.begin(); blockIter != entryNewStartTicks.end(); ++blockIter) {
//          graphState.currentlySelectedEntryBlocks[blockIter->second] = blockIter->first;
//      }//for

        boost::shared_ptr<Command> moveSequencerEntryBlockCommand(new MoveSequencerEntryBlockCommand(graphState.currentlySelectedEntryBlocks, graphState.currentlySelectedEntryOriginalStartTicks, entryNewStartTicks));
        CommandManager::Instance().setNewCommand(moveSequencerEntryBlockCommand, true);
        graphState.currentlySelectedEntryOriginalStartTicks = entryNewStartTicks;
    }//if
}//handleSequencerMainCanvasLMBRelease

void FMidiAutomationMainWindow::handleSequencerMainCanvasMMBRelease()
{
    //Nothing
}//handleSequencerMainCanvasMMBRelease

void FMidiAutomationMainWindow::handleSequencerMainCanvasRMBRelease()
{
    //Nothing
}//handleSequencerMainCanvasRMBRelease

void FMidiAutomationMainWindow::handleSequencerMainCanvasMouseMove(gdouble xPos)
{
    std::cout << "handleSequencerMainCanvasMouseMove" << std::endl;

    if (true == ctrlCurrentlyPressed) {
        //We are scrolling the canvas
        gdouble curOffsetX = graphState.offsetX;

        handleGraphTimeScroll(xPos, graphState, mousePressDownX, mousePressDownY, drawingAreaWidth);

        if (graphState.offsetX != curOffsetX) {
            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
        }//if       
    } else {
        if (graphState.selectedEntity == SequencerEntrySelection) {
            int curX = xPos;
            curX = std::max(0, curX);
            curX = std::min(curX, drawingAreaWidth-1);
            int diffTick = graphState.verticalPixelTickValues[curX] - graphState.verticalPixelTickValues[mousePressDownX];

            int firstCurTick = std::numeric_limits<int>::min();
            for (std::multimap<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator blockIter = graphState.currentlySelectedEntryBlocks.begin(); blockIter != graphState.currentlySelectedEntryBlocks.end(); ++blockIter) {
                int curTick = graphState.currentlySelectedEntryOriginalStartTicks[blockIter->second] + diffTick;

//            std::cout << "x: " << curX << std::endl;
//            std::cout << "diffTick: " << diffTick << "   --  curTick: " << curTick << std::endl;

                (blockIter->second)->moveBlock(curTick);

                if (std::numeric_limits<int>::min() == firstCurTick) {
                    firstCurTick = curTick;
                }//if
            }//for

            firstCurTick = std::max(0, firstCurTick);
            positionTickEntry->set_text(boost::lexical_cast<Glib::ustring>(firstCurTick));
        }//if
    }//if
}//handleSequencerMainCanvasMouseMove


