/************************************************************************************
 * Copyright (C) 2016                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Kanagaraj K                                                           *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/

 /**********************************IMPORTANT NOTES***********************************
 1. This file contains the PEGASIS code.
 2. The Network scenario can contain any number of sensors
 3. Mobility can be set to sensors by setting velocity which is set to NO_MOBILITY by default
 ************************************************************************************/


#include "main.h"
#include "DSR.h"
#include "List.h"
#include "Stack.h"
#include "../BatteryModel/BatteryModel.h"
#include "../ZigBee/802_15_4.h"

int *ClusterElements;
int CH = 0;
int CL_SIZE = 0;
int CL_POSITION = 0;

int fn_NetSim_PEGASIS_CheckDestination(NETSIM_ID nDeviceId, NETSIM_ID nDestinationId)
//Function to check whether the Device ID is same as the Destination ID
{
	if (nDeviceId == nDestinationId)
		return 1;
	else
		return 0;
}

int fn_NetSim_PEGASIS_GetNextHop(NetSim_EVENTDETAILS* pstruEventDetails)
//Function to determine the DeviceId of the next hop
{
	int nextHop = 0;
	NETSIM_ID nInterface;
	int i;
	int ClusterId;
	int cl_flag = 0;
	double energy = 0.0;
	//Static Routes defined for 4 Clusters.
	//If the sensor is the Cluster Head, it forwards it to the Sink.
	//Otherwise, it forwards the packet to the Cluster Head of its cluster.
	energy = battery_get_remaining_energy((ptrBATTERY)WSN_PHY(pstruEventDetails->nDeviceId)->battery);
	if (energy <= 0.0) return 0;
	repeat:
	if (pstruEventDetails->nDeviceId == CH)
	{

		nextHop = get_first_dest_from_packet(pstruEventDetails->pPacket);
	}
	else
	{
		nextHop = fn_NetSim_PEGASIS_next_closest_node(pstruEventDetails->nDeviceId);
	}
	fprintf(stderr, "\nTx ID: %d	Next Hop ID: %d	Packet ID: %d Head Node ID: %d\n",pstruEventDetails->nDeviceId, DEVICE_CONFIGID(nextHop),pstruEventDetails->pPacket->nPacketId,CH);

	if (WSN_MAC(nextHop)->nNodeStatus == OFF)
	{
		fprintf(stderr, "\nNode already dead\n");
		
		fn_NetSim_PEGASIS_Init();
		fn_NetSim_PEGASIS_run();
		goto repeat;
	}

		//Updating the Transmitter ID, Receiver ID and NextHopIP in the pstruEventDetails
	free(pstruEventDetails->pPacket->pstruNetworkData->szNextHopIp);
	pstruEventDetails->pPacket->pstruNetworkData->szNextHopIp = dsr_get_dev_ip(nextHop);
	pstruEventDetails->pPacket->nTransmitterId = pstruEventDetails->nDeviceId;
	pstruEventDetails->pPacket->nReceiverId = nextHop;


	return 1;
}

int fn_NetSim_PEGASIS_run()
{
	if(sensor_count>0)
	fn_netsim_matlab_run(sensor_count);
	return 1;
}

int fn_NetSim_PEGASIS_form_cluster(double* cl_head, double* r_order, int* temp)
//Cluster heads are assigned to respective clusters using the data obtained from MATLAB
{
	int i = 0;
	CH = temp[(int)(cl_head[0]-1)];
	
	for (i = 0; i < sensor_count; i++)
	{
		ClusterElements[i] = temp[(int)(r_order[i]-1)];
		if (ClusterElements[i] == CH)
			CL_POSITION = i;
	}

		return 1;
}

void fn_NetSim_PEGASIS_Init()
{
	int i = 0;
	sensor_count = 0;
		
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		
		if (!strcmp(DEVICE(i + 1)->type, "SENSOR") && WSN_MAC(i + 1)->nNodeStatus != OFF)
		{
			double energy = battery_get_remaining_energy((ptrBATTERY)WSN_PHY(i + 1)->battery);
			if (energy > 0.0)
				sensor_count++;
		}
	}
	if(sensor_count>0)
	ClusterElements = (int*)calloc(sensor_count, sizeof*(ClusterElements));
}

int fn_NetSim_PEGASIS_next_closest_node(int dev_id)
{
	int i = 0;
	int index = 0;
	for (i = 0; i < sensor_count; i++)
	{
		if (ClusterElements[i] == dev_id)
			index = i;
	}
	if (index < CL_POSITION)
		return ClusterElements[index + 1];
	else
		return ClusterElements[index - 1];
	
}

