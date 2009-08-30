#include <gtkmm.h>
#include <libglademm.h>
#include <iostream>
#include <fstream>
#include "FMidiAutomationMainWindow.h"
#include "FMidiAutomationData.h"
#include <boost/array.hpp>

namespace
{

void handleGraphTimeScroll(GdkEventMotion *event, GraphState &graphState, gdouble mousePressDownX, gdouble mousePressDownY, int drawingAreaWidth)
{
    gdouble offsetX = -(event->x - mousePressDownX);
//    gdouble offsetY = -(event->y - mousePressDownY);

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

void handleGraphTimeZoom(GdkScrollDirection direction, GraphState &graphState)
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

    graphState.ticksPerPixel = scrollLevels[curPos];
}//handleGraphTimeZoom

}//anonymous namespace

Globals::Globals()
{
    versionStr = "FMidiAutomation - version 1.0.0 - August 8th 2009";
    topBarFontSize = 12;
    topBarFont = "Arial";
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
    
    uiXml->get_widget("addButton", tmpAddButton);
    uiXml->get_widget("removeButton", tmpRemoveButton);
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

    mainWindow->signal_key_press_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::key_pressed));
    mainWindow->signal_key_release_event().connect(sigc::mem_fun(*this, &FMidiAutomationMainWindow::key_released));

    
    tmpAddButton->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleTmpAddButton) );
    tmpRemoveButton->signal_clicked().connect ( sigc::mem_fun(*this, &FMidiAutomationMainWindow::handleTmpRemoveButton) );

    shiftCurrentlyPressed = false;
    ctrlCurrentlyPressed = false;
    altCurrentlyPressed = false;
    leftMouseCurrentlyPressed = false;

    on_menuNew();
}//constructor

FMidiAutomationMainWindow::~FMidiAutomationMainWindow()
{
    //Nothing
}//destructor
    
Gtk::Window *FMidiAutomationMainWindow::MainWindow()
{
    return mainWindow;
}//MainWindow

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
    datas->tempoChanges.insert(std::make_pair(0U, Tempo(90, 4)));
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
    if ((event->button == 1) && (event->y > 60)) {
        leftMouseCurrentlyPressed = true;
        mousePressDownX = event->x;
        mousePressDownY = event->y;

        graphState.inMotion = true;
        graphState.baseOffset = graphState.offset;
    }//if

    return true;
}//mouseButtonPressed

bool FMidiAutomationMainWindow::mouseButtonReleased(GdkEventButton *event)
{
    if (event->button == 1) {
        leftMouseCurrentlyPressed = false;
        graphState.inMotion = false;
    }//if

    return true;
}//mouseButtonReleased

bool FMidiAutomationMainWindow::mouseMoved(GdkEventMotion *event)
{
    if (false == leftMouseCurrentlyPressed) {
        return false;
    }//if

    static guint32 lastHandledTime = 0; //XXX: This is safe to do, right? Only one thread ever gets here?

    if ((event->time - lastHandledTime) < 100) {
        return false;
    } else {
        lastHandledTime = event->time;
    }//if

    if (true == ctrlCurrentlyPressed) {
        gdouble curOffset = graphState.offset;

        handleGraphTimeScroll(event, graphState, mousePressDownX, mousePressDownY, drawingAreaWidth);

        if (graphState.offset != curOffset) {
            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphDrawingArea->queue_draw();
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

////    if (true == ctrlCurrentlyPressed) {
        int curTicksPerPixel = graphState.ticksPerPixel;

        handleGraphTimeZoom(event->direction, graphState);

        if (curTicksPerPixel != graphState.ticksPerPixel) {
            graphState.refreshVerticalLines(drawingAreaWidth, drawingAreaHeight);
            graphDrawingArea->queue_draw();
        }//if
////    }//if

    return true;
}//handleScroll

void FMidiAutomationMainWindow::handleTmpAddButton()
{
}//handleTmpAddButton

void FMidiAutomationMainWindow::handleTmpRemoveButton()
{
}//handleTmpRemoveButton
