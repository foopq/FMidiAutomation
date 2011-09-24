/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "Tempo.h"
#include <boost/lexical_cast.hpp>
#include "Data/FMidiAutomationData.h"
#include "GraphState.h"
#include "Globals.h"

namespace
{

bool updateTempoBoxWithSelected(fmaipair<FMidiAutomationData::TempoChangesIter, FMidiAutomationData::TempoChangesIter> tempoChanges, Gtk::Entry *bpmEntry, Gtk::Entry *beatsPerBarEntry, Gtk::Entry *barSubdivisionsEntry)
{
    for (auto tempoMarkerPair : tempoChanges) {
        if (true == tempoMarkerPair.second->currentlySelected) {
            unsigned int bpmNumerator = tempoMarkerPair.second->bpm / 100;
            unsigned int bpmDenominator = tempoMarkerPair.second->bpm - (bpmNumerator * 100);

            std::string bpmString = boost::lexical_cast<std::string>(bpmNumerator) + "." + boost::lexical_cast<std::string>(bpmDenominator);
            if (0 == (bpmDenominator % 10)) {
                bpmString.append("0");
            }//if

            bpmEntry->set_text(bpmString);
            beatsPerBarEntry->set_text(boost::lexical_cast<std::string>(tempoMarkerPair.second->beatsPerBar));
            barSubdivisionsEntry->set_text(boost::lexical_cast<std::string>(tempoMarkerPair.second->barSubDivisions));
            return true;
        }//if
    }//foreach

    return false;
}//updateTempoBoxWithSelected

void actuallyDrawTempoBars(unsigned int drawingAreaWidth, unsigned int drawingAreaHeight, 
                            std::vector<std::pair<unsigned int, LineType> > &verticalLines, 
                            std::vector<std::pair<unsigned int, std::string> > &lowerLineText,
                            Cairo::RefPtr<Cairo::Context> context)
{
    Globals &globals = Globals::Instance();

    context->reset_clip();
    context->set_source_rgba(0.0, 1.0, 0.0, 0.2);
    context->set_line_width(1.0);

    typedef std::pair<unsigned int, LineType> VLPT;
    for (VLPT verticalLinePair : verticalLines) {
        if (LineType::BarStart == verticalLinePair.second) {
            context->move_to(verticalLinePair.first, 61);
            context->line_to(verticalLinePair.first, drawingAreaHeight);
        }//if
    }//foreach

    context->stroke();

    context->set_source_rgba(0.7, 0.7, 0.7, 0.7);
    for (VLPT verticalLinePair : verticalLines) {
        if (LineType::SubdivisionLine == verticalLinePair.second) {
            context->move_to(verticalLinePair.first, 61);
            context->line_to(verticalLinePair.first, 50);
        }//if
    }//foreach

    context->stroke();

    context->reset_clip();
    
    //Second text
    context->set_source_rgba(1.0, 1.0, 1.0, 0.7);
    //context->select_font_face(globals.topBarFont.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    //context->set_font_size(globals.topBarFontSize);

    std::string fontStr;
    {
        std::ostringstream tmpSS;
        tmpSS << globals.bottomBarFont << " bold " << globals.bottomBarFontSize;
        fontStr = tmpSS.str();
    }

    typedef std::pair<unsigned int, std::string> TLPT;
    for (TLPT textLinePair : lowerLineText) {
        context->move_to(textLinePair.first, 60 - ((30 - globals.bottomBarFontSize) / 2 + globals.bottomBarFontSize));

        Glib::RefPtr<Pango::Layout> pangoLayout = Pango::Layout::create(context);
        Pango::FontDescription font_descr(fontStr.c_str());

        pangoLayout->set_font_description(font_descr);
        pangoLayout->set_text(textLinePair.second.c_str());
        pangoLayout->update_from_cairo_context(context);  //gets cairo cursor position
        pangoLayout->add_to_cairo_context(context);       //adds text to cairos stack of stuff to be drawn
        context->fill();
        context->stroke();
    }//foreach

    context->stroke();
}//actuallyDrawTempoBars

void drawTimeSignatureTicks(int firstPixelTick, int lastPixelTick, FMidiAutomationData::TempoChangesIter tempoMarker, 
                                Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, FMidiAutomationData &datas, 
                                unsigned int drawingAreaWidth, unsigned int drawingAreaHeight, std::vector<int> &verticalPixelTickValues, int ticksPerPixel)
{
    std::vector<std::pair<unsigned int, LineType> > verticalLines;
    std::vector<std::pair<unsigned int, std::string> > lowerLineText;

    auto tempoChanges = datas.getTempoChanges();
    if (tempoMarker != tempoChanges.first) {
        tempoMarker--;
    }//if

    if (ticksPerPixel < 1) {
        ticksPerPixel = 0;
    }//if

    while ((tempoMarker != tempoChanges.second) && (tempoMarker->first < lastPixelTick)) {
        std::map<int, std::shared_ptr<Tempo> >::iterator nextTempoMarker = tempoMarker;
        nextTempoMarker++;

        int nextStartTick = std::numeric_limits<int>::max();
        if (nextTempoMarker != tempoChanges.second) {
            nextStartTick = nextTempoMarker->first;
        }//if

        int curTempoMarkerStartTick = tempoMarker->first;
        int ticksPerBar = tempoMarker->second->ticksPerBar;
        int startBar = tempoMarker->second->startBar;
        int barSubDivisions = tempoMarker->second->barSubDivisions;

        int dist = -1;
        int lastAddedBarPixel = std::numeric_limits<int>::min();
        int lastAddedBarSubdivisionPixel = std::numeric_limits<int>::min(); 
        for (int curTick : verticalPixelTickValues) {
            dist++;

            if ((curTick >= nextStartTick) || (dist > ((int)drawingAreaWidth))) {
                break;
            }//if

            if (curTick < curTempoMarkerStartTick) {
                continue;
            }//if

            int subTicks = (curTick - curTempoMarkerStartTick) % (ticksPerBar / barSubDivisions);
            if (subTicks <= ticksPerPixel) {
                if ((dist - 20) > lastAddedBarSubdivisionPixel) {
                    verticalLines.push_back(std::make_pair(dist, LineType::SubdivisionLine));

                    lastAddedBarSubdivisionPixel = dist;
                }//if
            }//if

            int diffTicks = (curTick - curTempoMarkerStartTick) % ticksPerBar;
            if (diffTicks <= ticksPerPixel) {
                if ((dist - 50) > lastAddedBarPixel) {
                    int barCount = startBar + ((curTick - curTempoMarkerStartTick) / ticksPerBar);
                    std::ostringstream tmpSS;
                    tmpSS << (barCount + 1);

                    verticalLines.push_back(std::make_pair(dist, LineType::BarStart));
                    lowerLineText.push_back(std::make_pair(dist, tmpSS.str()));

                    lastAddedBarPixel = dist;
                }//if
            }//if
        }//foreach

        tempoMarker++;
    }//while

    actuallyDrawTempoBars(drawingAreaWidth, drawingAreaHeight, verticalLines, lowerLineText, context);
}//drawTimeSignatureTicks

}//anonymous namespace

Tempo::Tempo(unsigned int bpm_, unsigned int beatsPerBar_, unsigned int barSubDivisions_)
{
    bpm = bpm_;
    beatsPerBar = beatsPerBar_;
    barSubDivisions = barSubDivisions_;
    currentlySelected = false;
    xPixelPos = -1;
}//constructor

TempoGlobals::TempoGlobals()
{
    tempoDataSelected = false;
}//constructor

void drawTempoBar(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, FMidiAutomationData &datas, 
                    unsigned int drawingAreaWidth, unsigned int drawingAreaHeight, std::vector<int> &verticalPixelTickValues, int ticksPerPixel)
{
    int firstPixelTick = graphState.verticalPixelTickValues[0];
    int lastPixelTick = graphState.verticalPixelTickValues[graphState.verticalPixelTickValues.size()-1];

    auto tempoChanges = datas.getTempoChanges();

    for (auto tempoMarkerPair : tempoChanges) {
        tempoMarkerPair.second->xPixelPos = -1;
    }//foreach

    auto firstTempoMarker = datas.getTempoChangesLowerBound(firstPixelTick);

    drawTimeSignatureTicks(firstPixelTick, lastPixelTick, firstTempoMarker, context, graphState, datas, 
                            drawingAreaWidth, drawingAreaHeight, verticalPixelTickValues, ticksPerPixel);

    while ((firstTempoMarker != tempoChanges.second) && (firstTempoMarker->first < lastPixelTick)) {
        unsigned int timePointerPixel = 0;

        std::vector<int>::iterator bound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), firstTempoMarker->first);
        timePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), bound);

        context->reset_clip();
        context->rectangle(timePointerPixel-5, 45-5, 10, 10);
        context->clip();
        context->set_source_rgba(0.0, 0.8, 0.0, 0.8);
        context->paint();

        if (true == firstTempoMarker->second->currentlySelected) {
            context->set_source_rgba(0.0, 0.0, 1.0, 1.0);
            context->set_line_width(2.0);
            context->move_to(timePointerPixel-5, 45-5);
            context->line_to(timePointerPixel+5, 45+10);
            context->move_to(timePointerPixel-5, 45+10);
            context->line_to(timePointerPixel+5, 45-5);
            context->stroke();
        }//if

        firstTempoMarker->second->xPixelPos = timePointerPixel;

        ++firstTempoMarker;
    }//while
}//drawTempoBar

void updateTempoBox(GraphState &graphState, FMidiAutomationData &datas, Gtk::Entry *bpmEntry, Gtk::Entry *beatsPerBarEntry, Gtk::Entry *barSubdivisionsEntry)
{
    if (true == updateTempoBoxWithSelected(datas.getTempoChanges(), bpmEntry, beatsPerBarEntry, barSubdivisionsEntry)) {
        return;
    }//if

    auto tempoChanges = datas.getTempoChanges();
    auto firstTempoMarker = datas.getTempoChangesLowerBound(graphState.curPointerTick);

    if ((firstTempoMarker == tempoChanges.second) || (firstTempoMarker->first != graphState.curPointerTick)) {
        --firstTempoMarker;
    }//if

    unsigned int bpmNumerator = firstTempoMarker->second->bpm / 100;
    unsigned int bpmDenominator = firstTempoMarker->second->bpm - (bpmNumerator * 100);

    std::string bpmString = boost::lexical_cast<std::string>(bpmNumerator) + "." + boost::lexical_cast<std::string>(bpmDenominator);
    if (0 == (bpmDenominator % 10)) {
        bpmString.append("0");
    }//if

    //bpmEntry->set_text(boost::lexical_cast<std::string>(firstTempoMarker->second.bpm));
    bpmEntry->set_text(bpmString);
    beatsPerBarEntry->set_text(boost::lexical_cast<std::string>(firstTempoMarker->second->beatsPerBar));
    barSubdivisionsEntry->set_text(boost::lexical_cast<std::string>(firstTempoMarker->second->barSubDivisions));
}//updateTempoBox

bool checkForTempoSelection(int xPos, fmaipair<FMidiAutomationData::TempoChangesIter, FMidiAutomationData::TempoChangesIter> tempoChanges)
{
    bool foundIt = false;

    typedef std::pair<int, std::shared_ptr<Tempo> > TempoMarkerPair;
    for (TempoMarkerPair tempoMarkerPair : tempoChanges) {
        if ((tempoMarkerPair.second->xPixelPos != -1) && (abs(xPos - tempoMarkerPair.second->xPixelPos) <= 5)) {
            tempoMarkerPair.second->currentlySelected = true;
            foundIt = true;
        } else {
            tempoMarkerPair.second->currentlySelected = false;
        }//if
    }//foreach

    /*
    bool tmp = false;
    typedef std::pair<int, std::shared_ptr<Tempo> > TempoMarkerPair;
    for (TempoMarkerPair tempoMarkerPair : tempoChanges) {
        if (tempoMarkerPair.second->currentlySelected == true) {
            tmp = true;
        }//if
    }//foreach
    */

    return foundIt;
}//checkForTempoSelection

void updateTempoChangesUIData(fmaipair<FMidiAutomationData::TempoChangesIter, FMidiAutomationData::TempoChangesIter> tempoChanges)
{
    if (tempoChanges.first == tempoChanges.second) {
        return;
    }//if

    int lastStartBar = 0;
    std::map<int, std::shared_ptr<Tempo> >::iterator curTempoMarker = tempoChanges.begin();
    std::map<int, std::shared_ptr<Tempo> >::iterator nextTempoMarker = curTempoMarker;
    nextTempoMarker++;

    for (/*nothing*/; curTempoMarker != tempoChanges.end(); ++curTempoMarker) {
        curTempoMarker->second->startBar = lastStartBar;

        float bpm = ((float)curTempoMarker->second->bpm / 100);
        float bps = bpm / 60.0f;
        float ticksPerBeat = 1000.0f / bps;

        curTempoMarker->second->ticksPerBar = ticksPerBeat * curTempoMarker->second->beatsPerBar;

        if (nextTempoMarker != tempoChanges.end()) {
            int diffTicks = nextTempoMarker->first - curTempoMarker->first;
            curTempoMarker->second->numBars = diffTicks / curTempoMarker->second->ticksPerBar;
        } else {
            curTempoMarker->second->numBars = std::numeric_limits<int>::max();
        }//if

        lastStartBar += curTempoMarker->second->numBars + 1;

        if (nextTempoMarker != tempoChanges.end()) {
            nextTempoMarker++;
        }//if
    }//for
}//updateTempoChangesUIData


template<class Archive>
void Tempo::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(bpm);
    ar & BOOST_SERIALIZATION_NVP(beatsPerBar);
    ar & BOOST_SERIALIZATION_NVP(barSubDivisions);

    ar & BOOST_SERIALIZATION_NVP(startBar);
    ar & BOOST_SERIALIZATION_NVP(numBars);
    ar & BOOST_SERIALIZATION_NVP(ticksPerBar);

    currentlySelected = false;
    xPixelPos = -1;
}//serialize


template void Tempo::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void Tempo::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);

