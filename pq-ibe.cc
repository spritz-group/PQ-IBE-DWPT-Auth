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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/vector.h"
#include "ns3/string.h"
#include "ns3/socket.h"
#include "ns3/double.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/command-line.h"
#include "ns3/mobility-model.h"
#include "ns3/constant-velocity-helper.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include <iostream>

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PBIBESimulation");

void PrintInfo() {
	Ptr<Node> n0 = ns3::NodeList::GetNode(7);
	Ptr<MobilityModel> m0 = n0->GetObject<MobilityModel> ();

	std::cout << "n0 Vel:" << m0->GetVelocity() << std::endl;
	std::cout << "n0 Vel:" << m0->GetPosition() << std::endl;

	Simulator::Schedule(Seconds(1), &PrintInfo);
}

// TODO: Get time that this finishes to start the next client
// TODO: how to send only one message in UdpEcho
// TODO: with movement

int main(int argc, char *argv[]) {
	
	std::string phyMode("OfdmRate6MbpsBW10MHz");
	bool verbose = true;
	uint32_t nPads = 1;
	uint32_t speed = 10;
	uint32_t port;
	double starting_time_chain;

	// parse arguments from command line
	CommandLine cmd(__FILE__);
	cmd.AddValue("nPads", "Number of Pads", nPads);
	cmd.AddValue("v", "Velocity of the vehicle", speed);
	cmd.AddValue("verbose", "Tell applications to log if true", verbose);

	cmd.Parse(argc, argv);

	if (verbose) {
        LogComponentEnable("PBIBESimulation", LOG_LEVEL_INFO);
        LogComponentEnable("PBIBESimulation", LOG_LEVEL_INFO);
		Time::SetResolution(Time::NS);
		LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

	// Time::SetResolution(Time::NS);	

	// create nNodes CPs and RSU in wired network, RSU is the first node
	NodeContainer secondLayerNodes;
	secondLayerNodes.Create(nPads+1);

	/* Create the link between the nodes */
	CsmaHelper secondLayerP2P;
	secondLayerP2P.SetChannelAttribute("DataRate", StringValue("100Mbps"));
	secondLayerP2P.SetChannelAttribute("Delay", StringValue("0.345ms")); // half for delay in app 

	NetDeviceContainer secondLayerDevices;
	secondLayerDevices = secondLayerP2P.Install(secondLayerNodes);

	// create nodes for RSU and CSPA communication
	// one is sufficient, the other is RSU from secondLayerNodes
	NodeContainer firstLayerNodes;
	// firstLayerNodes.Add(secondLayerNodes.Get(0)); // RSU
	firstLayerNodes.Create(1); // RSU
	firstLayerNodes.Create(1); // CSPA

	CsmaHelper firstLayerP2P; // csma is like Eth bus
	firstLayerP2P.SetChannelAttribute("DataRate", StringValue("100Mbps")); 
	firstLayerP2P.SetChannelAttribute("Delay", StringValue("0.345ms")); // half due to two packets exchange in app

	NetDeviceContainer firstLayerDevices;
	firstLayerDevices = firstLayerP2P.Install(firstLayerNodes);

	// create nodes for each of the two 5G communication channels for EV-CSPA
	NodeContainer channel5GNodesOne;
	// channel5GNodesOne.Add(firstLayerNodes.Get(1)); // CSPA
	channel5GNodesOne.Create(1); // CSPA
	channel5GNodesOne.Create(1); // EV

	// modeled as P2P for simplicity
	PointToPointHelper channel5GP2POne;
	channel5GP2POne.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	channel5GP2POne.SetChannelAttribute("Delay", StringValue("139.36ms"));

	NetDeviceContainer channel5GOneDevices;
	channel5GOneDevices = channel5GP2POne.Install(channel5GNodesOne);

	// create nodes for each of the two 5G communication channels for EV-RSU
	NodeContainer channel5GNodesTwo;
	channel5GNodesTwo.Create(1); // RSU
	channel5GNodesTwo.Create(1); // EV

	// modeled as P2P for simplicity
	PointToPointHelper channel5GP2PTwo;
	channel5GP2PTwo.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	channel5GP2PTwo.SetChannelAttribute("Delay", StringValue("0.33ms"));

	NetDeviceContainer channel5GTwoDevices;
	channel5GTwoDevices = channel5GP2PTwo.Install(channel5GNodesTwo);

	// network DSRC channel between EV and CPs
	// Setting up the DSRC Channel
	NodeContainer dsrcNodes;
	dsrcNodes.Create(1); // EV
	NodeContainer::Iterator i;
	for (uint32_t i = 1; i < secondLayerNodes.GetN(); i++) {
  		dsrcNodes.Create(1);
	}
	// modeled as P2P for simplicity
	CsmaHelper dsrcChannel;
	dsrcChannel.SetChannelAttribute("DataRate", StringValue("27Mbps"));
	dsrcChannel.SetChannelAttribute("Delay", StringValue("0.36ms"));

	NetDeviceContainer dscrDevices;
	dscrDevices = dsrcChannel.Install(dsrcNodes);

	// YansWifiPhyHelper dsrcPhy;
	// YansWifiChannelHelper dsrcChannel = YansWifiChannelHelper::Default();
  	// Ptr<YansWifiChannel> channel = dsrcChannel.Create();
	// dsrcPhy.SetChannel(channel);
	// dsrcPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);

	// NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default();
	// Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default();
	// // if(verbose) {
	// // 	wifi80211p.EnableLogComponents();      // Turn on all Wifi 802.11p logging
	// // } 
	// wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
	// 									"DataMode",StringValue (phyMode),
	// 								   "ControlMode",StringValue (phyMode));
	// NetDeviceContainer dscrDevices = wifi80211p.Install (dsrcPhy, wifi80211pMac, dsrcNodes);

	

	/* Setting up mobility for the RSU and CPs */
	MobilityHelper mobilityFixSecondLayer;
	double l = 0;
	for (uint32_t i = 0; i < nPads; i++) {
		l = l + 2.5;
		Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  		positionAlloc->Add(Vector(l, 0.0, 0.0)); // CPs
		mobilityFixSecondLayer.SetPositionAllocator(positionAlloc);
		mobilityFixSecondLayer.SetMobilityModel("ns3::ConstantPositionMobilityModel");
		mobilityFixSecondLayer.Install(dsrcNodes.Get(i+1));
	}
	// mobilityFixSecondLayer.SetPositionAllocator(positionAlloc);
	// mobilityFixSecondLayer.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	// mobilityFixSecondLayer.Install(dsrcNodes);

	/* Setting mobility model for CSPA */
	MobilityHelper mobilityFixFirstLayer;
	Ptr<ListPositionAllocator> positionAllocFirstLayer = CreateObject<ListPositionAllocator>();
    positionAllocFirstLayer->Add(Vector(5.0, 10.0, 0.0)); // CSPA
	mobilityFixFirstLayer.SetPositionAllocator(positionAllocFirstLayer);
	mobilityFixFirstLayer.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityFixFirstLayer.Install(firstLayerNodes.Get(0));

	/* Mobility for EV --> changing position over the pads */
	MobilityHelper evMobility;
	evMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
	evMobility.Install(dsrcNodes.Get(0));

	Ptr<ConstantVelocityMobilityModel> ev = DynamicCast<ConstantVelocityMobilityModel>(dsrcNodes.Get(0)->GetObject<MobilityModel>());
	ev->SetPosition(Vector(0.0, 0.0, 0.0)); // Initial Position
	ev->SetVelocity(Vector(speed, 0.0, 0.0));

	// // generalize for position of pads (as points)
	// // iterate our nodes and print their position.
    // for (NodeContainer::Iterator j = secondLayerNodes.Begin(); j != secondLayerNodes.End(); ++j) {
    //     Ptr<Node> object = *j;
    //     Ptr<MobilityModel> position = object->GetObject<MobilityModel>();
    //     // NS_ASSERT(position);
    //     Vector pos = position->GetPosition();
	// 	std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
	// }
	
	// std::cout << "------------------------------------------------" << std::endl;

	// for (NodeContainer::Iterator j = firstLayerNodes.Begin(); j != firstLayerNodes.End(); ++j) {
    //     Ptr<Node> object = *j;
    //     Ptr<MobilityModel> position = object->GetObject<MobilityModel>();
    //     // NS_ASSERT(position);
    //     Vector pos = position->GetPosition();
	// 	std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
	// }

	// Internet stack for UDP applicaiton in order to send messages
	// for each node
	// Stack CSPA - EV
	InternetStackHelper stackCspaEV;
    stackCspaEV.Install(channel5GNodesOne);

    Ipv4AddressHelper addressCspaEV;
    addressCspaEV.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesCspaEV = addressCspaEV.Assign(channel5GOneDevices);

	// Stack RSU - CPs
	InternetStackHelper stackRsuCPs;
    stackRsuCPs.Install(secondLayerNodes);

    Ipv4AddressHelper addressRsuCPs;
    addressRsuCPs.SetBase("10.1.4.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesRsuCPs = addressRsuCPs.Assign(secondLayerDevices);

	// Stack EV - RSU
	InternetStackHelper stackEvRSU;
    stackEvRSU.Install(channel5GNodesTwo);

    Ipv4AddressHelper addressEvRSU;
    addressEvRSU.SetBase("10.1.3.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesEvRSU = addressEvRSU.Assign(channel5GTwoDevices);

	// Stack CSPA - RSU
	InternetStackHelper stackCspaRSU;
    stackCspaRSU.Install(firstLayerNodes);

    Ipv4AddressHelper addressCspaRSU;
    addressCspaRSU.SetBase("10.1.2.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesCspaRSU = addressCspaRSU.Assign(firstLayerDevices);

	// Stack EV - CPs
	InternetStackHelper stackEvCPs;
    stackEvCPs.Install(dsrcNodes);

    Ipv4AddressHelper addressEvCPs;
    addressEvCPs.SetBase("10.1.5.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesEvCPs = addressEvCPs.Assign(dscrDevices);


	/* Application layer exchange to mimic the delay of the computation */

	// Application layer CSPA - EV
    UdpEchoServerHelper echoServerCspaEV(9); // (port) as parameter

    ApplicationContainer serverAppsCspaEV = echoServerCspaEV.Install(channel5GNodesOne.Get(0)); // CSPA
    serverAppsCspaEV.Start(Seconds(0.0));
    serverAppsCspaEV.Stop(Seconds(60.0));

    UdpEchoClientHelper echoClientCspaEV(interfacesCspaEV.GetAddress(0), 9);
    echoClientCspaEV.SetAttribute("MaxPackets", UintegerValue(1));
    echoClientCspaEV.SetAttribute("PacketSize", UintegerValue(128)); // Bytes for m1-m2

    ApplicationContainer clientAppsCspaEV = echoClientCspaEV.Install(channel5GNodesOne.Get(1)); // EV
    clientAppsCspaEV.Start(Seconds(0.0));
    clientAppsCspaEV.Stop(Seconds(60.0));

	// Application layer CSPA - RSU
	UdpEchoServerHelper echoServerCspaRSU(10); // (port) as parameter

    ApplicationContainer serverAppsCspaRSU = echoServerCspaRSU.Install(firstLayerNodes.Get(1)); // CSPA
    serverAppsCspaRSU.Start(Seconds(0.0));
    serverAppsCspaRSU.Stop(Seconds(60.0));

    UdpEchoClientHelper echoClientCspaRSU(interfacesCspaRSU.GetAddress(1), 10);
    echoClientCspaRSU.SetAttribute("MaxPackets", UintegerValue(1));
    echoClientCspaRSU.SetAttribute("PacketSize", UintegerValue(128)); // Bytes for m3 and response

    ApplicationContainer clientAppsCspaRSU = echoClientCspaRSU.Install(firstLayerNodes.Get(0)); // RSU
    clientAppsCspaRSU.Start(Seconds(0.279));
    clientAppsCspaRSU.Stop(Seconds(60.0));

	// UDP application EV - RSU
	UdpEchoServerHelper echoServerEvRSU(11); // (port) as parameter

    ApplicationContainer serverAppsEvRSU = echoServerEvRSU.Install(channel5GNodesTwo.Get(1)); // RSU
    serverAppsEvRSU.Start(Seconds(0.0));
    serverAppsEvRSU.Stop(Seconds(60.0));

    UdpEchoClientHelper echoClientEvRSU(interfacesEvRSU.GetAddress(1), 11);
    echoClientEvRSU.SetAttribute("MaxPackets", UintegerValue(1));
    echoClientEvRSU.SetAttribute("PacketSize", UintegerValue(96)); // Bytes for m4-m5

    ApplicationContainer clientAppsEvRSU = echoClientEvRSU.Install(channel5GNodesTwo.Get(0)); // EV
    clientAppsEvRSU.Start(Seconds(0.280));
    clientAppsEvRSU.Stop(Seconds(60.0));

	// Application layer RSU - CPs
	UdpEchoServerHelper echoServerRsuCPs(12); // (port) as parameter

    ApplicationContainer serverAppsRsuCPs = echoServerRsuCPs.Install(secondLayerNodes.Get(1)); // First CP
    serverAppsRsuCPs.Start(Seconds(0.0));
    serverAppsRsuCPs.Stop(Seconds(60.0));

    UdpEchoClientHelper echoClientRsuCPs(interfacesRsuCPs.GetAddress(1), 12);
    echoClientRsuCPs.SetAttribute("MaxPackets", UintegerValue(1));
    echoClientRsuCPs.SetAttribute("PacketSize", UintegerValue(32)); // Bytes for m6 and response

    ApplicationContainer clientAppsRsuCPs = echoClientRsuCPs.Install(secondLayerNodes.Get(0)); // RSU (as client sending message)
    clientAppsRsuCPs.Start(Seconds(0.281));
    clientAppsRsuCPs.Stop(Seconds(60.0));

	// Message between EV and CPs: m7 to m9
	// starting time is on average the time taken for the other messages
	starting_time_chain = 0.285+nPads*0.00036; // taking into account time for building the hash chain
	for(uint32_t i = 0; i < nPads; i++) {
		port = 15+i;
		UdpEchoServerHelper echoServerEvCPs(port); // (port) as parameter

		ApplicationContainer serverAppsEvCPs = echoServerEvCPs.Install(dsrcNodes.Get(i+1)); // First CP
		serverAppsEvCPs.Start(Seconds(0.0));
		serverAppsEvCPs.Stop(Seconds(60.0));

		UdpEchoClientHelper echoClientEvCPs(interfacesEvCPs.GetAddress(i+1), port);
		echoClientEvCPs.SetAttribute("MaxPackets", UintegerValue(1));
		echoClientEvCPs.SetAttribute("PacketSize", UintegerValue(32)); // Bytes for m6 and response

		ApplicationContainer clientAppsEvCPs = echoClientEvCPs.Install(dsrcNodes.Get(0)); // EV (as client sending message)
		clientAppsEvCPs.Start(Seconds(starting_time_chain+i*0.00036));
		clientAppsEvCPs.Stop(Seconds(60.0));
	}


	// Simulator::Schedule(Seconds(1), &PrintInfo);
	// Simulator::Stop(Seconds(60.0));
	Simulator::Run();
    Simulator::Destroy();
    return 0;

}
