# ShowControlButton
ShowControlButton is a simple, single-pushbutton controller in a box. Inside the box: a Feather M4 Express microcontroller, a relay breakout, an Ethernet breakout, and the capability to put a single pushbutton in your entertainment project and have it trigger a contact closure or trigger the contact closure and send a configurable OSC message to your show network.
## Easy to use
If you don't need OSC capabilities, set up is simple: supply power to the box using the Micro USB port on the back of the box, insert a 1/4" TS plug into the contact closure port and connect the other end to your footswitch device, MIDI footswitch controller, PLC input, or other device with a sourcing discrete input, up to 24V DC. BAM! Press the button, the contact closes for five seconds. Bonus: the 10mm diffused blue LED lights up when the contact is closed.

If you want OSC capabilities, the set up takes a few extra steps. You'll need to configure the network interface for your network, then configure the outgoing OSC message. Push the button: OSC message sent **and** contact closed!
## Network Configuration
The default network configuration for the ShowControlButton:

|Field|Setting|
|---|---|
|IP Address|192.168.1.100|
|Subnet Mask|255.255.255.0|
|Gateway|192.168.1.1|
|DNS Server|8.8.8.8|

To change the network settins for the ShowControlButton, connect the device to your laptop or computer with an Ethernet cable using the Ethernet port on the back of the box. Once connected, supply power using the Micro USB port on the back of the box. Ensure that the network interface on your laptop or computer is in the same address range and is using the same subnet mask as the ShowControl Button. Open a web browser and point it to the IP address of the ShowControlButton at port 5000 (e.g. http://192.168.1.100:5000).

> [!IMPORTANT]
> As of this writing, the ShowControlButton only detects a network connection at power up. If it is not connected to a network at power up, it will shut down the network interface and it will not detect the presence of a network later, even if one is connected, until power has been removed and resupplied. Future versions of the firmware should address this issue.

> [!TIP]
> "Uh oh! I changed my network settings at some point and I don't remember them. How do I access the network configuration web page?!" Easy: press and hold the reset button (located inside the small hole on the bottom of the box) for at least five seconds. This should restore the network configuration settings to the default settings. Connect your network cable, supply power, and point your web browser to http://192.168.1.100:5000 and you should be good to go!

## OSC Set up
Configuring the settings for the OSC message you wish to send when the ShowControlButton is pressed is as simple as navigating to the IP address of the device, just as when configuring the network settings. In the second form on the configuration page, you can set the message's destination IP address, the destination port, and the OSC message itself. As with any OSC message, use a blank space to separate any arguments from the address/method and from each other; the code will determine automatically if the argument is an integer, float, or string. Though we have not tested for large numbers of arguments, theoretically you should be able to store quite a few!

> [!IMPORTANT]
> The firmware will interpret any decimal value as a float: "1" is an integer, but "1.0" is a float. If the argument type **must** be an integer, don't include a ".0".

## Troubleshooting ##
The ShowControlButton software has not undergone robust stress testing; typically, if something goes wrong, remove power, count to ten, resupply power then try again. Restore to faculty defaults and reset your network configuration and OSC message configuration, if necessary.

## Parts List ##
- Adafruit Feather M4 Express (https://www.adafruit.com/product/3857)
- Adafruit Relay (Non-latching) Featherwing (https://www.adafruit.com/product/2895)
- Adafruit Ethernet Featherwing (https://www.adafruit.com/product/3201)
- 10mm Diffused Blue LED (https://www.adafruit.com/product/847)
- 10mm LED Bevel LED Holder (https://www.adafruit.com/product/2171)
- 12mm Tactile button (https://www.adafruit.com/product/1119)
- 22mm Plastic Button (white) (https://www.automationdirect.com/adc/shopping/catalog/pushbuttons_-z-_switches_-z-_indicators/pushbuttons/gcx3105)
- Micro USB extension (https://www.digikey.com/en/products/detail/adafruit-industries-llc/6068/25897272)
- Ethernet Coupler (https://www.mcmaster.com/catalog/3243T1)
- 1/4" TS Audio Jack (https://www.mcmaster.com/catalog/1760N21)
- Tactile Button mounting plate (3d printed) (https://github.com/rmdPurdue/ShowControlButton/blob/main/support%20files/Tactile%2BButton%2BMount.stp)
- Enclosure (including feet and standoffs, laser cut from 6mm draftboard) (https://github.com/rmdPurdue/ShowControlButton/tree/main/support%20files/laser-cut)
