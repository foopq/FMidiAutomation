#ifndef __FMIDIAUTOMATIONMAINWINDOW_H
#define __FMIDIAUTOMATIONMAINWINDOW_H

#include <gtkmm.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include "FMidiAutomationData.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>



struct Globals
{
    Globals();
    ~Globals();

    static Globals &Instance();

    std::string versionStr;

    std::string topBarFont;
    unsigned int topBarFontSize;
    bool darkTheme;
};//Globals

enum LineType
{
    BarStart,
    BarBeat,
    SubdivisionLine,
    SecondLine
};//LineType

struct GraphState
{    
    double baseOffset; //when actively scrolling
    bool inMotion; //when actively scrolling

    int zeroithTickPixel;
    double offset;
    int barsSubdivisionAmount;
    int ticksPerPixel; //negative means N pixels per tick
    std::vector<std::pair<unsigned int, LineType> > verticalLines;
    std::vector<std::pair<unsigned int, std::string> > upperLineText;
    std::vector<std::pair<unsigned int, std::string> > lowerLineText;

    std::vector<int> verticalPixelTickValues;
    std::vector<float> horizontalPixelValues;

    //Time at which the pointer is at
    int curPointerTick;

    GraphState();
    ~GraphState();

    void refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight);
};//GraphState

class FMidiAutomationMainWindow
{
    Gtk::Button *tmpAddButton;
    Gtk::Button *tmpRemoveButton;
 
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Window *mainWindow;
    Gtk::ScrolledWindow *trackListWindow;
    Gtk::DrawingArea *graphDrawingArea;
    Gtk::ImageMenuItem *menuOpen;
    Gtk::ImageMenuItem *menuSave;
    Gtk::ImageMenuItem *menuSaveAs;
    Gtk::ImageMenuItem *menuNew;
    Gtk::ImageMenuItem *menuQuit;

    int drawingAreaWidth;
    int drawingAreaHeight;
    Glib::ustring currentFilename;

    bool shiftCurrentlyPressed;
    bool ctrlCurrentlyPressed;
    bool altCurrentlyPressed;
    bool leftMouseCurrentlyPressed;
    gdouble mousePressDownX;
    gdouble mousePressDownY;
    
    boost::shared_ptr<Gtk::Image> backingImage;
    boost::shared_ptr<Gtk::Image> backingTexture;
    boost::shared_ptr<Gtk::Image> origBackingImage;
    boost::shared_ptr<Gtk::Image> origBackingTexture;

    std::vector <Gtk::Window *> automationTrackWindows;
    boost::shared_ptr<FMidiAutomationData> datas;

    GraphState graphState;
    
    void handleTmpAddButton();
    void handleTmpRemoveButton();
    
    void handleGraphResize(Gtk::Allocation&);
    
    void refreshGraphBackground();
    bool updateGraph(GdkEventExpose*);   

    void on_menuOpen();
    void on_menuSave();
    void on_menuSaveAs();
    void on_menuNew();
    void on_menuQuit();

    bool key_pressed(GdkEventKey *event);
    bool key_released(GdkEventKey *event);
    bool mouseButtonPressed(GdkEventButton *event);
    bool mouseButtonReleased(GdkEventButton *event);
    bool mouseMoved(GdkEventMotion *event);
    bool handleScroll(GdkEventScroll *event);

    void setThemeColours();
       
public:    
    FMidiAutomationMainWindow();
    ~FMidiAutomationMainWindow();
    
    Gtk::Window *MainWindow();
    
    
};//FMidiAutomationMainWindow

#endif
