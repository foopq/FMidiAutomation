/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "FMidiAutomationMainWindow.h"
#include "jack.h"
#include <iostream>
#include <libgnomecanvasmm.h>
#include <glibmm/exception.h>

FMidiAutomationMainWindow *mainWindow;

void queue_draw()
{
    mainWindow->queue_draw();
}//queue_draw

int main(int argc, char** argv) 
{
    srand(time(NULL));

    Glib::thread_init();
    Gnome::Canvas::init();

//    g_threads_init();
//    gdk_threads_init();
//    gtk_init();

    //gdk_threads_enter();
    Gtk::Main kit(argc, argv);
//    gdk_threads_leave();

    mainWindow = new FMidiAutomationMainWindow();
    mainWindow->init();

    JackSingleton &jackSingleton = JackSingleton::Instance();

    kit.run(*mainWindow->MainWindow());

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

