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


bool CurveEditor::handleKeyEntryOnSelectedKeyTickEntryEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    if (mainWindow->getGraphState().currentlySelectedKeyframes.size() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyTickEntry", entry);

        std::string entryText = entry->get_text();
        int tick = boost::lexical_cast<int>(entryText);

        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().getCurrentlySelectedEntryBlock();
        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;

        if ( ((tick - currentlySelectedEntryBlock->getStartTick()) == selectedKey->tick) || 
             (mainWindow->getGraphState().getCurrentlySelectedEntryBlock()->getCurve()->getKeyframeAtTick(tick) != NULL) ) {
            return false;
        }//if

        mainWindow->getGraphState().getCurrentlySelectedEntryBlock()->getCurve()->deleteKey(selectedKey);
        selectedKey->tick = tick - currentlySelectedEntryBlock->getStartTick();
        mainWindow->getGraphState().getCurrentlySelectedEntryBlock()->getCurve()->addKey(selectedKey);
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

    if (mainWindow->getGraphState().currentlySelectedKeyframes.size() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyValueEntry", entry);

        std::string entryText = entry->get_text();
        int value = boost::lexical_cast<int>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;
        selectedKey->value = value;
        setKeyUIValues(uiXml, mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second);

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

    if (mainWindow->getGraphState().currentlySelectedKeyframes.size() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyInTanXEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;
        selectedKey->inTangent[0] = value;
        setKeyUIValues(uiXml, mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second);

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

    if (mainWindow->getGraphState().currentlySelectedKeyframes.size() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyInTanYEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;
        selectedKey->inTangent[1] = value;
        setKeyUIValues(uiXml, mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second);

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

    if (mainWindow->getGraphState().currentlySelectedKeyframes.size() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyOutTanXEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;
        selectedKey->outTangent[0] = value;
        setKeyUIValues(uiXml, mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second);

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

    if (mainWindow->getGraphState().currentlySelectedKeyframes.size() != 1) {
        return false;
    }//if

    try {
        Gtk::Entry *entry;
        uiXml->get_widget("selectedKeyOutTanYEntry", entry);

        std::string entryText = entry->get_text();
        double value = boost::lexical_cast<double>(entryText);

        boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;
        selectedKey->outTangent[1] = value;
        setKeyUIValues(uiXml, mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second);

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

    if (mainWindow->getGraphState().currentlySelectedKeyframes.empty() == true) {
        return;
    }//if

    boost::shared_ptr<Keyframe> selectedKey = mainWindow->getGraphState().currentlySelectedKeyframes.begin()->second;
    selectedKey->curveType = curveType;

    if (CurveType::Bezier == curveType) {
        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().getCurrentlySelectedEntryBlock();
        boost::shared_ptr<Keyframe> afterSelectedKey = currentlySelectedEntryBlock->getNextKeyframe(selectedKey);

        if (afterSelectedKey != NULL) {
            int tickDiff = (afterSelectedKey->tick - selectedKey->tick) / 3;

            if (selectedKey->outTangent[0] == std::numeric_limits<int>::min()) {
                selectedKey->outTangent[0] = tickDiff;
                selectedKey->outTangent[1] = 0;
            }//if

            if (afterSelectedKey->inTangent[0] == std::numeric_limits<int>::min()) {
                afterSelectedKey->inTangent[0] = tickDiff;
                afterSelectedKey->inTangent[1] = 0;
            }//if
        }//if
    }//if

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
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().getCurrentlySelectedEntryBlock();
    if (currentlySelectedEntryBlock->getCurve()->getKeyframeAtTick(curMouseUnderTick) != NULL) {
        return;
    }//if

    boost::shared_ptr<Command> addKeyframeCommand(new AddKeyframesCommand(currentlySelectedEntryBlock, 
                                                                            curMouseUnderTick - currentlySelectedEntryBlock->getStartTick(), curMouseUnderValue));
    CommandManager::Instance().setNewCommand(addKeyframeCommand, true);
}//handleAddKeyframe

void CurveEditor::handleDeleteKeyframe()
{
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().getCurrentlySelectedEntryBlock();

    if (mainWindow->getGraphState().currentlySelectedKeyframes.empty() == true) {
        return;
    }//if

    boost::shared_ptr<Command> deleteKeyframeCommand(new DeleteKeyframesCommand(currentlySelectedEntryBlock, mainWindow->getGraphState().currentlySelectedKeyframes));
    CommandManager::Instance().setNewCommand(deleteKeyframeCommand, true);
}//handleDeletedKeyframe

void CurveEditor::getKeySelection(GraphState &graphState, int mousePressDownX, int mousePressDownY, bool ctrlPressed)
{
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().getCurrentlySelectedEntryBlock();

    assert(currentlySelectedEntryBlock != NULL);
    if ((currentlySelectedEntryBlock == NULL) || (mainWindow->getGraphState().displayMode != DisplayMode::Curve)) {
        return;
    }//if

    boost::shared_ptr<Keyframe> selectedKey;
    boost::shared_ptr<Animation> curve = currentlySelectedEntryBlock->getCurve();
    int numKeys = curve->getNumKeyframes();

//    std::cout << "getKeySelection: " << numKeys << std::endl;

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

    if (selectedKey != NULL) {
        selectedKey->setSelectedState(KeySelectedType::Key);
        graphState.selectedEntity = selectedEntity;

//        std::cout << "found key: " << mousePressDownX << " - " << mousePressDownY << std::endl;

        if (true == ctrlPressed) {
            if (mainWindow->getGraphState().currentlySelectedKeyframes.find(selectedKey->tick) != mainWindow->getGraphState().currentlySelectedKeyframes.end()) {
                mainWindow->getGraphState().currentlySelectedKeyframes.erase(mainWindow->getGraphState().currentlySelectedKeyframes.find(selectedKey->tick));
                mainWindow->getGraphState().movingKeyOrigTicks.erase(mainWindow->getGraphState().movingKeyOrigTicks.find(selectedKey));
                mainWindow->getGraphState().movingKeyOrigValues.erase(mainWindow->getGraphState().movingKeyOrigValues.find(selectedKey));

                selectedKey->setSelectedState(KeySelectedType::NotSelected);
            } else {
                mainWindow->getGraphState().currentlySelectedKeyframes.insert(std::make_pair(selectedKey->tick, selectedKey));
            }//if
        } else {
            for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = mainWindow->getGraphState().currentlySelectedKeyframes.begin();
                    keyIter != mainWindow->getGraphState().currentlySelectedKeyframes.end(); ++ keyIter) {
                keyIter->second->setSelectedState(KeySelectedType::NotSelected);
            }//for

            mainWindow->getGraphState().currentlySelectedKeyframes.clear();
            mainWindow->getGraphState().movingKeyOrigTicks.clear();
            mainWindow->getGraphState().movingKeyOrigValues.clear();
            mainWindow->getGraphState().currentlySelectedKeyframes.insert(std::make_pair(selectedKey->tick, selectedKey));
        }//if
    } else {
        if (true == ctrlPressed) {
            //Nothing
        } else {
            for (std::multimap<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = mainWindow->getGraphState().currentlySelectedKeyframes.begin();
                    keyIter != mainWindow->getGraphState().currentlySelectedKeyframes.end(); ++ keyIter) {
                keyIter->second->setSelectedState(KeySelectedType::NotSelected);
            }//for

            mainWindow->getGraphState().currentlySelectedKeyframes.clear();
            mainWindow->getGraphState().movingKeyOrigTicks.clear();
            mainWindow->getGraphState().movingKeyOrigValues.clear();
        }//if
//std::cout << "no found key: " << mousePressDownX << " - " << mousePressDownY << std::endl;
    }//if
}//getKeySelection

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
        boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock = mainWindow->getGraphState().getCurrentlySelectedEntryBlock();
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



