/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include <gtkmm.h>
#include <libglademm.h>
#include <iostream>
#include <fstream>
#include "FMidiAutomationMainWindow.h"
#include "Data/FMidiAutomationData.h"
#include "FMidiAutomationCurveEditor.h"
#include "Command_Sequencer.h"
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "jack.h"
#include "Data/Sequencer.h"
#include "Data/SequencerEntry.h"
#include "UI/SequencerUI.h"
#include "UI/SequencerEntryUI.h"
#include "EntryBlockProperties.h"
#include "PasteManager.h"
#include "EntryProperties.h"
#include "Animation.h"
#include "jackPortDialog.h"
#include "GraphState.h"
#include "Globals.h"
#include "SerializationHelper.h"
#include "Tempo.h"
#include "WindowManager.h"


namespace
{

std::shared_ptr<SequencerEntryBlockUI> findWrappingEntryBlockUI(std::shared_ptr<SequencerUI> sequencer, std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    for (auto entryIter : sequencer->getEntryPair()) {
        for (auto entryBlockIter : entryIter.first->getEntryBlocksPair()) {
            if (entryBlockIter.second->getBaseEntryBlock() == entryBlock) {
                return entryBlockIter.second;
            }//if
        }//for
    }//for

    return std::shared_ptr<SequencerEntryBlockUI>();
}//findWrappingEntryBlockUI

}//anonymous namespace

std::shared_ptr<Globals> Globals::instance;
std::recursive_mutex Globals::globalsMutex;

Globals::Globals()
{
    versionStr = "FMidiAutomation - version 1.0.0 - October 2011";
    topBarFontSize = 12;
    topBarFont = "Arial";
    bottomBarFontSize = 12;
    bottomBarFont = "Arial";
    darkTheme = true;
};//constructor

Globals::~Globals()
{
    //Nothing
}//destructor

Globals &Globals::Instance()
{
    std::lock_guard<std::recursive_mutex> lock(globalsMutex);

    if (instance == nullptr) {
        ResetInstance();
    }//if

    return *instance;
}//Instance

void Globals::ResetInstance()
{
    std::lock_guard<std::recursive_mutex> lock(globalsMutex);
    instance.reset(new Globals());
}//ResetInstance

template<class Archive>
void Globals::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(projectData);
}//serialize


template void Globals::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void Globals::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);


FMidiAutomationMainWindow::FMidiAutomationMainWindow()
{
    lastHandledTime = 0;
    isExiting = false;

    uiXml = Gtk::Builder::create_from_file("FMidiAutomation.glade");

    curveEditor.reset(new CurveEditor(this, uiXml));

    uiXml->get_widget("mainWindow", mainWindow);
    uiXml->get_widget("trackListWindow", trackListWindow);
    
    backingImage.reset(new Gtk::Image());
    backingTexture.reset(new Gtk::Image());
    origBackingImage.reset(new Gtk::Image());
    origBackingTexture.reset(new Gtk::Image());
    
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    pixbuf = Gdk::Pixbuf::create_from_file("pics/background.png");
    origBackingTexture->set(pixbuf);
    pixbuf = Gdk::Pixbuf::create_from_file("pics/fractal.tga");
    origBackingImage->set(pixbuf);
    
    mainWindow->signal_delete_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleOnClose));    

    uiXml->get_widget("graphDrawingArea", graphDrawingArea);
    graphDrawingArea->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
    graphDrawingArea->signal_size_allocate().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleGraphResize) );
    graphDrawingArea->signal_expose_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::updateGraph) );
    graphDrawingArea->signal_button_press_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::mouseButtonPressed) );
    graphDrawingArea->signal_button_release_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::mouseButtonReleased) );
    graphDrawingArea->signal_motion_notify_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::mouseMoved) );
    graphDrawingArea->signal_scroll_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleScroll) );

    uiXml->get_widget("menu_open", menuOpen);
    uiXml->get_widget("menu_recent", menuOpenRecent);
    uiXml->get_widget("menu_save", menuSave);
    uiXml->get_widget("menu_saveas", menuSaveAs);
    uiXml->get_widget("menu_new", menuNew);
    uiXml->get_widget("menu_quit", menuQuit);
    uiXml->get_widget("menu_copy", menuCopy);
    uiXml->get_widget("menu_cut", menuCut);
    uiXml->get_widget("menu_paste", menuPaste);
    uiXml->get_widget("menu_paste_instance", menuPasteInstance);

    uiXml->get_widget("menu_splitEntryBlock", menuSplitEntryBlock);
    uiXml->get_widget("menu_joinEntryBlocks", menuJoinEntryBlocks);

    menuOpen->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuOpen));
    menuSave->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuSave));
    menuSaveAs->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuSaveAs));
    menuNew->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuNew));
    menuQuit->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuQuit));

    menuSplitEntryBlock->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuSplitEntryBlocks));
    menuJoinEntryBlocks->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuJoinEntryBlocks));

    Gtk::MenuItem *menuItem;
    uiXml->get_widget("menu_resetTangents", menuItem);
    menuItem->signal_activate().connect(sigc::mem_fun(*curveEditor, &CurveEditor::handleResetTangents));

    uiXml->get_widget("menuitem_align_main", menuItem);
    menuItem->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuAlignMainCursor));
    uiXml->get_widget("menuitem_align_left", menuItem);
    menuItem->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuAlignLeftCursor));
    uiXml->get_widget("menuitem_align_right", menuItem);
    menuItem->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuAlignRightCursor));

    menuCopy->set_sensitive(false);
    menuCut->set_sensitive(false);
    menuPaste->set_sensitive(false);
    menuPasteInstance->set_sensitive(false);

    uiXml->get_widget("menu_pasteSEBToSelectedEntry", menu_pasteSEBToSelectedEntry);
    menu_pasteSEBToSelectedEntry->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menupasteSEBToSelectedEntry));
    menu_pasteSEBToSelectedEntry->set_sensitive(false);

    Gtk::MenuItem *menu_pasteSEBInstancesToSelectedEntry;
    uiXml->get_widget("menu_pasteSEBInstancesToSelectedEntry", menu_pasteSEBInstancesToSelectedEntry);
    menu_pasteSEBInstancesToSelectedEntry->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menupasteSEBInstancesToSelectedEntry));
    menu_pasteSEBInstancesToSelectedEntry->set_sensitive(false);

    menuCopy->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuCopy));
    menuCut->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuCut));
    menuPaste->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuPaste));
    menuPasteInstance->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuPasteInstance));

    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    uiXml->get_widget("menu_redo", menuRedo);
    uiXml->get_widget("menu_undo", menuUndo);

    menuUndo->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuUndo));
    menuRedo->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuRedo));

    mainWindow->signal_key_press_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::key_pressed));
    mainWindow->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::key_released));

    shiftCurrentlyPressed = false;
    ctrlCurrentlyPressed = false;
    altCurrentlyPressed = false;
    leftMouseCurrentlyPressed = false;
    mousePressDownX = 0;
    mousePressDownY = 0;

    uiXml->get_widget("leftTickEntryBox", leftTickEntryBox);
    uiXml->get_widget("rightTickEntryBox", rightTickEntryBox);
    uiXml->get_widget("cursorTickEntryBox", cursorTickEntryBox);
    uiXml->get_widget("leftBarEntryBox", leftBarEntryBox);
    uiXml->get_widget("rightBarEntryBox", rightBarEntryBox);
    uiXml->get_widget("cursorBarEntryBox", cursorBarEntryBox);
    uiXml->get_widget("transportTimeEntry", transportTimeEntry);
    uiXml->get_widget("currentSampledValue", currentSampledValue);

    leftTickEntryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnLeftTickEntryBox));
    rightTickEntryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnRightTickEntryBox));
    cursorTickEntryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnCursorTickEntryBox));

    uiXml->get_widget("focusStealingButton", focusStealingButton);
    focusStealingButton->grab_focus();

    uiXml->get_widget("bpmEntry", bpmEntry);
    uiXml->get_widget("beatsPerBarEntry", beatsPerBarEntry);
    uiXml->get_widget("barSubdivisionsEntry", barSubdivisionsEntry);

    uiXml->get_widget("selectedKeyframeFrame", selectedKeyframeFrame);

    uiXml->get_widget("bpmFrameCheckButton", bpmFrameCheckButton);

    Gtk::Viewport *bpmFrame;
    uiXml->get_widget("viewport8", bpmFrame);
    bpmFrame->signal_button_press_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClick) );
    bpmEntry->signal_grab_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClickBase) );
    beatsPerBarEntry->signal_grab_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClickBase) );
    barSubdivisionsEntry->signal_grab_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClickBase) );

    uiXml->get_widget("positionTickEntry", positionTickEntry);
    uiXml->get_widget("positionValueEntry", positionValueEntry);
    uiXml->get_widget("positionValueLabel", positionValueLabel);

    positionTickEntry->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnPositionTickEntryBox));
 
    Gtk::Entry *entry;
    uiXml->get_widget("selectedEntryBlockNameEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnSelectedEntryBlockNameEntryBox));

    uiXml->get_widget("statusLabel", statusBar);
    statusTextAlpha = 1.0;
    needsStatusTextUpdate = false;
    setStatusText(Glib::ustring("Welcome to FMidiAutomation"));
    boost::function<void (void)> statusThreadFunc = [=]() { statusTextThreadFunc(); };
    statusTextThread = std::thread(statusThreadFunc);

    setThemeColours();

    Gtk::ToolButton *button;
    uiXml->get_widget("addButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleAddPressed) );
    uiXml->get_widget("deleteButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleDeletePressed) );
    uiXml->get_widget("upButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleUpButtonPressed) );
    uiXml->get_widget("downButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleDownButtonPressed) );
    uiXml->get_widget("sequencerButton", sequencerButton);
    sequencerButton->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleSequencerButtonPressed) );
    uiXml->get_widget("curveButton", curveButton);
    curveButton->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleCurveButtonPressed) );

    sequencerButton->set_sensitive(false);

    Gtk::Frame *frame;
    uiXml->get_widget("selectedKeyframeFrame", frame);
    frame->set_visible(false);
    uiXml->get_widget("selectedEntryBlockFrame", frame);
    frame->set_visible(true);

    Glib::RefPtr<Gtk::AccelGroup> accelGroup = mainWindow->get_accel_group();
    menuNew->add_accelerator("activate", accelGroup, GDK_N, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuOpen->add_accelerator("activate", accelGroup, GDK_O, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuSave->add_accelerator("activate", accelGroup, GDK_S, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuQuit->add_accelerator("activate", accelGroup, GDK_Q, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

    menuUndo->add_accelerator("activate", accelGroup, GDK_Z, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuRedo->add_accelerator("activate", accelGroup, GDK_Z, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK, Gtk::ACCEL_VISIBLE);

    menuCopy->add_accelerator("activate", accelGroup, GDK_C, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuCut->add_accelerator("activate", accelGroup, GDK_X, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuPaste->add_accelerator("activate", accelGroup, GDK_V, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuPasteInstance->add_accelerator("activate", accelGroup, GDK_V, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK, Gtk::ACCEL_VISIBLE);

    Gtk::ImageMenuItem *menuPorts;
    uiXml->get_widget("menu_ports", menuPorts);
    menuPorts->add_accelerator("activate", accelGroup, GDK_P, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuPorts->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuPorts));

    recordMidi = false;

//    Glib::signal_idle().connect( sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_idle) );
    idleConnection = Glib::signal_timeout().connect( sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_idle), 16 );

    uiXml->get_widget("rewButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleRewPressed) );
    uiXml->get_widget("playButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handlePlayPressed) );
    uiXml->get_widget("pauseButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handlePausePressed) );
    uiXml->get_widget("recButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleRecordPressed) );

    setTitle("Unknown");

    Gtk::ToggleButton *toggleButton;
    uiXml->get_widget("jackEnabledToggleButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleJackPressed) );
    toggleButton->set_active(true);

    mainWindow->set_icon_from_file("pics/tmpicon.jpg");

    Gtk::RadioButton *radioButton;
    uiXml->get_widget("radiobutton_merge", radioButton);
    radioButton->set_active(true);
    radioButton->signal_toggled().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleInsertModeChanged) );
 
    mainWindow->signal_set_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleSetFocus) );

    Globals &globals = Globals::Instance();
    Gtk::VBox *entryVBox;
    uiXml->get_widget("entryVBox", entryVBox);
    sequencer.reset(new SequencerUI(globals.projectData.getEntryGlade(), entryVBox, this));
}//constructor

FMidiAutomationMainWindow::~FMidiAutomationMainWindow()
{
    idleConnection.disconnect();
    isExiting = true;
    statusTextMutex.unlock();
    statusTextThread.join();
}//destructor
 
std::shared_ptr<SequencerUI> FMidiAutomationMainWindow::getSequencer()
{
    return sequencer;
}//getSequencer

bool FMidiAutomationMainWindow::handleOnClose(GdkEventAny *)
{
    idleConnection.disconnect();
    WindowManager &windowManager = WindowManager::Instance();
    windowManager.unregisterWindow(shared_from_this());
    return false;
}//handleOnClose

void FMidiAutomationMainWindow::init(bool curveEditorOnlyMode_, std::shared_ptr<SequencerEntryBlockUI> editingEntryBlock_)
{
    graphState = std::make_shared<GraphState>();
    mainWindow->show();

    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    uiXml->get_widget("menu_redo", menuRedo);
    uiXml->get_widget("menu_undo", menuUndo);

    Gtk::MenuItem *menu_pasteSEBInstancesToSelectedEntry;
    uiXml->get_widget("menu_pasteSEBInstancesToSelectedEntry", menu_pasteSEBInstancesToSelectedEntry);

    uiXml->get_widget("menu_pasteSEBToSelectedEntry", menu_pasteSEBToSelectedEntry);

    CommandManager::Instance().setMenuItems(menuUndo, menuRedo);
    PasteManager::Instance().setMenuItems(menuPaste, menuPasteInstance, menu_pasteSEBToSelectedEntry, menu_pasteSEBInstancesToSelectedEntry);

    boost::function<void (void)> titleStarFunc = [=]() { setTitleChanged(); };
    CommandManager::Instance().setTitleStar(titleStarFunc);

    //Globals &globals = Globals::Instance();
    //boost::function<void (const std::string &)> loadCallback = [=](const Glib::ustring &filename) { actuallyLoadFile(filename); };
    //globals.config.getMRUList().setLoadCallback(loadCallback);
    MRUList &mruList = MRUList::Instance();
    mruList.registerTopMenu(this, menuOpenRecent);

    curveEditorOnlyMode = curveEditorOnlyMode_;
    editingEntryBlock = editingEntryBlock_;

    getGraphState().entryBlockSelectionState.ClearSelected();

    if (editingEntryBlock != nullptr) {
        getGraphState().entryBlockSelectionState.AddSelectedEntryBlock(editingEntryBlock);
    }//if

    if (true == curveEditorOnlyMode) {
        Gtk::Viewport *sequencerEntryViewport;
        uiXml->get_widget("sequencerEntryViewport", sequencerEntryViewport);
        sequencerEntryViewport->set_visible(false);

        Gtk::ToggleButton *toggleButton;
        uiXml->get_widget("jackEnabledToggleButton", toggleButton);
        toggleButton->set_visible(false);

        Gtk::HButtonBox *mergeReplaceButtonBox;
        uiXml->get_widget("mergeReplaceButtonBox", mergeReplaceButtonBox);
        mergeReplaceButtonBox->set_visible(false);

        setTitle(editingEntryBlock->getBaseEntryBlock()->getTitle());
    } else {
        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setTime(0);
    }//if
}//init

void FMidiAutomationMainWindow::queue_draw()
{
    graphDrawingArea->queue_draw();
}//queue_draw

boost::function<void (const std::string &)> FMidiAutomationMainWindow::getLoadCallback()
{
    boost::function<void (const std::string &)> loadCallback = [=](const Glib::ustring &filename) { actuallyLoadFile(filename); };
    return loadCallback;
}//getLoadCallback

bool FMidiAutomationMainWindow::getCurveEditorOnlyMode()
{
    return curveEditorOnlyMode;
}//getCurveEditorOnlyMode

std::shared_ptr<SequencerEntryBlockUI> FMidiAutomationMainWindow::getEditingEntryBlock()
{
    return editingEntryBlock;
}//getEditingEntryBlock

void FMidiAutomationMainWindow::handleSetFocus(Gtk::Widget *widget)
{
std::cout << "handleSetFocus" << std::endl;

    queue_draw();
}//handleSetFocus

void FMidiAutomationMainWindow::setTitle(Glib::ustring currentFilename)
{
    mainWindow->set_title("FMidiAutomation - " + currentFilename);
}//setTitle

void FMidiAutomationMainWindow::setTitleChanged()
{
    Glib::ustring curTitle = mainWindow->get_title();
    if (curTitle[curTitle.length()-1] != '*') {
        curTitle = curTitle.substr(sizeof("FMidiAutomation - ")-1);
        curTitle = curTitle + " *";
        setTitle(curTitle);
    }//if
}//setTitleChanged

void FMidiAutomationMainWindow::setThemeColours()
{
return;

    Globals &globals = Globals::Instance();

    Gdk::Color fgColour;
    Gdk::Color bgColour;
    Gdk::Color editBoxBgColour;
    Gdk::Color textColour;
    Gdk::Color darkTextColour;
    Gdk::Color black;

    black.set_rgb(0, 0, 0);

    if (true == globals.darkTheme) {
        fgColour.set_rgb(52429, 42429, 52429);
        bgColour.set_rgb(10000, 10000, 10000);
        editBoxBgColour.set_rgb(25000, 25000, 25000);
        textColour.set_rgb(55982, 55982, 55982);
        darkTextColour.set_rgb(45982, 45982, 45982);
    }//if

    Gtk::Widget *tmpWidget;

    for (int x = 1; x <= 12; ++x) {
        //if ((x < 11) && (x != 1)) {
        //    Gtk::ToolButton *toolbutton;
        //    uiXml->get_widget(std::string("toolbutton") + boost::lexical_cast<std::string>(x), toolbutton);
        //    toolbutton->get_label_widget()->modify_fg(Gtk::STATE_NORMAL, darkTextColour);
        //}//if

        if (x < 12) {
            std::string viewportStr("viewport");
            viewportStr = viewportStr + boost::lexical_cast<std::string>(x);

            uiXml->get_widget(viewportStr, tmpWidget);
            tmpWidget->modify_bg(Gtk::STATE_NORMAL, bgColour);
            tmpWidget->modify_fg(Gtk::STATE_NORMAL, fgColour);
        }//if

        if (x < 3) {
            Gtk::Toolbar *toolbar;
            uiXml->get_widget(std::string("toolbar") + boost::lexical_cast<std::string>(x), toolbar);
            toolbar->modify_bg(Gtk::STATE_NORMAL, bgColour);
        }//if

        if (x < 5) {
            Gtk::MenuItem *menuItem;
            uiXml->get_widget(std::string("menuitem") + boost::lexical_cast<std::string>(x), menuItem);
            menuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
        }//if

        if (x < 4) {
            Gtk::Menu *menu;
            uiXml->get_widget(std::string("menu") + boost::lexical_cast<std::string>(x), menu);
            menu->modify_bg(Gtk::STATE_NORMAL, bgColour);
        }//if

        if (x < 8) {
            Gtk::Label *label;
            uiXml->get_widget(std::string("label") + boost::lexical_cast<std::string>(x), label);
            label->modify_fg(Gtk::STATE_NORMAL, darkTextColour);
        }//if
    }//for

    Gtk::Label *label;
    uiXml->get_widget("statusLabel", label);
    label->modify_fg(Gtk::STATE_NORMAL, darkTextColour);

    uiXml->get_widget("menubar", tmpWidget);
    tmpWidget->modify_bg(Gtk::STATE_NORMAL, bgColour);

    Gtk::ImageMenuItem *imageMenuItem;
    uiXml->get_widget("menu_new", imageMenuItem);
    imageMenuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
    uiXml->get_widget("menu_open", imageMenuItem);
    imageMenuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
    uiXml->get_widget("menu_save", imageMenuItem);
    imageMenuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
    uiXml->get_widget("menu_saveas", imageMenuItem);
    imageMenuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
    uiXml->get_widget("menu_quit", imageMenuItem);
    imageMenuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
    uiXml->get_widget("menu_about", imageMenuItem);
    imageMenuItem->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);

    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    uiXml->get_widget("menu_redo", menuRedo);
    uiXml->get_widget("menu_undo", menuUndo);
    menuUndo->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);
    menuRedo->get_child()->modify_fg(Gtk::STATE_NORMAL, textColour);

    CommandManager::Instance().setMenuItems(menuUndo, menuRedo);

    Gtk::SeparatorMenuItem *separatorMenuItem;
    uiXml->get_widget("separatormenuitem1", separatorMenuItem);
    separatorMenuItem->modify_bg(Gtk::STATE_NORMAL, bgColour);

    focusStealingButton->modify_bg(Gtk::STATE_NORMAL, bgColour);

    leftTickEntryBox->modify_base(Gtk::STATE_NORMAL, bgColour);
    leftTickEntryBox->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    leftTickEntryBox->modify_bg(Gtk::STATE_NORMAL, fgColour);
    rightTickEntryBox->modify_base(Gtk::STATE_NORMAL, bgColour);
    rightTickEntryBox->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    rightTickEntryBox->modify_bg(Gtk::STATE_NORMAL, fgColour);
    cursorTickEntryBox->modify_base(Gtk::STATE_NORMAL, bgColour);
    cursorTickEntryBox->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    cursorTickEntryBox->modify_bg(Gtk::STATE_NORMAL, fgColour);
    leftBarEntryBox->modify_base(Gtk::STATE_NORMAL, bgColour);
    leftBarEntryBox->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    leftBarEntryBox->modify_bg(Gtk::STATE_NORMAL, fgColour);
    rightBarEntryBox->modify_base(Gtk::STATE_NORMAL, bgColour);
    rightBarEntryBox->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    rightBarEntryBox->modify_bg(Gtk::STATE_NORMAL, fgColour);
    cursorBarEntryBox->modify_base(Gtk::STATE_NORMAL, bgColour);
    cursorBarEntryBox->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    cursorBarEntryBox->modify_bg(Gtk::STATE_NORMAL, fgColour);
    transportTimeEntry->modify_base(Gtk::STATE_NORMAL, bgColour);
    transportTimeEntry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    transportTimeEntry->modify_bg(Gtk::STATE_NORMAL, fgColour);

    bpmEntry->modify_base(Gtk::STATE_NORMAL, bgColour);
    bpmEntry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    bpmEntry->modify_bg(Gtk::STATE_NORMAL, fgColour);
    beatsPerBarEntry->modify_base(Gtk::STATE_NORMAL, bgColour);
    beatsPerBarEntry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    beatsPerBarEntry->modify_bg(Gtk::STATE_NORMAL, fgColour);
    barSubdivisionsEntry->modify_base(Gtk::STATE_NORMAL, bgColour);
    barSubdivisionsEntry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
    barSubdivisionsEntry->modify_bg(Gtk::STATE_NORMAL, fgColour);

    Gtk::Frame *bpmFrame;
    uiXml->get_widget("bpmFrame", bpmFrame);
    bpmFrame->modify_bg(Gtk::STATE_NORMAL, black);
}//setThemeColours

Gtk::Window *FMidiAutomationMainWindow::MainWindow()
{
    return mainWindow;
}//MainWindow

GraphState &FMidiAutomationMainWindow::getGraphState()
{
    return *graphState;
}//getGraphState

void FMidiAutomationMainWindow::handleEditSelectedInSeparateWindow()
{
    std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (selectedEntryBlock == nullptr) {
        return;
    }//if

    WindowManager &windowManager = WindowManager::Instance();
    std::shared_ptr<FMidiAutomationMainWindow> newWindow = windowManager.createWindow(true, selectedEntryBlock);

    newWindow->sequencer->deepCloneEntries(sequencer);

    std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlockClone = findWrappingEntryBlockUI(newWindow->sequencer, selectedEntryBlock->getBaseEntryBlock());
    assert(selectedEntryBlockClone != nullptr);

    std::cout << "11" << std::endl;

    newWindow->graphState->entryBlockSelectionState.ClearSelected();
    newWindow->graphState->entryBlockSelectionState.AddSelectedEntryBlock(selectedEntryBlockClone);

std::cout << "22" << std::endl;
    newWindow->handleCurveButtonPressedHelper(selectedEntryBlockClone);

std::cout << "33" << std::endl;
    newWindow->queue_draw();
}//handleEditSelectedInSeparateWindow

void FMidiAutomationMainWindow::forceCurveEditorMode(std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock)
{
    handleCurveButtonPressedHelper(selectedEntryBlock);
    queue_draw();
}//forceCurveEditorMode

bool FMidiAutomationMainWindow::handleEntryWindowScroll(Gtk::ScrollType scrollType, double pos)
{
    sequencer->notifyOnScroll(pos);
    queue_draw();
    return true;
}//handleEntryWindowScroll

void FMidiAutomationMainWindow::handleInsertModeChanged()
{
    Gtk::RadioButton *radioButton;
    uiXml->get_widget("radiobutton_merge", radioButton);

    bool isMerge = radioButton->get_active();
    if (true == isMerge) {
        getGraphState().insertMode = InsertMode::Merge;
    } else {
        getGraphState().insertMode = InsertMode::Replace;
    }//if
}//handleInsertModeChanged

void FMidiAutomationMainWindow::handleJackPressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();

    Gtk::ToggleButton *toggleButton;
    uiXml->get_widget("jackEnabledToggleButton", toggleButton);
    bool isActive = toggleButton->get_active();

    jackSingleton.setProcessingMidi(isActive);

    if (true == isActive) {
        setStatusText("Connected to Jack daemon");
    } else {
        setStatusText("Disconnected from Jack daemon");
    }//if
}//handleJackPressed

void FMidiAutomationMainWindow::handleRewPressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    if (false == jackSingleton.areProcessingMidi()) {
        setStatusText("Can't rewind while not connected to the Jack daemon");
        return;
    }//if

    Globals &globals = Globals::Instance();

    jackSingleton.setTime(0);

    cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(0));
    getGraphState().curPointerTick = 0;
    updateTempoBox(*graphState, globals.projectData, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
    getGraphState().setOffsetCenteredOnTick(0, drawingAreaWidth);
    getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    queue_draw();
}//handleRewPressed

void FMidiAutomationMainWindow::handlePlayPressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    if (false == jackSingleton.areProcessingMidi()) {
        setStatusText("Can't play while not connected to the Jack daemon");
        return;
    }//if

    jackSingleton.setTransportState(JackTransportRolling);
}//handlePlayPressed

void FMidiAutomationMainWindow::handlePausePressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    if (false == jackSingleton.areProcessingMidi()) {
        setStatusText("Can't pause while not connected to the Jack daemon");
        return;
    }//if

    jackSingleton.setTransportState(JackTransportStopped);
    jackSingleton.setRecordMidi(false);

    if (true == recordMidi) {
        processRecordedMidi();
    }//if

    recordMidi = false;
}//handlePausePressed

void FMidiAutomationMainWindow::handleRecordPressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    if (false == jackSingleton.areProcessingMidi()) {
        setStatusText("Can't record while not connected to the Jack daemon");
        return;
    }//if

    boost::function<void (void)> startRecordFunc = [=]() { startRecordThread(); };

    //FIXME: We probably have some issues when destroying the class and this thread..
    if (recordThread != nullptr) {
        recordThread->detach();
    }//if

    recordThread.reset(new std::thread(startRecordFunc));
}//handleRecordPressed

void FMidiAutomationMainWindow::startRecordThread()
{
    if (false == recordMidi) {
        for (unsigned int count = 4; count > 0; --count) {
            Glib::ustring statusText = "Starting record in ";
            statusText = statusText + boost::lexical_cast<Glib::ustring>(count);
            setStatusText(statusText);

            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }//if

        setStatusText(Glib::ustring("Recording started"));

        recordMidi = true;

        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setRecordMidi(true);
        jackSingleton.setTransportState(JackTransportRolling);

    } else {
        recordMidi = false;

        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setTransportState(JackTransportStopped);
        jackSingleton.setRecordMidi(false);

        setStatusText(Glib::ustring("Recording stopped"));

        processRecordedMidi();

////        std::vector<unsigned char> &recordedBuffer = getRecordBuffer();
    }//if
}//startRecordThread

void FMidiAutomationMainWindow::setStatusText(Glib::ustring text)
{
    {
        std::lock_guard<std::mutex> dataLock(statusTextDataMutex);
        statusTextAlpha = 1.0;
        currentStatusText = Glib::ustring("    ") + text;
        needsStatusTextUpdate = true;
    }

    statusTextMutex.unlock();
}//setStatusText

void FMidiAutomationMainWindow::statusTextThreadFunc()
{
    while (false == isExiting) {
        if (statusTextAlpha > 0.5) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            std::lock_guard<std::mutex> dataLock(statusTextDataMutex);
            statusTextAlpha -= 0.05;
        } else {
            if (statusTextAlpha > 0.1) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(500));
                std::lock_guard<std::mutex> dataLock(statusTextDataMutex);
                statusTextAlpha -= 0.01;
            } else {
                {
                    std::lock_guard<std::mutex> dataLock(statusTextDataMutex);
                    currentStatusText = Glib::ustring("");
                    needsStatusTextUpdate = true;
                }
                statusTextMutex.lock();
            }//if
        }//if

        {
            std::lock_guard<std::mutex> dataLock(statusTextDataMutex);
            needsStatusTextUpdate = true;
        }
    }//while
}//statusTextThreadFunc

void FMidiAutomationMainWindow::handleAddPressed()
{
    Globals &globals = Globals::Instance();

    if (false == globals.tempoGlobals.tempoDataSelected) {
        std::shared_ptr<Command> addSequencerEntryCommand(new AddSequencerEntryCommand(false, this));
        CommandManager::Instance().setNewCommand(addSequencerEntryCommand, true);
        trackListWindow->queue_draw();
    }//if

    if (true == globals.tempoGlobals.tempoDataSelected) {
        try {
            float bpm = boost::lexical_cast<float>(bpmEntry->get_text()) * 100;
            unsigned int beatsPerBar = boost::lexical_cast<unsigned int>(beatsPerBarEntry->get_text());
            unsigned int barSubDivisions = boost::lexical_cast<unsigned int>(barSubdivisionsEntry->get_text());

            if ((bpm <= 0) || (beatsPerBar <= 0) || (barSubDivisions <=0)) {
                return;
            }//if

            bool foundSelected = false;
            typedef std::pair<int, std::shared_ptr<Tempo> > TempoMarkerPair;
            for (TempoMarkerPair tempoMarkerPair : globals.projectData.getTempoChanges()) {
                if (true == tempoMarkerPair.second->currentlySelected) {
                    std::shared_ptr<Tempo> tempo = tempoMarkerPair.second;

                    //boost::function<void (void)> callback = boost::lambda::bind(&updateTempoChangesUIData, boost::lambda::var(datas->getTempoChanges()));
                    boost::function<void (void)> callback = [&]() { updateTempoChangesUIData(globals.projectData.getTempoChanges()); };
                    std::shared_ptr<Command> updateTempoChangeCommand(new UpdateTempoChangeCommand(tempo, (unsigned int)bpm, beatsPerBar, barSubDivisions, callback, this));
                    CommandManager::Instance().setNewCommand(updateTempoChangeCommand, true);

                    foundSelected = true;
                    break;
                }//if
            }//foreach

            if (false == foundSelected) {                
                auto tempoChanges = globals.projectData.getTempoChanges();

                //Essentially clear the selection state of the tempo changes
                (void)checkForTempoSelection(-100, tempoChanges);
        
                //XXX: This could be O(log n) if we proxied to the map
                int curPointerTick = getGraphState().curPointerTick;
                auto foundTempoChangeIter = std::find_if(tempoChanges.first, tempoChanges.second, 
                                                        [=](const std::pair<const int, std::shared_ptr<Tempo> > &a) 
                                                        { 
                                                            return a.first == curPointerTick; 
                                                        }//lambda
                                            );//find

                if (foundTempoChangeIter == tempoChanges.second) {
                    std::shared_ptr<Tempo> tempo(new Tempo);
                    tempo->bpm = (unsigned int)bpm;
                    tempo->beatsPerBar = beatsPerBar;
                    tempo->barSubDivisions = barSubDivisions;
                    tempo->currentlySelected = true;

                    //boost::function<void (void)> callback = boost::lambda::bind(&updateTempoChangesUIData, boost::lambda::var(datas->getTempoChanges()));
                    boost::function<void (void)> callback = [&]() { updateTempoChangesUIData(globals.projectData.getTempoChanges()); };
                    std::shared_ptr<Command> addTempoChangeCommand(new AddTempoChangeCommand(tempo, getGraphState().curPointerTick, callback, this));
                    CommandManager::Instance().setNewCommand(addTempoChangeCommand, true);
                }//if
            }//if

            queue_draw();
        } catch (...) {
            //Nothing
        }//try/catch
    }//if

    Gtk::ScrolledWindow *entryScrollWindow;
    uiXml->get_widget("entryScrolledWindow", entryScrollWindow);
    if (entryScrollWindow->get_vscrollbar() != nullptr) {
        entryScrollWindow->get_vscrollbar()->signal_change_value().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleEntryWindowScroll) );
    }//if
}//handleAddPressed

void FMidiAutomationMainWindow::handleDeletePressed()
{
    Globals &globals = Globals::Instance();

    if (true == globals.tempoGlobals.tempoDataSelected) {
        auto tempoChanges = globals.projectData.getTempoChanges();

        auto mapIter = tempoChanges.first;
        ++mapIter;

        for (/*nothing*/; mapIter != tempoChanges.second; ++mapIter) {
            if (true == mapIter->second->currentlySelected) {
                //boost::function<void (void)> callback = boost::lambda::bind(&updateTempoChangesUIData, boost::lambda::var(datas->getTempoChanges()));
                boost::function<void (void)> callback = [&]() { updateTempoChangesUIData(globals.projectData.getTempoChanges()); };
                std::shared_ptr<Command> deleteTempoChangeCommand(new DeleteTempoChangeCommand(getGraphState().curPointerTick, callback, this));
                CommandManager::Instance().setNewCommand(deleteTempoChangeCommand, true);

                queue_draw();
                break;
            }//if
        }//for
    } else {
        std::shared_ptr<SequencerEntryUI> entry = sequencer->getSelectedEntry();

        if (entry != nullptr) {
            std::shared_ptr<Command> deleteSequencerEntryCommand(new DeleteSequencerEntryCommand(entry, this));
            CommandManager::Instance().setNewCommand(deleteSequencerEntryCommand, true);
        }//if
    }//if
}//handleDeletePressed

void FMidiAutomationMainWindow::handleUpButtonPressed()
{
    std::shared_ptr<SequencerEntryUI> entry = sequencer->getSelectedEntry();

    if (entry != nullptr) {
        if (entry->getIndex() == 0) {
            return;
        }//if

        std::shared_ptr<Command> sequencerEntryUpCommand(new SequencerEntryUpCommand(entry, this));
        CommandManager::Instance().setNewCommand(sequencerEntryUpCommand, true);
    }//if
}//handleUpPressed

void FMidiAutomationMainWindow::handleDownButtonPressed()
{
    std::shared_ptr<SequencerEntryUI> entry = sequencer->getSelectedEntry();

    if (entry != nullptr) {
        if (entry->getIndex() == (sequencer->getNumEntries() - 1)) {
            return;
        }//if

        std::shared_ptr<Command> sequencerEntryDownCommand(new SequencerEntryDownCommand(entry, this));
        CommandManager::Instance().setNewCommand(sequencerEntryDownCommand, true);
    }//if
}//handleDownButtonPressed

void FMidiAutomationMainWindow::handleSequencerButtonPressed()
{
    Gtk::Frame *frame;
    uiXml->get_widget("selectedKeyframeFrame", frame);
    frame->set_visible(false);
    uiXml->get_widget("selectedEntryBlockFrame", frame);
    frame->set_visible(true);

    std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    assert(selectedEntryBlock != nullptr);

    selectedEntryBlock->setValuesPerPixel(getGraphState().valuesPerPixel);
    selectedEntryBlock->setOffsetY(getGraphState().offsetY);

    getGraphState().displayMode = DisplayMode::Sequencer;

    selectedEntryBlock->setUITickPositions(getGraphState().curPointerTick, getGraphState().leftMarkerTick, 
                                            getGraphState().rightMarkerTick );
    getGraphState().curPointerTick = getGraphState().lastSequencerPointerTick;
    getGraphState().leftMarkerTick = getGraphState().lastSequencerLeftPointerTick;
    getGraphState().rightMarkerTick = getGraphState().lastSequencerRightPointerTick;

    sequencerButton->set_sensitive(false);
    curveButton->set_sensitive(true);

    selectedKeyframeFrame->hide_all();

    positionValueEntry->hide_all();
    positionValueLabel->hide_all();

    positionTickEntry->property_editable() = true;

    PasteManager::Instance().clearCommand();

    if (getGraphState().entryBlockSelectionState.HasSelected() == true) {
        positionTickEntry->set_text(boost::lexical_cast<Glib::ustring>(getGraphState().entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->getStartTick()));
    } else {
        positionTickEntry->set_text("");
    }//if

    //Update edit menu
    Gtk::MenuItem *menuItem;
    uiXml->get_widget("menu_splitEntryBlock", menuItem);
    menuItem->set_visible(true);
    uiXml->get_widget("menu_joinEntryBlocks", menuItem);
    menuItem->set_visible(true);
    uiXml->get_widget("menu_pasteSEBToSelectedEntry", menuItem);
    menuItem->set_visible(true);
    uiXml->get_widget("menu_pasteSEBInstancesToSelectedEntry", menuItem);
    menuItem->set_visible(true);

    uiXml->get_widget("menu_resetTangents", menuItem);
    menuItem->set_visible(false);

    getGraphState().setOffsetCenteredOnTick(getGraphState().curPointerTick, drawingAreaWidth);
    getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    updateCursorTick(getGraphState().curPointerTick, false);
    queue_draw();
}//handleSequencerButtonPressed

void FMidiAutomationMainWindow::handleCurveButtonPressed()
{
    std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    handleCurveButtonPressedHelper(selectedEntryBlock);
}//handleCurveButtonPressed

void FMidiAutomationMainWindow::handleCurveButtonPressedHelper(std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock)
{
std::cout << "handleCurveButtonPressedHelper 1" << std::endl;

    //std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (selectedEntryBlock == nullptr) {
        return;
    }//if

    Gtk::Frame *frame;
    uiXml->get_widget("selectedKeyframeFrame", frame);
    frame->set_visible(true);
    uiXml->get_widget("selectedEntryBlockFrame", frame);
    frame->set_visible(false);

std::cout << "handleCurveButtonPressedHelper 2" << std::endl;

    positionTickEntry->property_editable() = false;

    positionValueEntry->set_text("");

std::cout << "handleCurveButtonPressedHelper 3" << std::endl;

    getGraphState().valuesPerPixel = selectedEntryBlock->getValuesPerPixel();
    getGraphState().offsetY = selectedEntryBlock->getOffsetY();

    getGraphState().displayMode = DisplayMode::Curve;
    getGraphState().lastSequencerPointerTick = getGraphState().curPointerTick;
    getGraphState().lastSequencerLeftPointerTick = getGraphState().leftMarkerTick;
    getGraphState().lastSequencerRightPointerTick = getGraphState().rightMarkerTick;
    std::tie(getGraphState().curPointerTick, getGraphState().leftMarkerTick, getGraphState().rightMarkerTick) = selectedEntryBlock->getUITickPositions();

    sequencerButton->set_sensitive(true);
    curveButton->set_sensitive(false);

    selectedKeyframeFrame->show_all();

    positionValueEntry->show_all();
    positionValueLabel->show_all();

std::cout << "handleCurveButtonPressedHelper 4" << std::endl;

    curveEditor->getKeySelection(*graphState, std::numeric_limits<int>::min(), std::numeric_limits<int>::min(), false, selectedEntryBlock);

    if (getGraphState().keyframeSelectionState.HasSelected() == true) {
        curveEditor->setKeyUIValues(uiXml, getGraphState().keyframeSelectionState.GetFirstKeyframe());
    } else {
        curveEditor->setKeyUIValues(uiXml, std::shared_ptr<Keyframe>());
    }//if

std::cout << "handleCurveButtonPressedHelper 5" << std::endl;

    PasteManager::Instance().clearCommand();

    //Update edit menu
    Gtk::MenuItem *menuItem;
    uiXml->get_widget("menu_splitEntryBlock", menuItem);
    menuItem->set_visible(false);
    uiXml->get_widget("menu_joinEntryBlocks", menuItem);
    menuItem->set_visible(false);
    uiXml->get_widget("menu_pasteSEBToSelectedEntry", menuItem);
    menuItem->set_visible(false);
    uiXml->get_widget("menu_pasteSEBInstancesToSelectedEntry", menuItem);
    menuItem->set_visible(false);

    uiXml->get_widget("menu_resetTangents", menuItem);
    menuItem->set_visible(true);

std::cout << "handleCurveButtonPressedHelper 6" << std::endl;

    getGraphState().setOffsetCenteredOnTick(getGraphState().curPointerTick, drawingAreaWidth);
std::cout << "handleCurveButtonPressedHelper 6b: " << drawingAreaWidth << " - " << drawingAreaHeight << std::endl;

    getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);

std::cout << "handleCurveButtonPressedHelper 6c" << std::endl;
    getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);

std::cout << "handleCurveButtonPressedHelper 6d" << std::endl;
    updateCursorTick(getGraphState().curPointerTick, false);

std::cout << "handleCurveButtonPressedHelper 7" << std::endl;

    queue_draw();
}//handleCurveButtonPressed

void FMidiAutomationMainWindow::unsetAllCurveFrames()
{
    focusStealingButton->grab_focus();

    Gdk::Color black;
    black.set_rgb(0, 0, 0);

    Gtk::Frame *bpmFrame;
    uiXml->get_widget("bpmFrame", bpmFrame);
    ////bpmFrame->get_parent()->modify_bg(Gtk::STATE_NORMAL, black);
    bpmFrameCheckButton->set_active(false);

    Globals &globals = Globals::Instance();
    globals.tempoGlobals.tempoDataSelected = false;
}//unsetAllCurveFrames

bool FMidiAutomationMainWindow::handleBPMFrameClick(GdkEventButton *event)
{
    handleBPMFrameClickBase();
    return false;
}//handleBPMFrameClick

void FMidiAutomationMainWindow::handleBPMFrameClickBase()
{
    sequencer->notifySelected(nullptr);

    Globals &globals = Globals::Instance();

    Gdk::Color yellow;
    yellow.set_rgb(65535, 65535, 0);

    Gtk::Frame *bpmFrame;
    uiXml->get_widget("bpmFrame", bpmFrame);
////    bpmFrame->get_parent()->modify_bg(Gtk::STATE_NORMAL, yellow);

    globals.tempoGlobals.tempoDataSelected = true;
    bpmFrameCheckButton->set_active(true);
}//handleBPMFrameClickBase

bool FMidiAutomationMainWindow::handleKeyEntryOnLeftTickEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    try {
        std::string entryText = leftTickEntryBox->get_text();
        int pos = boost::lexical_cast<int>(entryText);
        if ((pos == -1) || ((entryText.empty() == false) && (pos >= 0) && ((getGraphState().rightMarkerTick == -1) || (getGraphState().rightMarkerTick > pos)))) {
            getGraphState().leftMarkerTick = pos;
            queue_draw();
            return true;
        } else {
            return false;
        }//if
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnLeftTickEntryBox

bool FMidiAutomationMainWindow::handleKeyEntryOnRightTickEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    try {
        std::string entryText = rightTickEntryBox->get_text();
        int pos = boost::lexical_cast<int>(rightTickEntryBox->get_text());
        if ((pos == -1) || ((entryText.empty() == false) && (pos >= 0) && ((getGraphState().leftMarkerTick == -1) || (getGraphState().leftMarkerTick < pos)))) {
            getGraphState().rightMarkerTick = pos;
            queue_draw();
            return true;
        } else {
            return false;
        }//if
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnRightTickEntryBox

bool FMidiAutomationMainWindow::handleKeyEntryOnCursorTickEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    try {
        int pos = boost::lexical_cast<int>(cursorTickEntryBox->get_text());
        if (pos >= 0) {
            Globals &globals = Globals::Instance();

            getGraphState().curPointerTick = pos;
            updateTempoBox(*graphState, globals.projectData, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
            queue_draw();
            return true;
        } else {
            return false;
        }//if
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnCursorTickEntryBox

bool FMidiAutomationMainWindow::handleKeyEntryOnPositionTickEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    try {
        int curTick = boost::lexical_cast<int>(positionTickEntry->get_text());
        if (curTick >= 0) {
            curTick = std::max(0, curTick);
            getGraphState().entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->moveBlock(curTick);
            setTitleChanged();
            queue_draw();
            return true;
        } else {
            return false;
        }//if
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnPositionTickEntryBox

bool FMidiAutomationMainWindow::handleKeyEntryOnSelectedEntryBlockNameEntryBox(GdkEventKey *event)
{
    if ((event->keyval != GDK_Return) && (event->keyval != GDK_KP_Enter) && (event->keyval != GDK_ISO_Enter) && (event->keyval != GDK_3270_Enter)) {
        return false;
    }//if

    Gtk::Entry *entry;
    uiXml->get_widget("selectedEntryBlockNameEntry", entry);

    if (entry->get_text().empty() == false) {
        std::shared_ptr<Command> changeSequencerEntryBlockTitleCommand(new ChangeSequencerEntryBlockPropertiesCommand(getGraphState().entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock(), entry->get_text(), this));
        CommandManager::Instance().setNewCommand(changeSequencerEntryBlockTitleCommand, true);

        setTitleChanged();
        queue_draw();
    }//if

    return true;
}//handleKeyEntryOnSelectedEntryBlockNameEntryBox

void FMidiAutomationMainWindow::handleGraphResize(Gtk::Allocation &allocation)
{
std::cout << "**************************************************FMidiAutomationMainWindow::handleGraphResize " << this << std::endl;

    drawingAreaWidth = allocation.get_width();
    drawingAreaHeight = allocation.get_height();
 
std::cout << "drawingAreaWidth: " << drawingAreaWidth << " - " << "drawingAreaHeight: " << drawingAreaHeight << std::endl;

    getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    refreshGraphBackground();

    static bool firstTime = true;
    if (true == firstTime) {
        firstTime = false;
        doTestInit();
    }//if

    //sequencer can be nullptr on load / startup
    if (sequencer != nullptr) {
        sequencer->adjustFillerHeight();
    }//if
}//handleGraphResize

void FMidiAutomationMainWindow::on_menuCopy()
{
    if (getGraphState().displayMode == DisplayMode::Sequencer) {
        PasteManager::Instance().setPasteOnly(false);
        if (getGraphState().entryBlockSelectionState.HasSelected() == true) {
            std::shared_ptr<PasteSequencerEntryBlocksCommand> pasteSequencerEntryBlocksCommand(new PasteSequencerEntryBlocksCommand(getGraphState().entryBlockSelectionState.GetEntryBlocksMapCopy()));
            PasteManager::Instance().setNewCommand(pasteSequencerEntryBlocksCommand);
        }//if
    } else {
        PasteManager::Instance().setPasteOnly(true);
        if (getGraphState().keyframeSelectionState.HasSelected() == true) {
            std::shared_ptr<PasteSequencerKeyframesCommand> pasteSequencerKeyframesCommand(new PasteSequencerKeyframesCommand(getGraphState().keyframeSelectionState.GetSelectedKeyframesCopy()));
            PasteManager::Instance().setNewCommand(pasteSequencerKeyframesCommand);
        }//if
    }//if
}//on_menuCopy

void FMidiAutomationMainWindow::on_menuCut()
{
    if (getGraphState().displayMode == DisplayMode::Sequencer) {
        if (getGraphState().entryBlockSelectionState.HasSelected() == true) {
            on_menuCopy();
            handleDeleteSequencerEntryBlocks();
        }//if
    } else {
        if (getGraphState().keyframeSelectionState.HasSelected() == true) {
            on_menuCopy();
            curveEditor->handleDeleteKeyframes();
        }//if
    }//if
}//on_menuCut

void FMidiAutomationMainWindow::on_handleDelete()
{
    if (getGraphState().displayMode == DisplayMode::Sequencer) {
        if (getGraphState().entryBlockSelectionState.HasSelected() == true) {
            handleDeleteSequencerEntryBlocks();
        }//if
    } else {
        if (getGraphState().keyframeSelectionState.HasSelected() == true) {
            curveEditor->handleDeleteKeyframes();
        }//if
    }//if
}//on_handleDelete

void FMidiAutomationMainWindow::on_menuPaste()
{
    PasteManager::Instance().doPaste(std::shared_ptr<SequencerEntryUI>(), this);
    queue_draw();
}//on_menuPaste

void FMidiAutomationMainWindow::on_menuPasteInstance()
{
    PasteManager::Instance().doPasteInstance(std::shared_ptr<SequencerEntryUI>(), this);
    queue_draw();
}//on_menuPasteInstance

void FMidiAutomationMainWindow::on_menupasteSEBToSelectedEntry()
{
    std::shared_ptr<SequencerEntryUI> selectedEntry = sequencer->getSelectedEntry();
    PasteManager::Instance().doPaste(selectedEntry, this);
    queue_draw();
}//on_menupasteSEBToSelectedEntry

void FMidiAutomationMainWindow::on_menupasteSEBInstancesToSelectedEntry()
{
    std::shared_ptr<SequencerEntryUI> selectedEntry = sequencer->getSelectedEntry();
    PasteManager::Instance().doPasteInstance(selectedEntry, this);
    queue_draw();
}//on_menupasteSEBInstancesToSelectedEntry

void FMidiAutomationMainWindow::on_menuPorts()
{
    JackPortDialog portsDialog(uiXml);
}//on_menuPorts

void FMidiAutomationMainWindow::on_menuQuit()
{
    Gtk::Main::quit();
}//on_menuQuit

void FMidiAutomationMainWindow::on_menuSplitEntryBlocks()
{
    if (getGraphState().entryBlockSelectionState.HasSelected() == false) {
        return;
    }//if

    std::set<std::shared_ptr<SequencerEntryBlockUI> > origEntryBlocks;
    auto entryBlockPair = getGraphState().entryBlockSelectionState.GetCurrentlySelectedEntryBlocks();

    for (auto entryBlock : entryBlockPair) {
        if ( (entryBlock.second->getBaseEntryBlock()->getStartTick() < getGraphState().curPointerTick) && 
             (getGraphState().curPointerTick < entryBlock.second->getBaseEntryBlock()->getStartTick() + entryBlock.second->getBaseEntryBlock()->getDuration()) ) {
            origEntryBlocks.insert(entryBlock.second);
        }//if
    }//foreach

    if (origEntryBlocks.empty() == true) {
        return;
    }//if

    std::vector<std::shared_ptr<SequencerEntryBlockUI> > newEntryBlocks;
    for (auto entryBlockIter : entryBlockPair) {
        auto splitPair = entryBlockIter.second->getBaseEntryBlock()->deepCloneSplit(getGraphState().curPointerTick);

        std::shared_ptr<SequencerEntryBlockUI> split1(new SequencerEntryBlockUI(splitPair.first, entryBlockIter.second->getOwningEntry()));
        std::shared_ptr<SequencerEntryBlockUI> split2(new SequencerEntryBlockUI(splitPair.second, entryBlockIter.second->getOwningEntry()));

        newEntryBlocks.push_back(split1);
        newEntryBlocks.push_back(split2);
    }//foreach

    std::shared_ptr<Command> splitSequencerEntryBlocksCommand(new SplitSequencerEntryBlocksCommand(origEntryBlocks, newEntryBlocks, this));
    CommandManager::Instance().setNewCommand(splitSequencerEntryBlocksCommand, true);

    queue_draw();
}//on_menuSplitEntryBlocks

void FMidiAutomationMainWindow::on_menuJoinEntryBlocks()
{
    bool workToBeDone = false;
    auto entryBlockPair = getGraphState().entryBlockSelectionState.GetCurrentlySelectedEntryBlocks();

    std::map<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<std::deque<std::shared_ptr<SequencerEntryBlockUI> > > > entryBlockMap;

    for (auto entryBlock : entryBlockPair) {
        if (entryBlockMap[entryBlock.second->getOwningEntry()] == nullptr) {
            entryBlockMap[entryBlock.second->getOwningEntry()] = std::make_shared<std::deque<std::shared_ptr<SequencerEntryBlockUI> > >();
        }//if

        entryBlockMap[entryBlock.second->getOwningEntry()]->push_back(entryBlock.second);
    }//foreach

    std::vector<std::shared_ptr<SequencerEntryBlockUI> > origEntryBlocks;
    for (auto selectedIter : getGraphState().entryBlockSelectionState.GetCurrentlySelectedEntryBlocks()) {
        origEntryBlocks.push_back(selectedIter.second);
    }//for

    std::vector<std::shared_ptr<SequencerEntryBlockUI> > newEntryBlocks;
    for (auto entryQueuePair : entryBlockMap) {        
        std::deque<std::shared_ptr<SequencerEntryBlockUI> > &curDeque = *entryQueuePair.second;
        if (curDeque.size() > 1) {
            workToBeDone = true;
        } else {
            continue;
        }//if

        std::shared_ptr<SequencerEntryBlockUI> curEntryBlockUI = curDeque.front();
        std::shared_ptr<SequencerEntryBlock> curEntryBlockBase = curEntryBlockUI->getBaseEntryBlock();

        assert(curEntryBlockBase->getOwningEntry() == entryQueuePair.first->getBaseEntry());

        std::shared_ptr<SequencerEntryBlock> replacementBlock = curEntryBlockBase->deepClone(curEntryBlockBase->getOwningEntry(), curEntryBlockBase->getStartTick());
        std::shared_ptr<SequencerEntryBlockUI> replacementBlockUI(new SequencerEntryBlockUI(replacementBlock, entryQueuePair.first));
        newEntryBlocks.push_back(replacementBlockUI);
        curDeque.pop_front();

        while (curDeque.empty() == false) {
            std::shared_ptr<SequencerEntryBlockUI> firstBlock = curDeque.front();
            curDeque.pop_front();

            //Do the actual merge
            replacementBlock->getCurve()->mergeOtherAnimation(firstBlock->getBaseEntryBlock()->getCurve(), getGraphState().insertMode);
            replacementBlock->getSecondaryCurve()->mergeOtherAnimation(firstBlock->getBaseEntryBlock()->getSecondaryCurve(), getGraphState().insertMode);
        }//while
    }//foreach

    if (true == workToBeDone) {
        std::shared_ptr<Command> mergeSequencerEntryBlocksCommand(new MergeSequencerEntryBlocksCommand(origEntryBlocks, newEntryBlocks, this));
        CommandManager::Instance().setNewCommand(mergeSequencerEntryBlocksCommand, true);

        queue_draw();
    }//if
}//on_menuJoinEntryBlocks

namespace
{

int on_menuAlign_CursorHelper(std::shared_ptr<GraphState> graphState)
{
    int tick = 0;

    switch (graphState->displayMode) {        
        case DisplayMode::Curve:
            {
            std::shared_ptr<SequencerEntryBlockUI> firstBlock = graphState->entryBlockSelectionState.GetFirstEntryBlock();
            if (firstBlock == nullptr) {
                return -1;
            }//if

            std::shared_ptr<Keyframe> firstKeyframe = graphState->keyframeSelectionState.GetFirstKeyframe();
            if (firstKeyframe == nullptr) {
                return -1;
            }//if

            tick = firstKeyframe->tick + firstBlock->getBaseEntryBlock()->getStartTick();
            }
            break;

        case DisplayMode::Sequencer:
            {
            std::shared_ptr<SequencerEntryBlockUI> firstBlock = graphState->entryBlockSelectionState.GetFirstEntryBlock();
            if (firstBlock == nullptr) {
                return -1;
            }//if

            tick = firstBlock->getBaseEntryBlock()->getStartTick();
            }
            break;
    }//switch

    return tick;
}//on_menuAlign_CursorHelper

}//anonymous namespace

void FMidiAutomationMainWindow::on_menuAlignMainCursor()
{
    int tick = on_menuAlign_CursorHelper(graphState);
    if (tick < 0) {
        return;
    }//if

    graphState->curPointerTick = tick;
    updateCursorTick(getGraphState().curPointerTick, true);
    queue_draw();
}//on_menuAlignMainCursor

void FMidiAutomationMainWindow::on_menuAlignLeftCursor()
{
    int tick = on_menuAlign_CursorHelper(graphState);
    if (tick < 0) {
        return;
    }//if

    graphState->leftMarkerTick = tick;
    leftTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState->leftMarkerTick));
    queue_draw();
}//on_menuAlignLeftCursor

void FMidiAutomationMainWindow::on_menuAlignRightCursor()
{
    int tick = on_menuAlign_CursorHelper(graphState);
    if (tick < 0) {
        return;
    }//if

    graphState->rightMarkerTick = tick;
    rightTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState->rightMarkerTick));
    queue_draw();
}//on_menuAlignRightCursor

void FMidiAutomationMainWindow::on_menuNew()
{
    WindowManager &windowManager = WindowManager::Instance();
    std::shared_ptr<FMidiAutomationMainWindow> mainWindow = windowManager.getMainWindow();
    windowManager.closeAllWindows();

    Globals::ResetInstance();
    Globals &globals = Globals::Instance();

    mainWindow->currentFilename = "";
    mainWindow->setTitle("Unknown");

    mainWindow->getGraphState().doInit();

    Gtk::VBox *entryVBox;
    mainWindow->uiXml->get_widget("entryVBox", entryVBox);
    mainWindow->sequencer.reset(new SequencerUI(globals.projectData.getEntryGlade(), entryVBox, this));

    mainWindow->getGraphState().entryBlockSelectionState.ClearSelected();
    mainWindow->getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    mainWindow->getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);

    mainWindow->queue_draw();
}//on_menuNew

void FMidiAutomationMainWindow::on_menuSave()
{
    if (false == currentFilename.empty()) {
        std::string filename = Glib::locale_from_utf8(currentFilename);

        std::ofstream outputStream(filename.c_str());
        assert(outputStream.good());
        if (false == outputStream.good()) {
            return;
        }//if

        boost::archive::xml_oarchive outputArchive(outputStream);

        const unsigned int FMidiAutomationVersion = 1;
        outputArchive & BOOST_SERIALIZATION_NVP(FMidiAutomationVersion);

        Globals &globals = Globals::Instance();        
        outputArchive & BOOST_SERIALIZATION_NVP(globals);

        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.doSave(outputArchive);

        WindowManager &windowManager = WindowManager::Instance();
        windowManager.doSave(outputArchive);

        setTitle(currentFilename);

        MRUList &mruList = MRUList::Instance();
        mruList.addFile(currentFilename);
    } else {
        on_menuSaveAs();
    }//if

    queue_draw();
}//on_menuSave

void FMidiAutomationMainWindow::doSave(boost::archive::xml_oarchive &outputArchive)
{
    outputArchive & BOOST_SERIALIZATION_NVP(graphState);
    sequencer->doSave(outputArchive);

    int numEntryBlocksSelected = getGraphState().entryBlockSelectionState.GetNumSelected();
    outputArchive & BOOST_SERIALIZATION_NVP(numEntryBlocksSelected);

    if (0 != numEntryBlocksSelected) {
        for (auto entryBlockUIIter : getGraphState().entryBlockSelectionState.GetCurrentlySelectedEntryBlocks()) {
            std::shared_ptr<SequencerEntryBlock> entryBlockBase = entryBlockUIIter.second->getBaseEntryBlock();
            outputArchive & BOOST_SERIALIZATION_NVP(entryBlockBase);
        }//for
    }//if
}//doSave

void FMidiAutomationMainWindow::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    inputArchive & BOOST_SERIALIZATION_NVP(graphState);
    sequencer->doLoad(inputArchive);

    int numEntryBlocksSelected = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(numEntryBlocksSelected);

    getGraphState().entryBlockSelectionState.ClearSelected();

    for (int entryBlockIter = 0; entryBlockIter < numEntryBlocksSelected; ++entryBlockIter) {
        std::shared_ptr<SequencerEntryBlock> entryBlockBase;
        inputArchive & BOOST_SERIALIZATION_NVP(entryBlockBase);

        std::shared_ptr<SequencerEntryBlockUI> entryBlockUI = findWrappingEntryBlockUI(sequencer, entryBlockBase);
        assert(entryBlockUI != nullptr);

        getGraphState().entryBlockSelectionState.AddSelectedEntryBlock(entryBlockUI);
    }//for
}//doLoad

void FMidiAutomationMainWindow::on_menuSaveAs()
{
    Gtk::FileChooserDialog dialog("Save...", Gtk::FILE_CHOOSER_ACTION_SAVE);
//    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_normal;
    filter_normal.set_name("Automation files (*.fma)");
    filter_normal.add_pattern("*.fma");
    dialog.add_filter(filter_normal);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    int result = dialog.run();
    switch(result) {
        case(Gtk::RESPONSE_OK):
        {
            currentFilename = dialog.get_filename();
            if (currentFilename.find(".fma") == std::string::npos) {
                currentFilename.append(".fma");
            }//if
            on_menuSave();
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
            break;
        default:
            break;
    }//switch
}//on_menuSaveAs

void FMidiAutomationMainWindow::actuallyLoadFile(const Glib::ustring &currentFilename_)
{
    std::string filename = Glib::locale_from_utf8(currentFilename_);

    std::ifstream inputStream(filename.c_str());
    if (false == inputStream.good()) {
        //FIXME: We should probably add some sort of message box or something
        return;
    }//if

    WindowManager &windowManager = WindowManager::Instance();
    std::shared_ptr<FMidiAutomationMainWindow> mainWindow = windowManager.getMainWindow();
    windowManager.closeAllWindows();

    Globals::ResetInstance();

    ResetSharedPtrMapSingletonList();

    currentFilename = currentFilename_;

    boost::archive::xml_iarchive inputArchive(inputStream);

    unsigned int FMidiAutomationVersion = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(FMidiAutomationVersion);

/*    
//    inputArchive & BOOST_SERIALIZATION_NVP(datas);
    inputArchive & BOOST_SERIALIZATION_NVP(graphState);

Gtk::VBox *entryVBox;
uiXml->get_widget("entryVBox", entryVBox);
sequencer.reset(new SequencerUI(globals.projectData.getEntryGlade(), entryVBox, this));

JackSingleton &jackSingleton = JackSingleton::Instance();
jackSingleton.doLoad(inputArchive);

globals.projectData.getSequencer()->doLoad(inputArchive);
*/

    Globals &globals = Globals::Instance();        
    inputArchive & BOOST_SERIALIZATION_NVP(globals);

    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.doLoad(inputArchive);

    windowManager.doLoad(inputArchive);

    std::cout << "FINISHED LOADING BITS" << std::endl;

//    getGraphState().displayMode = DisplayMode::Sequencer;
//    getGraphState().selectedEntity = SelectedEntity::Nobody;

    setTitle(filename);

    trackListWindow->queue_draw();

    MRUList &mruList = MRUList::Instance();
    mruList.addFile(currentFilename_);

    getGraphState().entryBlockSelectionState.ClearSelected();
    getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    queue_draw();

    Gtk::ScrolledWindow *entryScrollWindow;
    uiXml->get_widget("entryScrolledWindow", entryScrollWindow);
    if (entryScrollWindow->get_vscrollbar() != nullptr) {
        entryScrollWindow->get_vscrollbar()->signal_change_value().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleEntryWindowScroll) );
    }//if
}//actuallyLoadFile

void FMidiAutomationMainWindow::on_menuOpen()
{
     Gtk::FileChooserDialog dialog("Load", Gtk::FILE_CHOOSER_ACTION_OPEN);
//    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_normal;
    filter_normal.set_name("Automation files (*.fma)");
    filter_normal.add_pattern("*.fma");
    dialog.add_filter(filter_normal);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    int result = dialog.run();
    switch(result) {
        case(Gtk::RESPONSE_OK):
        {
            currentFilename = dialog.get_filename();
            actuallyLoadFile(currentFilename);
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
            break;
        default:
            break;
    }//switch 
}//on_menuOpen

void FMidiAutomationMainWindow::on_menuUndo()
{
    CommandManager::Instance().doUndo();
    queue_draw();
}//on_menuUndo

void FMidiAutomationMainWindow::on_menuRedo()
{
    CommandManager::Instance().doRedo();
    queue_draw();
}//on_menuRedo

void FMidiAutomationMainWindow::updateCursorTick(int tick, bool updateJack)
{
    Globals &globals = Globals::Instance();

    getGraphState().curPointerTick = tick;
    getGraphState().curPointerTick = std::max(getGraphState().curPointerTick, 0);
    cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(getGraphState().curPointerTick));
    updateTempoBox(*graphState, globals.projectData, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);

    if (true == updateJack) {
        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setTime(getGraphState().curPointerTick);

        transportTimeEntry->set_text(boost::lexical_cast<std::string>(getGraphState().curPointerTick));
    }//if

    if (getGraphState().displayMode == DisplayMode::Sequencer) {
        if(sequencer->getSelectedEntry() != nullptr) {
            double sampledValueBase = sequencer->getSelectedEntry()->getBaseEntry()->sample(getGraphState().curPointerTick);
            int sampledValue = sampledValueBase + 0.5;
            if (sampledValueBase < 0) {
                sampledValue = sampledValueBase - 0.5;
            }//if

            currentSampledValue->set_text(boost::lexical_cast<std::string>(sampledValue));
        }//if
    }//if

    if (getGraphState().displayMode == DisplayMode::Curve) {
        if (getGraphState().entryBlockSelectionState.HasSelected() == true) {
            double sampledValueBase = getGraphState().entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->getOwningEntry()->sample(getGraphState().curPointerTick);
            int sampledValue = sampledValueBase + 0.5;
            if (sampledValueBase < 0) {
                sampledValue = sampledValueBase - 0.5;
            }//if

            currentSampledValue->set_text(boost::lexical_cast<std::string>(sampledValue));
        }//if
    }//if

    queue_draw();
}//updateCursorTick

bool FMidiAutomationMainWindow::key_pressed(GdkEventKey *event)
{
    switch (event->keyval) {
        case GDK_Shift_L:
        case GDK_Shift_R:
            shiftCurrentlyPressed = true;
            break;

        case GDK_Control_L:
        case GDK_Control_R:
            ctrlCurrentlyPressed = true;
            break;

        case GDK_Alt_L:
        case GDK_Alt_R:
            altCurrentlyPressed = true;
            break;

        case GDK_Delete:
        case GDK_KP_Delete:
        case GDK_BackSpace:            
            on_handleDelete();
            break;
    }//switch

    return false;
}//key_pressed

bool FMidiAutomationMainWindow::key_released(GdkEventKey *event)
{
    switch (event->keyval) {
        case GDK_Shift_L:
        case GDK_Shift_R:
            shiftCurrentlyPressed = false;
            break;

        case GDK_Control_L:
        case GDK_Control_R:
            ctrlCurrentlyPressed = false;
            break;

        case GDK_Alt_L:
        case GDK_Alt_R:
            altCurrentlyPressed = false;
            break;
    }//switch

    return false;
}//key_released

bool FMidiAutomationMainWindow::on_idle()
{
    if (true == isExiting) {
        return false;
    }//if

    JackSingleton &jackSingleton = JackSingleton::Instance();

    if (jackSingleton.getTransportState() == JackTransportRolling) {
        //static boost::posix_time::ptime lastTime(boost::posix_time::neg_infin);
        //boost::posix_time::ptime curTime = boost::posix_time::microsec_clock::universal_time();
    //    boost::posix_time::time_duration diffDuration = curTime - lastTime;
    //    if (diffDuration.total_milliseconds() < 20) {
    //        return true;
    //    } else {
          //  lastTime = curTime;
    //    }//if

        int curFrame = jackSingleton.getTransportFrame();
        if (getGraphState().curPointerTick != curFrame) {
            getGraphState().setOffsetCenteredOnTick(curFrame, drawingAreaWidth);
            getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
            updateCursorTick(curFrame, false);

//            cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(curFrame));

//            getGraphState().curPointerTick = curFrame;
//            updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
//            getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
//            queue_draw();
        }//if
    }//if

    if (true == needsStatusTextUpdate) {
        std::lock_guard<std::mutex> dataLock(statusTextDataMutex);
        statusBar->set_text(currentStatusText);

        Gdk::Color textColour;
        int colourComponent = 65535.0 * statusTextAlpha;
        textColour.set_rgb(colourComponent, colourComponent, colourComponent);

        statusBar->modify_fg(Gtk::STATE_NORMAL, textColour);
        needsStatusTextUpdate = false;
    }//if

    return true;
}//on_idle

void FMidiAutomationMainWindow::handleDeleteKeyframe()
{
    menuCopy->set_sensitive(false);
    menuCut->set_sensitive(false);

    curveEditor->handleDeleteKeyframes();
}//handleDeleteKeyframe

void FMidiAutomationMainWindow::handleAddSequencerEntryBlock()
{
    std::shared_ptr<SequencerEntryUI> selectedEntry = sequencer->getSelectedEntry();
    if (selectedEntry != nullptr) {
        if (selectedEntry->getBaseEntry()->getEntryBlock(getGraphState().curPointerTick) != nullptr) {
            return;
        }//if

        std::shared_ptr<SequencerEntryBlock> entryBlock(new SequencerEntryBlock(selectedEntry->getBaseEntry(), getGraphState().curPointerTick, std::shared_ptr<SequencerEntryBlock>()));

        std::shared_ptr<SequencerEntryBlockUI> entryBlockUI(new SequencerEntryBlockUI(entryBlock, selectedEntry));

        std::vector<std::pair<std::shared_ptr<SequencerEntryUI>, std::shared_ptr<SequencerEntryBlockUI>>> entryBlocks;
        entryBlocks.push_back(std::make_pair(selectedEntry, entryBlockUI));

        std::shared_ptr<Command> addSequencerEntryBlocksCommand(new AddSequencerEntryBlocksCommand(entryBlocks, this));
        CommandManager::Instance().setNewCommand(addSequencerEntryBlocksCommand, true);

        queue_draw();
    }//if
}//handleAddSequencerEntryBlock

void FMidiAutomationMainWindow::handleDeleteSequencerEntryBlocks()
{
    std::cout << "handleDeleteSequencerEntryBlocks" << std::endl;

    //std::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (getGraphState().entryBlockSelectionState.HasSelected() == true) {
        std::vector<std::shared_ptr<SequencerEntryBlockUI> > selectedSet;
        for (auto selectedIter : getGraphState().entryBlockSelectionState.GetCurrentlySelectedEntryBlocks()) {
            selectedSet.push_back(selectedIter.second);
        }//for

        std::shared_ptr<Command> deleteSequencerEntryBlocksCommand(new DeleteSequencerEntryBlocksCommand(selectedSet, this));
        CommandManager::Instance().setNewCommand(deleteSequencerEntryBlocksCommand, true);

        queue_draw();
    }//if
}//handleDeleteSequencerEntryBlock

/*
void FMidiAutomationMainWindow::handleDeleteSequencerEntryBlock()
{
std::cout << "handleDeleteSequencerEntryBlock 1" << std::endl;

    if (getGraphState().entryBlockSelectionState.HasSelected() == false) {
        return;
    }//if

std::cout << "handleDeleteSequencerEntryBlock 2" << std::endl;

    //std::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    std::shared_ptr<Command> deleteSequencerEntryBlockCommand(new DeleteSequencerEntryBlockCommand(getGraphState().entryBlockSelectionState.GetFirstEntryBlock(), this));
    CommandManager::Instance().setNewCommand(deleteSequencerEntryBlockCommand, true);

    queue_draw();
}//handleDeleteSequencerEntryBlock
*/

void FMidiAutomationMainWindow::handleSequencerEntryProperties()
{
    std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (selectedEntryBlock == nullptr) {
        return;
    }//if

    EntryBlockProperties entryBlockProperties(uiXml, selectedEntryBlock->getBaseEntryBlock());

    if (entryBlockProperties.wasChanged == true) {
        std::shared_ptr<Command> changeSequencerEntryBlockTitleCommand(new ChangeSequencerEntryBlockPropertiesCommand(selectedEntryBlock->getBaseEntryBlock(), entryBlockProperties.newTitle, this));
        CommandManager::Instance().setNewCommand(changeSequencerEntryBlockTitleCommand, true);

        queue_draw();
    }//if
}//handleSequencerEntryProperties

void FMidiAutomationMainWindow::handleSequencerEntryCurve()
{
    handleCurveButtonPressed();
}//handleSequencerEntryCurve

void FMidiAutomationMainWindow::editSequencerEntryProperties(std::shared_ptr<SequencerEntry> entry, bool createUpdatePoint)
{
std::cout << "editSequencerEntryProperties 1" << std::endl;
    EntryProperties entryProperties(uiXml, entry, !createUpdatePoint);

    if (true == entryProperties.wasChanged) {
 std::cout << "editSequencerEntryProperties 2" << std::endl;       
        if (true == createUpdatePoint) {
            std::shared_ptr<Command> changeSequencerEntryPropertiesCommand(new ChangeSequencerEntryPropertiesCommand(entry, entryProperties.origImpl, entryProperties.newImpl, this));
            CommandManager::Instance().setNewCommand(changeSequencerEntryPropertiesCommand, true);

            queue_draw();
        } else {
            //Here is we added a new entry
            entry->setNewDataImpl(entryProperties.newImpl);
        }//if
    }//if

std::cout << "editSequencerEntryProperties 3" << std::endl;   
}//editSequencerEntryProperties

void FMidiAutomationMainWindow::doTestInit()
{
    return; 

    /*
    std::shared_ptr<Command> addSequencerEntryCommand(new AddSequencerEntryCommand(sequencer, true));
    CommandManager::Instance().setNewCommand(addSequencerEntryCommand);

    //sequencer->notifySelected(boost::dynamic_pointer_cast<AddSequencerEntryCommand>(addSequencerEntryCommand)->entry.get());
    boost::dynamic_pointer_cast<AddSequencerEntryCommand>(addSequencerEntryCommand)->entry->select();

    handleAddSeqencerEntryBlock();

    std::shared_ptr<SequencerEntryBlock> entryBlock = boost::dynamic_pointer_cast<AddSequencerEntryCommand>(addSequencerEntryCommand)->entry->getEntryBlock(0);
    std::shared_ptr<SequencerEntryBlock> entryBlock2 = sequencer->getSelectedEntryBlock(-1, -1, true);

    assert(entryBlock == entryBlock2);
    
    getGraphState().selectedEntity = SequencerEntrySelection;
    getGraphState().currentlySelectedEntryOriginalStartTick = entryBlock->getStartTick();
    getGraphState().currentlySelectedEntryBlock = entryBlock;

    getGraphState().displayMode = DisplayMode::Curve;
    getGraphState().lastSequencerPointerTick = entryBlock->getStartTick(); //getGraphState().curPointerTick;
    getGraphState().curPointerTick = entryBlock->getStartTick();

    sequencerButton->set_sensitive(true);
    curveButton->set_sensitive(false);

    getGraphState().setOffsetCenteredOnTick(getGraphState().curPointerTick, drawingAreaWidth);
    getGraphState().refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    getGraphState().refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    updateCursorTick(getGraphState().curPointerTick, false);

    handleCurveButtonPressed();
    */
}//doTestInit


