// Construction of Fat-tree Architecture
// Authors: Linh Vu, Daji Wong

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Nanyang Technological University 
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
 * Authors: Linh Vu <linhvnl89@gmail.com>, Daji Wong <wong0204@e.ntu.edu.sg>
 * Modifiedy by: Vipul Harsh <vipulharsh93@gmail.com>
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/random-variable.h"
#include "ns3/ipv4-routing-table-entry.h"

/*
	- Adapted from the paper "Towards Reproducible Performance Studies of Datacenter Network Architectures Using An Open-Source Simulation Approach"

	- The code is constructed in the following order:
		1. Creation of Node Containers from a topology file
		2. Initialize settings for On/Off Application/bulk.send application, traffic matrix read from a file
		3. Connect hosts to top of the rack switches
		4. Connect network switch using edges read from the topology file
		6. Start Simulation

	
	- Simulation Settings, along with examples:
		- Simulation running time: 100 seconds
		- Packet size: 1024 bytes
		- Data rate for packet sending: 1 Mbps
		- Data rate for device channel: 1000 Mbps
		- Delay time for device: 0.001 ms
		- Communication pairs selection: Random Selection with uniform probability
		- Traffic flow pattern: Exponential random traffic

        - Statistics Output:
                - Flowmonitor XML output file: Fat-tree.xml is located in the /statistics folder
            

*/

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("Fat-Tree-Architecture");

// Function to create address string from numbers
//
char * ipOctectsToString(int a,int b, int c, int d){

	int first = a;
	int second = b;
	int third = c;
	int fourth = d;

	char *address =  new char[30];
	char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];	
	//address = firstOctet.secondOctet.thirdOctet.fourthOctet;

	bzero(address,30);

	snprintf(firstOctet,10,"%d",first);
	strcat(firstOctet,".");
	snprintf(secondOctet,10,"%d",second);
	strcat(secondOctet,".");
	snprintf(thirdOctet,10,"%d",third);
	strcat(thirdOctet,".");
	snprintf(fourthOctet,10,"%d",fourth);

	strcat(thirdOctet,fourthOctet);
	strcat(secondOctet,thirdOctet);
	strcat(firstOctet,secondOctet);
	strcat(address,firstOctet);

	return address;
}

inline int truncateBytes(int bytes){
   return (bytes < 10? 0 : bytes);
}

// Main function
//
int 
	main(int argc, char *argv[])
{

   int nreqarg = 5;
   if(argc <= nreqarg){
      cout<<nreqarg-1<<" arguments required, "<<argc-1<<" given."<<endl;
      cout<<"Usage> <exec> <topology_file> <tm_file> <result_file> <drop queue limit(e.g. 100 for a limit of 100 packets)> <data rate for on/off (e.g. 10 for 10 Mbps) >"<<endl;
      exit(0);
   }
   string topology_filename = argv[1];
   string tm_filename = argv[2];
   string result_filename = argv[3];
   int drop_queue_limit = atoi(argv[4]);
   string on_off_datarate = argv[5];


   cout<<"Running topology: "<<topology_filename<<", output result to "<<result_filename<<endl;
   cout<<"Setting drop queue limit: "<<drop_queue_limit<<endl;

  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (drop_queue_limit));

//==== Simulation Parameters ====//
   double simTimeInSec = 10000.0;
	bool onOffApplication = false;	


//=========== Define topology  ===========//
//
   static int num_tor = 0;
   static int total_host = 0;	// number of hosts in the entire network	
   static vector<vector<int> > networkLinks;
   static vector<vector<int> > hostsInTor;
   static vector<vector<double> > serverTM;
   static map<int, int> hostToTor;
	//char filename [] = "statistics/File-From-Graph.xml";// filename for Flow Monitor xml output file
   //string topology_filename = "topology/ns3_deg4_sw8_svr8_os1_i1.edgelist";
   
   class topologyUtility{
      public:
         static void readServerTmFromFile(string tmfile){
            //< read TM from the TM File
            ifstream myfile(tmfile.c_str());
            string line;
            if (myfile.is_open()){
               string delimiter = ",";
               while(myfile.good()){
                  getline(myfile, line);
                  std::replace(line.begin(), line.end(), ',', ' ');  // replace ',' by ' '
                  vector<double> array;
                  stringstream ss(line);
                  double temp;
                  while (ss >> temp){
                      array.push_back(temp); 
                      //cout<<temp<<",";
                  }
                  serverTM.push_back(array);
                  //cout<<endl;
               }
               myfile.close();
            }
         }
         static void readTopologyFromFile(string topofile){
            //< read graph from the graphFile
            ifstream myfile(topofile.c_str());
            string line;
            vector<pair<int, int> > networkEdges, hostEdges;
            if (myfile.is_open()){
               while(myfile.good()){
                  getline(myfile, line);
                  if (line.find("->") == string::npos && line.find(" ") == string::npos) break;
                  if(line.find(" ") != string::npos){ //host --> rack link
                     assert(line.find("->") == string::npos);
                     int sw1 = atoi(line.substr(0, line.find(" ")).c_str());
                     int sw2 = atoi(line.substr(line.find(" ") + 1).c_str());
                     num_tor = max(max(num_tor, sw1+1), sw2+1); 
                     networkEdges.push_back(pair<int, int>(sw1, sw2));
                     //cout<<"(sw1, sw2, num_tor): "<<sw1<<", "<<sw2<<", "<<num_tor<<endl;
                  }
                  if(line.find("->") != string::npos){ //host --> rack link
                     assert(line.find(" ") == string::npos);
                     int host = atoi(line.substr(0, line.find("->")).c_str());
                     int rack = atoi(line.substr(line.find("->") + 2).c_str());
                     if(rack >= num_tor){
                        cout<<"Graph file has out of bounds nodes, "<<host<<"->"<<rack<<", NSW: "<<num_tor<<endl;
                        exit(0);
                     }
                     hostEdges.push_back(pair<int, int>(host, rack));
                     hostToTor[host] = rack;
                     total_host++;
                     //cout<<"(host, rack, num_tor): "<<host<<", "<<rack<<", "<<num_tor<<endl;
                  }
               }
               myfile.close();
            }
            cout<<"num_tor: "<<num_tor<<endl;
            cout<<"total_host: "<<total_host<<endl;
            cout<<"num_network_edges: "<<networkEdges.size()<<endl;
            cout<<"num_host_edges: "<<hostEdges.size()<<endl;
            networkLinks.resize(num_tor);
            hostsInTor.resize(num_tor);
            for(int i=0; i<networkEdges.size(); i++){
               pair<int, int> link = networkEdges[i];
               networkLinks[link.first].push_back(link.second);
            }
            for(int i=0; i<hostEdges.size(); i++){
               pair<int, int> hostlink = hostEdges[i];
               hostsInTor[hostlink.second].push_back(hostlink.first);
            }
            //sanity check
            for(int i=0; i<num_tor; i++){ 
               sort(hostsInTor[i].begin(), hostsInTor[i].end());
               for(int t=1; t<hostsInTor[i].size(); t++){
                  if(hostsInTor[i][t] != hostsInTor[i][t-1]+1){
                     cout<<"Hosts in Rack "<<i<<" are not contiguous"<<endl;
                     exit(0);
                  }
               }
            }
         }
         static pair<char*, char*> getLinkBaseIpAddress(int sw, int h){
            char* subnet = ipOctectsToString(10, (getFirstOctetOfTor(sw) | 128), getSecondOctetOfTor(sw), 0);
            char* base = ipOctectsToString(0, 0, 0, 2 + getNumHostsInRack(sw) + 2*h); 
            return pair<char*, char*>(subnet, base);
         }
         static char* getHostIpAddress(int host){
            int rack = getHostRack(host);
            int ind = getHostIndexInRack(host);
            return ipOctectsToString(10, getFirstOctetOfTor(rack), getSecondOctetOfTor(rack), ind*2 + 2);
         }
         static pair<char*, char*> getHostBaseIpAddress(int sw, int h){
            char* subnet = ipOctectsToString(10, getFirstOctetOfTor(sw), getSecondOctetOfTor(sw), 0);
            char* base = ipOctectsToString(0, 0, 0, (h*2) + 1); 
            return pair<char*, char*>(subnet, base);
         }
         static pair<int, int> getRackHostsLimit(int rack){ //rack contains hosts from [l,r) ===> return (l, r)
            pair<int, int> ret(0, 0);
            if(hostsInTor[rack].size() > 0){
              ret.first = hostsInTor[rack][0];
              ret.second = hostsInTor[rack].back()+1;
            }
            return ret;
         }
         static int getHostRack(int host){
            return hostToTor[host];
         }
         static int getNumHostsInRack(int rack){
            return hostsInTor[rack].size();
         }
         static int getHostIndexInRack(int host){
            return host - getRackHostsLimit(getHostRack(host)).first;
         }
         static int getFirstOctetOfTor(int tor){
            return tor/256;
         }
         static int getSecondOctetOfTor(int tor){
            return tor%256;
         }
   };

   topologyUtility::readTopologyFromFile(topology_filename);
   topologyUtility::readServerTmFromFile(tm_filename);

// Initialize parameters for On/Off application
//
	int port = 9; //port for background app
	int packetSize = 1024;		// 1024 bytes
	string dataRate_OnOff = on_off_datarate + "Mbps"; //"1Mbps";
	char maxBytes [] = "0" ; //"50000"; // "0";		// unlimited

// Initialize parameters for PointToPoint protocol
//
	char dataRate [] = "1000Mbps";	// 1Gbps
	float delay = 0.001;		// 0.001 ms


	
// Output some useful information
//	
	std::cout << "Total number of hosts =  "<< total_host<<"\n";
   std::cout << "On/Off flow data rate = "<< dataRate_OnOff<<"."<<endl;
	std::cout << "------------- "<<"\n";

// Initialize Internet Stack and Routing Protocols
//	
	InternetStackHelper internet;
	Ipv4NixVectorHelper nixRouting; 
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4GlobalRoutingHelper globalRouting;
	Ipv4ListRoutingHelper list;
	//list.Add (staticRouting, 0);	
	//list.Add (nixRouting, 10);	
	list.Add (globalRouting, 20);	
	internet.SetRoutingHelper(list);

//=========== Creation of Node Containers ===========//

	NodeContainer tors;				// NodeContainer for all ToR switches
   tors.Create(num_tor);
   internet.Install(tors);

   NodeContainer rackhosts[num_tor];
	for (int i=0; i<num_tor;i++){  	
		rackhosts[i].Create (hostsInTor[i].size());
		internet.Install (rackhosts[i]);		
	}

//=========== Initialize settings for On/Off Application ===========//
//

// Generate traffics for the simulation
//

   cout<<"Creating application flows .... "<<endl;

   double max_traffic = 0.0;
   for(int i=0; i<total_host; i++){
      //cout<<"row.size(): "<<serverTM[i].size()<<endl;
      max_traffic = max(max_traffic, *std::max_element(serverTM[i].begin(),serverTM[i].end()));
   }
   double traffic_wt = 0.1;
   cout<<"Max Traffic: "<<max_traffic<<", Weighted: "<<max_traffic * traffic_wt<<endl;

	ApplicationContainer** flow_app = new ApplicationContainer*[total_host];
   for(int i=0; i<total_host; i++)
      flow_app[i] = new ApplicationContainer[total_host];
   int nflows = 0, total_bytes=0;
   for(int i=0; i<total_host; i++){
      for(int j=0; j<total_host; j++){
         int bytes = truncateBytes((int)(serverTM[i][j] * traffic_wt));
         //cout<<"bytes: "<<bytes<<endl;
         if(bytes < 1) continue;
         nflows++;
         total_bytes += bytes;
         char* bytes_c = new char[50];
         sprintf(bytes_c,"%d",bytes);
         string bytes_s = bytes_c;
         char *add = topologyUtility::getHostIpAddress(i);

         OnOffHelper oo = OnOffHelper("ns3::TcpSocketFactory",Address(InetSocketAddress(Ipv4Address(add), port))); // ip address of server
         BulkSendHelper source ("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address(add), port)));
         // Initialize On/Off Application with addresss of server
         if(onOffApplication){
            cout<<"Initializing On Off flow"<<endl;
            double onTime = 0.010, offTime = 0.010;
            oo.SetAttribute("OnTime",RandomVariableValue(ExponentialVariable(onTime)));  
            oo.SetAttribute("OffTime",RandomVariableValue(ExponentialVariable(offTime))); 
            oo.SetAttribute("PacketSize",UintegerValue (packetSize));
            oo.SetAttribute("DataRate",StringValue (dataRate_OnOff));      
            oo.SetAttribute("MaxBytes",StringValue (bytes_c));
         }
         //Initialize BulkSend application
         else{
            // Set the amount of data to send in bytes.  Zero is unlimited.
            source.SetAttribute("MaxBytes",StringValue (bytes_c));
         }
         //cout<<i<<"-->"<<j<<std::endl;
         int jrack = topologyUtility::getHostRack(j);
         // Install On/Off Application to the client
         if(onOffApplication){
            NodeContainer onoff;
            onoff.Add(rackhosts[jrack].Get(topologyUtility::getHostIndexInRack(j)));
            flow_app[i][j] = oo.Install (onoff);
            //std::cout << "Finished creating On/Off traffic"<<"\n";
         }
         //For bulk send
         else{
            NodeContainer bulksend;
            bulksend.Add(rackhosts[jrack].Get(topologyUtility::getHostIndexInRack(j)));
            flow_app[i][j] = source.Install (bulksend);
            //flow_app[i][j].Start (Seconds (1.0));
            //flow_app[i][j].Stop (Seconds (simTimeInSec));
            //std::cout << "Finished creating Bulk send and Packet Sink Applications"<<"\n";
         }
      }
   }
   cout<<"Total Flows in the system: "<<nflows<<" carrying "<<total_bytes<<" bytes"<<endl;

   for(int i=0;i<total_host; i++){
      //Create packet sink application on every server
      PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
      int irack = topologyUtility::getHostRack(i);
      ApplicationContainer sinkApps = sink.Install (rackhosts[irack].Get(topologyUtility::getHostIndexInRack(i)));
      sinkApps.Start (Seconds (0.0));
      sinkApps.Stop (Seconds (simTimeInSec));
   }

   cout<<"Finished creating applications"<<endl;

   
// Inintialize Address Helper
//	
  	Ipv4AddressHelper address;

// Initialize PointtoPoint helper
//	
	PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  	p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));

//=========== Connect switches to hosts ===========//
//

	vector<NetDeviceContainer> sh[num_tor]; 	
	vector<Ipv4InterfaceContainer> ipShContainer[num_tor];
	for(int i=0; i<num_tor; i++){
		sh[i].resize(topologyUtility::getNumHostsInRack(i));
		ipShContainer[i].resize(topologyUtility::getNumHostsInRack(i));
	}
   for (int i=0;i<num_tor;i++){
      for (int h=0; h<topologyUtility::getNumHostsInRack(i); h++){			
         sh[i][h] = p2p.Install(tors.Get(i), rackhosts[i].Get(h));
         //Assign subnet
         pair<char* , char*> subnet_base = topologyUtility::getHostBaseIpAddress(i, h);
         char *subnet = subnet_base.first;
         char *base =subnet_base.second;
         address.SetBase (subnet, "255.255.255.0",base);
         ipShContainer[i][h] = address.Assign(sh[i][h]);
      }
   }
	std::cout << "Finished connecting tors and hosts  "<< "\n";

//=========== Connect  switches to switches ===========//
//
	vector<NetDeviceContainer> ss[num_tor]; 	
	vector<Ipv4InterfaceContainer> ipSsContainer[num_tor];
	for(int i=0; i<num_tor; i++){
		ss[i].resize(networkLinks[i].size());
		ipSsContainer[i].resize(networkLinks[i].size());
	}
	for (int i=0;i<num_tor;i++){
			for (int h=0;h<networkLinks[i].size();h++){
				int nbr = networkLinks[i][h];
				ss[i][h] = p2p.Install(tors.Get(i), tors.Get(nbr));
				//Assign subnet
				pair<char* , char*> subnet_base = topologyUtility::getLinkBaseIpAddress(i, h);
				char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            //cout<<"Network Link; "<<i<<" <--> "<<nbr<< " Nodecontainer.size: "<<ss[i][h].GetN() << endl;
            //cout<<subnet<<" , "<<base<<endl;
				address.SetBase (subnet, "255.255.255.0",base);
				ipSsContainer[i][h] = address.Assign(ss[i][h]);
			}			
	}
	std::cout << "Finished connecting switches and switches  "<< "\n";
	std::cout << "------------- "<<"\n";

//=========== Start the simulation ===========//
//

   //Check if ECMP routing is working 
	//Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
   cout<<"Populating routing tables:"<<endl;
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

   Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
   //for(int i=0; i<num_tor; i++)
   //   globalRouting.PrintRoutingTableAt (Seconds (5.0), tors.Get(i), routingStream);
   //for(int i=0; i<num_tor; i++)
   //   globalRouting.PrintRoutingTableAt (Seconds (10.0), tors.Get(i), routingStream);

   std::cout << "Start Simulation.. "<<"\n";
   for (int i=0;i<total_host;i++){
      for (int j=0;j<total_host;j++){
         int bytes = truncateBytes((int)(serverTM[i][j] * traffic_wt));
         if(bytes < 1) continue;
		   //flow_app[i][j].Start (Seconds (1.0));
		   flow_app[i][j].Start (Seconds (rand() * 100.0/RAND_MAX));
  		   flow_app[i][j].Stop (Seconds (simTimeInSec));
      }
	}

// Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
// Run simulation.
//
  	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(simTimeInSec+1.0));
  	Simulator::Run ();

  	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile(result_filename, true, true);

	std::cout << "Simulation finished "<<"\n";

  	Simulator::Destroy ();
  	NS_LOG_INFO ("Done.");

	return 0;
}




