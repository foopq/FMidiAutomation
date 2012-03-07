#include <gtkmm.h>


int main(int argc, char** argv) 
{


    srand(time(nullptr));

    Glib::thread_init();

    Gtk::Main kit(argc, argv);

    Glib::RefPtr<Gtk::Builder> uiXml;
    Gtk::Window *mainWindow;

    uiXml = Gtk::Builder::create_from_file(argv[1]);
    uiXml->get_widget("mainWindow", mainWindow);

    mainWindow->show();
    kit.run(*mainWindow);

    return (EXIT_SUCCESS);
}//main
