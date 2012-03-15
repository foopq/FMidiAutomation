/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#include <gtk/gtk.h>
#include <iostream>
#include "jackPortDialog.h"
#include "jack.h"
#include "Data/Sequencer.h"
#include "Data/SequencerEntry.h"
#include <set>
#include <limits>
#include <math.h>
#include "Globals.h"




namespace
{
std::shared_ptr<JackPortFlowCanvas> flowCanvas;
std::vector<std::string> *curNamingPorts = nullptr;
std::shared_ptr<JackPortModule> jackInputModule;
std::shared_ptr<JackPortModule> jackOutputModule;
std::vector<std::shared_ptr<EntryModule>> entryModules;

static CanvasPositions canvasPositions;

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

void setUpFlowCanvas(Glib::RefPtr<Gtk::Builder> uiXml)
{
    static bool replacedPlaceholder = false;

    ::flowCanvas.reset(new JackPortFlowCanvas());
    ::flowCanvas->clearModules();

    JackSingleton &jackSingleton = JackSingleton::Instance();
    std::vector<std::string> inputPorts = jackSingleton.getInputPorts();
    std::vector<std::string> outputPorts = jackSingleton.getOutputPorts();

    jackInputModule.reset(new JackPortModule(::flowCanvas, "Jack Input", 0, 0, true, uiXml, inputPorts));
    jackOutputModule.reset(new JackPortModule(::flowCanvas, "Jack Output", 0, 0, false, uiXml, outputPorts));

    std::map<std::string, std::shared_ptr<JackPortPort>> inputPortNameMap;
    std::map<std::string, std::shared_ptr<JackPortPort>> outputPortNameMap;

    for (std::string portName : inputPorts) {
        std::shared_ptr<JackPortPort> port(new JackPortPort(jackInputModule, portName, true, 0x953c02ff, uiXml));
        jackInputModule->add_port(port);
        inputPortNameMap[portName] = port;
    }//foreach

    for (std::string portName : outputPorts) {
        std::shared_ptr<JackPortPort> port(new JackPortPort(jackOutputModule, portName, false, 0x027055ff, uiXml));
        jackOutputModule->add_port(port);
        outputPortNameMap[portName] = port;
    }//foreach

    //jackInputModule->doresize();
    //jackOutputModule->doresize();

    ::flowCanvas->addModule(jackInputModule);
    ::flowCanvas->addModule(jackOutputModule);

    jackInputModule->restorePosition();
    jackOutputModule->restorePosition();

    Globals &globals = Globals::Instance();

    entryModules.clear();
    auto entryPair = globals.projectData.getSequencer()->getEntryPair();

    std::cout << "setUpFlowCanvas: " << std::distance(entryPair.first, entryPair.second) << " - " << globals.projectData.getSequencer().get() << std::endl;

    for (auto entryMapIter : entryPair) {
        std::shared_ptr<EntryModule> entryModule(new EntryModule(::flowCanvas, entryMapIter->getTitle(), 0, 0, uiXml, entryMapIter));

        std::shared_ptr<EntryPort> inputPort(new EntryPort(entryModule, "Input", true, 0x953c02ff, uiXml));
        entryModule->do_add_port(inputPort);

        std::shared_ptr<EntryPort> outputPort(new EntryPort(entryModule, "Output", false, 0x027055ff, uiXml));
        entryModule->do_add_port(outputPort);

        //entryModule->doresize();
        ::flowCanvas->addModule(entryModule);
        entryModule->restorePosition();

        entryModules.push_back(entryModule);

        //Set up connections for this module
        std::set<jack_port_t *> jackInputMap = entryMapIter->getInputPorts();
        std::set<jack_port_t *> jackOutputMap = entryMapIter->getOutputPorts();

        for (jack_port_t *jackPort : jackInputMap) {
            std::string portName = jackSingleton.getInputPortName(jackPort);
            assert(portName.empty() == false);

            std::shared_ptr<JackPortPort> jackPortPort = inputPortNameMap[portName];
            flowCanvas->connect(jackPortPort, inputPort);
        }//foreach

        for (jack_port_t *jackPort : jackOutputMap) {
            std::string portName = jackSingleton.getOutputPortName(jackPort);
            assert(portName.empty() == false);

            std::shared_ptr<JackPortPort> jackPortPort = outputPortNameMap[portName];
            flowCanvas->connect(jackPortPort, outputPort);
        }//foreach
    }//for

    //::flowCanvas->show_all();

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

//////////////////////////////

JackPortFlowCanvas::JackPortFlowCanvas()
{
    //Nothing
}//constructor

JackPortFlowCanvas::~JackPortFlowCanvas()
{
    //Nothing
}//destructor

void JackPortFlowCanvas::addModule(std::shared_ptr<JackModuleBase> module)
{
    modules.insert(module);
}//addModule

void JackPortFlowCanvas::clearModules()
{
    modules.clear();
}//clearModules

void JackPortFlowCanvas::disconnect(std::shared_ptr<JackPortBase> c1, std::shared_ptr<JackPortBase> c2)
{
    std::shared_ptr<JackPortPort> jackPort;
    std::shared_ptr<EntryPort> entryPort;
    std::shared_ptr<EntryModule> entryModule;    

    jackPort = std::dynamic_pointer_cast<JackPortPort>(c1);
    entryPort = std::dynamic_pointer_cast<EntryPort>(c2);
    if (jackPort == nullptr) {
        jackPort = std::dynamic_pointer_cast<JackPortPort>(c2);
        if (jackPort == nullptr) {
            return;
        }//if
        entryPort = std::dynamic_pointer_cast<EntryPort>(c1);
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
        ////flowCanvas->remove_connection(jackPort, entryPort);
    } else {
        ////flowCanvas->remove_connection(entryPort, jackPort);
    }//if    
}//disconnect

void JackPortFlowCanvas::connect(std::shared_ptr<JackPortBase> c1, std::shared_ptr<JackPortBase> c2)
{
    std::shared_ptr<JackPortPort> jackPort;
    std::shared_ptr<EntryPort> entryPort;
    std::shared_ptr<EntryModule> entryModule;

    jackPort = std::dynamic_pointer_cast<JackPortPort>(c1);
    entryPort = std::dynamic_pointer_cast<EntryPort>(c2);
    if (jackPort == nullptr) {
        jackPort = std::dynamic_pointer_cast<JackPortPort>(c2);
        if (jackPort == nullptr) {
            return;
        }//if
        entryPort = std::dynamic_pointer_cast<EntryPort>(c1);
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
        ////flowCanvas->add_connection(jackPort, entryPort, colour);
    } else {
        unsigned int colour = 0x027055ff;
        ////flowCanvas->add_connection(entryPort, jackPort, colour);
    }//if
}//connect

fmaipair<decltype(JackPortFlowCanvas::connectionsList.begin()), decltype(JackPortFlowCanvas::connectionsList.end())> JackPortFlowCanvas::connections()
{
    return fmai_make_pair(connectionsList.begin(), connectionsList.end());
}//connections

//////////////////////////////

JackPortModule::JackPortModule(std::shared_ptr<JackPortFlowCanvas> canvas, const std::string& title, double x, double y, bool inputs_, 
                                Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_)
                //: FlowCanvas::Module(*canvas, title, x, y, true, true)
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

/*
void JackPortModule::doresize()
{
    resize();
}//doresize
*/

void JackPortModule::add_port(std::shared_ptr<JackPortBase> port)
{
    //add_port(port);
    portLookup[port->getTitle()] = port;
}//do_add_port

void JackPortModule::remove_port(std::shared_ptr<JackPortBase> port)
{
    auto portIter = portLookup.find(port->getTitle());
    if (portIter != portLookup.end()) {
        portLookup.erase(portIter);
    }//if
}//remove_port

std::shared_ptr<JackPortBase> JackPortModule::get_port(const std::string &title)
{
    auto portIter = portLookup.find(title);
    if (portIter != portLookup.end()) {
        return portIter->second;
    } else {
        return std::shared_ptr<JackPortBase>();
    }//if
}//get_port

std::vector<std::string> JackPortModule::getPorts()
{
    return ports;
}//getPorts

std::shared_ptr<JackPortBase> get_port(const std::string &title)
{
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return std::shared_ptr<JackPortBase>();
}//get_port

void JackPortModule::restorePosition()
{
    if (true == inputs) {
        if (canvasPositions.jackInputPosition.first < 1) {
            move(1250, 1500);
            canvasPositions.jackInputPosition.first = 1250;
            canvasPositions.jackInputPosition.second = 1500;
        } else {
            move(canvasPositions.jackInputPosition.first, canvasPositions.jackInputPosition.second);
        }//if
    } else {
        if (canvasPositions.jackOutputPosition.first < 1) {
            move(1550, 1500);
            canvasPositions.jackOutputPosition.first = 1550;
            canvasPositions.jackOutputPosition.second = 1500;
        } else {
            move(canvasPositions.jackOutputPosition.first, canvasPositions.jackOutputPosition.second);
        }//if
    }//if
}//restorePosition

void JackPortModule::move(double dx, double dy) {
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

            std::shared_ptr<JackPortPort> port(new JackPortPort(shared_from_this(), portName, inputs, colour, uiXml));

            this->add_port(port);
            //this->resize();
        }//if
    }//if

    dialog->hide();
}//menu_addPort

void JackPortModule::removePort(const std::string &title)
{
    if (ports.size() == 1) {
        return;
    }//if

    std::shared_ptr<JackPortBase> port = get_port(title);

    while (port->hasConnections() == false) {
        std::shared_ptr<JackConnection> connection = port->getFirstConnection();
        flowCanvas->disconnect(connection->source(), connection->dest());
        port->remove_connection(connection);
    }//foreach

    if (port != nullptr) {
        remove_port(port);
        //resize();
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
    /*
    _menu.reset(new Gtk::Menu());
    Gtk::Menu::MenuList& items = _menu->items();

    items.push_back(Gtk::Menu_Helpers::MenuElem("Add Port", sigc::mem_fun(this, &JackPortModule::menu_addPort)));
    _menu->popup(ev->button, ev->time);
    */

    Glib::ustring ui_info =
                            "<ui>"
                            "  <popup name='PopupMenu'>"
                            "    <menuitem action='AddPort'/>"
                            "  </popup>"
                            "</ui>";

    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("ContextMenu", "Context Menu"));
    m_refActionGroup->add(Gtk::Action::create("AddPort", "Add Port"), sigc::mem_fun(*this, &JackPortModule::menu_addPort));

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);

    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try {
        m_refUIManager->add_ui_from_string(ui_info);
    } catch(const Glib::Error& ex) {
        std::cerr << "building menus failed: " <<  ex.what();
    } 
    #else
    std::auto_ptr<Glib::Error> ex;
    m_refUIManager->add_ui_from_string(ui_info, ex);
    if(ex.get()) {
        std::cerr << "building menus failed: " <<  ex->what();
    }
    #endif //GLIBMM_EXCEPTIONS_ENABLED

    m_pMenuPopup = dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu"));
    if(m_pMenuPopup != nullptr) {
        m_pMenuPopup->show_all_children();
        m_pMenuPopup->popup(ev->button, ev->time);
    } else {
        g_warning("menu not found");
    }//if

    return true;
}//show_menu

//////////////////////////////

EntryModule::EntryModule(std::shared_ptr<JackPortFlowCanvas> canvas, const std::string& title, double x, double y, Glib::RefPtr<Gtk::Builder> uiXml_, std::shared_ptr<SequencerEntry> entry_)
                            //: FlowCanvas::Module(*canvas, title, x, y, true, true)
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

/*
void EntryModule::doresize()
{
    resize();
}//doresize
*/

std::vector<std::shared_ptr<JackPortBase>> EntryModule::ports()
{
    return portList;
}//ports

void EntryModule::do_add_port(std::shared_ptr<JackPortBase> port)
{
    //add_port(port);
}//do_add_port

void EntryModule::restorePosition()
{
    if (canvasPositions.entryPositions.find(entry.get()) != canvasPositions.entryPositions.end()) {
        std::pair<unsigned int, unsigned int> lastPos = canvasPositions.entryPositions[entry.get()];
        move(lastPos.first, lastPos.second);

        std::cout << "entry1: " << lastPos.first << " - " << lastPos.second << std::endl;
    } else {
        unsigned int posx = 1400;
        unsigned int posy = 1500;

        readjustPosition(posx, posy);

        canvasPositions.entryPositions[entry.get()] = std::make_pair(posx, posy);

        move(posx, posy);

        std::cout << "entry2: " << posx << " - " << posy << std::endl;
    }//if
}//restorePosition

void EntryModule::move(double dx, double dy) {
    move(dx, dy);

    if (canvasPositions.entryPositions.find(entry.get()) != canvasPositions.entryPositions.end()) {
        std::pair<unsigned int, unsigned int> lastPos = canvasPositions.entryPositions[entry.get()];
        canvasPositions.entryPositions[entry.get()] = std::make_pair(lastPos.first + dx, lastPos.second + dy);
    }//if
}//move

std::shared_ptr<SequencerEntry> EntryModule::getEntry() const
{
    return entry;
}//getEntry

void EntryModule::addPortConnection(std::shared_ptr<EntryPort> entryPort, const std::string &jackPortTitle)
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

void EntryModule::removePortConnection(std::shared_ptr<EntryPort> entryPort, const std::string &jackPortTitle)
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

JackPortPort::JackPortPort(std::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : JackPortBase(module_, title_, isInput_, colour)
{
    uiXml = uiXml_;
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
    Glib::ustring ui_info =
                            "<ui>"
                            "  <popup name='PopupMenu'>"
                            "    <menuitem action='AddPort'/>"
                            "    <menuitem action='RenamePort'/>"
                            "    <separator/>"
                            "    <menuitem action='RemovePort'/>"
                            "  </popup>"
                            "</ui>";

    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("ContextMenu", "Context Menu"));
    m_refActionGroup->add(Gtk::Action::create("AddPort", "Add Port"), sigc::mem_fun(*std::dynamic_pointer_cast<JackPortModule>(module()), &JackPortModule::menu_addPort));
    m_refActionGroup->add(Gtk::Action::create("RenamePort", "Rename Port"), sigc::mem_fun(*this, &JackPortPort::menu_renamePort));
    m_refActionGroup->add(Gtk::Action::create("RemovePort", "Remove Port"), sigc::mem_fun(*this, &JackPortPort::menu_removePort));

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);

    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try {
        m_refUIManager->add_ui_from_string(ui_info);
    } catch(const Glib::Error& ex) {
        std::cerr << "building menus failed: " <<  ex.what();
    } 
    #else
    std::auto_ptr<Glib::Error> ex;
    m_refUIManager->add_ui_from_string(ui_info, ex);
    if(ex.get()) {
        std::cerr << "building menus failed: " <<  ex->what();
    }
    #endif //GLIBMM_EXCEPTIONS_ENABLED

    m_pMenuPopup = dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu"));
    if(m_pMenuPopup != nullptr) {
        m_pMenuPopup->show_all_children();
        m_pMenuPopup->popup(ev->button, ev->time);
    } else {
        g_warning("menu not found");
    }//if

    return true;
}//show_menu

void JackPortPort::menu_renamePort()
{
    //module()->setCurNamingPorts();

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
            //set_name(portName);

            //module->doresize();
        }//if
    }//if

    dialog->hide();
}//renamePort

void JackPortPort::menu_removePort()
{
    //module()->removePort(title);
}//removePort

std::string JackPortPort::getTitle() const
{
    return title;
}//getTitle;

//////////////////////////////

EntryPort::EntryPort(std::shared_ptr<EntryModule> module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : JackPortBase(module_, title_, isInput_, colour)
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

JackPortDialog::JackPortDialog(Glib::RefPtr<Gtk::Builder> uiXml)
{
    Gtk::Dialog *portsDialog = nullptr;
    uiXml->get_widget("jackPortDialog", portsDialog);
    uiXml->get_widget("jackPortDialogDrawingArea", jackPortDialogDrawingArea);

    jackPortDialogDrawingArea->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
    jackPortDialogDrawingArea->signal_draw().connect ( sigc::mem_fun(*this, &JackPortDialog::updateGraph) );
    jackPortDialogDrawingArea->signal_button_press_event().connect ( sigc::mem_fun(*this, &JackPortDialog::mouseButtonPressed) );
    jackPortDialogDrawingArea->signal_button_release_event().connect ( sigc::mem_fun(*this, &JackPortDialog::mouseButtonReleased) );
    jackPortDialogDrawingArea->signal_motion_notify_event().connect ( sigc::mem_fun(*this, &JackPortDialog::mouseMoved) );

    setUpFlowCanvas(uiXml);

    int result = portsDialog->run();

    if (result == 0) { //OK
        std::vector<std::string> inputPorts = jackInputModule->getPorts();
        std::vector<std::string> outputPorts = jackOutputModule->getPorts();

        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.setInputPorts(inputPorts);
        jackSingleton.setOutputPorts(outputPorts);

        //Store port connections on sequencer entries
        std::map<std::shared_ptr<EntryModule>, std::shared_ptr<std::set<jack_port_t *> > > entryInputMap;
        std::map<std::shared_ptr<EntryModule>, std::shared_ptr<std::set<jack_port_t *> > > entryOutputMap;

        for (auto connection : flowCanvas->connections()) {
            std::shared_ptr<JackPortBase> sourceConnection = connection->source();
            std::shared_ptr<JackPortBase> destConnection = connection->dest();

            if ((sourceConnection == nullptr) || (destConnection == nullptr)) {
                continue;
            }//if

            std::shared_ptr<JackPortBase> entryPort = sourceConnection;
            std::shared_ptr<JackPortBase> jackPort = destConnection;
            if (entryPort == nullptr) {
                entryPort = destConnection;
                jackPort = sourceConnection;
            }//if

            assert((entryPort != nullptr) && (jackPort != nullptr));

            std::shared_ptr<EntryModule> entryModule = std::dynamic_pointer_cast<EntryModule>(entryPort->module());
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

/////////////////////////

JackModuleBase::JackModuleBase()
{
    //Nothing
}//constructor

JackModuleBase::~JackModuleBase()
{
    //Nothing
}//destructor

std::string JackModuleBase::name()
{
    return nameStr;
}//name

/////////////////////////

JackPortBase::JackPortBase(std::shared_ptr<JackModuleBase> module, const std::string &name, bool is_input_, uint32_t colour_)
{
    title = name;
    colour = colour_;
    is_input = is_input_;
    jackPortModule = module;
}//constructor

JackPortBase::~JackPortBase()
{
    //Nothing
}//destructor

bool JackPortBase::hasConnections()
{
    return !connections.empty();
}//hasConnections

std::shared_ptr<JackConnection> JackPortBase::getFirstConnection()
{
    if (hasConnections() == true) {
        return connections[0];
    } else {
        return std::shared_ptr<JackConnection>();
    }//if
}//getFirstConnection

void JackPortBase::remove_connection(std::shared_ptr<JackConnection> connection)
{
    auto foundIter = std::find(connections.begin(), connections.end(), connection);
    if (foundIter != connections.end()) {
        connections.erase(foundIter);
    }//if
}//remove_connection

std::shared_ptr<JackModuleBase> JackPortBase::module()
{
    return jackPortModule;
}//module

std::string JackPortBase::getTitle()
{
    return title;
}//getTitle

bool JackPortBase::isInput()
{
    return is_input;
}//isInput

/////////////////////////

JackConnection::JackConnection()
{
    //Nothing
}//constructor

JackConnection::~JackConnection()
{
    //Nothing
}//destructor

std::shared_ptr<JackPortBase> JackConnection::source()
{
    return sourcePort;
}//source

std::shared_ptr<JackPortBase> JackConnection::dest()
{
    return destPort;
}//dest


