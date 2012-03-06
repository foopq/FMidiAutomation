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
#include <functional>
#include <jack/transport.h>
#include <thread>


struct FMidiAutomationData;
class SequencerUI;
class SequencerEntry;
struct GraphState;
struct CurveEditor;
class SequencerEntryBlockUI;
class CommandManager;

enum class UIThreadOperation : char
{
    Nothing,
    finishProcessRecordedMidiOp,
};//UIThreadOperation

enum class WindowMode : char
{
    MainWindow,
    CurveEditorOnly
};//WindowMode

class FMidiAutomationMainWindow : public std::enable_shared_from_this<FMidiAutomationMainWindow>
{
    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Window *mainWindow;

    Gtk::ScrolledWindow *trackListWindow;
    Gtk::DrawingArea *graphDrawingArea;
    Gtk::ImageMenuItem *menuCopy;
    Gtk::ImageMenuItem *menuCut;
    Gtk::ImageMenuItem *menuPaste;
    Gtk::ImageMenuItem *menuRedo;
    Gtk::ImageMenuItem *menuUndo;
    Gtk::ImageMenuItem *menuPasteInstance;
    Gtk::Entry *leftTickEntryBox;
    Gtk::Entry *rightTickEntryBox;
    Gtk::Entry *cursorTickEntryBox;    
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

    sigc::connection idleConnection;

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
 
    std::shared_ptr<CurveEditor> curveEditor;
    std::shared_ptr<GraphState> graphState;
    std::shared_ptr<SequencerUI> sequencer;

    Gtk::Label *statusBar;
    float statusTextAlpha;
    Glib::ustring currentStatusText;
    bool needsStatusTextUpdate;
    std::thread statusTextThread;
    std::mutex statusTextMutex; //We intentionally leak this to avoid segfaulting on exit
    std::mutex statusTextDataMutex;

    UIThreadOperation queuedUIThreadOperation;

    guint32 lastHandledTime; //last handled scroll time
    bool recordMidi;
    bool curveEditorOnlyMode;
    std::shared_ptr<SequencerEntryBlockUI> editingEntryBlock;
    bool isExiting;

    std::shared_ptr<std::thread> recordThread;
 
    /* functions */
    void setStatusText(Glib::ustring text);
    void statusTextThreadFunc();

    void handleGraphResize(Gtk::Allocation&);
    
    void refreshGraphBackground();
    bool updateGraph(const Cairo::RefPtr<Cairo::Context> &context);   

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
    void on_menuAlignMainCursor();
    void on_menuAlignLeftCursor();
    void on_menuAlignRightCursor();

    void on_handleDelete();

    bool handleOnClose(GdkEventAny *);

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
    void handleSequencerButtonPressedNoGraphStateSelectedEntryBlock();
    void handleCurveButtonPressed();
    void handleCurveButtonPressedHelper(std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock);

    void handleRewPressed();
    void handlePlayPressed();
    void handlePausePressed();
    void handleRecordPressed();

    void handleJackPressed();
    void handleInsertModeChanged();
    void startRecordThread();
    void handleEditSelectedInSeparateWindow();
    bool handleEntryWindowScroll(Gtk::ScrollType, double);

    void handleAddSequencerEntryBlock();
    void handleDeleteSequencerEntryBlocks();
//    void handleDeleteSequencerEntryBlock();
    void handleSequencerEntryProperties();
    void handleSequencerEntryCurve();

    void handleDeleteKeyframe();

    void updateCursorTick(int tick, bool updateJack);
    void handleSetFocus(Gtk::Widget *widget);

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
    void init(bool curveEditorOnlyMode_, std::shared_ptr<SequencerEntryBlockUI> editingEntryBlock); //Note: We split the constructor so that ::mainWindow is always valid
    
    Gtk::Window *MainWindow();
    GraphState &getGraphState();

    void doTestInit();

    std::shared_ptr<SequencerUI> getSequencer();
    void unsetAllCurveFrames();
    void editSequencerEntryProperties(std::shared_ptr<SequencerEntry> entry, bool createUpdatePoint);
    std::function<void (const std::string &)> getLoadCallback();
    WindowMode getWindowMode();
    bool IsInSequencer();
    Gtk::ImageMenuItem *getMenuUndo();
    Gtk::ImageMenuItem *getMenuRedo();
    std::function<void (void)> getTitleStarFunc();
    void queue_draw();

    //For serialization
    bool getCurveEditorOnlyMode();
    std::shared_ptr<SequencerEntryBlockUI> getEditingEntryBlock();
    void forceCurveEditorMode(std::shared_ptr<SequencerEntryBlockUI> selectedEntryBlock);
    void doSave(boost::archive::xml_oarchive &outputArchive);
    void doLoad(boost::archive::xml_iarchive &inputArchive);
};//FMidiAutomationMainWindow



#endif
