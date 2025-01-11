// greateyesExample.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//----------------------------------------------------------------------------------------------------


#include <thread>
// #include "stdafx.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <stdint.h>
#include <chrono>
#include "greateyes.h"


//----------------------------------------------------------------------------------------------------

/*
greateyes example program for cameras with firmware 12 and later

Content:

0. parameter setup
1. connect to camera
2. setup cooling control
3. setup auto shutter
4. acquisition of a full frame
5. acquisition of a binned frame
6. acquisition of a cropped frame
7. acquisition burst mode
8. acquisition of binned cropped frames in a burst
9. disconnect camera

Result Image: 
All results will be saved as raw data (16/32 bit) and as a scaled 8 bit PGM image.
The 16 bit raw data will be saved as 16 bit PGM image.
The 32 bit raw data will be saved as pixel map text image with pixel separated through one space char (" ") and rows seperated through new line ("\n").
To view a pixel map text image import it with ImageJ as "Text Image".
*/

//----------------------------------------------------------------------------------------------------
/****************************************
* 0. parameter setup
****************************************/

//camera interface 
//use connectionType_Ethernet or connectionType_USB
int connectionType = connectionType_USB;

//ip adress necessary if connectionType = connectionType_Ethernet
char* ip = "192.168.1.233";

//exposure time in ms
int exposureTimeMilliseconds = 10;

//pixel clock
int readoutSpeed = readoutSpeed_1_MHz;

//set bpp for cameras with 18 Bit adc
int setBytesPerPixel = 4;

//change number to option provided with sdk	
const int coolingHardwareOption = 42223;

//set temperature for temperature control in degree Celsius
bool switchOnTemperatureControl = false;
int setTemperature = 15;

//shutter timings in SHUTTER_AUTO mode
int shutterOpenTimeMilliseconds = 25;
int shutterCloseTimeMilliseconds = 25;

//binning parameter
int binningX = 2;
int binningY = 2;

//crop parameter
int cropY = 35;
int cropX = 512;

//burst parameter
int numberOfMeasurements = 3;

//bias correction 
bool enableBiasCorrection = false;

//configuration of which camera TTL inputs/outputs to use (is used later when starting acquisition)
bool enableSyncOutput = true;
bool enableShutterOutput = true;	//only affect on enabled auto shutter 
bool useExternalTrigger = false;
int triggerTimeoutMilliseconds = 10000;

//In case of using USB interface it can take up to 15 seconds until the system provides the device. 
int connectUsbTimeoutMilliseconds = 15000;


/****************************************
/* end of parameter setup
****************************************/
//----------------------------------------------------------------------------------------------------

int lastStatus = 0;

char* fileExtString[5] = {	"",
							"_8Bit_scaled.pgm",		//8 bit pgm image 
							"_16Bit_raw.pgm",		//16 bit pgm image
							"_32Bit_raw.txt",		//32 bit greyscale "ImageJ" compatible text image 
							"_32Bit_raw.txt"};		//32 bit greyscale "ImageJ" compatible text image 

static const unsigned int maxColor_8Bit  = 0xFF;
static const unsigned int maxColor_16Bit = 0xFFFF;
static const unsigned int maxColor_24Bit = 0xFFFFFF;

using namespace std;

//Save PGM or Text Image
//----------------------------------------------------------------------------------------------------

void writeToFile(void* inBuf, int width, int height, int bytesPerPixel, std::string filename)
{
		
	//8 bit array
	uint8_t* charData = reinterpret_cast <uint8_t*>(inBuf);

	//16 bit array
	uint16_t* shortData = reinterpret_cast <uint16_t*>(inBuf);

	//32 bit array
	uint32_t* longData = reinterpret_cast <uint32_t*>(inBuf);

	//raw data array
	uint32_t* imgBuf = new uint32_t[width * height];
	uint32_t* imgBufStart = imgBuf;

	int maxColorValue_source = 0;

	//raw data 
	//------------------------------------
	std::ofstream fRaw(filename + fileExtString[bytesPerPixel], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);	
		
	if (bytesPerPixel == 2){
		//maxColorValue for 16 Bit dynamic range
		maxColorValue_source = maxColor_16Bit;
		//Header for 16 Bit PGM
		fRaw << "P2\n" << width << "\n" << height << "\n" << maxColorValue_source << "\n";			
	}	
	else if ((bytesPerPixel == 3) || (bytesPerPixel == 4)){
		//maxColorValue for 24 bit dynamic range in 32 bit array
		maxColorValue_source = maxColor_24Bit;
		//no header für 32 bit text image file			
	}		
	
	int maxVal = 0;
	int minVal = maxColorValue_source;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; ++x)	
		{	
			switch (bytesPerPixel) 
			{			
			case 2:
					*imgBuf = *shortData;
					shortData++;						
					break;
			case 3:					
					*imgBuf = charData[0] + (charData[1] << 8) + (charData[2] << 16);
					charData += bytesPerPixel;
					break;
			case 4:
					*imgBuf = *longData;
					longData++;					
					break;
			default:
					break;
			}
			
			//get min\max for 8bit scaled image
			if (maxVal < *imgBuf)
			{
				maxVal = *imgBuf;
			}
			
			if (minVal > *imgBuf)
			{
				minVal = *imgBuf;
			}

			//write to file
			fRaw << *imgBuf << " ";

			imgBuf++;
		}
		fRaw << "\n";
	}				
	fRaw.close();
		

	//write 8 bit scaled data 
	//------------------------------------
	imgBuf = imgBufStart;	
	double maxColorValue_dest = maxColor_8Bit;

	std::ofstream fScaled(filename + fileExtString[1], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	
	fScaled << "P2\n" << width << "\n" << height << "\n" << maxColorValue_dest << "\n";
	
	if (maxColorValue_dest != 0)
	{
		double stepSize = (maxVal - minVal) / maxColorValue_dest;

		if (stepSize != 0)
		{			
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; ++x)	
				{					
					fScaled << floor((*imgBuf - minVal) / stepSize) << " ";
					imgBuf++;
				}
				fScaled << "\n";
			}

		}

	}	
	fScaled.close();

	imgBuf = imgBufStart;
	delete[]imgBuf;
}

//----------------------------------------------------------------------------------------------------

void printLastCameraStatus(int cameraStatus)
{	
	std::cout << "Status: ";
	switch (cameraStatus)
	{
	case 0:	std::cout << "OK";
			break;
	case 1:	std::cout << "No Connection";
			break;
	case 2:	std::cout << "Error opening USB device";
			break;
	case 3:	std::cout << "Error while writing to camera";
			break;
	case 4:	std::cout << "Error while reading from camera";
			break;
	case 5:	std::cout << "External trigger timed out";
			break;
	case 6:	std::cout << "New camera connected";
			break;
	case 7:	std::cout << "Unknown camera";
			break;
	case 8:	std::cout << "Parameter out of range";
			break;
	case 9:	std::cout << "no image acquired";
			break;
	case 10: std::cout << "camera busy";
			break;
	case 11: std::cout << "Cooling turned off";
			break;
	case 12: std::cout << "acquisition stopped";
			break;
	case 13: std::cout << "burst mode settings result in too large image";
			break;
	case 14: std::cout << "read out speed not available";
			break;
	case 15: std::cout << "not critical error";
		break;
	case 16: std::cout << "illegal combination of crop and binning";
		break;
	default: std::cout << "unknown error";
			break;

	}
	std::cout << std::endl;
				
}

//----------------------------------------------------------------------------------------------------

void waitForReturn(){	
	char inVal;
	std::cout << "press enter" << std::endl;
	inVal = cin.get();
}

//----------------------------------------------------------------------------------------------------

bool ExitOnError(bool retVal, char* functionName){
		
	if (retVal == true){
		return true;
	}
		
	std::cout << "function " << functionName << " returned error"  << std::endl;

	if (functionName == "ConnectToSingleCameraServer()")
		std::cout << "can not connect to server" << std::endl;
	else
		printLastCameraStatus(lastStatus);

	if (lastStatus == 15){
		return true;
	}

	if (lastStatus == 16){
		return true;
	}

	waitForReturn();
	exit(0);
	return false;

}

//----------------------------------------------------------------------------------------------------

bool WaitWhileCameraBusy(int cameraAddr){

	int counter = 0;
	while (DllIsBusy(cameraAddr))
	{			
		if ((counter++ % 1000000) == 0)
		{
			std::cout << "." << std::flush;
		}
	}
	std::cout << std::endl;
	return true;

}

//----------------------------------------------------------------------------------------------------

int main(int argc, char * argv[])
{			
	int modelID = 0;
	char model[64];
	char *modelPtr = model;
	const int cameraAddr = 0;
	int bytesPerPixel = 2;

	bool sensorSupportsBinX = false;
	bool sensorSupportsCropX = false;

	int sensor_defaultWidth = 0;
	int sensor_defaultHeight = 0;

	int numberOfCamsConnected = 0;	

	char* supportBinningXString = "not supported";
	char* supportCropXString = "not supported";

	const char* connectionTypeString[4] = { "USB", "", "", "Ethernet" };

	/****************************************
	* 1. connect to camera
	* **************************************/
			
	//set connectionType
	ExitOnError(SetupCameraInterface(connectionType, ip, lastStatus, cameraAddr), "SetupCameraInterface()");
	std::cout << "camera interface set: " << connectionTypeString[connectionType] << std::endl;

	//ethernet: connect to single camera server (camera with ethernet interface)
	if (connectionType == connectionType_Ethernet)
	{		
		std::cout << "ip set: " << ip << std::endl;
		std::cout << std::endl;
		
		std::cout << "try to connect to the camera server ..." << std::endl;
		ExitOnError(ConnectToSingleCameraServer(cameraAddr), "ConnectToSingleCameraServer()");
		std::cout << "camera server: connected" << std::endl;

		numberOfCamsConnected = 1;
	}

	//USB: Get number of devices connected to the pc. 
	//It can take up to 15 seconds until the system provides the device. 
	else 
	{	
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
		std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
		while ((elapsed.count() <= connectUsbTimeoutMilliseconds) && (numberOfCamsConnected == 0)){
			numberOfCamsConnected = GetNumberOfConnectedCams();
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);			
		}
	}

	//connect to camera; no matter which interface
	if (numberOfCamsConnected > 0)
	{
		std::cout << "device(s) found: " << numberOfCamsConnected << std::endl;
		ExitOnError(ConnectCamera(modelID, modelPtr, lastStatus, cameraAddr), "ConnectCamera()");
		std::cout << "connected to camera " << modelPtr << std::endl;				
		std::cout << std::endl;
	}
	else
	{
		std::cout << "no device found" << std::endl;
		waitForReturn();
 		return -1;
	}	
	
	//initialize camera
	ExitOnError(InitCamera(lastStatus, cameraAddr), "InitCamera()");
	std::cout << "camera initialized " << std::endl;

	//get firmware version
	int firmWareVersion = GetFirmwareVersion(cameraAddr);
	std::cout << "firmware: " << firmWareVersion << std::endl;

	//get default size of image 
	ExitOnError(GetImageSize(sensor_defaultWidth, sensor_defaultHeight, bytesPerPixel, cameraAddr), "GetImageSize()");
	std::cout << "default image size: " << sensor_defaultWidth << " x " << sensor_defaultHeight << std::endl;
	std::cout << "bytes per pixel: " << (bytesPerPixel) << " Byte" << std::endl;	

	//get size of pixel
	int pixelSize = GetSizeOfPixel(cameraAddr);
	std::cout << "pixel size: " << pixelSize << " micrometer" << std::endl;

	//check sensor features
	sensorSupportsBinX = SupportedSensorFeature(sensorFeature_binningX, lastStatus, cameraAddr);			
	if (sensorSupportsBinX)	supportBinningXString = "supported";
	std::cout << "sensor feature binning in x (columns): " << supportBinningXString << std::endl;

	sensorSupportsCropX = SupportedSensorFeature(sensorFeature_cropX, lastStatus, cameraAddr);
	if (sensorSupportsCropX) supportCropXString = "supported";		
	std::cout << "sensor feature crop in x (columns): " << supportCropXString << std::endl;
	std::cout << std::endl;

	/****************************************
	* 2. setup cooling control
	* **************************************/	

	//variables for temperature control
	//temperature/cooling
	int sensorTemperature = 0;
	int backsideTemperature = 0;
	int temperatureLevels = 0;
		
	bool setTemperatureValid = true;

	int minTemperature = 0;
	int maxTemperature = 0;

	//initial setup of temperature control
	ExitOnError(TemperatureControl_Init(coolingHardwareOption, minTemperature, maxTemperature, lastStatus, cameraAddr), "TemperatureControl_Init()");
	std::cout << "temperature control initialized." << std::endl;
	std::cout << "minTemperature: " << minTemperature << " degree Celsius" << std::endl;
	std::cout << "maxTemperature: " << maxTemperature << " degree Celsius" << std::endl;
	std::cout << std::endl;

	if (switchOnTemperatureControl)
	{
		//check setTemperature value
		if (setTemperature > maxTemperature){
			setTemperatureValid = false;
		}
		else if (setTemperature < minTemperature){
			setTemperatureValid = false;
		}

		//set temperature
		if (setTemperatureValid)
		{
			std::cout << "setting temperature: " << setTemperature << " degree Celsius" << std::endl;
			ExitOnError(TemperatureControl_SetTemperature(setTemperature, lastStatus, cameraAddr), "TemperatureControl_SetTemperature()");
		}
		else
		{
			std::cout << "set temperature value (" << setTemperature << " degree Celsius) not valid" << std::endl;
			std::cout << "no temperature set" << std::endl;
		}
		std::cout << std::endl;
	}
	
	

	//readout temperature values ()	
	ExitOnError(TemperatureControl_GetTemperature(0, sensorTemperature, lastStatus, cameraAddr), "TemperatureControl_GetTemperature()");			
	std::cout << "sensor temperature: " << sensorTemperature << " degree Celsius" << std::endl;
		
	ExitOnError(TemperatureControl_GetTemperature(1, backsideTemperature, lastStatus, cameraAddr), "TemperatureControl_GetTemperature()");			
	std::cout << "backside temperature: " << backsideTemperature << " degree Celsius" << std::endl;
	std::cout << std::endl;

	/****************************************
	* 3. setup auto shutter 
	* **************************************/
				
	//configure shutter output
	const int SHUTTER_CLOSED = 0;
	const int SHUTTER_OPEN = 1;
	const int SHUTTER_AUTO = 2;
				
	ExitOnError(SetShutterTimings(shutterOpenTimeMilliseconds, shutterCloseTimeMilliseconds, lastStatus, cameraAddr), "SetShutterTimings()");			
	std::cout << "shutter timings set" << std::endl;
	std::cout << "opentime: " << shutterOpenTimeMilliseconds << " ms" << std::endl;
	std::cout << "closetime: " << shutterCloseTimeMilliseconds << " ms" << std::endl;
			
	ExitOnError(OpenShutter(SHUTTER_AUTO, lastStatus, cameraAddr), "OpenShutter()");
	std::cout << "automatic shutter mode set" << std::endl;
	std::cout << std::endl;

	/****************************************
	* 4. acquisition of a full frame
	* **************************************/

	//sensor parameters
	int width = 0;
	int height = 0;		

	//set exposure time
	ExitOnError(SetExposure(exposureTimeMilliseconds, lastStatus, cameraAddr), "SetExposure()");	
	std::cout << "exposure time set: " << exposureTimeMilliseconds << " ms" << std::endl;

	//set readout speed
	ExitOnError(SetReadOutSpeed(readoutSpeed, lastStatus, cameraAddr), "SetReadOutSpeed()");
	std::cout << "readout speed set" << std::endl;

	//set bitDepth of incomming data array
	ExitOnError(SetBitDepth(setBytesPerPixel, lastStatus, cameraAddr), "SetBitDepth()");
	std::cout << "bit depth set to: " << (setBytesPerPixel * 8) << " bit" << std::endl;

	//get size of image
	ExitOnError(GetImageSize(width, height, bytesPerPixel, cameraAddr), "GetImageSize()");		
	std::cout << "camera is prepared to acquire a full frame: " << width << " x " << height << std::endl;		

	//allocate memory for image 	
	void *imageBuf = new unsigned char[width * height * bytesPerPixel];	

	//start acquisition
	ExitOnError(StartMeasurement(enableBiasCorrection, enableSyncOutput, enableShutterOutput, useExternalTrigger, triggerTimeoutMilliseconds, lastStatus, cameraAddr), "StartMeasurement()");
	std::cout << "image acquisition started" << std::endl;

	//wait until image acquisition is complete 
	//check DllIsBusy() function
	WaitWhileCameraBusy(cameraAddr);

	//get image data
	ExitOnError(GetMeasurementData_DynBitDepth(imageBuf, lastStatus, cameraAddr), "GetMeasurementData_DynBitDepth()");
	float timeElapsed = GetLastMeasTimeNeeded(cameraAddr);
	std::cout << "image acquisition complete. time elapsed:  " << timeElapsed << " ms" << std::endl;

	writeToFile(imageBuf, width, height, bytesPerPixel, "image_normal");
	std::cout << "image saved to image_normal" << std::endl;
	std::cout << std::endl;

	//delete image buffer
	delete[] imageBuf;
			

	/****************************************
	* 5. acquisition of a binned frame
	* **************************************/
	
	int noBinning = 1;	
	
	if (firmWareVersion < 12){
		noBinning = 0;
	}

	if (sensorSupportsBinX == false){
		binningX = noBinning;
	}

	//setup acquisition parameters 
	ExitOnError(SetBinningMode(binningX, binningY, lastStatus, cameraAddr), "SetBinningMode()");		
	std::cout << "binning set: " << binningX << " x " << binningY << std::endl;		
		
	//get size of image
	ExitOnError(GetImageSize(width, height, bytesPerPixel, cameraAddr), "GetImageSize()");
	std::cout << "camera is prepared to acquire a binned frame: " << width << " x " << height << std::endl;		

	//allocate memory for image
	imageBuf = new unsigned char[width * height * bytesPerPixel];

	//start acquisition
	ExitOnError(StartMeasurement(enableBiasCorrection, enableSyncOutput, false, useExternalTrigger, triggerTimeoutMilliseconds, lastStatus, cameraAddr), "StartMeasurement()");
	std::cout << "image acquisition started (binning enabled)" << std::endl;
		
	//wait until image acquisition is complete 
	//check DllIsBusy() function
	WaitWhileCameraBusy(cameraAddr);

	//get image data
	ExitOnError(GetMeasurementData_DynBitDepth(imageBuf, lastStatus, cameraAddr), "GetMeasurementData_DynBitDepth()");
	timeElapsed = GetLastMeasTimeNeeded(cameraAddr);
	std::cout << "image acquisition (binning) complete. time elapsed:  " << timeElapsed << " ms" << std::endl;

	writeToFile((imageBuf), width, height, bytesPerPixel, "image_binning");
	std::cout << "image saved to image_binning" << std::endl;
	std::cout << std::endl;

	//delete image buffer
	delete[] imageBuf;	

	//turn off binning
	ExitOnError(SetBinningMode(noBinning, noBinning, lastStatus, cameraAddr), "SetBinningMode()");
	std::cout << "turn off binning" << std::endl;
	

		
	/****************************************
	* 6. acquisition cropped frame
	* *************************************/		

	bool enableCropMode = true;					

	if (sensorSupportsCropX == false){
		cropX = sensor_defaultWidth; 
	}
	
	//setup and activate crop mode			
	ExitOnError(SetupCropMode2D(cropX, cropY, lastStatus, cameraAddr), "SetupCropMode2D()");	
	ExitOnError(ActivateCropMode(enableCropMode, lastStatus, cameraAddr) >= 0, "ActivateCropMode()");	
	
	std::cout << "crop mode set: " << cropX << " x " << cropY << std::endl;		

	//get size of image
	ExitOnError(GetImageSize(width, height, bytesPerPixel, cameraAddr), "GetImageSize()");
	std::cout << "camera is prepared to acquire a cropped frame: " << width << " x " << height << std::endl;		
			
	//allocate memory for image
	imageBuf = new unsigned char[width * height *bytesPerPixel];

	//start acquisition
	ExitOnError(StartMeasurement(enableBiasCorrection, enableSyncOutput, enableShutterOutput, useExternalTrigger, triggerTimeoutMilliseconds, lastStatus, cameraAddr), "StartMeasurement()");
	std::cout << "image acquisition started (with crop mode enabled)" << std::endl;

	//wait until image acquisition is complete 
	//check DllIsBusy() function
	WaitWhileCameraBusy(cameraAddr);

	//get image data
	ExitOnError(GetMeasurementData_DynBitDepth(imageBuf, lastStatus, cameraAddr), "GetMeasurementData_DynBitDepth()");
	timeElapsed = GetLastMeasTimeNeeded(cameraAddr);
	std::cout << "image acquisition (cropped) complete. time elapsed:  " << timeElapsed << " ms" << std::endl;

	writeToFile((imageBuf), width, height, bytesPerPixel, "image_crop");
	std::cout << "image saved to image_crop" << std::endl;
	std::cout << std::endl;

	//delete image buffer
	delete[] imageBuf;
				
	ExitOnError(ActivateCropMode(false, lastStatus, cameraAddr), "ActivateCropMode()");
	std::cout << "turn off crop mode" << std::endl;					
	
	/****************************************
	* 7. acquisition burst mode
	* **************************************/
	
	//auto shutter in burst mode is not supported
	// --> disable auto shutter mode		
	ExitOnError(OpenShutter(SHUTTER_OPEN, lastStatus, cameraAddr), "OpenShutter()");
	std::cout << "auto shutter disabled and shutter opened" << std::endl;
	
	//burst mode parameters
	bool enableBurstMode = true;	
	
	//setup and activate burst mode		
	ExitOnError(SetupBurstMode(numberOfMeasurements, lastStatus, cameraAddr), "SetupBurstMode()");		
	ExitOnError(ActivateBurstMode(enableBurstMode, lastStatus, cameraAddr) > 0, "ActivateBurstMode()");
	std::cout << "burst mode number of measurements set: " << numberOfMeasurements << std::endl;			
			
	//get size of image
	ExitOnError(GetImageSize(width, height, bytesPerPixel, cameraAddr), "GetImageSize()");
	std::cout << "camera is prepared to acquire burst frames: " << width << " x " << height << std::endl;
		
	//allocate memory for image
	imageBuf = new unsigned char[width * height *bytesPerPixel];

	//start acquisition
	ExitOnError(StartMeasurement(enableBiasCorrection, enableSyncOutput, enableShutterOutput, useExternalTrigger, triggerTimeoutMilliseconds, lastStatus, cameraAddr), "StartMeasurement()");
	std::cout << "image acquisition started (burst)" << std::endl;

	//wait until image acquisition is complete 
	//check DllIsBusy() function
	WaitWhileCameraBusy(cameraAddr);

	//get image data
	ExitOnError(GetMeasurementData_DynBitDepth(imageBuf, lastStatus, cameraAddr), "GetMeasurementData_DynBitDepth()");
	timeElapsed = GetLastMeasTimeNeeded(cameraAddr);
	std::cout << "image acquisition (brust) complete. time elapsed:  " << timeElapsed << " ms" << std::endl;

	writeToFile((imageBuf), width, height, bytesPerPixel, "image_burst");
	std::cout << "image saved to image_burst" << std::endl;
	std::cout << std::endl;

	//delete image buffer
	delete[] imageBuf;		
	
	/**************************************************
	* 8. acquisition of binned cropped frames in a burst 
	**************************************************/
		
	//setup and activate burst mode		
	ExitOnError(SetupBurstMode(numberOfMeasurements, lastStatus, cameraAddr), "SetupBurstMode()");
	ExitOnError(ActivateBurstMode(enableBurstMode, lastStatus, cameraAddr) > 0, "ActivateBurstMode()");
	std::cout << "burst mode number of measurements set: " << numberOfMeasurements << std::endl;
			
	//setup and activate crop mode
	ExitOnError(SetupCropMode2D(cropX, cropY, lastStatus, cameraAddr), "SetupCropMode2D()");
	ExitOnError(ActivateCropMode(enableCropMode, lastStatus, cameraAddr) >= 0, "ActivateCropMode()");
	std::cout << "crop mode set: " << cropX << " x " << cropY << std::endl;

	//setup binning parameters  (binning of a cropped image is not supported for firmware < 12)	
	ExitOnError(SetBinningMode(binningX, binningY, lastStatus, cameraAddr), "SetBinningMode()");
	if (lastStatus != Message_IllegalCombinationBinCrop){
		std::cout << "binning set: " << binningX << " x " << binningY << std::endl;
	}

	//get size of image
	ExitOnError(GetImageSize(width, height, bytesPerPixel, cameraAddr), "GetImageSize()");
	std::cout << "camera is prepared to acquire burst frames: " << width << " x " << height << std::endl;

	//allocate memory for image
	imageBuf = new unsigned char[width * height * bytesPerPixel];

	//start acquisition
	ExitOnError(StartMeasurement(enableBiasCorrection, enableSyncOutput, enableShutterOutput, useExternalTrigger, triggerTimeoutMilliseconds, lastStatus, cameraAddr), "StartMeasurement()");
	std::cout << "image acquisition started (binned cropped burst)" << std::endl;

	//wait until image acquisition is complete 
	//check DllIsBusy() function
	WaitWhileCameraBusy(cameraAddr);

	//get image data
	ExitOnError(GetMeasurementData_DynBitDepth(imageBuf, lastStatus, cameraAddr), "GetMeasurementData_DynBitDepth()");
	timeElapsed = GetLastMeasTimeNeeded(cameraAddr);
	std::cout << "image acquisition (binned cropped burst) complete. time elapsed:  " << timeElapsed << " ms" << std::endl;

	writeToFile((imageBuf), width, height, bytesPerPixel, "image_binned_cropped_burst");
	std::cout << "image saved to image_binned_cropped_burst" << std::endl;
	std::cout << std::endl;

	//delete image buffer
	delete[] imageBuf;	
	
	/****************************************
	* 9. disconnect camera
	* **************************************/

	//readout temperature values again
	ExitOnError(TemperatureControl_GetTemperature(0, sensorTemperature, lastStatus, cameraAddr), "TemperatureControl_GetTemperature()");
	std::cout << "sensor temperature: " << sensorTemperature << " degree Celsius" << std::endl;
	ExitOnError(TemperatureControl_GetTemperature(1, backsideTemperature, lastStatus, cameraAddr), "TemperatureControl_GetTemperature()");
	std::cout << "backside temperature: " << backsideTemperature << " degree Celsius" << std::endl;

	//turn off cooling
	if (switchOnTemperatureControl)
	{
		std::cout << "switch off temperature control " << std::endl;
		ExitOnError(TemperatureControl_SwitchOff(lastStatus, cameraAddr), "TemperatureControl_SwitchOff()");
		std::cout << std::endl;
	}

	//disconnect camera
	ExitOnError(DisconnectCamera(lastStatus, cameraAddr), "DisconnectCamera()");

	if (connectionType == connectionType_Ethernet)
		ExitOnError(DisconnectCameraServer(cameraAddr), "DisconnectCameraServer()");

	waitForReturn();
	return 0;

}

//----------------------------------------------------------------------------------------------------

