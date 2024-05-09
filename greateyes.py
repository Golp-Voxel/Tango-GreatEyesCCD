import os
import sys
import numpy as np
from ctypes import *
import cv2
from PIL import Image
import time
import matplotlib.pyplot as plt
import numpy as np


from time import process_time 
np.set_printoptions(threshold=sys.maxsize)

GreatEyesLib = None


def setGreatEyesDLL(libraryDLL):
    global GreatEyesLib
 
    # os.add_dll_directory("D:\\Develop\\msys2\\mingw64\\bin")
    # os.add_dll_directory("D:\\Develop\\msys2\\mingw64\\lib") 
    greateyes_dll = libraryDLL
    GreatEyesLib = windll.LoadLibrary(greateyes_dll)



# 1. Constant
# 1.1 Possible value of statusMSG
#--------------------------------------------------------------------------------------------------------
class statusMSG:
    MESSAGE_Camera_Ok                 = 0;   # camera detected and ok
    MESSAGE_NoCamera_Connected        = 1;   # no camera detected
    MESSAGE_could_not_open_USBDevice  = 2;   # there is a problem with the USB interface
    MESSAGE_WriteConfigTable_Failed   = 3;   # transferring data to cam failed - TimeOut!
    MESSAGE_WriteReadRequest_Failed   = 4;   # receiving data  from cam failed - TimeOut!
    MESSAGE_NoTrigger                 = 5;   # no extern trigger signal within time window of TriggerTimeOut
    MESSAGE_NewCameraDetected         = 6;   # new cam detected - you need to perform CamSettings
    MESSAGE_UnknownCamID              = 7;	 # this DLL was not written for connected cam - please request new greateyes.dll
    MESSAGE_OutofRange                = 8;	 # one ore more parameters are out of range
    Message_NoNewData                 = 9;	 # no new image data
    Message_Busy                      = 10;	 # camera busy
    Message_CoolingTurnedOff          = 11;	 # cooling turned off
    Message_MeasurementStopped        = 12;	 # measurement stopped
    Message_BurstModeTooMuchPixels    = 13;	 # too many pixels for BurstMode. Set lower number of measurements or higher binning level
    Message_TimingTableNotFound       = 14;	 # timing table for selected readout speed not found
    Message_NotCritical               = 15;	 # function stopped but there is no critical error (no valid result; catched division by zero). please try to call function again. 
    Message_IllegalCombinationBinCrop = 16;	 # for firmware < v12 the combination of binning and crop mode is not supported 


# 1.2 Function Constant
#--------------------------------------------------------------------------------------------------------
maxPixelBurstTransfer             = 8823794;

sensorFeature_capacityMode        = 0;
sensorFeature_cropX               = 1;
sensorFeature_binningX            = 2;
sensorFeature_gainSwitch          = 3;


readoutSpeed_50_kHz               = 50;
readoutSpeed_100_kHz              = 100;
readoutSpeed_250_kHz              = 250;
readoutSpeed_500_kHz              = 500;
readoutSpeed_1_MHz                = 1000;
readoutSpeed_3_MHz                = 3000;
readoutSpeed_5_MHz                = 5000;


connectionType_USB                = 0;
connectionType_Ethernet           = 3;


# 1.3 Utils
#--------------------------------------------------------------------------------------------------------
def printLastCameraStatus(cameraStatus):
    print("Status: ", end = " " )
    match cameraStatus:
        case statusMSG.MESSAGE_Camera_Ok:
            print("OK")
        case statusMSG.MESSAGE_NoCamera_Connected:
            print("No Connection")
        case statusMSG.MESSAGE_could_not_open_USBDevice:
            print("Error opening USB device")
        case statusMSG.MESSAGE_WriteConfigTable_Failed:
            print("Error while writing to camera")
        case statusMSG.MESSAGE_WriteReadRequest_Failed:
            print("Error while reading from camera")
        case statusMSG.MESSAGE_NoTrigger:
            print("External trigger timed out")
        case statusMSG.MESSAGE_NewCameraDetected:
            print("New camera connected")
        case statusMSG.MESSAGE_UnknownCamID:
            print("Unknown camera")
        case statusMSG.MESSAGE_OutofRange:
            print("Parameter out of range")
        case statusMSG.Message_NoNewData:
            print("no image acquired")
        case statusMSG.Message_Busy:
            print("camera busy")
        case statusMSG.Message_CoolingTurnedOff:
            print("Cooling turned off")
        case statusMSG.Message_MeasurementStopped:
            print("acquisition stopped")
        case statusMSG.Message_BurstModeTooMuchPixels:
            print("burst mode settings result in too large image")
        case statusMSG.Message_TimingTableNotFound:
            print("read out speed not available")
        case statusMSG.Message_NotCritical:
            print("not critical error")
        case statusMSG.Message_IllegalCombinationBinCrop:
            print("illegal combination of crop and binning")
        case _:
            print("unknown error")
            
    print("\n")


def waitForReturn():	
	inVal = input("press enter")


def ExitOnError(retVal, functionName, lastStatus):
		
	if retVal == True:
		return True;
		
	print(f"function {functionName} returned error")

	if functionName == "ConnectToSingleCameraServer()":
		print(f"can not connect to server");
	else:
		printLastCameraStatus(lastStatus);

	if lastStatus == statusMSG.Message_NotCritical:
		return True;
	
	if lastStatus == statusMSG.Message_IllegalCombinationBinCrop:
		return True;

	waitForReturn()
	sys.exit(0)


def WaitWhileCameraBusy(cameraAddr):
    counter = 0
    while DllIsBusy(cameraAddr):
        if (counter % 1000000) == 0:
            print(".", end = " " )
        counter += 1

    print(f"\n")
    
    return True;


def DisplayImage(inBuf, width, height, bytesPerPixel, filename):
    writetofile = False
    image_array = np.reshape(inBuf,(height,width))
    # f = open("imagebuf_afte_WriteToFile_06_05_1.txt", "w")
    # f.write(str(image_array))
    # f.close()
    plt.imshow(image_array[12+10:][8+10:],cmap="gray")
    plt.colorbar()
    plt.show()           


# 2. Exported DLL Functions
# --------------------------------------------------------------------------------------------------------
# 2.1 Setup camera inteface (USB/Ethernet)
# --------------------------------------------------------------------------------------------------------
def SetupCameraInterface(connectionType, ip, statusMSG, cameraAddr) -> bool:
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _SetupCameraInterface = GreatEyesLib.SetupCameraInterface
    _SetupCameraInterface.argtypes = [c_int, c_char_p, POINTER(c_int), c_int]
    _SetupCameraInterface.restype = c_bool
    arg_ip = c_char_p(ip.encode("utf-8"))
    arg_statusMSG = c_int(statusMSG[0])
    
    # print(f"Last status {arg_statusMSG.value}")
    status = _SetupCameraInterface(connectionType, arg_ip, byref(arg_statusMSG), cameraAddr)
    # print(f"Last status {arg_statusMSG.value}")
    
    # printLastCameraStatus(status)
    # print (f"Status {status}")
    # print(f"Status {status} - ", printLastCameraStatus(status));
    statusMSG[0] = arg_statusMSG.value
    
    return status

    
# 2.2 Connecting to a greateyes camera server 
# --------------------------------------------------------------------------------------------------------

# Necessary for connection via Ethernet only.
def ConnectToMultiCameraServer():
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _ConnectToMultiCameraServer = GreatEyesLib.ConnectToMultiCameraServer
    _ConnectToMultiCameraServer.restype = c_bool
    
    status = _ConnectToMultiCameraServer()
    
    return status
    

def ConnectToSingleCameraServer(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _ConnectToSingleCameraServer = GreatEyesLib.ConnectToSingleCameraServer
    _ConnectToSingleCameraServer.argtypes = [c_int]
    _ConnectToSingleCameraServer.restype = c_bool
    
    status = _ConnectToSingleCameraServer(addr)
    
    return status
    
def DisconnectCameraServer(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _DisconnectCameraServer = GreatEyesLib.DisconnectCameraServer
    _DisconnectCameraServer.argtypes = [c_int]
    _DisconnectCameraServer.restype = c_bool
    
    status = _DisconnectCameraServer(addr)
    
    return status


# 2.3 Connecting to a greateyes camera (USB/Ethernet) 
# --------------------------------------------------------------------------------------------------------

def GetNumberOfConnectedCams():
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetNumberOfConnectedCams = GreatEyesLib.GetNumberOfConnectedCams
    _GetNumberOfConnectedCams.restype = c_int
    
    numCams = _GetNumberOfConnectedCams()
    
    return numCams
    
    
def ConnectCamera(modelID, modelStr, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _ConnectCamera = GreatEyesLib.ConnectCamera
    _ConnectCamera.argtypes = [POINTER(c_int), POINTER(c_char_p), POINTER(c_int), c_int]
    _ConnectCamera.restype = c_bool
    
    arg_modelID = c_int(modelID)
    
    # Convert the Python string to a bytes object
    arg_modelStr_encoded = modelStr.encode("utf-8")

    # Create a ctypes pointer to a char pointer
    arg_modelStr = POINTER(c_char_p)(create_string_buffer(arg_modelStr_encoded))

    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _ConnectCamera(byref(arg_modelID), arg_modelStr, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status
    

def DisconnectCamera(statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _DisconnectCamera = GreatEyesLib.DisconnectCamera
    _DisconnectCamera.argtypes = [POINTER(c_int), c_int]
    _DisconnectCamera.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _DisconnectCamera(byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status
    
    
# 2.4 Initialization of greateyes camera (USB/Ethernet) 
# --------------------------------------------------------------------------------------------------------
def InitCamera(statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _InitCamera = GreatEyesLib.InitCamera
    _InitCamera.argtypes = [POINTER(c_int), c_int]
    _InitCamera.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _InitCamera(byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status  


# 2.5 Set Functions
# --------------------------------------------------------------------------------------------------------

def SetExposure(exposureTime, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _SetExposure = GreatEyesLib.SetExposure
    _SetExposure.argtypes = [c_int, POINTER(c_int), c_int]
    _SetExposure.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _SetExposure(exposureTime, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def SetReadOutSpeed(readOutSpeed, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _SetExposure = GreatEyesLib.SetExposure
    _SetExposure.argtypes = [c_int, POINTER(c_int), c_int]
    _SetExposure.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _SetExposure(readOutSpeed, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def SetShutterTimings(openTime, closeTime, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _SetShutterTimings = GreatEyesLib.SetShutterTimings
    _SetShutterTimings.argtypes = [c_int, c_int, POINTER(c_int), c_int]
    _SetShutterTimings.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _SetShutterTimings(openTime, closeTime, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def OpenShutter(state, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _OpenShutter = GreatEyesLib.OpenShutter
    _OpenShutter.argtypes = [c_int, POINTER(c_int), c_int]
    _OpenShutter.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _OpenShutter(state, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def SetBitDepth(bytesPerPixel, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _SetBitDepth = GreatEyesLib.SetBitDepth
    _SetBitDepth.argtypes = [c_int, POINTER(c_int), c_int]
    _SetBitDepth.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    # print(f"Last status {arg_statusMSG.value}")
    status = _SetBitDepth(bytesPerPixel, byref(arg_statusMSG), addr)
    # print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status
    
    
# 2.6 Get Functions
# --------------------------------------------------------------------------------------------------------
def GetDLLVersion(size):
    if GreatEyesLib == None:
        return False
        
    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetDLLVersion = GreatEyesLib.GetDLLVersion
    _GetDLLVersion.argtypes = [POINTER(c_int)]
    _GetDLLVersion.restype = c_char_p
    
    arg_size = c_int(size[0])
    result = _GetDLLVersion(byref(arg_size))
    
    print(f"Size {arg_size.value}")
    size[0] = arg_size.value
    
    # Convert the result to a Python string
    version = result.decode("utf-8") if result else None

    print (f"Version {version} ")
    
    return version != None   


def GetFirmwareVersion(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetFirmwareVersion = GreatEyesLib.GetFirmwareVersion
    _GetFirmwareVersion.argtypes = [c_int]
    _GetFirmwareVersion.restype = c_int
    
    version = _GetFirmwareVersion(addr)
    
    return version

    
def GetImageSize(width, height, bytesPerPixel, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetImageSize = GreatEyesLib.GetImageSize
    _GetImageSize.argtypes = [POINTER(c_int), POINTER(c_int), POINTER(c_int), c_int]
    _GetImageSize.restype = c_bool
    
    arg_width = c_int(width[0])
    arg_height = c_int(height[0])
    arg_bytesPerPixel = c_int(bytesPerPixel[0])
    
    status = _GetImageSize(byref(arg_width), byref(arg_height), byref(arg_bytesPerPixel), addr)
    
    width[0] = arg_width.value
    height[0] = arg_height.value
    bytesPerPixel[0] = arg_bytesPerPixel.value
    
    # print (f"Status {status}")
    
    return status


def GetSizeOfPixel(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetSizeOfPixel = GreatEyesLib.GetSizeOfPixel
    _GetSizeOfPixel.argtypes = [c_int]
    _GetSizeOfPixel.restype = c_int
    
    size = _GetSizeOfPixel(addr)
    
    return size
    

def DllIsBusy(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _DllIsBusy = GreatEyesLib.DllIsBusy
    _DllIsBusy.argtypes = [c_int]
    _DllIsBusy.restype = c_bool
    
    status = _DllIsBusy(addr)
    
    return status


def GetMaxExposureTime(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetMaxExposureTime = GreatEyesLib.GetMaxExposureTime
    _GetMaxExposureTime.argtypes = [c_int]
    _GetMaxExposureTime.restype = c_int
    
    exposureTime = _GetMaxExposureTime(addr)
    
    return exposureTime
    

def GetMaxBinningX(statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetMaxBinningX = GreatEyesLib.GetMaxBinningX
    _GetMaxBinningX.argtypes = [POINTER(c_int), c_int]
    _GetMaxBinningX.restype = c_int
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    value = _GetMaxBinningX(arg_statusMSG, addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"MaxBinningX {value}")
    
    return value


def GetMaxBinningY(statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetMaxBinningY = GreatEyesLib.GetMaxBinningY
    _GetMaxBinningY.argtypes = [POINTER(c_int), c_int]
    _GetMaxBinningY.restype = c_int
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    value = _GetMaxBinningY(arg_statusMSG, addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"MaxBinningY {value}")
    
    return value

    
def SupportedSensorFeature(feature, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _SupportedSensorFeature = GreatEyesLib.SupportedSensorFeature
    _SupportedSensorFeature.argtypes = [c_int, POINTER(c_int), c_int]
    _SupportedSensorFeature.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _SupportedSensorFeature(feature, arg_statusMSG, addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def GetLastMeasTimeNeeded(addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetLastMeasTimeNeeded = GreatEyesLib.GetLastMeasTimeNeeded
    _GetLastMeasTimeNeeded.argtypes = [c_int]
    _GetLastMeasTimeNeeded.restype = c_float
    
    time = _GetLastMeasTimeNeeded(addr)
    
    # print (f"Time {time}")
    
    return time



# 2.7 Temperature Control Functions
# --------------------------------------------------------------------------------------------------------

def TemperatureControl_Init(coolingHardware, minTemperature, maxTemperature, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _TemperatureControl_Init = GreatEyesLib.TemperatureControl_Init
    _TemperatureControl_Init.argtypes = [c_int, POINTER(c_int),  POINTER(c_int), POINTER(c_int), c_int]
    _TemperatureControl_Init.restype = c_int
    
    arg_minTemperature = c_int(minTemperature[0])
    arg_maxTemperature = c_int(maxTemperature[0])
    arg_statusMSG = c_int(statusMSG[0])
    
    # print(f"Last status {arg_statusMSG.value}")
    status = _TemperatureControl_Init(coolingHardware, byref(arg_minTemperature), byref(arg_maxTemperature), byref(arg_statusMSG), addr)
    # print(f"Last status {arg_statusMSG.value}")
    minTemperature[0] = arg_minTemperature.value
    maxTemperature[0] = arg_maxTemperature.value
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status
    
    
def TemperatureControl_GetTemperature(thermistor, temperature, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _TemperatureControl_GetTemperature = GreatEyesLib.TemperatureControl_GetTemperature
    _TemperatureControl_GetTemperature.argtypes = [c_int, POINTER(c_int),  POINTER(c_int), c_int]
    _TemperatureControl_GetTemperature.restype = c_bool
    
    arg_temperature = c_int(temperature[0])
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _TemperatureControl_GetTemperature(thermistor, byref(arg_temperature), byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    temperature[0] = arg_temperature.value
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def TemperatureControl_SetTemperature(temperature, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _TemperatureControl_SetTemperature = GreatEyesLib.TemperatureControl_SetTemperature
    _TemperatureControl_SetTemperature.argtypes = [c_int, POINTER(c_int), c_int]
    _TemperatureControl_SetTemperature.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _TemperatureControl_SetTemperature(temperature, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def TemperatureControl_SwitchOff(statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _TemperatureControl_SwitchOff = GreatEyesLib.TemperatureControl_SwitchOff
    _TemperatureControl_SwitchOff.argtypes = [POINTER(c_int), c_int]
    _TemperatureControl_SwitchOff.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _TemperatureControl_SwitchOff(byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status
    

# 2.8 Image Acquisition
# --------------------------------------------------------------------------------------------------------

def StartMeasurement_DynBitDepth(correctBias, showSync, showShutter, triggerMode, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _StartMeasurement_DynBitDepth = GreatEyesLib.StartMeasurement_DynBitDepth
    _StartMeasurement_DynBitDepth.argtypes = [c_bool, c_bool, c_bool, c_bool, POINTER(c_int), c_int]
    _StartMeasurement_DynBitDepth.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    print(f"Last status {arg_statusMSG.value}")
    status = _StartMeasurement_DynBitDepth(correctBias, showSync, showShutter, triggerMode, byref(arg_statusMSG), addr)
    print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status


def GetMeasurementData_DynBitDepth(pInDataStart, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _GetMeasurementData_DynBitDepth = GreatEyesLib.GetMeasurementData_DynBitDepth
    _GetMeasurementData_DynBitDepth.argtypes = [c_void_p, POINTER(c_int), c_int]
    _GetMeasurementData_DynBitDepth.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    # print(sys.getsizeof(pInDataStart))
    # print(f"Last status {arg_statusMSG.value}")
    status = _GetMeasurementData_DynBitDepth(pInDataStart, byref(arg_statusMSG), addr)
    # print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status
    # except:
    #     print("OSError: exception: access violation reading 0x00007F1CD6A10FDC")
    #     return "Rossa"




#  3. OBSOLETE FUNCTIONS
# ------------------
#  If you are using one of this functions, it is recommended to update them.
# / Please have a look into the sdk manual.
# --------------------------------------------------------------------------------------------------------

    
def StartMeasurement(correctBias, showSync, showShutter, triggerMode, triggerTimeOut, statusMSG, addr):
    if GreatEyesLib == None:
        return False

    # GreatEyesLib = windll.LoadLibrary(greateyes_dll)

    _StartMeasurement = GreatEyesLib.StartMeasurement
    _StartMeasurement.argtypes = [c_bool, c_bool, c_bool, c_bool, c_int, POINTER(c_int), c_int]
    _StartMeasurement.restype = c_bool
    
    arg_statusMSG = c_int(statusMSG[0])
    
    # print(f"Last status {arg_statusMSG.value}")
    status = _StartMeasurement(correctBias, showSync, showShutter, triggerMode, triggerTimeOut, byref(arg_statusMSG), addr)
    # print(f"Last status {arg_statusMSG.value}")
    statusMSG[0] = arg_statusMSG.value
    
    # print (f"Status {status}")
    
    return status



