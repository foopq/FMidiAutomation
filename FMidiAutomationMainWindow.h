/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __FMIDIAUTOMATIONMAINWINDOW_H
#define __FMIDIAUTOMATIONMAINWINDOW_H

#include <gtkmm.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <set>
#include "FMidiAutomationData.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <boost/thread.hpp>
#include <jack/transport.h>

struct TempoGlobals;
class Sequencer;
class SequencerEntryBlock;
class SequencerEntry;
struct GraphState;
struct CurveEditor;
struct Keyframe;

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
    KeyValue,
    InTangent,
    OutTangent,
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

namespace UIThreadOperation
{
enum UIThreadOperation
{
    Nothing,
    finishProcessRecordedMidiOp,
};//UIThreadOperation
}//UIThreadOperation

class EntryBlockSelectionState
{
    std::map<boost::shared_ptr<SequencerEntryBlock>, int> currentlySelectedEntryOriginalStartTicks;
    std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > currentlySelectedEntryBlocks;
    std::set<boost::shared_ptr<SequencerEntryBlock> > origSelectedEntryBlocks; //for rubberbanding

public:
    EntryBlockSelectionState() {}
    ~EntryBlockSelectionState() {}

    bool HasSelected();
    bool IsSelected(boost::shared_ptr<SequencerEntryBlock> entryBlock);
    bool IsOrigSelected(boost::shared_ptr<SequencerEntryBlock> entryBlock); //checks origSelectedEntryBlocks
    void ClearSelected();
    void ResetRubberbandingSelection();

    int GetNumSelected();
    boost::shared_ptr<SequencerEntryBlock> GetFirstEntryBlock();
    int GetOriginalStartTick(boost::shared_ptr<SequencerEntryBlock> entryBlock);

    std::multimap<int, boost::shared_ptr<SequencerEntryBlock> > GetEntryBlocksMapCopy();
    std::map<boost::shared_ptr<SequencerEntryBlock>, int> GetEntryOriginalStartTicksCopy();
    std::set<boost::shared_ptr<SequencerEntryBlock> > GetOrigSelectedEntryBlocksCopy();

    std::pair<decltype(currentlySelectedEntryBlocks.begin()), decltype(currentlySelectedEntryBlocks.end())> GetCurrentlySelectedEntryBlocks();

    void SetCurrentlySelectedEntryOriginalStartTicks(std::map<boost::shared_ptr<SequencerEntryBlock>, int> &origStartTicks); //FIXME: This feels very questionable

    void AddSelectedEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock);
    void RemoveSelectedEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock);
};//EntryBlockSelectionState

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
    std::vector<int> roundedHorizontalValues;

    SelectedEntity selectedEntity;

    int curMousePosX;
    int curMousePosY;

    //Time at which the pointer is at
    int curPointerTick;
    int curPointerTickXPixel; //how far over is it?   

    int leftMarkerTick;
    int rightMarkerTick;
    int leftMarkerTickXPixel;
    int rightMarkerTickXPixel;

    EntryBlockSelectionState entryBlockSelectionState;
    ///boost::shared_ptr<SequencerEntryBlock> getCurrentlySelectedEntryBlock(); -- replaced by entryBlockSelectionState.GetFirstEntryBlock()

    bool didMoveKey;
    bool didMoveKeyOutTangent;
    bool didMoveKeyInTangent;

    boost::shared_ptr<Keyframe> selectedKey; //mostly useful for who owns the selected tangent grab handle
    std::map<int, boost::shared_ptr<Keyframe> > currentlySelectedKeyframes; //not a multimap since keys at ticks are unique
    std::map<boost::shared_ptr<Keyframe>, int> movingKeyOrigTicks;
    std::map<boost::shared_ptr<Keyframe>, double> movingKeyOrigValues;

    DisplayMode::DisplayMode displayMode;
    int lastSequencerPointerTick; //for swaping back to the seqeucner

    //Rubberband selection stuff
    bool doingRubberBanding;
    std::set<boost::shared_ptr<Keyframe> > origSelectedKeyframes;
    
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
    boost::shared_ptr<CurveEditor> curveEditor;
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
    Gtk::MenuItem *menuSplitEntryBlock;
    Gtk::MenuItem *menuJoinEntryBlocks;
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
    Gtk::Frame *selectedKeyframeFrame;
    Gtk::Entry *positionTickEntry;
    Gtk::Entry *positionValueEntry;
    Gtk::Label *positionValueLabel;
    Gtk::Entry *currentSampledValue;
    Gtk::CheckButton *bpmFrameCheckButton;

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
    gdouble mousePressReleaseX;
    gdouble mousePressReleaseY;
    static const int MainCanvasOffsetY = 60;
    
    boost::shared_ptr<Gtk::Image> backingImage;
    boost::shared_ptr<Gtk::Image> backingTexture;
    boost::shared_ptr<Gtk::Image> origBackingImage;
    boost::shared_ptr<Gtk::Image> origBackingTexture;

    std::vector <Gtk::Window *> automationTrackWindows;
    
    boost::shared_ptr<FMidiAutomationData> datas;
    GraphState graphState;
    boost::shared_ptr<Sequencer> sequencer;

    Gtk::Label *statusBar;
    float statusTextAlpha;
    Glib::ustring currentStatusText;
    bool needsStatusTextUpdate;
    void statusTextThreadFunc();
    boost::thread statusTextThread;
    void setStatusText(Glib::ustring text);
    boost::mutex statusTextMutex;

    UIThreadOperation::UIThreadOperation queuedUIThreadOperation;

    bool recordMidi;
    
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
    void on_menuPorts();
    void on_menuPasteInstance();
    void on_menuSplitEntryBlock();
    void on_menuJoinEntryBlocks();

    void on_handleDelete();

    bool key_pressed(GdkEventKey *event);
    bool key_released(GdkEventKey *event);
    bool mouseButtonPressed(GdkEventButton *event);
    bool mouseButtonReleased(GdkEventButton *event);
    bool mouseMoved(GdkEventMotion *event);
    bool handleScroll(GdkEventScroll *event);

    bool handleKeyEntryOnLeftTickEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnRightTickEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnCursorTickEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnPositionTickEntryBox(GdkEventKey *event);

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
    void handleRecordPressed();

    void handleJackPressed();

    boost::shared_ptr<boost::thread> recordThread;
    void startRecordThread();

    bool handleEntryWindowScroll(Gtk::ScrollType, double);

    void handleAddSequencerEntryBlock();
    void handleDeleteSequencerEntryBlocks();
    void handleDeleteSequencerEntryBlock();
    void handleSequencerEntryProperties();
    void handleSequencerEntryCurve();

    void handleDeleteKeyframe();

    void updateCursorTick(int tick, bool updateJack);

    void setThemeColours();

    bool on_idle();

    void setTitle(Glib::ustring currentFilename);
    void setTitleChanged();

    void processRecordedMidi();
    void finishProcessRecordedMidi();

    void doUIQueuedThreadStuff();

    //ui mouse handlers
    void handleSequencerFrameRegionLMBPress();
    void handleSequencerFrameRegionLMBRelease();
    void handleSequencerFrameRegionMMBPress();
    void handleSequencerFrameRegionMMBRelease();
    void handleSequencerFrameRegionRMBPress();
    void handleSequencerFrameRegionRMBRelease();
    void handleSequencerFrameRegionMouseMove();

    void handleSequencerMainCanvasLMBPress();
    void handleSequencerMainCanvasLMBRelease(gdouble xPos);
    void handleSequencerMainCanvasMMBPress();
    void handleSequencerMainCanvasMMBRelease();
    void handleSequencerMainCanvasRMBPress(guint button, guint32 time);
    void handleSequencerMainCanvasRMBRelease();
    void handleSequencerMainCanvasMouseMove(gdouble xPos, gdouble yPos);

    void handleSequencerTickMarkerRegionLMBPress(gdouble xPos);
    void handleSequencerTickMarkerRegionLMBRelease(gdouble xPos, gdouble yPos);
    void handleSequencerTickMarkerRegionMMBPress();
    void handleSequencerTickMarkerRegionMMBRelease();
    void handleSequencerTickMarkerRegionRMBPress();
    void handleSequencerTickMarkerRegionRMBRelease(gdouble xPos, gdouble yPos);
    void handleSequencerTickMarkerRegionMouseMove();

    void handleCurveEditorFrameRegionLMBPress();
    void handleCurveEditorFrameRegionLMBRelease();
    void handleCurveEditorFrameRegionMMBPress();
    void handleCurveEditorFrameRegionMMBRelease();
    void handleCurveEditorFrameRegionRMBPress();
    void handleCurveEditorFrameRegionRMBRelease();
    void handleCurveEditorFrameRegionMouseMove();

    void handleCurveEditorMainCanvasLMBPress();
    void handleCurveEditorMainCanvasLMBRelease();
    void handleCurveEditorMainCanvasMMBPress();
    void handleCurveEditorMainCanvasMMBRelease();
    bool handleCurveEditorMainCanvasRMBPress(gdouble xPos, guint button, guint32 time);
    void handleCurveEditorMainCanvasRMBRelease();
    void handleCurveEditorMainCanvasMouseMove(gdouble xPos, gdouble yPos);

    void handleCurveEditorTickMarkerRegionLMBPress();
    void handleCurveEditorTickMarkerRegionLMBRelease();
    void handleCurveEditorTickMarkerRegionMMBPress();
    void handleCurveEditorTickMarkerRegionMMBRelease();
    void handleCurveEditorTickMarkerRegionRMBPress();
    void handleCurveEditorTickMarkerRegionRMBRelease();
    void handleCurveEditorTickMarkerRegionMouseMove();

    void handleCurveEditorLeftValueRegionLMBPress();
    void handleCurveEditorLeftValueRegionLMBRelease();
    void handleCurveEditorLeftValueRegionMMBPress();
    void handleCurveEditorLeftValueRegionMMBRelease();
    void handleCurveEditorLeftValueRegionRMBPress();
    void handleCurveEditorLeftValueRegionRMBRelease();
    void handleCurveEditorLeftValueRegionMouseMove();

    friend struct FMidiAutomationCurveEditor;
       
public:    
    FMidiAutomationMainWindow();
    ~FMidiAutomationMainWindow();
    
    Gtk::Window *MainWindow();
    GraphState &getGraphState();

    void doTestInit();

    void unsetAllCurveFrames();
    void editSequencerEntryProperties(boost::shared_ptr<SequencerEntry> entry, bool createUpdatePoint);
    void queue_draw();
};//FMidiAutomationMainWindow


BOOST_CLASS_VERSION(GraphState, 1);

#endif
