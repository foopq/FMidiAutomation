/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __TEMPO_H
#define __TEMPO_H

#include <gtkmm.h>
#include <memory>
#include <vector>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>

struct GraphState;
struct FMidiAutomationData;

struct Tempo
{    
    unsigned int bpm; //times 100
    unsigned int beatsPerBar;
    unsigned int barSubDivisions;

    int xPixelPos;
    bool currentlySelected;

    //UI datas
    int startBar;
    int numBars;
    float ticksPerBar;

    Tempo() {}
    Tempo(unsigned int bpm, unsigned int beatsPerBar, unsigned int barSubDivisions);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//Tempo

void drawTempoBar(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, std::shared_ptr<FMidiAutomationData> datas, 
                    unsigned int drawingAreaWidth, unsigned int drawingAreaHeight, std::vector<int> &verticalPixelTickValues, int ticksPerPixel);
void updateTempoBox(GraphState &graphState, std::shared_ptr<FMidiAutomationData> datas, Gtk::Entry *bpmEntry, Gtk::Entry *beatsPerBarEntry, Gtk::Entry *barSubdivisionsEntry);
bool checkForTempoSelection(int xPos, std::map<int, std::shared_ptr<Tempo> > &tempoChanges);
void updateTempoChangesUIData(std::map<int, std::shared_ptr<Tempo> > &tempoChanges);

BOOST_CLASS_VERSION(Tempo, 1);

#endif
