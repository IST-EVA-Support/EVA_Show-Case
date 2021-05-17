# Welcome to ADLINK EVA Show Case!

In this git repository, you can download the source code of the plug-ins we designed for the scenes. And each show case demonstrate the combination of  the pipeline elements we are going to present for you. EVASDK can help you to quick establish and realize the application for the scenarios you are going to apply. We are glad to help you speeding up the implementation!

## Clone the Source Code

Use the following command to clone the source code from GitHub:

```
> git clone git@github.com:IST-EVA-Support/EVA_Show-Case.git
> cd EVA_Show-Case
```

Currently we provide show cases below:

|             | Description                                                  | Path to demo                                     | Categories               |
| ----------- | ------------------------------------------------------------ | ------------------------------------------------ | ------------------------ |
| Show Case 1 | In this case scene, you will know how EVA used to play the role in prepare the pipeline for detecting people break into the specific area. In real world, some of the area is restricted to prevent people to get in to prevent some operation, human life or equipment safe. In this Scene, you will see the corridor is treated as the imaginary area to prevent people break through. If the break in disclosed, the region will turn red if the display is set to true for visualizing and alert event will recorded in user defined alert-type into meta data for downstream element to active. | [src/demo/geofence](src/demo/geofence)           | Geo-Fence                |
| Show Case 2 | In this case scene, you will know how EVA used to play the role in prepare the pipeline for detecting people break into the specific area and if the person does not wear the clean room suit. While manufacturing, the clean suit is required to wear on. In this scene, geo-fence is also used to detect the people cross the entrance gate. And the downstream element clean suit detection then will also active to detect if the employees who does not follow the SOP to wear clean suit then enter the FAB. | [src/demo/weardetection](src/demo/weardetection) | Geo-Fence/Wear-Detection |

Each show case contains the way to install and run. Just path to each demo show case you are interested and run it simply. 

Visit our EVA portal for more advanced and detailed description: < >

