# Welcome to ADLINK EVA Showcase!

In this git repository, you can download the source code of the plug-ins we designed for the showcases. Each showcase demonstrates the pipeline elements you will need. EVASDK can help you to quickly set up and build the applications for your specific implementation.

## Clone the Source Code

Use the following command to clone the source code from GitHub:

```
> git clone https://github.com/IST-EVA-Support/EVA_Show-Case.git
> cd EVA_Show-Case
> find . -type f -iname *.sh -exec chmod +x {} \;
```

Showcases currently available:

|            | Description                                                  | Path to demo                                     | Categories                |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------ | ------------------------- |
| Showcase 1 | In this showcase you will see how EVA is used to prepare the pipeline for detecting people entering a specific area. Factories commonly have restricted areas to keep workers safe from moving automation or robots. In this showcase you will see how a corridor is designated as a restricted area to prevent people from entering it. If someone enters the restricted area, the region will turn red if the display is set to true, and an alert event will be recorded as metadata for a downstream element to activate. | [src/demo/geofence](src/demo/geofence)           | Geofencing                |
| Showcase 2 | In this showcase you will see how EVA is used to prepare the pipeline for detecting people entering a specific area and checking if the person is wearing a clean room suit. For certain manufacturing processes, a clean room suit is required when entering an environmentally controlled space. In this showcase, geofencing is also used to detect when someone crosses the entrance. The downstream element for clean room suit detection will then be activated to detect if the person is wearing the appropriate clothes before entering the FAB. | [src/demo/weardetection](src/demo/weardetection) | Geofencing/Wear Detection |
| Showcase 3 | In this showcase you will see how EVA is used to prepare the pipeline for detecting abnormal alignment of cookies on the production line. We can use Custom Vision to test the recognition effect of AI. In this demonstration, Custom Vision can be exported into a universal ONNX model, and the EVA can be used to quickly verify the video or the actual shooting results. | [src/demo/cookie-inspection](src/demo/cookie-inspection) | Product Inspection |
| Showcase 4 | In this showcase you will see how EVA is used to prepare the pipeline for monitoring the parts preparation and its order in the parts container. This monitoring is one of the assemble standard operation to prevent missing to assemble parts like screws in the next assemble steps. In additional to this purpose, the preparation time is also the critical fact for the analyst for formulating the optimal assembly standard. Collecting this information extremely help to reflect the true preparation time instead of coming from few simulating samples. | [src/demo/part-preparation](src/demo/part-preparation) | Product Inspection/Preparation |
| Showcase 5 | In this showcase you will see how EVA is used to prepare the pipeline for monitoring the product assembly based on the standard operation procedure. Each parts is required assembled orderly in time. Otherwise the whole procedure will exceed the standard assemble time designed. Each part will calculate its own consuming time and then summed for the analyst to dynamically adjust or designed the whole product assembly standard operation procedure. Ultimately, the convenient combination with showcase 4 for illustrating the whole algorithm plug and play in EVA. (Required to install showcase 4 first.) | [src/demo/part-assembly](src/demo/part-assembly) | Product Inspection/Assembly |

Each showcase describes how to install and run the demo. Simply enter the path to each demo showcase you are interested in and run it.

Visit our EVA portal for more information: < Under Construction >

