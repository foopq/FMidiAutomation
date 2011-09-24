/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#include "WindowManager.h"
#include "FMidiAutomationMainWindow.h"
#include "jack.h"
#include "UI/SequencerUI.h"
#include "UI/SequencerEntryUI.h"
#include "UI/SequencerEntryBlockUI.h"
#include "Data/SequencerEntryBlock.h"
#include "SerializationHelper.h"
#include "Globals.h"

namespace
{

std::shared_ptr<SequencerEntryBlockUI> findWrappingEntryBlockUI(std::shared_ptr<SequencerUI> sequencer, std::shared_ptr<SequencerEntryBlock> entryBlock)
{
    for (auto entryIter : sequencer->getEntryPair()) {
        for (auto entryBlockIter : entryIter.first->getEntryBlocksPair()) {
            if (entryBlockIter.second->getBaseEntryBlock() == entryBlock) {
                return entryBlockIter.second;
            }//if
        }//for
    }//for

    return std::shared_ptr<SequencerEntryBlockUI>();
}//findWrappingEntryBlockUI

}//anonymous namespace



WindowManager &WindowManager::Instance()
{
    static WindowManager windowManager;
    return windowManager;
}//Instance

std::shared_ptr<FMidiAutomationMainWindow> WindowManager::createMainWindow()
{
    mainWindow.reset(new FMidiAutomationMainWindow);
    mainWindow->init(false, std::shared_ptr<SequencerEntryBlockUI>());

    MRUList &mruList = MRUList::Instance();
    mruList.setLoadCallback(mainWindow->getLoadCallback());

    return mainWindow;
}//createMainWindow

std::shared_ptr<FMidiAutomationMainWindow> WindowManager::createWindow(bool curveEditorOnlyMode, std::shared_ptr<SequencerEntryBlockUI> editingEntryBlock)
{
    std::shared_ptr<FMidiAutomationMainWindow> newWindow(new FMidiAutomationMainWindow);
    newWindow->init(curveEditorOnlyMode, editingEntryBlock);

    windows.insert(newWindow);
    return newWindow;
}//createWindow

void WindowManager::unregisterWindow(std::shared_ptr<FMidiAutomationMainWindow> window)
{
    auto windowIter = windows.find(window);
    if (windowIter != windows.end()) {
        windows.erase(windowIter);
    }//if

    MRUList &mruList = MRUList::Instance();
    mruList.unregisterTopMenu(window.get());

    /* -- reenable if we ever do multiple sequencer windows
    if (windows.empty() == true) {
        JackSingleton &jackSingleton = JackSingleton::Instance();
        jackSingleton.stopClient();
        Gtk::Main::quit();
    }//if
    */
}//unregisterWindow

std::shared_ptr<FMidiAutomationMainWindow> WindowManager::getMainWindow()
{
    return mainWindow;
}//getMainWindow

void WindowManager::closeAllWindows()
{
    //XXX: Do we leak windows?
    for (auto window : windows) {
        window->MainWindow()->hide();
    }//for

    windows.clear();
}//closeAllWindows

void WindowManager::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    mainWindow->doLoad(inputArchive);

    int numWindows = 0;
    inputArchive & BOOST_SERIALIZATION_NVP(numWindows);

    windows.clear();
    for (int win = 0; win < numWindows; ++win) {
        std::shared_ptr<FMidiAutomationMainWindow> newWindow(new FMidiAutomationMainWindow);
        windows.insert(newWindow);

        bool curveEditorOnlyMode;
        std::shared_ptr<SequencerEntryBlock> editingEntryBlockBase;

        inputArchive & BOOST_SERIALIZATION_NVP(curveEditorOnlyMode);
        inputArchive & BOOST_SERIALIZATION_NVP(editingEntryBlockBase);
        assert(editingEntryBlockBase != nullptr);

        newWindow->doLoad(inputArchive);

        std::shared_ptr<SequencerEntryBlockUI> editingEntryBlock = findWrappingEntryBlockUI(newWindow->getSequencer(), editingEntryBlockBase);
        assert(editingEntryBlock != nullptr);

        newWindow->init(curveEditorOnlyMode, editingEntryBlock);
        newWindow->forceCurveEditorMode(editingEntryBlock);
    }//for
}//doLoad

void WindowManager::doSave(boost::archive::xml_oarchive &outputArchive)
{
    mainWindow->doSave(outputArchive);

    int numWindows = windows.size();
    outputArchive & BOOST_SERIALIZATION_NVP(numWindows);

    for (auto window : windows) {
        bool curveEditorOnlyMode = window->getCurveEditorOnlyMode();
        std::shared_ptr<SequencerEntryBlock> editingEntryBlock = window->getEditingEntryBlock()->getBaseEntryBlock();

        outputArchive & BOOST_SERIALIZATION_NVP(curveEditorOnlyMode);
        outputArchive & BOOST_SERIALIZATION_NVP(editingEntryBlock);

        window->doSave(outputArchive);
    }//for
}//doSave


