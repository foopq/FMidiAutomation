#include <iostream>
#include "jackPortDialog.h"
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Ellipse.hpp>
#include <flowcanvas/Module.hpp>
#include <flowcanvas/Port.hpp>
#include "jack.h"
#include <boost/foreach.hpp>
#include <set>

namespace
{
class JackPortFlowCanvas;
static boost::shared_ptr<JackPortFlowCanvas> flowCanvas;

class JackPortFlowCanvas : public virtual FlowCanvas::Canvas
{
public:    
    JackPortFlowCanvas();
    ~JackPortFlowCanvas();

    void addModule(boost::shared_ptr<FlowCanvas::Module> module);

private:
    std::set<boost::shared_ptr<FlowCanvas::Module> > modules;
};//JackPortFlowCanvas

class JackPortModule : public FlowCanvas::Module
{
public:
	JackPortModule(boost::shared_ptr<FlowCanvas::Canvas> canvas, const std::string& title, double x, double y, bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_);
    ~JackPortModule();

    void menu_addPort();

private:    
    bool checkPortName(GdkEventKey *event);
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

JackPortModule::JackPortModule(boost::shared_ptr<FlowCanvas::Canvas> canvas, const std::string& title, double x, double y, bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_)
                : FlowCanvas::Module(canvas, title, x, y, true, true)
{
    uiXml = uiXml_;
    inputs = inputs_;
    ports = ports_;

    create_menu();

    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);
    entry->signal_key_release_event().connect(sigc::mem_fun(*this, &JackPortModule::checkPortName));
}//constructor

JackPortModule::~JackPortModule()
{
    //Nothing
}//destructor

void JackPortModule::menu_addPort()
{
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
            boost::shared_ptr<JackPortPort> port(new JackPortPort(thisModule, portName, true, colour, uiXml));

            this->add_port(port);
            this->resize();
        }//if
    }//if

    dialog->hide();
}//menu_addPort

bool JackPortModule::checkPortName(GdkEventKey *event)
{
    Gtk::Entry *entry;
    uiXml->get_widget("portNameEntry", entry);

    Gtk::Button *button;
    uiXml->get_widget("portNameDialogOKButton", button);

    std::string portName = entry->get_text();

    if ((portName.empty() == true) || (std::find(ports.begin(), ports.end(), portName) != ports.end())) {
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

JackPortPort::JackPortPort(boost::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_)
                : FlowCanvas::Port(module_, title_, isInput, colour)
{
    uiXml = uiXml_;
    module = module_;
    title = title_;

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
}//renamePort

void JackPortPort::menu_removePort()
{
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
    }//if

    JackSingleton &jackSingleton = JackSingleton::Instance();
    std::vector<std::string> inputPorts = jackSingleton.getInputPorts();
    std::vector<std::string> outputPorts = jackSingleton.getOutputPorts();

    boost::shared_ptr<JackPortModule> jackInputModule(new JackPortModule(::flowCanvas, "Jack Input", 0, 0, true, uiXml, inputPorts));
    jackInputModule->set_stacked_border(true);

    boost::shared_ptr<JackPortModule> jackOutputModule(new JackPortModule(::flowCanvas, "Jack Output", 0, 0, false, uiXml, outputPorts));
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

    jackInputModule->move(250, 150);
    jackOutputModule->move(250, 250);

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
    }//if

    portsDialog->hide();
}//constructor

JackPortDialog::~JackPortDialog()
{
    //Nothing
}//destructor

