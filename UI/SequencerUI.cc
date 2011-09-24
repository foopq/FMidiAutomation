/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "SequencerUI.h"
#include "SequencerEntryUI.h"
#include "Data/SequencerEntry.h"
#include "Animation.h"
#include "jack.h"
#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
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


SequencerUI::SequencerUI(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_, FMidiAutomationMainWindow *mainWindow_)
{
    std::cout << "Sequencer UI constructor" << std::endl;

    doInit(entryGlade_, parentWidget_, mainWindow_);
}//constructor

void SequencerUI::doInit(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_, FMidiAutomationMainWindow *mainWindow_)
{
    mainWindow = mainWindow_;
    entryGlade = entryGlade_;
    parentWidget = parentWidget_;

    tmpLabel.set_text("");
    tmpLabel.show();

    selectedEntry = nullptr;

    parentWidget->children().clear();
    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//doInit

void SequencerUI::updateEntryFocus(unsigned int y)
{
    std::cout << "updateEntryFocus: " << y << std::endl;

    for (auto entryIter : entries) {
        auto regionPair = entryIter.first->getUIBounds();

        if ((regionPair.first <= y) && (y <= regionPair.second)) {
            entryIter.first->setFocus();

            auto regionPair = entryIter.first->getUIBounds();
            std::cout << "** set focus for " << entryIter.first->getTitle() << " - " << regionPair.first << " | " << regionPair.second << std::endl;
            break;
        }//if
    }//foreach
}//updateEntryFocus

void SequencerUI::adjustFillerHeight()
{
    int totalHeight = 0;
    for (auto mapIter : entries) {
        if (true == mapIter.first->IsFullBox()) {
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

void SequencerUI::adjustEntryIndices()
{
    int index = 0;
    for (Glib::List_Iterator<Gtk::Box_Helpers::Child> entryIter = parentWidget->children().begin(); entryIter != parentWidget->children().end(); ++entryIter, ++index) {
        Gtk::Widget *curWidget = entryIter->get_widget();

        for (auto mapIter : entries) {
            Gtk::Widget *entryHookWidget = mapIter.first->getHookWidget();

            if (entryHookWidget == curWidget) {
                mapIter.first->setIndex(index);
                mapIter.second = index;
            }//if
        }//for
    }//for
}//adjustEntryIndices

void SequencerUI::notifyOnScroll(double pos)
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

    mainWindow->queue_draw();
}//notifyOnScroll

unsigned int SequencerUI::getNumEntries() const
{
    return entries.size();
}//getNumEntries

void SequencerUI::deepCloneEntries(std::shared_ptr<SequencerUI> other)
{
    entries.clear();
    
    for (auto entryIter : other->entries) {
        entries[entryIter.first->deepClone(entryGlade)] = entryIter.second;
    }//for
}//deepCloneEntries

void SequencerUI::addEntry(int index, std::shared_ptr<SequencerEntryUI> entry)
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

std::shared_ptr<SequencerEntryUI> SequencerUI::addEntry(int index, bool useDefaults, std::shared_ptr<SequencerEntry> origEntry)
{
    std::shared_ptr<SequencerEntryUI> newEntry(new SequencerEntryUI(entryGlade, entries.size()+1, origEntry, shared_from_this()));

    if (false == useDefaults) {
        editSequencerEntryProperties(newEntry, false);
    }//if

    addEntry(index, newEntry);
    return newEntry;
}//addEntry

void SequencerUI::deleteEntry(std::shared_ptr<SequencerEntryUI> entry)
{
    assert(entries.find(entry) != entries.end());

    parentWidget->children().remove(*entry->getHookWidget());
    entries.erase(entries.find(entry));
    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//deleteEntry

unsigned int SequencerUI::getEntryIndex(std::shared_ptr<SequencerEntryUI> entry)
{
    return entry->getIndex();
}//getEntryIndex

std::shared_ptr<SequencerEntryUI> SequencerUI::getSelectedEntry()
{
    for (auto mapIter : entries) {
        if (mapIter.first == selectedEntry) {
            return mapIter.first;
        }//if
    }//for

    return std::shared_ptr<SequencerEntryUI>();
}//getSelectedEntry

fmaipair<decltype(SequencerUI::entries.begin()), decltype(SequencerUI::entries.end())> SequencerUI::getEntryPair()
{
    return fmai_make_pair(entries.begin(), entries.end());
}//getEntryPair

void SequencerUI::doSwapEntryBox(Gtk::Viewport *current, Gtk::Viewport *next)
{
    Glib::List_Iterator<Gtk::Box_Helpers::Child> foundWidget = parentWidget->children().find(*current);

    parentWidget->children().insert(foundWidget, Gtk::Box_Helpers::Element(*next));    

    foundWidget = parentWidget->children().find(*current);
    parentWidget->children().erase(foundWidget);

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//doSwapEntryBox

void SequencerUI::notifySelected(std::shared_ptr<SequencerEntryUI> selectedEntry_)
{
    if (selectedEntry == selectedEntry_) {
        return;
    }//if

    if (selectedEntry != nullptr) {
        selectedEntry->deselect();
    }//if

    selectedEntry = selectedEntry_;

    if (selectedEntry != nullptr) {
        selectedEntry->select();
    }//if

    mainWindow->unsetAllCurveFrames();
}//notifySelected

std::shared_ptr<SequencerEntryBlockUI> SequencerUI::getSelectedEntryBlock() const
{
    return selectedEntryBlock;
}//getSelectedEntryBlock

std::shared_ptr<SequencerEntryBlockUI> SequencerUI::getSelectedEntryBlock(int x, int y, bool setSelection) //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
{
//    std::cout << "getSelectedEntryBlock: " << x << " - " << y << "    " << setSelection << std::endl;

//std::cout << "getSelectedEntryBlock: " << selectionInfos.size() << std::endl;

    //FIXME: What was this for?
/*    
    if (x < 0) {
        selectedEntryBlock = (*entries.begin()).first->getEntryBlock(0);

if (selectedEntryBlock == nullptr) {
    std::cout << "clearSelectedEntryBlock3" << std::endl;
}//if
        return selectedEntryBlock;
    }//if
*/

    for (SequencerEntryBlockSelectionInfo selectionInfo : selectionInfos) {

//        std::cout << "getSelectedEntryBlock entry: " << selectionInfo.entryBlock.get() << std::endl;

//        std::cout << "drawnArea: " << selectionInfo.drawnArea.get_x() << " - " << selectionInfo.drawnArea.get_y() << " - " << selectionInfo.drawnArea.get_width() << " - " << selectionInfo.drawnArea.get_height() << std::endl;

        if ( ((selectionInfo.drawnArea.get_x() <= x) && ((selectionInfo.drawnArea.get_x() + selectionInfo.drawnArea.get_width()) >= x)) &&
             ((selectionInfo.drawnArea.get_y() <= y) && ((selectionInfo.drawnArea.get_y() + selectionInfo.drawnArea.get_height()) >= y)) ) {
            if (true == setSelection) {
                selectedEntryBlock = selectionInfo.entryBlock;
                selectionInfo.entry.lock()->select();

//if (selectedEntryBlock == nullptr) {
//    std::cout << "clearSelectedEntryBlock2" << std::endl;
//}//if
            }//if
            return selectedEntryBlock;
        }//if
    }//foreach

    return std::shared_ptr<SequencerEntryBlockUI>();
}//getSelectedEntryBlock

void SequencerUI::updateSelectedEntryBlocksInRange(EntryBlockSelectionState &entryBlockSelectionState,
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

    for (SequencerEntryBlockSelectionInfo selectionInfo : selectionInfos) {
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

void SequencerUI::clearSelectedEntryBlock()
{
    std::cout << "clearSelectedEntryBlock" << std::endl;

    selectedEntryBlock.reset();
}//clearSelectedEntryBlock

void SequencerUI::editSequencerEntryProperties(std::shared_ptr<SequencerEntryUI> entry, bool createUpdatePoint)
{
    mainWindow->editSequencerEntryProperties(entry->getBaseEntry(), createUpdatePoint);
}//editSequencerEntryProperties

/*
void SequencerUI::cloneEntryMap()
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

    selectedEntry = nullptr;

 std::cout << "cEM 5" << std::endl;   
}//cloneEntryMap
*/

void SequencerUI::setEntryMap(std::map<std::shared_ptr<SequencerEntryUI>, int > entryMap)
{
////!!!! Unset selected entry block
////!!!! Switch select selected entry

    std::map<int, std::shared_ptr<SequencerEntryUI> > entriesRev;

    for (auto mapIter : entries) {
        entriesRev[mapIter.second] = mapIter.first;
        //parentWidget->children().remove(*mapIter->first->getHookWidget());
    }//for

    entries = entryMap;

    parentWidget->children().clear();

    for (auto mapIter : entriesRev) {
        Gtk::Widget *entryHookWidget = mapIter.second->getHookWidget();
        parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));
    }//for

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);

    selectedEntry = nullptr;
}//setEntryMap

void SequencerUI::doSave(boost::archive::xml_oarchive &outputArchive)
{
    int SequencerUIVersion = 1;
    outputArchive & BOOST_SERIALIZATION_NVP(SequencerUIVersion);

    int numEntries = entries.size();
    outputArchive & BOOST_SERIALIZATION_NVP(numEntries);

    for (auto entryIter : entries) {
        int index = entryIter.second;
        outputArchive & BOOST_SERIALIZATION_NVP(index);

        std::shared_ptr<SequencerEntry> baseEntry = entryIter.first->getBaseEntry();
        assert(baseEntry != nullptr);
        outputArchive & BOOST_SERIALIZATION_NVP(baseEntry);

        entryIter.first->doSave(outputArchive);
    }//for
}//doSave

void SequencerUI::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    int SequencerUIVersion = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(SequencerUIVersion);

    int numEntries = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(numEntries);

    entries.clear();
    parentWidget->children().clear();

    for (int entry = 0; entry < numEntries; ++entry) {
        int index = -1;
        inputArchive & BOOST_SERIALIZATION_NVP(index);

        std::shared_ptr<SequencerEntry> baseEntry;
        inputArchive & BOOST_SERIALIZATION_NVP(baseEntry);
        assert(baseEntry != nullptr);

        std::shared_ptr<SequencerEntryUI> entryUI(new SequencerEntryUI(entryGlade, index, baseEntry, shared_from_this()));
        entryUI->doLoad(inputArchive);
        entries[entryUI] = index;
    }//for

    std::map<int, std::shared_ptr<SequencerEntryUI> > entriesRev;

    for (auto mapIter : entries) {
        entriesRev[mapIter.second] = mapIter.first;
    }//for

    for (auto mapIter : entriesRev) {
        Gtk::Widget *entryHookWidget = mapIter.second->getHookWidget();
        parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));
    }//for

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//doLoad

/*
std::map<std::shared_ptr<SequencerEntry>, int > SequencerUI::getEntryMap()
{
    return entries;
}//getEntryMap
*/

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

///////////////////////////////////////////////////////////////////////////////////UI
// Rendering code

void SequencerUI::drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues)
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

    //Globals &globals = Globals::Instance();

    for (auto mapIter : entries) {
        Gtk::Widget *entryHookWidget = mapIter.first->getHookWidget();

        Gdk::Rectangle entryRect;
        entryHookWidget->get_window()->get_frame_extents(entryRect);

        int x;
        int y;
        int width;
        int height;
        int depth;
        entryHookWidget->get_window()->get_geometry(x, y, width, height, depth);

        entryHookWidget->get_window()->get_origin(x1, y1);
        mapIter.second = y1;
        int absEntryStartY = mapIter.second + 1;

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
            mapIter.first->setUIBounds(relativeStartY, relativeStartY + relativeEndY - 1);

            mapIter.first->drawEntryBoxes(context, verticalPixelTickValues, relativeStartY, relativeStartY + relativeEndY - 1, selectionInfos, 
                                            mainWindow->getGraphState().entryBlockSelectionState);
            
            context->reset_clip();
            context->rectangle(0, relativeStartY, 100, relativeEndY);
            context->clip();

            if ((mapIter.first->getIndex() % 2) == 0) {
                context->set_source_rgba(1.0, 0.0, 1.0, 0.3);
            } else {
                context->set_source_rgba(0.0, 1.0, 1.0, 0.3);
            }//if
            context->paint();
        } else {
            mapIter.first->setUIBounds(std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max());
        }//if

//std::cout << "top: " << mapIter->second << " --- x: " << x << "   y: " << y << "    width: " << width << "   height: " << height << "   depth: " << depth << std::endl;

    }//foreach

//    std::cout << std::endl;
}//drawEntryBoxes



