#ifndef __CONFIG_H
#define __CONFIG_H

#include <deque>
#include <string>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <gtkmm.h>
#include <functional>

class FMidiAutomationMainWindow;

struct MRUFileLoadHelper
{
    Glib::ustring filename;
    std::function<void (const Glib::ustring &)> loadCallback;

public:
    MRUFileLoadHelper(Glib::ustring &filename, std::function<void (const Glib::ustring &)> &loadCallback);

    void doLoadFile();
};//MRUFileLoadHelper

class MRUList
{
    Glib::ustring configFileName;
    std::deque<Glib::ustring> fileList;
    unsigned int depth;
    std::shared_ptr<Gtk::Menu> mruSubmenu;
    std::vector<std::shared_ptr<Gtk::MenuItem>> mruMenuItems;
    std::vector<std::shared_ptr<MRUFileLoadHelper>> mruFileLoadHelpers;
    std::function<void (const Glib::ustring &)> loadCallback;
    std::map<FMidiAutomationMainWindow *, Gtk::MenuItem *> menuOpenRecentList;

    Glib::RefPtr<Gtk::UIManager> m_refUIManager;

    void loadFileList();
    void saveFileList();
    void updateRecentMenu();

    MRUList(unsigned int depth);
public:
    virtual ~MRUList();

    static MRUList &Instance();

    void addFile(const Glib::ustring &fileName);
    std::pair<decltype(fileList.begin()), decltype(fileList.end())> getFileList();

    void registerTopMenu(FMidiAutomationMainWindow *window, Gtk::MenuItem *menuOpenRecent);
    void unregisterTopMenu(FMidiAutomationMainWindow *window);
    void setLoadCallback(std::function<void (const Glib::ustring &)> loadCallback);
};//MRUList

class FMidiAutomationConfig
{
public:    
    FMidiAutomationConfig();
    ~FMidiAutomationConfig();

    void loadConfig();
    void saveConfig();
};//FMidiAutomationConfig

#endif

