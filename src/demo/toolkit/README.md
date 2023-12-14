# Showcase 1: Toolkit

## Showcase Requirements

Before installing this showcase to the device, please install ADLINK EVASDK and set the EVA environment as necessary.

## Showcase 1 Installation

The path to the respective demo folder includes install.sh and run.sh for each showcase.

Install.sh will perform the following steps:

1. Build the required plug-ins for each showcase and install them into ADLINK EVASDK.
2. Download the inference intermediate model file and convert it to TensorRT format on the local device.
3. Download the required showcase demo videos and plug-in setup files.
### Make sure execute all [steps](https://github.com/IST-EVA-Support/EVA_Show-Case/tree/dev#clone-the-source-code) before installation.

### For this showcase: 

Use the path:

```
> cd src/demo/toolkit
```

For Windows, 

```
> cd src\demo\toolkit\windows
```

Run install.sh with root privileges:

```
> ./install.sh
```

For Windows:

```
> install-win.bat
```

This is required to modify the path to the OpenCV library. If you have installed EVA on a non-ADLINK device, please check the requirements in our EVA portal.

The mobilenetssdv2 is the default installed, if want to install yolov3, use the argument below:

```
> ./install.sh yolov3
```

For Windows:

```
> install-win.bat yolov3
```

<a id="runsh"></a>

After installation, execute run.sh for the pipeline command:

```
> ./run.sh
```

For Windows:

```
> run-win.bat
```

The mobilenetssdv2 is the default run, if yolov3 is installed, use the argument below:

```
> ./run.sh yolov3
```

For Windows:

```
> run-win.bat yolov3
```



Or you can open EVA_IDE and load pygraph then execute, please see the section, [Run This Showcase Through EVA IDE](#Run-This-Showcase-Through-EVA-IDE).


*Modified installation details can be found at the EVA Portal: https://eva-support.adlinktech.com/



For Linux, add "+h264parse" in file : /home/USER_ACCOUNT/adlink/eva/IDE/config/element_list.txt. 

For Windows 10, add "+h264parse" in file : C:\ADLINK\eva\IDE\config\element_list.txt
