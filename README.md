##USB Switch Controller with mqtt/wifi for ESP8266 / ESP32

A basic implementation for the software side to make an usb switch automatically detect a computer and switch to it if its the only one.
[ROLINE USB 3.0 Switch 14.01.2312](https://www.amazon.de/ROLINE-Switch-inkl-Kabel-Manuell/dp/B01N7FSTX3)

##Wiring
Please don't forget a small resistor (>1000ohms should work) between the input lines D1, D2, D7, to prevent to much leakage as the ESP is 3.3V and the input lines are 5.0V.

![Attachment Points Hub](https://github.com/SciLor/usbswitchcontroller/blob/master/USBSwitchController/images/BoardAttachmentPoints.jpg?raw=true)
![Attachment Points Node](https://github.com/SciLor/usbswitchcontroller/blob/master/USBSwitchController/images/WiresAttachedNodeMcu.jpg?raw=true)

There may have been an easier solution as there seems to be a type of UART. But this way it is made as an IOT-device with mqtt

[Like my work? Spread the word and donate!](http://www.scilor.com/donate.html)
