# Welcome to ADLINK EVA Showcase!

In this git repository, you can download the source code of the plug-ins we designed for the showcases. Each showcase demonstrates the pipeline elements you will need. EVASDK can help you to quickly set up and build the applications for your specific implementation.

## Clone the Source Code

Use the following command to clone the source code from GitHub:

```
> git clone git@github.com:IST-EVA-Support/EVA_Show-Case.git
> cd EVA_Show-Case
```

Showcases currently available:

|            | Description                                                  | Path to demo                                     | Categories                |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------ | ------------------------- |
| Showcase 1 | In this showcase you will see how EVA is used to prepare the pipeline for detecting people entering a specific area. Factories commonly have restricted areas to keep workers safe from moving automation or robots. In this showcase you will see how a corridor is designated as a restricted area to prevent people from entering it. If someone enters the restricted area, the region will turn red if the display is set to true, and an alert event will be recorded as metadata for a downstream element to activate. | [src/demo/geofence](src/demo/geofence)           | Geofencing                |
| Showcase 2 | In this showcase you will see how EVA is used to prepare the pipeline for detecting people entering a specific area and checking if the person is wearing a clean room suit. For certain manufacturing processes, a clean room suit is required when entering an environmentally controlled space. In this showcase, geofencing is also used to detect when someone crosses the entrance. The downstream element for clean room suit detection will then be activated to detect if the person is wearing the appropriate clothes before entering the FAB. | [src/demo/weardetection](src/demo/weardetection) | Geofencing/Wear Detection |

Each showcase describes how to install and run the demo. Simply enter the path to each demo showcase you are interested in and run it.

Visit our EVA portal for more information: < Under Construction >

