# Showcase 2: Wear Detection

## Showcase Requirements

Before installing this showcase case to the device, please install ADLINK EVASDK and set the EVA environment as necessary.

## Showcase 2 Installation

The path to the respective demo folder includes install.sh and run.sh for each showcase.

Install.sh will perform the following steps:

1. Build the required plug-ins for each showcase and install them into ADLINK EVASDK.
2. Download the inference intermediate model file and convert it to TensorRT format on the local device.
3. Download the required showcase demo videos and plug-in setup files.
### Make sure execute all [steps](https://github.com/IST-EVA-Support/EVA_Show-Case/tree/dev#clone-the-source-code) before installation.

### For this showcase: 

Use the path:

```
> cd src/demo/weardetection
```

For Windows, 

```
> cd src\demo\geofence\windows
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

Then you will see the pop-up display window of this showcase as in the example below.

![image-showcase2](../../../figures/image-showcase2.png)

The blue area denotes the restrict area which defined in alert-def-area.txt depends on the video. This file provides the defined area with normalized clockwise coordinates x and y each row. Once you modified this weardetection plugin source code, required to rebuild it simply direct to the path [/src/plugins/weardetection](/src/plugins/weardetection) and run weardetection-build.sh for ubuntu system or weardetection-build.bat for windows 10. (windows version of this showcase will provide in later version) The email alert plugin were implemented in python. Once modified the email alert plugin, direct to [/src/plugins/alert/email](/src/plugins/alert/email) and run email-build.sh for ubuntu or email-build.bat for windows 10. (windows version of this showcase will provide in later version). The same rebuild procedure for other alert plugins. More detail setting could be found in EVA Portal.

*Modified installation details can be found at the EVA Portal: < Under Construction >

## Training Materials

The training materials can be downloaded with the following links.

Training images: https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase1-2/geo-fencing-training-images.zip 

Training notation for mobilenetSSDv2: https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase1-2/geo-fencing-mobilenetSSDv2.zip

Training notation for yolov3: https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase1-2/geo-fencing-yolov3.zip

Training architecture sites listed below:

mobilenetSSDv2: 

https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf1.md

https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf1_detection_zoo.md, ssd_mobilenet_v2_coco

yolov3: 

https://github.com/AlexeyAB/darknet/tree/Yolo_v3

Note: Showcases 1 and 2 use the same training materials.



<a id="Run-This-Showcase-Through-EVA-IDE"></a>

## Run This Showcase Through EVA IDE(For EVASDK 3.5.2 or later)

In this showcase, you can run the pipeline by execute <a href="#runsh">run.sh</a> but also EVA IDE. Open EVA IDE and make sure your current path is in src/demo/weardetection as root:

```
> EVA_ROOT/bin/EVA_IDE
```

EVA_ROOT is the path where the EVA is installed, the default installed path is /opt/adlink/eva/. So directly call EVA_IDE:

```
> /opt/adlink/eva/bin/EVA_IDE
```

And you will see the IDE show up as below:

![EVAIDE](../../../figures/EVAIDE.png)

Then select the pygraph you want to run, here for example select showcase2-mobilenetssdv2.pygraph in this showcase folder through File->Load. Then you can see this showcase pipeline:

![showcase2-file-load](../../../figures/showcase2-file-load.png)

![showcase1-pipeline](../../../figures/showcase2-pipeline.png)

The settings are default set relevant to this scenario<a href="#note1"><Note 1></a> and one alert require to be set. Click on the email_alert node in the pipeline and the property window will show the node properties detail at left side. See the figure below:

![emailalert-node](../../../figures/emailalert-node.png) ![emailalert-node-property](../../../figures/emailalert-node-property-showcase2.png)

Provide an email address you want to receive from the alert for this show case in "receiver-address". Then press the play button ![play-button](../../../figures/play-button.png) and you will see the scenario video start to play.

<a id="note1"></a>

<Note 1> If your IDE can not show/add the plugin node "h264parse" or "weardetection" after loading the pygraph file, manually add it into the while list. The file element_list.txt will be generated after running IDE once. 

For Linux, add "+h264parse" or "+weardetection" in file : /home/USER_ACCOUNT/adlink/eva/IDE/config/element_list.txt. 

For Windows 10, add "+h264parse" or "+weardetection" in file : C:\ADLINK\eva\IDE\config\element_list.txt

