#include <iostream>
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
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-classifier.h" 



using namespace ns3;

int main (int argc, char *argv[])
{
    NodeContainer ueNodes;
    ueNodes.Create(1);

    NodeContainer enbNodes;
    enbNodes.Create(1);
uint16_t bandwidth = 100; // = 20 MHZ
    // Setup LTE and EPC package
    Ptr<LteHelper> lte = CreateObject<LteHelper> ();
// *************************************************************
// Set LTE frequency bands
//lte->setDIBandwidth(20e6);
//setting power
/*Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46.0));
Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));
onfig::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue (true));
Config::SetDefault ("ns3::LteUePowerControl::ClosedLoop", BooleanValue (true));
Config::SetDefault ("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue (true));
Config::SetDefault ("ns3::LteUePowerControl::Alpha", DoubleValue (1.0));*/
// end of power setting
lte->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth)); // Bandwidth (20 MHz)
lte->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth)); // Bandwidth (20 MHz)
//lte->SetEnbDeviceAttribute("DownlinkFrequency", UintegerValue(2100000000)); // Downlink frequency (e.g., 2.1 GHz)

//**************************************************************
    Ptr<PointToPointEpcHelper>  epc = CreateObject<PointToPointEpcHelper> ();
    lte->SetEpcHelper (epc);

    // Setup S/P-GW and PDN server
    Ptr<Node> pgw = epc->GetPgwNode ();
    Ptr<Node> sgw = epc->GetPgwNode();
    NodeContainer servers;
    servers.Create (1);
    Ptr<Node> server = servers.Get (0);
    InternetStackHelper internet;
    internet.Install (servers);

    // Setup transmission rate and latency between eNodeB and S/P-GW
    PointToPointHelper p2pEnbPgw;

// *************************************************************************
	// my added code... setting limit to the queues
//p2pEnbPgw.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(100));
 p2pEnbPgw.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize ("100p"))); //added
// *********** the default value of the maxpacket is 100   which is the value we are meant to set

// *************************************************************************


    p2pEnbPgw.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    p2pEnbPgw.SetChannelAttribute("Delay", TimeValue(Seconds(0.005))); // Latency 5ms
    NetDeviceContainer iDevsEnbPgw = p2pEnbPgw.Install(sgw, enbNodes.Get(0));
//error rate set
   Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(0.005));
    iDevsEnbPgw.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    // Setup transmission rate and latency between S/P-GW and servers
    PointToPointHelper p2pPgwServer;
    p2pPgwServer.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    p2pPgwServer.SetChannelAttribute("Delay", TimeValue(Seconds(0.012))); // Latency 12ms
    p2pPgwServer.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize ("100p")));//added
    NetDeviceContainer iDevsPgwServer = p2pPgwServer.Install(pgw, server);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer ifaces = ipv4.Assign (iDevsPgwServer);
    //Ipv4Address serverAddr = ifaces.GetAddress (1);

    Ipv4StaticRoutingHelper routing;
    Ptr<Ipv4StaticRouting> pdnStaticRouting = routing.GetStaticRouting (server->GetObject<Ipv4> ());
    pdnStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  
    // Setup UE & eNB details
    MobilityHelper enbMob;
    enbMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMob.Install(enbNodes);
    MobilityHelper ueMob;
    ueMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ueMob.Install(ueNodes);
     //set postion
	 NodeContainer allNodes = NodeContainer ( enbNodes, ueNodes);
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
        positionAlloc->Add (Vector (1500, 0.0, 0.0));
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (allNodes);


    //Set the distance between eNodeB and UE to 250 meters
   //Ptr<ConstantPositionMobilityModel> enbMobility = enbNodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
   //enbMobility->SetPosition(Vector(0.0, 0.0, 0.0));

    //Set the distance between eNodeB and UE to 250 meters
   // Ptr<ConstantPositionMobilityModel> ueMobility = ueNodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
   //ueMobility->SetPosition(Vector(5000, 0.0, 0.0));

    //ueMob.Install(ueNodes);
    //enbMob.Install(enbNodes);

    	//int dist = ueMobility->GetDistanceFrom(enbMobility);
   
 // std::cerr << "distance " << dist <<std::endl;

    NetDeviceContainer enbLteDevs = lte->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueLteDevs = lte->InstallUeDevice (ueNodes);

    internet.Install (ueNodes);
    Ipv4InterfaceContainer ueIface;
    ueIface = epc->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
    Ptr<Node> ueNode = ueNodes.Get (0);
    Ptr<Ipv4StaticRouting> ueStaticRouting = routing.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epc->GetUeDefaultGatewayAddress (), 1);

    lte->Attach (ueLteDevs);

    BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address ("7.0.0.2"), 9));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetAttribute ("SendSize", UintegerValue (512));
    ApplicationContainer sourceApps = source.Install (server);
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (40.0));

    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer sinkApps = sink.Install (ueNodes.Get (0));
    sinkApps.Start (Seconds (5.0));
    sinkApps.Stop (Seconds (40.0));
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    Simulator::Stop(Seconds(40.001));

    lte->EnableMacTraces ();
    lte->EnableRlcTraces ();
    lte->EnablePdcpTraces ();

    Simulator::Run ();
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
    double totalThroughput = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        totalThroughput += i->second.rxBytes * 8.0 / 10.0 / 1000 / 1000; // Convert bytes to Mbps
    }
    double avgThroughput = totalThroughput / stats.size();
    std::cout << "Average TCP throughput: " << avgThroughput << " Mbps" << std::endl;
    Simulator::Destroy ();
    return 0;
}
