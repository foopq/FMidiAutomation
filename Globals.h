/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __GLOBALS_H
#define __GLOBALS_H

#include "Config.h"

class Sequencer;
struct GraphState;

void queue_draw();

struct TempoGlobals
{
    TempoGlobals();

    bool tempoDataSelected;
};//TempoGlobals

struct Globals
{
    Globals();
    ~Globals();

    static Globals &Instance();

    FMidiAutomationConfig config;

    std::string versionStr;

    std::string topBarFont;
    unsigned int topBarFontSize;

    std::string bottomBarFont;
    unsigned int bottomBarFontSize;

    bool darkTheme;

    Gtk::DrawingArea *graphDrawingArea;
    std::shared_ptr<GraphState> graphState;
    std::shared_ptr<Sequencer> sequencer;

    TempoGlobals tempoGlobals;
};//Globals


#endif

