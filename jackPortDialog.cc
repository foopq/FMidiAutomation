#include <iostream>
#include "jackPortDialog.h"
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Ellipse.hpp>
#include <flowcanvas/Module.hpp>
#include <flowcanvas/Port.hpp>
#include "jack.h"
#include "FMidiAutomationMainWindow.h"
#include "Sequencer.h"
#include <boost/foreach.hpp>
#include <set>
#include <limits>
#include <math.h>

namespace
{
class JackPortFlowCanvas;
class JackPortModule;
class EntryModule;
class EntryPort;

boost::shared_ptr<JackPortFlowCanvas> flowCanvas;
std::vector<std::string> *curNamingPorts = NULL;
boost::shared_ptr<JackPortModule> jackInputModule;
boost::shared_ptr<JackPortModule> jackOutputModule;
std::vector<boost::shared_ptr<EntryModule> > entryModules;

struct CanvasPositions
{
    std::pair<double, double> jackInputPosition;
    std::pair<double, double> jackOutputPosition;
    std::map<boost::weak_ptr<SequencerEntry> , std::pair<unsigned int, unsigned int> > entryPositions;

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
    ~JackPortFlowCanvas();

    void addModule(boost::shared_ptr<FlowCanvas::Module> module);
    void clearModules();

    virtual void connect(boost::shared_ptr<FlowCanvas::Connectable> c1, boost::shared_ptr<FlowCanvas::Connectable> c2);
    virtual void disconnect(boost::shared_ptr<FlowCanvas::Connectable> c1, boost::shared_ptr<FlowCanvas::Connectable> c2);

private:
    std::set<boost::shared_ptr<FlowCanvas::Module> > modules;
};//JackPortFlowCanvas

class JackPortModule : public FlowCanvas::Module
{
public:
	JackPortModule(boost::shared_ptr<FlowCanvas::Canvas> canvas, const std::string& title, double x, double y, bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_);
    ~JackPortModule();

    void menu_addPort();
    void removePort(const std::string &name);
    virtual void move(double dx, double dy);
    bool checkPortName(GdkEventKey *event);
    void setCurNamingPorts();
    void restorePosition();

    std::vector<std::string> getPorts();

private:    
	void create_menu();

    bool inputs;
    Glib::RefPtr<Gtk::Builder> uiXml;
    std::vector<std::string> ports;
};//JackPortModule

class EntryModule : public FlowCanvas::Module
{
public:
	EntryModule(boost::shared_ptr<FlowCanvas::Canvas> canvas, const std::string& title, double x, double y, Glib::RefPtr<Gtk::Builder> uiXml_, boost::shared_ptr<SequencerEntry> entry_);
    ~EntryModule();

    virtual void move(double dx, double dy);
    void restorePosition();

    void addPortConnection(boost::shared_ptr<EntryPort> entryPort, const std::string &jackTitle);
    void removePortConnection(boost::shared_ptr<EntryPort> entryPort, const std::string &jackTitle);

    boost::shared_ptr<SequencerEntry> getEntry() const;
private:    
    boost::shared_ptr<SequencerEntry> entry;
    Glib::RefPtr<Gtk::Builder> uiXml;
    std::set<std::string> inputConnections;
    std::set<std::string> outputConnections;
};//EntryModule

class JackPortPort : public FlowCanvas::Port
{
public:
	JackPortPort(boost::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~JackPortPort();

    std::string getTitle() const;

private:    
	void create_menu();
    void menu_renamePort();
    void menu_removePort();

    boost::shared_ptr<JackPortModule> module;
    std::string title;
    Glib::RefPtr<Gtk::Builder> uiXml;
    bool isInput;
};//JackPortPort

class EntryPort : public FlowCanvas::Port
{
public:
	EntryPort(boost::shared_ptr<EntryModule> module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~EntryPort();

    bool isInput() const;

private:    
    boost::shared_ptr<EntryModule> entryModule;
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

void JackPortFlowCanvas::addModule(boost::shared_ptr<FlowCanvas::Module> module)
{
    modules.insert(module);
    this->add_item(module);
}//addModule

void JackPortFlowCanvas::clearModules()
{
    BOOST_FOREACH (boost::shared_ptr<FlowCanvas::Module> item, modules) {
        remove_item(item);
    }//foreach

    _items.clear();

    modules.clear();
}//clearModules

void JackPortFlowCanvas::disconnect(boost::shared_ptr<FlowCanvas::Connectable> c1, boost::shared_ptr<FlowCanvas::Connectable> c2)
{
    boost::shared_ptr<JackPortPort> jackPort;
    boost::shared_ptr<EntryPort> entryPort;
    boost::shared_ptr<EntryModule> entryModule;    

    jackPort = boost::dynamic_pointer_cast<JackPortPort>(c1);
    entryPort = boost::dynamic_pointer_cast<EntryPort>(c2);
    if (jackPort == NULL) {
        jackPort = boost::dynamic_pointer_cast<JackPortPort>(c2);
        if (jackPort == NULL) {
            return;
        }//if
        entryPort = boost::dynamic_pointer_cast<EntryPort>(c1);
    }//if

    if (entryPort == NULL) {
        return;
    }//if

    BOOST_FOREACH (boost::shared_ptr<EntryModule> curEntryModule, entryModules) {
        BOOST_FOREACH (boost::shared_ptr<FlowCanvas::Port> port, curEntryModule->ports()) {
            if (port == entryPort) {
                entryModule = curEntryModule;
                break;
            }//if
        }//foreach

        if (entryModule != NULL) {
            break;
        }//if
    }//forach

    if (entryModule == NULL) {
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

void JackPortFlowCanvas::connect(boost::shared_ptr<FlowCanvas::Connectable> c1, boost::shared_ptr<FlowCanvas::Connectable> c2)
{
    boost::shared_ptr<JackPortPort> jackPort;
    boost::shared_ptr<EntryPort> entryPort;
    boost::shared_ptr<EntryModule> entryModule;    

    jackPort = boost::dynamic_pointer_cast<JackPortPort>(c1);
    entryPort = boost::dynamic_pointer_cast<EntryPort>(c2);
    if (jackPort == NULL) {
        jackPort = boost::dynamic_pointer_cast<JackPortPort>(c2);
        if (jackPort == NULL) {
            return;
        }//if
        entryPort = boost::dynamic_pointer_cast<EntryPort>(c1);
    }//if

    if (entryPort == NULL) {
        return;
    }//if

    BOOST_FOREACH (boost::shared_ptr<EntryModule> curEntryModule, entryModules) {
        BOOST_FOREACH (boost::shared_ptr<FlowCanvas::Port> port, curEntryModule->ports()) {
            if (port == entryPort) {
                entryModule = curEntryModule;
                break;
            }//if
        }//foreach

        if (entryModule != NULL) {
            break;
        }//if
    }//forach

    if (entryModule == NULL) {
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

JackPortModule::JackPortModule(boost::shared_ptr<FlowCanvas::Canvas> canvas, const std::string& title, double x, double y, bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_)
                : FlowCanvas::Module(canvas, title, x, y, true, true)
{
    uiXml = uiXml_;
    inputs = inputs_;
    ports = ports_;

    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &JackPortModule::checkPortName));

    create_menu();
}//constructor

JackPortModule::~JackPortModule()
{
    //Nothing
}//destructor

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

    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);
    entry->set_text("");

    Gtk::Label *label;
    uiXml->get_widget("portNameDialogTitle", label);
    label->set_text("Add Port");

    Gtk::Button *button;
    uiXml->get_widget("portNameDialogOKButton", button);
    button->set_sensitive(false);

    Gtk::Dialog *dialog;
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

            boost::shared_ptr<JackPortModule> thisModule = boost::dynamic_pointer_cast<JackPortModule>(shared_from_this());
            boost::shared_ptr<JackPortPort> port(new JackPortPort(thisModule, portName, inputs, colour, uiXml));

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

    boost::shared_ptr<FlowCanvas::Port> port = get_port(title);

    while (port->connections().empty() == false) {
        boost::shared_ptr<FlowCanvas::Connection> connection = (*port->connections().begin()).lock();
        flowCanvas->remove_connection(connection->source().lock(), connection->dest().lock());
        port->remove_connection(connection);
    }//foreach

    if (port != NULL) {
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
    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);

    Gtk::Button *button;
    uiXml->get_widget("portNameDialogOKButton", button);

    std::string portName = entry->get_text();

    if ((portName.empty() == true) || (std::find(curNamingPorts->begin(), curNamingPorts->end(), portName) != curNamingPorts->end())) {
        button->set_sensitive(false);
    } else {
        button->set_sensitive(true);
    }//if

    return true;
}//checkPortName

void JackPortModule::create_menu()
{
    _menu = new Gtk::Menu();
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(this, &JackPortModule::menu_addPort)));
}//create_menu

//////////////////////////////

EntryModule::EntryModule(boost::shared_ptr<FlowCanvas::Canvas> canvas, const std::string& title, double x, double y, Glib::RefPtr<Gtk::Builder> uiXml_, boost::shared_ptr<SequencerEntry> entry_)
                            : FlowCanvas::Module(canvas, title, x, y, true, true)
{
    uiXml = uiXml_;
    entry = entry_;
}//constructor

EntryModule::~EntryModule()
{
    //Nothing
}//destructor

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

    const float minDistance = 80;
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

    for (std::map<boost::weak_ptr<SequencerEntry>, std::pair<unsigned int, unsigned int> >::const_iterator entryIter = canvasPositions.entryPositions.begin(); entryIter != canvasPositions.entryPositions.end(); ++entryIter) {
        dist = sqrt( (posx - entryIter->second.first) * (posx - entryIter->second.first) + 
                     (posy - entryIter->second.second) * (posy - entryIter->second.second) );

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
    if (canvasPositions.entryPositions.find(entry) != canvasPositions.entryPositions.end()) {
        std::pair<unsigned int, unsigned int> lastPos = canvasPositions.entryPositions[entry];
        FlowCanvas::Module::move(lastPos.first, lastPos.second);

        std::cout << "entry1: " << lastPos.first << " - " << lastPos.second << std::endl;
    } else {
        unsigned int posx = 1400;
        unsigned int posy = 1500;

        readjustPosition(posx, posy);

        canvasPositions.entryPositions[entry] = std::make_pair(posx, posy);

        FlowCanvas::Module::move(posx, posy);

        std::cout << "entry2: " << posx << " - " << posy << std::endl;
    }//if
}//restorePosition

void EntryModule::move(double dx, double dy) {
    FlowCanvas::Module::move(dx, dy);

    if (canvasPositions.entryPositions.find(entry) != canvasPositions.entryPositions.end()) {
        std::pair<unsigned int, unsigned int> lastPos = canvasPositions.entryPositions[entry];
        canvasPositions.entryPositions[entry] = std::make_pair(lastPos.first + dx, lastPos.second + dy);
    }//if
}//move

boost::shared_ptr<SequencerEntry> EntryModule::getEntry() const
{
    return entry;
}//getEntry

void EntryModule::addPortConnection(boost::shared_ptr<EntryPort> entryPort, const std::string &jackPortTitle)
{
    if (entryPort->isInput() == true) {
        inputConnections.insert(jackPortTitle);
    } else {
        outputConnections.insert(jackPortTitle);
    }//if
}//addPortConnection

void EntryModule::removePortConnection(boost::shared_ptr<EntryPort> entryPort, const std::string &jackPortTitle)
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

JackPortPort::JackPortPort(boost::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : FlowCanvas::Port(module_, title_, !isInput_, colour)
{
    uiXml = uiXml_;
    module = module_;
    title = title_;
    isInput = isInput_;

    create_menu();
}//constructor

JackPortPort::~JackPortPort()
{
    //Nothing
}//destructor

void JackPortPort::create_menu()
{
    _menu = new Gtk::Menu();
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(module.get(), &JackPortModule::menu_addPort)));
    items.push_back(Gtk::Menu_Helpers::MenuElem("Rename Port", sigc::mem_fun(this, &JackPortPort::menu_renamePort)));
    items.push_back(Gtk::Menu_Helpers::SeparatorElem());
    items.push_back(Gtk::Menu_Helpers::MenuElem("Remove Port", sigc::mem_fun(this, &JackPortPort::menu_removePort)));
}//create_menu

void JackPortPort::menu_renamePort()
{
    module->setCurNamingPorts();

    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);
    entry->set_text(title);

    Gtk::Label *label;
    uiXml->get_widget("portNameDialogTitle", label);
    label->set_text("Rename Port");

    Gtk::Button *button;
    uiXml->get_widget("portNameDialogOKButton", button);
    button->set_sensitive(false);

    Gtk::Dialog *dialog;
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

            module->resize();
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

EntryPort::EntryPort(boost::shared_ptr<EntryModule> module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : FlowCanvas::Port(module_, title_, isInput_, colour)
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
        replacedPlaceholder = true;

        ::flowCanvas.reset(new JackPortFlowCanvas());

        Gtk::Label *label;
        uiXml->get_widget("flowCanvasPlaceholderLabel", label);

        Gtk::Container *parent = label->get_parent();
        parent->remove(*label);

        parent->add(*flowCanvas);

        Gtk::ScrolledWindow *scrolledWindow;
        uiXml->get_widget("jackPortScrolledWindow", scrolledWindow);
        scrolledWindow->get_hscrollbar()->set_range(0, 3000);
        scrolledWindow->get_vscrollbar()->set_range(0, 3000);

        scrolledWindow->get_hscrollbar()->set_value(1200);
        scrolledWindow->get_vscrollbar()->set_value(1300);
    } else {
        Gtk::Container *parent = ::flowCanvas->get_parent();
        parent->remove(*flowCanvas);

        ::flowCanvas.reset(new JackPortFlowCanvas());

        parent->add(*flowCanvas);
    }//if

    flowCanvas->clearModules();

    JackSingleton &jackSingleton = JackSingleton::Instance();
    std::vector<std::string> inputPorts = jackSingleton.getInputPorts();
    std::vector<std::string> outputPorts = jackSingleton.getOutputPorts();

    jackInputModule.reset(new JackPortModule(::flowCanvas, "Jack Input", 0, 0, true, uiXml, inputPorts));
    jackInputModule->set_stacked_border(true);

    jackOutputModule.reset(new JackPortModule(::flowCanvas, "Jack Output", 0, 0, false, uiXml, outputPorts));
    jackOutputModule->set_stacked_border(true);

    std::map<std::string, boost::shared_ptr<JackPortPort> > inputPortNameMap;
    std::map<std::string, boost::shared_ptr<JackPortPort> > outputPortNameMap;

    BOOST_FOREACH (std::string portName, inputPorts) {
        boost::shared_ptr<JackPortPort> port(new JackPortPort(jackInputModule, portName.c_str(), true, 0x953c02ff, uiXml));
        jackInputModule->add_port(port);
        inputPortNameMap[portName] = port;
    }//foreach

    BOOST_FOREACH (std::string portName, outputPorts) {
        boost::shared_ptr<JackPortPort> port(new JackPortPort(jackOutputModule, portName.c_str(), false, 0x027055ff, uiXml));
        jackOutputModule->add_port(port);
        outputPortNameMap[portName] = port;
    }//foreach

    jackInputModule->resize();
    jackOutputModule->resize();

    ::flowCanvas->addModule(jackInputModule);
    ::flowCanvas->addModule(jackOutputModule);

    jackInputModule->restorePosition();
    jackOutputModule->restorePosition();

    Globals &globals = Globals::Instance();

    entryModules.clear();
    std::pair<std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator, std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator> entryPair = globals.sequencer->getEntryPair();
    for (std::map<boost::shared_ptr<SequencerEntry>, int >::const_iterator entryMapIter = entryPair.first; entryMapIter != entryPair.second; ++entryMapIter) {
        boost::shared_ptr<EntryModule> entryModule(new EntryModule(::flowCanvas, entryMapIter->first->getTitle().c_str(), 0, 0, uiXml, entryMapIter->first));

        boost::shared_ptr<EntryPort> inputPort(new EntryPort(entryModule, "Input", true, 0x953c02ff, uiXml));
        entryModule->add_port(inputPort);

        boost::shared_ptr<EntryPort> outputPort(new EntryPort(entryModule, "Output", false, 0x027055ff, uiXml));
        entryModule->add_port(outputPort);

        entryModule->resize();
        ::flowCanvas->addModule(entryModule);
        entryModule->restorePosition();

        entryModules.push_back(entryModule);

        //Set up connections for this module
        std::set<jack_port_t *> jackInputMap = entryMapIter->first->getInputPorts();
        std::set<jack_port_t *> jackOutputMap = entryMapIter->first->getOutputPorts();

        BOOST_FOREACH(jack_port_t *jackPort, jackInputMap) {
            std::string portName = jackSingleton.getInputPortName(jackPort);
            assert(portName.empty() == false);

            boost::shared_ptr<JackPortPort> jackPort = inputPortNameMap[portName];
            flowCanvas->connect(jackPort, inputPort);
        }//foreach

        BOOST_FOREACH(jack_port_t *jackPort, jackOutputMap) {
            std::string portName = jackSingleton.getOutputPortName(jackPort);
            assert(portName.empty() == false);

            boost::shared_ptr<JackPortPort> jackPort = outputPortNameMap[portName];
            flowCanvas->connect(jackPort, outputPort);
        }//foreach
    }//for

    ::flowCanvas->show_all();
}//setUpFlowCanvas

}//anonymous namespace

JackPortDialog::JackPortDialog(Glib::RefPtr<Gtk::Builder> uiXml)
{
    Gtk::Dialog *portsDialog;
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
        std::map<boost::shared_ptr<EntryModule>, boost::shared_ptr<std::set<jack_port_t *> > > entryInputMap;
        std::map<boost::shared_ptr<EntryModule>, boost::shared_ptr<std::set<jack_port_t *> > > entryOutputMap;

        BOOST_FOREACH (boost::shared_ptr<FlowCanvas::Connection> connection, flowCanvas->connections()) {
            boost::shared_ptr<FlowCanvas::Connectable> sourceConnection = connection->source().lock();
            boost::shared_ptr<FlowCanvas::Connectable> destConnection = connection->dest().lock();

            if ((sourceConnection == NULL) || (destConnection == NULL)) {
                continue;
            }//if

            boost::shared_ptr<EntryPort> entryPort = boost::dynamic_pointer_cast<EntryPort>(sourceConnection);
            boost::shared_ptr<JackPortPort> jackPort = boost::dynamic_pointer_cast<JackPortPort>(destConnection);
            if (entryPort == NULL) {
                entryPort = boost::dynamic_pointer_cast<EntryPort>(destConnection);
                jackPort = boost::dynamic_pointer_cast<JackPortPort>(sourceConnection);
            }//if

            assert((entryPort != NULL) && (jackPort != NULL));

            boost::shared_ptr<EntryModule> entryModule = boost::dynamic_pointer_cast<EntryModule>(entryPort->module().lock());
            assert(entryModule != NULL);

            if (entryInputMap.find(entryModule) == entryInputMap.end()) {
                entryInputMap[entryModule] = boost::shared_ptr<std::set<jack_port_t *> >(new std::set<jack_port_t *>());
            }//if

            if (entryOutputMap.find(entryModule) == entryOutputMap.end()) {
                entryOutputMap[entryModule] = boost::shared_ptr<std::set<jack_port_t *> >(new std::set<jack_port_t *>());
            }//if

            std::string portName = jackPort->getTitle();

            if (entryPort->isInput() == true) {
                std::cout << "Module " << entryModule->name() << " has input to " << portName << std::endl;

                jack_port_t *jackPortPtr = jackSingleton.getOutputPort(portName);
                assert(jackPortPtr != NULL);

                entryInputMap[entryModule]->insert(jackPortPtr);
            } else {
                std::cout << "Module " << entryModule->name() << " has output to " << portName << std::endl;

                jack_port_t *jackPortPtr = jackSingleton.getInputPort(portName);
                assert(jackPortPtr != NULL);

                entryOutputMap[entryModule]->insert(jackPortPtr);
            }//if
        }//foreach

        for (std::map<boost::shared_ptr<EntryModule>, boost::shared_ptr<std::set<jack_port_t *> > >::const_iterator inputMapIter = entryInputMap.begin(); 
                inputMapIter != entryInputMap.end(); ++inputMapIter) {
            inputMapIter->first->getEntry()->setInputPorts(*inputMapIter->second);
        }//for

        for (std::map<boost::shared_ptr<EntryModule>, boost::shared_ptr<std::set<jack_port_t *> > >::const_iterator outputMapIter = entryOutputMap.begin();
                outputMapIter != entryOutputMap.end(); ++outputMapIter) {
            outputMapIter->first->getEntry()->setOutputPorts(*outputMapIter->second);
        }//for
    }//if

    portsDialog->hide();
}//constructor

JackPortDialog::~JackPortDialog()
{
    //Nothing
}//destructor

