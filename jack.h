#ifndef __JACK_H
#define __JACK_H

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <boost/thread/mutex.hpp> 
#include <boost/thread/thread.hpp>

class JackSingleton
{
    jack_client_t *jackClient;
    jack_transport_state_t curTransportState;
    int curFrame;
    boost::mutex mutex; 
    boost::condition_variable condition;
    boost::shared_ptr<boost::thread> thread;

    JackSingleton();

public:
    ~JackSingleton();

    static JackSingleton &Instance();

    jack_transport_state_t getTransportState();
    int getTransportFrame();

    void setTransportState(jack_transport_state_t state);
    void setTime(int frame);

    //Do not use these:
    int process(jack_nframes_t nframes, void *arg);
    void error(const char *desc);
    void jack_shutdown(void *arg);
};//JackSingleton

#endif

