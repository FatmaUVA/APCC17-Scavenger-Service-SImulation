/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology
//            net1      net2
//       n1 -------[R1]------- n2
//             
//        	500 Kbps
//                5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include <iostream>
#include "ns3/PPBP-application.h"
#include "ns3/PPBP-helper.h"

using namespace ns3;
uint32_t startTime = 0, prevLost1 = 0;
uint64_t prevRxBytes1 = 0;
double prevTxpkt1 = 0;
double thru_interval=0.2;

NS_LOG_COMPONENT_DEFINE ("Expt1");

void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Ptr<OutputStreamWrapper> stream)
{
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin ();
//  Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
//  std::cout<<"Flow ID : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
    double Thr1 = (((stats->second.rxBytes-prevRxBytes1) * 8.0)/(1024*1024))/thru_interval;
    double Bytes = (stats->second.rxBytes-prevRxBytes1);
    double lostPkt1 = stats->second.lostPackets - prevLost1;
    double lostRate1 = (lostPkt1 / (stats->second.txPackets - prevTxpkt1))*100;
    prevRxBytes1=stats->second.rxBytes;
    prevLost1 = stats->second.lostPackets;
    prevTxpkt1 = stats->second.txPackets;
//    std::cout<<Thr1<<" "<<lostPkt1<<" "<<lostRate1<<std::endl;
    startTime=ns3::Simulator::Now().GetSeconds();
//    std::cout<<Bytes<<" "<<Thr1<<" "<<lostPkt1<<" "<<lostRate1<<std::endl;   
    *stream->GetStream ()<<Bytes<<" "<<Thr1<<" "<<lostPkt1<<" "<<lostRate1<<std::endl;
  Simulator::Schedule(Seconds(thru_interval),&ThroughputMonitor, fmhelper, flowMon, stream);
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
  double lostRate = (lostPkts/rxPkts*1.0)*100;
  double thru = ((rxBytes* 8.0)/(1024*1024))/thru_interval;
//  if (thru<1000)
//    *stream->GetStream () <<thru<<" "<<lostPkts<<" "<<lostRate<<std::endl;
  *stream->GetStream () <<rxBytes<<" "<<thru<<" "<<lostPkts<<" "<<lostRate<<std::endl;

  prevRxBytes = rQ1->GetTotalReceivedBytes ();
  prevLostPkts = rQ1->GetTotalDroppedPackets ();
  prevRxPkts = rQ1->GetTotalReceivedPackets ();

  Simulator::Schedule(Seconds(thru_interval),&LinkUtilization,rQ1,stream);
}

int
main (int argc, char *argv[])
{
//  bool flow_monitor = true;
  bool tracing = false;
  uint32_t maxBytes = 0;
  std::string protocol = "TcpHtcp";
  std::string prefix_file_name = "sender-router-recv-monitor-per-sec";
//Qdisc setup:
  std::string queueDiscType = "PfifoFast";       //PfifoFast or CoDel
  uint32_t queueSize = 10000;//in packets
  double simTime = 20.0; //in seconds
  uint64_t tcpBuff = 625000;
  double mean_flow_arrival=20;
  std::string mean_rate="100Mbps";
  std::string FileName="traffic-gen";
//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes", "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("protocol", "Flag to choose TCP variant", protocol);
  cmd.AddValue("queueSize", "FLoag to specify the queue size",queueSize);
  cmd.AddValue("simTime", "Simulation Time",simTime);
  cmd.AddValue("file_name", "File name",prefix_file_name);
  cmd.AddValue("tcpBuff" , "TCP buffer size", tcpBuff);
  cmd.AddValue("mean_flow_arrival","Burst (flows) mean arrival rate",mean_flow_arrival);
  cmd.AddValue("mean_rate","Mean flow rate (CRB)",mean_rate);
  cmd.AddValue("FileName","FIle name to save per time period burst",FileName);
  cmd.Parse (argc, argv);


//Router tc layer config  
  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (queueSize));

//Node tc layer config
  TrafficControlHelper tchPfifo1;
  tchPfifo1.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (10000));
//  tchPfifo1.SetRootQueueDisc ("ns3::TbfQueueDisc", "Limit", IntegerValue (50000),"Burst",DoubleValue (50000),"Rate",DoubleValue (20000000));

//netdev config
 Config::SetDefault ("ns3::Queue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
 Config::SetDefault ("ns3::Queue::MaxPackets", UintegerValue (queueSize));


//TCP settings
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1446));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (tcpBuff));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (tcpBuff));

  if(protocol == "TcpNewReno")  
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  else
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHtcp::GetTypeId ()));

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> n1 = CreateObject<Node> ();
  Names::Add ("node1", n1);
  Ptr<Node> n2 = CreateObject<Node> ();
  Names::Add ("node2", n2);
  Ptr<Node> r = CreateObject<Node> ();
  Names::Add ("router", r);

  NodeContainer net1 (n1, r);
  NodeContainer net2 (r, n2);
  
  NodeContainer routers (r);
  NodeContainer nodes (n1,n2);

  NS_LOG_INFO ("Create channels.");

//
// Explicitly create the point-to-point link required by the topology (shown above).
//
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer ndc1 = pointToPoint2.Install (net1);
  NetDeviceContainer ndc2 = pointToPoint2.Install (net2);

/*
// add error model to simulate packet loss:q
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.0001));
  ndc1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
*/
//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install (nodes);
  internet.Install (routers);

////add dqisc to the router netDevice
//make sure to install the qdisc after installing the Internet stack and before assigining IP address
//  tchPfifo.Install (ndc2.Get (0));
// Pointers to trace tc layer queues
 
//router link net2 
  QueueDiscContainer qdiscs2 = tchPfifo.Install (ndc2.Get (0));
  Ptr<QueueDisc> q2 = qdiscs2.Get (0);
//node 2
  QueueDiscContainer qdiscs3 = tchPfifo1.Install (ndc2.Get (1));
  Ptr<QueueDisc> q3 = qdiscs3.Get (0);
//node 1
  QueueDiscContainer qdiscs1 = tchPfifo1.Install (ndc1.Get (0));
  Ptr<QueueDisc> q1 = qdiscs1.Get (0);

  Ptr<NetDevice> nd2 = ndc2.Get (0);
  Ptr<PointToPointNetDevice> ptpnd2 = DynamicCast<PointToPointNetDevice> (nd2);
  Ptr<Queue> queue2 = ptpnd2->GetQueue ();

  Ptr<NetDevice> nd1 = ndc1.Get (0);
  Ptr<PointToPointNetDevice> ptpnd1 = DynamicCast<PointToPointNetDevice> (nd1);
  Ptr<Queue> queue1 = ptpnd2->GetQueue ();

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = ipv4.Assign (ndc1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = ipv4.Assign (ndc2);

  NS_LOG_INFO ("Create Applications.");

//
// Create a BulkSendApplication and install it on node 1
//

  uint16_t port = 9;  // well-known echo port number
//  uint8_t tos = 0x08; //band 2


  InetSocketAddress destAddress (i2.GetAddress (1), port);
//  OnOffHelper source ("ns3::UdpSocketFactory", destAddress);
//  source.SetAttribute ("DataRate",  StringValue ("200Mbps"));
//  source.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//  source.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  PPBPHelper source ("ns3::UdpSocketFactory", destAddress);
  source.SetAttribute ("BurstIntensity",  StringValue (mean_rate));
  source.SetAttribute ("MeanBurstArrivals",  DoubleValue (mean_flow_arrival));
//  source.SetAttribute ("MeanBurstTimeLength",  DoubleValue (1));

  ApplicationContainer sourceApp = source.Install (nodes.Get (0));
  sourceApp.Start (Seconds(0.0));
  sourceApp.Stop (Seconds(simTime));
// Create two packetSInk applications and install it on node 2
//

//  PacketSinkHelper sink ("ns3::TcpSocketFactory",
//                         InetSocketAddress (Ipv4Address::GetAny (), port));
PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (simTime));
   

// ======================================================================
// Calculate and populate routing tables
// ----------------------------------------------------------------------
  NS_LOG_INFO ("L3: Populate routing tables.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//
// Set up tracing if enabled
//
  if (tracing)
    {
      AsciiTraceHelper ascii;
      pointToPoint2.EnableAsciiAll (ascii.CreateFileStream ("sender-router-recv-monitor-per-sec.tr"));
      pointToPoint.EnablePcapAll ("sender-router-recv-monitor-per-sec", true);
    }


///
// FlowMonitor
///
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();

//tracing thru
  Ptr<OutputStreamWrapper> uStream = Create<OutputStreamWrapper> (FileName, std::ios::out);
//  std::cout<<"Burst(Bytes) HTCP-Throuhgput(Mbps) HTCP-lost-packets HTCP-lost-rate(%)\n";
 *uStream->GetStream ()<< "Burst(Bytes) HTCP-Throuhgput(Mbps) HTCP-lost-packets HTCP-lost-rate(%)\n";
// Simulator::Schedule(Seconds(thru_interval),&ThroughputMonitor,&fmHelper, allMon,uStream);
 Simulator::Schedule(Seconds(thru_interval),&LinkUtilization, q2, uStream);
//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  std::cout << std::endl << "*** Application statistics ***" << std::endl;
  double thr = 0;
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  uint32_t totalPacketsThr = sink1->GetTotalRx ();
  thr = totalPacketsThr * 8 / (simTime * 1000000.0); //Mbit/s
  std::cout << "  Rx Bytes sent by H1: " << totalPacketsThr << std::endl;
  std::cout << "  Average Goodput: " << thr << " Mbit/s" << std::endl;
  std::cout << std::endl << "*** TC Layer statistics ***" << std::endl;
  std::cout << "  Packets dropped by the TC layer R : " << q2->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Bytes dropped by the TC layer: " << q2->GetTotalDroppedBytes () << std::endl;
  std::cout << "  Packets dropped by the netdevice: " << queue2->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Packets requeued by the TC layer: " << q2->GetTotalRequeuedPackets () << std::endl;

  std::cout << "  Packets dropped by the TC layer Nodel 1 : " << q1->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Bytes dropped by the TC layer: " << q1->GetTotalDroppedBytes () << std::endl;
  std::cout << "  Packets dropped by the netdevice: " << queue1->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Packets requeued by the TC layer: " << q1->GetTotalRequeuedPackets () << std::endl;

//  fmHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", false, false);

}
