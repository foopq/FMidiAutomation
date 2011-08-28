/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationCurveEditor.h"
#include "Animation.h"
#include "Sequencer.h"
#include "Command.h"
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>


bool CurveEditor::handleKeyEntryOnSelectedKeyTickEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().keyframeSelectionState.GetNumSelected() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyTickEntry", entry);

        std::string entryText = entry->get_text();
        int tick = boost::lexical_cast<int>(entryText);

        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();
        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();

        if ( ((tick - currentlySelectedEntryBlock->getStartTick()) == selectedKey->tick) || 
             (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(tick) != NULL) ) {
            return false;
        }//if

        currentlySelectedEntryBlock->getCurve()->deleteKey(selectedKey);
        selectedKey->tick = tick - currentlySelectedEntryBlock->getStartTick();
        currentlySelectedEntryBlock->getCurve()->addKey(selectedKey);
        setKeyUIValues(uiXml, selectedKey);

        mainWindow->queue_draw();

        return true;
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnSelectedKeyTickEntryEntryBox

bool CurveEditor::handleKeyEntryOnSelectedKeyValueEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().keyframeSelectionState.GetNumSelected() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyValueEntry", entry);

        std::string entryText = entry->get_text();
        int value = boost::lexical_cast<int>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();
        selectedKey->value = value;
        setKeyUIValues(uiXml, selectedKey);

        mainWindow->queue_draw();

        return true;
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnSelectedKeyValueEntryEntryBox

bool CurveEditor::handleKeyEntryOnSelectedKeyInTanXEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().keyframeSelectionState.GetNumSelected() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyInTanXEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();
        selectedKey->inTangent[0] = value;
        setKeyUIValues(uiXml, selectedKey);

        mainWindow->queue_draw();

        return true;
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnSelectedKeyInTanXEntryEntryBox

bool CurveEditor::handleKeyEntryOnSelectedKeyInTanYEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().keyframeSelectionState.GetNumSelected() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyInTanYEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();
        selectedKey->inTangent[1] = value;
        setKeyUIValues(uiXml, selectedKey);

        mainWindow->queue_draw();

        return true;
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnSelectedKeyInTanYEntryEntryBox

bool CurveEditor::handleKeyEntryOnSelectedKeyOutTanXEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().keyframeSelectionState.GetNumSelected() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyOutTanXEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();
        selectedKey->outTangent[0] = value;
        setKeyUIValues(uiXml, selectedKey);

        mainWindow->queue_draw();

        return true;
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnSelectedKeyOutTanXEntryEntryBox

bool CurveEditor::handleKeyEntryOnSelectedKeyOutTanYEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().keyframeSelectionState.GetNumSelected() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyOutTanYEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();
        selectedKey->outTangent[1] = value;
        setKeyUIValues(uiXml, selectedKey);

        mainWindow->queue_draw();

        return true;
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnSelectedKeyOutTanYEntryEntryBox

void CurveEditor::handleSelectionChangeOnSelectedKeyTypeComboBox()
{
    Gtk::ComboBox *comboBox;
    uiXml->get_widget("selectedKeyTypeComboBox", comboBox);

    CurveType::CurveType curveType;
    int activeIndex = comboBox->property_active();
    switch (activeIndex) {
        case 0:
            curveType = CurveType::Bezier;
            break;
        case 1:
            curveType = CurveType::Linear;
            break;
        case 2:
            curveType = CurveType::Step;
            break;
    }//switch

    if (mainWindow->getGraphState().keyframeSelectionState.HasSelected() == false) {
        return;
    }//if

    boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().keyframeSelectionState.GetFirstKeyframe();
    selectedKey->curveType = curveType;

    if (CurveType::Bezier == curveType) {
        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();
        boost::shared_ptr<Keyframe> afterSelectedKey = currentlySelectedEntryBlock->getNextKeyframe(selectedKey);

        if (afterSelectedKey != NULL) {
            int tickDiff = (afterSelectedKey->tick - selectedKey->tick) / 3;

            if (selectedKey->outTangent[0] == std::numeric_limits<int>::min()) {
                selectedKey->outTangent[0] = tickDiff;
                selectedKey->outTangent[1] = 0;
            }//if

            if (selectedKey->inTangent[0] == std::numeric_limits<int>::min()) {
                selectedKey->inTangent[0] = tickDiff;
                selectedKey->inTangent[1] = 0;
            }//if

            if (afterSelectedKey->inTangent[0] == std::numeric_limits<int>::min()) {
                afterSelectedKey->inTangent[0] = tickDiff;
                afterSelectedKey->inTangent[1] = 0;
            }//if
        }//if
    }//if

    setKeyUIValues(uiXml, selectedKey);
    mainWindow->queue_draw();
}//handleSelectionChangeOnSelectedKeyTypeComboBox


void CurveEditor::setUpWidgets()
{    
    Gtk::ComboBox *comboBox;
    uiXml->get_widget("selectedKeyTypeComboBox", comboBox);
    comboBox->signal_changed().connect(sigc::mem_fun(*this, &CurveEditor::handleSelectionChangeOnSelectedKeyTypeComboBox));

    Gtk::Entry *entry;

    uiXml->get_widget("selectedKeyTickEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &CurveEditor::handleKeyEntryOnSelectedKeyTickEntryEntryBox));

    uiXml->get_widget("selectedKeyValueEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &CurveEditor::handleKeyEntryOnSelectedKeyValueEntryEntryBox));

    uiXml->get_widget("selectedKeyInTanXEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &CurveEditor::handleKeyEntryOnSelectedKeyInTanXEntryEntryBox));

    uiXml->get_widget("selectedKeyInTanYEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &CurveEditor::handleKeyEntryOnSelectedKeyInTanYEntryEntryBox));

    uiXml->get_widget("selectedKeyOutTanXEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &CurveEditor::handleKeyEntryOnSelectedKeyOutTanXEntryEntryBox));

    uiXml->get_widget("selectedKeyOutTanYEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &CurveEditor::handleKeyEntryOnSelectedKeyOutTanYEntryEntryBox));
}//setUpWidgets

CurveEditor::CurveEditor(FMidiAutomationMainWindow *mainWindow_, Glib::RefPtr<Gtk::Builder> uiXml_)
{
    mainWindow = mainWindow_;
    curMouseUnderTick = 0;
    curMouseUnderValue = 0;
    uiXml = uiXml_;

    setUpWidgets();
}//constructor

CurveEditor::~CurveEditor()
{
    //Nothing
}//destructor

void CurveEditor::setUnderMouseTickValue(int tick, int value)
{
    curMouseUnderTick = tick;
    curMouseUnderValue = value;
}//setUnderMouseTickValue

void CurveEditor::handleAddKeyframe()
{
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();
    if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(curMouseUnderTick) != NULL) {
        return;
    }//if

    boost::shared_ptr<Command> addKeyframeCommand(new AddKeyframesCommand(currentlySelectedEntryBlock, 
                                                                            curMouseUnderTick - currentlySelectedEntryBlock->getStartTick(), curMouseUnderValue));
    CommandManager::Instance().setNewCommand(addKeyframeCommand, true);
}//handleAddKeyframe

void CurveEditor::handleDeleteKeyframes()
{
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();

    if (mainWindow->getGraphState().keyframeSelectionState.HasSelected() == false) {
        return;
    }//if

    boost::shared_ptr<Command> deleteKeyframesCommand(new DeleteKeyframesCommand(currentlySelectedEntryBlock, mainWindow->getGraphState().keyframeSelectionState.GetSelectedKeyframesCopy()));
    CommandManager::Instance().setNewCommand(deleteKeyframesCommand, true);
}//handleDeletedKeyframe

boost::shared_ptr<Keyframe> CurveEditor::getKeySelection(GraphState &graphState, int mousePressDownX, int mousePressDownY, bool ctrlPressed)
{
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();

    assert(currentlySelectedEntryBlock != NULL);
    if ((currentlySelectedEntryBlock == NULL) || (mainWindow->getGraphState().displayMode != DisplayMode::Curve)) {
        return boost::shared_ptr<Keyframe>();
    }//if

    boost::shared_ptr<Keyframe> selectedKey;
    boost::shared_ptr<Animation> curve = currentlySelectedEntryBlock->getCurve();
    int numKeys = curve->getNumKeyframes();

std::cout << "getKeySelection: " << numKeys << std::endl;

    SelectedEntity selectedEntity = Nobody;
    for (int index = 0; index < numKeys; ++index) {
        boost::shared_ptr<Keyframe> curKey = curve->getKeyframe(index);
        if ( (curKey->drawnStartX <= mousePressDownX) && (curKey->drawnStartX + 9 >= mousePressDownX) &&
             (curKey->drawnStartY <= mousePressDownY) && (curKey->drawnStartY + 9 >= mousePressDownY) ) {
            selectedKey = curKey;
            selectedEntity = KeyValue;
        }//if

        if ( (curKey->drawnOutX <= mousePressDownX) && (curKey->drawnOutX + 9 >= mousePressDownX) &&
             (curKey->drawnOutY <= mousePressDownY) && (curKey->drawnOutY + 9 >= mousePressDownY) ) {
            selectedKey = curKey;
            selectedEntity = OutTangent;
        }//if

        if ( (curKey->drawnInX <= mousePressDownX) && (curKey->drawnInX + 9 >= mousePressDownX) &&
             (curKey->drawnInY <= mousePressDownY) && (curKey->drawnInY + 9 >= mousePressDownY) ) {
            selectedKey = curKey;
            selectedEntity = InTangent;
        }//if
    }//for

////    graphState.selectedKey = selectedKey;
////    std::cout << "graphState.selectedKey: " << graphState.selectedKey << std::endl;

    if (selectedKey != NULL) {
        selectedKey->setSelectedState(KeySelectedType::Key);
        graphState.selectedEntity = selectedEntity;

//        std::cout << "found key: " << mousePressDownX << " - " << mousePressDownY << std::endl;

        if (true == ctrlPressed) {
            //If we find the selected key...
            if (mainWindow->getGraphState().keyframeSelectionState.IsSelected(selectedKey) == true) {
                mainWindow->getGraphState().keyframeSelectionState.RemoveKeyframe(selectedKey);
                selectedKey->setSelectedState(KeySelectedType::NotSelected);
                std::cout << "NotSelected1" << std::endl;
            } else {
                mainWindow->getGraphState().keyframeSelectionState.AddKeyframe(selectedKey);
            }//if
        } else {
            //If we can't find the selected key...
            if (mainWindow->getGraphState().keyframeSelectionState.IsSelected(selectedKey) == false) {
std::cout << "clear1" << std::endl;                
                mainWindow->getGraphState().keyframeSelectionState.ClearSelectedKeyframes();
                mainWindow->getGraphState().keyframeSelectionState.AddKeyframe(selectedKey);
            }//if
        }//if
    } else {
        if (true == ctrlPressed) {
            //Nothing
        } else {
std::cout << "clear2" << std::endl;            
            mainWindow->getGraphState().keyframeSelectionState.ClearSelectedKeyframes();
        }//if
//std::cout << "no found key: " << mousePressDownX << " - " << mousePressDownY << std::endl;
    }//if

    return selectedKey;
}//getKeySelection

void CurveEditor::updateSelectedKeyframesInRange(KeyframeSelectionState &keyframeSelectionState,
//        std::map<int, boost::shared_ptr<Keyframe> > &currentlySelectedKeyframes,
//                                                    std::set<boost::shared_ptr<Keyframe> > &origSelectedKeyframes,
                                                    gdouble mousePressDownX, gdouble mousePressDownY, gdouble mousePosX, gdouble mousePosY,
                                                    int areaWidth, int areaHeight)
{
    mousePressDownX = std::max<gdouble>(mousePressDownX, 0);
    mousePressDownX = std::min<gdouble>(mousePressDownX, areaWidth);

    mousePressDownY = std::max<gdouble>(mousePressDownY, 61);
    mousePressDownY = std::min<gdouble>(mousePressDownY, areaHeight);

    mousePosX = std::max<gdouble>(mousePosX, 0);
    mousePosX = std::min<gdouble>(mousePosX, areaWidth);

    mousePosY = std::max<gdouble>(mousePosY, 61);
    mousePosY = std::min<gdouble>(mousePosY, areaHeight);
    
    if (mousePosX < mousePressDownX) {
        std::swap(mousePosX, mousePressDownX);
    }//if

    if (mousePosY < mousePressDownY) {
        std::swap(mousePosY, mousePressDownY);
    }//if

    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();

    assert(currentlySelectedEntryBlock != NULL);
    if ((currentlySelectedEntryBlock == NULL) || (mainWindow->getGraphState().displayMode != DisplayMode::Curve)) {
        return;
    }//if

    boost::shared_ptr<Animation> curve = currentlySelectedEntryBlock->getCurve();
    int numKeys = curve->getNumKeyframes();

    for (int index = 0; index < numKeys; ++index) {
        boost::shared_ptr<Keyframe> curKey = curve->getKeyframe(index);

        if (curKey->drawnStartX < mousePosX &&
            curKey->drawnStartX + 9 > mousePressDownX &&
            curKey->drawnStartY < mousePosY &&
            curKey->drawnStartY + 9 > mousePressDownY) {

            if (keyframeSelectionState.IsSelected(curKey) == false) {
                keyframeSelectionState.AddKeyframe(curKey);
                curKey->setSelectedState(KeySelectedType::Key);
            }//if
        } else {
            if (keyframeSelectionState.IsOrigSelected(curKey) == false) {
                keyframeSelectionState.RemoveKeyframe(curKey);
                curKey->setSelectedState(KeySelectedType::NotSelected);
            }//if
        }//if
    }//for
}//updateSelectedKeyframesInRange

void CurveEditor::setKeyUIValues(Glib::RefPtr<Gtk::Builder> uiXml, boost::shared_ptr<Keyframe> currentlySelectedKeyframe)
{
    Gtk::ComboBox *comboBox;
    uiXml->get_widget("selectedKeyTypeComboBox", comboBox);
    if (currentlySelectedKeyframe != NULL) {
        assert(currentlySelectedKeyframe->curveType != CurveType::Init);

        switch (currentlySelectedKeyframe->curveType) {
            case CurveType::Step:
                comboBox->set_active(2);
                break;
            case CurveType::Linear:
                comboBox->set_active(1);
                break;
            case CurveType::Bezier:
                comboBox->set_active(0);
                break;
            default:
                break;
        }//switch
    }//if

    Gtk::Entry *entry;

    uiXml->get_widget("selectedKeyTickEntry", entry);
    if (currentlySelectedKeyframe != NULL) {
        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().entryBlockSelectionState.GetFirstEntryBlock();
        entry->set_text(boost::lexical_cast<std::string>(currentlySelectedKeyframe->tick + currentlySelectedEntryBlock->getStartTick()));
    } else {
        entry->set_text("");
    }//if

    uiXml->get_widget("selectedKeyValueEntry", entry);
    if (currentlySelectedKeyframe != NULL) {
        int tmpValue = currentlySelectedKeyframe->value + 0.5;
        if (currentlySelectedKeyframe->value < 0) {
            tmpValue = currentlySelectedKeyframe->value - 0.5;
        }//if
        entry->set_text(boost::lexical_cast<std::string>(tmpValue));
    } else {
        entry->set_text("");
    }//if

    uiXml->get_widget("selectedKeyInTanXEntry", entry);
    if ((currentlySelectedKeyframe != NULL) && (currentlySelectedKeyframe->inTangent[0] >= 0)) {
        entry->set_text(boost::lexical_cast<std::string>(currentlySelectedKeyframe->inTangent[0]));
    } else {
        entry->set_text("");
    }//if

    uiXml->get_widget("selectedKeyInTanYEntry", entry);
    if ((currentlySelectedKeyframe != NULL) && (currentlySelectedKeyframe->inTangent[1] >= 0)) {
        entry->set_text(boost::lexical_cast<std::string>(currentlySelectedKeyframe->inTangent[1]));
    } else {
        entry->set_text("");
    }//if

    uiXml->get_widget("selectedKeyOutTanXEntry", entry);
    if ((currentlySelectedKeyframe != NULL) && (currentlySelectedKeyframe->outTangent[0] >= 0)) {
        entry->set_text(boost::lexical_cast<std::string>(currentlySelectedKeyframe->outTangent[0]));
    } else {
        entry->set_text("");
    }//if

    uiXml->get_widget("selectedKeyOutTanYEntry", entry);
    if ((currentlySelectedKeyframe != NULL) && (currentlySelectedKeyframe->outTangent[1] >= 0)) {
        entry->set_text(boost::lexical_cast<std::string>(currentlySelectedKeyframe->outTangent[1]));
    } else {
        entry->set_text("");
    }//if
}//setKeyUIValues



