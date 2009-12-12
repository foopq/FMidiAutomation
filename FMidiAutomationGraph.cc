#include <gtkmm.h>
#include <libglademm.h>
#include <gdkmm/general.h>
#include <cairomm/surface.h>
#include <iostream>
#include <sstream>
#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
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
    context->set_source_rgba(0.3, 0.3, 0.3, 0.3);
    context->set_line_width(1.0);
    for (std::vector<std::pair<unsigned int, LineType> >::const_iterator valueLineIter = graphState.horizontalLines.begin(); valueLineIter != graphState.horizontalLines.end(); ++valueLineIter) {
        context->move_to(61, valueLineIter->first + 60);
        context->line_to(areaWidth, valueLineIter->first + 60);
    }//for

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

void FMidiAutomationMainWindow::refreshGraphBackground()
{
//    Glib::RefPtr<Gdk::Pixbuf> scaledImagePixbuf = origBackingImage->get_pixbuf()->copy();
    Glib::RefPtr<Gdk::Pixbuf> scaledTexturePixbuf = origBackingTexture->get_pixbuf()->copy();
    
    //backingImage->set(scaledImagePixbuf);
    //backingTexture->set(scaledTexturePixbuf);
    
    scaleImage(backingTexture, scaledTexturePixbuf, drawingAreaWidth, drawingAreaHeight, false);
//    scaleImage(backingImage, scaledImagePixbuf, drawingAreaWidth, drawingAreaHeight, true);
}//refreshGraphBackground

bool FMidiAutomationMainWindow::updateGraph(GdkEventExpose*)
{
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

    std::vector<int> roundedHorizontalValues;
    roundedHorizontalValues.reserve(graphState.horizontalPixelValues.size());
    BOOST_FOREACH (double val, graphState.horizontalPixelValues) {
        if (val >=0) {
            roundedHorizontalValues.push_back(val+0.5);
        } else {
            roundedHorizontalValues.push_back(val-0.5);
        }//if
    }//foreach

    if (graphState.displayMode == DisplayMode::Curve) {
        const boost::shared_ptr<SequencerEntryImpl> entryImpl = graphState.currentlySelectedEntryBlock->getOwningEntry()->getImpl();
        int minValue = entryImpl->minValue;
        int maxValue = entryImpl->maxValue;

        //std::vector<int>::reverse_iterator maxIter = std::upper_bound(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), maxValue);
        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> maxIterPair = std::equal_range(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), maxValue);

        if (maxIterPair.first != roundedHorizontalValues.rend()) {
            int rangeDist = std::distance(maxIterPair.first, maxIterPair.second) / 2.0;
            std::vector<int>::reverse_iterator maxIter = maxIterPair.second;
////            std::advance(maxIter, rangeDist);

            unsigned int dist = (drawingAreaHeight-60) - std::distance(roundedHorizontalValues.rbegin(), maxIter);

            context->reset_clip();
            context->rectangle(60, 60, drawingAreaWidth-60, dist+1);
            context->clip();

            context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
            context->paint();
        }//if

        //std::vector<int>::reverse_iterator minIter = std::upper_bound(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), minValue);
        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> minIterPair = std::equal_range(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), minValue);
        if (minIterPair.first != roundedHorizontalValues.rend()) {
            int rangeDist = std::distance(minIterPair.first, minIterPair.second) / 2.0;
            std::vector<int>::reverse_iterator minIter = minIterPair.first;

            unsigned int dist = std::distance(roundedHorizontalValues.rbegin(), minIter);

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
    offsetX = 0;
    offsetY = 0;
    barsSubdivisionAmount = 1;
    ticksPerPixel = 2;
    inMotion = false;
    zeroithTickPixel = std::numeric_limits<int>::min();
    curPointerTick = 0;
    curPointerTickXPixel = 0;
    selectedEntity = Nobody;
    leftMarkerTick = -1;
    rightMarkerTick = -1;
    leftMarkerTickXPixel = -1;
    rightMarkerTickXPixel = -1;
}//constructor

GraphState::~GraphState()
{
    //Nothing
}//destructor

void GraphState::refreshHorizontalLines(unsigned int areaWidth, unsigned int areaHeight)
{
    if (displayMode != DisplayMode::Curve) {
        return;
    }//if

    if (std::numeric_limits<double>::max() == valuesPerPixel) {
        const boost::shared_ptr<SequencerEntryImpl> entryImpl = currentlySelectedEntryBlock->getOwningEntry()->getImpl();
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

////            std::cout << "absTickCountModded: " << absTickCountModded << "  --  tickCount: " << tickCount << "   offsetX: " << offsetX << "   x: " << x << std::endl;
            if (absTickCountModded < ticksPerPixel) { //XXX: <=??
                tickCount = tickCount - (tickCount % 1000);

                if (tickCount == lastRecordedTickCount) {
                    continue;
                }//if
        
                lastRecordedTickCount = tickCount;

                verticalLines.push_back(std::make_pair(x, SecondLine));

                if ((0 == tickCount) && (x > 0)) {
                    zeroithTickPixel = x - 1;                    
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
                    zeroithTickPixel = x - 1;
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
    if (zeroithTickPixel != std::numeric_limits<int>::max()) {
        for (int pos = 0; pos < zeroithTickPixel; ++pos) {
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


