/*
 *
 *	This is a simple program that illustrates how to call the MATLAB
 *	Engine functions from NetSim C Code.
 *
 */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "mat.h"
#include "mex.h"
#include "main.h"
#include "../ZigBee/802_15_4.h"
#include "../BatteryModel/BatteryModel.h"
#include "direct.h"
#include "DSR.h"


char buf[BUFSIZ];
Engine *ep;
int status;
mxArray *Did = NULL, *NDid = NULL, *Xc = NULL, *Yc = NULL, *Pwr = NULL, *turn_in=NULL, *max_energy=NULL;
//double **d_id, **x, **y, **p;
mxArray *out;
double *result[3];
int dim = 0;
int rnd = 1;
double fn_netsim_matlab_init()
{
	/*
	 * Start the MATLAB engine Process
	 */
	fprintf(stderr, "\nPress any key to start MATLAB...\n");
	_getch();
	if (!(ep = engOpen(NULL))) {
		MessageBox((HWND)NULL, (LPCWSTR)"Can't start MATLAB engine",
			(LPCWSTR) "MATLAB_Interface.c", MB_OK);
		exit(-1);
	}

	engEvalString(ep, "desktop");
	//specify the path of the DSR folder where the clustering.m file is present
	double temp = 0.0, maxenergy = 0.0;
	int i = 0;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		if (!strcmp(DEVICE(i + 1)->type, "SENSOR") && WSN_MAC(i + 1)->nNodeStatus != OFF)
		{
			temp = battery_get_remaining_energy((ptrBATTERY)WSN_PHY(i + 1)->battery);
			if (temp > maxenergy)
				maxenergy = temp;
		}
	}
	engPutVariable(ep, "max_energy", max_energy);
	sprintf(buf, "max_energy=%f", maxenergy);
	engEvalString(ep, buf);
	sprintf(buf, "fid = fopen('log.txt','w+'); fclose(fid);");
	status = engEvalString(ep, buf);
	engPutVariable(ep, "turn_in", turn_in);
	engEvalString(ep, "turn_in=0");
	sprintf(buf, "scrsz = get(groot, 'ScreenSize')");
	engEvalString(ep, buf);
	sprintf(buf, "fig1 = gcf");
	engEvalString(ep, buf);
	sprintf(buf, "fig1.Position = [scrsz(3) / 5 scrsz(4) / 4 scrsz(3) / 1.5 scrsz(4) / 2]");
	engEvalString(ep, buf);
	return 0;
}

void fn_netsim_matlab_run()
{
	#define net_size sensor_count
	int i = 0, sinkx = 0, sinky = 0;
	double *route_order = NULL, *cluster_head = NULL;
	int *temp_order=NULL;
	dim = sensor_count;
	int grid_length;
	FILE *fp = NULL;
	char filename[BUFSIZ];
	strcpy(filename, pszAppPath);
	strcat(filename, "\\netsim_input.csv");	
	fp = fopen(filename, "w+");
	if (fp)
	{
		fprintf(fp, "Device_Number,X,Y,Remaining Energy(mJ),Device_Id\n");
		fclose(fp);
	}

	char* temp = NULL;
	temp = getenv("NETSIM_SIM_AREA_X");
	if (temp)
	{
		if (atoi(temp))
			grid_length = atoi(temp);
		else
			grid_length = 0xFFFFFFFF;
	}
	

	temp_order = (int*)calloc(sensor_count, sizeof*(temp_order));
	int devid = 0;
	fp = fopen(filename, "a+");
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		if (!strcmp(DEVICE(i + 1)->type, "SINKNODE"))
		{
			sinkx = DEVICE_POSITION(i + 1)->X;
			sinky = DEVICE_POSITION(i + 1)->Y;
			continue;
		}
		double energy = battery_get_remaining_energy((ptrBATTERY)WSN_PHY(i + 1)->battery);
		if (!strcmp(DEVICE(i + 1)->type, "SENSOR") && WSN_MAC(i + 1)->nNodeStatus != OFF && energy>0.0)
		{	
			if (fp)
				fprintf(fp, "%d,%f,%f,%f,%d\n", (devid+1), DEVICE_POSITION(i + 1)->X, DEVICE_POSITION(i + 1)->Y,
				energy,(i+1));
			temp_order[devid] = (i + 1);
			devid++;
		}
		
	}
	if (fp)
		fclose(fp);

	sprintf(buf, "Did=csvread('%s',1,0,[1,0,%d,0])",filename, sensor_count);
	engEvalString(ep, buf);
	sprintf(buf, "Xc=csvread('%s',1,1,[1,1,%d,1])", filename, sensor_count);
	engEvalString(ep, buf);
	sprintf(buf, "Yc=csvread('%s',1,2,[1,2,%d,2])", filename, sensor_count);
	engEvalString(ep, buf);
	sprintf(buf, "Pwr=csvread('%s',1,3,[1,3,%d,3])", filename, sensor_count);
	engEvalString(ep, buf);
	sprintf(buf, "NDid=csvread('%s',1,4,[1,4,%d,4])", filename, sensor_count);
	engEvalString(ep, buf);	
	sprintf(buf, "[route_order,cl_head,turn_in]=PEGASIS(Did,NDid,Xc,Yc,Pwr,%d,%d,%d,%d,%d,turn_in,max_energy)", grid_length, sensor_count, sinkx, sinky, rnd);
	status = engEvalString(ep, buf);
	rnd++;
	out = engGetVariable(ep, "route_order");//contains the order of devices in the chain
	route_order = mxGetPr(out);

	out = engGetVariable(ep, "cl_head");//contains the cluster head id
	cluster_head = mxGetPr(out);
	fn_NetSim_PEGASIS_form_cluster(cluster_head, route_order, temp_order);
}

double fn_netsim_matlab_finish()
{
	//Close the MATLAB Engine Process
	fprintf(stderr, "\nPress any key to close MATLAB...\n");
	_getch();
	status = engEvalString(ep, "exit");
	return 0;
}

