# Show Case 1 : Geo-fence

## Pre-request of The show cases

Before installing this show case to the device, please install ADLINK EVASDK and set the EVA environment respectively.

## Installation of The Show Case 1

Path to the demo folder you are interested and will see the install.sh and run.sh for each show case.

The install.sh will do the following steps:

1. Build each show case required plug-ins and install them into ADLINK EVASDK.
2. Download the inference intermediate model file and convert it to the TensorRT based on the local device.
3. Download each show case demo videos and plug-in required setup files.

### For this show case: 

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

![image-showcase1](../../../figures/image-showcase1.png)

## Training Materials

The training materials can be downloaded here:

training images: https://adlinkdxstorage.blob.core.windows.net/file/geo-fencing-training-images.zip

training notation for mobilenetSSDv2: https://adlinkdxstorage.blob.core.windows.net/file/geo-fencing-mobilenetSSDv2.zip

training notation for yolov3: https://adlinkdxstorage.blob.core.windows.net/file/geo-fencing-yolov3.zip

Training architecture site list below: 

mobilenetSSDv2: https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf1_detection_zoo.md , ssd_mobilenet_v2_coco

yolov3: https://github.com/AlexeyAB/darknet/tree/Yolo_v3

Note: Show case 1 and show case 2 use the same training materials.

*All the detail installation modification could be reference to the EVA Portal here: < Under Construction >