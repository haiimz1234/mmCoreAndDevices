#include "PrizmatixMain.h"
#ifdef WIN32
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
#endif
#include "FixSnprintf.h"
/*******************
* #include "ThorlabsElliptecSlider.h"
Prizmatix-Devices
	Prizmatix-LED-CTRL: LEDs Control
	Prizmatix-Pulser: Optogenetics Pulser

Start/Stop

**/
//  output:  $(OutDir)$(TargetName)$(TargetExt)
//const char* g_DeviceNameHub = "LED-CTRL";
const char* g_DeviceOneLED = "Prizmatix Ctrl";
const char* g_DevicePulser = "Prizmatix Pulser";
 
const char* g_On = "On";
const char* g_Off = "Off";

//MMThreadLock PrizmatixHub::lock_;
MMThreadLock PrizmatixPulser::lock_;
MMThreadLock PrizmatixLED::lock_;
 
///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
  // RegisterDevice(g_DeviceNameHub, MM::HubDevice, "LEDs Control (required)");
   RegisterDevice(g_DevicePulser, MM::GenericDevice, "Prizmatix Pulser");
   RegisterDevice(g_DeviceOneLED, MM::GenericDevice, "Prizmatix Ctrl");

}
   
  
MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   
     if (strcmp(deviceName, g_DeviceOneLED) == 0)
   {
      return new PrizmatixLED(1,(char *)deviceName);
   }
   else if (strcmp(deviceName, g_DevicePulser) == 0)
   {
	   return new PrizmatixPulser(1, (char*)deviceName);
   }
   
  
   return 0;
}


MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

/***************************************************************************
///////////////////////////////////////////////////////////////////////////////
// PrizmatixHub implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//

PrizmatixHub::PrizmatixHub() :
   initialized_ (false),
   switchState_ (0),
   shutterState_ (0),
	nmLeds(0),
	version_(0)
{
   portAvailable_ = false;
   invertedLogic_ = false;
   timedOutputActive_ = false;

   InitializeDefaultErrorMessages();

   SetErrorText(ERR_PORT_OPEN_FAILED, "Failed opening Prizmatix USB device");
   SetErrorText(ERR_BOARD_NOT_FOUND, "Did not find an Prizmatix board .  Is the Prizmatix board connected to this serial port?");
   SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The Prizmatix Hub device is needed to create this device");
  
  // CPropertyAction* pAct = new CPropertyAction(this, &PrizmatixHub::OnPort);
  // CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
  
}

PrizmatixHub::~PrizmatixHub()
{
   Shutdown();
}

void PrizmatixHub::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_DeviceNameHub);
}

bool PrizmatixHub::Busy()
{
   return false;
}


int PrizmatixHub::GetControllerVersion(int& version)
{
   int ret = DEVICE_OK;
   const char* command="V:0\n";
  
// SendSerialCommand
   ret = WriteToComPort(port_.c_str(), (const unsigned char*) command, strlen((char *)command));
   if (ret != DEVICE_OK)
      return ret;
   
   std::string answer;
   ret = GetSerialAnswer(port_.c_str(), "\r\n", answer);
   if (ret != DEVICE_OK)
      return ret ;
   if (answer.data()[0] == 'P') return GetControllerVersionPulser(version, answer);
   version = -1;
   int Mik=answer.find_last_of('_');
   nmLeds=atoi(answer.data()+Mik+1);   
   if (nmLeds == 0) return 1;
   return ret;

}

int PrizmatixHub::GetControllerVersionPulser(int& version, std::string answer)
{
	version = (answer.data()[1] == 'P') ? 1 : 0;	 
	return DEVICE_OK;
}

bool PrizmatixHub::SupportsDeviceDetection(void)
{
   return true;
}

MM::DeviceDetectionStatus PrizmatixHub::DetectDevice(void)
{
   if (initialized_)
      return MM::CanCommunicate;

   // all conditions must be satisfied...
   MM::DeviceDetectionStatus result = MM::Misconfigured;
   char answerTO[MM::MaxStrLength];
   
   try
   {
      std::string portLowerCase = port_;
      for( std::string::iterator its = portLowerCase.begin(); its != portLowerCase.end(); ++its)
      {
         *its = (char)tolower(*its);
      }
      if( 0< portLowerCase.length() &&  0 != portLowerCase.compare("undefined")  && 0 != portLowerCase.compare("unknown") )
      {
         result = MM::CanNotCommunicate;
         // record the default answer time out
         GetCoreCallback()->GetDeviceProperty(port_.c_str(), "AnswerTimeout", answerTO);

         // device specific default communication parameters
         // for Arduino Duemilanova
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking, g_Off);
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, "57600" );
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
         // Arduino timed out in GetControllerVersion even if AnswerTimeout  = 300 ms
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "1500.0");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs", "0");
         MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str());
         pS->Initialize();
         // The first second or so after opening the serial port, the Arduino is waiting for firmwareupgrades.  Simply sleep 2 seconds.
         CDeviceUtils::SleepMs(2000);
         MMThreadGuard myLock(lock_);
         PurgeComPort(port_.c_str());
         int v = 0;
         int ret = GetControllerVersion(v);
         // later, Initialize will explicitly check the version #
         if( DEVICE_OK != ret )
         {
            LogMessageCode(ret,true);
         }
         else
         {
            // to succeed must reach here....
            result = MM::CanCommunicate;
         }
         pS->Shutdown();
         // always restore the AnswerTimeout to the default
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", answerTO);

      }
   }
   catch(...)
   {
      LogMessage("Exception in DetectDevice!",false);
   }

   return result;
}


int PrizmatixHub::Initialize()
{
   // Name
   int ret; 
   // The first second or so after opening the serial port, the Arduino is waiting for firmwareupgrades.  Simply sleep 1 second.
   CDeviceUtils::SleepMs(2000);

   MMThreadGuard myLock(lock_);

   // Check that we have a controller:
   PurgeComPort(port_.c_str());
   ret = GetControllerVersion(version_);
   if( DEVICE_OK != ret)
      return ret;
  // if(nmLeds <=0) return ERR_COMMUNICATION;
  
   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;
   initialized_ = true;
   return DEVICE_OK;
}

int PrizmatixHub::DetectInstalledDevices()
{

   if (MM::CanCommunicate == DetectDevice()) 
   {
	   char* Name = 0;
	   Name = (char*)g_DevicePulser;
	   ::CreateDevice(Name);
	   AddInstalledDevice(::CreateDevice(Name));
	   return DEVICE_OK;
      std::vector<std::string> peripherals; 
      peripherals.clear();
	
	/**  switch(nmLeds)* * /
	
	  Name=(char *) g_DeviceOneLED;
		peripherals.push_back(Name);
		Name = (char*)g_DevicePulser;
		peripherals.push_back(Name);
	 
      for (size_t i=0; i < peripherals.size(); i++) 
      {
         MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
         if (pDev) 
         {
            AddInstalledDevice(pDev);
         }
      }
   }

   return DEVICE_OK;
}



int PrizmatixHub::Shutdown()
{
   initialized_ = false;
   return DEVICE_OK;
}



int PrizmatixHub::OnVersion(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
      pProp->Set((long)version_);
   }
   return DEVICE_OK;
}
 *******************************************************************************************/
///////////////////////////////////////////////////////////////////////////////
// PrizmatixLED implementation
// ~~~~~~~~~~~~~~~~~~~~~~

PrizmatixLED::PrizmatixLED(int nmLeds_,char *Name) :
      nmLeds(nmLeds_) ,
	portAvailable_(false),
	timedOutputActive_(false),
	port_("undefined")
 
{
	
   InitializeDefaultErrorMessages();

   // add custom error messages
 
   SetErrorText(ERR_INITIALIZE_FAILED, "Initialization of the device failed");
   SetErrorText(ERR_WRITE_FAILED, "Failed to write data to the device");
   SetErrorText(ERR_CLOSE_FAILED, "Failed closing the device");
   SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The Prizmatix Hub device is needed to create this device");
   name_ = std::string(Name);
  
   // Description
   int nRet = CreateProperty(MM::g_Keyword_Description, "Prizmatix Control", MM::String, true);
   assert(DEVICE_OK == nRet);

   CPropertyAction* pAct = new CPropertyAction(this, &PrizmatixLED::OnPort);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
   // Name
 // nRet = CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);
 //  assert(DEVICE_OK == nRet);

   // parent ID display
  // CreateHubIDProperty();
}

PrizmatixLED::~PrizmatixLED()
{
   Shutdown();
}

void PrizmatixLED::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

int PrizmatixLED::GetControllerVersion()
{
	int ret = DEVICE_OK;
	const char* command = "V:0\n";
	MMThreadGuard myLock(lock_);
	PurgeComPort(port_.c_str());
	// SendSerialCommand
	ret = WriteToComPort(port_.c_str(), (const unsigned char*)command, strlen((char*)command));
	if (ret != DEVICE_OK)
		return ret;
	int nmLoop = 0;
	std::string answer;
	do {
		ret = GetSerialAnswer(port_.c_str(), "\r\n", answer);
		if (ret != DEVICE_OK || answer[0] != 'V')
		{
			Sleep(200);
			nmLoop++;
			if(nmLoop > 8)
			return ret;
			WriteToComPort(port_.c_str(), (const unsigned char*)command, strlen((char*)command));
		}
	} while (answer[0] != 'V');
 
 
	int Mik = answer.find_last_of('_');
	nmLeds = atoi(answer.data() + Mik + 1);
	if (nmLeds == 0) return 1;
	return ret;

}

int PrizmatixLED::Initialize()
{
   
	if (!IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	};
	GetControllerVersion();
   

   {  // Firmware property
			char *Com="V:1";
			int  ret;
			std::string answer;
			{
				MMThreadGuard myLock(lock_);
				PurgeComPort(port_.c_str());
				SendSerialCommandH(Com);
				
				   ret = GetSerialAnswerH(answer,'V');
			}
			int NumFirm=atoi(answer.data()+2);			
			char NameF[25];
			switch(NumFirm)
			{
				 
				case 1: strcpy(NameF,"UHPTLCC-USB");break;
				case 2: strcpy(NameF,"UHPTLCC-USB-STBL");break;
				case 3: strcpy(NameF,"FC-LED");break;
				case 4: strcpy(NameF,"Combi-LED");break;
				case 5: strcpy(NameF,"UHP-M-USB");break;
				case 6: strcpy(NameF,"UHP-F-USB");break;
				case 7: strcpy(NameF,"BLCC - 04 - USB");break;
				case 8: strcpy(NameF, "Silver - LED - USB"); break;					
				default:
					NumFirm=0;break;
			}
			if(NumFirm > 0)
				CreateProperty("Firmware Name", NameF, MM::String, true);
			if(NumFirm==2)
			{ // Add Stbl
				  
			   CPropertyAction* pAct = new CPropertyAction (this, &PrizmatixLED::OnSTBL);
			    ret = CreateProperty("STBL", "0", MM::Integer, false, pAct);
				   if (ret != DEVICE_OK)
					  return ret;

				   AddAllowedValue("STBL", "0");
				   AddAllowedValue("STBL", "1");
			}
   }
   
 
   ///


       char* command="S:0\n";
  
// SendSerialCommand
	  SendSerialCommandH(command);
 
	   long nmWrite;
     
	    std::string answer;
   int  ret = GetSerialAnswerH(  answer,'S');
   const char * NameLeds=answer.data();
   nmWrite=answer.length();
   // set property list
   // -----------------
   
   // State
   // -----
   int nRet;
   int Mik=1;
   int Until;
   
   for(int i=0;i< nmLeds ;i++)
   {
	   ValLeds[i]=0;
		  OnOffLeds[i]=0;
		  // CPropertyAction
			   CPropertyActionEx* pAct = new CPropertyActionEx (this, &PrizmatixLED::OnPowerLEDEx,i);
		   char Name[20],StateName[20];
		   Until=Mik+1;
		   while(Until< nmWrite && NameLeds[Until] !=',') Until++;
		   if( Mik+1 < nmWrite && Mik  <Until )
		   {
				memcpy(Name,NameLeds+Mik,Until-Mik);
				Name[Until-Mik]=0;
				Mik=Until+1;
				  while(Mik< nmWrite && NameLeds[Mik] ==' ') Mik++;
		   }
		   else
				sprintf(Name,"LED%d",i);  
			nRet= CreateProperty(Name, "0", MM::Integer,  false, pAct);
			 SetPropertyLimits(Name, 0, 100);
			  
		//-----
					CPropertyActionEx* pAct5 = new CPropertyActionEx (this, &PrizmatixLED::OnOfOnEx,i);
						sprintf(StateName,"State %s",Name); 
					 ret = CreateProperty(StateName, "0", MM::Integer, false, pAct5);
				   if (ret != DEVICE_OK)
					  return ret;

				   AddAllowedValue(StateName, "0");
				   AddAllowedValue(StateName, "1");
		//-----
		
			 
		  
   }
   nRet = UpdateStatus();
   if (nRet != DEVICE_OK)
      return nRet;

   initialized_ = true;

   return DEVICE_OK;
}

int PrizmatixLED::Shutdown()
{
   
 SendSerialCommandH("P:0");
   initialized_ = false;
   return DEVICE_OK;
}
int PrizmatixLED::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
	if (pAct == MM::BeforeGet)
	{
		pProp->Set(port_.c_str());
	}
	else if (pAct == MM::AfterSet)
	{
		pProp->Get(port_);
		portAvailable_ = true;
	}
	return DEVICE_OK;
}
int PrizmatixLED::WriteToPort(char *Str)
{
  					 
   if (!IsPortAvailable())
      return ERR_NO_PORT_SET;

   MMThreadGuard myLock(GetLock());

   PurgeComPortH();
   if(Str ==0)
   {
		char Buf[100],StrNum[1024];
   
	   strcpy(Buf,"P:");
	   for(int i=0;i< nmLeds;i++)
	   {
		  if( OnOffLeds[i]==0)
			 strcat(Buf,"0");
		  else
			 strcat(Buf,_itoa(ValLeds[i],StrNum,10));
		   strcat(Buf,",");
	   }
		SendSerialCommandH(Buf);
   }
   else
   {
	   SendSerialCommandH(Str);
   }
		
    SetTimedOutput(false);

   return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

 int PrizmatixLED::OnOfOnEx(MM::PropertyBase* pProp, MM::ActionType eAct,long Param)
 {
	  if (eAct == MM::BeforeGet)// && OnOffLeds[Param]>=0 )
   {
      // nothing to do, let the caller use cached property
   }
   else if (eAct == MM::AfterSet )//|| OnOffLeds[Param] ==-1)
   {
	   long pos;
		pProp->Get(pos);
		OnOffLeds[Param]=pos;
		WriteToPort(0);
	 }
	   return DEVICE_OK;
 }
 int PrizmatixLED::OnSTBL(MM::PropertyBase* pProp, MM::ActionType eAct)
 {
	  if (eAct == MM::BeforeGet )//&& ValLeds[Param] >=0)
   {
      // nothing to do, let the caller use cached property
   }
   else if (eAct == MM::AfterSet)//|| ValLeds[Param]==-1)
   {
      long Stat;
      pProp->Get(Stat);
      char Buf[20];
	 sprintf(Buf,"K:1,8,%d",Stat);
  
	     return WriteToPort(Buf);
   }

   return DEVICE_OK;
 }
int PrizmatixLED::OnPowerLEDEx(MM::PropertyBase* pProp, MM::ActionType eAct,long Param)
{
   if (eAct == MM::BeforeGet )//&& ValLeds[Param] >=0)
   {
      // nothing to do, let the caller use cached property
   }
   else if (eAct == MM::AfterSet)//|| ValLeds[Param]==-1)
   {
      double volts;
      pProp->Get(volts);
     	ValLeds[Param]= floor((volts*4095/100));

	     return WriteToPort(0);
   }

   return DEVICE_OK;
}


/////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// PrizmatixPulser implementation
// ~~~~~~~~~~~~~~~~~~~~~~

PrizmatixPulser::PrizmatixPulser(int nmLeds_, char* Name) :
	nmLeds(nmLeds_),
	IsWorking(0),
	portAvailable_(false),
	timedOutputActive_(false),
	port_("undefined"),
	mThread_(0)

{

	InitializeDefaultErrorMessages();

	// add custom error messages

	SetErrorText(ERR_INITIALIZE_FAILED, "Initialization of the device failed");
	SetErrorText(ERR_WRITE_FAILED, "Failed to write data to the device");
	SetErrorText(ERR_CLOSE_FAILED, "Failed closing the device");
	SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The Prizmatix Hub device is needed to create this device");
	name_ = std::string(Name);

	// Description
	int nRet = CreateProperty(MM::g_Keyword_Description, "Prizmatix Control", MM::String, true);
	assert(DEVICE_OK == nRet);
	CPropertyAction* pAct = new CPropertyAction(this, &PrizmatixPulser::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
 

	// parent ID display
//	CreateHubIDProperty();
}

PrizmatixPulser::~PrizmatixPulser()
{
	Shutdown();
}

void PrizmatixPulser::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}
int PrizmatixPulser::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
	if (pAct == MM::BeforeGet)
	{
		 pProp->Set(port_.c_str());
	}
	else if (pAct == MM::AfterSet)
	{
		 pProp->Get(port_);
		 portAvailable_ = true;
	}
	return DEVICE_OK;
}


int PrizmatixPulser::Initialize()
{

 
	if (!IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
	 

	///
 
	// set property list
	// -----------------

	// State
	// -----
	int nRet;
	int Mik = 1;
	int Until;
	// Buttom begin work
//	CPropertyActionEx* pAct5 = new CPropertyActionEx(this, &PrizmatixPulser::OnOfOnEx, i);
	CPropertyAction* pAct5 = new CPropertyAction(this, &PrizmatixPulser::OnOfOnEx);
	 
	int ret = CreateProperty("Start/Stop", "0", MM::Integer, false, pAct5);
	if (ret != DEVICE_OK)
		return ret;
	AddAllowedValue("Start/Stop", "0");
	AddAllowedValue("Start/Stop", "1");
	/////////////////
	CPropertyAction* pAct = new CPropertyAction(this, &PrizmatixPulser::OnTextChange);
	ret = CreateProperty("Command", "", MM::String, false, pAct);
	if (ret != DEVICE_OK)
		return ret; 
	mThread_ = new PrizmatixPulserReadThread(*this);
	mThread_->Start();


	/******************************************************
	for (int i = 0; i < nmLeds; i++)
	{
		ValLeds[i] = 0;
		OnOffLeds[i] = 0;
		// CPropertyAction
		CPropertyActionEx* pAct = new CPropertyActionEx(this, &PrizmatixPulser::OnPowerLEDEx, i);
		char Name[20], StateName[20];
		Until = Mik + 1;
		while (Until < nmWrite && NameLeds[Until] != ',') Until++;
		if (Mik + 1 < nmWrite && Mik < Until)
		{
			memcpy(Name, NameLeds + Mik, Until - Mik);
			Name[Until - Mik] = 0;
			Mik = Until + 1;
			while (Mik < nmWrite && NameLeds[Mik] == ' ') Mik++;
		}
		else
			sprintf(Name, "LED%d", i);
		nRet = CreateProperty(Name, "0", MM::Integer, false, pAct);
		SetPropertyLimits(Name, 0, 100);

		//-----
		CPropertyActionEx* pAct5 = new CPropertyActionEx(this, &PrizmatixPulser::OnOfOnEx, i);
		sprintf(StateName, "State %s", Name);
		ret = CreateProperty(StateName, "0", MM::Integer, false, pAct5);
		if (ret != DEVICE_OK)
			return ret;

		AddAllowedValue(StateName, "0");
		AddAllowedValue(StateName, "1");
		//-----



	}
	**************************************/
	nRet = UpdateStatus();
	if (nRet != DEVICE_OK)
		return nRet;

	initialized_ = true;
//??????????	this->GetCoreCallback()->SetConfig("DAC","Command");
	return DEVICE_OK;
}

int PrizmatixPulser::Shutdown()
{
	 
 SendSerialCommandH("P:0");
	if (initialized_)
		delete(mThread_);	 
	initialized_ = false;
	return DEVICE_OK;
}

int PrizmatixPulser::WriteToPort(char* Str)
{
	 
	if ( !IsPortAvailable())
		return ERR_NO_PORT_SET;

	MMThreadGuard myLock(GetLock());

	PurgeComPortH();
	 
		SendSerialCommandH(Str);
	 

 SetTimedOutput(false);

	return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
int PrizmatixPulser::CheckArrive(char N)
{
	switch (N)
	{

	case 'A':
		case 'F': // finish
			this->SetProperty("Start/Stop", "0");
		 
			this->OnPropertiesChanged();//Changed("Begin", "0");
				 
			
		break;
		case 'R':
			 
		break;
	}
	return 1;
}

int PrizmatixPulser::OnOfOnEx(MM::PropertyBase* pProp, MM::ActionType eAct)//, long Param)
{
	if (eAct == MM::BeforeGet)// && OnOffLeds[Param]>=0 )
	{
		// nothing to do, let the caller use cached property
	}
	else if (eAct == MM::AfterSet)//|| OnOffLeds[Param] ==-1)
	{
		
		pProp->Get(IsWorking);
		if (IsWorking == 1)
		{
		 
			if (!this->IsPortAvailable())
				return ERR_NO_PORT_SET;
			 this->WriteToComPortH((const unsigned char* )CommandSend.c_str(), CommandSend.length());
		}

		 
	}
	return DEVICE_OK;
}
 
int PrizmatixPulser::OnTextChange(MM::PropertyBase* pProp, MM::ActionType eAct)//, long Param)
{
	if (eAct == MM::BeforeGet)//&& ValLeds[Param] >=0)
	{
		// nothing to do, let the caller use cached property
	}
	else if (eAct == MM::AfterSet)//|| ValLeds[Param]==-1)
	{
		
		pProp->Get(CommandSend);
		size_t pos = CommandSend.find('_');
		// Replace all occurrences of character 'e' with 'P'
		while (pos != std::string::npos)
		{
			CommandSend.replace(pos, 1, 1, ',');
			pos = CommandSend.find('_');
		}
		 
		//std::string getCurrentConfig
		//this->GetCoreCallback()->SetConfig
	//	this->GetCoreCallback()->SetConfig()
		//	this->GetCoreCallback()->SetConfig()
			////"Channel", "WRCMND", "Emission", "State", "1");
	}

	return DEVICE_OK;
}

 

PrizmatixPulserReadThread::PrizmatixPulserReadThread(PrizmatixPulser& aInput) :
	state_(0),
	Pulser_(aInput)
{
}

PrizmatixPulserReadThread::~PrizmatixPulserReadThread()
{
	Stop();
	wait();
}

int PrizmatixPulserReadThread::svc()
{
	while (!stop_)
	{
	 
		 
		 if(Pulser_.IsWorking==1)
		 { 
			 unsigned long bytesRead = 0;
			 unsigned char answer[1];
			 
			 Pulser_.ReadFromComPortH(answer, 1, bytesRead);
			 if (bytesRead != 0)
				 Pulser_.CheckArrive(answer[0]);
		//	aInput_.ReportStateChange(state);
		 
			CDeviceUtils::SleepMs(500);
		 }
		 else
		CDeviceUtils::SleepMs(1500);
	}
	return DEVICE_OK;
}


void PrizmatixPulserReadThread::Start()
{
	stop_ = false;
	activate();
}

