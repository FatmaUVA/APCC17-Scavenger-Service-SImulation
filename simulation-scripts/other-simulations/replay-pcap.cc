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

/* Fatma
*This script read a text file with three columns: packet_no time  packetlen
*then it uses UDP packets to replay the data
*It uses the modified udp-trace-client.cc to replay pcap instead of specificaly
* MPEG4 stream trace files
*/

//         Topology
//
// n1---------------------n2
//   point-to-point link


#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/double.h"
#include "ns3/boolean.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Replaye PCAP");

void
stopSimulation ()
{
  Simulator::Stop (); 
}
int
main (int argc, char *argv[])
{
//  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-64KMTU-parsed-ts-len.txt";
//  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-cropped.txt";
//  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/test-pcap-64KMTU";
  std::string filename = "/users/fha6np/ns-workspace/ns-allinone-3.26/ns-3.26/60s-1514MTU-parsed-ts-len-1G.txt";
//  CommandLine cmd;
//  cmd.AddValue("nPackets", "Number of packets to echo", nPackets);
//  cmd.Parse (argc, argv);
//  std::string filename= "";
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Creating Topology");
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
//  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (64000));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  UdpServerHelper traceServer (9);

  ApplicationContainer serverApps = traceServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (5.0));

//  Config::SetDefault ("ns3::UdpReplayTraceClient::Loop", BooleanValue (true));// to keep on looping the pcap file until the file transmission finishes

  uint8_t tos = 0x10; //band 0
  InetSocketAddress destAddress2 (interfaces.GetAddress (1), 9);
  destAddress2.SetTos (tos);
  UdpReplayTraceClientHelper traceClient  (destAddress2, filename);
//  UdpReplayTraceClientHelper traceClient  (interfaces.GetAddress (1), filename);
  traceClient.SetAttribute ("MaxPacketSize", UintegerValue (1448));
  traceClient.SetAttribute ("SimTime", DoubleValue (20.0));
  traceClient.SetAttribute ("Loop", BooleanValue (true));
//  traceClient.SetAttribute ("MaxPacketSize", UintegerValue (63934));
//  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
//  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = traceClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (5.0));

//  AsciiTraceHelper ascii;
//  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("replay-pcap.tr"));
  pointToPoint.EnablePcapAll ("replay-pcap-simTime-10"); 
//  Simulator::Stop (Seconds (1.5));
//  Simulator::Schedule(Seconds(1.5),&stopSimulation);
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
