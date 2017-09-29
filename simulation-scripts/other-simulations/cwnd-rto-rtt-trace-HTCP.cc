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
//        	500 Kbps
//                5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include <iostream>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

bool firstCwnd = true;
bool firstSshThr = true;
bool firstRtt = true;
bool firstRto = true;
Ptr<OutputStreamWrapper> cWndStream;
Ptr<OutputStreamWrapper> rttStream;
Ptr<OutputStreamWrapper> rtoStream;
uint32_t cWndValue;
//uint32_t ssThreshValue;

static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
}

static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}
static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}

int
main (int argc, char *argv[])
{
  bool flow_monitor = false;
  bool tracing = true;
  uint32_t maxBytes = 0;
  std::string protocol = "TcpHtcp";
  std::string prefix_file_name = "router-with-tracing";

//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("protocol", "Flag to choose TCP variant", protocol);
  cmd.Parse (argc, argv);


//TCP settings
  if(protocol == "TcpNewReno")  
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  else
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHtcp::GetTypeId ()));

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1446));

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
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer ndc1 = pointToPoint2.Install (net1);
  NetDeviceContainer ndc2 = pointToPoint.Install (net2);

/*
// add error model to simulate packet loss
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  ndc1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
*/
//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install (nodes);
  internet.Install (routers);
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

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i2.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (100.0));

//
// Create a PacketSinkApplication and install it on node 2
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (100.0));

  std::cout<<"Before setting global router\n";
// ======================================================================
// Calculate and populate routing tables
// ----------------------------------------------------------------------
  NS_LOG_INFO ("L3: Populate routing tables.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::cout<<"After setting global router\n";



//
// Set up tracing if enabled
//
  if (tracing)
    {
      
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),std::ios::out);
      internet.EnableAsciiIpv4All (ascii_wrap);
      Simulator::Schedule (Seconds (0.00001), &TraceCwnd, prefix_file_name + "-cwnd.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRtt, prefix_file_name + "-rtt.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRto, prefix_file_name + "-rto.data");

    }

  // Flow monitor
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowHelper.InstallAll ();
    }

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
    }

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}
