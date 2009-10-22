#include "Tempo.h"
#include "FMidiAutomationMainWindow.h"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

namespace
{

bool updateTempoBoxWithSelected(std::map<int, boost::shared_ptr<Tempo> > &tempoChanges, Gtk::Entry *bpmEntry, Gtk::Entry *beatsPerBarEntry, Gtk::Entry *barSubdivisionsEntry)
{
    typedef std::pair<int, boost::shared_ptr<Tempo> > TempoMarkerPair;
    BOOST_FOREACH(TempoMarkerPair tempoMarkerPair, tempoChanges) {
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
    BOOST_FOREACH(VLPT verticalLinePair, verticalLines) {
        if (BarStart == verticalLinePair.second) {
            context->move_to(verticalLinePair.first, 61);
            context->line_to(verticalLinePair.first, drawingAreaHeight);
        }//if
    }//foreach

    context->stroke();

    context->set_source_rgba(0.7, 0.7, 0.7, 0.7);
    BOOST_FOREACH(VLPT verticalLinePair, verticalLines) {
        if (SubdivisionLine == verticalLinePair.second) {
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
    BOOST_FOREACH(TLPT textLinePair, lowerLineText) {
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

void drawTimeSignatureTicks(int firstPixelTick, int lastPixelTick, std::map<int, boost::shared_ptr<Tempo> >::iterator tempoMarker, 
                                Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, boost::shared_ptr<FMidiAutomationData> datas, 
                                unsigned int drawingAreaWidth, unsigned int drawingAreaHeight, std::vector<int> &verticalPixelTickValues, int ticksPerPixel)
{
    std::vector<std::pair<unsigned int, LineType> > verticalLines;
    std::vector<std::pair<unsigned int, std::string> > lowerLineText;

    if (tempoMarker != datas->tempoChanges.begin()) {
        tempoMarker--;
    }//if

    if (ticksPerPixel < 1) {
        ticksPerPixel = 0;
    }//if

    while ((tempoMarker != datas->tempoChanges.end()) && (tempoMarker->first < lastPixelTick)) {
        std::map<int, boost::shared_ptr<Tempo> >::iterator nextTempoMarker = tempoMarker;
        nextTempoMarker++;

        int nextStartTick = std::numeric_limits<int>::max();
        if (nextTempoMarker != datas->tempoChanges.end()) {
            nextStartTick = nextTempoMarker->first;
        }//if

        int curTempoMarkerStartTick = tempoMarker->first;
        int ticksPerBar = tempoMarker->second->ticksPerBar;
        int startBar = tempoMarker->second->startBar;
        int barSubDivisions = tempoMarker->second->barSubDivisions;

        int dist = -1;
        int lastAddedBarPixel = std::numeric_limits<int>::min();
        int lastAddedBarSubdivisionPixel = std::numeric_limits<int>::min(); 
        BOOST_FOREACH(int curTick, verticalPixelTickValues) {
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
                    verticalLines.push_back(std::make_pair(dist, SubdivisionLine));

                    lastAddedBarSubdivisionPixel = dist;
                }//if
            }//if

            int diffTicks = (curTick - curTempoMarkerStartTick) % ticksPerBar;
            if (diffTicks <= ticksPerPixel) {
                if ((dist - 50) > lastAddedBarPixel) {
                    int barCount = startBar + ((curTick - curTempoMarkerStartTick) / ticksPerBar);
                    std::ostringstream tmpSS;
                    tmpSS << (barCount + 1);

                    verticalLines.push_back(std::make_pair(dist, BarStart));
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

void drawTempoBar(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, boost::shared_ptr<FMidiAutomationData> datas, 
                    unsigned int drawingAreaWidth, unsigned int drawingAreaHeight, std::vector<int> &verticalPixelTickValues, int ticksPerPixel)
{
    int firstPixelTick = graphState.verticalPixelTickValues[0];
    int lastPixelTick = graphState.verticalPixelTickValues[graphState.verticalPixelTickValues.size()-1];

    typedef std::pair<int, boost::shared_ptr<Tempo> > TempoMarkerPair;
    BOOST_FOREACH(TempoMarkerPair tempoMarkerPair, datas->tempoChanges) {
        tempoMarkerPair.second->xPixelPos = -1;
    }//foreach

    std::map<int, boost::shared_ptr<Tempo> >::iterator firstTempoMarker = datas->tempoChanges.lower_bound(firstPixelTick);

    drawTimeSignatureTicks(firstPixelTick, lastPixelTick, firstTempoMarker, context, graphState, datas, 
                            drawingAreaWidth, drawingAreaHeight, verticalPixelTickValues, ticksPerPixel);

    while ((firstTempoMarker != datas->tempoChanges.end()) && (firstTempoMarker->first < lastPixelTick)) {
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

void updateTempoBox(GraphState &graphState, boost::shared_ptr<FMidiAutomationData> datas, Gtk::Entry *bpmEntry, Gtk::Entry *beatsPerBarEntry, Gtk::Entry *barSubdivisionsEntry)
{
    if (true == updateTempoBoxWithSelected(datas->tempoChanges, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry)) {
        return;
    }//if

    std::map<int, boost::shared_ptr<Tempo> >::const_iterator firstTempoMarker = datas->tempoChanges.lower_bound(graphState.curPointerTick);

    if ((firstTempoMarker == datas->tempoChanges.end()) || (firstTempoMarker->first != graphState.curPointerTick)) {
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

bool checkForTempoSelection(int xPos, std::map<int, boost::shared_ptr<Tempo> > &tempoChanges)
{
    bool foundIt = false;

    typedef std::pair<int, boost::shared_ptr<Tempo> > TempoMarkerPair;
    BOOST_FOREACH(TempoMarkerPair tempoMarkerPair, tempoChanges) {
        if ((tempoMarkerPair.second->xPixelPos != -1) && (abs(xPos - tempoMarkerPair.second->xPixelPos) <= 5)) {
            tempoMarkerPair.second->currentlySelected = true;
            foundIt = true;
        } else {
            tempoMarkerPair.second->currentlySelected = false;
        }//if
    }//foreach

    bool tmp = false;
    typedef std::pair<int, boost::shared_ptr<Tempo> > TempoMarkerPair;
    BOOST_FOREACH(TempoMarkerPair tempoMarkerPair, tempoChanges) {
        if (tempoMarkerPair.second->currentlySelected == true) {
            tmp = true;
        }//if
    }//foreach

    return foundIt;
}//checkForTempoSelection

void updateTempoChangesUIData(std::map<int, boost::shared_ptr<Tempo> > &tempoChanges)
{
    if (tempoChanges.empty() == true) {
        return;
    }//if

    int lastStartBar = 0;
    std::map<int, boost::shared_ptr<Tempo> >::iterator curTempoMarker = tempoChanges.begin();
    std::map<int, boost::shared_ptr<Tempo> >::iterator nextTempoMarker = curTempoMarker;
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

    currentlySelected = false;
    xPixelPos = -1;
}//serialize


template void Tempo::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void Tempo::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);

