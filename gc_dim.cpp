#include "dim.h"

using namespace std;

void setupParameters()
{
	// change parameters here as needed

	//Calibration values:
	calN2=1.0733399f;
	calTPC=1.0167405666f;
	calTRD=0.7029554088f;

	//for TPC average:
	averageTPC=FALSE; // toggle to compute average
	haveTPCdata=FALSE;

	//TRD
	CO2_TRD=0, Xe_TRD=0, N2_TRD=0;

	// create Retention Time values:
	RTCO2_min = 3.8f; //the "f" is to explicitly tell the compiler this is a float. I get "truncation from 'double' to 'float'" warning otherwise !?
	RTCO2_max = 4.1f;
	RTCO2_c1only_min = 3.7f;	//for TRD analysis (using only column 1:)
	RTCO2_c1only_max = 4.2f;	//for TRD analysis (using only column 1:)
	RTArgon_min = 5.55f;
	RTArgon_max = 6.25f;



	MethodRuntime=960000; //runtime of GC method + 1 minute in ms.
	roundDuration = 5000;
	PathToACQUIRINGTXT =targetDirectory+"\\ACQUIRING.TXT";

	//alarm stuff:
	AliveCounterTRD=0, AliveCounterTPC=0;


	//log file
	LogFile.open ("DIM_logfile.log", ios::out | ios::app);

	// set DIM_DNS_NODE and start dim server:
	DimServer::setDnsNode("alitpcdimdns"); //tpc dim dns node: 'alitpcdimdns' // or 'localhost' for local dns
	DimServer::start("ALICE_GC");
	LogFile << "\n \n \n ### DIM SERVER STARTED in "<< targetDirectory << " ### \n \n";
}


int _tmain(int argc, TCHAR *argv[])
{
	if(getTargetDir(argc, argv) != 0) return -1;

	setupParameters();

	// for tpc average:
	vector<float> vecTPC_Argon, vecTPC_CO2, vecTPC_N2;

	while(1)
	{
		// continuously look for new measurement-files. Update Dim Services with content of said files. Do this eternally.
		LogFile<< "\n \n ### Begin of Loop ###" <<endl;

		// --- try to open file "ACQUIRING.TXT" that is produced by Agilent ChemStation during measurement and contains the folderpath for the next measurement.
		acquiringNow = FALSE;
		do
		{
			ifstream CheckIfAcquiring(PathToACQUIRINGTXT);	
			acquiringNow = CheckIfAcquiring.good();
			CheckIfAcquiring.close();
			Sleep(roundDuration);
		}

		while (!acquiringNow); //this loop does not need a further exit. If in doubt or if something bad happened: Continuously look if a new measurement is taking place.
		// ---

		//now get folderpath of current measurement from that file.
		ifstream thatfile(PathToACQUIRINGTXT);
		for (int i=0;i<4;i++) thatfile >> lines[i]; //read content of that file line by line.
		LatestFolderName = targetDirectory+"\\"+CutTheCrap(lines[2]); // CutTheCrap() returns the Foldername in the corresponding line of that file.
		thatfile.close();
		LogFile << "New Folder: "<<LatestFolderName <<endl;

		// try to open Result.xml that is created at the end of the measurement.
		FoundXmlFile=FALSE;
		forceExit=FALSE;
		roundNumber = 0;

		LogFile << "checking for Result.xml in " << LatestFolderName+"\\Result.xml" <<endl;
		do
		{
			ifstream checkXML(LatestFolderName+"\\Result.xml");	
			FoundXmlFile = checkXML.good();
			checkXML.close();
			Sleep(roundDuration);
			roundNumber++;
			// this loop needs a second exit, if the XML file in this very folder is not created for any reason. So:
			if(roundNumber>(MethodRuntime/roundDuration-roundNumber))  // is true, after the time MethodRuntime has passed
			{
				LogFile << "no Result.xml found in " << LatestFolderName <<". Break do..while.. Loop" <<endl;
				break;
			}
		}
		while(!FoundXmlFile);
		LogFile << "roundNumber for checking if Result.xml exists: " << roundNumber <<endl;
		Sleep(roundDuration);
		//Probe if there is already vaild information in the file.
		roundNumber=0;
		do
		{
			Sleep(roundDuration);
			StreamNumber = GetStreamNumber(LatestFolderName);
			roundNumber++;
			// this loop needs a second exit, too. See above. So:
			if(roundNumber>(MethodRuntime/roundDuration-roundNumber)) // is true, after the time MethodRuntime has passed
			{
				LogFile << "Result.xml in " << LatestFolderName <<" corrupt?! Break do..while.. Loop" <<endl;
				break;
			}
		}
		while (	StreamNumber == -1); // GetStreamNumber returns -1 when there is no file that can be opened. If forceExit=True, there already was a very long wait, so do this only once!

		if(StreamNumber != -1)
		{
			InjectionTimeAndDate_store1 = GetIjnectionTime(LatestFolderName);
			NumberOfPeaks = GetNumberOfPeaks(LatestFolderName);
			PeakAreaPercentData = GetPeakData(LatestFolderName,"AreaPercent");
			PeakAreaData = GetPeakData(LatestFolderName,"Area");
			RTData = GetPeakData(LatestFolderName,"RetTime");
			//initialize output to zero
			Argon_TPC=0, CO2_TPC=0, N2_TPC=0, Xe_TRD=0, CO2_TRD=0, N2_TRD=0, N2_TPC=0;
			trd1=FALSE, trd2=FALSE, tpc1=FALSE, tpc2=FALSE;
			bool have_seen_TPC_N2_peak = FALSE;
			for (int i=0;i<NumberOfPeaks;i++)
			{
				if ( StreamNumber == 1 ) // StreamNumber 1 is TPC gas
				{		
					// decide, based on PeakAreaPercent, which Peak is which and update services
					if ( PeakAreaPercentData.farr[i] > 70. && RTCO2_max < RTData.farr[i] ) Argon_TPC = PeakAreaData.farr[i], tpc2=TRUE;
					if ( PeakAreaPercentData.farr[i] > 6. && RTCO2_min < RTData.farr[i] && RTCO2_max > RTData.farr[i] ) CO2_TPC = PeakAreaData.farr[i], tpc1=TRUE;
					// for TPC N2, there currently are two very small peaks where N2 is supposed to be. If there are two peaks, take the larger one.
					if ( 11.6 < RTData.farr[i] && RTData.farr[i] <12.3 && !have_seen_TPC_N2_peak ) N2_TPC = PeakAreaData.farr[i], have_seen_TPC_N2_peak=TRUE;
					if ( 11.6 < RTData.farr[i] && RTData.farr[i] <12.3 && have_seen_TPC_N2_peak && N2_TPC<PeakAreaData.farr[i] ) N2_TPC = PeakAreaData.farr[i];
				}
				if ( StreamNumber == 5 ) // TRD Pur OUT
				{
					tpcOutlierBool=TRUE;
					if(NumberOfPeaks==3 || NumberOfPeaks==2)
					{
						continue; // if exactly three or two peaks in TRD gas: Identify peaks by order. See below. This is certain peak identification for the GC method using only column 1.
					}

					else
					{
						if ( 4.2 < RTData.farr[i] && 4.97 > RTData.farr[i] ) Xe_TRD = PeakAreaData.farr[i],trd2=TRUE;
						if ( 3.6 < RTData.farr[i] && 4.2 > RTData.farr[i] ) CO2_TRD = PeakAreaData.farr[i],trd1=TRUE;
						if ( 3.35 < RTData.farr[i] && RTData.farr[i] <3.65 ) N2_TRD = PeakAreaData.farr[i];

					}
				}
			}

		}


		// ##########
		// Solution TPC outlier problem: Calculate mean
		if(StreamNumber==5 && haveTPCdata && vecTPC_Argon.size()>0 && averageTPC) //that is: if all TPC methods of a sequence are done
		{
			CO2_TPC_cal=0; Argon_TPC_cal=0; N2_TPC_cal=0;
			for(size_t i=0; i < vecTPC_Argon.size(); i++)
			{
				Argon_TPC_cal+=vecTPC_Argon[i];
				CO2_TPC_cal+=vecTPC_CO2[i];
				N2_TPC_cal+=vecTPC_N2[i];
			}
			Argon_TPC_cal=Argon_TPC_cal/vecTPC_Argon.size();
			LogFile << "##### RESULT ###### Argon_TPC_cal = " << Argon_TPC_cal<< " with vecTPC_Argon.size() = " << vecTPC_Argon.size() <<endl;
			CO2_TPC_cal=CO2_TPC_cal/vecTPC_Argon.size();
			LogFile << "##### RESULT ###### CO2_TPC_cal = " << CO2_TPC_cal<< " with vecTPC_CO2.size() = " << vecTPC_CO2.size() <<endl;
			N2_TPC_cal=N2_TPC_cal/vecTPC_Argon.size();
			tpcArgonContent.updateService();
			tpcCO2Content.updateService();
			tpcN2Content.updateService();
			vecTPC_Argon.resize(0);
			vecTPC_CO2.resize(0);
			vecTPC_N2.resize(0);
			haveTPCdata=FALSE;
		}
		// ##########

		//TRD: easy peak identification for measurement with GC using only column 1:
		if(StreamNumber==5 && NumberOfPeaks==3)
		{
			N2_TRD = PeakAreaData.farr[0];
			CO2_TRD = PeakAreaData.farr[1],trd1=TRUE;
			Xe_TRD = PeakAreaData.farr[2],trd2=TRUE;
		}
		else if(StreamNumber==5 && NumberOfPeaks==2)
		{
			CO2_TRD = PeakAreaData.farr[0],trd1=TRUE;
			Xe_TRD = PeakAreaData.farr[1],trd2=TRUE;
		}
		else if(StreamNumber==5)
		{
			LogFile << ">>>>>>>>>>>>>>>>> TRD measurement shows unusual number of peaks. CHECK!!!" << endl;
			cout << ">>>>>>>>>>>>>>>>> TRD measurement shows unusual number of peaks. CHECK!!!" << endl;
		}
		//TRD: calculate calibrated values + update DIM services
		if(trd1&&trd2)
		{
			AliveCounterTRD++;
			dimsAliveCounterTRD.updateService();

			CO2_TRD_cal=0; Xe_TRD_cal=0; N2_TRD_cal=0;
			CO2_TRD_cal=100*CO2_TRD/(CO2_TRD+calTRD*Xe_TRD+calN2*N2_TRD);
			Xe_TRD_cal=100*calTRD*Xe_TRD/(CO2_TRD+calTRD*Xe_TRD+calN2*N2_TRD);
			N2_TRD_cal=100*calN2*N2_TRD/(CO2_TRD+calTRD*Xe_TRD+calN2*N2_TRD);
			trdXeContent.updateService();
			trdCO2Content.updateService();
			trdN2Content.updateService();
			tpcOutlier=0,haveTPCdata=FALSE; //workaround TPC outlier problem
		}

		if(tpc1&&tpc2)
		{
			AliveCounterTPC++;
			dimsAliveCounterTPC.updateService();

			CO2_TPC_cal=0; Argon_TPC_cal=0; N2_TPC_cal=0;
			CO2_TPC_cal=100*CO2_TPC/(CO2_TPC+calTPC*Argon_TPC+calN2*N2_TPC); // uses the same calN2 as TRD
			Argon_TPC_cal=100*calTPC*Argon_TPC/(CO2_TPC+calTPC*Argon_TPC+calN2*N2_TPC);
			N2_TPC_cal=100*calN2*N2_TPC/(CO2_TPC+calTPC*Argon_TPC+calN2*N2_TPC);
			tpcOutlier++; //don't send first two TPC measurements to DCS (Outliers, not understood)
			LogFile <<"tpcOutlier = "<<tpcOutlier<<endl;
			LogFile << "gc results: CO2 = " << CO2_TPC_cal<< "Ar = " << Argon_TPC_cal << "N2 = " << N2_TPC_cal <<endl;
			if(tpcOutlier>2 || tpcOutlierBool==FALSE || !averageTPC) // second opton if DIM server is restared in the middle of a run
			{
				if(averageTPC)
				{
					vecTPC_Argon.push_back(Argon_TPC_cal);
					vecTPC_CO2.push_back(CO2_TPC_cal);
					vecTPC_N2.push_back(N2_TPC_cal);
					haveTPCdata=TRUE;
				}
				else
				{
					tpcArgonContent.updateService();
					tpcCO2Content.updateService();
					tpcN2Content.updateService();
				}
			}
		}

		LogFile << "vecTPC_Argon.size() = " << vecTPC_Argon.size() <<endl;
		if(vecTPC_Argon.size()>0)
		{
			for (size_t i = 0; i<vecTPC_Argon.size(); i++)
			{
				LogFile<< "i = " << i << ", vecTPC_Argon[i] = " << vecTPC_Argon[i] <<endl;
			}
		}

		LogFile << "end of cycle" <<endl;
		Sleep(5000); // just in case. Ein bisschen Schlaf muss sein
	}
}


int getTargetDir(int argc, TCHAR *argv[])
{
	// check for input when starting the DIM server from the command line and return the targetDirectory as string
	size_t length_of_arg;
	DWORD dwError=0;
	// If the directory is not specified as a command-line argument,
	// print usage.
	if(argc != 2)
	{
		_tprintf(TEXT("\n Usage: \n %s <directory name>\n \n E.g. type: \n %s . \n in order to run %s in its root folder. \n\n\n"), argv[0], argv[0], argv[0]);
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
	WideCharToMultiByte(CP_ACP,0,argv[1],-1, ch,260,&DefChar, NULL);

	//set the targetDirectory string:
	targetDirectory = string(ch);
	return 0;
}


string from_variant(VARIANT& vt)
	//conversion from _bstr_t to std::string
{
	_bstr_t bs(vt);
	return std::string(static_cast<const char*>(bs));
}

int GetStreamNumber(string LatestFolderName)
	// returns the Name of the Stream (Stream 1, 2, 3 ... 10) that was used by the Gaschromatograph's Stream-Selector,
	// by reading "Location" from "Result.xml"
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
				// printf("Unable to load Result.xml in function GetStreamNumber()\n");
			}
			else
			{
				xmlDoc->setProperty("SelectionLanguage", "XPath");		
				// Find out from which Stream of the Chromatograph's Streamselector the XML file was produced.
				MSXML::IXMLDOMNodeListPtr SampleName = xmlDoc->getElementsByTagName("Location"); //Pointer to List of Elements with specified name
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
	// by reading "InjectionDateTime" from "Result.xml"
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
	for (size_t i=0; i<line.length(); i++) // iterate over all characters in 'line'
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





