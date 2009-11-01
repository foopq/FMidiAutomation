#include <gtkmm.h>
#include <libglademm.h>
#include <iostream>
#include <fstream>
#include "FMidiAutomationMainWindow.h"
#include "FMidiAutomationData.h"
#include "Command.h"
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp> 
#include "jack.h"
#include "Sequencer.h"

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

void handleGraphTimeScroll(GdkEventMotion *event, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaWidth)
{
    gdouble offsetX = -(event->x - mousePressDownX);
//    gdouble offsetY = -(event->y - mousePressDownY);

    if ((offsetX < 0) && (graphState.zeroithTickPixel != std::numeric_limits<int>::max()) && (graphState.zeroithTickPixel >= (drawingAreaWidth/2))) {
        return;
    }//if

    gdouble curOffset = graphState.offset;
    graphState.offset = graphState.baseOffset + offsetX;

    int tickCountStart = 0 * graphState.ticksPerPixel + graphState.offset * graphState.ticksPerPixel;
    int tickCountEnd = drawingAreaWidth * graphState.ticksPerPixel + graphState.offset * graphState.ticksPerPixel;

    if ((tickCountStart < 0) && (tickCountEnd > 0)) {
        int tickCountMiddle = (drawingAreaWidth / 2) * graphState.ticksPerPixel + graphState.offset * graphState.ticksPerPixel;
        if (tickCountMiddle < 0) {
            graphState.offset = curOffset;
        }//if
    }//if
}//handleGraphTimeScroll

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
    uiXml = Gtk::Builder::create_from_file("FMidiAutomation.glade");

    uiXml->get_widget("mainWindow", mainWindow);
    uiXml->get_widget("trackListWindow", trackListWindow);
    
    uiXml->get_widget("graphDrawingArea", graphDrawingArea);
    
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
    menuOpen->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuOpen));
    menuSave->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuSave));
    menuSaveAs->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuSaveAs));
    menuNew->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuNew));
    menuQuit->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuQuit));

    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuRedo;
    uiXml->get_widget("menu_redo", menuRedo);
    uiXml->get_widget("menu_undo", menuUndo);

    CommandManager::Instance().setMenuItems(menuUndo, menuRedo);

    menuUndo->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuUndo));
    menuRedo->signal_activate().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_menuRedo));

    mainWindow->signal_key_press_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::key_pressed));
    mainWindow->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::key_released));

    shiftCurrentlyPressed = false;
    ctrlCurrentlyPressed = false;
    altCurrentlyPressed = false;
    leftMouseCurrentlyPressed = false;

    on_menuNew();

    uiXml->get_widget("leftTickEntryBox", leftTickEntryBox);
    uiXml->get_widget("rightTickEntryBox", rightTickEntryBox);
    uiXml->get_widget("cursorTickEntryBox", cursorTickEntryBox);
    uiXml->get_widget("leftBarEntryBox", leftBarEntryBox);
    uiXml->get_widget("rightBarEntryBox", rightBarEntryBox);
    uiXml->get_widget("cursorBarEntryBox", cursorBarEntryBox);
    uiXml->get_widget("transportTimeEntry", transportTimeEntry);

    leftTickEntryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnLeftTickEntryBox));
    rightTickEntryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnRightTickEntryBox));
    cursorTickEntryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleKeyEntryOnCursorTickEntryBox));

    uiXml->get_widget("focusStealingButton", focusStealingButton);
    focusStealingButton->grab_focus();

    uiXml->get_widget("bpmEntry", bpmEntry);
    uiXml->get_widget("beatsPerBarEntry", beatsPerBarEntry);
    uiXml->get_widget("barSubdivisionsEntry", barSubdivisionsEntry);

    Gtk::Viewport *bpmFrame;
    uiXml->get_widget("viewport8", bpmFrame);
    bpmFrame->signal_button_press_event().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClick) );
    bpmEntry->signal_grab_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClickBase) );
    beatsPerBarEntry->signal_grab_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClickBase) );
    barSubdivisionsEntry->signal_grab_focus().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleBPMFrameClickBase) );

    Gtk::Label *statusBar;
    uiXml->get_widget("statusLabel", statusBar);
    statusBar->set_text("Welcome to FMidiAutomation");

    setThemeColours();

    datas.reset(new FMidiAutomationData);
    datas->addTempoChange(0U, boost::shared_ptr<Tempo>(new Tempo(12000, 4, 4)));

    Gtk::ToolButton *button;
    uiXml->get_widget("addButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleAddPressed) );
    uiXml->get_widget("deleteButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleDeletePressed) );

    Glib::RefPtr<Gtk::AccelGroup> accelGroup = mainWindow->get_accel_group();
    menuUndo->add_accelerator("activate", accelGroup, GDK_Z, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    menuRedo->add_accelerator("activate", accelGroup, GDK_R, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

//    Glib::signal_idle().connect( sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_idle) );
    Glib::signal_timeout().connect( sigc::mem_fun(*this, &FMidiAutomationMainWindow::on_idle), 20 );

    uiXml->get_widget("rewButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleRewPressed) );
    uiXml->get_widget("playButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handlePlayPressed) );
    uiXml->get_widget("pauseButton", button);
    button->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handlePausePressed) );

    datas->entryGlade = readEntryGlade();
    Gtk::VBox *entryVBox;
    uiXml->get_widget("entryVBox", entryVBox);
    sequencer.reset(new Sequencer(datas->entryGlade, entryVBox));

    Gtk::ScrolledWindow *entryScrollWindow;
    uiXml->get_widget("entryScrolledWindow", entryScrollWindow);
    entryScrollWindow->get_vscrollbar()->signal_change_value().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleEntryWindowScroll) );

}//constructor

FMidiAutomationMainWindow::~FMidiAutomationMainWindow()
{
    //Nothing
}//destructor
    
void FMidiAutomationMainWindow::setThemeColours()
{
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

        if (x < 11) {
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

bool FMidiAutomationMainWindow::handleEntryWindowScroll(Gtk::ScrollType scrollType, double pos)
{
    sequencer->notifyOnScroll(pos);
    return true;
}//handleEntryWindowScroll

void FMidiAutomationMainWindow::handleRewPressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.setTime(0);

    cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(0));
    graphState.curPointerTick = 0;
    updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
    graphState.setOffsetCenteredOnTick(0, drawingAreaWidth);
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    graphDrawingArea->queue_draw();
}//handleRewPressed

void FMidiAutomationMainWindow::handlePlayPressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.setTransportState(JackTransportRolling);
}//handlePlayPressed

void FMidiAutomationMainWindow::handlePausePressed()
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.setTransportState(JackTransportStopped);
}//handlePausePressed

void FMidiAutomationMainWindow::handleAddPressed()
{
    Globals &globals = Globals::Instance();

    if (false == globals.tempoGlobals.tempoDataSelected) {
        boost::shared_ptr<Command> addSequencerEntryCommand(new AddSequencerEntryCommand(sequencer));
        CommandManager::Instance().setNewCommand(addSequencerEntryCommand);
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
                    CommandManager::Instance().setNewCommand(updateTempoChangeCommand);

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
                    CommandManager::Instance().setNewCommand(addTempoChangeCommand);
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
                CommandManager::Instance().setNewCommand(deleteTempoChangeCommand);

                graphDrawingArea->queue_draw();
                break;
            }//if
        }//for
    } else {
        boost::shared_ptr<SequencerEntry> entry = sequencer->getSelectedEntry();

        if (entry != NULL) {
            boost::shared_ptr<Command> deleteSequencerEntryCommand(new DeleteSequencerEntryCommand(sequencer, entry));
            CommandManager::Instance().setNewCommand(deleteSequencerEntryCommand);
        }//if
    }//if
}//handleDeletePressed

void FMidiAutomationMainWindow::unsetAllCurveFrames()
{
    focusStealingButton->grab_focus();

    Gdk::Color black;
    black.set_rgb(0, 0, 0);

    Gtk::Frame *bpmFrame;
    uiXml->get_widget("bpmFrame", bpmFrame);
    bpmFrame->modify_bg(Gtk::STATE_NORMAL, black);

    Globals &globals = Globals::Instance();
    globals.tempoGlobals.tempoDataSelected = true;
}//unsetAllCurveFrames

bool FMidiAutomationMainWindow::handleBPMFrameClick(GdkEventButton *event)
{
    handleBPMFrameClickBase();
    return false;
}//handleBPMFrameClick

void FMidiAutomationMainWindow::handleBPMFrameClickBase()
{
    Globals &globals = Globals::Instance();

    Gdk::Color yellow;
    yellow.set_rgb(65535, 65535, 0);

    Gtk::Frame *bpmFrame;
    uiXml->get_widget("bpmFrame", bpmFrame);
    bpmFrame->modify_bg(Gtk::STATE_NORMAL, yellow);

    globals.tempoGlobals.tempoDataSelected = true;
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

void FMidiAutomationMainWindow::handleGraphResize(Gtk::Allocation &allocation)
{
    drawingAreaWidth = allocation.get_width();
    drawingAreaHeight = allocation.get_height();
    
    graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
    refreshGraphBackground();
}//handleGraphResize

void FMidiAutomationMainWindow::on_menuQuit()
{
    Gtk::Main::quit();
}//on_menuQuit

void FMidiAutomationMainWindow::on_menuNew()
{
    datas.reset(new FMidiAutomationData);
    datas->addTempoChange(0U, boost::shared_ptr<Tempo>(new Tempo(12000, 4, 4)));
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
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_normal;
    filter_normal.set_name("Automation files");
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

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    int result = dialog.run();
    switch(result) {
        case(Gtk::RESPONSE_OK):
        {
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
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
            break;
        default:
            break;
    }//switch 

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

bool FMidiAutomationMainWindow::mouseButtonPressed(GdkEventButton *event)
{
    switch (event->button) {
        case 1: //Left
        {
            graphState.selectedEntity = Nobody;
            leftMouseCurrentlyPressed = true;
            mousePressDownX = event->x;
            mousePressDownY = event->y;

            if (event->y > 60) {
                graphState.inMotion = true;
                graphState.baseOffset = graphState.offset;

                if (false == ctrlCurrentlyPressed) {
                    unsetAllCurveFrames();
                }//if
            } else {
                if ((event->y > 30) && (false == ctrlCurrentlyPressed)) {
                    if ((graphState.leftMarkerTickXPixel >= 0) && (abs(event->x - graphState.leftMarkerTickXPixel) <= 5)) {
                        graphState.selectedEntity = LeftTickBar;
                    }//if
                    else if ((graphState.rightMarkerTickXPixel >= 0) && (abs(event->x - graphState.rightMarkerTickXPixel) <= 5)) {
                        graphState.selectedEntity = RightTickBar;
                    }//if
                    else if (abs(event->x - graphState.curPointerTickXPixel) <= 5) {
                        graphState.selectedEntity = PointerTickBar;
                    }//if
                    else if (checkForTempoSelection(event->x, datas->tempoChanges) == true) {
                        graphState.selectedEntity = TempoChange;
                        handleBPMFrameClickBase();
                        updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
                        graphDrawingArea->queue_draw();
                    }//if

                    else {
                        //Essentially clear the selection state of the tempo changes
                        (void)checkForTempoSelection(-100, datas->tempoChanges);
                    }//if
                }//if
            }//if

        break;
        }

        case 3: //Right
        {
            mousePressDownX = event->x;
            mousePressDownY = event->y;

        break;
        }

        default:
            break;
    }//switch

    return true;
}//mouseButtonPressed

bool FMidiAutomationMainWindow::mouseButtonReleased(GdkEventButton *event)
{
    switch (event->button) {
        case 1: //Left
        {
            leftMouseCurrentlyPressed = false;
            graphState.inMotion = false;

            if ((event->y > 30) && (event->y <= 60) && (event->y == mousePressDownY) && (abs(event->x -mousePressDownX) <= 5)) {
                if (false == ctrlCurrentlyPressed) {
                    if (graphState.selectedEntity != TempoChange) {
                        updateCursorTick(graphState.verticalPixelTickValues[event->x], true);
                    }//if
                } else {
                    if ((graphState.rightMarkerTick == -1) || (graphState.rightMarkerTick > graphState.verticalPixelTickValues[event->x])) {
                        graphState.leftMarkerTick = graphState.verticalPixelTickValues[event->x];
                        graphState.leftMarkerTick = std::max(graphState.leftMarkerTick, 0);
                        leftTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.leftMarkerTick));
                        graphDrawingArea->queue_draw();
                    }//if
                }//if
            }//if
        break;
        }

        case 3: //Right
        {
            if ((event->y > 30) && (event->y <= 60) && (event->y == mousePressDownY) && (abs(event->x - mousePressDownX) <= 5)) {
                if ((graphState.leftMarkerTick == -1) || (graphState.leftMarkerTick < graphState.verticalPixelTickValues[event->x])) {
                    graphState.rightMarkerTick = graphState.verticalPixelTickValues[event->x];
                    graphState.rightMarkerTick = std::max(graphState.rightMarkerTick, 0);
                    rightTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.rightMarkerTick));
                    graphDrawingArea->queue_draw();
                }//if
            }//if
        break;
        }

        default:
            break;
    }//switch

    return true;
}//mouseButtonReleased

bool FMidiAutomationMainWindow::mouseMoved(GdkEventMotion *event)
{
    if (false == leftMouseCurrentlyPressed) {
        return false;
    }//if

    static guint32 lastHandledTime = 0; //XXX: This is safe to do, right? Only one thread ever gets here?

    if ((event->time - lastHandledTime) < 20) {
        return false;
    } else {
        lastHandledTime = event->time;
    }//if

    if (true == ctrlCurrentlyPressed) {
        if (event->y > 60) {
           //We are scrolling the canvas
            gdouble curOffset = graphState.offset;

            handleGraphTimeScroll(event, graphState, mousePressDownX, mousePressDownY, drawingAreaWidth);

            if (graphState.offset != curOffset) {
                graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
                graphDrawingArea->queue_draw();
            }//if
        }//if
    } else {
        if (graphState.selectedEntity == PointerTickBar) {
            //Did we set the current time pointer
            if ((event->x >= 0) && (event->x < drawingAreaWidth)) {
                updateCursorTick(graphState.verticalPixelTickValues[event->x], true);
            }//if
        }//if

        else if (graphState.selectedEntity == LeftTickBar) {
            //Did we set the current time pointer
            if ((event->x >= 0) && (event->x < drawingAreaWidth) && ((graphState.rightMarkerTick == -1) || (graphState.verticalPixelTickValues[event->x] < graphState.rightMarkerTick))) {
                graphState.leftMarkerTick = graphState.verticalPixelTickValues[event->x];
                graphState.leftMarkerTick = std::max(graphState.leftMarkerTick, 0);
                leftTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.leftMarkerTick));
                graphDrawingArea->queue_draw();
            }//if
        }//if

        else if (graphState.selectedEntity == RightTickBar) {
            //Did we set the current time pointer
            if ((event->x >= 0) && (event->x < drawingAreaWidth) && ((graphState.leftMarkerTick == -1) || (graphState.verticalPixelTickValues[event->x] > graphState.leftMarkerTick))) {
                graphState.rightMarkerTick = graphState.verticalPixelTickValues[event->x];
                graphState.rightMarkerTick = std::max(graphState.rightMarkerTick, 0);
                rightTickEntryBox->set_text(boost::lexical_cast<std::string>(graphState.rightMarkerTick));
                graphDrawingArea->queue_draw();
            }//if
        }//if
    }//if
  
    return true;
}//mouseMoved

bool FMidiAutomationMainWindow::handleScroll(GdkEventScroll *event)
{
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
            graphDrawingArea->queue_draw();
        }//if
    }//if

    return true;
}//handleScroll

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
            updateCursorTick(curFrame, false);

//            cursorTickEntryBox->set_text(boost::lexical_cast<std::string>(curFrame));

//            graphState.curPointerTick = curFrame;
//            updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);
//            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
//            graphDrawingArea->queue_draw();
        }//if
    }//if

    return true;
}//on_idle


