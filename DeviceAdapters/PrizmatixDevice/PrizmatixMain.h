#ifndef __prizmatix_main_
#define __prizmatix_main_
#include "MMDevice.h"
#include "DeviceBase.h"
#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
 
#define ERR_INITIALIZE_FAILED 102
#define ERR_WRITE_FAILED 103
#define ERR_CLOSE_FAILED 104
#define ERR_BOARD_NOT_FOUND 105
#define ERR_PORT_OPEN_FAILED 106
#define ERR_COMMUNICATION 107
#define ERR_NO_PORT_SET 108
#define ERR_VERSION_MISMATCH 109

 /*****************************
 
class PrizmatixHub : public HubBase<PrizmatixHub>  
{
public:
   PrizmatixHub();
   ~PrizmatixHub();
   int GetNmLEDS(){return nmLeds;};
   int Initialize();
   int Shutdown();
   void GetName(char* pszName) const;
   bool Busy();

   bool SupportsDeviceDetection(void);
   MM::DeviceDetectionStatus DetectDevice(void);
   int DetectInstalledDevices();

   // property handlers
   int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
  //MMM???? int OnLogic(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);

   // custom interface for child devices
   bool IsPortAvailable() {return portAvailable_;}
   bool IsLogicInverted() {return invertedLogic_;}
   bool IsTimedOutputActive() {return timedOutputActive_;}
   void SetTimedOutput(bool active) {timedOutputActive_ = active;}

   int PurgeComPortH() {return PurgeComPort(port_.c_str());}
   int WriteToComPortH(const unsigned char* command, unsigned len)
   {
	   return WriteToComPort(port_.c_str(), command, len);
   }
   int ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
   {
      return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
   }
     int  SendSerialCommandH(char *b)
	 {
		 if(initialized_==false) return 0;
		 return SendSerialCommand(port_.c_str(), b,"\n");
	 }
	  int  GetSerialAnswerH( std::string &answer)
	 {
		   
    return  GetSerialAnswer(port_.c_str(), "\r\n", answer);
		  
	 }
	  static MMThreadLock lock_;
   static MMThreadLock& GetLock() {return lock_;}
  
   void SetShutterState(unsigned state) {shutterState_ = state;}
   void SetSwitchState(unsigned state) {switchState_ = state;}
   unsigned GetShutterState() {return shutterState_;}
   unsigned GetSwitchState() {return switchState_;}
   int GetNmLeds() {return nmLeds;}
   int GetVersionAns() { return version_; }
private:
	int nmLeds;
	
   int GetControllerVersion(int&);
   int GetControllerVersionPulser(int& version, std::string answer);
   std::string port_;
   bool initialized_;
   bool portAvailable_;
   bool invertedLogic_;
   bool timedOutputActive_;
   int version_;
   
   unsigned switchState_;
   unsigned shutterState_;
};
//CGenericBase
 *******************************/
//PrizmatixLED   CSignalIOBase
class PrizmatixLED : public CGenericBase<PrizmatixLED>  
{
public:
   PrizmatixLED(int nmLeds,char *Name);
   ~PrizmatixLED();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
   std::string port_;
   bool portAvailable_;
   bool IsPortAvailable() { return portAvailable_; };
 
   void GetName(char* pszName) const;
   int PurgeComPortH() { return PurgeComPort(port_.c_str()); }
   int WriteToComPortH(const unsigned char* command, unsigned len) { return WriteToComPort(port_.c_str(), command, len); }
   int ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
   {
	   return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
   }
   int  SendSerialCommandH(char* b)
   {
	   if (initialized_ == false) return 0;
	   return SendSerialCommand(port_.c_str(), b, "\n");
   }
   int  GetSerialAnswerH(std::string& answer,char LookFor)
   {	
	   int  ret;
	   int nmLoop = 0;
	   do {
		    ret = GetSerialAnswer(port_.c_str(), "\r\n", answer);
		   if (ret != DEVICE_OK || nmLoop > 5)
			   return ret;
		   nmLoop++;
	   } while (LookFor!= 0 && answer[0] != LookFor);
	   return ret;

   }
   static MMThreadLock lock_;
   static MMThreadLock& GetLock() { return lock_; };

   int OnPort(MM::PropertyBase* pProp, MM::ActionType pAct);
   bool IsTimedOutputActive() { return timedOutputActive_; }
   void SetTimedOutput(bool active) { timedOutputActive_ = active; }

   // DA API
      virtual bool Busy()
	  {
		  return false;
	  }
	   /*  int SetGateOpen(bool open) {return DEVICE_OK;};  // abstract function in paret
	int GetGateOpen(bool& open) { return DEVICE_OK;};
	int SetSignal(double volts){return DEVICE_OK;} ;
	int GetSignal(double& volts) { return DEVICE_UNSUPPORTED_COMMAND;}     
	int GetLimits(double& minVolts, double& maxVolts) {return DEVICE_OK;}
   
   int IsDASequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}
   */
   // action interface
   // ----------------
 
   int OnPowerLEDEx(MM::PropertyBase* pProp, MM::ActionType eAct,long Param);
   int OnOfOnEx(MM::PropertyBase* pProp, MM::ActionType eAct,long Param);
   int OnSTBL(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
 
   int WriteToPort(char * Str);
   long  ValLeds[10];
   long  OnOffLeds[10];
   bool initialized_, timedOutputActive_;
   int GetControllerVersion();
  
   int nmLeds;
   std::string name_;
};

class PrizmatixPulserReadThread;
//PrizmatixPulser   
class PrizmatixPulser : public CGenericBase<PrizmatixPulser>
{
public:
	PrizmatixPulser(int nmLeds, char* Name);
	~PrizmatixPulser();
	long IsWorking;
	bool IsTimedOutputActive() { return timedOutputActive_; }
	void SetTimedOutput(bool active) { timedOutputActive_ = active; }
	int CheckArrive(char N);
	bool portAvailable_, timedOutputActive_;
	// MMDevice API
	// ------------
	int Initialize();
	int Shutdown();
	 
	bool IsPortAvailable() { return portAvailable_; };
	PrizmatixPulserReadThread* mThread_;
	void GetName(char* pszName) const;
	int PurgeComPortH() { return PurgeComPort(port_.c_str()); }
	int WriteToComPortH(const unsigned char* command, unsigned len) { return WriteToComPort(port_.c_str(), command, len); }
	int ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
	{
		return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
	}
	int  SendSerialCommandH(char* b)
	{
		if (initialized_ == false) return 0;
		return SendSerialCommand(port_.c_str(), b, "\n");
	}
	int  GetSerialAnswerH(std::string& answer)
	{

		return  GetSerialAnswer(port_.c_str(), "\r\n", answer);

	}
	static MMThreadLock lock_;
	static MMThreadLock& GetLock() { return lock_; }

	// DA API
	virtual bool Busy()
	{
		return false;
	}
 
// action interface
// ----------------
	int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);
//	int OnPowerLEDEx(MM::PropertyBase* pProp, MM::ActionType eAct, long Param);
	int OnOfOnEx(MM::PropertyBase* pProp, MM::ActionType eAct);// , long Param);
	
	int OnTextChange(MM::PropertyBase* pProp, MM::ActionType eAct);
	
private:
	std::string port_;
	std::string CommandSend;
	int WriteToPort(char* Str);
	long  ValLeds[10];
	long  OnOffLeds[10];
	bool initialized_;
	int nmLeds;
	std::string name_;
};



class PrizmatixPulserReadThread : public MMDeviceThreadBase
{
public:
	PrizmatixPulserReadThread(PrizmatixPulser& aInput);
	~PrizmatixPulserReadThread();
	int svc();
	int open(void*) { return 0; }
	int close(unsigned long) { return 0; }

	void Start();
	void Stop() { stop_ = true; }
	PrizmatixPulserReadThread& operator=(const PrizmatixPulserReadThread&)
	{
		return *this;
	}


private:
	long state_;
	PrizmatixPulser& Pulser_;
	bool stop_;
};
 
#endif