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
class SequencerEntryBlock;
class SequencerEntry;
struct GraphState;

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
    GraphState *graphState;
    boost::shared_ptr<Sequencer> sequencer;

    TempoGlobals tempoGlobals;
};//Globals

enum LineType
{
    BarStart,
    BarBeat,
    SubdivisionLine,
    SecondLine,
    ValueLine,
};//LineType

enum SelectedEntity
{
    PointerTickBar,
    LeftTickBar,
    RightTickBar,
    TempoChange,
    SequencerEntrySelection,
    Nobody
};//SelectedEntity

namespace DisplayMode
{
enum DisplayMode
{
    Sequencer,
    Curve,
};//DisplayMode
}//DisplayMode

struct GraphState
{    
    double baseOffsetX; //when actively scrolling
    double baseOffsetY;
    bool inMotion; //when actively scrolling

    int zeroithTickPixel;
    double offsetX; //scroll offset
    double offsetY;
    int barsSubdivisionAmount;
    int ticksPerPixel; //negative means N pixels per tick
    double valuesPerPixel;
    std::vector<std::pair<unsigned int, LineType> > verticalLines;
    std::vector<std::pair<unsigned int, std::string> > upperLineText;
    std::vector<std::pair<unsigned int, LineType> > horizontalLines;
    std::vector<std::pair<unsigned int, std::string> > valueLineText;

    std::vector<int> verticalPixelTickValues;
    std::vector<double> horizontalPixelValues;

    SelectedEntity selectedEntity;

    //Time at which the pointer is at
    int curPointerTick;
    int curPointerTickXPixel; //how far over is it?   

    int leftMarkerTick;
    int rightMarkerTick;
    int leftMarkerTickXPixel;
    int rightMarkerTickXPixel;

    int currentlySelectedEntryOriginalStartTick;
    boost::shared_ptr<SequencerEntryBlock> currentlySelectedEntryBlock;

    DisplayMode::DisplayMode displayMode;
    int lastSequencerPointerTick; //for swaping back to the seqeucner

    GraphState();
    ~GraphState();
    void doInit();

    void refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight);
    void refreshHorizontalLines(unsigned int areaWidth, unsigned int areaHeight);
    void setOffsetCenteredOnTick(int tick, int drawingAreaWidth);
    void setOffsetCenteredOnValue(double value, int drawingAreaHeight);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
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
    Gtk::ImageMenuItem *menuCopy;
    Gtk::ImageMenuItem *menuCut;
    Gtk::ImageMenuItem *menuPaste;
    Gtk::ImageMenuItem *menuPasteInstance;
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
    Gtk::ToolButton *sequencerButton;
    Gtk::ToolButton *curveButton;

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
    void on_menuCopy();
    void on_menuCut();
    void on_menuPaste();
    void on_menuPasteInstance();


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
    void handleSequencerButtonPressed();
    void handleCurveButtonPressed();

    void handleRewPressed();
    void handlePlayPressed();
    void handlePausePressed();

    bool handleEntryWindowScroll(Gtk::ScrollType, double);

    void handleAddSeqencerEntryBlock();
    void handleDeleteSeqencerEntryBlock();
    void handleSequencerEntryProperties();
    void handleSequencerEntryCurve();

    void updateCursorTick(int tick, bool updateJack);

    void setThemeColours();

    bool on_idle();

    void setTitle(Glib::ustring currentFilename);
    void setTitleChanged();
       
public:    
    FMidiAutomationMainWindow();
    ~FMidiAutomationMainWindow();
    
    Gtk::Window *MainWindow();

    void doTestInit();

    void unsetAllCurveFrames();
    void editSequencerEntryProperties(boost::shared_ptr<SequencerEntry> entry, bool createUpdatePoint);
};//FMidiAutomationMainWindow

#endif
