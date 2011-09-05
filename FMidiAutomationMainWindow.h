/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __FMIDIAUTOMATIONMAINWINDOW_H
#define __FMIDIAUTOMATIONMAINWINDOW_H

#include <gtkmm.h>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <boost/thread.hpp>
#include <jack/transport.h>

struct FMidiAutomationData;
class Sequencer;
class SequencerEntry;
struct GraphState;
struct CurveEditor;

namespace UIThreadOperation
{
enum UIThreadOperation
{
    Nothing,
    finishProcessRecordedMidiOp,
};//UIThreadOperation
}//UIThreadOperation


class FMidiAutomationMainWindow
{
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Window *mainWindow;
    std::shared_ptr<CurveEditor> curveEditor;
    Gtk::ScrolledWindow *trackListWindow;
    Gtk::DrawingArea *graphDrawingArea;
    Gtk::ImageMenuItem *menuOpen;
    Gtk::MenuItem *menuOpenRecent;
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
    Gtk::MenuItem *menu_pasteSEBToSelectedEntry;


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
    
    std::shared_ptr<Gtk::Image> backingImage;
    std::shared_ptr<Gtk::Image> backingTexture;
    std::shared_ptr<Gtk::Image> origBackingImage;
    std::shared_ptr<Gtk::Image> origBackingTexture;

    std::vector <Gtk::Window *> automationTrackWindows;
    
    std::shared_ptr<FMidiAutomationData> datas;
    std::shared_ptr<GraphState> graphState;
    std::shared_ptr<Sequencer> sequencer;

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
    void on_menuSplitEntryBlocks();
    void on_menuJoinEntryBlocks();
    void on_menupasteSEBToSelectedEntry();
    void on_menupasteSEBInstancesToSelectedEntry();

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
    bool handleKeyEntryOnSelectedEntryBlockNameEntryBox(GdkEventKey *event);

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

    void handleInsertModeChanged();

    std::shared_ptr<boost::thread> recordThread;
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

    void actuallyLoadFile(const Glib::ustring &filename);

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
    void init(); //Note: We split the constructor so that ::mainWindow is always valid
    
    Gtk::Window *MainWindow();
    GraphState &getGraphState();

    void doTestInit();

    void unsetAllCurveFrames();
    void editSequencerEntryProperties(std::shared_ptr<SequencerEntry> entry, bool createUpdatePoint);
    void queue_draw();
};//FMidiAutomationMainWindow



#endif
