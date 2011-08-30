#include "Config.h"
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <fstream>
#include <boost/foreach.hpp>

MRUList::MRUList(unsigned int depth_, Glib::ustring &&fileName_)
{
    depth = depth_;
    fileName = fileName_;
    menuOpenRecent = NULL;
    m_refUIManager = Gtk::UIManager::create();

    loadFileList();
}//constructor

MRUList::~MRUList()
{
    //Nothing
}//destructor

void MRUList::setTopMenu(Gtk::MenuItem *menuOpenRecent_)
{
    menuOpenRecent = menuOpenRecent_;
    updateRecentMenu();
}//setTopMenu

void MRUList::addFile(Glib::ustring &fileName)
{
    auto fileListIter = std::find(fileList.begin(), fileList.end(), fileName);
    if (fileListIter != fileList.end()) {
        fileList.erase(fileListIter);
    }//if

    fileList.push_front(fileName);

    if (fileList.size() > depth) {
        fileList.resize(depth);
    }//if

    saveFileList();
    updateRecentMenu();
}//addFile

void MRUList::updateRecentMenu()
{
    if (NULL == menuOpenRecent) {
        return;
    }//if

    mruSubmenu.reset(new Gtk::Menu);
    mruMenuItems.clear();

    BOOST_FOREACH (auto file, fileList) {
        std::shared_ptr<Gtk::MenuItem> item(new Gtk::MenuItem(file));
        mruMenuItems.push_back(item);
        mruSubmenu->append(*item);
    }//foreach

    menuOpenRecent->set_submenu(*mruSubmenu);
    menuOpenRecent->show_all();
}//updateRecentMenu


std::pair<decltype(MRUList::fileList.begin()), decltype(MRUList::fileList.end())> MRUList::getFileList()
{
    return std::make_pair(fileList.begin(), fileList.end());
}//getFileList

void MRUList::loadFileList()
{
    std::string filename = Glib::locale_from_utf8(fileName);

    std::ifstream inputStream(filename.c_str());
    if (false == inputStream.good()) {
        return;
    }//if

    boost::archive::xml_iarchive inputArchive(inputStream);

    unsigned int MRUListVersion = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(MRUListVersion);
    inputArchive & BOOST_SERIALIZATION_NVP(depth);

    unsigned int listSize = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(listSize);

    for (unsigned int pos = 0; pos < listSize; ++pos) {
        std::string fileStr;
        inputArchive & BOOST_SERIALIZATION_NVP(fileStr);
        fileList.push_back(fileStr);
    }//for

    updateRecentMenu();
}//loadFileList

void MRUList::saveFileList()
{
    std::string filename = Glib::locale_from_utf8(fileName);

    std::ofstream outputStream(filename.c_str());
    assert(outputStream.good());
    if (false == outputStream.good()) {
        return;
    }//if

    boost::archive::xml_oarchive outputArchive(outputStream);

    const unsigned int MRUListVersion = 1;
    outputArchive & BOOST_SERIALIZATION_NVP(MRUListVersion);
    outputArchive & BOOST_SERIALIZATION_NVP(depth);

    unsigned int listSize = fileList.size();
    outputArchive & BOOST_SERIALIZATION_NVP(listSize);

    BOOST_FOREACH (Glib::ustring fileStr, fileList) {
        std::string fileStrNarrow = Glib::locale_from_utf8(fileStr);
        outputArchive & BOOST_SERIALIZATION_NVP(fileStrNarrow);
    }//foreach
}//saveFileList


FMidiAutomationConfig::FMidiAutomationConfig() : mruList(5, Glib::ustring(getpwuid(getuid())->pw_dir) + Glib::ustring("/.fmidiautomation/mrulist"))
{
    Glib::ustring baseDir = Glib::ustring(getpwuid(getuid())->pw_dir) + Glib::ustring("/.fmidiautomation");
    std::string baseDirNarrow = Glib::locale_from_utf8(baseDir);

    DIR *pDir = opendir(baseDirNarrow.c_str());
    if (pDir != NULL) {
        (void)closedir(pDir);
    } else {
        mkdir(baseDirNarrow.c_str(), 0755);
    }//if
}//constructor

FMidiAutomationConfig::~FMidiAutomationConfig()
{
    //Nothing
}//destructor

void FMidiAutomationConfig::loadConfig()
{
    //Nothing yet
}//loadConfig

void FMidiAutomationConfig::saveConfig()
{
    //Nothing yet
}//saveConfig

MRUList &FMidiAutomationConfig::getMRUList()
{
    return mruList;
}//getMRUList


