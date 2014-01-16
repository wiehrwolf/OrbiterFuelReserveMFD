// ==============================================================
//                 ORBITER MODULE: FuelMFD
//           Copyright (C) 2006 Christian Wiehr
//                   All rights reserved
//
// FuelMFD.cpp
// Displays remaining fuel and time until tanks are empty
// ==============================================================

#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "orbitersdk.h"
#include "FuelMFD.h"

// ==============================================================
// Global variables

static struct {  // "Fuel  MFD" parameters
	int mode;      // identifier for new MFD mode
} g_FuelMFD;

// ==============================================================
// API interface

DLLCLBK void opcDLLInit (HINSTANCE hDLL)
{
	static char *name = "Fuel Status";
	MFDMODESPEC spec;
	spec.name    = name;
	spec.key     = OAPI_KEY_E;
	spec.msgproc = FuelMFD::MsgProc;

	g_FuelMFD.mode = oapiRegisterMFDMode (spec);
}

DLLCLBK void opcDLLExit (HINSTANCE hDLL)
{
	oapiUnregisterMFDMode (g_FuelMFD.mode);
}


// ==============================================================
// Fuel MFD implementation

FuelMFD::FuelMFD (DWORD w, DWORD h, VESSEL *vessel)
: MFD (w, h, vessel)
{
	LINE = h/25;
	CurrentLine = 1*LINE; // 1st line
	page = 0;
	disp_mode = 0; // time units in sec (1) or min&sec (0)
	MAX_LINES = h/LINE;
	//VESSEL *v;
}

FuelMFD::~FuelMFD ()
{

}

void FuelMFD::InitReferences (void)
{

}

// message parser
int FuelMFD::MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
		return (int)(new FuelMFD (LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam));
	}
	return 0;
}

bool FuelMFD::ConsumeKeyBuffered (DWORD key)
{
	switch (key) {
	case OAPI_KEY_P:
		if (page>0) page--;
		return true;
	case OAPI_KEY_N:
		if (page<maxpage) page++;
		return true;
	case OAPI_KEY_M:
		disp_mode = (disp_mode+1)%2;
		return true;
	}
	return false;
}

bool FuelMFD::ConsumeButton (int bt, int event)
{
	if (!(event & PANEL_MOUSE_LBDOWN)) return false;
	static const DWORD btkey[3] = { OAPI_KEY_P, OAPI_KEY_N, OAPI_KEY_M };
	if (bt < 3) return ConsumeKeyBuffered (btkey[bt]);
	else return false;
}

char *FuelMFD::ButtonLabel (int bt)
{
	char *label[3] = {"PRV", "NXT", "MOD"};
	return (bt < 3 ? label[bt] : 0);
}

int FuelMFD::ButtonMenu (const MFDBUTTONMENU **menu) const
{
	static const MFDBUTTONMENU mnu[3] = {
		{"Previous page", 0, 'P'},
		{"Next page", 0, 'N'},
		{"Switch time format", 0, 'M'},
	};
	if (menu) *menu = mnu;
	return 3;
}

char FuelMFD::PotToSuffix (int i)
{
	switch (i) {
	case 1: // 1000^1; kilo
		return 'k';
	case 2: // 1000^2; Mega
		return 'M';
	case 3: // 1000^3; Giga
		return 'G';
	case 4: // 1000^4; Tera
		return 'T';
	case 5: // 1000^5; Peta
		return 'P';
	case 6: // 1000^6; Exa
		return 'E';
	case 7: // 1000^7; Zetta
		return 'Z';
	case 8: // 1000^8; Yotta
		return 'Y';
	}
	return ' ';
}

void FuelMFD::FuelBar (HDC hDC, int tank_nr, int cur_line)
{
	char buffer[100];
	int bar_parts; //number of parts in the bar
	char bar[30];
	double tank_percent=100;
	double prop_mass; // kg
	double prop_max_mass; // kg
	double prop_flowrate; // kg/sec
	unsigned long long remaining_time; // seconds
	unsigned long long rem_time_sec; // mode 0
	unsigned long long rem_time_min; // mode 0
	long double rem_sec; // mode 1
	int sffx=0; // for rem_time_sec*1000^sffx sec

	prop_mass = v->GetPropellantMass(v->GetPropellantHandleByIndex(tank_nr-1));
	prop_max_mass = v->GetPropellantMaxMass(v->GetPropellantHandleByIndex(tank_nr-1));
	prop_flowrate = v->GetPropellantFlowrate(v->GetPropellantHandleByIndex(tank_nr-1));

	tank_percent = prop_mass / prop_max_mass * 100;
	if (prop_flowrate>0) remaining_time = prop_mass / prop_flowrate; 
	else if (prop_flowrate<0) remaining_time = - (prop_max_mass-prop_mass) / prop_flowrate; // refueling
	else remaining_time = 0;

	if (disp_mode==0)
	{
		rem_time_min = remaining_time / 60;
		rem_time_sec = remaining_time % 60;
	} else
	{
		sffx = remaining_time / 1000;
		if (sffx<=8) // ok, we got prefixes for that
			rem_sec = remaining_time*1.0 / (1000^sffx);
		else { // damn, too large :-/
			sffx = 8;
			rem_sec = remaining_time*1.0 / (1000^sffx);
		}
	}

	bar_parts = tank_percent/4; //max. lenght of the bar is 25 => 25parts * 4% = 100%
	if (bar_parts>25) bar_parts=25;
	if (bar_parts<0) bar_parts=0;

	for (int i=0; i<bar_parts; i++)
		if (i==12) bar[i]='|'; else bar[i]='='; // build up bar
	bar[bar_parts]='\0'; // Terminate string

	// --- Begin output ---
	
	SetTextColor(hDC, RGB(0,255,0));

	sprintf_s(buffer,"Fuel Tank #%i",tank_nr);
	TextOut(hDC, 5, cur_line, buffer, strlen(buffer));
	cur_line += LINE; //Next Line

	if ((tank_percent<=5) || ((prop_flowrate>0) && ( ((disp_mode==0)&&(rem_time_min==0)) || ((disp_mode==1)&&(sffx==0)&&(rem_time_sec<60)))))
		SetTextColor(hDC, RGB(255,0,0)); 
	//Alert! Fuel<=5% or (remaining time < 60 sec (mode 0) or remaining time < 60 s (mode 1; i==0) )
	sprintf_s(buffer,"%5.1f%% [%-25s]",tank_percent,bar);
	TextOut(hDC, 5, cur_line, buffer, strlen(buffer));

	cur_line += LINE;
	if (prop_flowrate>0) 
		if (disp_mode==0)
			sprintf_s(buffer,"Empty in %i min %02i sec",rem_time_min,rem_time_sec);
		else
			sprintf_s(buffer,"Empty in %.3f%cs",rem_sec,PotToSuffix(sffx));
	else if (prop_flowrate<0) 
	{
			SetTextColor(hDC, RGB(0,255,0)); // for full tank, we don't need a warning
			if (disp_mode==0)
				sprintf_s(buffer,"Full in %i min %02i sec",rem_time_min,rem_time_sec);
			else
				sprintf_s(buffer,"Full in %.3f%cs",rem_sec,PotToSuffix(sffx));
	}
	else sprintf_s(buffer,"\0");
	TextOut(hDC, 5, cur_line, buffer, strlen(buffer));
	SetTextColor(hDC, RGB(0,255,0)); // Reset Alarm -> color green
}

void FuelMFD::Update (HDC hDC)
{
	int maxtanks;
	char Buffer[100];
	
	v = oapiGetFocusInterface();

	maxtanks=v->GetPropellantCount();
	
	maxpage = maxtanks/6; // max 6 tanks per page
	if (maxtanks%6==0) maxpage--;
	
	SetTextColor(hDC, RGB(0, 255, 0)); // Display Color: Green
	Title (hDC, "Fuel Reserve Display");
	
	sprintf_s(Buffer,"%i/%i",page+1,maxpage+1);
	TextOut(hDC, 250, 0, Buffer, strlen(Buffer));

	CurrentLine = 2*LINE; //Beginning of the display
	
	for (int i=1; (i<=6) && (i+page*6<=maxtanks) ; i++)
	{
		FuelBar(hDC, i+page*6 , CurrentLine);
		CurrentLine += LINE*4;
	}
}

void FuelMFD::StoreStatus (void) const
{
	g_data.page = page;
	g_data.disp_mode = disp_mode;
}

void FuelMFD::RecallStatus (void)
{
	page = g_data.page;
	disp_mode = g_data.disp_mode;
}

FuelMFD::g_Data FuelMFD::g_data = {0,0};
