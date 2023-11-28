# Showcase 3: Cookie inspection

## Showcase Requirements

Before installing this showcase to the device, please install ADLINK EVASDK and set the EVA environment as necessary.

## Showcase 3 Installation

The path to the respective demo folder includes install.sh and run.sh for each showcase.

Install.sh will perform the following steps:

1. Download the demo video.
2. Download the inference ONNX model file and label file.
3. Download the required showcase demo videos and plug-in setup files.
### Make sure execute all [steps](https://github.com/IST-EVA-Support/EVA_Show-Case/tree/dev#clone-the-source-code) before installation.

### For this showcase: 

Use the path:

```
> cd src/demo/cookie-inspection
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




Or you can open EVA_IDE and load pygraph then execute, please see the section, [Run This Showcase Through EVA IDE](#Run-This-Showcase-Through-EVA-IDE).

Then you will see the pop-up display window of this showcase as in the example below.

![image-showcase3](../../../figures/image-showcase3.png)

*Modified installation details can be found at the EVA Portal: https://eva-support.adlinktech.com/

## Training Materials

The training materials can be downloaded with the following links.

Training images: https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase3/cookie-training-images.zip 

Training notation for ONNX: https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/training/showcase3/cookie-training-notation.zip 

Training architecture site list below: 

Azure Custom Vision: 

https://www.customvision.ai/

Sample scripts for exported models from Custom Vision

https://github.com/Azure-Samples/customvision-export-samples

Sample script for CustomVision's ONNX Object Detection model

https://github.com/Azure-Samples/customvision-export-samples/tree/main/samples/python/onnx/object_detection

*Sign up the Azure Service can be found at this page 
https://azurelessons.com/create-azure-free-account/#Create_Azure_Free_Account

*Creat new project in Custom Vision can be found at the EVA Portal: https://eva-support.adlinktech.com/

<a id="Run-This-Showcase-Through-EVA-IDE"></a>
