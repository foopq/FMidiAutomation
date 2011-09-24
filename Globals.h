/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __GLOBALS_H
#define __GLOBALS_H

#include "Config.h"
#include "Data/FMidiAutomationData.h"
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <thread>

class Sequencer;
struct GraphState;

struct TempoGlobals
{
    TempoGlobals();

    bool tempoDataSelected;
};//TempoGlobals

struct Globals
{
    Globals();
    ~Globals();

    static std::recursive_mutex globalsMutex;

    static Globals &Instance();
    static void ResetInstance();

    FMidiAutomationData projectData;
    FMidiAutomationConfig config;

    std::string versionStr;

    std::string topBarFont;
    unsigned int topBarFontSize;

    std::string bottomBarFont;
    unsigned int bottomBarFontSize;

    bool darkTheme;

    TempoGlobals tempoGlobals;

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
private:

    static std::shared_ptr<Globals> instance;
};//Globals

BOOST_CLASS_VERSION(Globals, 1);

#endif

