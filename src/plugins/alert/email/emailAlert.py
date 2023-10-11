import cv2
# import gst_helper
import gst_cv_helper
import numpy as np

import time
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
import threading
import gst_admeta as admeta
import adroi
from gi.repository import Gst, GObject, GLib, GstVideo, GstBase


def get_alert_message_from_meta_string(meta_string, alert_type):
    alert_message_list = meta_string.split(",")
    for content in alert_message_list:
        if alert_type in content:
            return content
    return ""



def send_mail_image(txt_msg, sender, pwd, receiver, img):
    content = MIMEMultipart()
    content.attach(MIMEText("Dear subscriber,\n\n Here comes an event alert below:\n " + txt_msg))
    retval, buf = cv2.imencode('.jpg', img)
    content.attach(MIMEImage(buf.tostring(), name='event.jpg'))

    content["Subject"] = "EVA Email Alert Notification"
    content["From"] = sender
    content["To"] = receiver

    smtp = smtplib.SMTP("smtp.gmail.com", 587)
    smtp.ehlo()
    smtp.starttls()
    smtp.login(sender, 'vbcsekwzsxxbgymh')
    status = smtp.send_message(content)
    if status == {}:
        print("<Alert Mail Sent Successfully!>")
    else:
        print("<Alert Mail Sent Fail!>")
    smtp.quit()


def send_mail_txt(txt_msg, sender, pwd, receiver):
    smtp = smtplib.SMTP('smtp.gmail.com', 587)
    smtp.ehlo()
    smtp.starttls()
    from_addr = sender
    to_addr = receiver
    smtp.login(from_addr, pwd)
    msg = "Subject:EVA Email Alert Notification\nDear subscriber,\n\n Here comes an event alert below:\n " + txt_msg
    status = smtp.sendmail(from_addr, to_addr, msg)
    if status == {}:
        print("<Alert Mail Sent Successfully!>")
    else:
        print("<Alert Mail Sent Fail!>")
    smtp.quit()


def gst_video_caps_make(fmt):
    return "video/x-raw, " \
           "format = (string) " + fmt + " , " \
                                        "width = " + GstVideo.VIDEO_SIZE_RANGE + ", " \
                                                                                 "height = " + GstVideo.VIDEO_SIZE_RANGE + ", " \
                                                                                                                           "framerate = " + GstVideo.VIDEO_FPS_RANGE


#class EmailAlert(Gst.Element):
class EmailAlert(GstBase.BaseTransform):
    GST_PLUGIN_NAME = 'email_alert'

    __gstmetadata__ = ("Email Alert",
                       "GstElement",
                       "Send email to user when the alert type matched in ADLINK detection result metadata.",
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
        "sender-address": (str, "Sender-address", "Sender's email address.", "", GObject.ParamFlags.READWRITE),
        "receiver-address": (str, "Receiver-address", "Receiver's email address.", "", GObject.ParamFlags.READWRITE),
        "sender-pwd": (str, "Sender-password", "Sender's email password.", "", GObject.ParamFlags.READWRITE),
        "query": (str, "Query", "ROI query.", "", GObject.ParamFlags.READWRITE),
        "attach-event-image": (bool, "Attach-event-image", "Attach event image to the mail.", True, GObject.ParamFlags.READWRITE)}

    def __init__(self):
        # Initialize properties before Base Class initialization
        self.alertType = ""
        self.senderAddress = "adlink.eva.alert@gmail.com"
        self.receiverAddress = ""
        self.senderPwd = ""
        self.query = ""
        self.attachEventImage = True
        self.alert_duration = 5
        self.send_time = time.time() - self.alert_duration
        self.ADLINK_MESSAGE = "\n\nBest,\nADLINK EMAIL ALERT PLUGIN. https://www.adlinktech.com/"

        super(EmailAlert, self).__init__()

        #self.sinkpad = Gst.Pad.new_from_template(self._sinkpadtemplate, 'sink')
        self.sinkpad.set_chain_function_full(self.chainfunc, None)
        self.sinkpad.set_chain_list_function_full(self.chainlistfunc, None)
        #self.sinkpad.set_event_function_full(self.eventfunc, None)
        #self.add_pad(self.sinkpad)

        #self.srcpad = Gst.Pad.new_from_template(self._srcpadtemplate, 'src')
        #self.srcpad.set_event_function_full(self.srceventfunc, None)
        #self.srcpad.set_query_function_full(self.srcqueryfunc, None)
        #self.add_pad(self.srcpad)

    def do_get_property(self, prop: GObject.GParamSpec):
        if prop.name == 'alert-type':
            return self.alertType
        elif prop.name == 'sender-address':
            return self.senderAddress
        elif prop.name == 'receiver-address':
            return self.receiverAddress
        elif prop.name == 'sender-pwd':
            return self.senderPwd
        elif prop.name == 'query':
            return self.query
        elif prop.name == 'attach-event-image':
            return self.attachEventImage
        else:
            raise AttributeError('unknown property %s' % prop.name)

    def do_set_property(self, prop: GObject.GParamSpec, value):
        if prop.name == 'alert-type':
            self.alertType = str(value)
        elif prop.name == 'sender-address':
            self.senderAddress = str(value)
        elif prop.name == 'receiver-address':
            self.receiverAddress = str(value)
        elif prop.name == 'sender-pwd':
            self.senderPwd = str(value)
        elif prop.name == 'query':
            self.query = str(value)
        elif prop.name == 'attach-event-image':
            self.attachEventImage = bool(value)
        else:
            raise AttributeError('unknown property %s' % prop.name)

    def chainfunc(self, pad: Gst.Pad, parent, buff: Gst.Buffer) -> Gst.FlowReturn:
    #def do_transform_ip(self, buff: Gst.Buffer) -> Gst.FlowReturn:
        if self.query == "": 
            boxes = admeta.get_detection_box(buff, 0)

            with boxes as det_box:
                if det_box is not None:
                    for box in det_box:
                        metaString = box.meta.decode('utf-8')
                        if self.alertType in metaString:  # Check if this is alert type
                            if time.time() - self.send_time > self.alert_duration:  # Check if out of duration
                                metaString = get_alert_message_from_meta_string(metaString, self.alertType)
                                #print("In python element, meta information = ", metaString)
                                self.send_time = time.time()
                                if self.senderPwd == "":
                                    self.senderPwd = "xjdtukuldxunugdl"

                                if not self.attachEventImage:
                                    mail_thread = threading.Thread(target=send_mail_txt,
                                                                args=(metaString + self.ADLINK_MESSAGE,
                                                                        self.senderAddress,
                                                                        self.senderPwd,
                                                                        self.receiverAddress))
                                    mail_thread.start()
                                else:
                                    img = gst_cv_helper.pad_and_buffer_to_numpy(pad, buff, ro=False)
                                    uint8_img = np.uint8(img)
                                    mail_thread = threading.Thread(target=send_mail_image,
                                                                args=(metaString + self.ADLINK_MESSAGE,
                                                                        self.senderAddress,
                                                                        self.senderPwd,
                                                                        self.receiverAddress,
                                                                        uint8_img))
                                    mail_thread.start()
        else:
            qrs = adroi.gst_buffer_adroi_query(hash(buff), self.query)
            if qrs is None or len(qrs) == 0:
                #print("     query is empty from frame meta in email_alert.")
                return self.srcpad.push(buff)
            
            for roi in qrs[0].rois:
                #print("     in email_alert")
                if roi.category == 'box':
                    if len(roi.events) > 0:
                        #print("roi.events > 0")
                        if time.time() - self.send_time > self.alert_duration:  # Check if out of duration
                            #eventString = roi.events[0];
                            relatedEvents = [s for s in roi.events if self.alertType in s]
                            #print("     In emai_alert, event information = ", eventString)
                            #print("     In emai_alert, related events information = ", relatedEvents)
                            
                            for metaString in relatedEvents:
                                self.send_time = time.time()
                                if self.senderPwd == "":
                                    self.senderPwd = "xjdtukuldxunugdl"

                                if not self.attachEventImage:
                                    mail_thread = threading.Thread(target=send_mail_txt,
                                                                args=(metaString + self.ADLINK_MESSAGE,
                                                                        self.senderAddress,
                                                                        self.senderPwd,
                                                                        self.receiverAddress))
                                    mail_thread.start()
                                else:
                                    img = gst_cv_helper.pad_and_buffer_to_numpy(pad, buff, ro=False)
                                    uint8_img = np.uint8(img)
                                    mail_thread = threading.Thread(target=send_mail_image,
                                                                args=(metaString + self.ADLINK_MESSAGE,
                                                                        self.senderAddress,
                                                                        self.senderPwd,
                                                                        self.receiverAddress,
                                                                        uint8_img))
                                    mail_thread.start()
                    
        return self.srcpad.push(buff)
        #return Gst.FlowReturn.OK

    def chainlistfunc(self, pad: Gst.Pad, parent, buff_list: Gst.BufferList) -> Gst.FlowReturn:
        return self.srcpad.push(buff_list.get(0))

    def eventfunc(self, pad: Gst.Pad, parent, event: Gst.Event) -> bool:
        return self.srcpad.push_event(event)

    def srcqueryfunc(self, pad: Gst.Pad, parent, query: Gst.Query) -> bool:
        return self.sinkpad.query(query)

    def srceventfunc(self, pad: Gst.Pad, parent, event: Gst.Event) -> bool:
        return self.sinkpad.push_event(event)


GObject.type_register(EmailAlert)
__gstelementfactory__ = (EmailAlert.GST_PLUGIN_NAME,
                         Gst.Rank.NONE, EmailAlert)
