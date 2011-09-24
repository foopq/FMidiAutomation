#include "Config.h"
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <fstream>

MRUFileLoadHelper::MRUFileLoadHelper(Glib::ustring &filename_, std::function<void (const Glib::ustring &)> &loadCallback_)
{
    filename = filename_;
    loadCallback = loadCallback_;
}//constructor

void MRUFileLoadHelper::doLoadFile()
{
    loadCallback(filename);
}//doLoadFile

MRUList::MRUList(unsigned int depth_)
{
    depth = depth_;
    m_refUIManager = Gtk::UIManager::create();

    configFileName = Glib::ustring(getpwuid(getuid())->pw_dir) + Glib::ustring("/.fmidiautomation/mrulist");

    loadFileList();
}//constructor

MRUList::~MRUList()
{
    //Nothing
}//destructor

MRUList &MRUList::Instance()
{
    static MRUList mruList(10);
    return mruList;
}//Instance

void MRUList::registerTopMenu(FMidiAutomationMainWindow *window, Gtk::MenuItem *menuOpenRecent_)
{
    std::cout << "%%%% registerTopMenu: " << window << std::endl;

    menuOpenRecentList[window] = menuOpenRecent_;
    updateRecentMenu();
}//registerTopMenu

void MRUList::unregisterTopMenu(FMidiAutomationMainWindow *window)
{
    std::cout << "%%%% unregisterTopMenu: " << window << std::endl;

    auto mapIter = menuOpenRecentList.find(window);
    if (mapIter != menuOpenRecentList.end()) {
        menuOpenRecentList.erase(mapIter);
    }//if
}//unregisterTopMenu

void MRUList::setLoadCallback(std::function<void (const Glib::ustring &)> loadCallback_)
{
    std::cout << "%%%% setLoadCallback" << std::endl;
    loadCallback = loadCallback_;
    assert(loadCallback);

    for (auto helperIter : mruFileLoadHelpers) {
        helperIter->loadCallback = loadCallback;
    }//for
}//setLoadCallback

void MRUList::addFile(const Glib::ustring &fileName)
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
    std::cout << "%%%%% updateRecentMenu: " << menuOpenRecentList.empty() << " - " << this << std::endl;

    if (menuOpenRecentList.empty() == true) {
        return;
    }//if

    mruSubmenu.reset(new Gtk::Menu);
    mruMenuItems.clear();
    mruFileLoadHelpers.clear();

    for (auto file : fileList) {
        std::shared_ptr<Gtk::MenuItem> item(new Gtk::MenuItem(file));
        mruMenuItems.push_back(item);
        mruSubmenu->append(*item);
       
        std::shared_ptr<MRUFileLoadHelper> helper(new MRUFileLoadHelper(file, loadCallback));
        mruFileLoadHelpers.push_back(helper);

        item->signal_activate().connect(sigc::mem_fun(*helper, &MRUFileLoadHelper::doLoadFile));
    }//foreach

    for (auto menu : menuOpenRecentList) {
        menu.second->set_submenu(*mruSubmenu);
        menu.second->show_all();
    }//for
}//updateRecentMenu

std::pair<decltype(MRUList::fileList.begin()), decltype(MRUList::fileList.end())> MRUList::getFileList()
{
    return std::make_pair(fileList.begin(), fileList.end());
}//getFileList

void MRUList::loadFileList()
{
    std::string filename = Glib::locale_from_utf8(configFileName);

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
    std::string filename = Glib::locale_from_utf8(configFileName);

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

    for (Glib::ustring fileStr : fileList) {
        std::string fileStrNarrow = Glib::locale_from_utf8(fileStr);
        outputArchive & BOOST_SERIALIZATION_NVP(fileStrNarrow);
    }//foreach
}//saveFileList


FMidiAutomationConfig::FMidiAutomationConfig()
{
    Glib::ustring baseDir = Glib::ustring(getpwuid(getuid())->pw_dir) + Glib::ustring("/.fmidiautomation");
    std::string baseDirNarrow = Glib::locale_from_utf8(baseDir);

    DIR *pDir = opendir(baseDirNarrow.c_str());
    if (pDir != nullptr) {
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


