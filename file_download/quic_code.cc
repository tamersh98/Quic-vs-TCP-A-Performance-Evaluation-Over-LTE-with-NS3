#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <numeric>
#include <typeinfo>
#include <cmath>

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
#include "ns3/random-variable-stream.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/nstime.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-classifier.h" 
#include "ns3/mobility-module.h"
#include "ns3/quic-module.h"
#include "ns3/error-model.h"
using namespace ns3;

//static uint64_t rxBytes = 0;
//static double timeFirstRxPacket = 0.0;


static std::list<int> RxTimes;
static std::list<int> bytesTotal;
void SinkRxTrace(std::string context, Ptr<const Packet> pkt, const Address &addr)
{

 std::cout<<"Packet received at "<<Simulator::Now().GetSeconds()<<" s\n";
}

void GetPositionRaw (Ptr<Node> node)		// Function to get network objects location.
{
   Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
   Vector position=mobility->GetPosition ();
   std::cout << "Position "<<Simulator::Now ()<<" x "<<position.x<<" y "<<position.y<<" z "<<position.z << std::endl;
   //Simulator::Schedule (Seconds (0.1), &GetPositionRaw, node);
}


int main (int argc, char *argv[])
{

  uint32_t    ueNodeNum = 1;
  uint32_t    enbNodeNum = 1;
  uint32_t    packet_size = 512;
  double      duration = 40;
  
  DataRate internet_bandwidth = DataRate("1Gbps");
  Time internet_delay = Seconds (0.012);
  Time delay_enb = Seconds (0.005);

  // Parse command line attribute

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(512*1024));


  //Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(100));
  //Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(100));

  

   

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  QuicHelper internet;
  internet.InstallQuic(remoteHost);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  Ptr<Node> sgw = epcHelper->GetSgwNode ();
  std::cout << "Id of pgw node: " << pgw->GetId() << std::endl;
  std::cout << "Id of sgw node: " << sgw->GetId() << std::endl;

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (internet_bandwidth));
  p2ph.SetChannelAttribute ("Delay", TimeValue (internet_delay));
 // p2ph.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  //internetDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

   // internetDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    //internetDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
  
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


  // Create enbNodes and install const mobility on all existing nodes so far
  NodeContainer enbNodes;
  enbNodes.Create (enbNodeNum);
   
   epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue(DataRate("1Gb/s")));
    epcHelper->SetAttribute("S1uLinkDelay", ns3::TimeValue(ns3::MilliSeconds(5)));

 
  std::cout << "Id of eNodeB node: " << enbNodes.Get (0)->GetId() << std::endl;

  
   
  //Create ueNodes and install constant mobility on them
  NodeContainer ueNodes;
  ueNodes.Create (ueNodeNum);
  Ptr<Node> testUE = ueNodes.Get (0);
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
    positionAlloc->Add (Vector (250, 0.0, 0.0));
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (allNodes);

  GetPositionRaw(ueNodes.Get(0));

  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

      // Set transmission power of the eNb to 46 dBm.
  Ptr<LteEnbNetDevice> lteEnbDev = enbDevs.Get(0)->GetObject<LteEnbNetDevice>();    
    lteEnbDev->GetPhy()->SetTxPower(46);

    // Set transmission power of the Ue to 23 dBm.
  Ptr<LteUeNetDevice> lteUeDev = ueDevs.Get(0)->GetObject<LteUeNetDevice>();
    lteUeDev->GetPhy()->SetTxPower(23);

  // Install the IP stack on the UEs
  internet.InstallQuic (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
  //std::cout << "Ip address of test UE node: " << ueIpIface.GetAddress(ueNodeNum - 1) << std::endl;

  // Assign IP address to UEs
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  lteHelper->Attach(ueDevs , enbDevs.Get(0));

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1100;
  //int filesize = 1024*64*10;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {

      
      BulkSendHelper source ("ns3::QuicSocketFactory", InetSocketAddress (ueIpIface.GetAddress(u), dlPort));
      source.SetAttribute ("MaxBytes", UintegerValue (1024*256));
      //source.SetAttribute ("Remote", remoteAddress);
      source.SetAttribute ("SendSize", UintegerValue (packet_size));
      serverApps.Add (source.Install (remoteHost));
   
      PacketSinkHelper sink ("ns3::QuicSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), dlPort)));
      clientApps.Add ( sink.Install (ueNodes.Get(u)));

      serverApps.Start (Seconds (5));
      clientApps.Start (Seconds (0));
    }

  std::cout<< "Total number of nodes: " << ns3::NodeList::GetNNodes() << std::endl;
  std::cout << "Duration: " << duration << " seconds" << std::endl;
  

   Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
   Ptr<PacketSink> pktSink = StaticCast<PacketSink> (clientApps.Get(0));
  std::stringstream ss; ss << "Some information";
  pktSink->TraceConnect("Rx", ss.str(), MakeCallback (&SinkRxTrace));
   
  Simulator::Stop (Seconds (10));
  Simulator::Run ();
  
  /* flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
    double totalThroughput = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        totalThroughput += i->second.rxBytes * 8.0 / 10.0 / 1000 / 1000; // Convert bytes to Mbps
    }
    double avgThroughput = totalThroughput / stats.size();*/
    Ptr<PacketSink> Quicsink = DynamicCast<PacketSink>(clientApps.Get(0));
    uint64_t tcpTotalBytesReceived = Quicsink->GetTotalRx();
    double avgThroughput = (tcpTotalBytesReceived * 8.0) / (35 * 1000 * 1000); 
    std::cout << "Average Quic throughput: " << avgThroughput << " Mbps" << std::endl;
     /* uint64_t rxBytes = 0;

      rxBytes = packet_size * DynamicCast<QuicServer> (serverApps.Get (0))->GetReceived ();
      double throughput = (rxBytes * 8) / (35 * 1000000.0); //Mbit/s
      std::cout << "throughput = " << throughput << " Mbit/s" << std::endl;*/
    
  Simulator::Destroy ();

  return 0;
}
