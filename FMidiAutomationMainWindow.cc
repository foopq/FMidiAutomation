#include <gtkmm.h>
#include <libglademm.h>
#include <iostream>
#include <fstream>
#include "FMidiAutomationMainWindow.h"
#include "FMidiAutomationData.h"
#include "FMidiAutomationCurveEditor.h"
#include "Command.h"
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include "jack.h"
#include "Sequencer.h"
#include "EntryBlockProperties.h"
#include "PasteManager.h"
#include "EntryProperties.h"
#include "Animation.h"
#include "jackPortDialog.h"

namespace
{

Glib::ustring readEntryGlade()
{
    std::ifstream inputStream("FMidiAutomationEntry.glade");
    assert(inputStream.good());
    if (false == inputStream.good()) {
        return "";
    }//if

    Glib::ustring retString;
    std::string line;
    while (std::getline(inputStream,line)) {
        retString += line;
    }//while

    return retString;
}//readEntryGlade

}//anonymous namespace

Globals::Globals()
{
    versionStr = "FMidiAutomation - version 1.0.0 - October 2009";
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
    static Globals globals;
    return globals;
}//Instance

FMidiAutomationMainWindow::FMidiAutomationMainWindow()
{
    Globals &globals = Globals::Instance();

    uiXml = Gtk::Builder::create_from_file("FMidiAutomation.glade");

    curveEditor.reset(new CurveEditor(this, uiXml));

    uiXml->get_widget("mainWindow", mainWindow);
    uiXml->get_widget("trackListWindow", trackListWindow);
    
    uiXml->get_widget("graphDrawingArea", graphDrawingArea);
    globals.graphDrawingArea = graphDrawingArea;
    globals.graphState = &graphState;

    graphState.displayMode = DisplayMode::Sequencer;
    
    backingImage.reset(new Gtk::Image());
    backingTexture.reset(new Gtk::Image());
    origBackingImage.reset(new Gtk::Image());
    origBackingTexture.reset(new Gtk::Image());
    
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file("pics/background.tga");
    origBackingTexture->set(pixbuf);
    pixbuf = Gdk::Pixbuf::create_from_file("pics/fractal.tga");
    origBackingImage->set(pixbuf);
    
    graphDrawingArea->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

    graphDrawingArea->signal_size_allocate().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleGraphResize) );
    graphDrawingArea->signal_expose_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::updateGraph) );
    graphDrawingArea->signal_button_press_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::mouseButtonPressed) );
    graphDrawingArea->signal_button_release_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::mouseButtonReleased) );
    graphDrawingArea->signal_motion_notify_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::mouseMoved) );
    graphDrawingArea->signal_scroll_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleScroll) );

    uiXml->get_widget("menu_open", menuOpen);
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

    menuSplitEntryBlock->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuSplitEntryBlock));
    menuJoinEntryBlocks->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuJoinEntryBlocks));

    menuCopy->set_sensitive(false);
    menuCut->set_sensitive(false);
    menuPaste->set_sensitive(false);
    menuPasteInstance->set_sensitive(false);

    menuCopy->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuCopy));
    menuCut->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuCut));
    menuPaste->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuPaste));
    menuPasteInstance->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuPasteInstance));

    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    uiXml->get_widget("menu_redo", menuRedo);
    uiXml->get_widget("menu_undo", menuUndo);

    CommandManager::Instance().setMenuItems(menuUndo, menuRedo);
    PasteManager::Instance().setMenuItems(menuPaste, menuPasteInstance);

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
 
    uiXml->get_widget("statusLabel", statusBar);
    statusTextAlpha = 1.0;
    needsStatusTextUpdate = false;
    setStatusText(Glib::ustring("Welcome to FMidiAutomation"));
    boost::function<void (void)> statusThreadFunc = boost::lambda::bind(boost::mem_fn(&FMidiAutomationMainWindow::statusTextThreadFunc), this);
    statusTextThread = boost::thread(statusThreadFunc);

    setThemeColours();

    datas.reset(new FMidiAutomationData);
    datas->addTempoChange(0U, boost::shared_ptr<Tempo>(new Tempo(12000, 4, 4)));

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
    Glib::signal_timeout().connect( sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_idle), 20 );

    uiXml->get_widget("rewButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleRewPressed) );
    uiXml->get_widget("playButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handlePlayPressed) );
    uiXml->get_widget("pauseButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handlePausePressed) );
    uiXml->get_widget("recButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleRecordPressed) );

    datas->entryGlade = readEntryGlade();

    Gtk::VBox *entryVBox;
    uiXml->get_widget("entryVBox", entryVBox);
    sequencer.reset(new Sequencer(datas->entryGlade, entryVBox, this));
    globals.sequencer = sequencer;

    Gtk::ScrolledWindow *entryScrollWindow;
    uiXml->get_widget("entryScrolledWindow", entryScrollWindow);
    entryScrollWindow->get_vscrollbar()->signal_change_value().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleEntryWindowScroll) );

    boost::function<void (void)> titleStarFunc = boost::lambda::bind(boost::mem_fn(&FMidiAutomationMainWindow::setTitleChanged), this);
    CommandManager::Instance().setTitleStar(titleStarFunc);

    setTitle("Unknown");

    Gtk::ToggleButton *toggleButton;
    uiXml->get_widget("jackEnabledToggleButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleJackPressed) );
    toggleButton->set_active(true);

    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.setTime(0);
}//constructor

FMidiAutomationMainWindow::~FMidiAutomationMainWindow()
{
    //Nothing
}//destructor
 
void FMidiAutomationMainWindow::queue_draw()
{
    graphDrawingArea->queue_draw();
}//queue_draw

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
    return graphState;
}//getGraphState

bool FMidiAutomationMainWindow::handleEntryWindowScroll(Gtk::ScrollType scrollType, double pos)
{
    sequencer->notifyOnScroll(pos);
    graphDrawingArea->queue_draw();
    return true;
}//handleEntryWindowScroll

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

    jackSingleton.setTime(0);

    cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(0));
    graphState.curPointerTick = 0;
    updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
    graphState.setOffsetCenteredOnTick(0, drawingAreaWidth);
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    graphDrawingArea->queue_draw();
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

    boost::function<void (void)> startRecordFunc = boost::lambda::bind(boost::mem_fn(&FMidiAutomationMainWindow::startRecordThread), this);

    if (recordThread != NULL) {
        recordThread->detach();
    }//if

    recordThread.reset(new boost::thread(startRecordFunc));
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
    needsStatusTextUpdate = true;
    statusTextAlpha = 1.0;
    currentStatusText = Glib::ustring("    ") + text;
    needsStatusTextUpdate = true;

    statusTextMutex.unlock();
}//setStatusText

void FMidiAutomationMainWindow::statusTextThreadFunc()
{
    while (1) {
        if (statusTextAlpha > 0.5) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            statusTextAlpha -= 0.05;
        } else {
            if (statusTextAlpha > 0.1) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(500));
                statusTextAlpha -= 0.01;
            } else {
                currentStatusText = Glib::ustring("");
                needsStatusTextUpdate = true;
                statusTextMutex.lock();
            }//if
        }//if

        needsStatusTextUpdate = true;
    }//while
}//statusTextThreadFunc

void FMidiAutomationMainWindow::handleAddPressed()
{
    Globals &globals = Globals::Instance();

    if (false == globals.tempoGlobals.tempoDataSelected) {
        boost::shared_ptr<Command> addSequencerEntryCommand(new AddSequencerEntryCommand(sequencer, false));
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
            typedef std::pair<int, boost::shared_ptr<Tempo> > TempoMarkerPair;
            BOOST_FOREACH(TempoMarkerPair tempoMarkerPair, datas->tempoChanges) {
                if (true == tempoMarkerPair.second->currentlySelected) {
                    boost::shared_ptr<Tempo> tempo = tempoMarkerPair.second;

                    boost::function<void (void)> callback = boost::lambda::bind(&updateTempoChangesUIData, boost::lambda::var(datas->tempoChanges));
                    boost::shared_ptr<Command> updateTempoChangeCommand(new UpdateTempoChangeCommand(tempo, (unsigned int)bpm, beatsPerBar, barSubDivisions, callback));
                    CommandManager::Instance().setNewCommand(updateTempoChangeCommand, true);

                    foundSelected = true;
                    break;
                }//if
            }//foreach

            if (false == foundSelected) {
                //Essentially clear the selection state of the tempo changes
                (void)checkForTempoSelection(-100, datas->tempoChanges);
        
                if (datas->tempoChanges.find(graphState.curPointerTick) == datas->tempoChanges.end()) {
                    boost::shared_ptr<Tempo> tempo(new Tempo);
                    tempo->bpm = (unsigned int)bpm;
                    tempo->beatsPerBar = beatsPerBar;
                    tempo->barSubDivisions = barSubDivisions;
                    tempo->currentlySelected = true;

                    boost::function<void (void)> callback = boost::lambda::bind(&updateTempoChangesUIData, boost::lambda::var(datas->tempoChanges));
                    boost::shared_ptr<Command> addTempoChangeCommand(new AddTempoChangeCommand(tempo, graphState.curPointerTick, datas, callback));
                    CommandManager::Instance().setNewCommand(addTempoChangeCommand, true);
                }//if
            }//if

            graphDrawingArea->queue_draw();
        } catch (...) {
            //Nothing
        }//try/catch
    }//if
}//handleAddPressed

void FMidiAutomationMainWindow::handleDeletePressed()
{
    Globals &globals = Globals::Instance();

    if (true == globals.tempoGlobals.tempoDataSelected) {
        std::map<int, boost::shared_ptr<Tempo> >::iterator mapIter = datas->tempoChanges.begin();
        ++mapIter;

        for (/*nothing*/; mapIter != datas->tempoChanges.end(); ++mapIter) {
            if (true == mapIter->second->currentlySelected) {
                boost::function<void (void)> callback = boost::lambda::bind(&updateTempoChangesUIData, boost::lambda::var(datas->tempoChanges));
                boost::shared_ptr<Command> deleteTempoChangeCommand(new DeleteTempoChangeCommand(graphState.curPointerTick, datas, callback));
                CommandManager::Instance().setNewCommand(deleteTempoChangeCommand, true);

                graphDrawingArea->queue_draw();
                break;
            }//if
        }//for
    } else {
        boost::shared_ptr<SequencerEntry> entry = sequencer->getSelectedEntry();

        if (entry != NULL) {
            boost::shared_ptr<Command> deleteSequencerEntryCommand(new DeleteSequencerEntryCommand(sequencer, entry));
            CommandManager::Instance().setNewCommand(deleteSequencerEntryCommand, true);
        }//if
    }//if
}//handleDeletePressed

void FMidiAutomationMainWindow::handleUpButtonPressed()
{
    boost::shared_ptr<SequencerEntry> entry = sequencer->getSelectedEntry();

    if (entry != NULL) {
        if (entry->getIndex() == 0) {
            return;
        }//if

        boost::shared_ptr<Command> sequencerEntryUpCommand(new SequencerEntryUpCommand(sequencer, entry));
        CommandManager::Instance().setNewCommand(sequencerEntryUpCommand, true);
    }//if
}//handleUpPressed

void FMidiAutomationMainWindow::handleDownButtonPressed()
{
    boost::shared_ptr<SequencerEntry> entry = sequencer->getSelectedEntry();

    if (entry != NULL) {
        if (entry->getIndex() == (sequencer->getNumEntries() - 1)) {
            return;
        }//if

        boost::shared_ptr<Command> sequencerEntryDownCommand(new SequencerEntryDownCommand(sequencer, entry));
        CommandManager::Instance().setNewCommand(sequencerEntryDownCommand, true);
    }//if
}//handleDownButtonPressed

void FMidiAutomationMainWindow::handleSequencerButtonPressed()
{
    boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    assert(selectedEntryBlock != NULL);

    selectedEntryBlock->setValuesPerPixel(graphState.valuesPerPixel);
    selectedEntryBlock->setOffsetY(graphState.offsetY);

    graphState.displayMode = DisplayMode::Sequencer;
    graphState.curPointerTick = graphState.lastSequencerPointerTick;

    sequencerButton->set_sensitive(false);
    curveButton->set_sensitive(true);

    selectedKeyframeFrame->hide_all();

    positionValueEntry->hide_all();
    positionValueLabel->hide_all();

    positionTickEntry->property_editable() = true;

    PasteManager::Instance().clearCommand();

    graphState.setOffsetCenteredOnTick(graphState.curPointerTick, drawingAreaWidth);
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    updateCursorTick(graphState.curPointerTick, false);
    graphDrawingArea->queue_draw();
}//handleSequencerButtonPressed

void FMidiAutomationMainWindow::handleCurveButtonPressed()
{
    boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (selectedEntryBlock == NULL) {
        return;
    }//if

    positionTickEntry->property_editable() = false;

    positionValueEntry->set_text("");

    graphState.valuesPerPixel = selectedEntryBlock->getValuesPerPixel();
    graphState.offsetY = selectedEntryBlock->getOffsetY();

    graphState.displayMode = DisplayMode::Curve;
    graphState.lastSequencerPointerTick = selectedEntryBlock->getStartTick(); //graphState.curPointerTick;
    graphState.curPointerTick = selectedEntryBlock->getStartTick();

    sequencerButton->set_sensitive(true);
    curveButton->set_sensitive(false);

    selectedKeyframeFrame->show_all();

    positionValueEntry->show_all();
    positionValueLabel->show_all();

    graphState.currentlySelectedKeyframe = curveEditor->getKeySelection(graphState, std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
    curveEditor->setKeyUIValues(uiXml, graphState.currentlySelectedKeyframe);

    PasteManager::Instance().clearCommand();

    graphState.setOffsetCenteredOnTick(graphState.curPointerTick, drawingAreaWidth);
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    updateCursorTick(graphState.curPointerTick, false);
    graphDrawingArea->queue_draw();
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
    sequencer->notifySelected(NULL);

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
        if ((pos == -1) || ((entryText.empty() == false) && (pos >= 0) && ((graphState.rightMarkerTick == -1) || (graphState.rightMarkerTick > pos)))) {
            graphState.leftMarkerTick = pos;
            graphDrawingArea->queue_draw();
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
        if ((pos == -1) || ((entryText.empty() == false) && (pos >= 0) && ((graphState.leftMarkerTick == -1) || (graphState.leftMarkerTick < pos)))) {
            graphState.rightMarkerTick = pos;
            graphDrawingArea->queue_draw();
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
            graphState.curPointerTick = pos;
            updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
            graphDrawingArea->queue_draw();
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
            graphState.getCurrentlySelectedEntryBlock()->moveBlock(curTick);
            graphDrawingArea->queue_draw();
            return true;
        } else {
            return false;
        }//if
    } catch(...) {
        return false;
    }//try/catch
}//handleKeyEntryOnPositionTickEntryBox

void FMidiAutomationMainWindow::handleGraphResize(Gtk::Allocation &allocation)
{
    drawingAreaWidth = allocation.get_width();
    drawingAreaHeight = allocation.get_height();
 
    std::cout << "graph resize: " << drawingAreaHeight << std::endl;

    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    refreshGraphBackground();

    static bool firstTime = true;
    if (true == firstTime) {
        firstTime = false;
        doTestInit();
        std::cout << "TEST INIT" << std::endl;
    }//if
}//handleGraphResize

void FMidiAutomationMainWindow::on_menuCopy()
{
    if (graphState.displayMode == DisplayMode::Sequencer) {
        PasteManager::Instance().setPasteOnly(false);
        if (graphState.currentlySelectedEntryBlocks.empty() == false) {
            boost::shared_ptr<PasteSequencerEntryBlockCommand> pasteSequencerEntryBlockCommand(new PasteSequencerEntryBlockCommand(graphState.currentlySelectedEntryBlocks));
            PasteManager::Instance().setNewCommand(pasteSequencerEntryBlockCommand);
        }//if
    } else {
        PasteManager::Instance().setPasteOnly(true);
        if (graphState.currentlySelectedKeyframe != NULL) {
            boost::shared_ptr<PasteSequencerKeyframeCommand> pasteSequencerKeyframeCommand(new PasteSequencerKeyframeCommand(graphState.currentlySelectedKeyframe));
            PasteManager::Instance().setNewCommand(pasteSequencerKeyframeCommand);
        }//if
    }//if
}//on_menuCopy

void FMidiAutomationMainWindow::on_menuCut()
{
    if (graphState.displayMode == DisplayMode::Sequencer) {
        if (graphState.currentlySelectedEntryBlocks.empty() == false) {
            handleDeleteSequencerEntryBlocks();
            on_menuCopy();
        }//if
    } else {
        if (graphState.currentlySelectedKeyframe != NULL) {
            on_menuCopy();
            curveEditor->handleDeleteKeyframe();
        }//if
    }//if
}//on_menuCut

void FMidiAutomationMainWindow::on_menuPaste()
{
    PasteManager::Instance().doPaste();
    graphDrawingArea->queue_draw();
}//on_menuPaste

void FMidiAutomationMainWindow::on_menuPasteInstance()
{
    PasteManager::Instance().doPasteInstance();
    graphDrawingArea->queue_draw();
}//on_menuPasteInstance

void FMidiAutomationMainWindow::on_menuPorts()
{
    JackPortDialog portsDialog(uiXml);
}//on_menuPorts

void FMidiAutomationMainWindow::on_menuQuit()
{
    Gtk::Main::quit();
}//on_menuQuit

void FMidiAutomationMainWindow::on_menuSplitEntryBlock()
{
    if (graphState.getCurrentlySelectedEntryBlock() == NULL) {
        return;
    }//if

////
//graphState.currentlySelectedEntryBlock = graphState.currentlySelectedEntryBlock->getOwningEntry()->splitEntryBlock(graphState.currentlySelectedEntryBlock, graphState.curPointerTick).first; ...
}//on_menuSplitEntryBlock

void FMidiAutomationMainWindow::on_menuJoinEntryBlocks()
{
}//on_menuJoinEntryBlock

void FMidiAutomationMainWindow::on_menuNew()
{
    Globals &globals = Globals::Instance();

    datas.reset(new FMidiAutomationData);
    datas->addTempoChange(0U, boost::shared_ptr<Tempo>(new Tempo(12000, 4, 4)));
    datas->entryGlade = readEntryGlade();

    currentFilename = "";
    setTitle("Unknown");

    graphState.doInit();

    Gtk::VBox *entryVBox;
    uiXml->get_widget("entryVBox", entryVBox);
    sequencer.reset(new Sequencer(datas->entryGlade, entryVBox, this));
    globals.sequencer = sequencer;
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

        outputArchive & BOOST_SERIALIZATION_NVP(datas);
        outputArchive & BOOST_SERIALIZATION_NVP(graphState);

        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.doSave(outputArchive);

        sequencer->doSave(outputArchive);

        setTitle(currentFilename);
    } else {
        on_menuSaveAs();
    }//if

    graphDrawingArea->queue_draw();
}//on_menuSave

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
            Globals &globals = Globals::Instance();

            currentFilename = dialog.get_filename();
            std::string filename = Glib::locale_from_utf8(currentFilename);
            std::ifstream inputStream(filename.c_str());
            assert(inputStream.good());
            if (false == inputStream.good()) {
                return;
            }//if

            boost::archive::xml_iarchive inputArchive(inputStream);

            unsigned int FMidiAutomationVersion = 0;
            inputArchive & BOOST_SERIALIZATION_NVP(FMidiAutomationVersion);

            inputArchive & BOOST_SERIALIZATION_NVP(datas);
            inputArchive & BOOST_SERIALIZATION_NVP(graphState);

            datas->entryGlade = readEntryGlade();

            Gtk::VBox *entryVBox;
            uiXml->get_widget("entryVBox", entryVBox);
            sequencer.reset(new Sequencer(datas->entryGlade, entryVBox, this));
            globals.sequencer = sequencer;

            JackSingleton &jackSingleton = JackSingleton::Instance();
            jackSingleton.doLoad(inputArchive);

            sequencer->doLoad(inputArchive);

            graphState.displayMode = DisplayMode::Sequencer;
            graphState.selectedEntity = Nobody;

            datas->entryGlade = readEntryGlade();

            setTitle(currentFilename);

            trackListWindow->queue_draw();
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
            break;
        default:
            break;
    }//switch 

    graphState.currentlySelectedEntryBlocks.clear();
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    graphDrawingArea->queue_draw();
}//on_menuOpen

void FMidiAutomationMainWindow::on_menuUndo()
{
    CommandManager::Instance().doUndo();
    graphDrawingArea->queue_draw();
}//on_menuUndo

void FMidiAutomationMainWindow::on_menuRedo()
{
    CommandManager::Instance().doRedo();
    graphDrawingArea->queue_draw();
}//on_menuRedo

void FMidiAutomationMainWindow::updateCursorTick(int tick, bool updateJack)
{
    graphState.curPointerTick = tick;
    graphState.curPointerTick = std::max(graphState.curPointerTick, 0);
    cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.curPointerTick));
    updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);

    if (true == updateJack) {
        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setTime(graphState.curPointerTick);

        transportTimeEntry->set_text(boost::lexical_cast<std::string>(graphState.curPointerTick));
    }//if

    if (graphState.displayMode == DisplayMode::Sequencer) {
        if(sequencer->getSelectedEntry() != NULL) {
            double sampledValueBase = sequencer->getSelectedEntry()->sample(graphState.curPointerTick);
            int sampledValue = sampledValueBase + 0.5;
            if (sampledValueBase < 0) {
                sampledValue = sampledValueBase - 0.5;
            }//if

            currentSampledValue->set_text(boost::lexical_cast<std::string>(sampledValue));
        }//if
    }//if

    if (graphState.displayMode == DisplayMode::Curve) {
        if (graphState.getCurrentlySelectedEntryBlock() != NULL) {
            double sampledValueBase = graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->sample(graphState.curPointerTick);
            int sampledValue = sampledValueBase + 0.5;
            if (sampledValueBase < 0) {
                sampledValue = sampledValueBase - 0.5;
            }//if

            currentSampledValue->set_text(boost::lexical_cast<std::string>(sampledValue));
        }//if
    }//if

    graphDrawingArea->queue_draw();
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
    JackSingleton &jackSingleton = JackSingleton::Instance();

    if (jackSingleton.getTransportState() == JackTransportRolling) {
        static boost::posix_time::ptime lastTime(boost::posix_time::neg_infin);
        boost::posix_time::ptime curTime = boost::posix_time::microsec_clock::universal_time();
        boost::posix_time::time_duration diffDuration = curTime - lastTime;
    //    if (diffDuration.total_milliseconds() < 20) {
    //        return true;
    //    } else {
            lastTime = curTime;
    //    }//if

        int curFrame = jackSingleton.getTransportFrame();
        if (graphState.curPointerTick != curFrame) {
            graphState.setOffsetCenteredOnTick(curFrame, drawingAreaWidth);
            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
            updateCursorTick(curFrame, false);

//            cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(curFrame));

//            graphState.curPointerTick = curFrame;
//            updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
//            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
//            graphDrawingArea->queue_draw();
        }//if
    }//if

    if (true == needsStatusTextUpdate) {
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

    curveEditor->handleDeleteKeyframe();
}//handleDeleteKeyframe

void FMidiAutomationMainWindow::handleAddSequencerEntryBlock()
{
    boost::shared_ptr<SequencerEntry> selectedEntry = sequencer->getSelectedEntry();
    if (selectedEntry != NULL) {
        if (selectedEntry->getEntryBlock(graphState.curPointerTick) != NULL) {
            return;
        }//if

        boost::shared_ptr<SequencerEntryBlock> entryBlock(new SequencerEntryBlock(selectedEntry, graphState.curPointerTick, boost::shared_ptr<SequencerEntryBlock>()));

        boost::shared_ptr<Command> addSequencerEntryBlockCommand(new AddSequencerEntryBlockCommand(selectedEntry, entryBlock));
        CommandManager::Instance().setNewCommand(addSequencerEntryBlockCommand, true);

        graphDrawingArea->queue_draw();
    }//if
}//handleAddSequencerEntryBlock

void FMidiAutomationMainWindow::handleDeleteSequencerEntryBlocks()
{
    //boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (graphState.currentlySelectedEntryBlocks.empty() == false) {
        boost::shared_ptr<Command> deleteSequencerEntryBlocksCommand(new DeleteSequencerEntryBlocksCommand(graphState.currentlySelectedEntryBlocks));
        CommandManager::Instance().setNewCommand(deleteSequencerEntryBlocksCommand, true);

        graphDrawingArea->queue_draw();
    }//if
}//handleDeleteSequencerEntryBlock

void FMidiAutomationMainWindow::handleDeleteSequencerEntryBlock()
{
    if (graphState.currentlySelectedEntryBlocks.empty() == true) {
        return;
    }//if

    //boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (graphState.currentlySelectedEntryBlocks.empty() == false) {
        boost::shared_ptr<Command> deleteSequencerEntryBlockCommand(new DeleteSequencerEntryBlockCommand(graphState.currentlySelectedEntryBlocks.begin()->second));
        CommandManager::Instance().setNewCommand(deleteSequencerEntryBlockCommand, true);

        graphDrawingArea->queue_draw();
    }//if
}//handleDeleteSequencerEntryBlock

void FMidiAutomationMainWindow::handleSequencerEntryProperties()
{
    boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock = sequencer->getSelectedEntryBlock();
    if (selectedEntryBlock == NULL) {
        return;
    }//if

    EntryBlockProperties entryBlockProperties(uiXml, selectedEntryBlock);

    if (entryBlockProperties.wasChanged == true) {
        boost::shared_ptr<Command> changeSequencerEntryBlockTitleCommand(new ChangeSequencerEntryBlockPropertiesCommand(selectedEntryBlock, entryBlockProperties.newTitle));
        CommandManager::Instance().setNewCommand(changeSequencerEntryBlockTitleCommand, true);

        graphDrawingArea->queue_draw();
    }//if
}//handleSequencerEntryProperties

void FMidiAutomationMainWindow::handleSequencerEntryCurve()
{
    handleCurveButtonPressed();
}//handleSequencerEntryCurve

void FMidiAutomationMainWindow::editSequencerEntryProperties(boost::shared_ptr<SequencerEntry> entry, bool createUpdatePoint)
{

std::cout << "editSequencerEntryProperties 1" << std::endl;
    EntryProperties entryProperties(uiXml, entry, !createUpdatePoint);



    if (true == entryProperties.wasChanged) {
 std::cout << "editSequencerEntryProperties 2" << std::endl;       
        if (true == createUpdatePoint) {
            boost::shared_ptr<Command> changeSequencerEntryPropertiesCommand(new ChangeSequencerEntryPropertiesCommand(entry, entryProperties.origImpl, entryProperties.newImpl));
            CommandManager::Instance().setNewCommand(changeSequencerEntryPropertiesCommand, true);

            graphDrawingArea->queue_draw();
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
    boost::shared_ptr<Command> addSequencerEntryCommand(new AddSequencerEntryCommand(sequencer, true));
    CommandManager::Instance().setNewCommand(addSequencerEntryCommand);

    //sequencer->notifySelected(boost::dynamic_pointer_cast<AddSequencerEntryCommand>(addSequencerEntryCommand)->entry.get());
    boost::dynamic_pointer_cast<AddSequencerEntryCommand>(addSequencerEntryCommand)->entry->select();

    handleAddSeqencerEntryBlock();

    boost::shared_ptr<SequencerEntryBlock> entryBlock = boost::dynamic_pointer_cast<AddSequencerEntryCommand>(addSequencerEntryCommand)->entry->getEntryBlock(0);
    boost::shared_ptr<SequencerEntryBlock> entryBlock2 = sequencer->getSelectedEntryBlock(-1, -1, true);

    assert(entryBlock == entryBlock2);
    
    graphState.selectedEntity = SequencerEntrySelection;
    graphState.currentlySelectedEntryOriginalStartTick = entryBlock->getStartTick();
    graphState.currentlySelectedEntryBlock = entryBlock;

    graphState.displayMode = DisplayMode::Curve;
    graphState.lastSequencerPointerTick = entryBlock->getStartTick(); //graphState.curPointerTick;
    graphState.curPointerTick = entryBlock->getStartTick();

    sequencerButton->set_sensitive(true);
    curveButton->set_sensitive(false);

    graphState.setOffsetCenteredOnTick(graphState.curPointerTick, drawingAreaWidth);
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphState.refreshHorizontalLines(drawingAreaWidth, drawingAreaHeight);
    updateCursorTick(graphState.curPointerTick, false);

    handleCurveButtonPressed();
    */
}//doTestInit




