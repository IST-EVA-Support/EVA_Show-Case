# Welcome to ADLINK EVA Show Case!

In this git repository, you can download the source code of the plug-ins we designed for the scenes.

## Clone the Source Code

use the following command to clone the source code from GitHub:

```
> git clone git@github.com:IST-EVA-Support/EVA_Show-Case.git
> cd EVA_Show-Case
```

Currently we provide show cases below:

|             | Description                                                  | Path to demo           | Categories               |
| ----------- | ------------------------------------------------------------ | ---------------------- | ------------------------ |
| Show Case 1 | In this case scene, you will know how EVA used to play the role in prepare the pipeline for detecting people break into the specific area. In real world, some of the area is restricted to prevent people to get in to prevent some operation, human life or equipment safe. In this Scene, you will see the corridor is treated as the imaginary area to prevent people break through. If the break in disclosed, the region will turn red if the display is set to true for visualizing and alert event will recorded in user defined alert-type into meta data for downstream element to active. | src/demo/geofence      | Geo-Fence                |
| Show Case 2 | In this case scene, you will know how EVA used to play the role in prepare the pipeline for detecting people break into the specific area and if the person does not wear the clean room suit. While manufacturing, the clean suit is required to wear on. In this scene, geo-fence is also used to detect the people cross the entrance gate. And the downstream element clean suit detection then will also active to detect if the employees who does not follow the SOP to wear clean suit then enter the FAB. | src/demo/weardetection | Geo-Fence/Wear-Detection |

## Pre-request of The show cases

Before installing each show case to the device, please install ADLINK EVASDK and set the EVA environment respectively.

## Installation of The Show Case

Path to the demo folder you are interested and will see the install.sh and run.sh for each show case.

The install.sh will do the following steps:

1. Build each show case required plug-ins and install them into ADLINK EVASDK.
2. Download the inference intermediate model file and convert it to the TensorRT based on the local device.
3. Download each show case demo videos and plug-in required setup files.

### Show Case 1: 

Path to the path:

```
> cd src/demo/geofence
```

Run the install.sh with root privilege:

```
> sudo ./install.sh
```

This required to modify the path to OpenCV library in advance, if you install EVA on non-ADLINK device please check the modification in our EVA portal.

After installation, execute the run.sh for pipeline command:

```
> ./run.sh
```

You can directly see the pop up display window of this show case as below snapshot:

![image-showcase1](figures/image-showcase1.png)

### Show Case 2:

Path to the path:

```
> cd src/demo/weardetection
```

Run the install.sh with root privilege:

```
> sudo ./install.sh
```

This required to modify the path to OpenCV library in advance, if you install EVA on non-ADLINK device please check the modification in our EVA portal

After installation, execute the run.sh for pipeline command:

```
> ./run.sh
```

You can directly see the pop up display window of this show case as below snapshot:

![image-showcase2](figures/image-showcase2.png)



*All the detail installation modification could be reference to the EVA Portal here: < Under Construction >

