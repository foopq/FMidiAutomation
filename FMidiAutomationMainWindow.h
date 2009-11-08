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
#include <jack/transport.h>

struct TempoGlobals;
class Sequencer;

struct Globals
{
    Globals();
    ~Globals();

    static Globals &Instance();

    std::string versionStr;

    std::string topBarFont;
    unsigned int topBarFontSize;

    std::string bottomBarFont;
    unsigned int bottomBarFontSize;

    bool darkTheme;

    Gtk::DrawingArea *graphDrawingArea;

    TempoGlobals tempoGlobals;
};//Globals

enum LineType
{
    BarStart,
    BarBeat,
    SubdivisionLine,
    SecondLine
};//LineType

enum SelectedEntity
{
    PointerTickBar,
    LeftTickBar,
    RightTickBar,
    TempoChange,
    Nobody
};//SelectedEntity

struct GraphState
{    
    double baseOffset; //when actively scrolling
    bool inMotion; //when actively scrolling

    int zeroithTickPixel;
    double offset; //scroll offset
    int barsSubdivisionAmount;
    int ticksPerPixel; //negative means N pixels per tick
    std::vector<std::pair<unsigned int, LineType> > verticalLines;
    std::vector<std::pair<unsigned int, std::string> > upperLineText;

    std::vector<int> verticalPixelTickValues;
    std::vector<float> horizontalPixelValues;

    SelectedEntity selectedEntity;

    //Time at which the pointer is at
    int curPointerTick;
    int curPointerTickXPixel; //how far over is it?   

    int leftMarkerTick;
    int rightMarkerTick;
    int leftMarkerTickXPixel;
    int rightMarkerTickXPixel;

    GraphState();
    ~GraphState();

    void refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight);
    void setOffsetCenteredOnTick(int tick, int drawingAreaWidth);
};//GraphState

class FMidiAutomationMainWindow
{
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Window *mainWindow;
    Gtk::ScrolledWindow *trackListWindow;
    Gtk::DrawingArea *graphDrawingArea;
    Gtk::ImageMenuItem *menuOpen;
    Gtk::ImageMenuItem *menuSave;
    Gtk::ImageMenuItem *menuSaveAs;
    Gtk::ImageMenuItem *menuNew;
    Gtk::ImageMenuItem *menuQuit;
    Gtk::Entry *leftTickEntryBox;
    Gtk::Entry *rightTickEntryBox;
    Gtk::Entry *cursorTickEntryBox;
    Gtk::Entry *leftBarEntryBox;
    Gtk::Entry *rightBarEntryBox;
    Gtk::Entry *cursorBarEntryBox;
    Gtk::Button *focusStealingButton;
    Gtk::Entry *bpmEntry;
    Gtk::Entry *beatsPerBarEntry;
    Gtk::Entry *barSubdivisionsEntry;
    Gtk::Entry *transportTimeEntry;

    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
    Gtk::Menu *m_pMenuPopup;

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
    boost::shared_ptr<Sequencer> sequencer;
    
    void handleGraphResize(Gtk::Allocation&);
    
    void refreshGraphBackground();
    bool updateGraph(GdkEventExpose*);   

    void on_menuOpen();
    void on_menuSave();
    void on_menuSaveAs();
    void on_menuNew();
    void on_menuQuit();
    void on_menuUndo();
    void on_menuRedo();

    bool key_pressed(GdkEventKey *event);
    bool key_released(GdkEventKey *event);
    bool mouseButtonPressed(GdkEventButton *event);
    bool mouseButtonReleased(GdkEventButton *event);
    bool mouseMoved(GdkEventMotion *event);
    bool handleScroll(GdkEventScroll *event);

    bool handleKeyEntryOnLeftTickEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnRightTickEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnCursorTickEntryBox(GdkEventKey *event);

    bool handleBPMFrameClick(GdkEventButton *event);
    void handleBPMFrameClickBase();

    void handleAddPressed();
    void handleDeletePressed();
    void handleUpButtonPressed();
    void handleDownButtonPressed();

    void handleRewPressed();
    void handlePlayPressed();
    void handlePausePressed();

    bool handleEntryWindowScroll(Gtk::ScrollType, double);

    void handleAddSeqencerEntryBlock();
    void handleDeleteSeqencerEntryBlock();
    void handleSequencerEntryProperties();

    void updateCursorTick(int tick, bool updateJack);

    void setThemeColours();

    bool on_idle();
       
public:    
    FMidiAutomationMainWindow();
    ~FMidiAutomationMainWindow();
    
    Gtk::Window *MainWindow();

    void unsetAllCurveFrames();
};//FMidiAutomationMainWindow

#endif
