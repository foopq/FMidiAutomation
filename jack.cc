#include "jack.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/function.hpp>
#include <iostream>
#include "FMidiAutomationMainWindow.h"

//extern FMidiAutomationMainWindow *mainWindow;

namespace
{

/*
void notifyJackUpdate(boost::condition_variable &condition)
{
    JackSingleton &jackSingleton = JackSingleton::Instance();

    int lastCurFrame = std::numeric_limits<int>::min();
    jack_transport_state_t lastCurState = (jack_transport_state_t)0xffff;

    boost::mutex mutex;
    boost::unique_lock<boost::mutex> lock(mutex);

    while (1) {
        condition.wait(lock);

        int curFrame = jackSingleton.getTransportFrame();
        jack_transport_state_t curState = jackSingleton.getTransportState();

        mainWindow->updateJackState(curState, curFrame);

        lastCurFrame = curFrame;
        lastCurState = curState;
        boost::this_thread::yield();
    }//while
}//notifyJackUpdate
*/

int process_impl(jack_nframes_t nframes, void *arg)
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    return jackSingleton.process(nframes, arg);
}//process

void error_impl(const char *desc)
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.error(desc);
}//error

void jack_shutdown_impl(void *arg)
{
    JackSingleton &jackSingleton = JackSingleton::Instance();
    jackSingleton.jack_shutdown(arg);
}//jack_shutdown


}//anonymous namespace

JackSingleton::JackSingleton()
{
//    boost::function<void (void)> threadFunc = boost::lambda::bind(&notifyJackUpdate, boost::lambda::var(condition));
//    thread.reset(new boost::thread(threadFunc));

    jackClient = jack_client_open("FMidiAutomation", JackNullOption, NULL);

    assert(jackClient != NULL);

    jack_set_error_function(&error_impl);
    jack_set_process_callback(jackClient, &process_impl, this);
    jack_on_shutdown(jackClient, &jack_shutdown_impl, this);

    bool activated = (jack_activate(jackClient) == 0);
    assert(true == activated);    
}//constuctor

JackSingleton::~JackSingleton()
{
    jack_client_close(jackClient);
}//destructor

JackSingleton &JackSingleton::Instance()
{
    static JackSingleton jackSingleton;

    return jackSingleton;
}//Instance

int JackSingleton::process(jack_nframes_t nframes, void *arg)
{
    jack_position_t pos;
    jack_transport_state_t newTransportState = jack_transport_query(jackClient, &pos);

    {
        boost::mutex::scoped_lock lock(mutex);
        bool needsPotentialUpdate = false;

        int newFrame = (int)(((float)pos.frame) / ((float)pos.frame_rate) * 1000.0f);

        if ((curTransportState != newTransportState) || (newFrame != curFrame)) {
            needsPotentialUpdate = true;
        }//if

        curTransportState = newTransportState;
        curFrame = newFrame;

        if (true == needsPotentialUpdate) {
//            condition.notify_one();
        }//if
    }

    return 0;
}//process

void JackSingleton::error(const char *desc)
{
    //Nothing
}//error

void JackSingleton::jack_shutdown(void *arg)
{
    //Nothing
}//jack_shutdown

jack_transport_state_t JackSingleton::getTransportState()
{
    boost::mutex::scoped_lock lock(mutex);
    return curTransportState;
}//getTransportState

int JackSingleton::getTransportFrame()
{
    boost::mutex::scoped_lock lock(mutex);
    return curFrame;
}//getTransportFrame

void JackSingleton::setTransportState(jack_transport_state_t state)
{
    if (state == JackTransportRolling) {
        jack_transport_start(jackClient);
    }//if

    else if (state == JackTransportStopped) {
        jack_transport_stop(jackClient);
    }//if
}//setTransportState

void JackSingleton::setTime(int frame)
{
    jack_position_t pos;
    (void)jack_transport_query(jackClient, &pos);

    jack_nframes_t jackFrame = (((float)pos.frame_rate) / 1000.0f * frame);
    jack_transport_locate(jackClient, jackFrame);
}//setTime


