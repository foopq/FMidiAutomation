/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "EntryProperties.h"
#include "Sequencer.h"
#include "SequencerEntry.h"
#include <iostream>
#include <boost/lexical_cast.hpp>

EntryProperties::EntryProperties(Glib::RefPtr<Gtk::Builder> uiXml_, std::shared_ptr<SequencerEntry> entry, bool hideCancelButton)
{
    uiXml = uiXml_;
    origImpl = entry->getImplClone();
    wasChanged = false;

    Gtk::Dialog *mainDialog;
    uiXml->get_widget("entryWindowProperties", mainDialog);

    Gtk::Notebook *notebook;
    uiXml->get_widget("entryWindowPropertiesNotebook", notebook);
    notebook->set_current_page(0);

    Glib::ustring title = entry->getTitle();
    Gtk::Entry *entryBox;
    uiXml->get_widget("titleEntry", entryBox);
    entryBox->set_text(title);

    uiXml->get_widget("msbEntry", entryBox);
    entryBox->set_text(boost::lexical_cast<Glib::ustring>((int)origImpl->msb));

    uiXml->get_widget("lsbEntry", entryBox);
    entryBox->set_text(boost::lexical_cast<Glib::ustring>((int)origImpl->lsb));

    Gtk::SpinButton *spinner;
    uiXml->get_widget("ccControllerNumberSpinButton", spinner);
    spinner->set_value(origImpl->msb);

    sigc::connection msbSpinnerChangedConnection = spinner->signal_value_changed().connect ( sigc::mem_fun(*this, &EntryProperties::ccControllerValueChanged) );

    uiXml->get_widget("lsbSpinButton", spinner);
    spinner->set_value(origImpl->lsb);

    sigc::connection lsbSpinnerChangedConnection = spinner->signal_value_changed().connect ( sigc::mem_fun(*this, &EntryProperties::lsbValueChanged) );

    Gtk::ComboBox *comboBox;
    uiXml->get_widget("channelComboBox", comboBox);
    int channel = (origImpl->channel+1) % 17;
    comboBox->set_active(channel);

    uiXml->get_widget("controlComboBox", comboBox);
    switch (origImpl->controllerType) {
        case ControlType::CC:
            comboBox->set_active(0);
            break;
        case ControlType::RPN:
            comboBox->set_active(1);
            break;
    }//switch

    Gtk::Button *button;
    uiXml->get_widget("entryWindowPropertiesCancelButton", button);
    if (true == hideCancelButton) {
        button->set_visible(false);
    } else {
        button->set_visible(true);
    }//if

    Gtk::CheckButton *checkButton;
    uiXml->get_widget("useSecondByteCheckButton", checkButton);
    checkButton->set_active(origImpl->useBothMSBandLSB);

    Gtk::RadioButton *radioButton7Bit;
    Gtk::RadioButton *radioButton14Bit;
    uiXml->get_widget("radioButton7Bit", radioButton7Bit);
    uiXml->get_widget("radioButton14Bit", radioButton14Bit);

    if (true == origImpl->sevenBit) {
        radioButton7Bit->set_active(true);
        radioButton14Bit->set_active(false);
    } else {
        radioButton7Bit->set_active(false);
        radioButton14Bit->set_active(true);
    }//if

    int result = mainDialog->run();

    newImpl.reset(new SequencerEntryImpl);
    if (result == 0) {
        uiXml->get_widget("titleEntry", entryBox);
        title = entryBox->get_text();
        newImpl->title = title;

        uiXml->get_widget("ccControllerNumberSpinButton", spinner);
        newImpl->msb = spinner->get_value();

        uiXml->get_widget("lsbSpinButton", spinner);
        newImpl->lsb = spinner->get_value();

        uiXml->get_widget("useSecondByteCheckButton", checkButton);
        newImpl->useBothMSBandLSB = checkButton->get_active();

        newImpl->sevenBit = radioButton7Bit->get_active();

        uiXml->get_widget("channelComboBox", comboBox);
        int selectedChannel = (comboBox->get_active_row_number()+16) % 17;
        newImpl->channel = selectedChannel;

        uiXml->get_widget("controlComboBox", comboBox);
        switch (comboBox->get_active_row_number()) {
            case 0:
                newImpl->controllerType = ControlType::CC;
                break;
            case 1:
                newImpl->controllerType = ControlType::RPN;
                break;
        }//switch

        if (*newImpl == *origImpl) {
            //Nothing
        } else {
            wasChanged = true;
        }//if
    }//if

    msbSpinnerChangedConnection.disconnect();
    lsbSpinnerChangedConnection.disconnect();

    mainDialog->hide();
}//constructor

void EntryProperties::ccControllerValueChanged()
{
    Gtk::SpinButton *spinner;
    uiXml->get_widget("ccControllerNumberSpinButton", spinner);
    unsigned int curVal = spinner->get_value();

    Gtk::Entry *entry;
    uiXml->get_widget("msbEntry", entry);
    entry->set_text(boost::lexical_cast<Glib::ustring>(curVal));
}//ccControllerValueChanged

void EntryProperties::lsbValueChanged()
{
    Gtk::SpinButton *spinner;
    uiXml->get_widget("lsbSpinButton", spinner);
    unsigned int curVal = spinner->get_value();

    Gtk::Entry *entry;
    uiXml->get_widget("lsbEntry", entry);
    entry->set_text(boost::lexical_cast<Glib::ustring>(curVal));
}//lsbValueChanged


