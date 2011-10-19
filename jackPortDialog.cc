/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include <iostream>
#include "jackPortDialog.h"
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Ellipse.hpp>
#include <flowcanvas/Module.hpp>
#include <flowcanvas/Port.hpp>
#include "jack.h"
#include "Data/Sequencer.h"
#include "Data/SequencerEntry.h"
#include <set>
#include <limits>
#include <math.h>
#include "Globals.h"

/*
//FIXME: This is a horrible hack!
void FlowCanvas::Canvas::arrange(bool) 
{
    //Nothing
}//arrange
*/   


namespace
{
class JackPortFlowCanvas;
class JackPortModule;
class EntryModule;
class EntryPort;

JackPortFlowCanvas *flowCanvas = nullptr;
std::vector<std::string> *curNamingPorts = nullptr;
JackPortModule *jackInputModule = nullptr;
JackPortModule *jackOutputModule = nullptr;
std::vector<EntryModule *> entryModules;

struct CanvasPositions
{
    std::pair<double, double> jackInputPosition;
    std::pair<double, double> jackOutputPosition;
    std::map<SequencerEntry *, std::pair<unsigned int, unsigned int> > entryPositions; //XXX: Can't use a weak_ptr since operator< is deleted

    CanvasPositions()
    {
        jackInputPosition.first = -1;
        jackInputPosition.second = -1;
        jackOutputPosition.first = -1;
        jackOutputPosition.second = -1;
    }//constructor
};//CanvasPositions

static CanvasPositions canvasPositions;

class JackPortFlowCanvas : public virtual FlowCanvas::Canvas
{
public:    
    JackPortFlowCanvas();
    virtual ~JackPortFlowCanvas();

    void addModule(FlowCanvas::Module *module);
    void clearModules();

    virtual void connect(FlowCanvas::Connectable *c1, FlowCanvas::Connectable *c2);
    virtual void disconnect(FlowCanvas::Connectable *c1, FlowCanvas::Connectable *c2);

private:
    std::set<FlowCanvas::Module *> modules;
};//JackPortFlowCanvas

class JackPortModule : public FlowCanvas::Module
{
public:
	JackPortModule(FlowCanvas::Canvas *canvas, const std::string& title, double x, double y, bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_);
    ~JackPortModule();

    void menu_addPort();
    void removePort(const std::string &name);
    virtual void move(double dx, double dy);
    bool checkPortName(GdkEventKey *event);
    void setCurNamingPorts();
    void restorePosition();

    void doresize();
    void do_add_port(FlowCanvas::Port *port);

    std::vector<std::string> getPorts();

private:    
	////void create_menu();
    virtual bool show_menu(GdkEventButton *ev);

    std::shared_ptr<Gtk::Menu> _menu;
    bool inputs;
    Glib::RefPtr<Gtk::Builder> uiXml;
    std::vector<std::string> ports;
};//JackPortModule

class EntryModule : public FlowCanvas::Module
{
public:
	EntryModule(FlowCanvas::Canvas *canvas, const std::string& title, double x, double y, Glib::RefPtr<Gtk::Builder> uiXml_, std::shared_ptr<SequencerEntry> entry_);
    ~EntryModule();

    virtual void move(double dx, double dy);
    void restorePosition();

    void addPortConnection(EntryPort *entryPort, const std::string &jackTitle);
    void removePortConnection(EntryPort *entryPort, const std::string &jackTitle);

    std::shared_ptr<SequencerEntry> getEntry() const;

    void doresize();
    void do_add_port(FlowCanvas::Port *port);

private:    
    std::shared_ptr<SequencerEntry> entry;
    Glib::RefPtr<Gtk::Builder> uiXml;
    std::set<std::string> inputConnections;
    std::set<std::string> outputConnections;
};//EntryModule

class JackPortPort : public FlowCanvas::Port
{
public:
	JackPortPort(JackPortModule *module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~JackPortPort();

    std::string getTitle() const;

private:    
	////void create_menu();
    virtual bool show_menu(GdkEventButton *ev);
    void menu_renamePort();
    void menu_removePort();

    std::shared_ptr<Gtk::Menu> _menu;
    JackPortModule *module;
    std::string title;
    Glib::RefPtr<Gtk::Builder> uiXml;
    bool isInput;
};//JackPortPort

class EntryPort : public FlowCanvas::Port
{
public:
	EntryPort(EntryModule *module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~EntryPort();

    bool isInput() const;

private:    
    EntryModule *entryModule;
    std::string title;
    Glib::RefPtr<Gtk::Builder> uiXml;
    bool isThisInput;
};//JackPortPort

//////////////////////////////

JackPortFlowCanvas::JackPortFlowCanvas() : FlowCanvas::Canvas(3000, 3000)
{
    //Nothing
}//constructor

JackPortFlowCanvas::~JackPortFlowCanvas()
{
    //Nothing
}//destructor

void JackPortFlowCanvas::addModule(FlowCanvas::Module *module)
{
    modules.insert(module);
    this->add_item(module);
}//addModule

void JackPortFlowCanvas::clearModules()
{
    for (auto item : modules) {
        remove_item(item);
    }//foreach

    //_items.clear();

    modules.clear();
}//clearModules

void JackPortFlowCanvas::disconnect(FlowCanvas::Connectable *c1, FlowCanvas::Connectable *c2)
{
    JackPortPort *jackPort = nullptr;
    EntryPort *entryPort = nullptr;
    EntryModule *entryModule = nullptr;    

    jackPort = dynamic_cast<JackPortPort *>(c1);
    entryPort = dynamic_cast<EntryPort *>(c2);
    if (jackPort == nullptr) {
        jackPort = dynamic_cast<JackPortPort *>(c2);
        if (jackPort == nullptr) {
            return;
        }//if
        entryPort = dynamic_cast<EntryPort *>(c1);
    }//if

    if (entryPort == nullptr) {
        return;
    }//if

    for (auto curEntryModule : entryModules) {
        for (auto port : curEntryModule->ports()) {
            if (port == entryPort) {
                entryModule = curEntryModule;
                break;
            }//if
        }//foreach

        if (entryModule != nullptr) {
            break;
        }//if
    }//forach

    if (entryModule == nullptr) {
        //Shouldn't happen...
        return;
    }//if

    entryModule->removePortConnection(entryPort, jackPort->getTitle());

    if (true == entryPort->isInput()) {
        flowCanvas->remove_connection(jackPort, entryPort);
    } else {
        flowCanvas->remove_connection(entryPort, jackPort);
    }//if    
}//disconnect

void JackPortFlowCanvas::connect(FlowCanvas::Connectable *c1, FlowCanvas::Connectable *c2)
{
    JackPortPort *jackPort = nullptr;
    EntryPort *entryPort = nullptr;
    EntryModule *entryModule = nullptr;

    jackPort = dynamic_cast<JackPortPort *>(c1);
    entryPort = dynamic_cast<EntryPort *>(c2);
    if (jackPort == nullptr) {
        jackPort = dynamic_cast<JackPortPort *>(c2);
        if (jackPort == nullptr) {
            return;
        }//if
        entryPort = dynamic_cast<EntryPort *>(c1);
    }//if

    if (entryPort == nullptr) {
        return;
    }//if

    for (auto curEntryModule : entryModules) {
        for (auto port : curEntryModule->ports()) {
            if (port == entryPort) {
                entryModule = curEntryModule;
                break;
            }//if
        }//foreach

        if (entryModule != nullptr) {
            break;
        }//if
    }//forach

    if (entryModule == nullptr) {
        //Shouldn't happen...
        return;
    }//if

    entryModule->addPortConnection(entryPort, jackPort->getTitle());

    if (true == entryPort->isInput()) {
        unsigned int colour = 0x953c02ff;
        flowCanvas->add_connection(jackPort, entryPort, colour);
    } else {
        unsigned int colour = 0x027055ff;
        flowCanvas->add_connection(entryPort, jackPort, colour);
    }//if
}//connect

//////////////////////////////

JackPortModule::JackPortModule(FlowCanvas::Canvas *canvas, const std::string& title, double x, double y, bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_)
                : FlowCanvas::Module(*canvas, title, x, y, true, true)
{
    uiXml = uiXml_;
    inputs = inputs_;
    ports = ports_;

    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &JackPortModule::checkPortName));

    ////create_menu();
}//constructor

JackPortModule::~JackPortModule()
{
    //Nothing
}//destructor

void JackPortModule::doresize()
{
    resize();
}//doresize

void JackPortModule::do_add_port(FlowCanvas::Port *port)
{
    add_port(port);
}//do_add_port

std::vector<std::string> JackPortModule::getPorts()
{
    return ports;
}//getPorts

void JackPortModule::restorePosition()
{
    if (true == inputs) {
        if (canvasPositions.jackInputPosition.first < 1) {
            FlowCanvas::Module::move(1250, 1500);
            canvasPositions.jackInputPosition.first = 1250;
            canvasPositions.jackInputPosition.second = 1500;
        } else {
            FlowCanvas::Module::move(canvasPositions.jackInputPosition.first, canvasPositions.jackInputPosition.second);
        }//if
    } else {
        if (canvasPositions.jackOutputPosition.first < 1) {
            FlowCanvas::Module::move(1550, 1500);
            canvasPositions.jackOutputPosition.first = 1550;
            canvasPositions.jackOutputPosition.second = 1500;
        } else {
            FlowCanvas::Module::move(canvasPositions.jackOutputPosition.first, canvasPositions.jackOutputPosition.second);
        }//if
    }//if
}//restorePosition

void JackPortModule::move(double dx, double dy) {
    FlowCanvas::Module::move(dx, dy);

    if (false == inputs) {
        canvasPositions.jackOutputPosition.first += dx;
        canvasPositions.jackOutputPosition.second += dy;
    } else {
        canvasPositions.jackInputPosition.first += dx;
        canvasPositions.jackInputPosition.second += dy;
    }//if
}//move

void JackPortModule::setCurNamingPorts()
{
    curNamingPorts = &ports;
}//setCurNamingPorts

void JackPortModule::menu_addPort()
{
    setCurNamingPorts();

    Gtk::Entry *entry = nullptr;
    uiXml->get_widget("portNameEntry", entry);
    entry->set_text("");

    Gtk::Label *label = nullptr;
    uiXml->get_widget("portNameDialogTitle", label);
    label->set_text("Add Port");

    Gtk::Button *button = nullptr;
    uiXml->get_widget("portNameDialogOKButton", button);
    button->set_sensitive(false);

    Gtk::Dialog *dialog = nullptr;
    uiXml->get_widget("portNameDialog", dialog);

    int result = dialog->run();

    if (result == 1) { //OK
        std::string portName = entry->get_text();
        if (portName.empty() == false) {
            ports.push_back(portName);

            unsigned int colour = 0x027055ff;
            if (true == inputs) {
                colour = 0x953c02ff;
            }//if

            JackPortPort *port = new JackPortPort(this, portName, inputs, colour, uiXml);

            this->add_port(port);
            this->resize();
        }//if
    }//if

    dialog->hide();
}//menu_addPort

void JackPortModule::removePort(const std::string &title)
{
    if (ports.size() == 1) {
        return;
    }//if

    FlowCanvas::Port *port = get_port(title);

    while (port->connections().empty() == false) {
        FlowCanvas::Connection *connection = *port->connections().begin();
        flowCanvas->remove_connection(connection->source(), connection->dest());
        port->remove_connection(connection);
    }//foreach

    if (port != nullptr) {
        remove_port(port);
        resize();
    }//if

    std::vector<std::string>::iterator oldIter = std::find(ports.begin(), ports.end(), title);
    if (oldIter != ports.end()) {
        ports.erase(oldIter);
    }//if
}//removeModule

bool JackPortModule::checkPortName(GdkEventKey *event)
{
    Gtk::Entry *entry = nullptr;
    uiXml->get_widget("portNameEntry", entry);

    Gtk::Button *button = nullptr;
    uiXml->get_widget("portNameDialogOKButton", button);

    std::string portName = entry->get_text();

    if ((portName.empty() == true) || (std::find(curNamingPorts->begin(), curNamingPorts->end(), portName) != curNamingPorts->end())) {
        button->set_sensitive(false);
    } else {
        button->set_sensitive(true);
    }//if

    return true;
}//checkPortName

/*
void JackPortModule::create_menu()
{
    _menu = new Gtk::Menu();
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(this, &JackPortModule::menu_addPort)));
}//create_menu
*/

bool JackPortModule::show_menu(GdkEventButton *ev)
{
    _menu.reset(new Gtk::Menu());
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(this, &JackPortModule::menu_addPort)));
    _menu->popup(ev->button, ev->time);
    return true;
}//show_menu

//////////////////////////////

EntryModule::EntryModule(FlowCanvas::Canvas *canvas, const std::string& title, double x, double y, Glib::RefPtr<Gtk::Builder> uiXml_, std::shared_ptr<SequencerEntry> entry_)
                            : FlowCanvas::Module(*canvas, title, x, y, true, true)
{
    std::cout << "EntryModule() constructor: " << this << std::endl;

    uiXml = uiXml_;
    entry = entry_;
}//constructor

EntryModule::~EntryModule()
{
    std::cout << "~EntryModule(): " << this << std::endl;

    //Nothing
}//destructor

void EntryModule::doresize()
{
    resize();
}//doresize

void EntryModule::do_add_port(FlowCanvas::Port *port)
{
    add_port(port);
}//do_add_port

namespace 
{
void readjustPosition(unsigned int &posx, unsigned int &posy)
{
    if (posy > 2800) {
        if (posx > 2800) {
            posy = 1520;
            posx = 1420;
            return;
        }//if

        posy = 1500;
        posx += 100;
    }//if    

    const float minDistance = 150;
    float dist;

    dist = sqrt( (posx - canvasPositions.jackOutputPosition.first) * (posx - canvasPositions.jackOutputPosition.first) + 
                 (posy - canvasPositions.jackOutputPosition.second) * (posy - canvasPositions.jackOutputPosition.second) );

    if (dist < minDistance) {
        posy += minDistance;
        readjustPosition(posx, posy);
        return;
    }//if

    dist = sqrt( (posx - canvasPositions.jackInputPosition.first) * (posx - canvasPositions.jackInputPosition.first) + 
                 (posy - canvasPositions.jackInputPosition.second) * (posy - canvasPositions.jackInputPosition.second) );

    if (dist < minDistance) {
        posy += minDistance;
        readjustPosition(posx, posy);
        return;
    }//if

    for (auto entryIter : canvasPositions.entryPositions) {
        dist = sqrt( (posx - entryIter.second.first) * (posx - entryIter.second.first) + 
                     (posy - entryIter.second.second) * (posy - entryIter.second.second) );

        if (dist < minDistance) {
            posy += minDistance;
            readjustPosition(posx, posy);
            return;
        }//if
    }//for
}//readjustPosition
}//anonymous namespace

void EntryModule::restorePosition()
{
    if (canvasPositions.entryPositions.find(entry.get()) != canvasPositions.entryPositions.end()) {
        std::pair<unsigned int, unsigned int> lastPos = canvasPositions.entryPositions[entry.get()];
        FlowCanvas::Module::move(lastPos.first, lastPos.second);

        std::cout << "entry1: " << lastPos.first << " - " << lastPos.second << std::endl;
    } else {
        unsigned int posx = 1400;
        unsigned int posy = 1500;

        readjustPosition(posx, posy);

        canvasPositions.entryPositions[entry.get()] = std::make_pair(posx, posy);

        FlowCanvas::Module::move(posx, posy);

        std::cout << "entry2: " << posx << " - " << posy << std::endl;
    }//if
}//restorePosition

void EntryModule::move(double dx, double dy) {
    FlowCanvas::Module::move(dx, dy);

    if (canvasPositions.entryPositions.find(entry.get()) != canvasPositions.entryPositions.end()) {
        std::pair<unsigned int, unsigned int> lastPos = canvasPositions.entryPositions[entry.get()];
        canvasPositions.entryPositions[entry.get()] = std::make_pair(lastPos.first + dx, lastPos.second + dy);
    }//if
}//move

std::shared_ptr<SequencerEntry> EntryModule::getEntry() const
{
    return entry;
}//getEntry

void EntryModule::addPortConnection(EntryPort *entryPort, const std::string &jackPortTitle)
{
    std::cout << "addPortconnection: " << jackPortTitle << " " << inputConnections.size() << " - " << this << std::endl;

    if (entryPort->isInput() == true) {
        for (auto title : inputConnections) {
            std::cout << "connection: " << title << std::endl;
        }//for

        inputConnections.insert(jackPortTitle);
    } else {
        outputConnections.insert(jackPortTitle);
    }//if
}//addPortConnection

void EntryModule::removePortConnection(EntryPort *entryPort, const std::string &jackPortTitle)
{
    if (entryPort->isInput() == true) {
        if (inputConnections.find(jackPortTitle) != inputConnections.end()) {
            inputConnections.erase(inputConnections.find(jackPortTitle));
        }//if
    } else {
        if (outputConnections.find(jackPortTitle) != outputConnections.end()) {
            outputConnections.erase(outputConnections.find(jackPortTitle));
        }//if
    }//if
}//removePortConnection

//////////////////////////////

JackPortPort::JackPortPort(JackPortModule *module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : FlowCanvas::Port(*module_, title_, !isInput_, colour)
{
    uiXml = uiXml_;
    module = module_;
    title = title_;
    isInput = isInput_;

    ////create_menu();
}//constructor

JackPortPort::~JackPortPort()
{
    //Nothing
}//destructor

/*
void JackPortPort::create_menu()
{
    _menu = new Gtk::Menu();
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(module.get(), &JackPortModule::menu_addPort)));
    items.push_back(Gtk::Menu_Helpers::MenuElem("Rename Port", sigc::mem_fun(this, &JackPortPort::menu_renamePort)));
    items.push_back(Gtk::Menu_Helpers::SeparatorElem());
    items.push_back(Gtk::Menu_Helpers::MenuElem("Remove Port", sigc::mem_fun(this, &JackPortPort::menu_removePort)));
}//create_menu
*/

bool JackPortPort::show_menu(GdkEventButton *ev)
{
    _menu.reset(new Gtk::Menu());
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(module, &JackPortModule::menu_addPort)));
    items.push_back(Gtk::Menu_Helpers::MenuElem("Rename Port", sigc::mem_fun(this, &JackPortPort::menu_renamePort)));
    items.push_back(Gtk::Menu_Helpers::SeparatorElem());
    items.push_back(Gtk::Menu_Helpers::MenuElem("Remove Port", sigc::mem_fun(this, &JackPortPort::menu_removePort)));

    _menu->popup(ev->button, ev->time);
    return true;
}//show_menu

void JackPortPort::menu_renamePort()
{
    module->setCurNamingPorts();

    Gtk::Entry *entry = nullptr;
    uiXml->get_widget("portNameEntry", entry);
    entry->set_text(title);

    Gtk::Label *label = nullptr;
    uiXml->get_widget("portNameDialogTitle", label);
    label->set_text("Rename Port");

    Gtk::Button *button = nullptr;
    uiXml->get_widget("portNameDialogOKButton", button);
    button->set_sensitive(false);

    Gtk::Dialog *dialog = nullptr;
    uiXml->get_widget("portNameDialog", dialog);

    int result = dialog->run();

    if (result == 1) { //OK
        std::string portName = entry->get_text();
        if (portName.empty() == false) {
            std::vector<std::string>::iterator oldIter = std::find(curNamingPorts->begin(), curNamingPorts->end(), title);
            if (oldIter != curNamingPorts->end()) {
                curNamingPorts->erase(oldIter);
            }//if

            curNamingPorts->push_back(portName);
            title = portName;
            set_name(portName);

            module->doresize();
        }//if
    }//if

    dialog->hide();
}//renamePort

void JackPortPort::menu_removePort()
{
    module->removePort(title);
}//removePort

std::string JackPortPort::getTitle() const
{
    return title;
}//getTitle;

//////////////////////////////

EntryPort::EntryPort(EntryModule *module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : FlowCanvas::Port(*module_, title_, isInput_, colour)
{
    uiXml = uiXml_;
    entryModule = module_;
    title = title_;
    isThisInput = isInput_;
}//constructor

EntryPort::~EntryPort()
{
    //Nothing
}//destructor

bool EntryPort::isInput() const
{
    return isThisInput;
}//isInput

/////////////////////////

void setUpFlowCanvas(Glib::RefPtr<Gtk::Builder> uiXml)
{
    static bool replacedPlaceholder = false;

    if (false == replacedPlaceholder) {        
        ::flowCanvas = new JackPortFlowCanvas();

        Gtk::Label *label = nullptr;
        uiXml->get_widget("flowCanvasPlaceholderLabel", label);

        Gtk::Container *parent = label->get_parent();
        parent->remove(*label);

        parent->add(*flowCanvas);
    } else {
        Gtk::Container *parent = ::flowCanvas->get_parent();
        parent->remove(*flowCanvas);

        ::flowCanvas = new JackPortFlowCanvas();

        parent->add(*flowCanvas);
    }//if

    flowCanvas->clearModules();

    JackSingleton &jackSingleton = JackSingleton::Instance();
    std::vector<std::string> inputPorts = jackSingleton.getInputPorts();
    std::vector<std::string> outputPorts = jackSingleton.getOutputPorts();

    jackInputModule = new JackPortModule(::flowCanvas, "Jack Input", 0, 0, true, uiXml, inputPorts);
    jackInputModule->set_stacked_border(true);

    jackOutputModule = new JackPortModule(::flowCanvas, "Jack Output", 0, 0, false, uiXml, outputPorts);
    jackOutputModule->set_stacked_border(true);

    std::map<std::string, JackPortPort *> inputPortNameMap;
    std::map<std::string, JackPortPort *> outputPortNameMap;

    for (std::string portName : inputPorts) {
        JackPortPort *port = new JackPortPort(jackInputModule, portName, true, 0x953c02ff, uiXml);
        jackInputModule->do_add_port(port);
        inputPortNameMap[portName] = port;
    }//foreach

    for (std::string portName : outputPorts) {
        JackPortPort *port = new JackPortPort(jackOutputModule, portName, false, 0x027055ff, uiXml);
        jackOutputModule->do_add_port(port);
        outputPortNameMap[portName] = port;
    }//foreach

    jackInputModule->doresize();
    jackOutputModule->doresize();

    ::flowCanvas->addModule(jackInputModule);
    ::flowCanvas->addModule(jackOutputModule);

    jackInputModule->restorePosition();
    jackOutputModule->restorePosition();

    Globals &globals = Globals::Instance();

    entryModules.clear();
    auto entryPair = globals.projectData.getSequencer()->getEntryPair();

    std::cout << "setUpFlowCanvas: " << std::distance(entryPair.first, entryPair.second) << " - " << globals.projectData.getSequencer().get() << std::endl;

    for (auto entryMapIter : entryPair) {
        EntryModule *entryModule = new EntryModule(::flowCanvas, entryMapIter->getTitle(), 0, 0, uiXml, entryMapIter);

        EntryPort *inputPort = new EntryPort(entryModule, "Input", true, 0x953c02ff, uiXml);
        entryModule->do_add_port(inputPort);

        EntryPort *outputPort = new EntryPort(entryModule, "Output", false, 0x027055ff, uiXml);
        entryModule->do_add_port(outputPort);

        entryModule->doresize();
        ::flowCanvas->addModule(entryModule);
        entryModule->restorePosition();

        entryModules.push_back(entryModule);

        //Set up connections for this module
        std::set<jack_port_t *> jackInputMap = entryMapIter->getInputPorts();
        std::set<jack_port_t *> jackOutputMap = entryMapIter->getOutputPorts();

        for (jack_port_t *jackPort : jackInputMap) {
            std::string portName = jackSingleton.getInputPortName(jackPort);
            assert(portName.empty() == false);

            JackPortPort *jackPortPort = inputPortNameMap[portName];
            flowCanvas->connect(jackPortPort, inputPort);
        }//foreach

        for (jack_port_t *jackPort : jackOutputMap) {
            std::string portName = jackSingleton.getOutputPortName(jackPort);
            assert(portName.empty() == false);

            JackPortPort *jackPortPort = outputPortNameMap[portName];
            flowCanvas->connect(jackPortPort, outputPort);
        }//foreach
    }//for

    ::flowCanvas->show_all();

    if (false == replacedPlaceholder) {        
        std::cout << "HERE ^^^^^^^^^^^^" << std::endl;

        Gtk::ScrolledWindow *scrolledWindow;
        uiXml->get_widget("jackPortScrolledWindow", scrolledWindow);

        assert(scrolledWindow != nullptr);

        if (scrolledWindow->get_hscrollbar() != nullptr) { //some gtkmm versions return null with get_[hv]scrollbar()
            scrolledWindow->get_hscrollbar()->set_range(0, 3000);
            scrolledWindow->get_vscrollbar()->set_range(0, 3000);

            scrolledWindow->get_hscrollbar()->set_value(1200);
            scrolledWindow->get_vscrollbar()->set_value(1300);        
        } else {
            std::cout << "Using buggy gtkmm" << std::endl;
        }//if
    }//if

    if (false == replacedPlaceholder) {        
        replacedPlaceholder = true;
    }//if
}//setUpFlowCanvas

}//anonymous namespace

JackPortDialog::JackPortDialog(Glib::RefPtr<Gtk::Builder> uiXml)
{
    Gtk::Dialog *portsDialog = nullptr;
    uiXml->get_widget("jackPortDialog", portsDialog);

    setUpFlowCanvas(uiXml);

    int result = portsDialog->run();

    if (result == 0) { //OK
        std::vector<std::string> inputPorts = jackInputModule->getPorts();
        std::vector<std::string> outputPorts = jackOutputModule->getPorts();

        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setInputPorts(inputPorts);
        jackSingleton.setOutputPorts(outputPorts);

        //Store port connections on sequencer entries
        std::map<EntryModule *, std::shared_ptr<std::set<jack_port_t *> > > entryInputMap;
        std::map<EntryModule *, std::shared_ptr<std::set<jack_port_t *> > > entryOutputMap;

        for (auto connection : flowCanvas->connections()) {
            FlowCanvas::Connectable *sourceConnection = connection->source();
            FlowCanvas::Connectable *destConnection = connection->dest();

            if ((sourceConnection == nullptr) || (destConnection == nullptr)) {
                continue;
            }//if

            EntryPort *entryPort = dynamic_cast<EntryPort *>(sourceConnection);
            JackPortPort *jackPort = dynamic_cast<JackPortPort *>(destConnection);
            if (entryPort == nullptr) {
                entryPort = dynamic_cast<EntryPort *>(destConnection);
                jackPort = dynamic_cast<JackPortPort *>(sourceConnection);
            }//if

            assert((entryPort != nullptr) && (jackPort != nullptr));

            EntryModule *entryModule = dynamic_cast<EntryModule*>(entryPort->module());
            assert(entryModule != nullptr);

            if (entryInputMap.find(entryModule) == entryInputMap.end()) {
                entryInputMap[entryModule] = std::shared_ptr<std::set<jack_port_t *> >(new std::set<jack_port_t *>());
            }//if

            if (entryOutputMap.find(entryModule) == entryOutputMap.end()) {
                entryOutputMap[entryModule] = std::shared_ptr<std::set<jack_port_t *> >(new std::set<jack_port_t *>());
            }//if

            std::string portName = jackPort->getTitle();

            if (entryPort->isInput() == true) {
                std::cout << "Module " << entryModule->name() << " has input to " << portName << std::endl;

                jack_port_t *jackPortPtr = jackSingleton.getInputPort(portName);
                assert(jackPortPtr != nullptr);

                entryInputMap[entryModule]->insert(jackPortPtr);
            } else {
                std::cout << "Module " << entryModule->name() << " has output to " << portName << std::endl;

                jack_port_t *jackPortPtr = jackSingleton.getOutputPort(portName);
                assert(jackPortPtr != nullptr);

                entryOutputMap[entryModule]->insert(jackPortPtr);
            }//if
        }//foreach

        for (auto inputMapIter : entryInputMap) {
            inputMapIter.first->getEntry()->setInputPorts(*inputMapIter.second);
        }//for

        for (auto outputMapIter : entryOutputMap) {
            outputMapIter.first->getEntry()->setOutputPorts(*outputMapIter.second);
        }//for
    }//if

    portsDialog->hide();
}//constructor

JackPortDialog::~JackPortDialog()
{
    //Nothing
}//destructor


