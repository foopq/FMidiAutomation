
#include <libglademm.h>
#include <gdkmm/general.h>
#include <cairomm/surface.h>
#include <iostream>
#include <sstream>
#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include "Animation.h"
#include <boost/array.hpp>
#include <boost/foreach.hpp>

namespace
{
    
Glib::RefPtr<Gdk::Pixbuf> scale_keeping_ratio(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf, int target_height, int target_width)
{
    if( (target_height == 0) || (target_width == 0) ) {
	return Glib::RefPtr<Gdk::Pixbuf>(); //This shouldn't happen anyway.
    }//if

    if(!pixbuf) {
	return pixbuf;
    }//if

    enum enum_scale_mode
    {
	SCALE_WIDTH,
	SCALE_HEIGHT,
	SCALE_NONE
    };//enum_scale_mode

    enum_scale_mode scale_mode = SCALE_NONE; //Start with either the width or height, and scale the other according to the ratio.

    const int pixbuf_height = pixbuf->get_height();
    const int pixbuf_width = pixbuf->get_width();

    if (pixbuf_height > target_height) {
	if (pixbuf_width > target_width) {
	    //Both are bigger than the target, so find the biggest one:
	    if (pixbuf_width > pixbuf_height) {
		scale_mode = SCALE_WIDTH;
	    } else {
		scale_mode = SCALE_HEIGHT;
	    }//if
	} else {
	    //Only the height is bigger:
	    scale_mode = SCALE_HEIGHT;
	}//if
    } else {
	if (pixbuf_width > target_width) {
	    //Only the height is bigger:
	    scale_mode = SCALE_WIDTH;
	}//if
    }//if

    if (scale_mode == SCALE_NONE) {
	return pixbuf;
    } else {
	if(scale_mode == SCALE_HEIGHT) {
	    const float ratio = (float)target_height / (float)pixbuf_height; 
	    target_width = (int)((float)pixbuf_width * ratio);
	} else {
	    if (scale_mode == SCALE_WIDTH) {
		const float ratio = (float)target_width / (float) pixbuf_width;
		target_height = (int)((float)pixbuf_height * ratio);
	    }//if
	}//if
    }//if

    if( (target_height == 0) || (target_width == 0) ) {
	return Glib::RefPtr<Gdk::Pixbuf>(); //This shouldn't happen anyway. It seems to happen sometimes though, when ratio is very small.
    }//if

    return pixbuf->scale_simple(target_width, target_height, Gdk::INTERP_BILINEAR);
}//scale_keeping_ratio 
    
void scaleImage(boost::shared_ptr<Gtk::Image> image, Glib::RefPtr<Gdk::Pixbuf> pixbuf, int width, int height, bool maintainAspect)
{
    if (true == maintainAspect) {
	Glib::RefPtr<Gdk::Pixbuf> pixbuf_scaled = scale_keeping_ratio(pixbuf, height, width);
	image->set(pixbuf_scaled);
    } else {
	Glib::RefPtr<Gdk::Pixbuf> pixbuf_scaled = pixbuf->scale_simple(width, height, Gdk::INTERP_BILINEAR);
	image->set(pixbuf_scaled);
    }//if
}//scaleImage

void drawLeftBar(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
    Globals &globals = Globals::Instance();

    //Bar backgrounds
    context->reset_clip();
    context->rectangle(0, 60, 60, areaHeight-60);
    context->clip();

    context->set_source_rgba(0.1, 0.1, 0.1, 0.8);
    context->paint();

    //Value ticks
    context->reset_clip();
    context->set_source_rgba(0.6, 0.6, 0.6, 0.7);
    context->set_line_width(1.0);
    for (std::vector<std::pair<unsigned int, LineType> >::const_iterator valueLineIter = graphState.horizontalLines.begin(); valueLineIter != graphState.horizontalLines.end(); ++valueLineIter) {
        context->move_to(40, valueLineIter->first + 60);
        context->line_to(60, valueLineIter->first + 60);
    }//for

    context->stroke();

    context->reset_clip();
    context->rectangle(61, 60, areaWidth-60, areaHeight-60);
    context->clip();

    context->set_source_rgba(0.3, 0.3, 0.3, 0.3);
    context->set_line_width(1.0);
    for (std::vector<std::pair<unsigned int, LineType> >::const_iterator valueLineIter = graphState.horizontalLines.begin(); valueLineIter != graphState.horizontalLines.end(); ++valueLineIter) {
        context->move_to(61, valueLineIter->first + 60);
        context->line_to(areaWidth, valueLineIter->first + 60);
    }//for

    context->stroke();

    context->reset_clip();
    context->rectangle(0, 60, 60, areaHeight-60);
    context->clip();

    //Second text
    context->set_source_rgba(1.0, 1.0, 1.0, 0.7);
    //context->select_font_face(globals.topBarFont.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    //context->set_font_size(globals.topBarFontSize);

    std::string fontStr;
    {
        std::ostringstream tmpSS;
        tmpSS << globals.topBarFont << " bold " << globals.topBarFontSize;
        fontStr = tmpSS.str();
    }

    for(std::vector<std::pair<unsigned int, std::string> >::const_iterator textIter = graphState.valueLineText.begin(); textIter != graphState.valueLineText.end(); ++textIter) {
        //context->move_to(textIter->first, 30 - ((30 - globals.topBarFontSize) / 2 + globals.topBarFontSize));

////        get actual dimensions of text and place appropriately..

        context->move_to(0, textIter->first + 60- (globals.topBarFontSize / 1));

        Glib::RefPtr<Pango::Layout> pangoLayout = Pango::Layout::create(context);
        Pango::FontDescription font_descr(fontStr.c_str());

        pangoLayout->set_font_description(font_descr);
        pangoLayout->set_text(textIter->second.c_str());
        pangoLayout->update_from_cairo_context(context);  //gets cairo cursor position
        pangoLayout->add_to_cairo_context(context);       //adds text to cairos stack of stuff to be drawn
        context->fill();
        context->stroke();
    }//for

    context->stroke();
}//drawLeftBar

void drawTopBar(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
    Globals &globals = Globals::Instance();

    //Bar backgrounds
    context->reset_clip();
    context->rectangle(0, 0, areaWidth, 30);
    context->clip();

    context->set_source_rgba(0.2, 0.2, 0.2, 0.8);
    context->paint();

    context->reset_clip();

    context->rectangle(0, 30, areaWidth, 30);
    context->clip();

    context->set_source_rgba(0.1, 0.1, 0.1, 0.8);
    context->paint();

    //Second bars
    context->reset_clip();
    context->set_source_rgba(0.0, 0.0, 1.0, 0.3);
    context->set_line_width(1.0);
    for (std::vector<std::pair<unsigned int, LineType> >::const_iterator vertLineIter = graphState.verticalLines.begin(); vertLineIter != graphState.verticalLines.end(); ++vertLineIter) {
        context->move_to(vertLineIter->first, 61);
        context->line_to(vertLineIter->first, areaHeight);
    }//for

    context->stroke();

    context->reset_clip();
    
    //Second text
    context->set_source_rgba(1.0, 1.0, 1.0, 0.7);
    //context->select_font_face(globals.topBarFont.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    //context->set_font_size(globals.topBarFontSize);

    std::string fontStr;
    {
        std::ostringstream tmpSS;
        tmpSS << globals.topBarFont << " bold " << globals.topBarFontSize;
        fontStr = tmpSS.str();
    }

    for(std::vector<std::pair<unsigned int, std::string> >::const_iterator textIter = graphState.upperLineText.begin(); textIter != graphState.upperLineText.end(); ++textIter) {
        context->move_to(textIter->first, 30 - ((30 - globals.topBarFontSize) / 2 + globals.topBarFontSize));

        Glib::RefPtr<Pango::Layout> pangoLayout = Pango::Layout::create(context);
        Pango::FontDescription font_descr(fontStr.c_str());

        pangoLayout->set_font_description(font_descr);
        pangoLayout->set_text(textIter->second.c_str());
        pangoLayout->update_from_cairo_context(context);  //gets cairo cursor position
        pangoLayout->add_to_cairo_context(context);       //adds text to cairos stack of stuff to be drawn
        context->fill();
        context->stroke();
    }//for

    context->stroke();
}//drawTopBar

void drawLeftMarker(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
//    Globals &globals = Globals::Instance();

    if (graphState.leftMarkerTick == -1) {
        return;
    }//if

    unsigned int timePointerPixel = 0;

    std::vector<int>::iterator bound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), graphState.leftMarkerTick);
    timePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), bound);

    if ((0 == timePointerPixel) && ((*bound) > graphState.curPointerTick)) {
        return;
    }//if

    //Saves a little trouble with edge cases and scrolling/zooming
    if (areaWidth == timePointerPixel) {
        return;
    }//if

    context->reset_clip();
    context->set_source_rgba(0.9, 0.7, 0.0, 1.0);
    context->set_line_width(1.0);
    context->move_to(timePointerPixel, 30);
    context->line_to(timePointerPixel, areaHeight);
    context->stroke();

    context->save();
    context->arc(timePointerPixel, 35, 5, 0.5*M_PI, 1.5*M_PI);
    context->fill_preserve();
    context->stroke();

    graphState.leftMarkerTickXPixel = timePointerPixel;
}//drawLeftMarker

void drawRightMarker(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
//    Globals &globals = Globals::Instance();

    if (graphState.rightMarkerTick == -1) {
        return;
    }//if

    unsigned int timePointerPixel = 0;

    std::vector<int>::iterator bound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), graphState.rightMarkerTick);
    timePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), bound);

    if ((0 == timePointerPixel) && ((*bound) > graphState.curPointerTick)) {
        return;
    }//if

    //Saves a little trouble with edge cases and scrolling/zooming
    if (areaWidth == timePointerPixel) {
        return;
    }//if

    context->reset_clip();
    context->set_source_rgba(0.9, 0.7, 0.0, 1.0);
    context->set_line_width(1.0);
    context->move_to(timePointerPixel, 30);
    context->line_to(timePointerPixel, areaHeight);
    context->stroke();

    context->save();
    context->arc(timePointerPixel, 35, 5, -0.5*M_PI, 0.5*M_PI);
    context->fill_preserve();
    context->stroke();

    graphState.rightMarkerTickXPixel = timePointerPixel;
}//drawRightMarker

void drawCurrentTimePointer(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
//    Globals &globals = Globals::Instance();

    unsigned int timePointerPixel = 0;

    std::vector<int>::iterator bound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), graphState.curPointerTick);
    timePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), bound);

    if ((0 == timePointerPixel) && ((*bound) > graphState.curPointerTick)) {
        return;
    }//if

    //Saves a little trouble with edge cases and scrolling/zooming
    if (areaWidth == timePointerPixel) {
        return;
    }//if

    context->reset_clip();
    context->set_source_rgba(1.0, 0.0, 0.0, 1.0);
    context->set_line_width(1.0);
    context->move_to(timePointerPixel, 30);
    context->line_to(timePointerPixel, areaHeight);
    context->stroke();

    context->save();
    context->arc(timePointerPixel, 35, 5, 0, 2.0*M_PI);
    context->fill_preserve();
    context->stroke();

    graphState.curPointerTickXPixel = timePointerPixel;
}//drawCurrentTimePointer

int determineTickCountGroupSize(int ticksPerPixel)
{
    const boost::array<int, 40> scrollLevels = 
               {{ -50, -48, -46, -44, -42, -40, -38, -36, -34, -32, -30, -28, -26, -24, -20, -16, -8, -4, -2, 1,
                  2, 4, 8, 16, 32, 50, 70, 80, 90, 95, 100, 110, 118, 128, 140, 150, 170, 200, 250, 300
                }};

    const boost::array<int, 40> groupSizes = 
               {{ 1, 1, 1, 1, 2, 2, 2, 2, 5, 5, 5, 5, 10, 15, 25, 50, 75, 125, 250, 500,
                  1000, 1000, 4000, 4000, 10000, 10000, 20000, 20000, 40000, 40000, 40000, 40000, 40000, 40000, 50000, 50000, 65000, 80000, 100000, 150000
                }};


    for (int pos = 0; pos < (int)scrollLevels.size(); ++pos) {
        if (scrollLevels[pos] == ticksPerPixel) {
            //std::cout << "tick group size: " << groupSizes[pos] << " - ticksPerPixel: " << ticksPerPixel << std::endl;
            
            return groupSizes[pos];
        }//if
    }//if

    //Should never get there
    return 1000;
}//determineTickCountGroupSize

}//anonymous namespace

void Animation::render(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
    std::map<int, boost::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != NULL) {
        curKeyframes = &instanceOf->keyframes;
    }//if

    if (curKeyframes->empty() == true) {
        return;
    }//if

    int minTick = graphState.verticalPixelTickValues[0];
    int maxTick = graphState.verticalPixelTickValues[graphState.verticalPixelTickValues.size()-1];
    double maxValue = graphState.horizontalPixelValues[0];
    double minValue = graphState.horizontalPixelValues[graphState.horizontalPixelValues.size()-1];

    context->reset_clip();
    context->rectangle(61, 61, areaWidth-61, areaHeight - 61);
    context->clip();

    typedef std::pair<int, boost::shared_ptr<Keyframe> > KeyframeMapType;

    //Render curve
    int lastTimePixel = std::numeric_limits<int>::min();
    int lastValuePixel = std::numeric_limits<int>::min();

    int maxEntryBlockValue = graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl()->maxValue;
    int minEntryBlockValue = graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl()->minValue;
    
    int tickValuesSize = graphState.verticalPixelTickValues.size();
    for (int index = 0; index < tickValuesSize; ++index) {
        int keyTick = graphState.verticalPixelTickValues[index];
        double keyValueBase = sample(keyTick);
        int keyValue = keyValueBase + 0.5;
        if (keyValueBase < 0) {
            keyValue = keyValueBase - 0.5;
        }//if

        if ((keyValue < minValue) || (keyValue > maxValue)) {
            continue;
        }//if

        keyValue = std::min(keyValue, maxEntryBlockValue);
        keyValue = std::max(keyValue, minEntryBlockValue);

        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> valueIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyValue);
        assert(valueIterPair.first != valueIterPair.second);
        int midValOffset = ((double)std::distance(valueIterPair.first, valueIterPair.second) / 2.0);
        unsigned int valuePointerPixel = std::distance(graphState.roundedHorizontalValues.rbegin(), valueIterPair.first) + midValOffset;

        context->set_source_rgba(0.6, 0.3, 0.7, 0.6);
        context->reset_clip();
        context->rectangle(index - 1, areaHeight - valuePointerPixel - 1, 2, 2);
        context->clip();
        context->paint();

        //Draw a faint line between this point and the last one, to ease any discontinuities
        if (lastTimePixel != std::numeric_limits<int>::min()) {
            context->reset_clip();
            context->set_source_rgba(0.2, 0.4, 0.1, 0.5);
            context->set_line_width(1.0);

            context->move_to(index, areaHeight - valuePointerPixel);
            context->line_to(lastTimePixel, lastValuePixel);

            context->stroke();
        }//if

        lastTimePixel = index;
        lastValuePixel = areaHeight - valuePointerPixel;
    }//foreach

    //Render keys
    int selectedRectX = std::numeric_limits<int>::min();
    int selectedRectY = std::numeric_limits<int>::min();

    CurveType::CurveType lastCurveType = CurveType::Init;
    boost::shared_ptr<Keyframe> lastDrawnKey;

    std::map<int, boost::shared_ptr<Keyframe> >::const_iterator keyIter = curKeyframes->begin();
    std::map<int, boost::shared_ptr<Keyframe> >::const_iterator nextKeyIter = keyIter;
    if (nextKeyIter != curKeyframes->end()) {
        ++nextKeyIter;
    }//if

    //BOOST_FOREACH (KeyframeMapType keyPair, *curKeyframes) {
    for (/*nothing*/; keyIter != curKeyframes->end(); ++keyIter) {
        KeyframeMapType keyPair;
        keyPair.first = keyIter->first;
        keyPair.second = keyIter->second;

        context->set_source_rgba(1.0, 0.1, 1.0, 0.7);

        int keyTick = keyPair.second->tick + *startTick;
        int keyValue = keyPair.second->value + 0.5;

        if (keyPair.second->value < 0) {
            keyValue = keyPair.second->value - 0.5;
        }//if

        if ((keyTick < minTick) || (keyTick > maxTick) || (keyValue < minValue) || (keyValue > maxValue)) {
            keyPair.second->drawnStartX = std::numeric_limits<int>::min();
            keyPair.second->drawnStartY = std::numeric_limits<int>::min();

            keyPair.second->drawnOutX = std::numeric_limits<int>::min();
            keyPair.second->drawnOutY = std::numeric_limits<int>::min();

            keyPair.second->drawnInX = std::numeric_limits<int>::min();
            keyPair.second->drawnInY = std::numeric_limits<int>::min();

            lastCurveType = keyPair.second->curveType;
            lastDrawnKey = keyPair.second;

            if (nextKeyIter != curKeyframes->end()) {
                ++nextKeyIter;
            }//if

            continue;
        }//if

        std::vector<int>::iterator timeBound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), keyTick);
        assert(timeBound != graphState.verticalPixelTickValues.end());

        unsigned int timePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), timeBound);

        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> valueIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyValue);
        assert(valueIterPair.first != valueIterPair.second);
        int midValOffset = ((double)std::distance(valueIterPair.first, valueIterPair.second) / 2.0);
        unsigned int valuePointerPixel = std::distance(graphState.roundedHorizontalValues.rbegin(), valueIterPair.first) + midValOffset;

        /*
        if ( (timePointerPixel - 4 <= graphState.curMousePosX) && (timePointerPixel - 4 + 9 >= graphState.curMousePosX) &&
             (areaHeight - valuePointerPixel - 4 <= graphState.curMousePosY) && (areaHeight - valuePointerPixel - 4 >= graphState.curMousePosY) ) {
            selectedRectX = timePointerPixel - 4;
            selectedRectY = areaHeight - valuePointerPixel - 4;

            std::cout << "FOUND" << std::endl;
        } else {
            std::cout << "NOT" << std::endl;
        }//if
        */

        if (KeySelectedType::Key == keyPair.second->selectedState) {
            selectedRectX = timePointerPixel - 4;
            selectedRectY = areaHeight - valuePointerPixel - 4;
        }//if

        keyPair.second->drawnStartX = timePointerPixel - 4;
        keyPair.second->drawnStartY = areaHeight - valuePointerPixel - 4;

//        std::cout << "drawnStartX: " << keyPair.second->drawnStartX << "  --  " << keyPair.second->drawnStartY << std::endl;

        context->reset_clip();
        context->rectangle(timePointerPixel - 4, areaHeight - valuePointerPixel - 4, 9, 9);
        context->clip();
        context->paint();

        //And tangent points, if any
        if (graphState.currentlySelectedKeyframes.size() == 1) {
            boost::shared_ptr<Keyframe> firstKeyframe = graphState.currentlySelectedKeyframes.begin()->second;

            //If we only have a single keyframe selected, and we're currently drawing it and it's bezier
            if ( ( (firstKeyframe == keyPair.second) || ((firstKeyframe == lastDrawnKey)) || 
                   ((nextKeyIter != curKeyframes->end()) && (nextKeyIter->second == firstKeyframe)) ) && 
                 ((keyPair.second->curveType == CurveType::Bezier) || (lastCurveType == CurveType::Bezier) || (nextKeyIter->second->curveType == CurveType::Bezier)) ) {

                bool shouldDrawOutTangent = false;
                bool shouldDrawInTangent = false;

                if ( ( (graphState.currentlySelectedKeyframes.begin()->second == keyPair.second) || 
                       ((nextKeyIter != curKeyframes->end()) && (nextKeyIter->second == firstKeyframe)) ) && 
                      (keyPair.second->curveType == CurveType::Bezier) ) {
                    shouldDrawOutTangent = true;
                }//if

                if (lastCurveType == CurveType::Bezier) {
                    shouldDrawInTangent = true;
                }//if

                //Out
                if (true == shouldDrawOutTangent) {
                    timeBound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), keyPair.second->outTangent[0] + keyTick);

                    if (timeBound != graphState.verticalPixelTickValues.end()) {
                        unsigned int outTangentTimePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), timeBound);

                        valueIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyPair.second->outTangent[1] + keyValue);
                        if (valueIterPair.first != valueIterPair.second) {
                            midValOffset = ((double)std::distance(valueIterPair.first, valueIterPair.second) / 2.0);
                        } else {
                            midValOffset = 0;
                            valueIterPair.first = std::lower_bound(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyPair.second->outTangent[1] + keyValue);
                        }//if

                        if (valueIterPair.first == graphState.roundedHorizontalValues.rbegin()) {
                            if ((*graphState.roundedHorizontalValues.rbegin()) > keyPair.second->outTangent[1] + keyValue) {
                                valueIterPair.first = graphState.roundedHorizontalValues.rend();
                            }//if
                        }//if

                        if (valueIterPair.first != graphState.roundedHorizontalValues.rend()) {
                            unsigned int outTangentValuePointerPixel = std::distance(graphState.roundedHorizontalValues.rbegin(), valueIterPair.first) + midValOffset;

                            context->set_source_rgba(0.1, 0.4, 0.8, 0.7);
                            context->reset_clip();
                            context->rectangle(outTangentTimePointerPixel - 4, areaHeight - outTangentValuePointerPixel - 4, 9, 9);
                            context->clip();
                            context->paint();

                            context->reset_clip();
                            context->set_source_rgba(0.2, 0.4, 0.9, 0.5);
                            context->set_line_width(1.0);

                            context->move_to(timePointerPixel, areaHeight - valuePointerPixel);
                            context->line_to(outTangentTimePointerPixel, areaHeight - outTangentValuePointerPixel);

                            context->stroke();

                            keyPair.second->drawnOutX = outTangentTimePointerPixel - 4;
                            keyPair.second->drawnOutY = areaHeight - outTangentValuePointerPixel - 4;
                        }//if
                    }//if
                }//if

                //In
                if (true == shouldDrawInTangent) {
                    timeBound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), keyTick - keyPair.second->inTangent[0]);
                    if (timeBound != graphState.verticalPixelTickValues.end()) {
                        unsigned int inTangentTimePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), timeBound);

                        valueIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyPair.second->inTangent[1] + keyValue);
                        if (valueIterPair.first != valueIterPair.second) {
                            midValOffset = ((double)std::distance(valueIterPair.first, valueIterPair.second) / 2.0);
                        } else {
                            midValOffset = 0;
                            valueIterPair.first = std::lower_bound(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyPair.second->inTangent[1] + keyValue);
                        }//if

                        if (valueIterPair.first == graphState.roundedHorizontalValues.rbegin()) {
                            if ((*graphState.roundedHorizontalValues.rbegin()) > keyPair.second->inTangent[1] + keyValue) {
                                valueIterPair.first = graphState.roundedHorizontalValues.rend();
                            }//if
                        }//if

                        if (valueIterPair.first != graphState.roundedHorizontalValues.rend()) {
                            unsigned int inTangentValuePointerPixel = std::distance(graphState.roundedHorizontalValues.rbegin(), valueIterPair.first) + midValOffset;

                            context->set_source_rgba(0.1, 0.4, 0.8, 0.7);
                            context->reset_clip();
                            context->rectangle(inTangentTimePointerPixel - 4, areaHeight - inTangentValuePointerPixel - 4, 9, 9);
                            context->clip();
                            context->paint();

                            context->reset_clip();
                            context->set_source_rgba(0.2, 0.4, 0.9, 0.5);
                            context->set_line_width(1.0);

                            context->move_to(timePointerPixel, areaHeight - valuePointerPixel);
                            context->line_to(inTangentTimePointerPixel, areaHeight - inTangentValuePointerPixel);

                            context->stroke();

                            keyPair.second->drawnInX = inTangentTimePointerPixel - 4;
                            keyPair.second->drawnInY = areaHeight - inTangentValuePointerPixel - 4;
                        }//if
                    }//if
                }//if (true == shouldDrawInTangent)
            }//if (keyframe is selected and bezier)
        }//if (graphState.currentlySelectedKeyframe.size() == 1)

        lastCurveType = keyPair.second->curveType;
        lastDrawnKey = keyPair.second;

        if (nextKeyIter != curKeyframes->end()) {
            ++nextKeyIter;
        }//if
    }//foreach

    if (selectedRectX != std::numeric_limits<int>::min()) {
        context->set_source_rgba(0.0, 0.8, 0.0, 0.7);

        context->reset_clip();
        context->rectangle(selectedRectX, selectedRectY, 9, 9);
        context->clip();
        context->paint();
    }//if
}//render

void FMidiAutomationMainWindow::refreshGraphBackground()
{
//    Glib::RefPtr<Gdk::Pixbuf> scaledImagePixbuf = origBackingImage->get_pixbuf()->copy();
    Glib::RefPtr<Gdk::Pixbuf> scaledTexturePixbuf = origBackingTexture->get_pixbuf()->copy();
    
    //backingImage->set(scaledImagePixbuf);
    //backingTexture->set(scaledTexturePixbuf);
    
    scaleImage(backingTexture, scaledTexturePixbuf, drawingAreaWidth, drawingAreaHeight, false);
//    scaleImage(backingImage, scaledImagePixbuf, drawingAreaWidth, drawingAreaHeight, true);
}//refreshGraphBackground

void FMidiAutomationMainWindow::doUIQueuedThreadStuff()
{
    switch (queuedUIThreadOperation) {
        default:
        case UIThreadOperation::Nothing:
            break;

        case UIThreadOperation::finishProcessRecordedMidiOp:
            finishProcessRecordedMidi();
            queuedUIThreadOperation = UIThreadOperation::Nothing;
            break;
    }//switch
}//doUIQueuedThreadStuff

bool FMidiAutomationMainWindow::updateGraph(GdkEventExpose*)
{
    doUIQueuedThreadStuff();

    if (false == graphDrawingArea->is_realized()) {
    	return false;
    }//if

    Glib::RefPtr<Gdk::Window> drawingAreaWindow = graphDrawingArea->get_window();
    Cairo::RefPtr<Cairo::Context> context = drawingAreaWindow->create_cairo_context();
    //Cairo::RefPtr<Cairo::Context> context = drawingAreaWindow->get_cairo_context();
    
    context->save();
    
    Cairo::Format format = Cairo::FORMAT_RGB24;
    Cairo::RefPtr< Cairo::ImageSurface > image_surface_ptr_ = Cairo::ImageSurface::create(format, drawingAreaWidth, drawingAreaHeight);
    
    Gdk::Cairo::set_source_pixbuf (context, backingTexture->get_pixbuf(), 0.0, 0.0);
    context->paint();
    
//    format = Cairo::FORMAT_ARGB32;
//    image_surface_ptr_ = Cairo::ImageSurface::create(format, drawingAreaWidth, drawingAreaHeight);
    
//    int offsetX = (drawingAreaWidth / 2) - (backingImage->get_pixbuf()->get_width() / 2);
//    int offsetY = (drawingAreaHeight / 2) - (backingImage->get_pixbuf()->get_height() / 2);
//    Gdk::Cairo::set_source_pixbuf(context, backingImage->get_pixbuf(), offsetX, offsetY);
//    context->paint();
 
    //Darken negative areas, if any
    if (graphState.zeroithTickPixel != std::numeric_limits<int>::max()) {
        context->reset_clip();
        context->rectangle(0, 61, graphState.zeroithTickPixel, drawingAreaHeight - 61);
        context->clip();

        context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
        context->paint();
    }//if

    graphState.roundedHorizontalValues.clear();
    graphState.roundedHorizontalValues.reserve(graphState.horizontalPixelValues.size());
    BOOST_FOREACH (double val, graphState.horizontalPixelValues) {
        if (val >=0) {
            graphState.roundedHorizontalValues.push_back(val+0.5);
        } else {
            graphState.roundedHorizontalValues.push_back(val-0.5);
        }//if
    }//foreach

    if (graphState.displayMode == DisplayMode::Curve) {
        const boost::shared_ptr<SequencerEntryImpl> entryImpl = graphState.getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl();
        int minValue = entryImpl->minValue;
        int maxValue = entryImpl->maxValue;

        //std::vector<int>::reverse_iterator maxIter = std::upper_bound(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), maxValue);
        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> maxIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), maxValue);

        if (maxIterPair.first != graphState.roundedHorizontalValues.rend()) {
            //int rangeDist = std::distance(maxIterPair.first, maxIterPair.second) / 2.0;
            std::vector<int>::reverse_iterator maxIter = maxIterPair.second;
////            std::advance(maxIter, rangeDist);

            unsigned int dist = (drawingAreaHeight-60) - std::distance(graphState.roundedHorizontalValues.rbegin(), maxIter);

            context->reset_clip();
            context->rectangle(60, 60, drawingAreaWidth-60, dist+1);
            context->clip();

            context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
            context->paint();
        }//if

        //std::vector<int>::reverse_iterator minIter = std::upper_bound(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), minValue);
        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> minIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), minValue);
        if (minIterPair.first != graphState.roundedHorizontalValues.rend()) {
            //int rangeDist = std::distance(minIterPair.first, minIterPair.second) / 2.0;
            std::vector<int>::reverse_iterator minIter = minIterPair.first;

            unsigned int dist = std::distance(graphState.roundedHorizontalValues.rbegin(), minIter);

            context->reset_clip();
            context->rectangle(60, drawingAreaHeight-dist, drawingAreaWidth-60, dist);
            context->clip();

            context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
            context->paint();
        }//if
    }//if

    drawTopBar(context, graphState, drawingAreaWidth, drawingAreaHeight);
    drawTempoBar(context, graphState, datas, drawingAreaWidth, drawingAreaHeight, graphState.verticalPixelTickValues, graphState.ticksPerPixel);
    drawLeftMarker(context, graphState, drawingAreaWidth, drawingAreaHeight);
    drawRightMarker(context, graphState, drawingAreaWidth, drawingAreaHeight);
    drawCurrentTimePointer(context, graphState, drawingAreaWidth, drawingAreaHeight);

    if (graphState.displayMode == DisplayMode::Sequencer) {
        sequencer->drawEntryBoxes(graphDrawingArea, context, graphState, drawingAreaWidth, drawingAreaHeight, graphState.verticalPixelTickValues);
    }//if

    if (graphState.displayMode == DisplayMode::Curve) {
        graphState.getCurrentlySelectedEntryBlock()->renderCurves(context, graphState, drawingAreaWidth, drawingAreaHeight);
        drawLeftBar(context, graphState, drawingAreaWidth, drawingAreaHeight);
    }//if


    /*
    int tmpw;
    int tmph;
    drawingAreaWindow->get_size(tmpw, tmph);
    //std::cout << "width: " << tmpw/2 << " (" << drawingAreaWidth << ")" << " height: " << tmph << " (" << drawingAreaHeight << ")" << std::endl;

    /////////////    
    context->set_line_width(10.0);

    // clip to the area indicated by the expose event so that we only redraw
    // the portion of the window that needs to be redrawn
    //context->rectangle(0, 0, tmpw, tmph);
    //context->clip();

    // draw red lines out from the center of the window
    context->set_source_rgb(0.8, 0.0, 0.0);
    context->move_to(0, 0);
    context->line_to(tmpw/2, tmph/2);
    context->line_to(0, tmph);
    context->move_to(tmpw/2, tmph/2);
    context->line_to(tmpw, tmph/2);
    context->stroke();

    //context->paint();
    */
    
    context->restore();
    
    //graphDrawingArea->show();
    
    updateTempoBox(graphState, datas, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);

    return true;
}//updateGraph

GraphState::GraphState()
{
    doInit();
}//constructor

void GraphState::doInit()
{
    offsetX = 0;
    offsetY = 0;
    barsSubdivisionAmount = 1;
    ticksPerPixel = 50;
    inMotion = false;
    zeroithTickPixel = std::numeric_limits<int>::max();
    curPointerTick = 0;
    curPointerTickXPixel = 0;
    selectedEntity = Nobody;
    leftMarkerTick = -1;
    rightMarkerTick = -1;
    leftMarkerTickXPixel = -1;
    rightMarkerTickXPixel = -1;
    displayMode = DisplayMode::Sequencer;
    selectedEntity = Nobody;
    curMousePosX = -100;
    curMousePosY = -100;
}//doInit

GraphState::~GraphState()
{
    //Nothing
}//destructor

boost::shared_ptr<SequencerEntryBlock> GraphState::getCurrentlySelectedEntryBlock()
{
    if (currentlySelectedEntryBlocks.empty() == true) {
        return boost::shared_ptr<SequencerEntryBlock>();
    } else {
        return ((*currentlySelectedEntryBlocks.begin()).second);
    }//if
}//getCurrentlySelectedEntryBlock

void GraphState::refreshHorizontalLines(unsigned int areaWidth, unsigned int areaHeight)
{
    if (displayMode != DisplayMode::Curve) {
        return;
    }//if

    if (std::numeric_limits<double>::max() == valuesPerPixel) {
        const boost::shared_ptr<SequencerEntryImpl> entryImpl = getCurrentlySelectedEntryBlock()->getOwningEntry()->getImpl();
        int minValue = entryImpl->minValue;
        int maxValue = entryImpl->maxValue;

        int delta = maxValue - minValue;
        valuesPerPixel = (double)delta / (double)(areaHeight - 60);
        valuesPerPixel *= 1.05;

//        std::cout << "min: " << minValue << "   max: " << maxValue << "   delta: " << delta << "   valuesPerPixel: " << valuesPerPixel << std::endl;
    }//if

    horizontalLines.clear();
    valueLineText.clear();
    horizontalPixelValues.clear();
    horizontalPixelValues.resize(areaHeight-60);

    double minPixelDist = 40;
    double firstValue = 0 * valuesPerPixel + offsetY * valuesPerPixel;
    double lastValue = (areaHeight-60) * valuesPerPixel + offsetY * valuesPerPixel;
    minPixelDist = std::max(minPixelDist,  (double)(areaHeight-60) / fabs(firstValue-lastValue));

//    unsigned int lastYTick = (-minPixelDist) + fmod(offsetY, minPixelDist);

//    std::cout << "lastYTick: " << lastYTick << "   -  " << offsetY << std::endl;

    unsigned int lastYTick = 0;

    int lastLineValue = std::numeric_limits<int>::max();
    for (unsigned int y = 0; y < areaHeight-60; ++y) {
        double value = y * valuesPerPixel + offsetY * valuesPerPixel;
        int roundedValue = 0;
        if (value > 0) {
            roundedValue = value + 0.5;
        } else {
            roundedValue = value - 0.5;
        }//if
        
        horizontalPixelValues[areaHeight-60-y-1] = value;

//        std::cout << "y: " << areaHeight-60-y << ":  " << roundedValue << "    " << areaHeight-60 << std::endl;

        if (((y - lastYTick) > minPixelDist) && (roundedValue != lastLineValue)) {
            lastLineValue = roundedValue;
            lastYTick = y;

            horizontalLines.push_back(std::make_pair(areaHeight-60-y-1, ValueLine));

            std::ostringstream tmpSS;
            tmpSS << roundedValue;
            valueLineText.push_back(std::make_pair(areaHeight-60-y-1, tmpSS.str()));
        }//if
    }//for

//    std::cout << "minPixelDist: " << minPixelDist << std::endl;
}//refreshHorizontalLines

void GraphState::refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight)
{
    //Ugly kluge to handle the case where if we've already reached the half-way zeroith point and keep scrolling over too far, bad things happen
    static int lastOffsetX = std::numeric_limits<int>::max();
    if (((areaWidth-2) * ticksPerPixel + offsetX * ticksPerPixel) < 0) {
        if (lastOffsetX == std::numeric_limits<int>::max()) {
            lastOffsetX = offsetX;
        }//if

        offsetX = lastOffsetX;
        return;
    } else {
        lastOffsetX = std::numeric_limits<int>::max();
    }//if

    verticalLines.clear();
    upperLineText.clear();

    int tickCountGroupSize = determineTickCountGroupSize(ticksPerPixel);

    int zeroithTickCount = 0;
    if ((displayMode == DisplayMode::Curve) && (getCurrentlySelectedEntryBlock() != NULL)) {
        zeroithTickCount = getCurrentlySelectedEntryBlock()->getStartTick();
    }//if

    int realZeroithTickPixel = std::numeric_limits<int>::max();

    //Determine frame ticks
    bool toggle = true;
    verticalPixelTickValues.clear();
    verticalPixelTickValues.reserve(areaWidth);
    zeroithTickPixel = std::numeric_limits<int>::max();
    int lastRecordedTickCount = std::numeric_limits<int>::max(); //I don't like using this, but I'm a little nervous about just skipping x ahead when appropriate.. only for case when ticksPerPixel < 0
    for (unsigned int x = 0; x < areaWidth; ++x) {
        if (ticksPerPixel > 1) {
            int tickCount = x * ticksPerPixel + offsetX * ticksPerPixel;

            int absTickCountModded = tickCount % tickCountGroupSize;
            if (absTickCountModded < 0) {
                absTickCountModded = -absTickCountModded;
            }//if

            verticalPixelTickValues.push_back(tickCount);

            if ((zeroithTickCount > tickCount) && (x > 0)) {
                zeroithTickPixel = x;
            }//if

////            std::cout << "absTickCountModded: " << absTickCountModded << "  --  tickCount: " << tickCount << "   offsetX: " << offsetX << "   x: " << x << std::endl;
            if (absTickCountModded < ticksPerPixel) { //XXX: <=??
                tickCount = tickCount - (tickCount % 1000);

                if (tickCount == lastRecordedTickCount) {
                    continue;
                }//if
        
                lastRecordedTickCount = tickCount;

                verticalLines.push_back(std::make_pair(x, SecondLine));

                if ((0 == tickCount) && (x > 0)) {
                    realZeroithTickPixel = x - 1;
                }//if

                std::ostringstream tmpSS;
                tmpSS << tickCount;
                upperLineText.push_back(std::make_pair(x, tmpSS.str()));
            }//if
        } else {
            float tickCountBase = ((float)(x + offsetX)) / ((float)(-ticksPerPixel));
            int tickCount = (int)(fabs(tickCountBase) + 0.5f);

            if (1 == ticksPerPixel) {
                tickCount = (int)(((float)(x + offsetX)) / ((float)(ticksPerPixel)) + 0.5f);
            }//if

            verticalPixelTickValues.push_back(tickCount);

            int absTickCountModded = tickCount % tickCountGroupSize;
            if (absTickCountModded < 0) {
                absTickCountModded = -absTickCountModded;
            }//if

//std::cout << "x: " << x << "     zeroithTickCount: " << zeroithTickCount << "      tickCount: " << tickCount << std::endl;                
            if ((zeroithTickCount > tickCount) && (x > 0)) {
                zeroithTickPixel = x;
            }//if

            if (0 == absTickCountModded) {
                tickCount = tickCount - (tickCount % tickCountGroupSize);

                if (tickCount == lastRecordedTickCount) {
                    continue;
                }//if
        
                lastRecordedTickCount = tickCount;

                verticalLines.push_back(std::make_pair(x, SecondLine));

                if ((1 != ticksPerPixel) && ((tickCountBase + 0.5f) < 0)) {
                    tickCount = -tickCount;
                }//if

                 if ((0 == tickCount) && (x > 0)) {
                    realZeroithTickPixel = x - 1;
                }//if

                if ((ticksPerPixel >= -42) || (true == toggle)) {
                    std::ostringstream tmpSS;
                    tmpSS << tickCount;
                    upperLineText.push_back(std::make_pair(x, tmpSS.str()));
                }//if

                toggle = !toggle;

//std::cout << "x: " << x << " - offset: " << offset << " - tickCount: " << tickCount << " absTickCountModded: " << absTickCountModded << " --- " << tmpSS.str() << std::endl;
            }//if
        }//if
    }//for

    //Our vertical tick values might not be correct for negative ticks
    if (realZeroithTickPixel != std::numeric_limits<int>::max()) {
        for (int pos = 0; pos < realZeroithTickPixel; ++pos) {
            if (verticalPixelTickValues[pos] > 0) {
                verticalPixelTickValues[pos] = -verticalPixelTickValues[pos];
            }//if
        }//for
    }//if

    //Ugly kluge
    if (ticksPerPixel < 0) {
        if ((verticalLines.empty() == false) && (verticalLines[0].first == 0) && (verticalLines[0].second == SecondLine)) {
            verticalLines.erase(verticalLines.begin());
            upperLineText.erase(upperLineText.begin());
        }//if
    }//if

    //Another ugly kluge
    if (std::numeric_limits<int>::max() == zeroithTickPixel) {
        zeroithTickPixel = realZeroithTickPixel;
    }//if
    
/*
    for (int pos = 0; pos < verticalPixelTickValues.size(); ++pos) {
        std::cout << "verticalPixelTickValues[" << pos << "]: " << verticalPixelTickValues[pos] << std::endl;
    }//foreach
    std::cout << std::endl;
    std::cout << "zeroithTickPixel: " << zeroithTickPixel << std::endl;
    std::cout << std::endl;
*/
}//refreshVerticalLines

void GraphState::setOffsetCenteredOnValue(double value, int drawingAreaHeight)
{
    value = std::max(value, 0.0);

    drawingAreaHeight -= 60;
    offsetY = value / valuesPerPixel - drawingAreaHeight / 2.0; //close enough..
}//setOffsetCenteredOnValue

void GraphState::setOffsetCenteredOnTick(int tick, int drawingAreaWidth)
{
    if (ticksPerPixel > 1) {
        int fullWindowTicks = drawingAreaWidth * ticksPerPixel;
        int halfWindowTicks = fullWindowTicks / 2;
        int halfWindowsToSkip = tick / halfWindowTicks;
        int remaningTicks = tick % halfWindowTicks;

        offsetX = (halfWindowsToSkip * (drawingAreaWidth/2)) + (remaningTicks / ticksPerPixel) - (drawingAreaWidth / 2);
    } else {
        //int fullWindowTicks = drawingAreaWidth / abs(graphState.ticksPerPixel);
        //int halfWindowTicks = fullWindowTicks / 2;
        int baseOffset = abs(ticksPerPixel) * tick;

        offsetX = baseOffset - (drawingAreaWidth / 2); // - halfWindowTicks;
    }//if
}//setOffsetCenteredOnTick


