#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "node-app.h"
#include "stdlib.h"
#include "ns3/ipv4.h"
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NodeApp");

NS_OBJECT_ENSURE_REGISTERED (NodeApp);

TypeId NodeApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NodeApp")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<NodeApp> ()
    ;

    return tid;
}

NodeApp::NodeApp(void) {

}

NodeApp::~NodeApp(void) {
    NS_LOG_FUNCTION (this);
}

float getRandomDelay() {
  return (rand() % 3) * 1.0 / 1000;
}

void NodeApp::StartApplication () {


    // Initialize socket
    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket (GetNode (), tid);

        // Note: This is equivalent to monitoring all network card IP addresses.
        InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 7071);
        m_socket->Bind (local);           // Bind the local IP and port
        m_socket->Listen ();
    }
    m_socket->SetRecvCallback (MakeCallback (&NodeApp::HandleRead, this));
    m_socket->SetAllowBroadcast (true);

    std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();
    // Establish connections to all nodes
    NS_LOG_INFO("node"<< m_id << " start");
    while(iter != m_peersAddresses.end()) {
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        Ptr<Socket> socketClient = Socket::CreateSocket (GetNode (), tid);
        socketClient->Connect (InetSocketAddress(*iter, 7071));
        m_peersSockets[*iter] = socketClient;
        iter++;
    }

    if (is_leader == 1) {
        Simulator::Schedule (Seconds(getRandomDelay()), &NodeApp::sendMessage, this);
    }

}

void NodeApp::sendMessage(void) {
  std::string myString = "Hello Blockchain";
  std::vector<uint8_t> myVector(myString.begin(), myString.end());
  uint8_t *data = &myVector[0];
  NodeApp::SendTX(data, sizeof(myString));
}

void NodeApp::StopApplication ()
{
  if (is_leader == 1) {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " Stop");
  }
}

void NodeApp::HandleRead (Ptr<Socket> socket)
{ 
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom (from)))
    {
        socket->SendTo(packet, 0, from);
        if (packet->GetSize () == 0)
        {
            break;
        }
        if (InetSocketAddress::IsMatchingType (from))
        {
            std::string msg = getPacketContent(packet, from);

             NS_LOG_INFO("Node " << GetNode ()->GetId () << " Received Message: " << msg);

        }
        socket->GetSockName (localAddress);
    }
}

std::string NodeApp::getPacketContent(Ptr<Packet> packet, Address from)
{ 
    char *packetInfo = new char[packet->GetSize () + 1];
    std::ostringstream totalStream;
    packet->CopyData (reinterpret_cast<uint8_t*>(packetInfo), packet->GetSize ());
    packetInfo[packet->GetSize ()] = '\0';
    totalStream << m_bufferedData[from] << packetInfo; 
    std::string totalReceivedData(totalStream.str());
    return totalReceivedData;
}  

void SendPacket(Ptr<Socket> socketClient,Ptr<Packet> p) {
    socketClient->Send(p);
}

void NodeApp::SendTX (uint8_t data[], int size) {
  NS_LOG_INFO("broadcast message at time: " << Simulator::Now ().GetSeconds () << " s");

  Ptr<Packet> p;

  p = Create<Packet> (data, size);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");


  std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();

  while(iter != m_peersAddresses.end()) {
    TypeId tId = TypeId::LookupByName ("ns3::UdpSocketFactory");

    Ptr<Socket> socketClient = m_peersSockets[*iter];
    double delay = getRandomDelay();

    Simulator::Schedule(Seconds(delay), SendPacket, socketClient, p);
    iter++;
  }
}
}