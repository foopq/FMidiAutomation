#include "FMidiAutomationMainWindow.h"
#include "jack.h"
#include <iostream>

FMidiAutomationMainWindow *mainWindow;

int main(int argc, char** argv) 
{
    srand(time(NULL));
    Gtk::Main kit(argc, argv);

    mainWindow = new FMidiAutomationMainWindow();
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

