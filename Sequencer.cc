#include "Sequencer.h"
#include "FMidiAutomationMainWindow.h"
#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

static const unsigned int entryWindowHeight = 138 + 6; //size plus padding
static const unsigned int smallEntryWindowHeight = 44 + 6; //size plus padding

namespace
{

void setThemeColours(Gtk::Widget *widget)
{
    Globals &globals = Globals::Instance();

    Gdk::Color fgColour;
    Gdk::Color bgColour;
    Gdk::Color editBoxBgColour;
    Gdk::Color textColour;
    Gdk::Color darkTextColour;
    Gdk::Color black;

    black.set_rgb(0, 0, 0);

    if (true == globals.darkTheme) {
        fgColour.set_rgb(52429, 42429, 52429);
        bgColour.set_rgb(10000, 10000, 10000);
        editBoxBgColour.set_rgb(25000, 25000, 25000);
        textColour.set_rgb(55982, 55982, 55982);
        darkTextColour.set_rgb(45982, 45982, 45982);
    }//if

    Gtk::Viewport *viewport = dynamic_cast<Gtk::Viewport *>(widget);
    if (viewport != NULL) {
        viewport->modify_bg(Gtk::STATE_NORMAL, bgColour);
        viewport->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::Label *label = dynamic_cast<Gtk::Label *>(widget);
    if (label != NULL) {
        label->modify_fg(Gtk::STATE_NORMAL, darkTextColour);
    }//if

    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(widget);
    if (entry != NULL) {
        entry->modify_base(Gtk::STATE_NORMAL, bgColour);
        entry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
        entry->modify_bg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::Frame *frame = dynamic_cast<Gtk::Frame *>(widget);
    if (frame != NULL) {
        frame->modify_bg(Gtk::STATE_NORMAL, black);
    }//if

    Gtk::Button *button = dynamic_cast<Gtk::Button *>(widget);
    if (button != NULL) {
        button->modify_bg(Gtk::STATE_NORMAL, bgColour);
    }//if

    Gtk::Table *table = dynamic_cast<Gtk::Table *>(widget);
    if (table != NULL) {        
        table->modify_bg(Gtk::STATE_NORMAL, bgColour);
        table->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::EventBox *eventBox = dynamic_cast<Gtk::EventBox *>(widget);
    if (eventBox != NULL) {
        eventBox->modify_bg(Gtk::STATE_NORMAL, bgColour);
        eventBox->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::ComboBox *comboBox = dynamic_cast<Gtk::ComboBox *>(widget);
    if (comboBox != NULL) {
        comboBox->modify_bg(Gtk::STATE_NORMAL, bgColour);
        comboBox->modify_text(Gtk::STATE_NORMAL, bgColour);
        comboBox->modify_base(Gtk::STATE_NORMAL, bgColour);
        comboBox->modify_fg(Gtk::STATE_NORMAL, bgColour);

//        comboBox->get_column(0);

        /*
        Gtk::TreeModel::Children children = comboBox->get_model()->children();
        BOOST_FOREACH (Gtk::TreeRow row, children) {

        }//foreach
        */
    }//if

    /*
    Gtk::Alignment *alignment = dynamic_cast<Gtk::Alignment *>(widget);
    if (alignment != NULL) {
        alignment->modify_bg(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_fg(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_base(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_text(Gtk::STATE_NORMAL, bgColour);
    }//if
    */

    Gtk::CellRendererText *cellRendererText = dynamic_cast<Gtk::CellRendererText *>(widget);
    if (cellRendererText != NULL) {
        std::cout << "crt" << std::endl;
    }//if

    Gtk::Container *container = dynamic_cast<Gtk::Container *>(widget);
    if (container != NULL) {
        Glib::ListHandle<Gtk::Widget *> children = container->get_children();
        BOOST_FOREACH (Gtk::Widget *childWidget, children) {
            ::setThemeColours(childWidget);
        }//forach
    }//if
}//setThemeColours

}//anonymous namespace

SequencerEntryBlock::SequencerEntryBlock(int startTick_, boost::shared_ptr<SequencerEntryBlock> instanceOf_)
{
    startTick_ = std::max(startTick_, 0);

    startTick = startTick_;
    instanceOf = instanceOf_;
    duration = 200;
}//constructor

void SequencerEntryBlock::moveBlock(int startTick_)
{
    startTick_ = std::max(startTick_, 0);
    startTick = startTick_;
}//moveBlock

void SequencerEntryBlock::setDuration(int duration_)
{
    duration = duration_;
}//setDuration

void SequencerEntryBlock::setTitle(const Glib::ustring &title_)
{
    title = title_;
}//setTitle

int SequencerEntryBlock::getStartTick() const
{
    return startTick;
}//getStartTick

int SequencerEntryBlock::getDuration() const
{
    return duration;
}//getDuration

Glib::ustring SequencerEntryBlock::getTitle() const
{
    return title;
}//getTitle

boost::shared_ptr<SequencerEntryBlock> SequencerEntryBlock::getInstanceOf() const
{
    return instanceOf;
}//getInstanceOf

SequencerEntry::SequencerEntry(const Glib::ustring &entryGlade, Sequencer *sequencer_, unsigned int entryNum)
{
    sequencer = sequencer_;

    uiXml = Gtk::Builder::create_from_string(entryGlade);
    uiXml->get_widget("entryViewport", mainWindow);
    uiXml->get_widget("smallEntryViewport", smallWindow);

    uiXml->get_widget("largeEntryFrame", largeFrame);
    uiXml->get_widget("smallEntryFrame", smallFrame);

//std::cout << "largeFrame: " << largeFrame << "   --    " << this << std::endl;
//largeFrame->get_label();

    Gtk::Button *switchButton;
    uiXml->get_widget("toggleButton", switchButton);
    switchButton->signal_clicked().connect ( sigc::mem_fun(*this, &SequencerEntry::handleSwitchPressed) );

    uiXml->get_widget("toggleButton1", switchButton);
    switchButton->signal_clicked().connect ( sigc::mem_fun(*this, &SequencerEntry::handleSwitchPressed) );

    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text("Automation " + boost::lexical_cast<std::string>(entryNum));
    entryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &SequencerEntry::handleKeyEntryOnLargeTitleEntryBox));
    uiXml->get_widget("titleEntry1", entryBox);
    entryBox->set_text("Automation " + boost::lexical_cast<std::string>(entryNum));
    entryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &SequencerEntry::handleKeyEntryOnSmallTitleEntryBox));

    Gtk::EventBox *eventBox;
    uiXml->get_widget("eventbox1", eventBox);
    eventBox->signal_button_press_event().connect ( sigc::mem_fun(*this, &SequencerEntry::mouseButtonPressed) );
    uiXml->get_widget("eventbox2", eventBox);
    eventBox->signal_button_press_event().connect ( sigc::mem_fun(*this, &SequencerEntry::mouseButtonPressed) );


    mainWindow->get_parent()->remove(*mainWindow);
    smallWindow->get_parent()->remove(*smallWindow);

    isFullBox = true;
    inHandler = false;

    curIndex = -1;

    deselect();

    setThemeColours();
}//constructor

SequencerEntry::~SequencerEntry()
{
    std::cout << "AAAAAAAAAAAAAAAAA" << std::endl;
}//destructor

void SequencerEntry::setThemeColours()
{
return;

    ::setThemeColours(mainWindow);
    ::setThemeColours(smallWindow);

    Globals &globals = Globals::Instance();

    Gdk::Color fgColour;
    Gdk::Color bgColour;
    Gdk::Color editBoxBgColour;
    Gdk::Color textColour;
    Gdk::Color darkTextColour;
    Gdk::Color black;

    black.set_rgb(0, 0, 0);

    Gdk::Color red;
    red.set_rgb(1, 0, 0);

    if (true == globals.darkTheme) {
        fgColour.set_rgb(52429, 42429, 52429);
        bgColour.set_rgb(10000, 10000, 10000);
        editBoxBgColour.set_rgb(25000, 25000, 25000);
        textColour.set_rgb(55982, 55982, 55982);
        darkTextColour.set_rgb(45982, 45982, 45982);
    }//if

    Glib::RefPtr<Glib::Object> obj = uiXml->get_object("cellrenderertext1");
    Glib::RefPtr<Gtk::CellRendererText> cellRendererText = Glib::RefPtr<Gtk::CellRendererText>::cast_dynamic(obj);

    if (cellRendererText != NULL) {
        #ifdef GLIBMM_PROPERTIES_ENABLED
//            cellRendererText->property_background_gdk() = bgColour;
            cellRendererText->property_foreground_gdk() = textColour;
//            cellRendererText->property_cell_background_set() = true;
            cellRendererText->property_cell_background_gdk() = bgColour;
        #else
            aaaaaa
            cellRendererText.set_property("background_gdk", bgColour);
        #endif
    }//if

    obj = uiXml->get_object("cellrenderertext2");
    cellRendererText = Glib::RefPtr<Gtk::CellRendererText>::cast_dynamic(obj);

    if (cellRendererText != NULL) {
        #ifdef GLIBMM_PROPERTIES_ENABLED
//            cellRendererText->property_background_gdk() = bgColour;
            cellRendererText->property_foreground_gdk() = textColour;
//            cellRendererText->property_cell_background_set() = true;
            cellRendererText->property_cell_background_gdk() = bgColour;
        #else
            aaaaaa
            cellRendererText.set_property("background_gdk", bgColour);
        #endif
    }//if
}//setThemeColours

bool SequencerEntry::handleKeyEntryOnLargeTitleEntryBox(GdkEventKey *event)
{
    mouseButtonPressed(NULL);
    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    Glib::ustring title = entryBox->get_text();

    uiXml->get_widget("titleEntry1", entryBox);
    entryBox->set_text(title);

    return true;
}//handleKeyEntryOnLargeTitleEntryBox

bool SequencerEntry::handleKeyEntryOnSmallTitleEntryBox(GdkEventKey *event)
{
    mouseButtonPressed(NULL);
    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry1", entryBox);
    Glib::ustring title = entryBox->get_text();

    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text(title);

    return true;
}//handleKeyEntryOnSmallTitleEntryBox

void SequencerEntry::handleSwitchPressed()
{
    mouseButtonPressed(NULL);
    isFullBox = !isFullBox;

    if (false == isFullBox) {
        if (smallWindow->get_parent() != NULL) {
            smallWindow->get_parent()->remove(*smallWindow);
        }//if
        sequencer->doSwapEntryBox(mainWindow, smallWindow);
    } else {
        if (mainWindow->get_parent() != NULL) {
            mainWindow->get_parent()->remove(*mainWindow);
        }//if
        sequencer->doSwapEntryBox(smallWindow, mainWindow);
    }//if

    Globals &globals = Globals::Instance();
    globals.graphDrawingArea->queue_draw();
}//handleSwitchPressed

bool SequencerEntry::IsFullBox() const
{
    return isFullBox;
}//IsFullBox

void SequencerEntry::setIndex(unsigned int index)
{
    Gtk::Label *label;
    uiXml->get_widget("indexLabel", label);
    label->set_text(boost::lexical_cast<std::string>(index+1));
    uiXml->get_widget("indexLabel1", label);
    label->set_text(boost::lexical_cast<std::string>(index+1));

    curIndex = index;
}//setIndex

Glib::ustring SequencerEntry::getTitle() const
{
    Gtk::Entry *label;
    uiXml->get_widget("titleEntry", label);
    return label->get_text();
}//getTitle

unsigned int SequencerEntry::getIndex()
{
    return curIndex;
}//getIndex

Gtk::Widget *SequencerEntry::getHookWidget()
{
    if (true == isFullBox) {
        return mainWindow;
    } else {
        return smallWindow;
    }//if
}//getHookWidget

bool SequencerEntry::mouseButtonPressed(GdkEventButton *event)
{
    Gdk::Color fgColour;
    Gdk::Color bgColour;

    fgColour.set_rgb(65535, 32768, 0);
    bgColour.set_rgb(10000, 10000, 10000);

    largeFrame->modify_bg(Gtk::STATE_NORMAL, fgColour);
    largeFrame->modify_fg(Gtk::STATE_NORMAL, fgColour);
    largeFrame->modify_base(Gtk::STATE_NORMAL, fgColour);


    smallFrame->modify_bg(Gtk::STATE_NORMAL, fgColour);
    smallFrame->modify_fg(Gtk::STATE_NORMAL, fgColour);
    smallFrame->modify_base(Gtk::STATE_NORMAL, fgColour);

    sequencer->notifySelected(this);

    return true;
}//mouseButtonPressed

void SequencerEntry::select()
{
    mouseButtonPressed(NULL);
}//select

void SequencerEntry::deselect()
{
    Gdk::Color fgColour;
    Gdk::Color bgColour;

    fgColour.set_rgb(52429, 42429, 52429);
    bgColour.set_rgb(10000, 10000, 10000);

//std::cout << "largeFrame2: " << largeFrame << "   ---   " << this << std::endl;
    largeFrame->get_label();

    largeFrame->modify_bg(Gtk::STATE_NORMAL, bgColour);
    largeFrame->modify_fg(Gtk::STATE_NORMAL, bgColour);
    largeFrame->modify_base(Gtk::STATE_NORMAL, bgColour);

    smallFrame->modify_bg(Gtk::STATE_NORMAL, bgColour);
    smallFrame->modify_fg(Gtk::STATE_NORMAL, bgColour);
    smallFrame->modify_base(Gtk::STATE_NORMAL, bgColour);
}//deselect

void SequencerEntry::addEntryBlock(int, boost::shared_ptr<SequencerEntryBlock> entryBlock)
{
    removeEntryBlock(entryBlock);
    entryBlocks[entryBlock->getStartTick()] = entryBlock;

    if (entryBlock->getTitle().empty() == true) {
        entryBlock->setTitle(getTitle() + Glib::ustring(" - ") + boost::lexical_cast<Glib::ustring>(entryBlocks.size()));

//        std::cout << "entryBlock title: " << entryBlock->getTitle() << std::endl;
    }//if

//    std::cout << "addEntryBlock: " << entryBlock->getStartTick() << std::endl;
}//addEntryBlock

void SequencerEntry::removeEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock)
{
    if (entryBlocks.find(entryBlock->getStartTick()) != entryBlocks.end()) {
        entryBlocks.erase(entryBlocks.find(entryBlock->getStartTick()));
    }//if
}//removeEntryBlock

Sequencer::Sequencer(const Glib::ustring &entryGlade_, Gtk::VBox *parentWidget_)
{
    entryGlade = entryGlade_;
    parentWidget = parentWidget_;

    tmpLabel.set_text("");
    tmpLabel.show();

    selectedEntry = NULL;

    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));
}//constructor

void Sequencer::adjustFillerHeight()
{
    int totalHeight = 0;
    for (std::map<boost::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
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

        for (std::map<boost::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
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
/*    
    std::cout << std::endl << "notifyOnScroll" << std::endl;
    for (std::map<boost::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        Gtk::Widget *entryHookWidget = mapIter->first->getHookWidget();

        int x = 0;
        int y = 0;
        entryHookWidget->get_window()->get_origin(x, y);

        mapIter->second = y;

//        std::cout << "entry: " << x << " - " << y << std::endl;
    }//foreach

    std::cout << std::endl;
*/

    Globals &globals = Globals::Instance();
    globals.graphDrawingArea->queue_draw();
}//notifyOnScroll

unsigned int Sequencer::getNumEntries() const
{
    return entries.size();
}//getNumEntries

void Sequencer::addEntry(boost::shared_ptr<SequencerEntry> entry, int index)
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
}//addEntry

boost::shared_ptr<SequencerEntry>Sequencer::addEntry(int index)
{
    boost::shared_ptr<SequencerEntry> newEntry(new SequencerEntry(entryGlade, this, entries.size()+1));
    addEntry(newEntry, index);
    return newEntry;
}//addEntry

void Sequencer::deleteEntry(boost::shared_ptr<SequencerEntry> entry)
{
    assert(entries.find(entry) != entries.end());

    parentWidget->children().remove(*entry->getHookWidget());
    entries.erase(entries.find(entry));
    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);
}//deleteEntry

unsigned int Sequencer::getEntryIndex(boost::shared_ptr<SequencerEntry> entry)
{
    return entry->getIndex();
}//getEntryIndex

boost::shared_ptr<SequencerEntry> Sequencer::getSelectedEntry()
{
    for (std::map<boost::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
        if (mapIter->first.get() == selectedEntry) {
            return mapIter->first;
        }//if
    }//for

    return boost::shared_ptr<SequencerEntry>();
}//getSelectedEntry

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
}//notifySelected

boost::shared_ptr<SequencerEntryBlock> Sequencer::getSelectedEntryBlock() const
{
    return selectedEntryBlock;
}//getSelectedEntryBlock

boost::shared_ptr<SequencerEntryBlock> Sequencer::getSelectedEntryBlock(int x, int y, bool setSelection) //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
{
//    std::cout << "getSelectedEntryBlock: " << x << " - " << y << "    " << setSelection << std::endl;

    BOOST_FOREACH (SequencerEntryBlockSelectionInfo selectionInfo, selectionInfos) {
//        std::cout << "drawnArea: " << selectionInfo.drawnArea.get_x() << " - " << selectionInfo.drawnArea.get_y() << " - " << selectionInfo.drawnArea.get_width() << " - " << selectionInfo.drawnArea.get_height() << std::endl;

        if ( ((selectionInfo.drawnArea.get_x() <= x) && ((selectionInfo.drawnArea.get_x() + selectionInfo.drawnArea.get_width()) >= x)) &&
             ((selectionInfo.drawnArea.get_y() <= y) && ((selectionInfo.drawnArea.get_y() + selectionInfo.drawnArea.get_height()) >= y)) ) {
            if (true == setSelection) {
                selectedEntryBlock = selectionInfo.entryBlock;
                selectionInfo.entry->select();
            }//if
            return selectedEntryBlock;
        }//if
    }//foreach

    return boost::shared_ptr<SequencerEntryBlock>();
}//getSelectedEntryBlock

void Sequencer::clearSelectedEntryBlock()
{
    selectedEntryBlock.reset();
}//clearSelectedEntryBlock

///////////////////////////////////////////////////////////////////////////////////
// Rendering code

void Sequencer::drawEntryBoxes(Gtk::DrawingArea *graphDrawingArea, Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight, std::vector<int> &verticalPixelTickValues)
{
    selectionInfos.clear();

//std::cout << std::endl;    
//std::cout << "drawEntryBoxes" << std::endl;

    int x1 = 0;
    int y1 = 0;

//    parentWidget->get_window()->get_origin(x1, y1);
//std::cout << "parentWidget y: " << y1 << std::endl;

    int drawingAreaStartY;
    graphDrawingArea->get_window()->get_origin(x1, drawingAreaStartY);

    for (std::map<boost::shared_ptr<SequencerEntry>, int >::iterator mapIter = entries.begin(); mapIter != entries.end(); ++mapIter) {
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

        if (((absEntryStartY + height) >= (drawingAreaStartY + 60)) && (absEntryStartY < (drawingAreaStartY + areaHeight))) {
//std::cout << "absEntryStartY: " << absEntryStartY << "    drawingAreaStartY: " << drawingAreaStartY << std::endl;            
            int relativeStartY = (absEntryStartY - drawingAreaStartY);
            int relativeEndY = height;

            if (relativeStartY < 61) {
                int diff = 62 - relativeStartY;
                relativeStartY = 62;
                relativeEndY -= diff;
            }//if

//std::cout << "relative start: " << relativeStartY << "  ---  rel end: " << relativeEndY << std::endl;

            mapIter->first->drawEntryBoxes(context, verticalPixelTickValues, relativeStartY, relativeStartY + relativeEndY - 1, selectionInfos, selectedEntryBlock);
            
            context->reset_clip();
            context->rectangle(0, relativeStartY, 100, relativeEndY);
            context->clip();

            if ((mapIter->first->getIndex() % 2) == 0) {
                context->set_source_rgba(1.0, 0.0, 1.0, 0.3);
            } else {
                context->set_source_rgba(0.0, 1.0, 1.0, 0.3);
            }//if
            context->paint();
            
        }//if

//std::cout << "top: " << mapIter->second << " --- x: " << x << "   y: " << y << "    width: " << width << "   height: " << height << "   depth: " << depth << std::endl;

    }//foreach

//    std::cout << std::endl;
}//drawEntryBoxes

void SequencerEntry::drawEntryBoxes(Cairo::RefPtr<Cairo::Context> context, std::vector<int> &verticalPixelTickValues, int relativeStartY, int relativeEndY, std::vector<SequencerEntryBlockSelectionInfo> &selectionInfos,
                                        boost::shared_ptr<SequencerEntryBlock> selectedEntryBlock)

{
    Globals &globals = Globals::Instance();

    for (std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator entryBlockIter = entryBlocks.begin(); entryBlockIter != entryBlocks.end(); ++entryBlockIter) {
        int startTick = entryBlockIter->second->getStartTick();
        int duration = entryBlockIter->second->getDuration();

        if ((startTick > verticalPixelTickValues[verticalPixelTickValues.size()-1]) || (startTick + duration < verticalPixelTickValues[0])) {
            continue;
        }//if

        std::map<int, boost::shared_ptr<SequencerEntryBlock> >::const_iterator nextEntryBlockIter = entryBlockIter;
        ++nextEntryBlockIter;

        int relativeStartXTick = startTick;
        relativeStartXTick = std::max(relativeStartXTick, verticalPixelTickValues[0]);

        int relativeEndXTick = startTick + duration;
        bool wasCutOff = false;
        if (nextEntryBlockIter != entryBlocks.end()) {
            if (nextEntryBlockIter->second->getStartTick() < relativeEndXTick) {
                relativeEndXTick = nextEntryBlockIter->second->getStartTick();
                wasCutOff = true;
            }//if
        }//if

        relativeEndXTick = std::min(relativeEndXTick, verticalPixelTickValues[verticalPixelTickValues.size()-1]);

        std::vector<int>::iterator bound = std::lower_bound(verticalPixelTickValues.begin(), verticalPixelTickValues.end(), relativeStartXTick);
        int relativeStartX = std::distance(verticalPixelTickValues.begin(), bound);

        bound = std::lower_bound(verticalPixelTickValues.begin(), verticalPixelTickValues.end(), relativeEndXTick);
        int relativeEndX = std::distance(verticalPixelTickValues.begin(), bound);

//        std::cout << "relativeStartXTick: " << relativeStartXTick << std::endl;
//        std::cout << "relativeStartX: " << verticalPixelTickValues[relativeStartX] << "    relativeEndX: " << verticalPixelTickValues[relativeEndX] << std::endl;
//        std::cout << "relStY: " << relativeStartY << "   relEnY: " << relativeEndY << std::endl;

        context->reset_clip();
        context->rectangle(relativeStartX, relativeStartY + 10, relativeEndX - relativeStartX, relativeEndY - relativeStartY - 10);
        context->clip();

        if (entryBlockIter->second->getInstanceOf() == NULL) {
            if (false == wasCutOff) {
                context->set_source_rgba(1.0, 0.0, 0.0, 0.3);
            } else {
                context->set_source_rgba(0.7, 0.0, 0.0, 0.3);
            }//if
        } else {
            if (false == wasCutOff) {
                context->set_source_rgba(1.0, 0.4, 0.0, 0.3);
            } else {
                context->set_source_rgba(0.7, 0.2, 0.0, 0.3);
            }//if
        }//if

        context->paint();

        if (entryBlockIter->second == selectedEntryBlock) {
            context->set_source_rgba(1.0, 1.0, 0.0, 0.8);
            context->set_line_cap(Cairo::LINE_CAP_ROUND);
            context->move_to(relativeStartX, relativeStartY + 10);
            context->line_to(relativeEndX, relativeEndY - 10);
            context->stroke();
        }//if

        context->move_to(relativeStartX + 10, relativeStartY + 10);

        std::string fontStr;
        {
            std::ostringstream tmpSS;
            tmpSS << globals.topBarFont << " bold " << globals.topBarFontSize;
            fontStr = tmpSS.str();
        }

        Glib::RefPtr<Pango::Layout> pangoLayout = Pango::Layout::create(context);
        Pango::FontDescription font_descr(fontStr.c_str());

        pangoLayout->set_font_description(font_descr);
        pangoLayout->set_text(entryBlockIter->second->getTitle());
        pangoLayout->update_from_cairo_context(context);  //gets cairo cursor position
        pangoLayout->add_to_cairo_context(context);       //adds text to cairos stack of stuff to be drawn
        context->set_source_rgba(1.0, 1.0, 1.0, 0.8);
        context->fill();
        context->stroke();

        SequencerEntryBlockSelectionInfo newSelectionInfo;
        newSelectionInfo.entry = this;
        newSelectionInfo.entryBlock = entryBlockIter->second;
        newSelectionInfo.drawnArea = Gdk::Rectangle(relativeStartX, relativeStartY + 10, relativeEndX - relativeStartX, relativeEndY - relativeStartY - 10);
        selectionInfos.push_back(newSelectionInfo);
    }//for
}//drawEntryBoxes

