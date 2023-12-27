

  
  gst-launch-1.0 filesrc location=bosch.mp4 ! matroskademux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=models/yolov4-416.engine batch=1 device=0 scale=0.008 mean="127 127 127" rgbconv=true engine-id="adrt" query="//" ! adtrans_yolo anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" threshold=0.4 label=models/yolov4-416.names use-sigmoid=True class-num=6 !  toolkit silent = false label=models/yolov4-416.names query="//adrt" ! videoconvert ! ximagesink
 

