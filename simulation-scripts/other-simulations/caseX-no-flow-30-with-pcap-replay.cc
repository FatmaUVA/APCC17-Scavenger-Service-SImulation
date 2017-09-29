 /*
 *
 * TestDistributed creates a dumbbell topology and logically splits it in
 * half.  The left half is placed on logical processor 0 and the right half
 * is placed on logical processor 1.
 *
 *                 -------   -------
 *                  RANK 0    RANK 1
 *                 ------- | -------
 *                         |
 * n0 ---------|           |           |---------- n6
 *             |           |           |
 * n1 -------\ |           |           | /------- n7
 *            n4 ----------|---------- n5
 * n2 -------/ |           |           | \------- n8
 *             |           |           |
 * n3 ---------|           |           |---------- n9
 *
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/packet-sink-helper.h"

#include <string>
#include <fstream>
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/rng-seed-manager.h"
#include <iostream>
#include <list>
#include <iterator>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("sim1-one-queue-no-mpi");

void ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> flowMon,Ptr<OutputStreamWrapper> stream,Ptr<OutputStreamWrapper> caidaStream)
{
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
  {
    Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
//    *stream->GetStream ()<<"Throuhgput(Mbps) lost_rate(%) duration start end dest_port\n";
    if (fiveTuple.destinationPort == 10)
      *caidaStream->GetStream ()<< stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024 << " " <<((stats->second.lostPackets *1.0)/ stats->second.txPackets)*100<<" "<<stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()<<" "<<stats->second.timeFirstTxPacket.GetSeconds()<<" "<<stats->second.timeLastRxPacket.GetSeconds()<<" "<<fiveTuple.destinationPort<<std::endl;
    else if (fiveTuple.destinationPort >= 50000) 
      *stream->GetStream ()<< stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024 << " " <<((stats->second.lostPackets *1.0)/ stats->second.txPackets)*100<<" "<<stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()<<" "<<stats->second.timeFirstTxPacket.GetSeconds()<<" "<<stats->second.timeLastRxPacket.GetSeconds()<<" "<<fiveTuple.destinationPort<<" "<<stats->second.rxBytes/1024/1024 <<std::endl;
  }
}


uint64_t prevRxBytes=0;
uint32_t prevLostPkts=0;
uint32_t prevRxPkts=0;
void
LinkUtilization (Ptr<QueueDisc> rQ1, Ptr<OutputStreamWrapper> stream)
{
  uint64_t rxBytes = rQ1->GetTotalReceivedBytes () - prevRxBytes;
  uint32_t lostPkts = rQ1->GetTotalDroppedPackets () - prevLostPkts;
  double rxPkts = rQ1->GetTotalReceivedPackets () - prevRxPkts;
  double lostRate = (lostPkts/(rxPkts+lostPkts)*1.0)*100;
  double thru = (rxBytes* 8.0)/(1024*1024);
  if (thru<1000)
    *stream->GetStream () <<thru<<" "<<lostPkts<<" "<<lostRate<<std::endl;

  prevRxBytes = rQ1->GetTotalReceivedBytes ();
  prevLostPkts = rQ1->GetTotalDroppedPackets ();
  prevRxPkts = rQ1->GetTotalReceivedPackets ();

  Simulator::Schedule(Seconds(1),&LinkUtilization,rQ1,stream);
}

int
main (int argc, char *argv[])
{

  bool tracing = false;
  
  uint32_t maxBytes = 4000000000; //4GB
  std::string protocol = "TcpHtcp";
  std::string prefix_file_name = "caseX-SWS";
//CAIDA replay
//  std::string filename1 = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-1G.txt";
//  std::string filename2 = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-1000-sec-1G.txt";
  std::string filename1 = "/users/fha6np/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-1G.txt";
  std::string filename2 = "/users/fha6np/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-1000-sec-1G.txt";
  std::string filename = filename1;
//Qdisc setup:
  std::string queueDiscType = "PfifoFast";//PfifoFast or CoDel
  uint32_t queueSize = 5000;//in packets
  double simTime = 10.0; //in seconds
  uint64_t tcpBuff = 500000; //in bytes
  double eMean = 20.0; //mean flow inter-arrival time
  uint32_t efCount=30; //number of EF senders
  uint32_t randSeed = 1;
  std::string caseX = "B";
  std::string log_dir = "6-1-2017-results";
  std::string service = "SWS";

  // Parse command line
  CommandLine cmd;
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  
  cmd.AddValue ("maxBytes", "Total number of bytes for application to send",maxBytes );
  cmd.AddValue ("protocol", "Flag to choose TCP variant", protocol);
  cmd.AddValue ("queueSize", "FLoag to specify the queue size",queueSize);
  cmd.AddValue ("simTime", "Simulation Time",simTime);
  cmd.AddValue ("file_name", "File name",prefix_file_name);
  cmd.AddValue ("tcpBuff" , "TCP buffer size", tcpBuff);
  cmd.AddValue ("eMean", "mean flow inter-arrival time", eMean);
  cmd.AddValue ("efCount", "Number of EF senders",efCount);
  cmd.AddValue ("randSeed", "Seed value to random number generation", randSeed);
  cmd.AddValue ("caseX", "case to simulate A or B",caseX);
  cmd.AddValue ("log_dir", "logging directory", log_dir);
  cmd.AddValue ("service", "SWS or BE", service);
  cmd.Parse (argc, argv);

//================================================================================
//*********************Intialization and general configuration*******************
//================================================================================
  if (service == "BE")
    queueSize = 10000;

  //Router tc layer config  
  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (queueSize));

//Node tc layer config
  TrafficControlHelper tchPfifo1;
  tchPfifo1.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (10000));

//netdev config
 Config::SetDefault ("ns3::Queue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
 Config::SetDefault ("ns3::Queue::MaxPackets", UintegerValue (queueSize));


//TCP settings
  if( caseX == "A")
    tcpBuff = 45000000; //27000000;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1460));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (tcpBuff));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (tcpBuff));

  if(protocol == "TcpNewReno")
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  else
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHtcp::GetTypeId ()));

//================================================================================
//**************************** Create Topology *******************************
//================================================================================
//

  // Create router nodes.  Left router
  // with system id 0, right router with
  // system id 1
  NodeContainer routerNodes;
  Ptr<Node> r1 = CreateObject<Node> ();
  Ptr<Node> r2 = CreateObject<Node> ();
  routerNodes.Add (r1);
  routerNodes.Add (r2);


  NodeContainer sendHostsContainer;
  NodeContainer recvHostsContainer;


// crete nodes based on the interarrival flows
// Random variable stream for file size and interarrival time
//  double e_mean=20.0;
  ns3::RngSeedManager::SetSeed(randSeed);
  Ptr<ExponentialRandomVariable> ev = CreateObject<ExponentialRandomVariable> ();
  ev->SetAttribute ("Mean", DoubleValue (eMean));
  ev->SetAttribute( "Stream", IntegerValue( 1 ) );
  double total_time=0, inter_time;
  std::list<double> inter_time_list;
  
  uint32_t noSendRecv=0;// count the number of sender or receiver, sender=receiver so that one counter is kept

//first create send and receive nodes for CAIDA
  sendHostsContainer.Add ( CreateObject<Node> ());
  recvHostsContainer.Add ( CreateObject<Node> ());
  noSendRecv++;

// create more nodes based on inter arrival time of the TCP flows
//  uint32_t efCount=30;
  inter_time = 0; //start first flow at time 0
  total_time = total_time + inter_time;
  for (uint32_t i =0; i<efCount;i++)
//  while (total_time <= simTime)
  {
    std::cout<<"inter-time = "<<inter_time<<std::endl;
    inter_time_list.push_back(total_time);
    noSendRecv++;
    sendHostsContainer.Add ( CreateObject<Node> ());
    recvHostsContainer.Add ( CreateObject<Node> ());
    inter_time = ev->GetValue ();
    total_time = total_time + inter_time;
  }

  simTime = total_time + 5; //add more 5 seconds to allow the last flow to grow
  std::cout<<"simulation time ="<<simTime<<std::endl;
  std::cout<<"total number of senders = "<< noSendRecv<<std::endl;

  //if sim time > 600 use the extended pcap file
  if (simTime > 600)
    filename = filename2;

  PointToPointHelper routerLink;
  routerLink.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  routerLink.SetChannelAttribute ("Delay", StringValue ("5ms"));

  std::string hLink = "100Mbps";
  if (caseX == "A")
    hLink = "1Gbps";
   
  PointToPointHelper hostLink;
  hostLink.SetDeviceAttribute ("DataRate", StringValue (hLink));
  hostLink.SetChannelAttribute ("Delay", StringValue ("2ms"));

  InternetStackHelper stack;
  stack.InstallAll ();

  // Add link connecting routers and setup qdisc
  NetDeviceContainer routerDevices;
  routerDevices = routerLink.Install (routerNodes);
  //pointers for qdiscs at the routers
  QueueDiscContainer rQdisc1 = tchPfifo.Install (routerDevices.Get (0));
  Ptr<QueueDisc> rQ1 = rQdisc1.Get (0);
  QueueDiscContainer rQdisc2 = tchPfifo.Install (routerDevices.Get (1));
  Ptr<QueueDisc> rQ2 = rQdisc2.Get (0);

  // Add links for send nodes to left router
  NetDeviceContainer leftRouterDevices;
  NetDeviceContainer sendHostsDevices;
  QueueDiscContainer sendHostsQdiscs;
  for (uint32_t i = 0; i < noSendRecv; ++i)
    {
      NetDeviceContainer temp;
      if (i==0) //CAIDA sender will have diffrent link rate
        temp = routerLink.Install (sendHostsContainer.Get (i), routerNodes.Get (0)); 
      else
        temp = hostLink.Install (sendHostsContainer.Get (i), routerNodes.Get (0));
      sendHostsDevices.Add (temp.Get (0));
      leftRouterDevices.Add (temp.Get (1));
      sendHostsQdiscs.Add (tchPfifo1.Install (sendHostsDevices.Get (i)));//install qdiscs at sender to control tc buffer size
    }

  // Add links for right side leaf nodes to right router
  NetDeviceContainer rightRouterDevices;
  NetDeviceContainer recvHostsDevices;
  QueueDiscContainer rightRouterQdiscs;
  for (uint32_t i = 0; i < noSendRecv; ++i)
    {
      NetDeviceContainer temp ;
      if (i==0) //CAIDA receiver different link rate
        temp = routerLink.Install (recvHostsContainer.Get (i), routerNodes.Get (1));
      else
        temp = hostLink.Install (recvHostsContainer.Get (i), routerNodes.Get (1));
      recvHostsDevices.Add (temp.Get (0));
      rightRouterDevices.Add (temp.Get (1));
     //add qdiscs at the router where the receviers are connected to control the router buffer size
     rightRouterQdiscs.Add (tchPfifo.Install (rightRouterDevices.Get (i)));
      // to access on eo f the queuediscs use Ptr<QueueDisc> queue1 = recvHostsQdiscs.Get (1);
    }

//================================================================================
//********************* Assign IP adresses to the network links  ****************
//================================================================================


  Ipv4InterfaceContainer routerInterfaces;
  Ipv4InterfaceContainer leftLeafInterfaces;
  Ipv4InterfaceContainer leftRouterInterfaces;
  Ipv4InterfaceContainer rightLeafInterfaces;
  Ipv4InterfaceContainer rightRouterInterfaces;

  Ipv4AddressHelper leftAddress;
  leftAddress.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4AddressHelper routerAddress;
  routerAddress.SetBase ("10.2.1.0", "255.255.255.0");

  Ipv4AddressHelper rightAddress;
  rightAddress.SetBase ("10.3.1.0", "255.255.255.0");

  // Router-to-Router interfaces
  routerInterfaces = routerAddress.Assign (routerDevices);

  // Left interfaces
  for (uint32_t i = 0; i < noSendRecv; ++i)
    {
      NetDeviceContainer ndc;
      ndc.Add (sendHostsDevices.Get (i));
      ndc.Add (leftRouterDevices.Get (i));
      Ipv4InterfaceContainer ifc = leftAddress.Assign (ndc);
      leftLeafInterfaces.Add (ifc.Get (0));
      leftRouterInterfaces.Add (ifc.Get (1));
      leftAddress.NewNetwork ();
    }

  // Right interfaces
  for (uint32_t i = 0; i < noSendRecv; ++i)
    {
      NetDeviceContainer ndc;
      ndc.Add (recvHostsDevices.Get (i));
      ndc.Add (rightRouterDevices.Get (i));
      Ipv4InterfaceContainer ifc = rightAddress.Assign (ndc);
      rightLeafInterfaces.Add (ifc.Get (0));
      rightRouterInterfaces.Add (ifc.Get (1));
      rightAddress.NewNetwork ();
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//================================================================================
//********************* Application setup at each node *******************
//================================================================================

  
// Random variable stream for file size and interarrival time
  double p_mean = 200.0;
  double p_shape = 4.0;

  if (caseX == "A")
    p_mean = 2000.0; //2GB

  Ptr<ParetoRandomVariable> pv = CreateObject<ParetoRandomVariable> ();
  pv->SetAttribute ("Mean", DoubleValue (p_mean));
  pv->SetAttribute ("Shape", DoubleValue (p_shape));
  pv->SetAttribute( "Stream", IntegerValue( 1 ) );

  uint16_t port = 50000;
  uint8_t tos;
  if ( service == "SWS")
    tos = 0x10; // 0x08 band 2(low prio) and 0x10 band 0(high prio)


//  The first node will have the CAIDA replay app
//
// Create a Udp Replay application and install it on sender1
//
   // tos = 0x10; //band 0
    InetSocketAddress destAddress2 (rightLeafInterfaces.GetAddress (0), 10);
  if ( service == "SWS")
  {
    tos = 0x10; //band 0
    destAddress2.SetTos (tos);
  }
    UdpReplayTraceClientHelper traceClient  (destAddress2, filename);
    traceClient.SetAttribute ("MaxPacketSize", UintegerValue (63934));
    ApplicationContainer caidaApp = traceClient.Install (sendHostsContainer.Get (0));
    caidaApp.Start (Seconds (0.1));
    caidaApp.Stop (Seconds (simTime));

//
// Create a Udp Replay application receiver and install it on recv1
//
    PacketSinkHelper sink2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 10));
    ApplicationContainer sinkApps2 = sink2.Install (recvHostsContainer.Get (0));
    sinkApps2.Start (Seconds (0.0));
    sinkApps2.Stop (Seconds (simTime));

//  tos = 0x08;
//  Create send applications on the send hosts
  uint32_t i = 1;//the first host /send or receive) has applications already installed to them
  for (std::list<double>::iterator it = inter_time_list.begin(); it != inter_time_list.end(); it++)
  {
    port++;
      InetSocketAddress destAddress (rightLeafInterfaces.GetAddress (i), port);
      if ( service == "SWS")
      {
        tos = 0x08; //band 3
        destAddress2.SetTos (tos);
      }
      BulkSendHelper clientHelper ("ns3::TcpSocketFactory", destAddress);
      // Set the amount of data to send in bytes.  Zero is unlimited.
      uint32_t fSize=pv->GetValue () *1000000;
//      std::cout <<"I am a sender with i = "<<i<<" File size = "<<fSize<<std::endl;
      clientHelper.SetAttribute ("MaxBytes", UintegerValue (fSize));//change it from MB to bytes
      ApplicationContainer clientApp = clientHelper.Install (sendHostsContainer.Get (i));
      clientApp.Start (Seconds(*it));
      clientApp.Stop (Seconds(simTime));

//      std::cout <<"I am a recv with i = "<<i<<std::endl;
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (recvHostsContainer.Get (i));
      sinkApp.Start (Seconds(0.0));
      sinkApp.Stop (Seconds(simTime));
    i++;
  }

//================================================================================
//************************** Monitoring and tracing Setup ************************
//================================================================================
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();

  std::string uFileName;
  std::string flowFileName;
  std::string caidaFileName;
  
  uFileName = log_dir + "/eMean-" + std::to_string(eMean) + "/utilization-case" + caseX + "-" + service + "-eMean-" + std::to_string(eMean) + "-seed-" + std::to_string(randSeed) + ".txt";
  flowFileName = log_dir + "/eMean-" + std::to_string(eMean) + "/parsed-case" + caseX + "-" + service + "-eMean-" + std::to_string(eMean) + "-seed-" + std::to_string(randSeed) + ".txt";
  caidaFileName = log_dir + "/eMean-" + std::to_string(eMean) + "/caida-loss-case" + caseX + "-" + service + "-eMean-" + std::to_string(eMean)+ "-seed-" + std::to_string(randSeed) + ".txt";
  
  Ptr<OutputStreamWrapper> uStream = Create<OutputStreamWrapper> (uFileName, std::ios::out);
  Ptr<OutputStreamWrapper> flowStream = Create<OutputStreamWrapper> (flowFileName, std::ios::out);
  Ptr<OutputStreamWrapper> caidaStream = Create<OutputStreamWrapper> (caidaFileName, std::ios::out);

  if(tracing)
  {
    routerLink.EnablePcap("router-left", routerDevices, true);
    routerLink.EnablePcap("router-right", routerDevices, true);
  }

//================================================================================
//************************** RUN THE SIMULATION ************************
//================================================================================
  Simulator::Schedule(Seconds(simTime-0.000000001),&ThroughputMonitor, &fmHelper, allMon, flowStream, caidaStream);
  Simulator::Schedule(Seconds(1),&LinkUtilization, rQ1, uStream);
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
    

  std::cout<<"The dropped packts by TC layer at router1 is: "<<rQ1->GetTotalDroppedPackets () << std::endl;
  std::cout<<"The received packets by TC layer at router1 is: "<<rQ1->GetTotalReceivedPackets () << std::endl;
  std::cout<<"The received bytes by TC layer at router1 is: "<<rQ1->GetTotalReceivedBytes () << std::endl;

//  fmHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", false,false);

  for (uint32_t j=0; j < noSendRecv; j++)
  {
      Ptr<QueueDisc> hostQ = sendHostsQdiscs.Get (j);
      std::cout<<"The dropped packts by TC layer at sender with id "<<j<<" is: "<<hostQ->GetTotalDroppedPackets () << std::endl;
      std::cout<<"The received packets by TC layer sender with id "<<j<<" is: "<<hostQ->GetTotalReceivedPackets () << std::endl;
      std::cout<<"The received bytes by TC layer sender with id "<<j<<" is: "<<hostQ->GetTotalReceivedBytes () << std::endl;
  }

  return 0;
}
