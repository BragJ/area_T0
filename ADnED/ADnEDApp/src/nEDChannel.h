/**
 * @brief nED V4 channel code to subscribe to V4 PV updates
 *
 * @author Matt Pearson
 * @date Sept 2014
 */

#ifndef NEDCHANNEL_H
#define NEDCHANNEL_H
#include <iostream>
#include <epicsThread.h>
#include <epicsTime.h>
#include <pv/epicsException.h>
#include <pv/createRequest.h>
#include <pv/event.h>
#include <pv/pvData.h>
#include <pv/clientFactory.h>
#include <pv/pvAccess.h>
#include <pv/monitor.h>

class ADnED;

typedef struct V4PulseData{
	//int client_pusle_id;
	int *client_tof;
	int *client_pixel_id;
}V4PulseData;

namespace nEDChannel {


  using std::tr1::shared_ptr;
  using namespace epics::pvData;
  using namespace epics::pvAccess;
 
  //Channel connect/disconnect class

  class nEDChannelRequester : public virtual ChannelRequester {

  public:
    
    nEDChannelRequester(std::string &requester_name);
    virtual ~nEDChannelRequester();

    void channelCreated(const Status& status, Channel::shared_pointer const & channel);
    void channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState);
    bool waitUntilConnected(double timeOut);
    std::string getRequesterName();
    void message(std::string const & message, MessageType messageType);
    
  private:
    
    std::string m_requesterName;
    Event m_connectEvent;
    
  };
 
  //Channel monitor class

  class nEDMonitorRequester : public virtual MonitorRequester
  {
    
  public:
    
    nEDMonitorRequester(std::string &requester_name, ADnED *nED, epicsUInt32 channel);
    virtual ~nEDMonitorRequester();
    
    void monitorConnect(Status const & status, MonitorPtr const & monitor, StructureConstPtr const & structure);
    void monitorEvent(MonitorPtr const & monitor);
    //void eventHandlerClient(shared_ptr<epics::pvData::PVStructure> const &pv_struct_client);
    void unlisten(MonitorPtr const & monitor);
    bool waitUntilConnected(double timeOut);
    boolean waitUntilDone();
   


    std::string getRequesterName();
    void message(std::string const & message, MessageType messageType);

 private:
    Event m_connectEvent;
    std::string m_requesterName;
    ADnED *p_nED;
    Event m_doneEvent;
    epicsUInt32 m_channelID;

  };


 
}

#endif //NEDCHANNEL_H





