#ifndef __CONFIG_H
#define __CONFIG_H

#include <deque>
#include <string>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <gtkmm.h>


class MRUList
{
    std::deque<Glib::ustring> fileList;
    unsigned int depth;
    std::string fileName;
    Gtk::MenuItem *menuOpenRecent;
    std::shared_ptr<Gtk::Menu> mruSubmenu;
    std::vector<std::shared_ptr<Gtk::MenuItem>> mruMenuItems;
    Glib::RefPtr<Gtk::UIManager> m_refUIManager;

    void loadFileList();
    void saveFileList();
    void updateRecentMenu();

public:
    MRUList(unsigned int depth, Glib::ustring &&fileName);
    virtual ~MRUList();

    void addFile(Glib::ustring &fileName);
    std::pair<decltype(fileList.begin()), decltype(fileList.end())> getFileList();

    void setTopMenu(Gtk::MenuItem *menuOpenRecent);
};//MRUList

class FMidiAutomationConfig
{
    MRUList mruList;

public:    
    FMidiAutomationConfig();
    ~FMidiAutomationConfig();

    void loadConfig();
    void saveConfig();

    MRUList &getMRUList();
};//FMidiAutomationConfig

#endif

