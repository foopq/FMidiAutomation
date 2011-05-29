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

bool handleGraphValueScroll(gdouble yPos, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaHeight)
{
    gdouble offsetY = yPos - mousePressDownY;

    double newOffset = graphState.baseOffsetY + offsetY;
    double medianValue = (drawingAreaHeight-60) / 2.0 * graphState.valuesPerPixel + newOffset * graphState.valuesPerPixel;

    const boost::shared_ptr<SequencerEntryImpl> entryImpl = graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl();
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
    int eventX = std::max(0, (int)xPos);
    eventX = std::min(eventX, drawingAreaWidth-1);
    int eventY = std::max(60, (int)yPos);
    eventY = std::min(eventY, drawingAreaHeight);
    eventY -= 60;

    int newTick = graphState.verticalPixelTickValues[eventX];
    if (graphState.zeroithTickPixel != std::numeric_limits<int>::max()) {
        newTick = std::max(newTick, graphState.verticalPixelTickValues[graphState.zeroithTickPixel+1]);
    }//if
    double newValue = graphState.horizontalPixelValues[eventY];

    newValue = std::max((int)newValue, graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl()->minValue);
    newValue = std::min((int)newValue, graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl()->maxValue);

    if ( ((newTick - graphState.getCurrentlySelectedEntryBlock()->getStartTick()) != graphState.currentlySelectedKeyframe->tick) && 
         (graphState.getCurrentlySelectedEntryBlock()->getCurve()->getKeyframeAtTick(newTick) != NULL) ) {
        return;
    }//if

    graphState.didMoveKey = true;
    graphState.didMoveKeyOutTangent = true;
    graphState.didMoveKeyInTangent = true;

    graphState.getCurrentlySelectedEntryBlock()->getCurve()->deleteKey(graphState.currentlySelectedKeyframe);

    graphState.currentlySelectedKeyframe->tick = newTick - graphState.getCurrentlySelectedEntryBlock()->getStartTick();
    graphState.currentlySelectedKeyframe->value = newValue;

    graphState.getCurrentlySelectedEntryBlock()->getCurve()->addKey(graphState.currentlySelectedKeyframe);
}//handleKeyScroll

void handleKeyTangentScroll(gdouble xPos, gdouble yPos, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaWidth, int drawingAreaHeight)
{
    int eventX = std::max(0, (int)xPos);
    eventX = std::min(eventX, drawingAreaWidth-1);
    int eventY = std::max(60, (int)yPos);
    eventY = std::min(eventY, drawingAreaHeight);
    eventY -= 60;

    int newTick = graphState.verticalPixelTickValues[eventX];
    double newValue = graphState.horizontalPixelValues[eventY];

    if (InTangent == graphState.selectedEntity) {
        graphState.didMoveKeyInTangent = true;

        graphState.currentlySelectedKeyframe->inTangent[0] = graphState.currentlySelectedKeyframe->tick - newTick;
        graphState.currentlySelectedKeyframe->inTangent[1] = newValue - graphState.currentlySelectedKeyframe->value;
    } else {
        graphState.didMoveKeyOutTangent = true;

        graphState.currentlySelectedKeyframe->outTangent[0] = newTick - graphState.currentlySelectedKeyframe->tick;
        graphState.currentlySelectedKeyframe->outTangent[1] = newValue - graphState.currentlySelectedKeyframe->value;
    }//if
}//handleKeyTangentScroll



}//anonymous namespace

void FMidiAutomationMainWindow::handleCurveEditorMainCanvasLMBPress()
{
    if (false == ctrlCurrentlyPressed) {
        unsetAllCurveFrames();

        graphState.currentlySelectedKeyframe = curveEditor->getKeySelection(graphState, mousePressDownX, mousePressDownY);
        curveEditor->setKeyUIValues(uiXml, graphState.currentlySelectedKeyframe);

        if (graphState.currentlySelectedKeyframe != NULL) {
            graphState.didMoveKey = false;
            graphState.movingKeyOrigTick = graphState.currentlySelectedKeyframe->tick - graphState.getCurrentlySelectedEntryBlock()->getStartTick();
            graphState.movingKeyOrigValue = graphState.currentlySelectedKeyframe->value;

            menuCopy->set_sensitive(true);
            menuCut->set_sensitive(true);
        } else {
            menuCopy->set_sensitive(false);
            menuCut->set_sensitive(false);
        }//if

        //Essentially clear the selection state of the tempo changes
        (void)checkForTempoSelection(-100, datas->tempoChanges);
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

    graphState.currentlySelectedKeyframe = curveEditor->getKeySelection(graphState, mousePressDownX, mousePressDownY);
    curveEditor->setKeyUIValues(uiXml, graphState.currentlySelectedKeyframe);

    if (graphState.currentlySelectedKeyframe == NULL) {
        graphState.selectedEntity = Nobody;

        std::string menuStr = "Add Keyframe";
        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = graphState.getCurrentlySelectedEntryBlock();

        int curMouseUnderTick = graphState.verticalPixelTickValues[xPos];

        if (xPos > graphState.zeroithTickPixel) { 
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
        graphState.selectedEntity = KeyValue;

        m_refActionGroup->add(Gtk::Action::create("ContextDeleteKeyframe", "Delete Keyframe"), sigc::mem_fun(curveEditor.get(), &CurveEditor::handleDeleteKeyframe));
        ui_info =
            "<ui>"
            "  <popup name='PopupMenu'>"
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
    if (graphState.selectedEntity == KeyValue) {
        if (true == graphState.didMoveKey) {                        
            //Move key back to where it was
            graphState.getCurrentlySelectedEntryBlock()->getCurve()->deleteKey(graphState.currentlySelectedKeyframe);
            std::swap(graphState.currentlySelectedKeyframe->tick, graphState.movingKeyOrigTick);
            std::swap(graphState.currentlySelectedKeyframe->value, graphState.movingKeyOrigValue);
            graphState.getCurrentlySelectedEntryBlock()->getCurve()->addKey(graphState.currentlySelectedKeyframe);

            boost::shared_ptr<Command> moveKeyframeCommand(new MoveKeyframeCommand(graphState.getCurrentlySelectedEntryBlock(), graphState.currentlySelectedKeyframe, graphState.movingKeyOrigTick, graphState.movingKeyOrigValue));
            CommandManager::Instance().setNewCommand(moveKeyframeCommand, true);
        }//if
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
    if (true == ctrlCurrentlyPressed) {
        //We are scrolling the canvas
        gdouble curOffsetX = graphState.offsetX;

        handleGraphTimeScroll(xPos, graphState, mousePressDownX, mousePressDownY, drawingAreaWidth);
        bool didScroll = handleGraphValueScroll(yPos, graphState, mousePressDownX, mousePressDownY, drawingAreaHeight);

        if ((graphState.offsetX != curOffsetX) || (true == didScroll)) {
            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
        }//if
    } else {
        if (graphState.selectedEntity == KeyValue) {
            handleKeyScroll(xPos, yPos, graphState, mousePressDownX, mousePressDownY, drawingAreaWidth, drawingAreaHeight);
            curveEditor->setKeyUIValues(uiXml, graphState.currentlySelectedKeyframe);
        }//if

        else if ((graphState.selectedEntity == InTangent) || (graphState.selectedEntity == OutTangent)) {
            handleKeyTangentScroll(xPos, yPos, graphState, mousePressDownX, mousePressDownY, drawingAreaWidth, drawingAreaHeight);
            curveEditor->setKeyUIValues(uiXml, graphState.currentlySelectedKeyframe);
        }//if
    }//if
}//handleSequencerCurveEditorMouseMove

