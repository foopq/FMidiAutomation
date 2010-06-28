#include "jack.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include "FMidiAutomationMainWindow.h"
#include <boost/serialization/vector.hpp>

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

//    jack_set_error_function(&error_impl); -- causes reentrant issues
    jack_set_process_callback(jackClient, &process_impl, this);
    jack_on_shutdown(jackClient, &jack_shutdown_impl, this);

    //input_port = jack_port_register (jackClient, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    //output_port = jack_port_register (jackClient, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    std::vector<std::string> inputPort;
    inputPort.push_back("midi_in");

    std::vector<std::string> outputPort;
    outputPort.push_back("midi_out");

    setInputPorts(inputPort);
    setOutputPorts(outputPort);

    bool activated = (jack_activate(jackClient) == 0);
    assert(true == activated);    

    recordMidi = false;
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

std::string JackSingleton::getOutputPortName(jack_port_t *port)
{
    for (std::map<std::string, jack_port_t *>::const_iterator iter = outputPorts.begin(); iter != outputPorts.end(); ++iter) {
        if (iter->second == port) {
            return iter->first;
        }//if
    }//for

    return "";
}//getOutputPortName

std::string JackSingleton::getInputPortName(jack_port_t *port)
{
    for (std::map<std::string, jack_port_t *>::const_iterator iter = inputPorts.begin(); iter != inputPorts.end(); ++iter) {
        if (iter->second == port) {
            return iter->first;
        }//if
    }//for

    return "";
}//getInputPortName

std::vector<std::string> JackSingleton::getInputPorts()
{
    boost::mutex::scoped_lock lock(mutex);

    std::vector<std::string> inputPortsVec;

    for (std::map<std::string, jack_port_t *>::const_iterator iter = inputPorts.begin(); iter != inputPorts.end(); ++iter) {
        inputPortsVec.push_back(iter->first);
    }//for

    return inputPortsVec;
}//getInputPorts

void JackSingleton::setInputPorts(std::vector<std::string> ports)
{
    boost::mutex::scoped_lock lock(mutex);

    std::vector<std::string> inputPortsVec;

    for (std::map<std::string, jack_port_t *>::const_iterator iter = inputPorts.begin(); iter != inputPorts.end(); ++iter) {
        inputPortsVec.push_back(iter->first);
    }//for

    std::vector<std::string> newPorts;
    std::vector<std::string> removedPorts;

    std::set_difference(ports.begin(), ports.end(), inputPortsVec.begin(), inputPortsVec.end(), std::back_inserter(newPorts));
    std::set_difference(inputPortsVec.begin(), inputPortsVec.end(), ports.begin(), ports.end(), std::back_inserter(removedPorts));

    BOOST_FOREACH (std::string portName, removedPorts) {
        jack_port_t *port = inputPorts[portName];
        jack_port_unregister(jackClient, port);
        inputPorts.erase(inputPorts.find(portName));
    }//foreach

    BOOST_FOREACH (std::string portName, newPorts) {
        jack_port_t *newInputPort = jack_port_register(jackClient, portName.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        inputPorts[portName] = newInputPort;
    }//foreach
}//setInputPorts

std::vector<std::string> JackSingleton::getOutputPorts()
{
    boost::mutex::scoped_lock lock(mutex);

    std::vector<std::string> outPortsVec;

    for (std::map<std::string, jack_port_t *>::const_iterator iter = outputPorts.begin(); iter != outputPorts.end(); ++iter) {
        outPortsVec.push_back(iter->first);
    }//for

    return outPortsVec;
}//getOutputPorts

void JackSingleton::setOutputPorts(std::vector<std::string> ports)
{
    boost::mutex::scoped_lock lock(mutex);

    std::vector<std::string> outputPortsVec;

    for (std::map<std::string, jack_port_t *>::const_iterator iter = outputPorts.begin(); iter != outputPorts.end(); ++iter) {
        outputPortsVec.push_back(iter->first);
    }//for

    std::vector<std::string> newPorts;
    std::vector<std::string> removedPorts;

    std::set_difference(ports.begin(), ports.end(), outputPortsVec.begin(), outputPortsVec.end(), std::back_inserter(newPorts));
    std::set_difference(outputPortsVec.begin(), outputPortsVec.end(), ports.begin(), ports.end(), std::back_inserter(removedPorts));

    BOOST_FOREACH (std::string portName, removedPorts) {
        jack_port_t *port = outputPorts[portName];
        jack_port_unregister(jackClient, port);
        outputPorts.erase(outputPorts.find(portName));
    }//foreach

    BOOST_FOREACH (std::string portName, newPorts) {
        jack_port_t *newOutputPort = jack_port_register(jackClient, portName.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        outputPorts[portName] = newOutputPort;
    }//foreach
}//setOutputPorts

jack_port_t *JackSingleton::getOutputPort(const std::string &portName)
{
    if (outputPorts.find(portName) != outputPorts.end()) {
        return outputPorts[portName];
    } else {
        return NULL;
    }//if
}//getOutputPort

jack_port_t *JackSingleton::getInputPort(const std::string &portName)
{
    if (inputPorts.find(portName) != inputPorts.end()) {
        return inputPorts[portName];
    } else {
        return NULL;
    }//if
}//getInputPort

void JackSingleton::setRecordMidi(bool record)
{
    if (true == record) {
        midiRecordBuffer.clear();
        midiRecordBuffer.reserve(1024 * 1024 * 10);

        midiRecordBufferHeaders.clear();
        midiRecordBufferHeaders.reserve(10000);
    }//if

    recordMidi = record;
}//setRecordMidi

std::vector<unsigned char> &JackSingleton::getRecordBuffer()
{
    return midiRecordBuffer;
}//getRecordBuffer

std::vector<MidiInputInfoHeader> &JackSingleton::getMidiRecordBufferHeaders()
{ 
    return midiRecordBufferHeaders;
}//getMidiRecordBufferHeaders

int JackSingleton::process(jack_nframes_t nframes, void *arg)
{
    boost::mutex::scoped_lock lock(mutex);

    jack_position_t pos;
    jack_transport_state_t newTransportState = jack_transport_query(jackClient, &pos);

//    void* port_buf = jack_port_get_buffer(input_port, nframes);
//    void* port_buf_out = jack_port_get_buffer(output_port, nframes);
//    jack_midi_event_t in_event;
//    jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

//    jack_midi_clear_buffer(port_buf_out);

    jack_nframes_t frameRate = pos.frame_rate;
    int newFrame = (int)(((float)pos.frame) / ((float)frameRate) * 1000.0f);

    //Transport
    {
        bool needsPotentialUpdate = false;

        if ((curTransportState != newTransportState) || (newFrame != curFrame)) {
            needsPotentialUpdate = true;
        }//if

        curTransportState = newTransportState;
        curFrame = newFrame;

        if (true == needsPotentialUpdate) {
//            condition.notify_one();
        }//if
    }//Transport

    //Record
    {
        if (true == recordMidi) {
            jack_midi_event_t in_event;
            for (std::map<std::string, jack_port_t *>::const_iterator portIter = inputPorts.begin(); portIter != inputPorts.end(); ++portIter) {
                void *port_buf = jack_port_get_buffer(portIter->second, nframes);
                jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

                if (0 < event_count) {
                    for(unsigned int i=0; i<event_count; i++) {
                        jack_midi_event_get(&in_event, port_buf, i);

                        //Copy all but footer and checksum
                        if (in_event.size > 2) {
                            int eventFrame = (int)(((float)in_event.time) / ((float)frameRate) * 1000.0f);

                            MidiInputInfoHeader header;
                            header.port = portIter->second;
                            header.curFrame = eventFrame + newFrame;
                            header.bufferPos = midiRecordBuffer.size();
                            header.length = in_event.size;

                            midiRecordBufferHeaders.push_back(header);
                            midiRecordBuffer.insert(midiRecordBuffer.end(), (unsigned char*)in_event.buffer, (unsigned char *)(in_event.buffer + in_event.size));
                        }//if
                    }//for
                }//if            
            }//for
        }//if
    }//Record

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

void JackSingleton::doLoad(boost::archive::xml_iarchive &inputArchive)
{
    std::vector<std::string> inputPorts;
    std::vector<std::string> outputPorts;

    inputArchive & BOOST_SERIALIZATION_NVP(inputPorts);
    inputArchive & BOOST_SERIALIZATION_NVP(outputPorts);

    setInputPorts(inputPorts);
    setOutputPorts(outputPorts);
}//doLoad

void JackSingleton::doSave(boost::archive::xml_oarchive &outputArchive)
{
    std::vector<std::string> inputPorts = getInputPorts();
    std::vector<std::string> outputPorts = getOutputPorts();

    outputArchive & BOOST_SERIALIZATION_NVP(inputPorts);
    outputArchive & BOOST_SERIALIZATION_NVP(outputPorts);
}//doSave


