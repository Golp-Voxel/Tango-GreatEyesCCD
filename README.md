# GreatEyes CCD - Tango Device Server

This repository contains the driver for controlling a GreatEyes CCD with the Tango Control. After cloning this repository with the following command

```
git clone https://github.com/Golp-Voxel/Tango_GreatEyesCCD.git
```

It is necessary to create the `tango-env` using the following command:

```
python -m venv tango-env
```

After activating it you can install all the models to run this tool by using the command:

```
pip install -r Requirements.txt
```

To complete the installation, it is necessary to copy the `GreatEyes_D.bat` template and change the paths to the installation folder. And the command to run the `...\tango-env\Scripts\activate` script. 

Then copy the `setting.ini` template and fill in the path to the dlls for the GreatEyes CCD.

