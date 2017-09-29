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
/*
             net1
        n1------------\
                       \
               net2     \     net13            net4
	n2-------------[R1]------------[R3]--------------n4
			/                 \
		       /                   \
		net12 /                     \ net34
                     /                       \
                    /                         \
      n3--------[R2]------------------------[R4]------------n5
        net3                   net24              net5
*/

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

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Link Failure");

uint32_t startTime = 0, prevLost1 = 0, prevLost2 = 0, prevLost3 = 0;
uint64_t prevRxBytes1 = 0,prevRxBytes2 = 0,prevRxBytes3 = 0;
double prevTxpkt1 = 0, prevTxpkt2 = 0, prevTxpkt3 = 0 ;

void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon)
{
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin ();
  //flow 1->4
  Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
//  std::cout<<"Flow ID : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
  double Thr1 = ((stats->second.rxBytes-prevRxBytes1) * 8.0)/(1024*1024);
  uint32_t lostPkt1 = stats->second.lostPackets - prevLost1;
  double lostRate1 = (lostPkt1 / (stats->second.txPackets - prevTxpkt1))*100;
  prevRxBytes1=stats->second.rxBytes;
  prevLost1 = stats->second.lostPackets;
  prevTxpkt1 = stats->second.txPackets;

  ++stats; //flow 3->5
  fiveTuple = classing->FindFlow (stats->first);
//  std::cout<<"Flow ID : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
  double Thr3 = ((stats->second.rxBytes-prevRxBytes3) * 8.0)/(1024*1024);
  uint32_t lostPkt3 = stats->second.lostPackets - prevLost3;
  double lostRate3 = (lostPkt3 / (stats->second.txPackets - prevTxpkt3))*100;
  prevRxBytes3 = stats->second.rxBytes;
  prevLost3 = stats->second.lostPackets;
  prevTxpkt3 = stats->second.txPackets;

  ++stats;
  ++stats; //flow 2->4
  fiveTuple = classing->FindFlow (stats->first);
//  std::cout<<"Flow ID : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
  double Thr2 = ((stats->second.rxBytes-prevRxBytes2) * 8.0)/(1024*1024);
  uint32_t lostPkt2 = stats->second.lostPackets - prevLost2;
  double lostRate2 = (lostPkt2 / (stats->second.txPackets - prevTxpkt2))*100;
  prevRxBytes2 =  stats->second.rxBytes;
  prevLost2 = stats->second.lostPackets;
  prevTxpkt2 = stats->second.txPackets;

  std::cout<<Thr1<<" "<<lostPkt1<<" "<<lostRate1<<" "<<Thr2<<" "<<lostPkt2<<" "<<lostRate2<<" "<<Thr3<<" "<<lostPkt3<<" "<<lostRate3<<std::endl;
  startTime=ns3::Simulator::Now().GetSeconds();

  Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon);
}

int
main (int argc, char *argv[])
{
//================================================================================
//*********************Intialization and general configuration*******************
//================================================================================
  bool flow_monitor = true;
  bool tracing = false;
  uint32_t maxBytes = 0;
  std::string protocol = "TcpHtcp";
  std::string prefix_file_name = "link-failure ";
//Qdisc setup:
  std::string queueDiscType = "PfifoFast"; 
  uint32_t queueSize = 5000;//in packets
//  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/test-pcap.txt";
//  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-1G.txt";
  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len.txt";
  double simTime = 60.0;
  uint64_t tcpBuff = 50000000; //in bytes
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
  cmd.AddValue("file_name", "File name", prefix_file_name);
  cmd.Parse (argc, argv);

//Dynamic routing configuration
// The below value configures the default behavior of global routing.
// By default, it is disabled.  To respond to interface events, set to true
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

//tc layer config at the routers
  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (queueSize));

//tc layer config at the senders
  TrafficControlHelper tchPfifo1;
  tchPfifo1.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (10000));

//netdev config
 Config::SetDefault ("ns3::Queue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
 Config::SetDefault ("ns3::Queue::MaxPackets", UintegerValue (queueSize));


//TCP settings
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1446));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (tcpBuff));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (tcpBuff));
//for 7G tcp buffer: 17500000

  if(protocol == "TcpNewReno")  
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  else
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHtcp::GetTypeId ()));

//================================================================================
//**************************** Create Topology *******************************
//================================================================================
//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> n1 = CreateObject<Node> ();
  Names::Add ("node1", n1);
  Ptr<Node> n2 = CreateObject<Node> ();
  Names::Add ("node2", n2);
  Ptr<Node> n3 = CreateObject<Node> ();
  Names::Add ("node3", n3);
  Ptr<Node> n4 = CreateObject<Node> ();
  Names::Add ("node4", n4);
  Ptr<Node> n5 = CreateObject<Node> ();
  Names::Add ("node5", n5);

  Ptr<Node> r1 = CreateObject<Node> ();
  Names::Add ("router1", r1);
  Ptr<Node> r2 = CreateObject<Node> ();
  Names::Add ("router2", r2);
  Ptr<Node> r3 = CreateObject<Node> ();
  Names::Add ("router3", r3);
  Ptr<Node> r4 = CreateObject<Node> ();
  Names::Add ("router4", r4);

  NodeContainer net1  (n1, r1);
  NodeContainer net2  (n2, r1);
  NodeContainer net3  (n3, r2);
  NodeContainer net4  (n4, r3);
  NodeContainer net5  (n5, r4);

  NodeContainer net12 (r1, r2);
  NodeContainer net13 (r1, r3);
  NodeContainer net24 (r2, r4);
  NodeContainer net34 (r3, r4);
 
  NodeContainer routers (r1,r2,r3,r4);
  NodeContainer nodes (n1,n2,n3,n4,n5);

  NS_LOG_INFO ("Create channels.");

//
// Explicitly create the point-to-point link required by the topology (shown above).
//
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer ndc1  = pointToPoint2.Install (net1);
  NetDeviceContainer ndc2  = pointToPoint2.Install (net2);
  NetDeviceContainer ndc3  = pointToPoint2.Install (net3);
  NetDeviceContainer ndc4  = pointToPoint2.Install (net4);
  NetDeviceContainer ndc5  = pointToPoint2.Install (net5);

  NetDeviceContainer ndc12  = pointToPoint2.Install (net12);
  NetDeviceContainer ndc13  = pointToPoint2.Install (net13);
  NetDeviceContainer ndc24  = pointToPoint2.Install (net24);
  NetDeviceContainer ndc34  = pointToPoint2.Install (net34);

//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install (nodes);
  internet.Install (routers);

////add dqisc to the router1
//make sure to install the qdisc after installing the Internet stack and before assigining IP address
//  tchPfifo.Install (ndc2.Get (0));
  
  QueueDiscContainer qdiscs1 = tchPfifo.Install (ndc13.Get (0));
  Ptr<QueueDisc> q1 = qdiscs1.Get (0);
  tchPfifo.Install (ndc4.Get (1)); //install it to R3

  Ptr<NetDevice> nd1 = ndc13.Get (0);
  Ptr<PointToPointNetDevice> ptpnd1 = DynamicCast<PointToPointNetDevice> (nd1);
  Ptr<Queue> queue1 = ptpnd1->GetQueue ();

//install tc at sender n1 to control the queue size (txqueuelen)
  tchPfifo1.Install (ndc1.Get (0));

//Index for link failure
//  Ptr<Node> n11 = routers.Get (1);
  Ptr<Ipv4> ipv4_2 = r2->GetObject<Ipv4> ();
  // The first ifIndex is 0 for loopback, then the first ndc installed is numbered 1,
  // then the next ndc is numbered 2, and the third ndc installed is numbered 3
  uint32_t ipv4ifIndex1 = 3;

//================================================================================
//********************* Assign IP adresses to the network links  ****************
//================================================================================

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;

// Hosts IP
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = ipv4.Assign (ndc1);
  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = ipv4.Assign (ndc2);
  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3 = ipv4.Assign (ndc3);
  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i4 = ipv4.Assign (ndc4);
  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i5 = ipv4.Assign (ndc5);

// Routers IP
  ipv4.SetBase ("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i13 = ipv4.Assign (ndc13);
  ipv4.SetBase ("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i12 = ipv4.Assign (ndc12);
  ipv4.SetBase ("10.1.24.0", "255.255.255.0");
  Ipv4InterfaceContainer i24 = ipv4.Assign (ndc24);
  ipv4.SetBase ("10.1.34.0", "255.255.255.0");
  Ipv4InterfaceContainer i34 = ipv4.Assign (ndc34);

//================================================================================
//********************* Application setup at each node *******************
//================================================================================

  NS_LOG_INFO ("Create Applications.");

//------------------- node1: HTCP flow sender ------------------------ 
// TCP flow n1 --> n4
  uint16_t port = 9; 
  uint8_t tos = 0x08; //band 2 (low priority)

  InetSocketAddress destAddress (i4.GetAddress (0), port);
  destAddress.SetTos (tos);
  BulkSendHelper source ("ns3::TcpSocketFactory", destAddress);
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (simTime));

//------------------- node2: CAIDA1 flow sender ------------------------ 
// UDP flow n2 --> n4

  tos = 0x10; //band 0 (high priority)
  InetSocketAddress destAddress2 (i4.GetAddress (0), 10);
  destAddress2.SetTos (tos);
  UdpReplayTraceClientHelper traceClient2  (destAddress2, filename);
//  traceClient2.SetAttribute ("MaxPacketSize", UintegerValue (1448));
  ApplicationContainer clientApps2 = traceClient2.Install (nodes.Get (1));
  clientApps2.Start (Seconds (1.0));
  clientApps2.Stop (Seconds (simTime));

//------------------- node3: CAIDA2 flow sender ------------------------ 
// UDP flow n3 --> n5

  tos = 0x10; //band 0 (high priority)
  InetSocketAddress destAddress3 (i5.GetAddress (0), 11);
  destAddress3.SetTos (tos);
  UdpReplayTraceClientHelper traceClient3 (destAddress3, filename);
//  traceClient3.SetAttribute ("MaxPacketSize", UintegerValue (1448));
  ApplicationContainer clientApps3 = traceClient3.Install (nodes.Get (2));
  clientApps3.Start (Seconds (0.0));
  clientApps3.Stop (Seconds (simTime));
/*
  uint16_t port = 9; 
  uint8_t tos = 0x08; //band 2 (low priority)

  InetSocketAddress destAddress (i5.GetAddress (0), port);
  destAddress.SetTos (tos);
  BulkSendHelper source ("ns3::TcpSocketFactory", destAddress);
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (2));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (simTime));
*/
//------------------- node4: CAIDA1 flow and HTCP flow receiver ------------------------ 
//

  // HTCP flow receiver app
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (3));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (simTime));

  // CAIDA1 flow receiver app
  PacketSinkHelper sink2 ("ns3::UdpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), 10));
  ApplicationContainer sinkApps2 = sink2.Install (nodes.Get (3));
  sinkApps2.Start (Seconds (0.0));
  sinkApps2.Stop (Seconds (simTime));

//------------------- node5: CAIDA2 flow receiver ------------------------ 
// 

  // CAIDA2 flow receiver app
  PacketSinkHelper sink3 ("ns3::UdpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), 11));
  ApplicationContainer sinkApps3 = sink3.Install (nodes.Get (4));
  sinkApps3.Start (Seconds (0.0));
  sinkApps3.Stop (Seconds (simTime));
/*
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (4));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (simTime));
*/
//================================================================================
//********************* Calculate and populate routing tables  *******************
//================================================================================
  NS_LOG_INFO ("L3: Populate routing tables.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//================================================================================
//************************** Monitoring and tracing Setup ************************
//================================================================================

// Flow monitor
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();

 if (tracing)
 {
   AsciiTraceHelper ascii;
   pointToPoint2.EnableAsciiAll (ascii.CreateFileStream ("link-failure.tr"));
   // Trace routing tables 
   Ipv4GlobalRoutingHelper g;
   Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
   g.PrintRoutingTableAllAt (Seconds (2), routingStream);
   g.PrintRoutingTableAllAt (Seconds (9), routingStream);
   
   //capture pcap
    pointToPoint2.EnablePcapAll ("link-failure", true);
 }


//================================================================================
//************************** RUN THE SIMULATION ************************
//================================================================================

  //schedule monitoring flows throughput
  Simulator::Schedule(Seconds(2),&ThroughputMonitor,&fmHelper, allMon);
  std::cout<<"HTCP-Throuhgput(Mbps) HTCP-lost-packets HTCP-lost-rate(%) CAIDA1-Throuhgput(Mbps) CAIDA1-lost-packets CAIDA1-lost-rate(%) CAIDA2-Throuhgput(Mbps) CAIDA2-lost-packets CAIDA2-lost-rate(%)\n";
  //schedule link failure events
  Simulator::Schedule (Seconds (30),&Ipv4::SetDown,ipv4_2, ipv4ifIndex1);
 
  
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");


//================================================================================
//************************** Print statistics  ************************
//================================================================================
  std::cout << std::endl << "*** Application statistics ***" << std::endl;
  double thr = 0;
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  uint32_t totalPacketsThr = sink1->GetTotalRx ();
  thr = totalPacketsThr * 8 / (simTime * 1000000.0); //Mbit/s
  std::cout << "  Rx Bytes sent by H1: " << totalPacketsThr << std::endl;
  std::cout << "  Average Goodput: " << thr << " Mbit/s" << std::endl;
  std::cout << std::endl << "*** TC Layer statistics ***" << std::endl;
  std::cout << "  Packets dropped by the TC layer R : " << q1->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Bytes dropped by the TC layer: " << q1->GetTotalDroppedBytes () << std::endl;
  std::cout << "  Packets dropped by the netdevice: " << queue1->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Packets requeued by the TC layer: " << q1->GetTotalRequeuedPackets () << std::endl;


  if (flow_monitor)
    {
      fmHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", false,false);
    }

//  std::cout << "Node1 Total Bytes Received : " << sink1->GetTotalRx () << std::endl;
//  Ptr<PacketSink> sink4 = DynamicCast<PacketSink> (sinkApps2.Get (0));
//  std::cout << "Node2 Total Bytes Received : " << sink4->GetTotalRx () << std::endl;

//  Ptr<PacketSink> sink5 = DynamicCast<PacketSink> (sinkApps3.Get (0));
//  std::cout << "Node3 Total Bytes Received : " << sink5->GetTotalRx () << std::endl;

}
