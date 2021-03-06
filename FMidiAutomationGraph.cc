/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/



//#include <libglademm.h>
//#include <gdkmm/general.h>
//#include <cairomm/surface.h>
#include <iostream>
#include <sstream>
#include "UI/SequencerEntryUI.h"
#include "Data/SequencerEntry.h"
#include "UI/SequencerUI.h"
#include "Animation.h"
#include <boost/array.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include "GraphState.h"
#include "Globals.h"
#include "FMidiAutomationMainWindow.h"
#include "Tempo.h"
#include "ProcessRecordedMidi.h"

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

    enum class ScaleMode : char
    {
        SCALE_WIDTH,
        SCALE_HEIGHT,
        SCALE_NONE
    };//ScaleMode

    ScaleMode scale_mode = ScaleMode::SCALE_NONE; //Start with either the width or height, and scale the other according to the ratio.

    const int pixbuf_height = pixbuf->get_height();
    const int pixbuf_width = pixbuf->get_width();

    if (pixbuf_height > target_height) {
	if (pixbuf_width > target_width) {
	    //Both are bigger than the target, so find the biggest one:
	    if (pixbuf_width > pixbuf_height) {
		scale_mode = ScaleMode::SCALE_WIDTH;
	    } else {
		scale_mode = ScaleMode::SCALE_HEIGHT;
	    }//if
	} else {
	    //Only the height is bigger:
	    scale_mode = ScaleMode::SCALE_HEIGHT;
	}//if
    } else {
	if (pixbuf_width > target_width) {
	    //Only the height is bigger:
	    scale_mode = ScaleMode::SCALE_WIDTH;
	}//if
    }//if

    if (scale_mode == ScaleMode::SCALE_NONE) {
	return pixbuf;
    } else {
	if(scale_mode == ScaleMode::SCALE_HEIGHT) {
	    const float ratio = (float)target_height / (float)pixbuf_height; 
	    target_width = (int)((float)pixbuf_width * ratio);
	} else {
	    if (scale_mode == ScaleMode::SCALE_WIDTH) {
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
    
void scaleImage(std::shared_ptr<Gtk::Image> image, Glib::RefPtr<Gdk::Pixbuf> pixbuf, int width, int height, bool maintainAspect)
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

void drawRubberBand(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, 
                        gdouble mousePressDownX, gdouble mousePressDownY, gdouble mousePosX, gdouble mousePosY)
{
    mousePressDownX = std::max<gdouble>(mousePressDownX, 0);
    mousePressDownX = std::min<gdouble>(mousePressDownX, areaWidth);

    mousePressDownY = std::max<gdouble>(mousePressDownY, 61);
    mousePressDownY = std::min<gdouble>(mousePressDownY, areaHeight);

    mousePosX = std::max<gdouble>(mousePosX, 0);
    mousePosX = std::min<gdouble>(mousePosX, areaWidth);

    mousePosY = std::max<gdouble>(mousePosY, 61);
    mousePosY = std::min<gdouble>(mousePosY, areaHeight);

int dashOffset = 0;
//dashOffset++;
std::vector<double> dashes;
dashes.push_back(10);

    context->set_dash(dashes, dashOffset);

    if ((fabs(mousePosX - mousePressDownX) > 2) && (fabs(mousePosY - mousePressDownY) > 2)) {
        context->reset_clip();
        context->set_source_rgba(1.0, 0.0, 0.0, 0.75);
        context->set_line_width(2.0);

/*        
        context->move_to(mousePressDownX, mousePressDownY);
        context->line_to(mousePosX, mousePressDownY);
        context->line_to(mousePosX, mousePosY);
        context->line_to(mousePressDownX, mousePosY);
        context->line_to(mousePressDownX, mousePressDownY);
*/

        if (mousePosX < mousePressDownX) {
            std::swap(mousePosX, mousePressDownX);
        }//if

        if (mousePosY < mousePressDownY) {
            std::swap(mousePosY, mousePressDownY);
        }//if

/*
        context->move_to(mousePressDownX, mousePressDownY);
        context->line_to(mousePosX, mousePressDownY);

        context->move_to(mousePressDownX, mousePosY);
        context->line_to(mousePosX, mousePosY);

        context->move_to(mousePressDownX, mousePosY);
        context->line_to(mousePressDownX, mousePressDownY);

        context->move_to(mousePosX, mousePosY);
        context->line_to(mousePosX, mousePressDownY);
*/

        context->rectangle(mousePressDownX, mousePressDownY, mousePosX - mousePressDownX, mousePosY - mousePressDownY);

        context->stroke();

        context->reset_clip();
    }//if

    dashes.clear();
    context->set_dash(dashes, 0);
}//drawRubberBand

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

bool EntryBlockSelectionState::HasSelected()
{
//    std::cout << "HasSelected: " << currentlySelectedEntryBlocks.size() << " " << (currentlySelectedEntryBlocks.empty() == false) << " " << this << std::endl;

    return (currentlySelectedEntryBlocks.empty() == false);
}//HasSelected

std::shared_ptr<SequencerEntryBlockUI> EntryBlockSelectionState::GetFirstEntryBlock()
{
    if (HasSelected() == true) {
        return ((*currentlySelectedEntryBlocks.begin()).second);
    } else {
        return std::shared_ptr<SequencerEntryBlockUI>();
    }//if
}//GetFirstEntryBlock

void EntryBlockSelectionState::ClearSelected()
{
//    std::cout << "EntryBlockSelectionState::ClearSelected() " << this << std::endl;

    currentlySelectedEntryBlocks.clear();
    currentlySelectedEntryOriginalStartTicks.clear();
}//ClearSelected

std::multimap<int, std::shared_ptr<SequencerEntryBlockUI> > EntryBlockSelectionState::GetEntryBlocksMapCopy()
{
    return currentlySelectedEntryBlocks;
}//GetEntryBlocksMapCopy

bool EntryBlockSelectionState::IsSelected(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    if (currentlySelectedEntryOriginalStartTicks.find(entryBlock) != currentlySelectedEntryOriginalStartTicks.end()) {
        return true;
    } else {
        return false;
    }//if
}//IsSelected

bool EntryBlockSelectionState::IsOrigSelected(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    if (origSelectedEntryBlocks.find(entryBlock) != origSelectedEntryBlocks.end()) {
        return true;
    } else {
        return false;
    }//if
}//IsOrigSelected

void EntryBlockSelectionState::ResetRubberbandingSelection()
{
    origSelectedEntryBlocks.clear();

    for (auto entryBlockIter = currentlySelectedEntryOriginalStartTicks.begin(); 
            entryBlockIter != currentlySelectedEntryOriginalStartTicks.end(); ++entryBlockIter) {
        origSelectedEntryBlocks.insert(entryBlockIter->first);
    }//for
}//ResetRubberbandingSelection

std::set<std::shared_ptr<SequencerEntryBlockUI> > EntryBlockSelectionState::GetOrigSelectedEntryBlocksCopy()
{
    std::cout << "EntryBlockSelectionState::GetOrigSelectedEntryBlocksCopy: " << origSelectedEntryBlocks.size() << std::endl;

    return origSelectedEntryBlocks;
}//GetOrigSelectedEntryBlocksCopy

int EntryBlockSelectionState::GetNumSelected()
{
    return currentlySelectedEntryBlocks.size();
}//GetNumSelected

int EntryBlockSelectionState::GetOriginalStartTick(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    if (currentlySelectedEntryOriginalStartTicks.find(entryBlock) != currentlySelectedEntryOriginalStartTicks.end()) {
        return currentlySelectedEntryOriginalStartTicks[entryBlock];
    } else {
        return 0;
    }//if
}//GetOriginalStartTick

std::map<std::shared_ptr<SequencerEntryBlockUI>, int> EntryBlockSelectionState::GetEntryOriginalStartTicksCopy()
{
    return currentlySelectedEntryOriginalStartTicks;
}//GetEntryOriginalStartTicksCopy

fmaipair<decltype(EntryBlockSelectionState::currentlySelectedEntryBlocks.begin()), decltype(EntryBlockSelectionState::currentlySelectedEntryBlocks.end())> EntryBlockSelectionState::GetCurrentlySelectedEntryBlocks()
{
    return fmai_make_pair(currentlySelectedEntryBlocks.begin(), currentlySelectedEntryBlocks.end());
}//GetCurrentlySelectedEntryBlocks

void EntryBlockSelectionState::SetCurrentlySelectedEntryOriginalStartTicks(std::map<std::shared_ptr<SequencerEntryBlockUI>, int> &origStartTicks)
{
    currentlySelectedEntryOriginalStartTicks.swap(origStartTicks);
}//SetCurrentlySelectedEntryOriginalStartTicks

void EntryBlockSelectionState::AddSelectedEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
//    std::cout << "EntryBlockSelectionState::AddSelectedEntryBlock: " << entryBlock.get() << " - " << this << std::endl;

    currentlySelectedEntryOriginalStartTicks[entryBlock]  = entryBlock->getBaseEntryBlock()->getStartTick();
    currentlySelectedEntryBlocks.insert(std::make_pair(entryBlock->getBaseEntryBlock()->getStartTick(), entryBlock));
}//AddSelectedEntryBlock

void EntryBlockSelectionState::RemoveSelectedEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
//    std::cout << "EntryBlockSelectionState::RemoveSelectedEntryBlock: " << entryBlock.get() << " - " << this << std::endl;

    auto tickRangePair = currentlySelectedEntryBlocks.equal_range(entryBlock->getBaseEntryBlock()->getStartTick());
    for (auto tickRangeIter = tickRangePair.first; tickRangeIter != tickRangePair.second; ++tickRangeIter) {
        if (tickRangeIter->second == entryBlock) {
            currentlySelectedEntryBlocks.erase(tickRangeIter);
            break;
        }//if
    }//for

    auto tickIter = currentlySelectedEntryOriginalStartTicks.find(entryBlock);
    assert(tickIter != currentlySelectedEntryOriginalStartTicks.end());
    currentlySelectedEntryOriginalStartTicks.erase(tickIter);
}//RemoveSelectedEntryBlock

bool KeyframeSelectionState::HasSelected()
{
    return (currentlySelectedKeyframes.empty() == false);
}//HasSelected

int KeyframeSelectionState::GetNumSelected()
{
    return currentlySelectedKeyframes.size();
}//GetNumSelected

std::shared_ptr<Keyframe> KeyframeSelectionState::GetFirstKeyframe()
{
    if (HasSelected() == true) {
        return currentlySelectedKeyframes.begin()->second;
    } else {
        return std::shared_ptr<Keyframe>();
    }//if
}//GetFirstEntryBlock

std::map<int, std::shared_ptr<Keyframe> > KeyframeSelectionState::GetSelectedKeyframesCopy()
{
    return currentlySelectedKeyframes;
}//GetSelectedKeyframesCopy

bool KeyframeSelectionState::IsOrigSelected(std::shared_ptr<Keyframe> keyframe)
{
   if (origSelectedKeyframes.find(keyframe) != origSelectedKeyframes.end()) {
       return true;
   } else {
       return false;
   }//if
}//IsOrigSelected

bool KeyframeSelectionState::IsSelected(std::shared_ptr<Keyframe> keyframe)
{
    for (auto selectedKeyIter : currentlySelectedKeyframes) {
        if (selectedKeyIter.second == keyframe) {
            return true;
        }//if
    }//foreach

    return false;
}//IsSelected

void KeyframeSelectionState::RemoveKeyframe(std::shared_ptr<Keyframe> keyframe)
{
    for (auto selectedKeyIter = currentlySelectedKeyframes.begin(); selectedKeyIter != currentlySelectedKeyframes.end(); ++selectedKeyIter) {
        if (selectedKeyIter->second == keyframe) {
            currentlySelectedKeyframes.erase(selectedKeyIter);

            auto tickIter = movingKeyOrigTicks.find(keyframe);
            if (tickIter != movingKeyOrigTicks.end()) {
                movingKeyOrigTicks.erase(tickIter);
            }//if

            auto valueIter = movingKeyOrigValues.find(keyframe);
            if (valueIter != movingKeyOrigValues.end()) {
                movingKeyOrigValues.erase(valueIter);
            }//if
        }//if
    }//for
}//RemoveKeyframe

void KeyframeSelectionState::AddKeyframe(std::shared_ptr<Keyframe> keyframe)
{
    currentlySelectedKeyframes.insert(std::make_pair(keyframe->tick, keyframe));
}//AddKeyframe

void KeyframeSelectionState::ClearSelectedKeyframes(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    for (auto keyIter : currentlySelectedKeyframes) {
        entryBlock->setSelectedState(keyIter.second, KeySelectedType::NotSelected);
    }//for
//    std::cout << "NotSelected2" << std::endl;

    currentlySelectedKeyframes.clear();
    movingKeyOrigTicks.clear();
    movingKeyOrigValues.clear();
}//ClearSelectedKeyframes

int KeyframeSelectionState::GetOrigTick(std::shared_ptr<Keyframe> keyframe)
{
    if (movingKeyOrigTicks.find(keyframe) != movingKeyOrigTicks.end()) {
        return movingKeyOrigTicks[keyframe];
    } else {
        return 0;
    }//if
}//GetOrigTick

double KeyframeSelectionState::GetOrigValue(std::shared_ptr<Keyframe> keyframe)
{
    if (movingKeyOrigValues.find(keyframe) != movingKeyOrigValues.end()) {
        return movingKeyOrigValues[keyframe];
    } else {
        return 0.0;
    }//if
}//GetOrigValue

fmaipair<decltype(KeyframeSelectionState::currentlySelectedKeyframes.begin()), decltype(KeyframeSelectionState::currentlySelectedKeyframes.end())> KeyframeSelectionState::GetCurrentlySelectedKeyframes()
{
    return fmai_make_pair(currentlySelectedKeyframes.begin(), currentlySelectedKeyframes.end());
}//GetCurrentlySelectedEntryBlocks

void KeyframeSelectionState::AddOrigKeyframe(std::shared_ptr<Keyframe> keyframe)
{
    movingKeyOrigTicks[keyframe] = keyframe->tick;
    movingKeyOrigValues[keyframe] = keyframe->value;
}//AddOrigKeyframe

void KeyframeSelectionState::ResetRubberbandingSelection()
{
    origSelectedKeyframes.clear();
    for (auto keyIter : currentlySelectedKeyframes) {
        origSelectedKeyframes.insert(keyIter.second);
    }//for
}//ResetRubberbandingSelection

void KeyframeSelectionState::SetCurrentlySelectedKeyframes(std::map<int, std::shared_ptr<Keyframe> > &origSelectedKeyframes)
{
    currentlySelectedKeyframes.swap(origSelectedKeyframes);
}//SetCurrentlySelectedKeyframes

void Animation::render(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    std::map<int, std::shared_ptr<Keyframe> > *curKeyframes = &keyframes;
    if (instanceOf != nullptr) {
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

    typedef std::pair<int, std::shared_ptr<Keyframe> > KeyframeMapType;

    //Render curve
    int lastTimePixel = std::numeric_limits<int>::min();
    int lastValuePixel = std::numeric_limits<int>::min();

    std::shared_ptr<SequencerEntryBlockUI> firstSelectedEntryBlock = graphState.entryBlockSelectionState.GetFirstEntryBlock();

    int maxEntryBlockValue = firstSelectedEntryBlock->getBaseEntryBlock()->getOwningEntry()->getImpl()->maxValue;
    int minEntryBlockValue = firstSelectedEntryBlock->getBaseEntryBlock()->getOwningEntry()->getImpl()->minValue;
    
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

        auto valueIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyValue);
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
    std::vector<int> selectedRectX; // = std::numeric_limits<int>::min();
    std::vector<int> selectedRectY; // = std::numeric_limits<int>::min();

    CurveType lastCurveType = CurveType::Init;
    std::shared_ptr<Keyframe> lastDrawnKey;

    std::map<int, std::shared_ptr<Keyframe> >::const_iterator keyIter = curKeyframes->begin();
    std::map<int, std::shared_ptr<Keyframe> >::const_iterator nextKeyIter = keyIter;
    if (nextKeyIter != curKeyframes->end()) {
        ++nextKeyIter;
    }//if

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

        lastCurveType = keyPair.second->curveType;

        if ((keyTick < minTick) || (keyTick > maxTick) || (keyValue < minValue) || (keyValue > maxValue)) {
            keyPair.second->drawnStartX = std::numeric_limits<int>::min();
            keyPair.second->drawnStartY = std::numeric_limits<int>::min();

            keyPair.second->drawnOutX = std::numeric_limits<int>::min();
            keyPair.second->drawnOutY = std::numeric_limits<int>::min();

            keyPair.second->drawnInX = std::numeric_limits<int>::min();
            keyPair.second->drawnInY = std::numeric_limits<int>::min();

            lastDrawnKey = keyPair.second;

            if (nextKeyIter != curKeyframes->end()) {
                ++nextKeyIter;
            }//if

            continue;
        }//if

        std::vector<int>::iterator timeBound = std::lower_bound(graphState.verticalPixelTickValues.begin(), graphState.verticalPixelTickValues.end(), keyTick);
        assert(timeBound != graphState.verticalPixelTickValues.end());

        unsigned int timePointerPixel = std::distance(graphState.verticalPixelTickValues.begin(), timeBound);

        auto valueIterPair = std::equal_range(graphState.roundedHorizontalValues.rbegin(), graphState.roundedHorizontalValues.rend(), keyValue);
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

        if (KeySelectedType::Key == entryBlock->getSelectedState(keyPair.second)) {
            selectedRectX.push_back(timePointerPixel - 4);
            selectedRectY.push_back(areaHeight - valuePointerPixel - 4);
        }//if

        keyPair.second->drawnStartX = timePointerPixel - 4;
        keyPair.second->drawnStartY = areaHeight - valuePointerPixel - 4;

        context->reset_clip();
        context->rectangle(timePointerPixel - 4, areaHeight - valuePointerPixel - 4, 9, 9);
        context->clip();
        context->paint();

        //And tangent points, if any
        if (graphState.keyframeSelectionState.GetNumSelected() == 1) {
            std::shared_ptr<Keyframe> firstKeyframe = graphState.keyframeSelectionState.GetFirstKeyframe();

            //If we only have a single keyframe selected, and we're currently drawing it and it's bezier
            if ( ( (firstKeyframe == keyPair.second) || ((firstKeyframe == lastDrawnKey)) || 
                   ((nextKeyIter != curKeyframes->end()) && (nextKeyIter->second == firstKeyframe)) ) && 
                 ((keyPair.second->curveType == CurveType::Bezier) || (lastCurveType == CurveType::Bezier) || (nextKeyIter->second->curveType == CurveType::Bezier)) ) {
                bool shouldDrawOutTangent = false;
                bool shouldDrawInTangent = false;

                if ( ( (firstKeyframe == keyPair.second) || ((nextKeyIter != curKeyframes->end()) && (nextKeyIter->second == firstKeyframe)) ) && 
                       (keyPair.second->curveType == CurveType::Bezier) ) {
                    shouldDrawOutTangent = true;
                    shouldDrawInTangent = true;
                }//if

                if ((lastCurveType == CurveType::Bezier) || (firstKeyframe == keyPair.second)) {
                    shouldDrawInTangent = true;
                    shouldDrawOutTangent = true;
                }//if                

                if (keyPair.second->curveType != CurveType::Bezier) {
                    shouldDrawInTangent = false;
                    shouldDrawOutTangent = false;
                }//if

//std::cout << "key is selected and bezier: " << shouldDrawInTangent << " - " << shouldDrawOutTangent << std::endl;
//std::cout << (firstKeyframe == keyPair.second) << " - " << ((nextKeyIter != curKeyframes->end()) && (nextKeyIter->second == firstKeyframe))
//            << " - " << (keyPair.second->curveType == CurveType::Bezier) << "(" << keyPair.second->curveType << ")" << std::endl;

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

    if (selectedRectX.empty() == false) { // != std::numeric_limits<int>::min()) {
        for (unsigned int index = 0; index < selectedRectX.size(); ++index) {
            context->set_source_rgba(0.0, 0.8, 0.0, 0.7);

            context->reset_clip();
            context->rectangle(selectedRectX[index], selectedRectY[index], 9, 9);
            context->clip();
            context->paint();
        }//for
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

bool FMidiAutomationMainWindow::updateGraph(const Cairo::RefPtr<Cairo::Context> &context)
{
//    std::cout << std::endl << std::endl << "updateGraph" << std::endl;

    doUIQueuedThreadStuff();

    if (false == graphDrawingArea->get_realized()) {
    	return false;
    }//if

    Glib::RefPtr<Gdk::Window> drawingAreaWindow = graphDrawingArea->get_window();
    //Cairo::RefPtr<Cairo::Context> context = drawingAreaWindow->create_cairo_context();
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
    if (graphState->zeroithTickPixel != std::numeric_limits<int>::max()) {
        context->reset_clip();
        context->rectangle(0, 60, graphState->zeroithTickPixel, drawingAreaHeight - 60);
        context->clip();

//        std::cout << "DARKEN 1: " << graphState->zeroithTickPixel << std::endl;
        context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
        context->paint();
    }//if

    graphState->roundedHorizontalValues.clear();
    graphState->roundedHorizontalValues.reserve(graphState->horizontalPixelValues.size());
    for (double val : graphState->horizontalPixelValues) {
        if (val >=0) {
            graphState->roundedHorizontalValues.push_back(val+0.5);
        } else {
            graphState->roundedHorizontalValues.push_back(val-0.5);
        }//if
    }//foreach

    if (graphState->displayMode == DisplayMode::Curve) {
        //int zzfirst = *(graphState->roundedHorizontalValues.begin());
        //int zzsecond = *(graphState->roundedHorizontalValues.rbegin());
        //std::cout << "HERE!!!! " << graphState->roundedHorizontalValues.size() << " - " << zzfirst << " - " << zzsecond << std::endl;
        //std::cout << graphState->maxValue << " - " << graphState->minValue << std::endl;
        std::shared_ptr<SequencerEntryImpl> impl = graphState->entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->getOwningEntry()->getImpl();
        int minValue = impl->minValue;
        int maxValue = impl->maxValue;

        //std::vector<int>::reverse_iterator maxIter = std::upper_bound(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), maxValue);
        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> maxIterPair = std::equal_range(graphState->roundedHorizontalValues.rbegin(), graphState->roundedHorizontalValues.rend(), maxValue);

        if (maxIterPair.first != graphState->roundedHorizontalValues.rend()) {
            //int rangeDist = std::distance(maxIterPair.first, maxIterPair.second) / 2.0;
            std::vector<int>::reverse_iterator maxIter = maxIterPair.second;
////            std::advance(maxIter, rangeDist);

            unsigned int dist = (drawingAreaHeight-60) - std::distance(graphState->roundedHorizontalValues.rbegin(), maxIter);

            context->reset_clip();
            context->rectangle(60, 60, drawingAreaWidth-60, dist+1);
            context->clip();

//            std::cout << "DARKEN 2: " << drawingAreaWidth-60 << " - " << dist+1 << std::endl;
            context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
            context->paint();
        }//if

        //std::vector<int>::reverse_iterator minIter = std::upper_bound(roundedHorizontalValues.rbegin(), roundedHorizontalValues.rend(), minValue);
        std::pair<std::vector<int>::reverse_iterator, std::vector<int>::reverse_iterator> minIterPair = std::equal_range(graphState->roundedHorizontalValues.rbegin(), graphState->roundedHorizontalValues.rend(), minValue);
        if (minIterPair.first != graphState->roundedHorizontalValues.rend()) {
            //int rangeDist = std::distance(minIterPair.first, minIterPair.second) / 2.0;
            std::vector<int>::reverse_iterator minIter = minIterPair.first;

            unsigned int dist = std::distance(graphState->roundedHorizontalValues.rbegin(), minIter);

            context->reset_clip();
            context->rectangle(60, drawingAreaHeight-dist, drawingAreaWidth-60, dist);
            context->clip();

//            std::cout << "DARKEN 3: " << drawingAreaHeight-dist << " - " << drawingAreaWidth-60 << " - " << dist << std::endl;
            context->set_source_rgba(0.0, 0.0, 0.0, 0.3);
            context->paint();
        }//if
    }//if

    Globals &globals = Globals::Instance();

    drawTopBar(context, *graphState, drawingAreaWidth, drawingAreaHeight);
    drawTempoBar(context, *graphState, globals.projectData, drawingAreaWidth, drawingAreaHeight, graphState->verticalPixelTickValues, graphState->ticksPerPixel);
    drawLeftMarker(context, *graphState, drawingAreaWidth, drawingAreaHeight);
    drawRightMarker(context, *graphState, drawingAreaWidth, drawingAreaHeight);
    drawCurrentTimePointer(context, *graphState, drawingAreaWidth, drawingAreaHeight);

    if (graphState->displayMode == DisplayMode::Sequencer) {
        sequencer->drawEntryBoxes(graphDrawingArea, context, drawingAreaWidth, drawingAreaHeight, graphState->verticalPixelTickValues);
    }//if

    if (graphState->displayMode == DisplayMode::Curve) {
        graphState->entryBlockSelectionState.GetFirstEntryBlock()->renderCurves(context, *graphState, drawingAreaWidth, drawingAreaHeight);
        drawLeftBar(context, *graphState, drawingAreaWidth, drawingAreaHeight);
    }//if

    if (graphState->doingRubberBanding == true) {
        drawRubberBand(context, *graphState, drawingAreaWidth, drawingAreaHeight, mousePressDownX, mousePressDownY, graphState->curMousePosX, graphState->curMousePosY);
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
    
    updateTempoBox(*graphState, globals.projectData, bpmEntry, beatsPerBarEntry, barSubdivisionsEntry);

    return true;
}//updateGraph

GraphState::GraphState()
{
    doInit();
}//constructor

void GraphState::doInit()
{
    lastOffsetX = std::numeric_limits<int>::max();
    offsetX = 0;
    offsetY = 0;
    barsSubdivisionAmount = 1;
    ticksPerPixel = 50;
    inMotion = false;
    zeroithTickPixel = std::numeric_limits<int>::max();
    curPointerTick = 0;
    curPointerTickXPixel = 0;
    selectedEntity = SelectedEntity::Nobody;
    leftMarkerTick = -1;
    rightMarkerTick = -1;
    leftMarkerTickXPixel = -1;
    rightMarkerTickXPixel = -1;
    displayMode = DisplayMode::Sequencer;
    selectedEntity = SelectedEntity::Nobody;
    curMousePosX = -100;
    curMousePosY = -100;
    doingRubberBanding = false;
    insertMode = InsertMode::Merge;
    displayMode = DisplayMode::Sequencer;
}//doInit

GraphState::~GraphState()
{
    //Nothing
}//destructor

/*
std::shared_ptr<SequencerEntryBlock> GraphState::getFirstCurrentlySelectedEntryBlock()
{
    if (entryBlockSelectionState.HasSelected() == false) {
        return std::shared_ptr<SequencerEntryBlock>();
    } else {
        return entryBlockSelectionState.GetFirstEntryBlock();
    }//if
}//getCurrentlySelectedEntryBlock
*/

void GraphState::refreshHorizontalLines(unsigned int areaWidth, unsigned int areaHeight)
{
    if ((displayMode != DisplayMode::Curve) || (0 == areaWidth)) {
        return;
    }//if

    if (std::numeric_limits<double>::max() == valuesPerPixel) {
        const std::shared_ptr<SequencerEntryImpl> entryImpl = entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->getOwningEntry()->getImpl();
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

//std::cout << "y: " << areaHeight-60-y << ":  " << roundedValue << "    " << areaHeight-60 << std::endl;

        if (((y - lastYTick) > minPixelDist) && (roundedValue != lastLineValue)) {
            lastLineValue = roundedValue;
            lastYTick = y;

            horizontalLines.push_back(std::make_pair(areaHeight-60-y-1, LineType::ValueLine));

            std::ostringstream tmpSS;
            tmpSS << roundedValue;
            valueLineText.push_back(std::make_pair(areaHeight-60-y-1, tmpSS.str()));
        }//if
    }//for

//    std::cout << "minPixelDist: " << minPixelDist << std::endl;
}//refreshHorizontalLines

void GraphState::refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight)
{
    if (0 == areaWidth) {
        return;
    }//if

//std::cout << "refreshVerticalLines entry" << std::endl;

    //Ugly kluge to handle the case where if we've already reached the half-way zeroith point and keep scrolling over too far, bad things happen
    if (((areaWidth-2) * ticksPerPixel + offsetX * ticksPerPixel) < 0) {
        if (lastOffsetX == std::numeric_limits<int>::max()) {
            lastOffsetX = offsetX;
        }//if

        offsetX = lastOffsetX;

//        std::cout << "refreshVerticalLines exit 1" << std::endl;
        return;
    } else {
        lastOffsetX = std::numeric_limits<int>::max();
    }//if

    verticalLines.clear();
    upperLineText.clear();

    int tickCountGroupSize = determineTickCountGroupSize(ticksPerPixel);

    int zeroithTickCount = 0;
    if ((displayMode == DisplayMode::Curve) && (entryBlockSelectionState.GetFirstEntryBlock() != nullptr)) {
        zeroithTickCount = entryBlockSelectionState.GetFirstEntryBlock()->getBaseEntryBlock()->getStartTick();
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

                verticalLines.push_back(std::make_pair(x, LineType::SecondLine));

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

                verticalLines.push_back(std::make_pair(x, LineType::SecondLine));

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
        if ((verticalLines.empty() == false) && (verticalLines[0].first == 0) && (verticalLines[0].second == LineType::SecondLine)) {
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

//    std::cout << "zeroithTickPixel: " << zeroithTickPixel << std::endl;
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
        //int fullWindowTicks = drawingAreaWidth / abs(graphState->ticksPerPixel);
        //int halfWindowTicks = fullWindowTicks / 2;
        int baseOffset = abs(ticksPerPixel) * tick;

        offsetX = baseOffset - (drawingAreaWidth / 2); // - halfWindowTicks;
    }//if
}//setOffsetCenteredOnTick


