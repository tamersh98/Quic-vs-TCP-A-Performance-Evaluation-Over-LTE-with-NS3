/*
 * Copyright (c) 2009 INRIA
 *
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
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =======
//         LAN (CSMA)
//
// - UDP flow from n0 to n1 of 1024 byte packets at intervals of 50 ms
//   - maximum of 320 packets sent (or limited by simulation duration)
//   - option to use IPv4 or IPv6 addressing
//   - option to disable logging statements

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/quic-module.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UdpClientServerExample");

int
main(int argc, char* argv[])
{
    // Declare variables used in command-line arguments
    bool useV6 = false;
    bool logging = true;
    Address serverAddress;

    CommandLine cmd(__FILE__);
    cmd.AddValue("useIpv6", "Use Ipv6", useV6);
    cmd.AddValue("logging", "Enable logging", logging);
    cmd.Parse(argc, argv);

    if (logging)
    {
        LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
    }

      Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(512*1024));

  NS_LOG_INFO("Create nodes in above topology.");
  NodeContainer n;
  n.Create(1);

  NodeContainer ueNodes;
  ueNodes.Create(1);

  QuicHelper internet;
  internet.InstallQuic(n);
  Ptr<PointToPointEpcHelper> epc = CreateObject<PointToPointEpcHelper>();
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
  lteHelper->SetEpcHelper(epc);

  Ptr<Node> pgw = epc->GetPgwNode();

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(0.005));
  em->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET")); 

    //create enb nodes
    NodeContainer enbNodes;
  enbNodes.Create(1);
    //connect server with epc pgw point to point
    PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
  p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(12)));
  NetDeviceContainer PgwDevices = p2ph.Install(pgw, n.Get(0));

  PgwDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
  PgwDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));


  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (PgwDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (n.Get(0)->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  //set SIU link 
   
  epc->SetAttribute("S1uLinkDataRate", DataRateValue(DataRate("1Gb/s")));
  epc->SetAttribute("S1uLinkDelay", ns3::TimeValue(ns3::MilliSeconds(5)));

  //set moblility 
  //Create ueNodes and install constant mobility on them
 
   //internet.Install(ueNodes);
 MobilityHelper enbMob;
    enbMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMob.Install(enbNodes);
    MobilityHelper ueMob;
    ueMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ueMob.Install(ueNodes);
     //set postion
	 NodeContainer allNodes = NodeContainer ( enbNodes, ueNodes);
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0, 0.0, 0.0));
    positionAlloc->Add (Vector (2500, 0.0, 0.0));
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (allNodes);

  lteHelper->SetPathlossModelType(TypeId::LookupByName("ns3::ThreeLogDistancePropagationLossModel"));
  lteHelper->SetPathlossModelAttribute("Distance1", DoubleValue(250));
  lteHelper->SetPathlossModelAttribute("Distance2", DoubleValue(1000));
  lteHelper->SetPathlossModelAttribute("Exponent1" ,DoubleValue(2));
  lteHelper->SetPathlossModelAttribute("Exponent2" ,DoubleValue(2.5));
  lteHelper->SetPathlossModelAttribute("ReferenceLoss" ,DoubleValue(46));;

    lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
    std::ifstream ifTraceFile;
    ifTraceFile.open("../../src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad",
                     std::ifstream::in);
    if (ifTraceFile.good())
    {
        // script launched by test.py
        lteHelper->SetFadingModelAttribute(
            "TraceFilename",
            StringValue("../../src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad"));
    }
    else
    {
        // script launched as an example
        lteHelper->SetFadingModelAttribute(
            "TraceFilename",
            StringValue("src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad"));
    }
    lteHelper->SetFadingModelAttribute ("TraceLength", TimeValue (Seconds (40.0)));
    lteHelper->SetFadingModelAttribute ("SamplesNum", UintegerValue (1000000));
    lteHelper->SetFadingModelAttribute ("WindowSize", TimeValue (Seconds (1)));
   

  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);
      // Set transmission power of the eNb to 46 dBm.
  Ptr<LteEnbNetDevice> lteEnbDev = enbDevs.Get(0)->GetObject<LteEnbNetDevice>();    
    lteEnbDev->GetPhy()->SetTxPower(46);

    // Set transmission power of the Ue to 23 dBm.
  Ptr<LteUeNetDevice> lteUeDev = ueDevs.Get(0)->GetObject<LteUeNetDevice>();
    lteUeDev->GetPhy()->SetTxPower(23);

  internet.InstallQuic (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epc->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
  //std::cout << "Ip address of test UE node: " << ueIpIface.GetAddress(ueNodeNum - 1) << std::endl;

  // Assign IP address to UEs
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epc->GetUeDefaultGatewayAddress (), 1);
  }

  lteHelper->Attach(ueDevs , enbDevs.Get(0));
    NS_LOG_INFO("Create UdpServer application on node 1.");
    uint16_t port = 4000;
    QuicServerHelper server(port);
    ApplicationContainer apps = server.Install(ueNodes.Get(0));
    apps.Start(Seconds(5.0));
    apps.Stop(Seconds(40.0));

    NS_LOG_INFO("Create UdpClient application on node 0 to send to node 1.");
    uint32_t MaxPacketSize = 512;
    uint32_t Numstreams = 16;
    Time interPacketInterval = Seconds(0.0003);
    uint32_t maxPacketCount = 100000000;
    QuicClientHelper client(ueIpIface.GetAddress(0), port);
    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    client.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    client.SetAttribute("NumStreams", UintegerValue(Numstreams));
    std::cout << "NumStreams: " << Numstreams << std::endl;
    apps = client.Install(n.Get(0));
    apps.Start(Seconds(5.1));
    apps.Stop(Seconds(40.0));

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop (Seconds (40));
    Simulator::Run();
    uint64_t totalpackets = server.GetServer()->GetReceived();
    double avgThroughput = (totalpackets * 8.0*MaxPacketSize) / (35 * 1000 * 1000); 
    std::cout << "Avg Throughput: " << avgThroughput << " Mbps" << std::endl;
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
