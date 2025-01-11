#ifndef GREATEYES_H
#define GREATEYES_H

// version 12.3.2006.11
// email: marc.raugust@greateyes.de
//		  sw@greateyes.de	
// please read the documentation to get more information about the functions

#if defined(_MSC_VER)
# pragma once
#endif
#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3))
# pragma once
#endif

#if defined(_WINDOWS) || defined(_WIN) ||  defined(_WIN32) || defined(_WIN64)
#ifndef DLL_IMPORT_STRING
		#define DLL_IMPORT_STRING __declspec(dllimport)
	#endif
#else
	#define DLL_IMPORT_STRING
#endif

#ifdef __cplusplus
 	extern "C" 
{
#endif

//--------------------------------------------------------------------------------------------------------
/*
Content:

1. Constant
	1.1 Possible value of statusMSG
	1.2 Misc

2. Exported DLL Functions
	2.1 Setup camera inteface (USB/Ethernet)
	2.2 Connecting to a greateyes camera server
	2.3 Connecting to a greateyes camera (USB/Ethernet)
	2.4 Initialization of greateyes camera (USB/Ethernet) 	
	2.5 Set functions
	2.6 Get functions
	2.7 Temperature control functions
	2.8 Image acquisition

3. OBSOLETE FUNCTIONS
*/
//--------------------------------------------------------------------------------------------------------


// 1. Constant
// 1.1 Possible value of statusMSG
//--------------------------------------------------------------------------------------------------------
static const 	int MESSAGE_Camera_Ok =0;    				// camera detected and ok
static const 	int MESSAGE_NoCamera_Connected = 1;  		// no camera detected
static const 	int MESSAGE_could_not_open_USBDevice = 2;   // there is a problem with the USB interface
static const 	int MESSAGE_WriteConfigTable_Failed = 3;  	// transferring data to cam failed - TimeOut!
static const 	int MESSAGE_WriteReadRequest_Failed = 4;    // receiving data  from cam failed - TimeOut!
static const 	int MESSAGE_NoTrigger = 5;     				// no extern trigger signal within time window of TriggerTimeOut
static const 	int MESSAGE_NewCameraDetected = 6;     		// new cam detected - you need to perform CamSettings
static const 	int MESSAGE_UnknownCamID = 7;    			// this DLL was not written for connected cam - please request new greateyes.dll
static const 	int MESSAGE_OutofRange = 8;					// one ore more parameters are out of range
static const 	int Message_NoNewData = 9;					// no new image data
static const 	int Message_Busy = 10;						// camera busy
static const 	int Message_CoolingTurnedOff = 11;			// cooling turned off
static const 	int Message_MeasurementStopped = 12;		// measurement stopped
static const    int Message_BurstModeTooMuchPixels = 13;	// too many pixels for BurstMode. Set lower number of measurements or higher binning level
static const    int Message_TimingTableNotFound = 14;		// timing table for selected readout speed not found
static const    int Message_NotCritical = 15;				// function stopped but there is no critical error (no valid result; catched division by zero). please try to call function again. 
static const    int Message_IllegalCombinationBinCrop = 16;	// for firmware < v12 the combination of binning and crop mode is not supported 

// 1.2 Function Constant
//--------------------------------------------------------------------------------------------------------
static const    int maxPixelBurstTransfer = 8823794;

static const	int sensorFeature_capacityMode = 0;
static const	int sensorFeature_cropX = 1;
static const	int sensorFeature_binningX = 2;
static const	int sensorFeature_gainSwitch = 3;


static const	int readoutSpeed_50_kHz = 50;
static const	int readoutSpeed_100_kHz = 100;
static const	int readoutSpeed_250_kHz = 250;
static const	int readoutSpeed_500_kHz = 500;
static const	int readoutSpeed_1_MHz = 1000;
static const	int readoutSpeed_3_MHz = 3000;
static const	int readoutSpeed_5_MHz = 5000;


static const	int connectionType_USB = 0;
static const	int connectionType_Ethernet = 3;


// 2. Exported DLL Functions
//--------------------------------------------------------------------------------------------------------

// 2.1 Setup camera inteface (USB/Ethernet)
//--------------------------------------------------------------------------------------------------------

 DLL_IMPORT_STRING bool SetupCameraInterface(int connectionType, char* ipAddress, int& statusMSG, int addr);
	// IN: connectionType		connectionType_USB (0) (default): Camera connected via USB 
	//							connectionType_Ethernet (3): Camera connected via Ethernet (TCP/IP)
	// IN: ipAddress			IP address of greateyesCameraServer
	// In: addr					index of connected devices; begins at addr = 0 for first device; max. 4 devices
	// Out: statusMSG           index of status message
	// Result: Bool             success true/false

// 2.2 Connecting to a greateyes camera server 
//--------------------------------------------------------------------------------------------------------

 DLL_IMPORT_STRING bool ConnectToMultiCameraServer();
	// Necessary for connection via Ethernet only. Call this function to connect to a greateyesCameraServer which is connected to up to four cameras. (MultiCamMode)
	// Suitable for operating multiple greateyes cameras with USB interface connected to a greateyes camera server.
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool ConnectToSingleCameraServer(int addr);
	// Necessary for connection via Ethernet only. Call this function to connect to up to four greateyesCameraServers. Each greateyesCameraServer operates one camera. (MultiServerMode)
	// Suitable for all greateyes cameras with ethernet interface.
	// In: addr					index of connected devices; begins at addr = 0 for first device; max. 4 devices
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool DisconnectCameraServer(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device; max. 4 devices
	//							The function ignores the parameter addr when connected with ConnectToCameraServer() (MultiCamMode)	
	// Result: Bool             success true/false

// 2.3 Connecting to a greateyes camera (USB/Ethernet) 
//--------------------------------------------------------------------------------------------------------

 DLL_IMPORT_STRING int GetNumberOfConnectedCams();
	// Result: 		            number of devices connected 
	//							Call this funktion before calling ConnectCamera()
	//							Not required if connected with ConnectToSingleCameraServer() to a SingleCameraServer. 
	
 DLL_IMPORT_STRING bool ConnectCamera(int& modelId, char*& modelStr, int& statusMSG, int addr);
	//			-replaces CheckCamera function; connects up to 4 devices; Ethernet and USB
	//			-call GetNumberOfConnectedCams before in case of connecting to cameras with USB interface directly or through a MultiCamMode
	//
	// Out: modelID   			internal model specific ID  (no serial number)
	// Out: model	          	model string
	// Out: statusMSG       	index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device; max. 4 devices
	//
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool DisconnectCamera(int& statusMSG, int addr);
	// In:      addr			index of connected devices; begins at addr = 0 for first device
    // Out:     statusMSG       index of status message
	// Result:  Bool            success true/false

// 2.4 Initialization of greateyes camera (USB/Ethernet) 
//--------------------------------------------------------------------------------------------------------
 DLL_IMPORT_STRING bool InitCamera(int& StatusMSG, int addr);
	// It is recommended to call InitCamera(..) at least one time after connecting to the camera. 	
	// OUT: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	//
	// Result: Bool    		   	success true/false

//--------------------------------------------------------------------------------------------------------


//	2.5 Set Functions
//--------------------------------------------------------------------------------------------------------
 DLL_IMPORT_STRING bool SetExposure(int exposureTime, int& statusMSG, int addr);
	// IN: exposureTime     	exposure time [0..2^31] ms
	// OUT: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool SetReadOutSpeed(int readOutSpeed, int& StatusMSG, int addr);
	// IN: readoutSpeed         sets pixel clock to [0..5]
	//							0 -> 1 MHz
	//							3 -> 3 MHz
	//							5 -> 500 kHz
	//							6 -> 50 kHZ
	// OUT: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	//	
	// Result: Bool    		   	success true/false

 DLL_IMPORT_STRING bool SetBinningMode(int binningX, int binningY, int& StatusMSG, int addr);
	// IN: binningX             Number of pixels [1...numPixelInX] to be binned in x-direction (if supported by CCD)                        	
    //
    //                          Note: 	For backwards compatibility, when using cameras with
    //                                  earlier firmware revisions (rev. 11 or lower) the binningX
    //                                  parameter was realized in software thus it does not reduce the overall SNR. 
    //									Also the parameter is interpreted differently as shown below:
    //
   	//									pow(2, binningX) = Number of pixels to be binned in x-direction   	
    //
	// IN: binningY             Number of pixels [1...numPixelInY] to be binned in y-direction                          
    //
	//		                    Note: 	For backwards compatibility, when using cameras with
	//                                  earlier firmware revisions (rev. 11 or lower) the binningY
	//                                  parameter is interpreted differently as shown below:
    //
	//                      			0		No binning of lines
	//                      			1		Binning of 2 lines
	//                      			2		Binning of 4 lines
	//                      			3		Binning of 8 lines
	//                      			4		Binning of 16 lines
	//                      			5		Binning of 32 lines
	//                      			6 		Binning of 64 lines
	//                      			7		Binning of 128 lines
	//                                  8 		Full vertical binning	
	//	
	// OUT: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	//
	// Result: Bool    		   	success true/false

 DLL_IMPORT_STRING bool SetShutterTimings(int openTime, int closeTime, int& statusMSG, int addr);
	// IN: openTime				time to wait before exposure [ms]
	// IN: closeTime			time to wait after exposure [ms]
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool OpenShutter(int state, int& statusMSG, int addr);
	// IN: state				shutter mode
	//							state = 0 --> close shutter		 (TTL Low)
	//							state = 1 --> open shutter		 (TTL High)
	//							state = 2 --> automatic shutter  (TTL High while image acquisition)
	//							For automatic shutter it is necessary to set shutter open and close time with SetShutterTimings() function.
	//							!!Automatic shutter does not work in combination with Burst - Mode.

	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool SyncOutput(bool syncHigh, int& statusMSG, int addr);
	// IN: syncHigh				sync output high/low
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool SetupBurstMode(int numberOfMeasurements, int& statusMSG, int addr);
	// IN: numberOfMeasurements	 number of measurements in series.
	// Out: statusMSG            index of status message  -->  see StatusMSG #13
	// In: addr					 index of connected devices; begins at addr = 0 for first device
	// Result: Bool              success true/false

 DLL_IMPORT_STRING int ActivateBurstMode(bool status, int& statusMSG, int addr);
	//							Automatic shutter does not in combination with Burst - Mode.
	//
	// IN: status	 			sets burst mode on/off
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Int              number of measurements set up  (SetupBurstMode)

 DLL_IMPORT_STRING bool SetupCropMode2D(int col, int line, int &statusMSG, int addr);
	// IN: col	 			 	 number of columns to read out
	// IN: line	 			 	 number of lines to read out
	// Out: statusMSG            index of status message
	// In: addr					 index of connected devices; begins at addr = 0 for first device
	// Result: Bool              success true/false
	//
	// Depending on the number of sensor readout ports used (see SetupSensorOutputMode) this function behaves slight differently.
	// 1 output 	: lines and columns are cropped from the border of the sensor output used
	// 2 outputs 	: columns that get returned are counted from the center of the sensor instead of the border. 
	//				  The number of returned columns is rounded down to the next multiple of 2.
	// (4 outputs 	: columns behave like with 2 outputs, cropping rows is not supported)


 DLL_IMPORT_STRING int ActivateCropMode(bool status, int &statusMSG, int addr);
	// IN: status	 			 sets crop mode on/off
	// Out: statusMSG            index of status message
	// In: addr					 index of connected devices; begins at addr = 0 for first device
	// Result: Int               number of lines set up (SetupCropMode)

DLL_IMPORT_STRING bool SetupNominalImageFormat(bool enable, int &statusMSG, int addr);
	// IN: enable	 			 sets crop mode to the nominal image format instead of the sensor format
	// Out: statusMSG            index of status message
	// In: addr					 index of connected devices; begins at addr = 0 for first device
	// Result: Bool              success true/false
	//
	// Depending on the sensor used, the sensor pixel format does not match the nominal image dimensions.
	// For example ELSE 1024 1024 series camera sensors have a sensor format of 1056 x 1027 pixel.
	// This function can be used to set the readout dimensions to the nominal image format of 1024 x 1024 pixel.
	// To achivee that the CropMode functions are used internally, with pre defined sizes depending on the camera model


 DLL_IMPORT_STRING bool SetupGain(int gainSetting, int& statusMSG, int addr);
	// IN: gainSetting			 0 -> Low ( Max. Dyn. Range )
	//							 1 -> Std ( High Sensitivity )
	// Out: statusMSG            index of status message
	// In: addr					 index of connected devices; begins at addr = 0 for first device
	// Result: Bool              success true/false

 DLL_IMPORT_STRING bool SetupCapacityMode(bool capacityMode, int& statusMSG, int addr);
	// In: capacityMode			 false -> Standard ( Low Noise )
	//							 true -> Extended ( High Signal )
	// Out: statusMSG            index of status message
	// In: addr					 index of connected devices; begins at addr = 0 for first device
	// Result: Bool              success true/false

 DLL_IMPORT_STRING bool SetupTransferOptions(bool safeFifoMode, bool saveUsbMode);
	// IN: safeFifoMode			 true/false
	//							 default: true
	// IN: saveUsbMode			 true/false
	//							 default: false	
	// Result: Bool              success true/false

 DLL_IMPORT_STRING bool SetupSensorOutputMode(int sensorOutputMode, int addr);
	// In: sensorOutputMode		[ 0 .. (NumberOfSensorOutputModes - 1) ]
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: bool             success true/false

 DLL_IMPORT_STRING int ClearFifo(int& statusMSG, int addr);
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Integer          number of cleared blocks


 DLL_IMPORT_STRING bool SetBitDepth(int bytesPerPixel, int& statusMSG, int addr);
	// In: bytesPerPixel		[2 .. 4] 
	//							set bytes per pixel for cameras with 18 bit adc (max. 20 bit dynamic range through oversampling)
	//							for cameras with 16 bit adc bytesPerPixel is always 2	
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: bool	            success true/false

 DLL_IMPORT_STRING bool SetExtTriggerTimeOut(int extTriggerTimeOut, int addr);
	// In: triggerTimeOut		timeout for ext. trigger event [ms]. Choose 0 for infinity.  
	// Result: bool	            success true/false
	//							set timeout for StartMeasurement_DynBitDepth() function with external trigger 

 DLL_IMPORT_STRING bool	SetBusyTimeout(int timeout);
	// In: timeOut				>= 0 ms (Default : 3000ms)
	// Result: bool	            success true/false
	//							Set timeout for function calls. If the timeout is set to 0, functions will return busy immediately. 
	//							Otherwise the function will try to get a slot for the time of setted timeout. 					

 DLL_IMPORT_STRING bool	SetLEDStatus(bool status, int& statusMSG, int addr);
 	// Switch backside LED's on/off
 	// In: status				true/false 
	// Result: bool	            success true/false
	
	
// 2.6 Get Functions
//--------------------------------------------------------------------------------------------------------
 DLL_IMPORT_STRING char* GetDLLVersion(int& size);
	// In: size					number of characters 
	// Result: char*			Dll version

 DLL_IMPORT_STRING int GetFirmwareVersion(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Integer			firmware version

 DLL_IMPORT_STRING bool GetImageSize(int& width, int& height, int& bytesPerPixel, int addr);
	// OUT: width				number of columns (number of pixel in x-direction along the readout register)
	// OUT: height				number of rows/lines (number of pixel in y-direction)
	// Out: bytesPerPixel		number of bytes per sample 
	//
	//							bytesPerPixel = 2  -> 16 Bit -> Use a 16 bit type pointer or an 8 bit type pointer and allocate 2 bytes per pixel.
	//							bytesPerPixel = 3  -> 24 Bit -> Use an 8 bit type pointer and allocate 3 bytes per pixel. Save 25% memory compared to 32 bit output.
	//							bytesPerPixel = 4  -> 32 Bit -> Use a 32 bit type pointer or an 8 bit type pointer and allocate 4 bytes per pixel. Easy to handle but 8 bit are useless.   
	//
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: bool				success true/false				 

 DLL_IMPORT_STRING int GetSizeOfPixel(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: int              size of pixel in µm

 DLL_IMPORT_STRING bool DllIsBusy(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Boolean          busy status of the camera

 DLL_IMPORT_STRING int GetMaxExposureTime(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: int              max ExposureTime in ms supported by the camera model / firmware version

 DLL_IMPORT_STRING int GetMaxBinningX(int& statusMSG, int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// OUT: statusMSG           index of status message
	//
	// Result: int              max. possible value for parameter binningX. (depends on sensor type and crop mode setting)

 DLL_IMPORT_STRING int GetMaxBinningY(int& statusMSG, int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// OUT: statusMSG           index of status message
	//
	// Result: int              max. possible value for parameter binningY. (depends on sensor type and crop mode setting)

 DLL_IMPORT_STRING bool SupportedSensorFeature(int feature, int& statusMSG, int addr);
	// This function provides information about the supported sensor features of the camera.
	// In: feature				possible features:
	//								sensorFeature_capacityMode -> Only sensors with this feature can operate in the capacity mode.
	//								sensorFeature_binningX -> Only sensors with this feature can bin in x - direction (serial).
	//								sensorFeature_cropX	-> Only sensors with this feature can operate in the crop mode. 

	// In: addr					index of connected devices; begins at addr = 0 for first device
	// OUT: statusMSG           index of status message
	// Result: Bool    		   	sensor supports feature (true/false)

 DLL_IMPORT_STRING int GetNumberOfSensorOutputModes(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	//							
	// Result: int				number	of possible output modes for camera model.
	//                          Usually a sensor has a single output only.
	//                          For larger format sensors, e.g. 4096px x 4096px sensors,
	//                          (modelID = 12) up to 10 output modes are specified.
	
 DLL_IMPORT_STRING const char* GetSensorOutputModeStrings(int index, int addr);
	// In: index				[ 0 .. (NumberOfSensorOutputModes - 1) ]
	// In: addr					index of connected devices; begins at addr = 0 for first device
	//							
	// Result: char*			output mode string

 DLL_IMPORT_STRING float GetLastMeasTimeNeeded(int addr);
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Float            time needed (exposure time + read out) in ms



//	2.7 Temperature Control Functions
//--------------------------------------------------------------------------------------------------------

 DLL_IMPORT_STRING int TemperatureControl_Init(int coolingHardware, int& minTemperature, int& maxTemperature, int& statusMSG, int addr);
	// IN: coolingOption		choose your hardware

	//							To get the CoolingOption for your camera please have a
	//							look in the SDK folder of your software release. There you
	//							can find a “TemperatureHardwareOption.txt”. If you can
	//							not find, please contact us.

	// Out: statusMSG			index of status message
	// Out: minTemperature		min. possible value in °C for parameter temperature of function TemperatureControl_SetTemperature()
	// Out: maxTemperature		max. possible value in °C for parameter temperature of function TemperatureControl_SetTemperature()
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: int				number of available cooling levels

 DLL_IMPORT_STRING bool TemperatureControl_GetTemperature(int thermistor, int& temperature, int& statusMSG, int addr);
	// In: thermistor			0: sensor temperature   1: backside temperature		100: sensor temperaure accelerated acquisition	101: backside temperaure accelerated acquisition 
	// 								It is recommended to check the backside temperature frequently. Maximum backside temperature is about 55°C.
	// 								If the backside temperature gets higher than 55°C, please turn off the cooling control and contact greateyes GmbH.

	// Out: temperature 		temperature in °C --> ( Kelvin - 273.15 ) 
	// Out: statusMSG           index of status message
	//							If the function returns statusMSG = 11, the camera resets cooling controll because the backside temperature is to high.
	//							In this case you should switch off cooling control (TemperatureControl_SwitchOff())
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool TemperatureControl_SetTemperature(int temperature, int& statusMSG, int addr);
	// IN: temperature			adjust temperature of CCD sensor in °C --> (Kelvin - 273.15). 
	//							value must be between TemperatureControl_GetMinTemperature() and TemperatureControl_GetMaxTemperature() 
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool TemperatureControl_SwitchOff(int& statusMSG, int addr);
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false


// 2.8 Image Acquisition
//--------------------------------------------------------------------------------------------------------
 DLL_IMPORT_STRING bool StartMeasurement_DynBitDepth(bool correctBias, bool showSync, bool showShutter, bool triggerMode, int& statusMSG, int addr);
	// Starts measurement in a thread
	// IN: correctBias          if true, each line will be intensity corrected dependent on the dark pixel values left and right of this line
	// IN: showSync        		if true, the sync output of the camera will rise during exposure time otherwise it remains low.
	// IN: showShutter			use auto shutter
	// IN: triggerMode    		external trigger	
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false
	
 DLL_IMPORT_STRING bool GetMeasurementData_DynBitDepth(void* pInDataStart, int& statusMSG, int addr);
	// Gets measurement started with StartMeasurement() function. Use DllIsBusy() function to check whether measurement is ready. Use GetImageSize() function to get size of sample.
	//
	// IN: pIndataStart         pointer to image array	
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false

 DLL_IMPORT_STRING bool PerformMeasurement_Blocking_DynBitDepth(bool correctBias, bool showSync, bool showShutter, bool triggerMode, int triggerTimeOut, void* pInDataStart, int& statusMSG, int addr);
	// Start and wait for measurement. Function blocks for duration of measurement. Function returns measurement in pInDataStart mem. Use GetImageSize() function to get size of sample.
	// (This function has more that six input parameters and might not work with some environments (for example Matlab). Alternatively use StartMeasurement_DynBitDepth() and GetMeasurementData_DynBitDepth()  )
	// IN: correctBias          if true, each line will be intensity corrected dependent on the dark pixel values left and right of this line
	// IN: showSync        		if true, the sync output of the camera will rise during exposure time otherwise it remains low.
	// IN: showShutter			use auto shutter
	// IN: triggerMode    		external trigger
	// IN: triggerTimeOut		timeout for ext. trigger event. Choose 0 for infinity.
	// IN: pIndataStart         pointer to image array	
	// Out: statusMSG           index of status message
	// In: addr					index of connected devices; begins at addr = 0 for first device
	// Result: Bool             success true/false	

 DLL_IMPORT_STRING bool StopMeasurement(int addr);
	// Stops measurement started by StartMeasurement() function. Do not work with PerformMeasurement_Blocking().
	// After stopped measurement the DllIsBusy() function will return false and the GetMeasurementData() funktion will return StatusMSG=12 (Message_MeasurementStopped).
	// Result: Bool             success true/false
	//
	// In: addr					index of connected devices; begins at addr = 0 for first device



// 3. OBSOLETE FUNCTIONS
//------------------
// If you are using one of this functions, it is recommended to update them.
// Please have a look into the sdk manual.
//--------------------------------------------------------------------------------------------------------

 DLL_IMPORT_STRING bool SetupCropMode(int line, int &statusMSG, int addr);
 DLL_IMPORT_STRING bool CheckCamera(int& modelID,char*& model,int& statusMSG);
 DLL_IMPORT_STRING bool GetImageData(bool correctBias, bool showSync, bool showShutter, bool triggerMode, int triggerTimeOut, unsigned short* pInDataStart, int& writeBytes, int& readBytes, int& statusMSG, int addr);
 DLL_IMPORT_STRING int SetupCoolingHardware(int coolingOption, int& statusMSG, int addr);
 DLL_IMPORT_STRING char* GetCoolingStepString(int index, int& statusMSG, int addr);
 DLL_IMPORT_STRING bool Control_CCD_Temperature(int coolingLevel,int& statusMSG, int addr);
 DLL_IMPORT_STRING bool GetTemperatureValue(int thermistor, int& temperature,int& statusMSG, int addr);
 DLL_IMPORT_STRING bool CloseCamera(int Cam, bool JustShootEm);
 DLL_IMPORT_STRING bool SetConnectionType(int type, char* ipAddress, int& statusMSG);
 DLL_IMPORT_STRING bool ConnectToCameraServer();
 DLL_IMPORT_STRING int TemperatureControl_Setup(int coolingHardware, int& statusMSG, int addr);
 DLL_IMPORT_STRING char* TemperatureControl_GetLevelString(int index, int& statusMSG, int addr);
 DLL_IMPORT_STRING bool TemperatureControl_SetTemperatureLevel(int coolingLevel, int& statusMSG, int addr);
 DLL_IMPORT_STRING bool CamSettings(int readoutSpeed, int exposureTime, int binningX, int binningY, int& numPixelInX, int& numPixelInY, int& pixSize, int& statusMSG, int addr);
 DLL_IMPORT_STRING bool PerformMeasurement_Blocking(bool correctBias, bool showSync, bool showShutter, bool triggerMode, int triggerTimeOut, unsigned short* pInDataStart, int& writeBytes, int& readBytes, int& statusMSG, int addr);
 DLL_IMPORT_STRING int GetBitDepth(int addr);
 DLL_IMPORT_STRING bool GetMeasurementData(unsigned short* pInDataStart, int& writeBytes, int& readBytes, int& statusMSG, int addr);
 DLL_IMPORT_STRING bool StartMeasurement(bool correctBias, bool showSync, bool showShutter, bool triggerMode, int triggerTimeOut, int& statusMSG, int addr);

//--------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif //GREATEYES_H
