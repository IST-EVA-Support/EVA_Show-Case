# Showcase 4: Components Preparation

## Showcase Requirements

Before installing this showcase to the device, please install ADLINK EVASDK and set the EVA environment as necessary.

## Showcase 4 Installation

The path to the respective demo folder includes install.sh and run.sh for this showcase.

Install.sh will perform the following steps:

1. Build the required plug-ins for this showcase and install them into ADLINK EVASDK.
2. Download the inference intermediate model file and optimize it to TensorRT format on the local device.
3. Download the required showcase demo videos and plug-in setup files.

### For this showcase: 

Use the path:

```
> cd src/demo/part-preparation
```

<!---

For Windows, 

```
> cd src\demo\part-preparation\windows
```

--->

Run install.sh with root privileges:

```
> ./install.sh
```

<!---

For Windows:

```
> install-win.bat
```

--->

This is required to modify the path to the OpenCV library. If you have installed EVA on a non-ADLINK device, please check the requirements in our EVA portal.

This showcase utilizes the DSSD (Deconvolutional Single Shot Detector) and is trained by NVIDIA TAO. Two models which pruned and without pruned are provided. The default used model is model without pruned. If you want to use pruned model,  use the argument below:

```
> ./install.sh -m pruned
```

<!---

This showcase default support NVIDIA Jetson series products. If you want to run under x86 device, use the argument below:

```
> ./install.sh -g x86
```

--->

<!---

For Windows:

```
> install-win.bat yolov3
```

--->

<a id="runsh"></a>

After installation, execute run.sh for the pipeline command:

```
> ./run.sh
```

<!---

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

--->

Or you can open EVA_IDE and load pygraph then execute, please see the section, [Run This Showcase Through EVA IDE](#Run-This-Showcase-Through-EVA-IDE).

Then you will see the pop-up display window of this showcase as in the example below.

![image-showcase1](../../../figures/image-showcase4-correct.png)

In the figure above, "container-parts" is the area where each parts must place inside orderly. The order and the number of each part is already defined in SOP(Standard Operating Procedure). Based on this SOP, the algorithm designed four required parts' order from left to right and their numbers. From left to right 2 light-guide-cover, 2 small-board-side-A, 4 screw and 1 wire orderly placed. The timer starts from the first part placed in the "container-parts" and ends when all the parts placed with correct order and numbers. If the operator place the part with wrong order, the "incorrect order!" message will be displayed(See the figure below).

![image-showcase1](../../../figures/image-showcase4-incorrect.png)

The timer starts with orange color means the preparation procedure is working and ends with green when all parts are ready. This showcase illustrate the procedure of the operator followed the preparation specification in case each part is missed before assemly. Just like the other showcase, once you modified this part-preparation plugin source code, required to rebuild it simply direct to the path [/src/plugins/assembly](/src/plugins/assembly) and run assembly-build.sh for ubuntu system. (Currently not support for windows, windows version of this showcase will provide in later version) The email alert plugin were implemented in python. Once modified the email alert plugin, direct to [/src/plugins/alert/email](/src/plugins/alert/email) and run email-build.sh for ubuntu or email-build.bat for windows 10. The same rebuild procedure for other alert plugins. More detail setting could be found in EVA Portal.

*Modified installation details can be found at the EVA Portal: < Under Construction >

## Training Materials

The training materials can be downloaded with the following links.

Training images: http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase1-2/geo-fencing-training-images.zip 

Training notation for mobilenetSSDv2: http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase1-2/geo-fencing-mobilenetSSDv2.zip

Training notation for yolov3: http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase1-2/geo-fencing-yolov3.zip

Training architecture site list below: 

mobilenetSSDv2: 

https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf1.md

https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf1_detection_zoo.md, ssd_mobilenet_v2_coco

yolov3: 

https://github.com/AlexeyAB/darknet/tree/Yolo_v3

Note: Showcases 4 and 5 use the same training materials.



## Run This Showcase Through EVA IDE(For EVASDK 3.5.2 or later)

In this showcase, you can run the pipeline by execute <a href="#runsh">run.sh</a> but also EVA IDE. Open EVA IDE and make sure your current path is in src/demo/part-preparation as root:

```
> EVA_ROOT/bin/EVA_IDE
```

EVA_ROOT is the path where the EVA is installed, the default installed path is /opt/adlink/eva/. So directly call EVA_IDE:

```
> /opt/adlink/eva/bin/EVA_IDE
```

And you will see the IDE show up as below:

![EVAIDE](../../../figures/EVAIDE.png)

Then select the pygraph you want to run, here for example select showcase1-mobilenetssdv2.pygraph in this showcase folder through File->Load. Then you can see this showcase pipeline:

![showcase1-file-load](../../../figures/showcase1-file-load.png)

![showcase1-pipeline](../../../figures/showcase1-pipeline.png)

The settings are default set relevant to this scenario<a href="#note1"><Note 1></a> and one alert require to be set. Click on the email_alert node in the pipeline and the property window will show the node properties detail at left side. See the figure below:

![emailalert-node](../../../figures/emailalert-node.png) ![emailalert-node-property](../../../figures/emailalert-node-property-showcase1.png)

Provide an email address you want to receive from the alert for this show case in "receiver-address". Then press the play button ![play-button](../../../figures/play-button.png) and you will see the scenario video start to play.

<a id="note1"></a>

<Note 1> If your IDE can not show/add the plugin node "h264parse" after loading the pygraph file, manually add it into the while list. The file element_list.txt will be generated after running IDE once. 

For Linux, add "+h264parse" in file : /home/USER_ACCOUNT/adlink/eva/IDE/config/element_list.txt. 

For Windows 10, add "+h264parse" in file : C:\ADLINK\eva\IDE\config\element_list.txt
