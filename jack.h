#ifndef __JACK_H
#define __JACK_H

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <boost/thread/mutex.hpp> 
#include <boost/thread/thread.hpp>
#include <map>
#include <vector>

class JackSingleton
{
    jack_client_t *jackClient;
    jack_transport_state_t curTransportState;
    int curFrame;
    boost::mutex mutex; 
    boost::condition_variable condition;
    boost::shared_ptr<boost::thread> thread;

    bool recordMidi;
    std::vector<unsigned char> midiRecordBuffer;

    std::map<std::string, jack_port_t *> inputPorts;
    std::map<std::string, jack_port_t *> outputPorts;

//.... N/M input/output ports/buffers, add, delete, rename?
//       -> process iterates over input ports, etc...

    JackSingleton();

public:
    ~JackSingleton();

    static JackSingleton &Instance();

    jack_transport_state_t getTransportState();
    int getTransportFrame();

    void setTransportState(jack_transport_state_t state);
    void setTime(int frame);

    std::vector<std::string> getInputPorts();
    void setInputPorts(std::vector<std::string> ports);

    std::vector<std::string> getOutputPorts();
    void setOutputPorts(std::vector<std::string> ports);

    //Do not use these:
    int process(jack_nframes_t nframes, void *arg);
    void error(const char *desc);
    void jack_shutdown(void *arg);

    void setRecordMidi(bool record);
    std::vector<unsigned char> &getRecordBuffer();
};//JackSingleton

#endif

