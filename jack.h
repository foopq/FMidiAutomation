/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/


#ifndef __JACK_H
#define __JACK_H

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <boost/thread/mutex.hpp> 
#include <boost/thread/thread.hpp>
#include <map>
#include <vector>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <boost/thread/recursive_mutex.hpp>

enum class ControlType : char;

struct MidiInputInfoHeader
{
    jack_port_t *port;
    int curFrame;
    unsigned int bufferPos;
    unsigned int length;
};//MidiInputInfoHeader

class JackSingleton
{
    jack_client_t *jackClient;
    jack_transport_state_t curTransportState;
    int curFrame;
    boost::recursive_mutex mutex; 
    boost::condition_variable condition;
    std::shared_ptr<boost::thread> thread;

    bool recordMidi;
    std::vector<unsigned char> midiRecordBuffer;
    std::vector<MidiInputInfoHeader> midiRecordBufferHeaders;

    std::map<std::string, jack_port_t *> inputPorts;
    std::map<std::string, jack_port_t *> outputPorts;

    std::map<jack_port_t *, void *> midiOutputBuffersRaw;
    std::map<jack_port_t *, std::vector<unsigned char> > midiOutputBuffers;

    std::map<jack_port_t *, std::map<unsigned int /*channel*/, std::map<unsigned int /*controller*/, unsigned char /*value*/> > > ccValueCache;

    bool processingMidi;

//.... N/M input/output ports/buffers, add, delete, rename?
//       -> process iterates over input ports, etc...

    JackSingleton();

    bool hasValueChanged(jack_port_t *port, unsigned int channel, unsigned int msb, unsigned int lsb, 
                            ControlType controllerType, unsigned int sampledValue);

public:
    ~JackSingleton();
    void stopClient();

    static JackSingleton &Instance();

    jack_transport_state_t getTransportState();
    int getTransportFrame();

    void setTransportState(jack_transport_state_t state);
    void setTime(int frame);

    std::vector<std::string> getInputPorts();
    void setInputPorts(std::vector<std::string> ports);

    std::vector<std::string> getOutputPorts();
    void setOutputPorts(std::vector<std::string> ports);

    jack_port_t *getOutputPort(const std::string &portName);
    jack_port_t *getInputPort(const std::string &portName);

    std::string getOutputPortName(jack_port_t *port);
    std::string getInputPortName(jack_port_t *port);

    //Do not use these:
    int process(jack_nframes_t nframes, void *arg);
    void error(const char *desc);
    void jack_shutdown(void *arg);

    void setRecordMidi(bool record);
    std::vector<unsigned char> &getRecordBuffer();
    std::vector<MidiInputInfoHeader> &getMidiRecordBufferHeaders();

    bool areProcessingMidi();
    void setProcessingMidi(bool processing);

    void doLoad(boost::archive::xml_iarchive &inputArchive);
    void doSave(boost::archive::xml_oarchive &outputArchive);
};//JackSingleton

#endif

