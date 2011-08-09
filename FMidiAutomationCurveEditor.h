/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __FMIDIAUTOMATIONCURVEEDITOR_H
#define __FMIDIAUTOMATIONCURVEEDITOR_H

#include <gtkmm.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <set>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>

class FMidiAutomationMainWindow;
struct Keyframe;
class GraphState;

class CurveEditor
{
    FMidiAutomationMainWindow *mainWindow;
    Glib::RefPtr<Gtk::Builder> uiXml;
    int curMouseUnderTick;
    int curMouseUnderValue;

    void setUpWidgets();
    bool handleKeyEntryOnSelectedKeyTickEntryEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSelectedKeyValueEntryEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSelectedKeyInTanXEntryEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSelectedKeyInTanYEntryEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSelectedKeyOutTanXEntryEntryBox(GdkEventKey *event);
    bool handleKeyEntryOnSelectedKeyOutTanYEntryEntryBox(GdkEventKey *event);
    void handleSelectionChangeOnSelectedKeyTypeComboBox();

public:
    CurveEditor(FMidiAutomationMainWindow *mainWindow, Glib::RefPtr<Gtk::Builder> uiXml);
    ~CurveEditor();

    void handleAddKeyframe();
    void handleDeleteKeyframes();
    void setUnderMouseTickValue(int tick, int value);
    boost::shared_ptr<Keyframe> getKeySelection(GraphState &graphState, int mousePressDownX, int mousePressDownY, bool ctrlPressed);
    void setKeyUIValues(Glib::RefPtr<Gtk::Builder> uiXml, boost::shared_ptr<Keyframe> currentlySelectedKeyframe);

    void updateSelectedKeyframesInRange(std::map<int, boost::shared_ptr<Keyframe> > &currentlySelectedKeyframes,
                                            std::set<boost::shared_ptr<Keyframe> > &origSelectedKeyframes,
                                            gdouble mousePressDownX, gdouble mousePressDownY, gdouble xPos, gdouble yPos,
                                            int areaWidth, int areaHeight);
};//CurveEditor

#endif

