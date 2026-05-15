// File: wsn-routing-comparison.cc
//
// Simple NS-3 Wireless Sensor Network simulation
// Comparing AODV (reactive) and DSDV (proactive)
//
// This is intentionally beginner-friendly.
// Do not expect elegance. Academic code survives mostly through ritual.
//
// HOW TO RUN:
// 1. Put this file inside:
//    ns-3-dev/scratch/
//
// 2. Build:
//    ./ns3 build
//
// 3. Run:
//    ./ns3 run scratch/wsn-routing-comparison
//
// 4. Change parameters below and rerun.
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include <cmath>
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WSNRoutingComparison");

// Runtime trace callbacks (include context string so they work with Config::Connect)
void AppTxTrace(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("APP TX: " << context << " size=" << packet->GetSize());
}

void AppRxTrace(std::string context, Ptr<const Packet> packet, const Address &from)
{
    NS_LOG_UNCOND("APP RX: " << context << " size=" << packet->GetSize() << " from=" << from);
}

void MacTxTrace(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("MAC TX: " << context << " size=" << packet->GetSize());
}

void MacRxTrace(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("MAC RX: " << context << " size=" << packet->GetSize());
}

void IpTxTrace(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("IP TX: " << context << " size=" << packet->GetSize());
}

void IpRxTrace(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("IP RX: " << context << " size=" << packet->GetSize());
}

// Ipv4L3Protocol traced-callbacks have signature: (Ptr<const Packet>, Ptr<Ipv4>, uint32_t)
void IpTxTraceNoContext(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, unsigned int interface)
{
    NS_LOG_UNCOND("IP TX: iface=" << interface << " size=" << packet->GetSize());
}

void IpRxTraceNoContext(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, unsigned int interface)
{
    NS_LOG_UNCOND("IP RX: iface=" << interface << " size=" << packet->GetSize());
}

int main(int argc, char *argv[])
{
    // ==============================
    // SIMULATION PARAMETERS
    // ==============================

    uint32_t numNodes = 25;
    double simulationTime = 1000.0;

    // Packet size:
    // 100 = small
    // 1000 = large
    uint32_t packetSize = 100;

    // Number of source nodes sending data
    uint32_t numSources = 1;

    // Choose routing protocol:
    // true  = AODV
    // false = DSDV
    bool useAodv = true;

    // WiFi data rate
    std::string phyMode = "DsssRate2Mbps";

    CommandLine cmd;

    cmd.AddValue("numNodes", "Number of nodes", numNodes);
    cmd.AddValue("packetSize", "Packet size", packetSize);
    cmd.AddValue("numSources", "Number of source nodes", numSources);
    cmd.AddValue("useAodv", "Use AODV if true, DSDV if false", useAodv);

    cmd.Parse(argc, argv);

    // ==============================
    // CREATE NODES
    // ==============================

    NodeContainer nodes;
    nodes.Create(numNodes);

    // ==============================
    // WIFI CONFIGURATION
    // ==============================

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue(phyMode),
        "ControlMode", StringValue(phyMode));

    YansWifiPhyHelper phy;
    // YansWifiChannelHelper channel = YansWifiChannelHelper::Default();

    // phy.SetChannel(channel.Create());

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(1000.0));
    phy.SetChannel(channel.Create());

    phy.Set("TxPowerStart", DoubleValue(40.0));
    phy.Set("TxPowerEnd", DoubleValue(40.0));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices;
    devices = wifi.Install(phy, mac, nodes);

    // ==============================
    // MOBILITY
    // ==============================   

    // Grid in 2000x2000 area
    MobilityHelper mobility;
    
    uint32_t gridWidth = std::sqrt(numNodes);
    double stepSize = 2000.0 / gridWidth;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(stepSize),
        "DeltaY", DoubleValue(stepSize),
        "GridWidth", UintegerValue(gridWidth),
        "LayoutType", StringValue("RowFirst"));

    // Static nodes
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // ==============================
    // INTERNET + ROUTING
    // ==============================

    InternetStackHelper internet;

    if (useAodv)
    {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);

        std::cout << "\nUsing AODV routing\n";
    }
    else
    {
        OlsrHelper olsr; // Ganti nama helpernya jadi OLSR
        internet.SetRoutingHelper(olsr);

        std::cout << "\nUsing OLSR routing (Proactive)\n";
    }

    internet.Install(nodes);

    // ==============================
    // IP ADDRESSING
    // ==============================

    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces;
    interfaces = ipv4.Assign(devices);

    // ==============================
    // APPLICATIONS
    // ==============================

    // Destination node:
    // last node in topology
    uint32_t destinationNode = numNodes - 1;

    uint16_t port = 9;

    // Packet receiver
    PacketSinkHelper sinkHelper(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            port));

    ApplicationContainer sinkApp =
        sinkHelper.Install(nodes.Get(destinationNode));

    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(simulationTime));

    // Create traffic sources
    for (uint32_t i = 0; i < numSources; i++)
    {
        OnOffHelper onoff(
            "ns3::UdpSocketFactory",
            InetSocketAddress(
                interfaces.GetAddress(destinationNode),
                port));

        onoff.SetAttribute(
            "DataRate",
            StringValue("64kbps"));

        onoff.SetAttribute(
            "PacketSize",
            UintegerValue(packetSize));

        onoff.SetAttribute(
            "OnTime",
            StringValue(
                "ns3::ConstantRandomVariable[Constant=1]"));

        onoff.SetAttribute(
            "OffTime",
            StringValue(
                "ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer app =
            onoff.Install(nodes.Get(i));

        app.Start(Seconds(15.0 + i));
        app.Stop(Seconds(simulationTime - 1));
    }

    // ==============================
    // FLOW MONITOR
    // ==============================

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // ==============================
    // RUN SIMULATION
    // ==============================

    Simulator::Stop(Seconds(simulationTime));

    // Enable packet metadata for NetAnim to track packets
    PacketMetadata::Enable();

    AnimationInterface anim("wsn-animation.xml");
    
    // Configure animation for packet visibility
    anim.EnablePacketMetadata(true);
    anim.SetMobilityPollInterval(Seconds(0.1));
    
    // Enable PCAP tracing to capture all packets
    std::stringstream ssPcap;
    ssPcap << "pcap-" 
           << (useAodv ? "aodv" : "olsr") << "-" 
           << numNodes << "nodes-" 
           << packetSize << "bytes-" 
           << numSources << "sources";
    
    std::string prefix = ssPcap.str();
    phy.EnablePcapAll(prefix);
    
    // Configure runtime traces (App, MAC, IP) so unlabeled NetAnim packets are identifiable
    // Config::Connect("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx",
    //                 MakeCallback(&AppTxTrace));

    // Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
    //                 MakeCallback(&AppRxTrace));

    // Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
    //                 MakeCallback(&MacTxTrace));

    // Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
    //                 MakeCallback(&MacRxTrace));

    // Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
    //                 MakeCallback(&IpTxTraceNoContext));

    // Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Rx",
    //                 MakeCallback(&IpRxTraceNoContext));

    // Enable AODV logging when the component is available.
    LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_INFO);

    std::cout << "\nSimulation started...\n";
    std::cout << "Watch packets move between nodes in NetAnim\n";
    std::cout << "PCAP traces saved to wsn-routing*.pcap files\n";
    std::cout << "Console traces will label App/MAC/IP events to map unlabeled packets\n";

    Simulator::Run();

    // ==============================
    // RESULTS
    // ==============================

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowmon.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    std::cout << "\n===== RESULTS =====\n";

    for (auto iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t =
            classifier->FindFlow(iter->first);

        std::cout << "\nFlow ID: " << iter->first << "\n";

        std::cout << "Source: "
                  << t.sourceAddress
                  << "\n";

        std::cout << "Destination: "
                  << t.destinationAddress
                  << "\n";

        std::cout << "Tx Packets: "
                  << iter->second.txPackets
                  << "\n";

        std::cout << "Rx Packets: "
                  << iter->second.rxPackets
                  << "\n";

        // Packet loss
        uint32_t lostPackets =
            iter->second.txPackets -
            iter->second.rxPackets;

        std::cout << "Lost Packets: "
                  << lostPackets
                  << "\n";

        // Delay
        if (iter->second.rxPackets > 0)
        {
            double avgDelay =
                iter->second.delaySum.GetSeconds() /
                iter->second.rxPackets;

            std::cout << "Average Delay: "
                      << avgDelay
                      << " s\n";

            // Jitter
            double avgJitter =
                iter->second.jitterSum.GetSeconds() /
                iter->second.rxPackets;

            std::cout << "Average Jitter: "
                      << avgJitter
                      << " s\n";
        }
    }

    // Save XML report
    std::stringstream ss;
    ss << "results-" 
       << (useAodv ? "aodv" : "olsr") << "-" 
       << numNodes << "nodes-" 
       << packetSize << "bytes-" 
       << numSources << "sources.xml";
    
    std::string filename = ss.str();

    monitor->SerializeToXmlFile(filename, true, true);

    Simulator::Destroy();

    std::cout << "\nSimulation finished.\n";
    std::cout << "Results saved to " << filename << "\n";

    return 0;
}
