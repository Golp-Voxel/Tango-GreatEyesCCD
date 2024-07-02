import os
from datetime import datetime
from PIL import Image
import numpy as np
from ctypes import *
import copy
import json
import time
from threading import Thread
import cv2
import threading
from Code.greateyes import *


import tango
from tango import AttrQuality, AttrWriteType, DevState, DispLevel, AttReqType, Database
from tango.server import Device, attribute, command
from tango.server import class_property, device_property

import configparser


config_info = configparser.ConfigParser()
config_info.read('setting.ini')


DLL_Location = str(config_info['DEFAULT']['DLL'])+"\\greateyes.dll" 


np.set_printoptions(threshold=sys.maxsize)

# ----------------------------------------------------------------------------------------------------
# /****************************************
# * 0. parameter setup
# ****************************************/

# camera interface 
# use connectionType_Ethernet or connectionType_USB
connectionType = connectionType_USB;

# ip adress necessary if connectionType = connectionType_Ethernet
ip = "192.168.1.233";

# exposure time in ms
exposureTimeMilliseconds = 10;

# pixel clock
readoutSpeed = readoutSpeed_1_MHz;

# set bpp for cameras with 18 Bit adc
setBytesPerPixel = 4;

# change number to option provided with sdk	
coolingHardwareOption = 42223;

# set temperature for temperature control in degree Celsius
switchOnTemperatureControl = False;
setTemperature             = 15;

# shutter timings in SHUTTER_AUTO mode
shutterOpenTimeMilliseconds  = 25;
shutterCloseTimeMilliseconds = 25;

# binning parameter
binningX = 2;
binningY = 2;

# crop parameter
cropY = 35;
cropX = 512;

# burst parameter
numberOfMeasurements = 3;

# bias correction 
enableBiasCorrection = False;

# configuration of which camera TTL inputs/outputs to use (is used later when starting acquisition)
enableSyncOutput           = True;
enableShutterOutput        = True;    #only affect on enabled auto shutter 
useExternalTrigger         = False;
triggerTimeoutMilliseconds = 10000;

# In case of using USB interface it can take up to 15 seconds until the system provides the device. 
connectUsbTimeoutMilliseconds = 15000;


# /****************************************
# /* end of parameter setup
# ****************************************/
# ----------------------------------------------------------------------------------------------------

# by reference var
lastStatus     = [3678]

# by reference var 
sizeDLLVersion = [0]

modelID        = 0
modelStr       = ""
cameraAddr     = 0
bytesPerPixel  = [2]

sensorSupportsBinX  = False
sensorSupportsCropX = False

sensor_defaultWidth = [0]
sensor_defaultHeight = [0]

numberOfCamsConnected = 0

supportBinningXString = "not supported"
supportCropXString    = "not supported"

connectionTypeString = ["USB", "", "", "Ethernet"]



class GreatEyes():

    def ConnectCamera():
        # /****************************************
        # * 1. connect to camera
        # * **************************************/
       
        # global vars
        global lastStatus
        # print(f"Last Status : {lastStatus[0]}")
        global numberOfCamsConnected
        
        # set connectionType
        status = SetupCameraInterface(connectionType, ip, lastStatus, cameraAddr)
        # print(f"Last Status : {lastStatus[0]}")
        ExitOnError(status, "SetupCameraInterface()", lastStatus[0]);
        print(f"camera interface set: {connectionTypeString[connectionType]}")

        # ethernet: connect to single camera server (camera with ethernet interface)
        if connectionType == connectionType_Ethernet:
            print(f"ip set: {ip}\n")
    
            print(f"try to connect to the camera server ...");
            status = ConnectToSingleCameraServer(cameraAddr)
            ExitOnError(status, "ConnectToSingleCameraServer()", lastStatus[0]);
            print(f"camera server: connected")

            numberOfCamsConnected = 1;
        # USB: Get number of devices connected to the pc. 
        # It can take up to 15 seconds until the system provides the device. 
        else: 
            start = datetime.now()
            elapsed = datetime.now() - start
            while (elapsed.total_seconds() * 1000) <= connectUsbTimeoutMilliseconds and numberOfCamsConnected == 0:
                numberOfCamsConnected = GetNumberOfConnectedCams()
                elapsed = datetime.now() - start
                # print(f"Elapsed {elapsed.total_seconds() * 1000}  - {connectUsbTimeoutMilliseconds}")               

        # connect to camera; no matter which interface
        if numberOfCamsConnected > 0:
            print(f"device(s) found: {numberOfCamsConnected}")
            status = ConnectCamera(modelID, modelStr, lastStatus, cameraAddr)
            ExitOnError(status, "ConnectCamera()", lastStatus[0])
            print(f"connected to camera {modelStr}\n")				
        else:
            print(f"no device found")
            #waitForReturn();
            return -1;

        # initialize camera
        status = InitCamera(lastStatus, cameraAddr)
        ExitOnError(status, "InitCamera()", lastStatus[0])
        print(f"camera initialized")

        # get firmware version
        firmWareVersion = GetFirmwareVersion(cameraAddr)
        print(f"firmware: {firmWareVersion}")

        # get default size of image
        status = GetImageSize(sensor_defaultWidth, sensor_defaultHeight, bytesPerPixel, cameraAddr)
        ExitOnError(status, "GetImageSize()", lastStatus[0]);
        print(f"default image size: {sensor_defaultWidth[0]} x {sensor_defaultHeight[0]}")
        print(f"bytes per pixel: {bytesPerPixel[0]} Byte")	

        # get size of pixel
        pixelSize = GetSizeOfPixel(cameraAddr);
        print(f"pixel size: {pixelSize} micrometer")

        # check sensor features
        sensorSupportsBinX = SupportedSensorFeature(sensorFeature_binningX, lastStatus, cameraAddr);			
        if sensorSupportsBinX:
             supportBinningXString = "supported"
        print(f"sensor feature binning in x (columns): {supportBinningXString}")

        sensorSupportsCropX = SupportedSensorFeature(sensorFeature_cropX, lastStatus, cameraAddr);
        if sensorSupportsCropX:
            supportCropXString = "supported";		
        print(f"sensor feature crop in x (columns): {supportCropXString}\n")


    def CoolingSystem():
        # /****************************************
        # * 2. setup cooling control
        # * **************************************/	

        # global vars
        global lastStatus
        # print(f"Last Status : {lastStatus[0]}")
        
        # variables for temperature control
        # temperature/cooling
        numCoolingLevel = 0
        thermistorSensorTemperature = 0
        thermistorBacksideTemperature = 1
        
        sensorTemperature = [0];
        backsideTemperature = [0];
        temperatureLevels = 0;
    
       
        setTemperatureValid = False;

        minTemperature = [0];
        maxTemperature = [0];

        if switchOnTemperatureControl:
            # initial setup of temperature control
            numCoolingLevel = TemperatureControl_Init(coolingHardwareOption, minTemperature, maxTemperature, lastStatus, cameraAddr)
            # ExitOnError(numCoolingLevel, "TemperatureControl_Init()", lastStatus[0]);
            print(f"temperature control initialized.")
            print(f"numCoolingLevel: {numCoolingLevel} cooling levels")
            print(f"minTemperature: {minTemperature[0]} degree Celsius")
            print(f"maxTemperature: {maxTemperature[0]} degree Celsius\n")


        if switchOnTemperatureControl:
            # check setTemperature value
            if setTemperature > maxTemperature[0]:
                setTemperatureValid = False;
        elif setTemperature < minTemperature[0]:
            setTemperatureValid = False;

        # set temperature
        if setTemperatureValid:
            print(f"setting temperature: {setTemperature} degree Celsius")
            status = TemperatureControl_SetTemperature(setTemperature, lastStatus, cameraAddr)
            ExitOnError(status, "TemperatureControl_SetTemperature()", lastStatus[0])
        else:
            print(f"set temperature value ({setTemperature} degree Celsius) not valid")
            print(f"no temperature set")

        print(f"\n")


        # readout temperature values ()
        status = TemperatureControl_GetTemperature(thermistorSensorTemperature, sensorTemperature, lastStatus, cameraAddr)
        ExitOnError(status, "TemperatureControl_GetTemperature()", lastStatus[0]);			
        print(f"sensor temperature: {sensorTemperature[0]} degree Celsius")
        
        status = TemperatureControl_GetTemperature(thermistorBacksideTemperature, backsideTemperature, lastStatus, cameraAddr)
        ExitOnError(status, "TemperatureControl_GetTemperature()", lastStatus[0]);			
        print(f"backside temperature: {backsideTemperature[0]} degree Celsius\n")

    
    def AutoShutter():  
        # /****************************************
        # * 3. setup auto shutter 
        # * **************************************/

        # global vars
        global lastStatus
        # print(f"Last Status : {lastStatus[0]}")
        
        # configure shutter output
        SHUTTER_CLOSED = 0;
        SHUTTER_OPEN = 1;
        SHUTTER_AUTO = 2;
            
        status = SetShutterTimings(shutterOpenTimeMilliseconds, shutterCloseTimeMilliseconds, lastStatus, cameraAddr)    
        ExitOnError(status, "SetShutterTimings()", lastStatus[0]);			
        print(f"shutter timings set")
        print(f"opentime: {shutterOpenTimeMilliseconds} ms")
        print(f"closetime: {shutterCloseTimeMilliseconds} ms")
        
        status = OpenShutter(SHUTTER_AUTO, lastStatus, cameraAddr)
        ExitOnError(status, "OpenShutter()", lastStatus[0]);
        print(f"automatic shutter mode set")
        print(f"\n")


    def AcquisitionFullFrame():
        # /****************************************
        # * 4. acquisition of a full frame
        # * **************************************/

        # sensor parameters
        width = [2048];
        height = [2052];		
        # print(f"bytesPerPixel {bytesPerPixel[0]}  - c_uint8 {c_uint8} ")
        # print(f"bytesPerPixel {bytesPerPixel[0]}  - c_uint16 {c_uint16} ")
        # print(f"bytesPerPixel {bytesPerPixel[0]}  - c_uint32 {c_uint32} ")
        
        # set exposure time
        # status = SetExposure(exposureTimeMilliseconds, lastStatus, cameraAddr)
        # ExitOnError(status, "SetExposure()", lastStatus[0]);	
        # print(f"exposure time set: {exposureTimeMilliseconds} ms")

        # # set readout speed
        # status = SetReadOutSpeed(readoutSpeed, lastStatus, cameraAddr)
        # ExitOnError(status, "SetReadOutSpeed()", lastStatus[0]);
        # print(f"readout speed set")

        # set bitDepth of incomming data array
        status = SetBitDepth(setBytesPerPixel, lastStatus, cameraAddr)
        ExitOnError(status, "SetBitDepth()", lastStatus[0]);
        # print(f"bit depth set to: \({setBytesPerPixel} * 8\) bit")

        # # get size of image
        # status = GetImageSize(width, height, bytesPerPixel, cameraAddr)
        # ExitOnError(status, "GetImageSize()", lastStatus[0]);		
        # print(f"camera is prepared to acquire a full frame: {width[0]} x {height[0]} ")		

        #pixel number
        array_size = width[0] * height[0]#* bytesPerPixel[0]
        # print(f"bytesPerPixel {bytesPerPixel[0]}")
        # print("Array size: "+str(array_size))
        # make the 8bit unsigned integer array type
        
        if bytesPerPixel[0] == 2:
            FARRAY = c_uint16 * array_size
        if bytesPerPixel[0] == 3 or bytesPerPixel[0] == 4:
            FARRAY = c_uint32 * array_size
        else:
            FARRAY = c_uint8 * array_size
            
        
        imageBuf = FARRAY(0)
        
        # allocate memory for image 	
        #void *imageBuf = new unsigned char[width * height * bytesPerPixel];	

        # start acquisition
        status = StartMeasurement(enableBiasCorrection, enableSyncOutput, enableShutterOutput, useExternalTrigger, triggerTimeoutMilliseconds, lastStatus, cameraAddr)
        ExitOnError(status, "StartMeasurement()", lastStatus[0]);
        # print(f"image acquisition started")

        # wait until image acquisition is complete 
        # check DllIsBusy() function
        WaitWhileCameraBusy(cameraAddr)
        # get image data
        print(imageBuf)
        status = GetMeasurementData_DynBitDepth(imageBuf, lastStatus, cameraAddr)
        ExitOnError(status, "GetMeasurementData_DynBitDepth()", lastStatus[0])
        # print("ROSSA")
        timeElapsed = GetLastMeasTimeNeeded(cameraAddr)
        # print(f"image acquisition complete. time elapsed:  {timeElapsed} ms")

        
        image_array = np.reshape(imageBuf,(height[0],width[0]))
        
        return image_array
        # delete image buffer
        # delete[] imageBuf;
			

    def DisconnectCamara():
        # /****************************************
	    # * 9. disconnect camera
	    # * **************************************/

        # global vars
        global lastStatus
        # print(f"Last Status : {lastStatus[0]}")
        
        # variables for temperature control
        # temperature/cooling
        numCoolingLevel = 0
        thermistorSensorTemperature = 0
        thermistorBacksideTemperature = 1

        sensorTemperature = [0];
        backsideTemperature = [0];
        
	    # readout temperature values again
        status = TemperatureControl_GetTemperature(thermistorSensorTemperature, sensorTemperature, lastStatus, cameraAddr)
        ExitOnError(status, "TemperatureControl_GetTemperature()", lastStatus[0])
        print(f"sensor temperature: {sensorTemperature[0]} degree Celsius")
        
        status = TemperatureControl_GetTemperature(thermistorBacksideTemperature, backsideTemperature, lastStatus, cameraAddr)
        ExitOnError(status, "TemperatureControl_GetTemperature()", lastStatus[0])
        print(f"backside temperature: {backsideTemperature[0]} degree Celsius")

        # turn off cooling
        if switchOnTemperatureControl:
            print(f"switch off temperature control ")
            status = TemperatureControl_SwitchOff(lastStatus, cameraAddr)
            ExitOnError(status, "TemperatureControl_SwitchOff()", lastStatus[0])
            print(f"\n")

        # disconnect camera
        status = DisconnectCamera(lastStatus, cameraAddr)
        ExitOnError(status, "DisconnectCamera()", lastStatus[0])

        if connectionType == connectionType_Ethernet:
            status = DisconnectCameraServer(cameraAddr)
            ExitOnError(status, "DisconnectCameraServer()", lastStatus[0])

        #waitForReturn();
        return 0
        

    def TestDLL():
        GetDLLVersion(sizeDLLVersion)
        
        if (SetupCameraInterface(connectionType, ip, lastStatus, cameraAddr) == False):
            pass

        if (ConnectCamera(modelID, modelStr, lastStatus, cameraAddr) == False):

            # on error - connecting to camera
            # if (connectionType == connectionType_Ethernet)
            # {
            #    DisconnectCameraServer(cameraAddr);
            # }
            print(f"ConnectCamera Failure ({lastStatus[0]}). \nExiting ")
            printLastCameraStatus(lastStatus[0]);
                
        return "Test"




class TestBitfield(Structure):
    _fields_ = [("x", c_uint16, 9),
                ("y", c_uint8, 7),
                ("z", c_uint16, 4)]  

                
def TestCTypesBitfield() -> None:
    bf = TestBitfield(1,255,3)
    print(bf.x, ", ", bf.y, ", ", bf.z)





class GreatEyes_D(Device):
    _my_current = 2.3456
    _my_range = 0.0
    _my_compliance = 0.0
    _output_on = False
    _available_cameras = ""
    CAMARA = None
    my_camera_ready = False
    save_image = []

    host = device_property(dtype=str, default_value="localhost")
    port = class_property(dtype=int, default_value=10000)

    # Tango Device Init function 
    def init_device(self):
        super().init_device()
        self.info_stream(f"Connection details: {self.host}:{self.port}")
        self.set_state(DevState.ON)
        self.info_stream("\r Try to start the GreatEyes Driver \r")
        setGreatEyesDLL(DLL_Location)
        GreatEyes.ConnectCamera()
        
        
        self.set_status("Thorlabs Camara Driver is ON")
        
    
    # Tango Device Delete function     
    def delete_device(self):
        GreatEyes.DisconnectCamara()


    current = attribute(
        label="Current",
        dtype=float,
        display_level=DispLevel.EXPERT,
        access=AttrWriteType.READ_WRITE,
        unit="A",
        format="8.4f",
        min_value=0.0,
        max_value=8.5,
        min_alarm=0.1,
        max_alarm=8.4,
        min_warning=0.5,
        max_warning=8.0,
        fget="get_current",
        fset="set_current",
        doc="the power supply current",
    )

    noise = attribute(
        label="Noise",
        dtype=((float,),),
        max_dim_x=1450,
        max_dim_y=1450,
        fget="get_noise",
    )

    Image_foto = attribute(
        label="Image Greateyes",
        dtype=((int,),),
        #data_format = tango.AttrDataFormat.IMAGE,
        max_dim_x=2100,
        max_dim_y=2100,
        fget="get_image_old",
        #fget="get_image",
    )

    # TakeImage = attribute(
    #     label="Take a Image",
    #     dtype=str,
    #     fget="take_image",
    #     doc="Test to take image",
    # )


    @attribute
    def voltage(self):
        return 10.0

    def get_current(self):
        return self._my_current

    def set_current(self, current):
        print("Current set to %f" % current)
        self._my_current = current

    def get_noise(self):
        a = np.random.random_sample((500, 500))
        print(type(a))
        return a
    
    def get_image_old(self):
        image_buffer_copy = GreatEyes.AcquisitionFullFrame()
        # image_buffer_copy = np.random.random_sample((2052, 2048))
        return image_buffer_copy
    
    def thread_function(self):
        self.save_image = GreatEyes.AcquisitionFullFrame()
        
    
    def take_image(self):
        x = threading.Thread(target=self.thread_function, args=())
        x.start()
        print("Wait before read the image")
        return "Wait"
    
    def get_image(self):
        return self.save_image

    # @command(dtype_in=int, dtype_out=str)
    # def set_expousure_time_us(self,parameter):
    #     self.CAMARA.exposure_time_us = parameter  # set exposure to 1.1 ms
    #     return "CAMARA "+ " was set exposure time "+ str(parameter) +" us\n"
            
    # @command(dtype_in=int, dtype_out=str)      
    # def set_frames_per_trigger_zero_for_unlimited(self,parameter):
    #     self.CAMARA.frames_per_trigger_zero_for_unlimited = parameter  # start camera in continuous mode
    #     return "CAMARA "+ " was set frames per trigger zero or unlimited "+ str(parameter) +"\n"
        
    # @command(dtype_in=int, dtype_out=str)       
    # def set_image_poll_timeout_ms(self,parameter):
    #     self.CAMARA.image_poll_timeout_ms = parameter  # 1 second polling timeout
    #     return "CAMARA "+ " was set image poll timeout "+ str(parameter) +" ms\n"


    @command(dtype_out=str)    
    def get_foto_JSON(self):

        send_JSON = {"Image":self.get_noise().tolist()}
            
        return json.dumps(send_JSON)
   
    @command(dtype_in=bool, dtype_out=bool)
    def output_on_off(self, on_off):
        self._output_on = on_off
        return self._output_on
        
if __name__ == "__main__":
    GreatEyes_D.run_server()
