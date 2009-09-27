#include <gtkmm.h>
#include <libglademm.h>
#include <gdkmm/general.h>
#include <cairomm/surface.h>
#include <iostream>
#include <sstream>
#include "FMidiAutomationMainWindow.h"
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


    drawTopBar(context, graphState, drawingAreaWidth, drawingAreaHeight);
    drawCurrentTimePointer(context, graphState, drawingAreaWidth, drawingAreaHeight);

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
    
    return true;
}//updateGraph

GraphState::GraphState()
{
    offset = 0;
    barsSubdivisionAmount = 1;
    ticksPerPixel = 2;
    inMotion = false;
    zeroithTickPixel = std::numeric_limits<int>::min();
    curPointerTick = 0;
}//constructor

GraphState::~GraphState()
{
    //Nothing
}//destructor

void GraphState::refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight)
{
    verticalLines.clear();
    upperLineText.clear();
    lowerLineText.clear();

    int tickCountGroupSize = determineTickCountGroupSize(ticksPerPixel);

//std::cout << "ticksPerPixel: " << ticksPerPixel << " -- tickCountGroupSize: " << tickCountGroupSize << std::endl;

    //Determine frame ticks
    verticalPixelTickValues.clear();
    zeroithTickPixel = std::numeric_limits<int>::max();
    int lastRecordedTickCount = std::numeric_limits<int>::max(); //I don't like using this, but I'm a little nervous about just skipping x ahead when appropriate.. only for case when ticksPerPixel < 0
    for (unsigned int x = 0; x < areaWidth; ++x) {
        if (ticksPerPixel > 1) {
            int tickCount = x * ticksPerPixel + offset * ticksPerPixel;

            int absTickCountModded = tickCount % tickCountGroupSize;
            if (absTickCountModded < 0) {
                absTickCountModded = -absTickCountModded;
            }//if

            verticalPixelTickValues.push_back(tickCount);

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
            float tickCountBase = ((float)(x + offset)) / ((float)(-ticksPerPixel));
            int tickCount = (int)(fabs(tickCountBase) + 0.5f);

            if (1 == ticksPerPixel) {
                tickCount = (int)(((float)(x + offset)) / ((float)(ticksPerPixel)) + 0.5f);
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

                std::ostringstream tmpSS;
                tmpSS << tickCount;
                upperLineText.push_back(std::make_pair(x, tmpSS.str()));

//std::cout << "x: " << x << " - offset: " << offset << " - tickCount: " << tickCount << " absTickCountModded: " << absTickCountModded << " --- " << tmpSS.str() << std::endl;
            }//if
        }//if
    }//for

    //Ugly kluge
    if (ticksPerPixel < 0) {
        if ((verticalLines.empty() == false) && (verticalLines[0].first == 0) && (verticalLines[0].second == SecondLine)) {
            verticalLines.erase(verticalLines.begin());
            upperLineText.erase(upperLineText.begin());
        }//if
    }//if
    
}//refreshVerticalLines


