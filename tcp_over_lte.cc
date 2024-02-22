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

using namespace ns3;

int main (int argc, char *argv[])
{
    NodeContainer ueNodes;
    ueNodes.Create(1);

    NodeContainer enbNodes;
    enbNodes.Create(1);

    // Setup LTE and EPC package
    Ptr<LteHelper> lte = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epc = CreateObject<PointToPointEpcHelper> ();
    lte->SetEpcHelper (epc);

    // Setup S/P-GW and PDN server
    Ptr<Node> pgw = epc->GetPgwNode ();

    NodeContainer servers;
    servers.Create (1);
    Ptr<Node> server = servers.Get (0);
    InternetStackHelper internet;
    internet.Install (servers);

    // Setup transmission rate and latency between eNodeB and S/P-GW
    PointToPointHelper p2pEnbPgw;
    p2pEnbPgw.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    p2pEnbPgw.SetChannelAttribute("Delay", TimeValue(Seconds(0.005))); // Latency 5ms
    NetDeviceContainer iDevsEnbPgw = p2pEnbPgw.Install(enbNodes.Get(0), pgw);

    // Setup transmission rate and latency between S/P-GW and servers
    PointToPointHelper p2pPgwServer;
    p2pPgwServer.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    p2pPgwServer.SetChannelAttribute("Delay", TimeValue(Seconds(0.012))); // Latency 12ms
    NetDeviceContainer iDevsPgwServer = p2pPgwServer.Install(pgw, server);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer ifaces = ipv4.Assign (iDevsPgwServer);
    Ipv4Address serverAddr = ifaces.GetAddress (1);

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

    // Set the distance between eNodeB and UE to 250 meters
    Ptr<ConstantPositionMobilityModel> ueMobility = ueNodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
    ueMobility->SetPosition(Vector(250.0, 0.0, 0.0));

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
    source.SetAttribute ("SendSize", UintegerValue (100000));
    ApplicationContainer sourceApps = source.Install (server);
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (40.0));

    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer sinkApps = sink.Install (ueNodes.Get (0));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (40.0));

    Simulator::Stop(Seconds(40.001));

    lte->EnableMacTraces ();
    lte->EnableRlcTraces ();
    lte->EnablePdcpTraces ();

    Simulator::Run ();

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
    std::cerr << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;

    double totalTime = 40.0; // Total simulation time in seconds
    double avgThroughput = sink1->GetTotalRx () * 8 / totalTime / 1000000; // Convert bytes to Mbps
    std::cerr << "Average TCP Throughput: " << avgThroughput << " Mbps" << std::endl;

    Simulator::Destroy ();
    return 0;
}
