/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#include "Sequencer.h"
#include "SequencerEntry.h"
#include "Animation.h"
#include "jack.h"
#include <iostream>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "SerializationHelper.h"

static const unsigned int entryWindowHeight = 138 + 6; //size plus padding
static const unsigned int smallEntryWindowHeight = 46 + 4; //size plus padding


Sequencer::Sequencer()
{
    std::cout << "Sequencer::Sequencer() - " << this << std::endl;
}//constructor

unsigned int Sequencer::getNumEntries() const
{
    return entries.size();
}//getNumEntries

void Sequencer::addEntry(std::shared_ptr<SequencerEntry> entry)
{
    entries.push_back(entry);
}//addEntry

void Sequencer::deleteEntry(std::shared_ptr<SequencerEntry> entry)
{
    auto entryIter = std::find(entries.begin(), entries.end(), entry);
    assert(entryIter != entries.end());
    entries.erase(entryIter);
}//deleteEntry

fmaipair<decltype(Sequencer::entries.begin()), decltype(Sequencer::entries.end())> Sequencer::getEntryPair()
{
    return fmai_make_pair(entries.begin(), entries.end());
}//getEntryPair

void Sequencer::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    inputArchive & BOOST_SERIALIZATION_NVP(entries);

    std::cout << "Sequencer::doLoad: " << this << std::endl;
}//doLoad

void Sequencer::doSave(boost::archive::xml_oarchive &outputArchive)
{
    outputArchive & BOOST_SERIALIZATION_NVP(entries);
}//doSave

void Sequencer::cloneEntryMap()
{
    assert(!"cloneEntryMap needs UI hooks");

    //std::map<std::shared_ptr<SequencerEntry>, int > entriesClone;
    //std::map<int, std::shared_ptr<SequencerEntry> > entriesCloneRev;
    decltype(entries) entriesClone;
    for (auto mapIter : entries) {
        std::shared_ptr<SequencerEntry> entryClone = mapIter->deepClone();
        //entriesClone[entryClone] = mapIter->second;
        //entriesCloneRev[mapIter->second] = entryClone;
        entriesClone.push_back(entryClone);

        //parentWidget->children().remove(*mapIter->first->getHookWidget());
    }//for

    entries.swap(entriesClone);

    //assert(entries.size() == entriesCloneRev.size());

/*    
    parentWidget->children().clear();

    for (auto mapIter : entriesCloneRev) {
        Gtk::Widget *entryHookWidget = mapIter->getHookWidget();
        parentWidget->children().push_back(Gtk::Box_Helpers::Element(*entryHookWidget));
    }//for
*/

/*    
    parentWidget->children().push_back(Gtk::Box_Helpers::Element(tmpLabel));

    adjustFillerHeight();
    adjustEntryIndices();
    notifyOnScroll(-1);

    selectedEntry = nullptr;
*/

 std::cout << "cEM 5" << std::endl;   
}//cloneEntryMap

void Sequencer::setEntryMap(SequencerEntriesType &entryMap)
{
    entries = entryMap;
}//setEntryMap

decltype(Sequencer::entries) Sequencer::getEntryMap()
{
    return entries;
}//getEntryMap




