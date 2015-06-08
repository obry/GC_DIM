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

#import <msxml6.dll> rename_namespace(_T("MSXML"))

#pragma comment(lib, "User32.lib") // _bstr_t is in here.

using namespace std;

typedef struct{
  float farr[21]; // this is more than needed, but right Number of Peaks is returned by GetNumberOfPeaks()
  // Open question: How to deal with this in PVSS? There must be better ideas.
  // ==> TPC prefers single service for each datapointelement anyway.
}COMPLEXDATA;

string from_variant(VARIANT& vt)
  //conversion from _bstr_t to std::string
{
  _bstr_t bs(vt);
  return std::string(static_cast<const char*>(bs));
}

int GetStreamNumber(string LatestFolderName)
  // returns the Name of the Stream (Stream 1, 2, 3 ... 10) that was used by the Gaschromatograph's Stream-Selector,
  // by reading "SampleName" from "Result.xml"
{
  HRESULT hr = CoInitialize(NULL); 
  if (SUCCEEDED(hr))
  {
    try
    {
      MSXML::IXMLDOMDocument2Ptr xmlDoc;
      hr = xmlDoc.CreateInstance(__uuidof(MSXML::DOMDocument60), NULL, CLSCTX_INPROC_SERVER);
      string tempLatestFolderName = LatestFolderName+"\\Result.xml";
      wstring widestr = wstring(tempLatestFolderName.begin(), tempLatestFolderName.end());
      const wchar_t * widecstr = widestr.c_str();
      if (xmlDoc->load(widecstr) != VARIANT_TRUE)
      {
	printf("Unable to load Result.xml in function GetStreamNumber()\n");
      }
      else
      {
	xmlDoc->setProperty("SelectionLanguage", "XPath");		
	// Find out from which Stream of the Chromatograph's Streamselector the XML file was produced.
	MSXML::IXMLDOMNodeListPtr SampleName = xmlDoc->getElementsByTagName("SampleName"); //Pointer to List of Elements with specified name
	MSXML::IXMLDOMElementPtr spElementTemp = (MSXML::IXMLDOMElementPtr) SampleName->item[0];
	MSXML::IXMLDOMTextPtr spText = spElementTemp->firstChild;
	// spText->nodeValu is e.g. "Stream 1". Extract the integer Number of the Stream:
	string StreamName = from_variant(spText->nodeValue);
	int StreamNumber = 0;
	string firstString= "";
	stringstream ss(StreamName);
	ss  >> firstString >> StreamNumber;
	return StreamNumber;
      }
    }
    catch (_com_error &e)
    {
      printf("ERROR: %ws\n", e.ErrorMessage());
    }
    CoUninitialize();
  }
  return -1;
}



string GetIjnectionTime(string LatestFolderName)
  // returns the InjectionDateTime
  // by reading "SampleName" from "Result.xml"
{
  HRESULT hr = CoInitialize(NULL); 
  if (SUCCEEDED(hr))
  {
    try
    {
      MSXML::IXMLDOMDocument2Ptr xmlDoc;
      hr = xmlDoc.CreateInstance(__uuidof(MSXML::DOMDocument60), NULL, CLSCTX_INPROC_SERVER);
      string tempLatestFolderName = LatestFolderName+"/Result.xml";
      wstring widestr = wstring(tempLatestFolderName.begin(), tempLatestFolderName.end());
      const wchar_t * widecstr = widestr.c_str();
      if (xmlDoc->load(widecstr) != VARIANT_TRUE)
      {
	printf("Unable to load Result.xml in function GetIjnectionTime()\n");
      }
      else
      {
	xmlDoc->setProperty("SelectionLanguage", "XPath");
	// Find out from which Stream of the Chromatograph's Streamselector the XML file was produced.
	MSXML::IXMLDOMNodeListPtr SampleName = xmlDoc->getElementsByTagName("InjectionDateTime"); //Pointer to List of Elements with specified name
	MSXML::IXMLDOMElementPtr spElementTemp = (MSXML::IXMLDOMElementPtr) SampleName->item[0];
	MSXML::IXMLDOMTextPtr spText = spElementTemp->firstChild;
	// spText->nodeValu is e.g. "Stream 1". Extract the integer Number of the Stream:
	string InjectionTime = from_variant(spText->nodeValue);
	return InjectionTime;
      }
    }
    catch (_com_error &e)
    {
      printf("ERROR: %ws\n", e.ErrorMessage());
    }
    CoUninitialize();
  }
  return "-1";
}
int GetNumberOfPeaks(string LatestFolderName)
  // returns the Number of Peaks that was found in "Result.xml"
{
  HRESULT hr = CoInitialize(NULL); 
  if (SUCCEEDED(hr))
  {
    try
    {
      MSXML::IXMLDOMDocument2Ptr xmlDoc;
      hr = xmlDoc.CreateInstance(__uuidof(MSXML::DOMDocument60), NULL, CLSCTX_INPROC_SERVER);
      string tempLatestFolderName = LatestFolderName+"/Result.xml";
      wstring widestr = wstring(tempLatestFolderName.begin(), tempLatestFolderName.end());
      const wchar_t * widecstr = widestr.c_str();
      if (xmlDoc->load(widecstr) != VARIANT_TRUE)
      {
	printf("Unable to load Result.xml in function GetNumberOfPeaks()\n");
      }
      else
      {
	xmlDoc->setProperty("SelectionLanguage", "XPath");
	MSXML::IXMLDOMNodeListPtr PeakAreaPercent = xmlDoc->getElementsByTagName("AreaPercent"); //Pointer to List of Elements with specified name
	int NumberOfPeaks = PeakAreaPercent->length; //Number of Peaks should be equal to the number of AreaPercent entrys in XML File.
	return NumberOfPeaks;
      }
    }
    catch (_com_error &e)
    {
      printf("ERROR: %ws\n", e.ErrorMessage());
    }
    CoUninitialize();
  }
  return -1;
}

COMPLEXDATA GetPeakData(string LatestFolderName, _bstr_t ElementName)
  //return Info with label 'ElementName' in "Result.xml" for all Peaks
{
  COMPLEXDATA Peaks;
  Peaks.farr[0]=-1; //fill with dummy data to be able to return something in any case.
  HRESULT hr = CoInitialize(NULL); 
  if (SUCCEEDED(hr))
  {
    try
    {
      MSXML::IXMLDOMDocument2Ptr xmlDoc;
      hr = xmlDoc.CreateInstance(__uuidof(MSXML::DOMDocument60), NULL, CLSCTX_INPROC_SERVER);
      // TODO: if (FAILED(hr))...

      // Convert 'string LatestFolderName' to something that xmlDoc->load() is able to read.
      // I know that 'ffd.cFileName' where I start with this is already something like wchar_t
      // But it works now, and I don't know yet how to combine two wchar_t like I need below etc.
      // Who ever reads this. Forgive me :))
      string tempLatestFolderName = LatestFolderName+"/Result.xml";
      wstring widestr = wstring(tempLatestFolderName.begin(), tempLatestFolderName.end());
      const wchar_t * widecstr = widestr.c_str();
      if (xmlDoc->load(widecstr) != VARIANT_TRUE)
      {
	printf("Unable to load Result.xml in GetPeakData()\n");
      }
      else
      {
	//printf("XML was successfully loaded\n");

	xmlDoc->setProperty("SelectionLanguage", "XPath");

	MSXML::IXMLDOMNodeListPtr PeakAreaPercent = xmlDoc->getElementsByTagName(ElementName); //Pointer to List of Elements with specified name
	int NumberOfPeaks = PeakAreaPercent->length; //Number of Peaks should be equal to the number of AreaPercent entrys in XML File.
	float * AreaPercentValue =new float[NumberOfPeaks];

	MSXML::IXMLDOMElementPtr spElementTemp; //need this to access values in elements
	for (int i = 0; i < PeakAreaPercent->length; i++) 
	{
	  spElementTemp = (MSXML::IXMLDOMElementPtr) PeakAreaPercent->item[i];
	  // Get the text node with the element content. If present it will be the first child.
	  MSXML::IXMLDOMTextPtr spText = spElementTemp->firstChild;
	  AreaPercentValue[i]=(float)wcstod(_bstr_t(spText->nodeValue),NULL); //using directly _variant_t Extractor 'operator float()' will produce strange numbers. wcstof() is undefined according to compiler.
	  Peaks.farr[i] = AreaPercentValue[i];
	}
	return Peaks;
      }
    }
    catch (_com_error &e)
    {
      printf("ERROR: %ws\n", e.ErrorMessage());
    }
    CoUninitialize();
  }
  return Peaks;
}

string CutTheCrap(string line)
{
  string word; // the resulting word
  bool PathStarted =FALSE;
  for (int i=0; i<line.length(); i++) // iterate over all characters in 'line'
  {
    if (line[i] == ':') // if this character is a colon, the path starts.
    {
      if (!PathStarted) i++; //skip this very colon.
      PathStarted = TRUE;
    }
    if (PathStarted && line[i] != NULL) word += line[i]; //append every not NULL charakter to the word until line.lenth() is reached.
  }
  return word;
}

int _tmain(int argc, TCHAR *argv[])
{
  float MethodRuntime=86000; //runtime of GC method in ms.
  LARGE_INTEGER filesize;
  TCHAR szDir[MAX_PATH];
  size_t length_of_arg;
  DWORD dwError=0;
  // If the directory is not specified as a command-line argument,
  // print usage.
  if(argc != 2)
  {
    _tprintf(TEXT("\nUsage: %s <directory name>\n"), argv[0]);
    return (-1);
  }
  // Check that the input path plus 3 is not longer than MAX_PATH.
  // Three characters are for the "\*" plus NULL appended below.
  StringCchLength(argv[1], MAX_PATH, &length_of_arg);

  if (length_of_arg > (MAX_PATH - 3))
  {
    _tprintf(TEXT("\nDirectory path is too long.\n"));
    return (-1);
  }

  _tprintf(TEXT("\nTarget directory is %s\n\n"), argv[1]);

  // Convert userinput of Target Directory ( argv[1] ) to string:
  char ch[260];
  char DefChar = ' ';
  string LatestFolderName;
  string targetDirectory;
  WideCharToMultiByte(CP_ACP,0,argv[1],-1, ch,260,&DefChar, NULL);
  targetDirectory = string(ch);

  //initialize some return values
  int NumberOfPeaks;
  int StreamNumber;

  // DIM par starts here
  // create variables for DIM Services of TPC:
  float CO2_TPC=0;
  float Argon_TPC=0;
  float N2_TPC=0;
  float Water_TPC=0;
  // create Retention Time values:
  float RTCO2_min = 2.9f; //the "f" is to explicitly tell the compiler this is a float. I get "truncation from 'double' to 'float'" warning otherwise !?
  float RTCO2_max = 3.3f;
  float RTArgon_min = 5.8f;
  float RTArgon_max = 6.25f;
  // create DIM-Services
  COMPLEXDATA PeakAreaPercentData;
  COMPLEXDATA PeakAreaData;
  COMPLEXDATA RTData;
  DimService Stream1("Stream1_PeakAreas","F:7",(void *)&PeakAreaPercentData, sizeof(PeakAreaPercentData));
  DimService tpcCO2Content("ALICE_GC.Actual.tpcCO2Content",CO2_TPC); 
  DimService tpcArgonContent("ALICE_GC.Actual.tpcArgonContent",Argon_TPC); 
  DimService tpcN2Content("ALICE_GC.Actual.tpcN2Content",N2_TPC); 
  DimService tpcWaterContent("ALICE_GC.Actual.tpcWaterContent",Water_TPC);
  DimServer::setDnsNode("localhost"); //tpc dim dns node: 'alitpcdimdns'
  DimServer::start("ALICE_GC");

  string InjectionTimeAndDate_store1;
  string InjectionTimeAndDate_store2 = "this must not be empty" ;

  //produce log file
  ofstream LogFile; //write stuff to file
  int SleepTime = 30000;
  string FolderNameBuffer1;
  string FolderNameBuffer2;
  LogFile.open ("DIM_logfile.log", ios::out | ios::app);
  LogFile << "\n \n \n ### DIM SERVER STARTED in "<< targetDirectory << " ### \n \n";

  bool acquiringNow;
  bool FoundXmlFile;
  bool forceExit;
  int roundNumber;
  float roundDuration = 5000;
  string PathToACQUIRINGTXT =targetDirectory+"\\ACQUIRING.TXT";
  string lines[4];


  while(1)
  {
    // continuously look for new measurement-files. Update Dim Services with content of said files. Do this eternally.
    LogFile<< "\n \n ### Begin of Loop ###" <<endl;

    // try to open file "ACQUIRING.TXT" that is produced by Agilent ChemStation during measurement and contains the folderpath for the next measurement.
    acquiringNow = FALSE;
    do
    {
      ifstream CheckIfAcquiring(PathToACQUIRINGTXT);	
      acquiringNow = CheckIfAcquiring.good();
      CheckIfAcquiring.close();
      Sleep(5000);
    }
    while (!acquiringNow); //this loop does not need a further exit. If in doubt or if something bad happened: Continuously look if a new measurement is taking place.


    //now get folderpath of current measurement from that file.
    ifstream thatfile(PathToACQUIRINGTXT);
    for (int i=0;i<4;i++) thatfile >> lines[i]; //read content of that file line by line.
    LatestFolderName = targetDirectory+"\\"+CutTheCrap(lines[2]); // CutTheCrap() will rwturn the Foldername in the corresponding line of that file.
    thatfile.close();
    LogFile << "New Folder: "<<LatestFolderName <<endl;

    // try to open Result.xml that is created at the end of the measurement.
    FoundXmlFile=FALSE;
    forceExit=FALSE;
    roundNumber = 0;
    do
    {
      ifstream checkXML(LatestFolderName+"/Result.xml");	
      FoundXmlFile = checkXML.good();
      checkXML.close();
      Sleep(roundDuration);
      // this loop needs a second exit, if the XML file in this very folder is not created for any reason.
      // A GC measurement is atm 8.6 minutes long.
      roundNumber++;
      if(roundNumber>(MethodRuntime+5000)/roundDuration) forceExit=TRUE;
    }
    while(!FoundXmlFile || forceExit);
    LogFile << "roundNumber for checking if Result.xml exists: " << roundNumber <<endl;
    //Probe with GetStreamNumber() if there is already vaild information in the file.
    roundNumber=0;
    do
    {
      Sleep(3000);
      StreamNumber = GetStreamNumber(LatestFolderName);
      // this loop needs a second exit, too. See above.s
      roundNumber++;
      if(roundNumber>(MethodRuntime+6000)/3000) forceExit=TRUE;
    }
    while (	StreamNumber == -1 || forceExit ); // GetStreamNumber returns -1 when there is no file that can be opened. If forceExit=True, there already was a very long wait, so do this only once!

    LogFile << "Some Result.XML was read and GetStreamNumber() returned : " << StreamNumber<<endl;
    Sleep(1000);
    InjectionTimeAndDate_store1 = GetIjnectionTime(LatestFolderName);
    LogFile << "Injection Time = " << InjectionTimeAndDate_store1 <<endl;
    NumberOfPeaks = GetNumberOfPeaks(LatestFolderName);
    PeakAreaPercentData = GetPeakData(LatestFolderName,"AreaPercent");
    PeakAreaData = GetPeakData(LatestFolderName,"Area");
    RTData = GetPeakData(LatestFolderName,"RetTime");
    cout << "Results.xml with Streamnumber 1 found in " << LatestFolderName <<endl;
    for (int i=0;i<NumberOfPeaks;i++)
    {
      if ( StreamNumber == 1 ) // StreamNumber 1 is TPC GAS ==> update DIM Services for TPC
      {
	// decide based on Retention Time which peak is which and update DIM services accordingly:
	if ( RTCO2_min<RTData.farr[i] && RTData.farr[i]<RTCO2_max ) CO2_TPC = PeakAreaPercentData.farr[i], LogFile<< "CO2 candidate: " << CO2_TPC <<endl , tpcCO2Content.updateService();
	if ( RTArgon_min<RTData.farr[i] && RTData.farr[i]<RTArgon_max ) Argon_TPC = PeakAreaPercentData.farr[i], LogFile<< "Argon candidate: " << Argon_TPC <<endl , tpcArgonContent.updateService();
      }
      if (PeakAreaPercentData.farr[0] >= 0 ) Stream1.updateService(); //only update Stream1 when it contains real data (that is no dummy data "-1").
    }
    LogFile << "end of cycle" <<endl;
    Sleep(5000); //ein bisschen Schlaf muss sein.
  }
}
