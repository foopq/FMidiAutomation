/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationMainWindow.h"
#include "jack.h"
#include "WindowManager.h"
#include "Globals.h"
#include <iostream>
//#include <libgnomecanvasmm.h>
#include <glibmm/exception.h>


//- jack needs to use reentrant locks
//- we do need to serialize ui state (sequencer ui)

int main(int argc, char** argv) 
{
    srand(time(nullptr));

std::cout << "here" << std::endl;

    Glib::thread_init();

std::cout << "here2" << std::endl;

//    Gnome::Canvas::init();

//    g_threads_init();
//    gdk_threads_init();
//    gtk_init();

std::cout << "here3" << std::endl;

    //gdk_threads_enter();
    Gtk::Main kit(argc, argv);
//    gdk_threads_leave();

    //mainWindow = new FMidiAutomationMainWindow();
    //mainWindow->init();

std::cout << "here4" << std::endl;

    WindowManager &windowManager = WindowManager::Instance();
    std::shared_ptr<FMidiAutomationMainWindow> mainWindow = windowManager.createMainWindow();
    //std::shared_ptr<FMidiAutomationMainWindow> mainWindow(new FMidiAutomationMainWindow);
    //mainWindow->init(false, std::shared_ptr<SequencerEntryBlockUI>());

    (void)JackSingleton::Instance();
    Globals::ResetInstance();

std::cout << "here5" << std::endl;

    kit.run(*mainWindow->MainWindow());

//    std::cout << "EXITING MAIN" << std::endl;
    // reenable if we ever do multiple sequencer windows
//    kit.run();

/*    
    Glib::RefPtr<Gtk::Builder> uiXml;
//    Gtk::Window *mainWindow;
    uiXml = Gtk::Builder::create_from_file("FMidiAutomationEntry.glade");
//    uiXml->get_widget("entryWindow", mainWindow);

    Gtk::Window *mainWindow2;
    uiXml->get_widget("entryWindow", mainWindow2);
//    std::cout << mainWindow << " -- " << mainWindow2 << std::endl;

mainWindow2->show();

int height = mainWindow2->get_height();
std::cout << "height: " << height << std::endl;

    kit.run(*mainWindow2);
*/    

//    delete mainWindow;

    return (EXIT_SUCCESS);
}//main

