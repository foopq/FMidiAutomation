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

namespace
{
class JackPortFlowCanvas;
class JackPortModule;
boost::shared_ptr<JackPortFlowCanvas> flowCanvas;
std::vector<std::string> *curNamingPorts = NULL;
boost::shared_ptr<JackPortModule> jackInputModule;
boost::shared_ptr<JackPortModule> jackOutputModule;

struct CanvasPositions
{
    std::pair<double, double> jackInputPosition;
    std::pair<double, double> jackOutputPosition;
//    std::map<SequencerEntry *, std::pair<unsigned int, unsigned int> > entryPositions;

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

class JackPortPort : public FlowCanvas::Port
{
public:
	JackPortPort(boost::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~JackPortPort();

private:    
	void create_menu();
    void menu_renamePort();
    void menu_removePort();

    boost::shared_ptr<JackPortModule> module;
    std::string title;
    Glib::RefPtr<Gtk::Builder> uiXml;
    bool isInput;
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
            FlowCanvas::Module::move(1550, 1500);
            canvasPositions.jackInputPosition.first = 1550;
            canvasPositions.jackInputPosition.second = 1500;
        } else {
            FlowCanvas::Module::move(canvasPositions.jackInputPosition.first, canvasPositions.jackInputPosition.second);
        }//if
    } else {
        if (canvasPositions.jackOutputPosition.first < 1) {
            FlowCanvas::Module::move(1250, 1500);
            canvasPositions.jackOutputPosition.first = 1250;
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
            if (false == inputs) {
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

JackPortPort::JackPortPort(boost::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput_, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : FlowCanvas::Port(module_, title_, isInput_, colour)
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

    BOOST_FOREACH (std::string portName, inputPorts) {
        boost::shared_ptr<JackPortPort> port(new JackPortPort(jackInputModule, portName.c_str(), true, 0x027055ff, uiXml));
        jackInputModule->add_port(port);
    }//foreach

    BOOST_FOREACH (std::string portName, outputPorts) {
        boost::shared_ptr<JackPortPort> port(new JackPortPort(jackOutputModule, portName.c_str(), false, 0x953c02ff, uiXml));
        jackOutputModule->add_port(port);
    }//foreach

    jackInputModule->resize();
    jackOutputModule->resize();

    ::flowCanvas->addModule(jackInputModule);
    ::flowCanvas->addModule(jackOutputModule);

    jackInputModule->restorePosition();
    jackOutputModule->restorePosition();

    Globals &globals = Globals::Instance();
    unsigned int numEntries = globals.sequencer->getNumEntries();


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
    }//if

    portsDialog->hide();
}//constructor

JackPortDialog::~JackPortDialog()
{
    //Nothing
}//destructor

