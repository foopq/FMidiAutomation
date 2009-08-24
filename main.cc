#include "FMidiAutomationMainWindow.h"

FMidiAutomationMainWindow *mainWindow;

int main(int argc, char** argv) 
{
    srand(time(NULL));
    Gtk::Main kit(argc, argv);

    mainWindow = new FMidiAutomationMainWindow();

    kit.run(*mainWindow->MainWindow());

//    delete mainWindow;

    return (EXIT_SUCCESS);
}//main
