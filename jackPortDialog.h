/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include <gtkmm.h>

class JackPortDialog
{

public:
    JackPortDialog(Glib::RefPtr<Gtk::Builder> uiXml);
    ~JackPortDialog();

};//JackPortDialog


