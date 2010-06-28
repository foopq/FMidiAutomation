#include "Sequencer.h"
#include "FMidiAutomationMainWindow.h"
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

static const unsigned int entryWindowHeight = 138 + 6; //size plus padding
static const unsigned int smallEntryWindowHeight = 46 + 4; //size plus padding

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

SequencerEntryBlock::SequencerEntryBlock(boost::shared_ptr<SequencerEntry> owningEntry_, int startTick_, boost::shared_ptr<SequencerEntryBlock> instanceOf_)
{
    startTick_ = std::max(startTick_, 0);

    startTick = startTick_;
    instanceOf = instanceOf_;
//    duration = 200;

    owningEntry = owningEntry_;

    valuesPerPixel = std::numeric_limits<double>::max();
    offsetY = 0;

    if (instanceOf == NULL) {
        curve.reset(new Animation(this, boost::shared_ptr<Animation>()));
        secondaryCurve.reset(new Animation(this, boost::shared_ptr<Animation>()));
    } else {
        curve.reset(new Animation(this, instanceOf->curve));
        secondaryCurve.reset(new Animation(this, instanceOf->secondaryCurve));
    }//if
}//constructor

double SequencerEntryBlock::getValuesPerPixel()
{
    return valuesPerPixel;
}//getValuesPerPixel

double SequencerEntryBlock::getOffsetY()
{
    return offsetY;
}//getOffsetY

void SequencerEntryBlock::setValuesPerPixel(double valuesPerPixel_)
{
    valuesPerPixel = valuesPerPixel_;
}//setValuesPerPixel

void SequencerEntryBlock::setOffsetY(double offsetY_)
{
    offsetY = offsetY_;
}//setOffsetY

void SequencerEntryBlock::moveBlock(int startTick_)
{
    boost::shared_ptr<SequencerEntry> owningEntry_ = owningEntry.lock();
    if (owningEntry_ == NULL) {
        return;
    }//if

    if (owningEntry_->getEntryBlock(startTick_) != NULL) {
        return;
    }//if

    owningEntry_->removeEntryBlock(shared_from_this());

    startTick_ = std::max(startTick_, 0);
//    std::cout << "move block from: " << startTick << " to " << startTick_ << std::endl;

    startTick = startTick_;
    owningEntry_->addEntryBlock(startTick, shared_from_this());
}//moveBlock

boost::shared_ptr<Keyframe> SequencerEntryBlock::getNextKeyframe(boost::shared_ptr<Keyframe> keyframe)
{
    boost::shared_ptr<Keyframe> afterFirst = curve->getNextKeyframe(keyframe);

    if (afterFirst != NULL) {
        return afterFirst;
    }//if

    boost::shared_ptr<Keyframe> afterSecond = secondaryCurve->getNextKeyframe(keyframe);

    if (afterSecond != NULL) {
        return afterSecond;
    }//if

    return boost::shared_ptr<Keyframe>();
}//getNextKeyframe

//void SequencerEntryBlock::setDuration(int duration_)
//{
//    duration = duration_;
//}//setDuration

void SequencerEntryBlock::cloneCurves(boost::shared_ptr<SequencerEntryBlock> entryBlock)
{
    curve->absorbCurve(entryBlock->curve);
    secondaryCurve->absorbCurve(entryBlock->secondaryCurve);
}//cloneCurves

void SequencerEntryBlock::setTitle(const Glib::ustring &title_)
{
    title = title_;
}//setTitle

int SequencerEntryBlock::getStartTick() const
{
    return startTick;
}//getStartTick

int *SequencerEntryBlock::getRawStartTick()
{
    return &startTick;
}//getRawStartTick

int SequencerEntryBlock::getDuration() const
{
    if (instanceOf == NULL) {
        int duration = 0;

        if (curve != NULL) {
            int numKeys = curve->getNumKeyframes();
            if (numKeys > 0) {
                boost::shared_ptr<Keyframe> lastKey = curve->getKeyframe(numKeys-1);

                duration = std::max(duration, lastKey->tick);
            }//if
        }//if

        if (secondaryCurve != NULL) {
            int numKeys = secondaryCurve->getNumKeyframes();
            if (numKeys > 0) {
                boost::shared_ptr<Keyframe> lastKey = secondaryCurve->getKeyframe(numKeys-1);

                duration = std::max(duration, lastKey->tick);
            }//if
        }//if

        return duration;
    } else {
        return instanceOf->getDuration();
    }//if
}//getDuration

Glib::ustring SequencerEntryBlock::getTitle() const
{
    return title;
}//getTitle

boost::shared_ptr<SequencerEntryBlock> SequencerEntryBlock::getInstanceOf() const
{
    return instanceOf;
}//getInstanceOf

boost::shared_ptr<SequencerEntry> SequencerEntryBlock::getOwningEntry() const
{
    return owningEntry.lock();
}//getOwningEntry

boost::shared_ptr<Animation> SequencerEntryBlock::getCurve()
{
   return curve;
}//getCurve

boost::shared_ptr<Animation> SequencerEntryBlock::getSecondaryCurve()
{
    return secondaryCurve;
}//getSecondaryCurve

void SequencerEntryBlock::renderCurves(Cairo::RefPtr<Cairo::Context> context, GraphState &graphState, unsigned int areaWidth, unsigned int areaHeight)
{
    curve->render(context, graphState, areaWidth, areaHeight);
    secondaryCurve->render(context, graphState, areaWidth, areaHeight);
}//renderCurves

template<class Archive>
void SequencerEntryBlock::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(owningEntry);
    ar & BOOST_SERIALIZATION_NVP(startTick);
    ar & BOOST_SERIALIZATION_NVP(instanceOf);
//    ar & BOOST_SERIALIZATION_NVP(duration);
    ar & BOOST_SERIALIZATION_NVP(valuesPerPixel);
    ar & BOOST_SERIALIZATION_NVP(offsetY);

    std::string titleStr = Glib::locale_from_utf8(title);
    ar & BOOST_SERIALIZATION_NVP(titleStr);
    title = titleStr;

    ar & BOOST_SERIALIZATION_NVP(curve);
    ar & BOOST_SERIALIZATION_NVP(secondaryCurve);

    curve->startTick = &startTick;
    secondaryCurve->startTick = &startTick;
}//serialize

template<class Archive>
void SequencerEntryBlockSelectionInfo::serialize(Archive &ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(entry);
    ar & BOOST_SERIALIZATION_NVP(entryBlock);

    int x = drawnArea.get_x();
    int y = drawnArea.get_y();
    int width = drawnArea.get_width();
    int height = drawnArea.get_height();

    ar & BOOST_SERIALIZATION_NVP(x);
    ar & BOOST_SERIALIZATION_NVP(y);
    ar & BOOST_SERIALIZATION_NVP(width);
    ar & BOOST_SERIALIZATION_NVP(height);

    drawnArea.set_x(x);
    drawnArea.set_y(y);
    drawnArea.set_width(width);
    drawnArea.set_height(height);
}//serialize

SequencerEntryImpl::SequencerEntryImpl()
{
    controllerType = CC;
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

boost::shared_ptr<SequencerEntryImpl> SequencerEntryImpl::clone()
{
    boost::shared_ptr<SequencerEntryImpl> retVal(new SequencerEntryImpl);
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

SequencerEntry::SequencerEntry(const Glib::ustring &entryGlade, Sequencer *sequencer_, unsigned int entryNum)
{
    impl.reset(new SequencerEntryImpl);

    doInit(entryGlade, sequencer_, entryNum);
}//constructor

void SequencerEntry::doInit(const Glib::ustring &entryGlade, Sequencer *sequencer_, unsigned int entryNum)
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

    std::cout << "doInit: " << (std::string("qAutomation ") + boost::lexical_cast<std::string>(entryNum)) << std::endl;

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

boost::shared_ptr<SequencerEntryImpl> SequencerEntry::getImplClone()
{
    return impl->clone();
}//getImplClone

const boost::shared_ptr<SequencerEntryImpl> SequencerEntry::getImpl()
{
    return impl;
}//getImpl

void SequencerEntry::setNewDataImpl(boost::shared_ptr<SequencerEntryImpl> impl_)
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
    sequencer->editSequencerEntryProperties(shared_from_this(), true);

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

    Globals &globals = Globals::Instance();
    globals.graphDrawingArea->queue_draw();
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

    std::cout << "setTitle: " << title << std::endl;

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

    sequencer->notifySelected(this);

    return true;
}//mouseButtonPressed

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

void SequencerEntry::addEntryBlock(int, boost::shared_ptr<SequencerEntryBlock> entryBlock)
{
    removeEntryBlock(entryBlock);
    entryBlocks[entryBlock->getStartTick()] = entryBlock;

    if (entryBlock->getTitle().empty() == true) {
        entryBlock->setTitle(getTitle() + Glib::ustring(" - ") + boost::lexical_cast<Glib::ustring>(entryBlocks.size()));

//        std::cout << "entryBlock title: " << entryBlock->getTitle() << std::endl;
    }//if

//    std::cout << "addEntryBlock: " << entryBlock->getTitle() << "  --  " << entryBlock->getStartTick() << "(" << entryBlocks.size() << ")" << std::endl;
}//addEntryBlock

void SequencerEntry::removeEntryBlock(boost::shared_ptr<SequencerEntryBlock> entryBlock)
{
    if (entryBlocks.find(entryBlock->getStartTick()) != entryBlocks.end()) {
//        std::cout << "removed at: " << entryBlock->getStartTick() << std::endl;
        entryBlocks.erase(entryBlocks.find(entryBlock->getStartTick()));
    } else {
//        std::cout << "not removed at: " << entryBlock->getStartTick() << std::endl;
    }//if
}//removeEntryBlock

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

    BOOST_FOREACH (jack_port_t *port, inputPorts) {
        std::string portName = jackSingleton.getInputPortName(port);
        std::cout << "IN1: " << portName << std::endl;

        assert(portName.empty() == false);
        inputPortsStr.push_back(portName);
    }//foreach

    BOOST_FOREACH (jack_port_t *port, outputPorts) {
        std::string portName = jackSingleton.getOutputPortName(port);
        std::cout << "OUT1: " << portName << std::endl;

        assert(portName.empty() == false);
        outputPortsStr.push_back(portName);
    }//foreach

    ar & BOOST_SERIALIZATION_NVP(inputPortsStr);
    ar & BOOST_SERIALIZATION_NVP(outputPortsStr);

    inputPorts.clear();
    outputPorts.clear();

    BOOST_FOREACH (std::string portStr, inputPortsStr) {
        jack_port_t *port = jackSingleton.getInputPort(portStr);
        inputPorts.insert(port);

        std::cout << "IN2: " << portStr << " - " << port << std::endl;
    }//foreach

    BOOST_FOREACH (std::string portStr, outputPortsStr) {
        jack_port_t *port = jackSingleton.getOutputPort(portStr);
        outputPorts.insert(port);

        std::cout << "OUT2: " << portStr << " - " << port << std::endl;
    }//foreach

    std::cout << "SE serialize: " << isFullBox << std::endl;
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

    std::cout << "TITLE: " << title << std::endl;
}//serialize

boost::shared_ptr<SequencerEntryBlock> SequencerEntry::getEntryBlock(int tick)
{
    if (entryBlocks.find(tick) != entryBlocks.end()) {
        return entryBlocks[tick];
    } else {
        return boost::shared_ptr<SequencerEntryBlock>();
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

    std::map<int, boost::shared_ptr<SequencerEntryBlock> >::iterator entryBlockIter = entryBlocks.upper_bound(tick);
    if (entryBlockIter != entryBlocks.begin()) {
        entryBlockIter--;
    }//if

    double val = entryBlockIter->second->getCurve()->sample(tick);

    val = std::min(val, (double)impl->maxValue);
    val = std::max(val, (double)impl->minValue);

    return val;
}//sample

void SequencerEntry::clearRecordTokenBuffer()
{
    recordTokenBuffer.clear();
}//clearRecordTokenBuffer

void SequencerEntry::addRecordToken(MidiToken &token)
{
    if (impl->recordMode == false) {
        return;
    }//if

    if ((token.type == CC) && (impl->controllerType != SequencerEntryImpl::CC)) {
        return;
    }//if

    if ((impl->channel != 16) && (impl->channel != token.channel)) {
        return;
    }//if

    if ((token.type == CC) && (impl->msb != token.controller)) {
        return;
    }//if

    recordTokenBuffer.push_back(token);
}//addRecordToken

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

    std::cout << "adjustFillerHeight: " << labelHeight << std::endl;
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

std::cout << "entries: " << entries.size() << std::endl;
}//addEntry

boost::shared_ptr<SequencerEntry> Sequencer::addEntry(int index, bool useDefaults)
{
    boost::shared_ptr<SequencerEntry> newEntry(new SequencerEntry(entryGlade, this, entries.size()+1));

    if (false == useDefaults) {
        editSequencerEntryProperties(newEntry, false);
    }//if

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

std::pair<std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator, std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator> Sequencer::getEntryPair() const
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

boost::shared_ptr<SequencerEntryBlock> Sequencer::getSelectedEntryBlock() const
{
    return selectedEntryBlock;
}//getSelectedEntryBlock

boost::shared_ptr<SequencerEntryBlock> Sequencer::getSelectedEntryBlock(int x, int y, bool setSelection) //x/y is in graphDrawingArea pixels .. this is for mouse over and selection
{
//    std::cout << "getSelectedEntryBlock: " << x << " - " << y << "    " << setSelection << std::endl;

    if (x < 0) {
        selectedEntryBlock = (*entries.begin()).first->getEntryBlock(0);

if (selectedEntryBlock == NULL) {
    std::cout << "clearSelectedEntryBlock3" << std::endl;
}//if
        return selectedEntryBlock;
    }//if

    BOOST_FOREACH (SequencerEntryBlockSelectionInfo selectionInfo, selectionInfos) {
//        std::cout << "drawnArea: " << selectionInfo.drawnArea.get_x() << " - " << selectionInfo.drawnArea.get_y() << " - " << selectionInfo.drawnArea.get_width() << " - " << selectionInfo.drawnArea.get_height() << std::endl;

        if ( ((selectionInfo.drawnArea.get_x() <= x) && ((selectionInfo.drawnArea.get_x() + selectionInfo.drawnArea.get_width()) >= x)) &&
             ((selectionInfo.drawnArea.get_y() <= y) && ((selectionInfo.drawnArea.get_y() + selectionInfo.drawnArea.get_height()) >= y)) ) {
            if (true == setSelection) {
                selectedEntryBlock = selectionInfo.entryBlock;
                selectionInfo.entry->select();

if (selectedEntryBlock == NULL) {
    std::cout << "clearSelectedEntryBlock2" << std::endl;
}//if
            }//if
            return selectedEntryBlock;
        }//if
    }//foreach

    return boost::shared_ptr<SequencerEntryBlock>();
}//getSelectedEntryBlock

void Sequencer::clearSelectedEntryBlock()
{
std::cout << "clearSelectedEntryBlock" << std::endl;
    selectedEntryBlock.reset();
}//clearSelectedEntryBlock

void Sequencer::editSequencerEntryProperties(boost::shared_ptr<SequencerEntry> entry, bool createUpdatePoint)
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

    typedef std::pair<boost::shared_ptr<SequencerEntry>, int > SequencerEntryMapType;
    BOOST_FOREACH (SequencerEntryMapType entryPair, entries) {
        std::string entryTitle = entryPair.first->getTitle();
        entryPair.first->doInit(entryGlade, this, entryNum);
        entryPair.first->setTitle(entryTitle);
        Gtk::Widget *entryHookWidget = entryPair.first->getHookWidget();

std::cout << "HERE: " << entryHookWidget << " - " << entryTitle << " - " << entryPair.first->getTitle() << std::endl;

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


//template void SequencerEntry::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
//template void SequencerEntryImpl::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
//template void SequencerEntryBlock::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);
//template void SequencerEntryBlockSelectionInfo::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive &ar, const unsigned int version);

//template void SequencerEntry::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
//template void SequencerEntryImpl::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
//template void SequencerEntryBlock::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);
//template void SequencerEntryBlockSelectionInfo::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive &ar, const unsigned int version);


