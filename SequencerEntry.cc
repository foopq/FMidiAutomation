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
#include "ProcessRecordedMidi.h"


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
        for (Gtk::TreeRow row : children) {

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
        for (Gtk::Widget *childWidget : children) {
            ::setThemeColours(childWidget);
        }//forach
    }//if
}//setThemeColours

}//anonymous namespace


SequencerEntryImpl::SequencerEntryImpl()
{
    controllerType = ControlType::CC;
    channel = 16;
    msb = 7;
    lsb = 0;

    //UI specific
    minValue = 0;
    maxValue = 127;
    sevenBit = true;
    useBothMSBandLSB = false; //implied true if sevenBit is true

    recordMode = false;
    soloMode = false;
    muteMode = false;
}//constructor

SequencerEntryImpl::~SequencerEntryImpl()
{
    //Nothing
}//destructor

std::shared_ptr<SequencerEntryImpl> SequencerEntryImpl::clone()
{
    std::shared_ptr<SequencerEntryImpl> retVal(new SequencerEntryImpl);
    *retVal = *this;
    return retVal;
}//clone

bool SequencerEntryImpl::operator==(SequencerEntryImpl &other)
{
    bool diff = false;

    diff |= this->controllerType != other.controllerType;
    diff |= this->msb != other.msb;
    diff |= this->lsb != other.lsb;
    diff |= this->minValue != other.minValue;
    diff |= this->maxValue != other.maxValue;
    diff |= this->sevenBit != other.sevenBit;
    diff |= this->useBothMSBandLSB != other.useBothMSBandLSB;
    diff |= this->channel != other.channel;
    diff |= this->title != other.title;
    diff |= this->recordMode != other.recordMode;
    diff |= this->soloMode != other.soloMode;
    diff |= this->muteMode != other.muteMode;

    return !diff;
}//operator==

SequencerEntry::SequencerEntry(const Glib::ustring &entryGlade, std::shared_ptr<Sequencer> sequencer_, unsigned int entryNum)
{
    impl.reset(new SequencerEntryImpl);

    doInit(entryGlade, sequencer_, entryNum);
}//constructor

void SequencerEntry::doInit(const Glib::ustring &entryGlade, std::shared_ptr<Sequencer> sequencer_, unsigned int entryNum)
{
    sequencer = sequencer_;
    
    uiXml = Gtk::Builder::create_from_string(entryGlade);
    uiXml->get_widget("entryViewport", mainWindow);
    uiXml->get_widget("smallEntryViewport", smallWindow);

    uiXml->get_widget("largeEntryFrame", largeFrame);
//    uiXml->get_widget("smallEntryFrame", smallFrame);
    uiXml->get_widget("smallEntryViewport", smallFrame);

    uiXml->get_widget("activeCheckButton", activeCheckButton);

//std::cout << "largeFrame: " << largeFrame << "   --    " << this << std::endl;
//largeFrame->get_label();

    Gtk::Button *switchButton;
    uiXml->get_widget("toggleButton", switchButton);
    switchButton->signal_clicked().connect ( sigc::mem_fun(*this, &SequencerEntry::handleSwitchPressed) );

    uiXml->get_widget("toggleButton1", switchButton);
    switchButton->signal_clicked().connect ( sigc::mem_fun(*this, &SequencerEntry::handleSwitchPressed) );

    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text("qAutomation " + boost::lexical_cast<std::string>(entryNum)); //!!!!!!!!!!!!!!!!!!!!!!!!! q!
    entryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &SequencerEntry::handleKeyEntryOnLargeTitleEntryBox));
    uiXml->get_widget("titleEntry1", entryBox);
    entryBox->set_text("qAutomation " + boost::lexical_cast<std::string>(entryNum)); //!!!!!!!!!!!!!!!!!!!!!!!!! q!
    impl->title = std::string("2Automation ") + boost::lexical_cast<std::string>(entryNum); //!!!!!!!!!!!!!!!!!!! q!
    entryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &SequencerEntry::handleKeyEntryOnSmallTitleEntryBox));
    entryBox->signal_focus_in_event().connect(sigc::mem_fun(*this, &SequencerEntry::handleEntryFocus));

//    std::cout << "doInit: " << (std::string("qAutomation ") + boost::lexical_cast<std::string>(entryNum)) << std::endl;

    Gtk::EventBox *eventBox;
    uiXml->get_widget("eventbox1", eventBox);
    eventBox->signal_button_press_event().connect ( sigc::mem_fun(*this, &SequencerEntry::mouseButtonPressed) );
    uiXml->get_widget("eventbox2", eventBox);
    eventBox->signal_button_press_event().connect ( sigc::mem_fun(*this, &SequencerEntry::mouseButtonPressed) );

    Gtk::ToggleButton *toggleButton;
    uiXml->get_widget("recButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntry::handleRecPressed) );
    uiXml->get_widget("recButton1", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntry::handleRecSmPressed) );

    uiXml->get_widget("muteButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntry::handleMutePressed) );
    uiXml->get_widget("muteButton1", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntry::handleMuteSmPressed) );

    uiXml->get_widget("soloButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntry::handleSoloPressed) );
    uiXml->get_widget("soloButton1", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntry::handleSoloSmPressed) );

    mainWindow->get_parent()->remove(*mainWindow);
    smallWindow->get_parent()->remove(*smallWindow);

    isFullBox = false;
    inHandler = false;

    curIndex = -1;

    deselect();

    setThemeColours();
}//doInit

SequencerEntry::~SequencerEntry()
{
    //Nothing
}//destructor

std::shared_ptr<SequencerEntry> SequencerEntry::deepClone()
{
    std::shared_ptr<SequencerEntry> clone(new SequencerEntry);

    clone->impl.reset(new SequencerEntryImpl);

    *clone->impl = *impl;

    clone->sequencer = sequencer;
    clone->uiXml = uiXml;
    clone->mainWindow = mainWindow;
    clone->smallWindow = smallWindow;
    clone->largeFrame = largeFrame;
    clone->smallFrame = smallFrame;
    clone->activeCheckButton = activeCheckButton;
    clone->isFullBox = isFullBox;
    clone->curIndex = curIndex;
 
    std::map<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > oldNewMap;

    for(std::map<int, std::shared_ptr<SequencerEntryBlock> >::const_iterator mapIter = entryBlocks.begin(); mapIter != entryBlocks.end(); ++mapIter) {
        std::shared_ptr<SequencerEntryBlock> entryBlockClone = mapIter->second->deepClone();
        clone->entryBlocks[mapIter->first] = entryBlockClone;

        oldNewMap[mapIter->second] = entryBlockClone;
    }//for

    for (std::map<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> >::const_iterator mapIter = oldNewMap.begin(); mapIter != oldNewMap.end(); ++mapIter) {
        if (mapIter->second->getInstanceOf() != NULL) {
            std::shared_ptr<SequencerEntryBlock> entryBlockClone = oldNewMap[mapIter->second->getInstanceOf()];
            assert(entryBlockClone != NULL);
            mapIter->second->setInstanceOf(entryBlockClone);
        }//if
    }//for

    clone->inputPorts = inputPorts;
    clone->outputPorts = outputPorts;

    clone->recordTokenBuffer = recordTokenBuffer;

    return clone;
}//deepClone

std::shared_ptr<SequencerEntryImpl> SequencerEntry::getImplClone()
{
    return impl->clone();
}//getImplClone

const std::shared_ptr<SequencerEntryImpl> SequencerEntry::getImpl()
{
    return impl;
}//getImpl

void SequencerEntry::setNewDataImpl(std::shared_ptr<SequencerEntryImpl> impl_)
{
    impl = impl_;

    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text(impl->title);
    uiXml->get_widget("titleEntry1", entryBox);
    entryBox->set_text(impl->title);

    std::cout << "setNewDataImpl: " << impl->title << std::endl;
}//setNewDataImpl

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

    impl->title = title;

    std::cout << "handleKeyEntryOnLaterTitle...: " << impl->title << std::endl;

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

    impl->title = title;

std::cout << "hadnleKE..2: " << impl->title << std::endl;

    return true;
}//handleKeyEntryOnSmallTitleEntryBox

void SequencerEntry::handleRecPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("recButton", toggleButtonLg);
    uiXml->get_widget("recButton1", toggleButtonSm);

    bool isActive = toggleButtonLg->get_active();
    toggleButtonSm->set_active(isActive);

    impl->recordMode = isActive;
}//handleRecPressed

void SequencerEntry::handleRecSmPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("recButton", toggleButtonLg);
    uiXml->get_widget("recButton1", toggleButtonSm);

    bool isActive = toggleButtonSm->get_active();
    toggleButtonLg->set_active(isActive);

    impl->recordMode = isActive;
}//handleRecSmPressed

void SequencerEntry::handleSoloPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("soloButton", toggleButtonLg);
    uiXml->get_widget("soloButton1", toggleButtonSm);

    bool isActive = toggleButtonLg->get_active();
    toggleButtonSm->set_active(isActive);

    impl->soloMode = isActive;
}//handleSoloPressed

void SequencerEntry::handleSoloSmPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("soloButton", toggleButtonLg);
    uiXml->get_widget("soloButton1", toggleButtonSm);

    bool isActive = toggleButtonSm->get_active();
    toggleButtonLg->set_active(isActive);

    impl->soloMode = isActive;
}//handleSoloSmPressed

void SequencerEntry::handleMutePressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("muteButton", toggleButtonLg);
    uiXml->get_widget("muteButton1", toggleButtonSm);

    bool isActive = toggleButtonLg->get_active();
    toggleButtonSm->set_active(isActive);

    impl->muteMode = isActive;
}//handleMutePressed

void SequencerEntry::handleMuteSmPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("muteButton", toggleButtonLg);
    uiXml->get_widget("muteButton1", toggleButtonSm);

    bool isActive = toggleButtonSm->get_active();
    toggleButtonLg->set_active(isActive);

    impl->muteMode = isActive;
}//handleMuteSmPressed

void SequencerEntry::handleSwitchPressed()
{
    sequencer.lock()->editSequencerEntryProperties(shared_from_this(), true);

    /*
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

    queue_draw();
    */
}//handleSwitchPressed

bool SequencerEntry::IsFullBox() const
{
    return isFullBox;
}//IsFullBox

void SequencerEntry::setLabelColour(Gdk::Color colour)
{
    Gtk::EventBox *labelBox;
    uiXml->get_widget("indexLabelEventBox", labelBox);

    labelBox->modify_bg(Gtk::STATE_NORMAL, colour);
}//setLabelColour

void SequencerEntry::setIndex(unsigned int index)
{
    Gtk::Label *label;
    uiXml->get_widget("indexLabel", label);

    /*
    if (label->get_text() == "###") {
        Gdk::Color defaultColour;

        int baseP[3] = {0xff, 0xcc, 0x99}; //stolen from qtractor
        float baseC[3] = {baseP[index %3] / 255.0f, baseP[(index/3)%3] / 255.0f, baseP[(index/9)%3] / 255.0f};

        defaultColour.set_rgb(baseC[0] * 65535, baseC[1] * 65535, baseC[2] * 65535);

        setLabelColour(defaultColour);
    }//if
    */

    label->set_text(boost::lexical_cast<std::string>(index+1));
    uiXml->get_widget("indexLabel1", label);
    label->set_text(boost::lexical_cast<std::string>(index+1));

    curIndex = index;
}//setIndex

Glib::ustring SequencerEntry::getTitle() const
{
    return impl->title;
}//getTitle

void SequencerEntry::setTitle(Glib::ustring title)
{
    if (title.empty() == false) {

//    std::cout << "setTitle: " << title << std::endl;

        impl->title = title;
        Gtk::Entry *entryBox;
        uiXml->get_widget("titleEntry", entryBox);
        entryBox->set_text(impl->title);
        uiXml->get_widget("titleEntry1", entryBox);
        entryBox->set_text(impl->title);
    }//if
}//setTitle

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

bool SequencerEntry::handleEntryFocus(GdkEventFocus*)
{
    mouseButtonPressed(NULL);
    return true;
}//handleEntryFocus

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

    sequencer.lock()->notifySelected(this);

    return true;
}//mouseButtonPressed

void SequencerEntry::setFocus()
{
    (void)mouseButtonPressed(NULL);
}//setFocus

void SequencerEntry::select()
{
    mouseButtonPressed(NULL);
    activeCheckButton->set_active(true);
//    std::cout << "select: " << getTitle() << std::endl;
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

    activeCheckButton->set_active(false);
//    std::cout << "deselect: " << getTitle() << std::endl;
}//deselect

void SequencerEntry::addEntryBlock(int, std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    removeEntryBlock(entryBlock);
    entryBlocks[entryBlock->getStartTick()] = entryBlock;

    if (entryBlock->getTitle().empty() == true) {
        entryBlock->setTitle(getTitle() + Glib::ustring(" - ") + boost::lexical_cast<Glib::ustring>(entryBlocks.size()));

//        std::cout << "entryBlock title: " << entryBlock->getTitle() << std::endl;
    }//if

//    std::cout << "addEntryBlock: " << entryBlock->getTitle() << "  --  " << entryBlock->getStartTick() << "(" << entryBlocks.size() << ")" << std::endl;
}//addEntryBlock

void SequencerEntry::removeEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    if (entryBlocks.find(entryBlock->getStartTick()) != entryBlocks.end()) {
//        std::cout << "removed at: " << entryBlock->getStartTick() << std::endl;
        entryBlocks.erase(entryBlocks.find(entryBlock->getStartTick()));
    } else {
//        std::cout << "not removed at: " << entryBlock->getStartTick() << std::endl;
    }//if
}//removeEntryBlock

void SequencerEntry::setUIBounds(unsigned int relativeStartY_, unsigned int relativeEndY_)
{
    relativeStartY = relativeStartY_;
    relativeEndY = relativeEndY_;
}//setUIBounds

std::pair<unsigned int, unsigned int> SequencerEntry::getUIBounds()
{
    return std::make_pair(relativeStartY, relativeEndY);
}//getUIBounds

template<class Archive>
void SequencerEntry::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(impl);
    ar & BOOST_SERIALIZATION_NVP(isFullBox);
    ar & BOOST_SERIALIZATION_NVP(curIndex);
    ar & BOOST_SERIALIZATION_NVP(entryBlocks);

    std::vector<std::string> inputPortsStr;
    std::vector<std::string> outputPortsStr;

    JackSingleton &jackSingleton = JackSingleton::Instance();

    for (jack_port_t *port : inputPorts) {
        std::string portName = jackSingleton.getInputPortName(port);
//        std::cout << "IN1: " << portName << std::endl;

        assert(portName.empty() == false);
        inputPortsStr.push_back(portName);
    }//foreach

    for (jack_port_t *port : outputPorts) {
        std::string portName = jackSingleton.getOutputPortName(port);
//        std::cout << "OUT1: " << portName << std::endl;

        assert(portName.empty() == false);
        outputPortsStr.push_back(portName);
    }//foreach

    ar & BOOST_SERIALIZATION_NVP(inputPortsStr);
    ar & BOOST_SERIALIZATION_NVP(outputPortsStr);

    inputPorts.clear();
    outputPorts.clear();

    for (std::string portStr : inputPortsStr) {
        jack_port_t *port = jackSingleton.getInputPort(portStr);
        inputPorts.insert(port);

//        std::cout << "IN2: " << portStr << " - " << port << std::endl;
    }//foreach

    for (std::string portStr : outputPortsStr) {
        jack_port_t *port = jackSingleton.getOutputPort(portStr);
        outputPorts.insert(port);

//        std::cout << "OUT2: " << portStr << " - " << port << std::endl;
    }//foreach

//    std::cout << "SE serialize: " << isFullBox << std::endl;
}//serialize

template<class Archive>
void SequencerEntryImpl::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(controllerType);
    ar & BOOST_SERIALIZATION_NVP(msb);
    ar & BOOST_SERIALIZATION_NVP(lsb);
    ar & BOOST_SERIALIZATION_NVP(channel);
    ar & BOOST_SERIALIZATION_NVP(minValue);
    ar & BOOST_SERIALIZATION_NVP(maxValue);
    ar & BOOST_SERIALIZATION_NVP(sevenBit);
    ar & BOOST_SERIALIZATION_NVP(useBothMSBandLSB);

    ar & BOOST_SERIALIZATION_NVP(recordMode);
    ar & BOOST_SERIALIZATION_NVP(soloMode);
    ar & BOOST_SERIALIZATION_NVP(muteMode);

    std::string titleStr = Glib::locale_from_utf8(title);
    ar & BOOST_SERIALIZATION_NVP(titleStr);
    title = titleStr;

//    std::cout << "TITLE: " << title << std::endl;
}//serialize

std::shared_ptr<SequencerEntryBlock> SequencerEntry::getEntryBlock(int tick)
{
    if (entryBlocks.find(tick) != entryBlocks.end()) {
        return entryBlocks[tick];
    } else {
        return std::shared_ptr<SequencerEntryBlock>();
    }//if
}//getEntryBlock

std::set<jack_port_t *> SequencerEntry::getInputPorts() const
{
    return inputPorts;
}//getInputPorts

std::set<jack_port_t *> SequencerEntry::getOutputPorts() const
{
    return outputPorts;
}//getOutputPorts

void SequencerEntry::setInputPorts(std::set<jack_port_t *> ports)
{
    inputPorts = ports;
}//setInputPorts

void SequencerEntry::setOutputPorts(std::set<jack_port_t *> ports)
{
    outputPorts = ports;
}//setOutputPorts

double SequencerEntry::sample(int tick)
{
    if (entryBlocks.empty() == true) {
        return 0;
    }//if

    std::map<int, std::shared_ptr<SequencerEntryBlock> >::iterator entryBlockIter = entryBlocks.upper_bound(tick);
    if (entryBlockIter != entryBlocks.begin()) {
        entryBlockIter--;
    }//if

    double val = entryBlockIter->second->getCurve()->sample(tick);

    val = std::min(val, (double)impl->maxValue);
    val = std::max(val, (double)impl->minValue);

    return val;
}//sample

unsigned char SequencerEntry::sampleChar(int tick)
{
    double value = sample(tick);
    value -= impl->minValue;
    value /= (double)(impl->maxValue - impl->minValue);

    if (true == impl->sevenBit) {
        value *= 127.0 + 0.5;
    } else {        
        value *= 255.0 + 0.5;
    }//if

    return (unsigned char)value;
}//sampleChar

void SequencerEntry::clearRecordTokenBuffer()
{
    std::cout << "clearRecordTokenBuffer" << std::endl;
    recordTokenBuffer.clear();
}//clearRecordTokenBuffer

void SequencerEntry::addRecordToken(std::shared_ptr<MidiToken> token)
{
    if (impl->recordMode == false) {
        std::cout << "out 1" << std::endl;
        return;
    }//if

    if ((token->type == MidiTokenType::CC) && (impl->controllerType != ControlType::CC)) {
        std::cout << "out 2" << std::endl;
        return;
    }//if

    if ((impl->channel != 16) && (impl->channel != token->channel)) {
        std::cout << "out 3" << std::endl;
        return;
    }//if

    if ((token->type == MidiTokenType::CC) && (impl->msb != token->controller)) {
        std::cout << "out 4" << std::endl;
        return;
    }//if

    std::cout << "add token" << std::endl;
    recordTokenBuffer.push_back(token);
}//addRecordToken

std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > SequencerEntry::splitEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock, int tick)
{
    if ((tick <= entryBlock->getStartTick()) || (tick >= (entryBlock->getStartTick() + entryBlock->getDuration()))) {
        return std::make_pair(entryBlock, entryBlock);
    }//if

    std::shared_ptr<Animation> curve = entryBlock->getCurve();
    std::shared_ptr<Animation> secondaryCurve = entryBlock->getSecondaryCurve();

    std::shared_ptr<SequencerEntryBlock> firstBlock(new SequencerEntryBlock(shared_from_this(), entryBlock->getStartTick(), std::shared_ptr<SequencerEntryBlock>()));
    std::shared_ptr<Animation> newCurve = firstBlock->getCurve();
    std::shared_ptr<Animation> newSecondaryCurve = firstBlock->getSecondaryCurve();    

    int curveNumKeys = curve->getNumKeyframes();
    int index = 0;
    for (index = 0; index < curveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = curve->getKeyframe(index);
        if (curKey->tick + entryBlock->getStartTick() < tick) {
            newCurve->addKey(curKey->deepClone());
        } else {
            break;
        }//if
    }//for

    int secondaryCurveNumKeys = secondaryCurve->getNumKeyframes();
    int secondaryIndex = 0;
    for (secondaryIndex = 0; secondaryIndex < secondaryCurveNumKeys; ++secondaryIndex) {
        std::shared_ptr<Keyframe> curKey = secondaryCurve->getKeyframe(index);
        if (curKey->tick + entryBlock->getStartTick() < tick) {
            newSecondaryCurve->addKey(curKey->deepClone());
        } else {
            break;
        }//if
    }//for

    int secondStartTick = secondaryCurve->getKeyframe(index)->tick + entryBlock->getStartTick();
    std::shared_ptr<SequencerEntryBlock> secondBlock(new SequencerEntryBlock(shared_from_this(), secondStartTick, std::shared_ptr<SequencerEntryBlock>()));
    newCurve = secondBlock->getCurve();
    newSecondaryCurve = secondBlock->getSecondaryCurve();    

    for (/*nothing*/; index < curveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = curve->getKeyframe(index);
        std::shared_ptr<Keyframe> keyClone = curKey->deepClone();

        keyClone->tick -= secondStartTick;
        newCurve->addKey(keyClone);
    }//for

    for (/*nothing*/; secondaryIndex < secondaryCurveNumKeys; ++index) {
        std::shared_ptr<Keyframe> curKey = secondaryCurve->getKeyframe(index);
        std::shared_ptr<Keyframe> keyClone = curKey->deepClone();

        keyClone->tick -= secondStartTick;
        newSecondaryCurve->addKey(keyClone);
    }//for

    removeEntryBlock(entryBlock);
    addEntryBlock(firstBlock->getStartTick(), firstBlock);
    addEntryBlock(secondBlock->getStartTick(), secondBlock);

    return std::make_pair(firstBlock, secondBlock);
}//splitEntryBlock


///////////////////////////////////////////////////////////////////////////////////
// Rendering code

void SequencerEntry::drawEntryBoxes(Cairo::RefPtr<Cairo::Context> context, std::vector<int> &verticalPixelTickValues, int relativeStartY, int relativeEndY, 
                                        std::vector<SequencerEntryBlockSelectionInfo> &selectionInfos,
                                        EntryBlockSelectionState &entryBlockSelectionState)

{
    Globals &globals = Globals::Instance();

    for (std::map<int, std::shared_ptr<SequencerEntryBlock> >::const_iterator entryBlockIter = entryBlocks.begin(); entryBlockIter != entryBlocks.end(); ++entryBlockIter) {

        //std::cout << "drawEntryBoxes SEB: " << entryBlockIter->second.get() << " - " << &entryBlockSelectionState << std::endl;

        int startTick = entryBlockIter->second->getStartTick();
        int duration = entryBlockIter->second->getDuration();

        if ((startTick > verticalPixelTickValues[verticalPixelTickValues.size()-1]) || (startTick + duration < verticalPixelTickValues[0])) {
            continue;
        }//if

        std::map<int, std::shared_ptr<SequencerEntryBlock> >::const_iterator nextEntryBlockIter = entryBlockIter;
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

        if (relativeEndX - relativeStartX < verticalPixelTickValues.size() * 0.05) {
            relativeEndX = relativeStartX + verticalPixelTickValues.size() * 0.05;
        }//if

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

        if (entryBlockSelectionState.IsSelected(entryBlockIter->second) == true) {
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
        newSelectionInfo.entry = shared_from_this();
        newSelectionInfo.entryBlock = entryBlockIter->second;
        newSelectionInfo.drawnArea = Gdk::Rectangle(relativeStartX, relativeStartY + 10, relativeEndX - relativeStartX, relativeEndY - relativeStartY - 10);
        selectionInfos.push_back(newSelectionInfo);

        //std::cout << "selectionInfos added " << newSelectionInfo.entryBlock.get() << std::endl;
    }//for
}//drawEntryBoxes


template void SequencerEntry::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
template void SequencerEntryImpl::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

template void SequencerEntry::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
template void SequencerEntryImpl::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);

