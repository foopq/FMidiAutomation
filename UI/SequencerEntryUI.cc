/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

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
#include "SequencerUI.h"
#include "SequencerEntryUI.h"
#include "Data/SequencerEntry.h"
#include "Data/SequencerEntryBlock.h"
#include "Globals.h"
#include "GraphState.h"
#include "SerializationHelper.h"

namespace
{

#if 0
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
    if (viewport != nullptr) {
        viewport->modify_bg(Gtk::STATE_NORMAL, bgColour);
        viewport->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::Label *label = dynamic_cast<Gtk::Label *>(widget);
    if (label != nullptr) {
        label->modify_fg(Gtk::STATE_NORMAL, darkTextColour);
    }//if

    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(widget);
    if (entry != nullptr) {
        entry->modify_base(Gtk::STATE_NORMAL, bgColour);
        entry->modify_text(Gtk::STATE_NORMAL, darkTextColour);
        entry->modify_bg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::Frame *frame = dynamic_cast<Gtk::Frame *>(widget);
    if (frame != nullptr) {
        frame->modify_bg(Gtk::STATE_NORMAL, black);
    }//if

    Gtk::Button *button = dynamic_cast<Gtk::Button *>(widget);
    if (button != nullptr) {
        button->modify_bg(Gtk::STATE_NORMAL, bgColour);
    }//if

    Gtk::Table *table = dynamic_cast<Gtk::Table *>(widget);
    if (table != nullptr) {        
        table->modify_bg(Gtk::STATE_NORMAL, bgColour);
        table->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::EventBox *eventBox = dynamic_cast<Gtk::EventBox *>(widget);
    if (eventBox != nullptr) {
        eventBox->modify_bg(Gtk::STATE_NORMAL, bgColour);
        eventBox->modify_fg(Gtk::STATE_NORMAL, fgColour);
    }//if

    Gtk::ComboBox *comboBox = dynamic_cast<Gtk::ComboBox *>(widget);
    if (comboBox != nullptr) {
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
    if (alignment != nullptr) {
        alignment->modify_bg(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_fg(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_base(Gtk::STATE_NORMAL, bgColour);
        alignment->modify_text(Gtk::STATE_NORMAL, bgColour);
    }//if
    */

    Gtk::CellRendererText *cellRendererText = dynamic_cast<Gtk::CellRendererText *>(widget);
    if (cellRendererText != nullptr) {
        std::cout << "crt" << std::endl;
    }//if

    Gtk::Container *container = dynamic_cast<Gtk::Container *>(widget);
    if (container != nullptr) {
        Glib::ListHandle<Gtk::Widget *> children = container->get_children();
        for (Gtk::Widget *childWidget : children) {
            ::setThemeColours(childWidget);
        }//forach
    }//if
}//setThemeColours
#endif

}//anonymous namespace

SequencerEntryUI::SequencerEntryUI(const Glib::ustring &entryGlade, unsigned int entryNum, std::shared_ptr<SequencerEntry> baseEntry_,
                                    std::shared_ptr<SequencerUI> owningSequencer_)
{
    baseEntry = baseEntry_;
    owningSequencer = owningSequencer_;

    assert(baseEntry != nullptr);
    assert(owningSequencer.lock() != nullptr);

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
    switchButton->signal_clicked().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleSwitchPressed) );

    uiXml->get_widget("toggleButton1", switchButton);
    switchButton->signal_clicked().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleSwitchPressed) );

    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text("qAutomation " + boost::lexical_cast<std::string>(entryNum)); //!!!!!!!!!!!!!!!!!!!!!!!!! q!
    entryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &SequencerEntryUI::handleKeyEntryOnLargeTitleEntryBox));
    uiXml->get_widget("titleEntry1", entryBox);
    entryBox->set_text("qAutomation " + boost::lexical_cast<std::string>(entryNum)); //!!!!!!!!!!!!!!!!!!!!!!!!! q!
    entryBox->signal_key_release_event().connect(sigc::mem_fun(*this, &SequencerEntryUI::handleKeyEntryOnSmallTitleEntryBox));
    entryBox->signal_focus_in_event().connect(sigc::mem_fun(*this, &SequencerEntryUI::handleEntryFocus));

    if (baseEntry->getTitle().empty() == false) {
        setTitle(baseEntry->getTitle());
    } else {
        setTitle(std::string("2Automation ") + boost::lexical_cast<std::string>(entryNum)); //!!!!!!!!!!!!!!!!!!! q!
    }//if

//    std::cout << "doInit: " << (std::string("qAutomation ") + boost::lexical_cast<std::string>(entryNum)) << std::endl;

    Gtk::EventBox *eventBox;
    uiXml->get_widget("eventbox1", eventBox);
    eventBox->signal_button_press_event().connect ( sigc::mem_fun(*this, &SequencerEntryUI::mouseButtonPressed) );
    uiXml->get_widget("eventbox2", eventBox);
    eventBox->signal_button_press_event().connect ( sigc::mem_fun(*this, &SequencerEntryUI::mouseButtonPressed) );

    Gtk::ToggleButton *toggleButton;
    uiXml->get_widget("recButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleRecPressed) );
    uiXml->get_widget("recButton1", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleRecSmPressed) );

    uiXml->get_widget("muteButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleMutePressed) );
    uiXml->get_widget("muteButton1", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleMuteSmPressed) );

    uiXml->get_widget("soloButton", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleSoloPressed) );
    uiXml->get_widget("soloButton1", toggleButton);
    toggleButton->signal_toggled().connect ( sigc::mem_fun(*this, &SequencerEntryUI::handleSoloSmPressed) );

    mainWindow->get_parent()->remove(*mainWindow);
    smallWindow->get_parent()->remove(*smallWindow);

    isFullBox = false;
    curIndex = -1;

    deselect();
    //setThemeColours();
}//constructor

SequencerEntryUI::~SequencerEntryUI()
{
    //Nothing
}//destructor

std::shared_ptr<SequencerEntry> SequencerEntryUI::getBaseEntry()
{
    assert(baseEntry != nullptr);
    return baseEntry;
}//getBaseEntry

fmaipair<decltype(SequencerEntryUI::entryBlocks.begin()), decltype(SequencerEntryUI::entryBlocks.end())> SequencerEntryUI::getEntryBlocksPair()
{
    return fmai_make_pair(entryBlocks.begin(), entryBlocks.end());
}//getEntryBlocksPair

void SequencerEntryUI::setBaseEntry(std::shared_ptr<SequencerEntry> baseEntry_)
{
    baseEntry = baseEntry_;
    assert(baseEntry != nullptr);

    entryBlocks.clear();

    for (auto entryBlockBase : baseEntry->getEntryBlocksPair()) {
        std::shared_ptr<SequencerEntryBlockUI> entryBlock(new SequencerEntryBlockUI(entryBlockBase.second, shared_from_this()));
        addEntryBlock(entryBlock);
    }//for

std::cout << "SequencerEntryUI::setBaseEntry: " << entryBlocks.size() << std::endl;
}//setBaseEntry

std::shared_ptr<SequencerEntryUI> SequencerEntryUI::deepClone(const Glib::ustring &entryGlade)
{
    std::shared_ptr<SequencerEntryUI> clone(new SequencerEntryUI(entryGlade, 99, baseEntry, owningSequencer.lock()));

    /*
    clone->uiXml = uiXml;
    clone->mainWindow = mainWindow;
    clone->smallWindow = smallWindow;
    clone->largeFrame = largeFrame;
    clone->smallFrame = smallFrame;
    clone->activeCheckButton = activeCheckButton;
    */
    clone->isFullBox = isFullBox;
    clone->curIndex = curIndex;
 
    for(auto mapIter : entryBlocks) {
        std::shared_ptr<SequencerEntryBlockUI> entryBlockClone = mapIter.second->deepClone(shared_from_this());
        clone->entryBlocks[mapIter.first] = entryBlockClone;
    }//for

    return clone;
}//deepClone

/*
void SequencerEntryUI::setThemeColours()
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

    if (cellRendererText) {
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

    if (cellRendererText) {
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
*/

bool SequencerEntryUI::handleKeyEntryOnLargeTitleEntryBox(GdkEventKey *event)
{
    mouseButtonPressed(nullptr);
    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    Glib::ustring title = entryBox->get_text();

    uiXml->get_widget("titleEntry1", entryBox);
    entryBox->set_text(title);

    baseEntry->setTitle(title);

    std::cout << "handleKeyEntryOnLaterTitle...: " << getTitle() << std::endl;

    return true;
}//handleKeyEntryOnLargeTitleEntryBox

bool SequencerEntryUI::handleKeyEntryOnSmallTitleEntryBox(GdkEventKey *event)
{
    mouseButtonPressed(nullptr);
    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry1", entryBox);
    Glib::ustring title = entryBox->get_text();

    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text(title);

    baseEntry->setTitle(title);

std::cout << "hadnleKE..2: " << getTitle() << std::endl;

    return true;
}//handleKeyEntryOnSmallTitleEntryBox

void SequencerEntryUI::handleRecPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("recButton", toggleButtonLg);
    uiXml->get_widget("recButton1", toggleButtonSm);

    bool isActive = toggleButtonLg->get_active();
    toggleButtonSm->set_active(isActive);

    //impl->recordMode = isActive;
    baseEntry->setRecordMode(isActive);
}//handleRecPressed

void SequencerEntryUI::handleRecSmPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("recButton", toggleButtonLg);
    uiXml->get_widget("recButton1", toggleButtonSm);

    bool isActive = toggleButtonSm->get_active();
    toggleButtonLg->set_active(isActive);

    //impl->recordMode = isActive;
    baseEntry->setRecordMode(isActive);
}//handleRecSmPressed

void SequencerEntryUI::handleSoloPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("soloButton", toggleButtonLg);
    uiXml->get_widget("soloButton1", toggleButtonSm);

    bool isActive = toggleButtonLg->get_active();
    toggleButtonSm->set_active(isActive);

    //impl->soloMode = isActive;
    baseEntry->setSoloMode(isActive);
}//handleSoloPressed

void SequencerEntryUI::handleSoloSmPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("soloButton", toggleButtonLg);
    uiXml->get_widget("soloButton1", toggleButtonSm);

    bool isActive = toggleButtonSm->get_active();
    toggleButtonLg->set_active(isActive);

    //impl->soloMode = isActive;
    baseEntry->setSoloMode(isActive);
}//handleSoloSmPressed

void SequencerEntryUI::handleMutePressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("muteButton", toggleButtonLg);
    uiXml->get_widget("muteButton1", toggleButtonSm);

    bool isActive = toggleButtonLg->get_active();
    toggleButtonSm->set_active(isActive);

    //impl->muteMode = isActive;
    baseEntry->setMuteMode(isActive);
}//handleMutePressed

void SequencerEntryUI::handleMuteSmPressed()
{
    Gtk::ToggleButton *toggleButtonLg;
    Gtk::ToggleButton *toggleButtonSm;

    uiXml->get_widget("muteButton", toggleButtonLg);
    uiXml->get_widget("muteButton1", toggleButtonSm);

    bool isActive = toggleButtonSm->get_active();
    toggleButtonLg->set_active(isActive);

    //impl->muteMode = isActive;
    baseEntry->setMuteMode(isActive);
}//handleMuteSmPressed

void SequencerEntryUI::handleSwitchPressed()
{
    owningSequencer.lock()->editSequencerEntryProperties(shared_from_this(), true);

    /*
    mouseButtonPressed(nullptr);
    isFullBox = !isFullBox;

    if (false == isFullBox) {
        if (smallWindow->get_parent() != nullptr) {
            smallWindow->get_parent()->remove(*smallWindow);
        }//if
        sequencer->doSwapEntryBox(mainWindow, smallWindow);
    } else {
        if (mainWindow->get_parent() != nullptr) {
            mainWindow->get_parent()->remove(*mainWindow);
        }//if
        sequencer->doSwapEntryBox(smallWindow, mainWindow);
    }//if

    queue_draw();
    */
}//handleSwitchPressed

bool SequencerEntryUI::IsFullBox() const
{
    return isFullBox;
}//IsFullBox

void SequencerEntryUI::setLabelColour(Gdk::RGBA &colour)
{
    Gtk::EventBox *labelBox;
    uiXml->get_widget("indexLabelEventBox", labelBox);

    labelBox->override_background_color(colour, Gtk::STATE_FLAG_NORMAL);
}//setLabelColour

void SequencerEntryUI::setIndex(unsigned int index)
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

Glib::ustring SequencerEntryUI::getTitle() const
{
    return baseEntry->getTitle();
}//getTitle

void SequencerEntryUI::setTitle(Glib::ustring title)
{
    if (title.empty() == false) {

//    std::cout << "setTitle: " << title << std::endl;

        baseEntry->setTitle(title);
        Gtk::Entry *entryBox;
        uiXml->get_widget("titleEntry", entryBox);
        entryBox->set_text(getTitle());
        uiXml->get_widget("titleEntry1", entryBox);
        entryBox->set_text(getTitle());
    }//if
}//setTitle

unsigned int SequencerEntryUI::getIndex()
{
    return curIndex;
}//getIndex

Gtk::Widget *SequencerEntryUI::getHookWidget()
{
    if (true == isFullBox) {
        return mainWindow;
    } else {
        return smallWindow;
    }//if
}//getHookWidget

bool SequencerEntryUI::handleEntryFocus(GdkEventFocus*)
{
    mouseButtonPressed(nullptr);
    return true;
}//handleEntryFocus

bool SequencerEntryUI::mouseButtonPressed(GdkEventButton *event)
{
    Gdk::RGBA fgColour;
    Gdk::RGBA bgColour;

    fgColour.set_rgba_u(65535, 32768, 0, 65535);
    bgColour.set_rgba_u(10000, 10000, 10000, 65535);

    largeFrame->override_background_color(fgColour, Gtk::STATE_FLAG_NORMAL);
    largeFrame->override_color(fgColour, Gtk::STATE_FLAG_NORMAL);
    //largeFrame->modify_base(Gtk::STATE_NORMAL, fgColour);

    smallFrame->override_background_color(fgColour, Gtk::STATE_FLAG_NORMAL);
    smallFrame->override_color(fgColour, Gtk::STATE_FLAG_NORMAL);
    //smallFrame->modify_base(Gtk::STATE_NORMAL, fgColour);

    owningSequencer.lock()->notifySelected(shared_from_this());

    return true;
}//mouseButtonPressed

void SequencerEntryUI::setFocus()
{
    (void)mouseButtonPressed(nullptr);
}//setFocus

void SequencerEntryUI::select()
{
    mouseButtonPressed(nullptr);
    activeCheckButton->set_active(true);
//    std::cout << "select: " << getTitle() << std::endl;
}//select

void SequencerEntryUI::deselect()
{
    Gdk::RGBA fgColour;
    Gdk::RGBA bgColour;

    fgColour.set_rgba_u(52429, 42429, 52429, 65535);
    bgColour.set_rgba_u(10000, 10000, 10000, 65535);

//std::cout << "largeFrame2: " << largeFrame << "   ---   " << this << std::endl;
    largeFrame->get_label();

    largeFrame->override_background_color(bgColour, Gtk::STATE_FLAG_NORMAL);
    largeFrame->override_color(bgColour, Gtk::STATE_FLAG_NORMAL);
    //largeFrame->modify_base(Gtk::STATE_NORMAL, bgColour);

    smallFrame->override_background_color(bgColour, Gtk::STATE_FLAG_NORMAL);
    smallFrame->override_color(bgColour, Gtk::STATE_FLAG_NORMAL);
    //smallFrame->modify_base(Gtk::STATE_NORMAL, bgColour);

    activeCheckButton->set_active(false);
//    std::cout << "deselect: " << getTitle() << std::endl;
}//deselect

void SequencerEntryUI::addEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    removeEntryBlock(entryBlock);
    entryBlocks[entryBlock->getBaseEntryBlock()->getStartTick()] = entryBlock;

    if (entryBlock->getBaseEntryBlock()->getTitle().empty() == true) {
        entryBlock->getBaseEntryBlock()->setTitle(getTitle() + Glib::ustring(" - ") + boost::lexical_cast<Glib::ustring>(entryBlocks.size()));

//        std::cout << "entryBlock title: " << entryBlock->getTitle() << std::endl;
    }//if

//    std::cout << "addEntryBlock: " << entryBlock->getTitle() << "  --  " << entryBlock->getStartTick() << "(" << entryBlocks.size() << ")" << std::endl;
}//addEntryBlock

void SequencerEntryUI::removeEntryBlock(std::shared_ptr<SequencerEntryBlockUI> entryBlock)
{
    if (entryBlocks.find(entryBlock->getBaseEntryBlock()->getStartTick()) != entryBlocks.end()) {
//        std::cout << "removed at: " << entryBlock->getStartTick() << std::endl;
        entryBlocks.erase(entryBlocks.find(entryBlock->getBaseEntryBlock()->getStartTick()));
    } else {
//        std::cout << "not removed at: " << entryBlock->getStartTick() << std::endl;
    }//if
}//removeEntryBlock

void SequencerEntryUI::setUIBounds(unsigned int relativeStartY_, unsigned int relativeEndY_)
{
    relativeStartY = relativeStartY_;
    relativeEndY = relativeEndY_;
}//setUIBounds

std::pair<unsigned int, unsigned int> SequencerEntryUI::getUIBounds()
{
    return std::make_pair(relativeStartY, relativeEndY);
}//getUIBounds

void SequencerEntryUI::doSave(boost::archive::xml_oarchive &outputArchive)
{
    int SequencerEntryUIVersion = 1;
    outputArchive & BOOST_SERIALIZATION_NVP(SequencerEntryUIVersion);

    int numEntryBlocks = entryBlocks.size();    
    outputArchive & BOOST_SERIALIZATION_NVP(numEntryBlocks);

    for (auto entryBlockIter : entryBlocks) {
        int index = entryBlockIter.first;
        std::shared_ptr<SequencerEntryBlockUI> entryBlockUI = entryBlockIter.second;
        std::shared_ptr<SequencerEntryBlock> entryBlockBase = entryBlockUI->getBaseEntryBlock();

        outputArchive & BOOST_SERIALIZATION_NVP(index);
        outputArchive & BOOST_SERIALIZATION_NVP(isFullBox);
        outputArchive & BOOST_SERIALIZATION_NVP(entryBlockBase);

        entryBlockUI->doSave(outputArchive);
    }//for
}//doSave

void SequencerEntryUI::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    int SequencerEntryUIVersion = -1;
    inputArchive & BOOST_SERIALIZATION_NVP(SequencerEntryUIVersion);

    int numEntryBlocks = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(numEntryBlocks);

    entryBlocks.clear();
    for (int entryBlock = 0; entryBlock < numEntryBlocks; ++entryBlock) {
        int index = -1;
        std::shared_ptr<SequencerEntryBlock> entryBlockBase;

        inputArchive & BOOST_SERIALIZATION_NVP(index);
        inputArchive & BOOST_SERIALIZATION_NVP(isFullBox);
        inputArchive & BOOST_SERIALIZATION_NVP(entryBlockBase);

        std::shared_ptr<SequencerEntryBlockUI> entryBlockUI(new SequencerEntryBlockUI(entryBlockBase, shared_from_this()));
        entryBlocks[index] = entryBlockUI;

        entryBlockUI->doLoad(inputArchive);
    }//for
}//doLoad

#if 0
std::pair<std::shared_ptr<SequencerEntryBlock>, std::shared_ptr<SequencerEntryBlock> > SequencerEntryUI::splitEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock, int tick)
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
#endif

///////////////////////////////////////////////////////////////////////////////////
// Rendering code

void SequencerEntryUI::drawEntryBoxes(Cairo::RefPtr<Cairo::Context> context, std::vector<int> &verticalPixelTickValues, int relativeStartY, int relativeEndY, 
                                        std::vector<SequencerEntryBlockSelectionInfo> &selectionInfos,
                                        EntryBlockSelectionState &entryBlockSelectionState)

{
    Globals &globals = Globals::Instance();


    auto nextEntryBlockIter = entryBlocks.begin();
    for (auto entryBlockIter : entryBlocks) {
        ++nextEntryBlockIter;

        //std::cout << "drawEntryBoxes SEB: " << entryBlockIter->second.get() << " - " << &entryBlockSelectionState << std::endl;

        int startTick = entryBlockIter.second->getBaseEntryBlock()->getStartTick();
        int duration = entryBlockIter.second->getBaseEntryBlock()->getDuration();

        if ((startTick > verticalPixelTickValues[verticalPixelTickValues.size()-1]) || (startTick + duration < verticalPixelTickValues[0])) {
            continue;
        }//if

        int relativeStartXTick = startTick;
        relativeStartXTick = std::max(relativeStartXTick, verticalPixelTickValues[0]);

        int relativeEndXTick = startTick + duration;
        bool wasCutOff = false;
        if (nextEntryBlockIter != entryBlocks.end()) {
            if (nextEntryBlockIter->second->getBaseEntryBlock()->getStartTick() < relativeEndXTick) {
                relativeEndXTick = nextEntryBlockIter->second->getBaseEntryBlock()->getStartTick();
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

        if (entryBlockIter.second->getBaseEntryBlock()->getInstanceOf() == nullptr) {
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

        if (entryBlockSelectionState.IsSelected(entryBlockIter.second) == true) {
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
        pangoLayout->set_text(entryBlockIter.second->getBaseEntryBlock()->getTitle());
        pangoLayout->update_from_cairo_context(context);  //gets cairo cursor position
        pangoLayout->add_to_cairo_context(context);       //adds text to cairos stack of stuff to be drawn
        context->set_source_rgba(1.0, 1.0, 1.0, 0.8);
        context->fill();
        context->stroke();

        SequencerEntryBlockSelectionInfo newSelectionInfo;
        newSelectionInfo.entry = shared_from_this();
        newSelectionInfo.entryBlock = entryBlockIter.second;
        newSelectionInfo.drawnArea = Gdk::Rectangle(relativeStartX, relativeStartY + 10, relativeEndX - relativeStartX, relativeEndY - relativeStartY - 10);
        selectionInfos.push_back(newSelectionInfo);

        //std::cout << "selectionInfos added " << newSelectionInfo.entryBlock.get() << std::endl;
    }//for
}//drawEntryBoxes



