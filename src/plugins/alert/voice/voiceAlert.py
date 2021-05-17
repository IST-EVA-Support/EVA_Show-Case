import cv2
# import gst_helper
import gst_cv_helper
import numpy as np

import time
import pyttsx3
import platform

import threading
import gst_admeta as admeta
from gi.repository import Gst, GObject, GLib, GstVideo


def voice_alert(content):
    engine = pyttsx3.init()
    engine.setProperty("rate", 120)

    platformName = platform.system()
    if "Windows" in platformName:
        voices = engine.getProperty("voices")
        engine.setProperty("voice", voices[1].id)
    if "Linux" in platformName:
        engine.setProperty('voice', 'english-us+f3')

    engine.setProperty('volume', 1)
    engine.say(content)
    engine.runAndWait()
    print("<Start voice alert!>")


def gst_video_caps_make(fmt):
    return "video/x-raw, " \
           "format = (string) " + fmt + " , " \
                                        "width = " + GstVideo.VIDEO_SIZE_RANGE + ", " \
                                                                                 "height = " + GstVideo.VIDEO_SIZE_RANGE + ", " \
                                                                                                                           "framerate = " + GstVideo.VIDEO_FPS_RANGE


class VoiceAlert(Gst.Element):
    GST_PLUGIN_NAME = 'voice_alert'

    __gstmetadata__ = ("Voice Alert",
                       "GstElement",
                       "Text to speech when the alert type matched in ADLINK detection result metadata.",
                       "Dr. Paul Lin <paul.lin@adlinktech.com>")

    __gsttemplates__ = (Gst.PadTemplate.new("src",
                                            Gst.PadDirection.SRC,
                                            Gst.PadPresence.ALWAYS,
                                            Gst.Caps.from_string(gst_video_caps_make("{ BGR }"))),
                        Gst.PadTemplate.new("sink",
                                            Gst.PadDirection.SINK,
                                            Gst.PadPresence.ALWAYS,
                                            Gst.Caps.from_string(gst_video_caps_make("{ BGR }"))))

    _sinkpadtemplate = __gsttemplates__[1]
    _srcpadtemplate = __gsttemplates__[0]

    __gproperties__ = {
        "alert-type": (str, "Alert-type", "Alert type string name.", "BreakIn", GObject.ParamFlags.READWRITE),
        "speech-content": (str, "Speech-content", "Content of the speech", "", GObject.ParamFlags.READWRITE)}

    def __init__(self):
        # Initialize properties before Base Class initialization
        self.alertType = ""
        self.speechContent = "Warning Alert"
        self.send_time = time.time()
        self.alert_duration = 5

        super(VoiceAlert, self).__init__()

        self.sinkpad = Gst.Pad.new_from_template(self._sinkpadtemplate, 'sink')
        self.sinkpad.set_chain_function_full(self.chainfunc, None)
        self.sinkpad.set_chain_list_function_full(self.chainlistfunc, None)
        self.sinkpad.set_event_function_full(self.eventfunc, None)
        self.add_pad(self.sinkpad)

        self.srcpad = Gst.Pad.new_from_template(self._srcpadtemplate, 'src')
        self.srcpad.set_event_function_full(self.srceventfunc, None)
        self.srcpad.set_query_function_full(self.srcqueryfunc, None)
        self.add_pad(self.srcpad)

    def do_get_property(self, prop: GObject.GParamSpec):
        if prop.name == 'alert-type':
            return self.alertType
        elif prop.name == 'speech-content':
            return self.speechContent
        else:
            raise AttributeError('unknown property %s' % prop.name)

    def do_set_property(self, prop: GObject.GParamSpec, value):
        if prop.name == 'alert-type':
            self.alertType = str(value)
        elif prop.name == 'speech-content':
            self.speechContent = str(value)
        else:
            raise AttributeError('unknown property %s' % prop.name)

    def chainfunc(self, pad: Gst.Pad, parent, buff: Gst.Buffer) -> Gst.FlowReturn:
        boxes = admeta.get_detection_box(buff, 0)

        with boxes as det_box:
            if det_box is not None:
                for box in det_box:
                    metaString = box.meta.decode('utf-8')
                    #print(metaString)
                    if self.alertType in metaString:  # Check if this is alert type
                        if time.time() - self.send_time > self.alert_duration:  # Check if out of duration
                            # print("In python element, meta information = ", metaString)
                            self.send_time = time.time()
                            voice_thread = threading.Thread(target=voice_alert,
                                                            args=(self.speechContent,))
                            voice_thread.start()
        return self.srcpad.push(buff)

    def chainlistfunc(self, pad: Gst.Pad, parent, buff_list: Gst.BufferList) -> Gst.FlowReturn:
        return self.srcpad.push(buff_list.get(0))

    def eventfunc(self, pad: Gst.Pad, parent, event: Gst.Event) -> bool:
        return self.srcpad.push_event(event)

    def srcqueryfunc(self, pad: Gst.Pad, parent, query: Gst.Query) -> bool:
        return self.sinkpad.query(query)

    def srceventfunc(self, pad: Gst.Pad, parent, event: Gst.Event) -> bool:
        return self.sinkpad.push_event(event)


GObject.type_register(VoiceAlert)
__gstelementfactory__ = (VoiceAlert.GST_PLUGIN_NAME,
                         Gst.Rank.NONE, VoiceAlert)
