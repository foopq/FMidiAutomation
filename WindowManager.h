/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#include <set>
#include <memory>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>

class FMidiAutomationMainWindow;
class SequencerEntryBlockUI;

class WindowManager
{
    std::shared_ptr<FMidiAutomationMainWindow> mainWindow;
    std::set<std::shared_ptr<FMidiAutomationMainWindow>> windows;

public:
    static WindowManager &Instance();

    std::shared_ptr<FMidiAutomationMainWindow> createWindow(bool curveEditorOnlyMode, std::shared_ptr<SequencerEntryBlockUI> editingEntryBlock);
    std::shared_ptr<FMidiAutomationMainWindow> createMainWindow();
    void unregisterWindow(std::shared_ptr<FMidiAutomationMainWindow> window);

    std::shared_ptr<FMidiAutomationMainWindow> getMainWindow();
    void closeAllWindows(); //except mainWindow
    void doLoad(boost::archive::xml_iarchive &inputArchive);
    void doSave(boost::archive::xml_oarchive &outputArchive);
};//WindowManager



