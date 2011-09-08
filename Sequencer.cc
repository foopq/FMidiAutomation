/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "Sequencer.h"
#include "SequencerEntry.h"
#include "Animation.h"
#include "jack.h"
#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "SerializationHelper.h"
#include "Globals.h"
#include "GraphState.h"
#include "FMidiAutomationMainWindow.h"

static const unsigned int entryWindowHeight = 138 + 6; //size plus padding
static const unsigned int smallEntryWindowHeight = 46 + 4; //size plus padding


Sequencer::Sequencer(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_, FMidiAutomationMainWindow *mainWindow_)
{
    std::cout << "Sequencer constructor" << std::endl;

    doInit(entryGlade_, parentWidget_, mainWindow_);
}//constructor

void Sequencer::doInit(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_, FMidiAutomationMainWindow *mainWindow_)
{
    mainWindow = mainWindow_;
    entryGlade = entryGlade_;
    parentWidget = parentWidget_;

    tmpLabel.set_text("");
    tmpLabel.show();

    selectedEntry = NULL;

    parentWidget->children().clear();
    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//doInit

void Sequencer::updateEntryFocus(unsigned int y)
{
    std::cout << "updateEntryFocus: " << y << std::endl;

    BOOST_FOREACH (auto entryIter, entries) {
        auto regionPair = entryIter.first->getUIBounds();

        if ((regionPair.first <= y) && (y <= regionPair.second)) {
            entryIter.first->setFocus();

            auto regionPair = entryIter.first->getUIBounds();
            std::cout << "** set focus for " << entryIter.first->getTitle() << " - " << regionPair.first << " | " << regionPair.second << std::endl;
            break;
        }//if
    }//foreach
}//updateEntryFocus

void Sequencer::adjustFillerHeight()
{
    int totalHeight = 0;
    for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        if (true == mapIter->first->IsFullBox()) {
            totalHeight += entryWindowHeight;
        } else {
            totalHeight += smallEntryWindowHeight;
        }//if
    }//foreach

    int height = parentWidget->get_parent()->get_height();
    int labelHeight = height - totalHeight;
    labelHeight = std::max(labelHeight, -1);
    tmpLabel.set_size_request(-1, labelHeight);
}//adjustFillerHeight

void Sequencer::adjustEntryIndices()
{
    int index = 0;
    for (Glib::List_Iterator<Gtk::Box_Helpers::Child> entryIter = parentWidget->children().begin(); entryIter != parentWidget->children().end(); ++entryIter, ++index) {
        Gtk::Widget *curWidget = entryIter->get_widget();

        for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
            Gtk::Widget *entryHookWidget = mapIter->first->getHookWidget();

            if (entryHookWidget == curWidget) {
                mapIter->first->setIndex(index);
                mapIter->second = index;
            }//if
        }//for
    }//for
}//adjustEntryIndices

void Sequencer::notifyOnScroll(double pos)
{
    std::cout << "notifyOnScroll" << std::endl;

/*    
    std::cout << std::endl << "notifyOnScroll" << std::endl;
    for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        Gtk::Widget *entryHookWidget = mapIter->first->getHookWidget();

        int x = 0;
        int y = 0;
        entryHookWidget->get_window()->get_origin(x, y);

        mapIter->second = y;

//        std::cout << "entry: " << x << " - " << y << std::endl;
    }//foreach

    std::cout << std::endl;
*/

    queue_draw();
}//notifyOnScroll

unsigned int Sequencer::getNumEntries() const
{
    return entries.size();
}//getNumEntries

void Sequencer::addEntry(std::shared_ptr<SequencerEntry> entry, int index)
{
    if (index < 0) {
        index = entries.size();
    }//if

    Gtk::Widget *entryHookWidget = entry->getHookWidget();

    Glib::List_Iterator<Gtk::Box_Helpers::Child> entryIter = parentWidget->children().begin();
    for (int pos = 0; pos < index; ++pos) {
        entryIter++;
    }//for

    parentWidget->children().insert(entryIter, Gtk::Box_Helpers::Element(*entryHookWidget));
    entries[entry] = index;

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);

std::cout << "entries: " << entries.size() << std::endl;
}//addEntry

std::shared_ptr<SequencerEntry> Sequencer::addEntry(int index, bool useDefaults)
{
    std::shared_ptr<SequencerEntry> newEntry(new SequencerEntry(entryGlade, shared_from_this(), entries.size()+1));

    if (false == useDefaults) {
        editSequencerEntryProperties(newEntry, false);
    }//if

    addEntry(newEntry, index);
    return newEntry;
}//addEntry

void Sequencer::deleteEntry(std::shared_ptr<SequencerEntry> entry)
{
    assert(entries.find(entry) != entries.end());

    parentWidget->children().remove(*entry->getHookWidget());
    entries.erase(entries.find(entry));
    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//deleteEntry

unsigned int Sequencer::getEntryIndex(std::shared_ptr<SequencerEntry> entry)
{
    return entry->getIndex();
}//getEntryIndex

std::shared_ptr<SequencerEntry> Sequencer::getSelectedEntry()
{
    for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        if (mapIter->first.get() == selectedEntry) {
            return mapIter->first;
        }//if
    }//for

    return std::shared_ptr<SequencerEntry>();
}//getSelectedEntry

std::pair<std::map<std::shared_ptr<SequencerEntry>, int >::const_iterator, std::map<std::shared_ptr<SequencerEntry>, int >::const_iterator> Sequencer::getEntryPair() const
{
    return std::make_pair(entries.begin(), entries.end());
}//getEntryPair

void Sequencer::doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next)
{
    Glib::List_Iterator<Gtk::Box_Helpers::Child> foundWidget = parentWidget->children().find(*current);

    parentWidget->children().insert(foundWidget, Gtk::Box_Helpers::Element(*next));    

    foundWidget = parentWidget->children().find(*current);
    parentWidget->children().erase(foundWidget);

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//doSwapEntryBox

void Sequencer::notifySelected(SequencerEntry *selectedEntry_)
{
    if (selectedEntry == selectedEntry_) {
        return;
    }//if

    if (selectedEntry != NULL) {
        selectedEntry->deselect();
    }//if

    selectedEntry = selectedEntry_;

    if (selectedEntry != NULL) {
        selectedEntry->select();
    }//if

    mainWindow->unsetAllCurveFrames();
}//notifySelected

std::shared_ptr<SequencerEntryBlock> Sequencer::getSelectedEntryBlock() const
{
    return selectedEntryBlock;
}//getSelectedEntryBlock

std::shared_ptr<SequencerEntryBlock> Sequencer::getSelectedEntryBlock(int x, int y, bool setSelection) //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
{
//    std::cout << "getSelectedEntryBlock: " << x << " - " << y << "    " << setSelection << std::endl;

//std::cout << "getSelectedEntryBlock: " << selectionInfos.size() << std::endl;

    if (x < 0) {
        selectedEntryBlock = (*entries.begin()).first->getEntryBlock(0);

if (selectedEntryBlock == NULL) {
    std::cout << "clearSelectedEntryBlock3" << std::endl;
}//if
        return selectedEntryBlock;
    }//if

    BOOST_FOREACH (SequencerEntryBlockSelectionInfo selectionInfo, selectionInfos) {

//        std::cout << "getSelectedEntryBlock entry: " << selectionInfo.entryBlock.get() << std::endl;

//        std::cout << "drawnArea: " << selectionInfo.drawnArea.get_x() << " - " << selectionInfo.drawnArea.get_y() << " - " << selectionInfo.drawnArea.get_width() << " - " << selectionInfo.drawnArea.get_height() << std::endl;

        if ( ((selectionInfo.drawnArea.get_x() <= x) && ((selectionInfo.drawnArea.get_x() + selectionInfo.drawnArea.get_width()) >= x)) &&
             ((selectionInfo.drawnArea.get_y() <= y) && ((selectionInfo.drawnArea.get_y() + selectionInfo.drawnArea.get_height()) >= y)) ) {
            if (true == setSelection) {
                selectedEntryBlock = selectionInfo.entryBlock;
                selectionInfo.entry.lock()->select();

//if (selectedEntryBlock == NULL) {
//    std::cout << "clearSelectedEntryBlock2" << std::endl;
//}//if
            }//if
            return selectedEntryBlock;
        }//if
    }//foreach

    return std::shared_ptr<SequencerEntryBlock>();
}//getSelectedEntryBlock

void Sequencer::updateSelectedEntryBlocksInRange(EntryBlockSelectionState &entryBlockSelectionState,
                                                    gdouble mousePressDownX, gdouble mousePressDownY, gdouble mousePosX, gdouble mousePosY,
                                                    int areaWidth, int areaHeight)
{
    mousePressDownX = std::max<gdouble>(mousePressDownX, 0);
    mousePressDownX = std::min<gdouble>(mousePressDownX, areaWidth);

    mousePressDownY = std::max<gdouble>(mousePressDownY, 61);
    mousePressDownY = std::min<gdouble>(mousePressDownY, areaHeight);

    mousePosX = std::max<gdouble>(mousePosX, 0);
    mousePosX = std::min<gdouble>(mousePosX, areaWidth);

    mousePosY = std::max<gdouble>(mousePosY, 61);
    mousePosY = std::min<gdouble>(mousePosY, areaHeight);
    
    if (mousePosX < mousePressDownX) {
        std::swap(mousePosX, mousePressDownX);
    }//if

    if (mousePosY < mousePressDownY) {
        std::swap(mousePosY, mousePressDownY);
    }//if

    BOOST_FOREACH (SequencerEntryBlockSelectionInfo selectionInfo, selectionInfos) {
        if (selectionInfo.drawnArea.get_x() < mousePosX &&
            selectionInfo.drawnArea.get_x() + selectionInfo.drawnArea.get_width() > mousePressDownX &&
            selectionInfo.drawnArea.get_y() < mousePosY &&
            selectionInfo.drawnArea.get_y() + selectionInfo.drawnArea.get_height() > mousePressDownY) {

            if (entryBlockSelectionState.IsSelected(selectionInfo.entryBlock) == false) {
                entryBlockSelectionState.AddSelectedEntryBlock(selectionInfo.entryBlock);
                selectionInfo.entry.lock()->select();
            }//if
        } else {
            if (entryBlockSelectionState.IsOrigSelected(selectionInfo.entryBlock) == false) {
                if (entryBlockSelectionState.IsSelected(selectionInfo.entryBlock) == true) {
                    entryBlockSelectionState.RemoveSelectedEntryBlock(selectionInfo.entryBlock);
                }//if
            }//if
        }//if
    }//foreach
}//updateSelectedEntryBlocksInRange

void Sequencer::clearSelectedEntryBlock()
{
    std::cout << "clearSelectedEntryBlock" << std::endl;

    selectedEntryBlock.reset();
}//clearSelectedEntryBlock

void Sequencer::editSequencerEntryProperties(std::shared_ptr<SequencerEntry> entry, bool createUpdatePoint)
{
    mainWindow->editSequencerEntryProperties(entry, createUpdatePoint);
}//editSequencerEntryProperties

void Sequencer::doLoad(boost::archive::xml_iarchive &inputArchive)
{

    inputArchive & BOOST_SERIALIZATION_NVP(entries);
    inputArchive & BOOST_SERIALIZATION_NVP(selectedEntryBlock);
    inputArchive & BOOST_SERIALIZATION_NVP(selectionInfos);

    parentWidget->children().clear();

    Glib::List_Iterator<Gtk::Box_Helpers::Child> entryIter = parentWidget->children().end();
    int entryNum = 0;

    std::vector<std::shared_ptr<SequencerEntry>> orderedEntries;
    BOOST_FOREACH (auto entryPair, entries) {
        orderedEntries.push_back(entryPair.first);
    }//foreach

    std::sort(orderedEntries.begin(), orderedEntries.end(), 
                [](const std::shared_ptr<SequencerEntry> &a, const std::shared_ptr<SequencerEntry> &b)
                    {
                        return a->getIndex() < b->getIndex();
                    }
            );

    BOOST_FOREACH (auto entry, orderedEntries) {
        std::string entryTitle = entry->getTitle();
        entry->doInit(entryGlade, shared_from_this(), entryNum);
        entry->setTitle(entryTitle);
        Gtk::Widget *entryHookWidget = entry->getHookWidget();

//std::cout << "HERE: " << entryHookWidget << " - " << entryTitle << " - " << entry->getTitle() << " - index: " << entry->getIndex() << std::endl;

        parentWidget->children().insert(entryIter, Gtk::Box_Helpers::Element(*entryHookWidget));
        
//        entries[entryPair.first] = entryNum;
        entryIter = parentWidget->children().end();
        entryNum++;
    }//foreach

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    std::cout << "entries2: " << entries.size() <<  std::endl;

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);

    selectedEntry = NULL;
}//doLoad

void Sequencer::doSave(boost::archive::xml_oarchive &outputArchive)
{
    outputArchive & BOOST_SERIALIZATION_NVP(entries);
    outputArchive & BOOST_SERIALIZATION_NVP(selectedEntryBlock);
    outputArchive & BOOST_SERIALIZATION_NVP(selectionInfos);
}//doSave

void Sequencer::cloneEntryMap()
{
    std::map<std::shared_ptr<SequencerEntry>, int > entriesClone;
    std::map<int, std::shared_ptr<SequencerEntry> > entriesCloneRev;
    for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        std::shared_ptr<SequencerEntry> entryClone = mapIter->first->deepClone();
        entriesClone[entryClone] = mapIter->second;
        entriesCloneRev[mapIter->second] = entryClone;

        //parentWidget->children().remove(*mapIter->first->getHookWidget());
    }//for

    entries.swap(entriesClone);

    assert(entries.size() == entriesCloneRev.size());

    parentWidget->children().clear();

    for (std::map<int, std::shared_ptr<SequencerEntry> >::iterator mapIter = entriesCloneRev.begin(); mapIter != entriesCloneRev.end(); ++mapIter) {
        Gtk::Widget *entryHookWidget = mapIter->second->getHookWidget();
        parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));
    }//for

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);

    selectedEntry = NULL;

 std::cout << "cEM 5" << std::endl;   
}//cloneEntryMap

void Sequencer::setEntryMap(std::map<std::shared_ptr<SequencerEntry>, int > entryMap)
{
////!!!! Unset selected entry block
////!!!! Switch select selected entry

    std::map<int, std::shared_ptr<SequencerEntry> > entriesRev;

    for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        entriesRev[mapIter->second] = mapIter->first;
        //parentWidget->children().remove(*mapIter->first->getHookWidget());
    }//for

    entries = entryMap;

    parentWidget->children().clear();

    for (std::map<int, std::shared_ptr<SequencerEntry> >::iterator mapIter = entriesRev.begin(); mapIter != entriesRev.end(); ++mapIter) {
        Gtk::Widget *entryHookWidget = mapIter->second->getHookWidget();
        parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));
    }//for

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);

    selectedEntry = NULL;
}//setEntryMap

std::map<std::shared_ptr<SequencerEntry>, int > Sequencer::getEntryMap()
{
    return entries;
}//getEntryMap

/*
std::shared_ptr<VectorStreambuf> Sequencer::serializeEntryMap()
{
    std::shared_ptr<VectorStreambuf> outputStream(new VectorStreambuf);

    boost::archive::binary_oarchive outputArchive(*outputStream, boost::archive::no_codecvt | boost::archive::no_header);

    outputArchive & BOOST_SERIALIZATION_NVP(entries);

    return outputStream;
}//serializeEntryMap

void Sequencer::deserializeEntryMap(std::shared_ptr<VectorStreambuf> streambuf)
{
    boost::archive::binary_iarchive inputArchive(*streambuf, boost::archive::no_codecvt | boost::archive::no_header);

    entries.clear();
    inputArchive & BOOST_SERIALIZATION_NVP(entries);

    CommandManager::Instance().refreshActualPointerData();
    clearSelectedEntryBlock();

    ... properly clear selected entry and entry block
    ... do command for process midi data
    ... fill in refreshAPD for commands
}//deserializeEntryMap
*/

///////////////////////////////////////////////////////////////////////////////////
// Rendering code

void Sequencer::drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues)
{
    selectionInfos.clear();

    //std::cout << "selectionInfos cleared" << std::endl;

//std::cout << std::endl;    

    int x1 = 0;
    int y1 = 0;

//    parentWidget->get_window()->get_origin(x1, y1);
//std::cout << "parentWidget y: " << y1 << std::endl;

    int drawingAreaStartY;
    graphDrawingArea->get_window()->get_origin(x1, drawingAreaStartY);

    Globals &globals = Globals::Instance();

    for (std::map<std::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        Gtk::Widget *entryHookWidget = mapIter->first->getHookWidget();

        Gdk::Rectangle entryRect;
        entryHookWidget->get_window()->get_frame_extents(entryRect);

        int x;
        int y;
        int width;
        int height;
        int depth;
        entryHookWidget->get_window()->get_geometry(x, y, width, height, depth);

        entryHookWidget->get_window()->get_origin(x1, y1);
        mapIter->second = y1;
        int absEntryStartY = mapIter->second + 1;

        if (((absEntryStartY + height) >= (drawingAreaStartY + 60)) && (absEntryStartY < (drawingAreaStartY + (int)areaHeight))) {
//std::cout << "absEntryStartY: " << absEntryStartY << "    drawingAreaStartY: " << drawingAreaStartY << std::endl;            
            int relativeStartY = (absEntryStartY - drawingAreaStartY);
            int relativeEndY = height;

            if (relativeStartY < 61) {
                int diff = 62 - relativeStartY;
                relativeStartY = 62;
                relativeEndY -= diff;
            }//if

//std::cout << "relative start: " << relativeStartY << "  ---  rel end: " << relativeEndY << std::endl;

            //std::cout << "drawEntryBoxes graphState: " << globals.graphState.get() << std::endl;

            //std::cout << "ui bound for " << mapIter->first->getTitle() << ": " << relativeStartY << " - " << relativeStartY + relativeEndY - 1 << std::endl;
            mapIter->first->setUIBounds(relativeStartY, relativeStartY + relativeEndY - 1);

            mapIter->first->drawEntryBoxes(context, verticalPixelTickValues, relativeStartY, relativeStartY + relativeEndY - 1, selectionInfos, 
                                            globals.graphState->entryBlockSelectionState);
            
            context->reset_clip();
            context->rectangle(0, relativeStartY, 100, relativeEndY);
            context->clip();

            if ((mapIter->first->getIndex() % 2) == 0) {
                context->set_source_rgba(1.0, 0.0, 1.0, 0.3);
            } else {
                context->set_source_rgba(0.0, 1.0, 1.0, 0.3);
            }//if
            context->paint();
        } else {
            mapIter->first->setUIBounds(std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max());
        }//if

//std::cout << "top: " << mapIter->second << " --- x: " << x << "   y: " << y << "    width: " << width << "   height: " << height << "   depth: " << depth << std::endl;

    }//foreach

//    std::cout << std::endl;
}//drawEntryBoxes



