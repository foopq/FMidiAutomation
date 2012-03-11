/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#include <gtkmm.h>
#include <memory>
#include <set>

//FIXME: unify calls to base classes

class JackPortModule;
class JackPortBase;
class JackModuleBase;
class EntryPort;
class SequencerEntry;

class JackConnection
{
public:    
    JackConnection();
    virtual ~JackConnection();

    std::shared_ptr<JackPortBase> source();
    std::shared_ptr<JackPortBase> dest();
};//JackConnection

class JackPortDialog
{
    Gtk::DrawingArea *jackPortDialogDrawingArea;

    bool updateGraph(const Cairo::RefPtr<Cairo::Context> &context);
    bool mouseButtonPressed(GdkEventButton *event);
    bool mouseButtonReleased(GdkEventButton *event);
    bool mouseMoved(GdkEventMotion *event);

public:
    JackPortDialog(Glib::RefPtr<Gtk::Builder> uiXml);
    ~JackPortDialog();

};//JackPortDialog

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

class JackPortFlowCanvas
{
public:    
    JackPortFlowCanvas();
    virtual ~JackPortFlowCanvas();

    void addModule(std::shared_ptr<JackModuleBase> module);
    void clearModules();

    virtual void connect(std::shared_ptr<JackPortBase> c1, std::shared_ptr<JackPortBase> c2);
    virtual void disconnect(std::shared_ptr<JackPortBase> c1, std::shared_ptr<JackPortBase> c2);

    virtual void add_connection(std::shared_ptr<JackPortBase> port1, std::shared_ptr<JackPortBase> port2, unsigned int colour);
    virtual void remove_connection(std::shared_ptr<JackPortBase> port1, std::shared_ptr<JackPortBase> port2);

private:
    std::set<std::shared_ptr<JackModuleBase>> modules;
};//JackPortFlowCanvas

class JackModuleBase
{
public:
    JackModuleBase();
    virtual ~JackModuleBase();

};//JackModuleBase

class JackPortModule : public JackModuleBase, public std::enable_shared_from_this<JackPortModule>
{
public:
	JackPortModule(std::shared_ptr<JackPortFlowCanvas> canvas, const std::string& title, double x, double y, 
                    bool inputs_, Glib::RefPtr<Gtk::Builder> uiXml_, std::vector<std::string> &ports_);
    ~JackPortModule();

    void menu_addPort();
    void removePort(const std::string &name);
    virtual void move(double dx, double dy);
    bool checkPortName(GdkEventKey *event);
    void setCurNamingPorts();
    void restorePosition();

    //void doresize();
    void add_port(std::shared_ptr<JackPortBase> port);
    void remove_port(std::shared_ptr<JackPortBase> port);
    std::shared_ptr<JackPortBase> get_port(const std::string &title);
    std::vector<std::string> getPorts();

private:    
	////void create_menu();
    virtual bool show_menu(GdkEventButton *ev);
    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
    Gtk::Menu *m_pMenuPopup; //FIXME: Is this a leak?

    bool inputs;
    Glib::RefPtr<Gtk::Builder> uiXml;
    std::vector<std::string> ports;
};//JackPortModule

class EntryModule : public JackModuleBase
{
public:
	EntryModule(std::shared_ptr<JackPortFlowCanvas> canvas, const std::string& title, double x, double y, 
                    Glib::RefPtr<Gtk::Builder> uiXml_, std::shared_ptr<SequencerEntry> entry_);
    ~EntryModule();

    virtual void move(double dx, double dy);
    void restorePosition();

    void addPortConnection(std::shared_ptr<EntryPort> entryPort, const std::string &jackTitle);
    void removePortConnection(std::shared_ptr<EntryPort> entryPort, const std::string &jackTitle);

    std::shared_ptr<SequencerEntry> getEntry() const;

    void doresize();
    void do_add_port(std::shared_ptr<JackPortBase> port);

    std::vector<std::shared_ptr<JackPortBase>> ports();

private:    
    std::shared_ptr<SequencerEntry> entry;
    Glib::RefPtr<Gtk::Builder> uiXml;
    std::set<std::string> inputConnections;
    std::set<std::string> outputConnections;
    std::vector<std::shared_ptr<JackPortBase>> portList;
};//EntryModule

class JackPortBase
{

public:
    JackPortBase(std::shared_ptr<JackModuleBase> module, const std::string &name, bool is_input, uint32_t colour);
    virtual ~JackPortBase();

    bool hasConnections();
    std::shared_ptr<JackConnection> getFirstConnection();
    void remove_connection(std::shared_ptr<JackConnection> connection);
};//JackPortBase

class JackPortPort : public JackPortBase
{
public:
	JackPortPort(std::shared_ptr<JackPortModule> module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~JackPortPort();

    std::string getTitle() const;

private:    
	////void create_menu();
    virtual bool show_menu(GdkEventButton *ev);
    void menu_renamePort();
    void menu_removePort();

    std::shared_ptr<Gtk::Menu> _menu;
    std::shared_ptr<JackPortModule> module;
    std::string title;
    Glib::RefPtr<Gtk::Builder> uiXml;
    bool isInput;
};//JackPortPort

class EntryPort : public JackPortBase
{
public:
	EntryPort(std::shared_ptr<EntryModule> module_, const std::string& title_, bool isInput, unsigned int colour, Glib::RefPtr<Gtk::Builder> uiXml_);
    ~EntryPort();

    bool isInput() const;

private:    
    std::shared_ptr<EntryModule> entryModule;
    std::string title;
    Glib::RefPtr<Gtk::Builder> uiXml;
    bool isThisInput;
};//EntryPort


