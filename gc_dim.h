#include <dis.hxx>
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#import <msxml6.dll> rename_namespace(_T("MSXML"))

#pragma comment(lib, "User32.lib") // _bstr_t is in here.

//data strcuture for handling (and sending via DIM) many values at once.
typedef struct{float farr[99];}COMPLEXDATA;

// functions:
void setupParameters();
int getTargetDir(int argc, TCHAR *argv[]);
std::string from_variant(VARIANT&);
int GetStreamNumber(std::string);
std::string GetIjnectionTime(std::string);
int GetNumberOfPeaks(std::string);
COMPLEXDATA GetPeakData(std::string, _bstr_t);
std::string CutTheCrap(std::string);

//std::string from_variant(VARIANT& vt);
//int GetStreamNumber(std::string LatestFolderName);
//std::string GetIjnectionTime(std::string LatestFolderName);
//int GetNumberOfPeaks(std::string LatestFolderName);
//COMPLEXDATA GetPeakData(std::string LatestFolderName, _bstr_t ElementName);
//std::string CutTheCrap(std::string line);


// stuff that needs to be available in functions:
float MethodRuntime;

// file readout:
std::string LatestFolderName;
std::string targetDirectory;

//initialize some return values
int NumberOfPeaks;
int StreamNumber;

// DIM par starts here
// create variables for DIM Services:
//TPC
float CO2_TPC, Argon_TPC, N2_TPC, CO2_TPC_cal, Argon_TPC_cal, N2_TPC_cal, calTPC;
float N22_TPC, N22_TPC_cal; // second N2 peak in TPC chromatogram

//TRD
float CO2_TRD, Xe_TRD, N2_TRD, Xe_TRD_cal, CO2_TRD_cal, N2_TRD_cal, calTRD, calN2;
// create Retention Time values:
float RTCO2_min, RTCO2_max, RTCO2_c1only_min, RTCO2_c1only_max, RTArgon_min, RTArgon_max;

//storage for information from XML files
COMPLEXDATA PeakAreaPercentData;
COMPLEXDATA PeakAreaData;
COMPLEXDATA RTData;

// create DIM-Services
//tpc
//DimService Stream1("Stream1	_PeakAreas","F:7",(void *)&PeakAreaPercentData, sizeof(PeakAreaPercentData));
//DimService tpcCO2Content("ALICE_GC.Actual.tpcCO2Content",CO2_TPC_cal); 
//DimService tpcArgonContent("ALICE_GC.Actual.tpcArgonContent",Argon_TPC_cal); 
//DimService tpcN2Content("ALICE_GC.Actual.tpcN2Content",N22_TPC_cal); 
DimService * testDim;
DimService * tpcCO2Content=0;
DimService * tpcArgonContent=0; 
DimService * tpcN2Content=0;
//trd
//DimService trdCO2Content("ALICE_GC.Actual.trdCO2Content",CO2_TRD_cal); 
//DimService trdXeContent("ALICE_GC.Actual.trdXeContent",Xe_TRD_cal); 
//DimService trdN2Content("ALICE_GC.Actual.trdN2Content",N2_TRD_cal); 
DimService * trdCO2Content=0;
DimService * trdXeContent=0;
DimService * trdN2Content=0;


// --- GC alarms, alarm stuff:
// Make AliveCounter service for TPC and TRD
int AliveCounterTRD, AliveCounterTPC;
DimService dimsAliveCounterTRD("ALICE_GC.FSM.AliveCounterTRD",AliveCounterTRD);
DimService dimsAliveCounterTPC("ALICE_GC.FSM.AliveCounterTPC",AliveCounterTPC);
// ---







std::string InjectionTimeAndDate_store1;

//log file
std::ofstream LogFile, LogResultsTPC, LogResultsTRD;

std::string FolderNameBuffer1;
std::string FolderNameBuffer2;

bool acquiringNow, FoundXmlFile, forceExit, trd1, trd2, tpc1, tpc2, tpcOutlierBool=FALSE;
int roundNumber, tpcOutlier;
float roundDuration;
std::string PathToACQUIRINGTXT;
std::string lines[4];

