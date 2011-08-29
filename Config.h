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
    int depth;
    std::string fileName;

    void loadFileList();
    void saveFileList();

public:
    MRUList(unsigned int depth, Glib::ustring &&fileName);
    virtual ~MRUList();

    void addFile(Glib::ustring &fileName);
    std::pair<decltype(fileList.begin()), decltype(fileList.end())> getFileList();
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

