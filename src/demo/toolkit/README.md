# Showcase 6: Toolkit

## Showcase Requirements

Before installing this showcase to the device, please install ADLINK EVASDK and set the EVA environment as necessary.

## Showcase 6 Installation

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



Run install.sh with root privileges:

```
> ./install.sh
```


This is required to modify the path to the OpenCV library. If you have installed EVA on a non-ADLINK device, please check the requirements in our EVA portal.



<a id="runsh"></a>

After installation, execute run.sh for the pipeline command:

```
> ./run.sh
```

This demo shows how to output the inference result of the model. The output labels are the same that are used during training. If the toolkit is missing it is shown on frame where its missing, additionally the missing and corerct positions are also displayed. 
diplay code meanings:
```
 > lb -> left bottom
 > lm -> left middle
 > lu -> left upper
 > rb -> right bottom
 > rm -> right middle
 > ru -> right upper
```

Or you can open EVA_IDE and load pygraph then execute, please see the section, [Run This Showcase Through EVA IDE](#Run-This-Showcase-Through-EVA-IDE).


*Modified installation details can be found at the EVA Portal: https://eva-support.adlinktech.com/



For Linux, add "+h264parse" in file : /home/USER_ACCOUNT/adlink/eva/IDE/config/element_list.txt. 


