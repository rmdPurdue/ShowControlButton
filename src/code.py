import gc
import os

import adafruit_connection_manager
import board
import digitalio
import time
import microosc

from asyncio import create_task, gather, run
from asyncio import sleep as async_sleep
from adafruit_debouncer import Debouncer
from adafruit_wiznet5k.adafruit_wiznet5k import WIZNET5K

from adafruit_httpserver import (
    MIMETypes,
    POST,
    Response,
    REQUEST_HANDLED_RESPONSE_SENT,
    Request,
    Server,
)

from html import HTML_FORM
from network_config import NetworkConfig
from osc_message import OSCMessage

# Initialize contestant button
button_pin = digitalio.DigitalInOut(board.D6)
button_pin.direction = digitalio.Direction.INPUT
button_pin.pull = digitalio.Pull.UP
button = Debouncer(button_pin, interval=0.01)

# Initialize reset button
reset_pin = digitalio.DigitalInOut(board.D11)
reset_pin.direction = digitalio.Direction.INPUT
reset_pin.pull = digitalio.Pull.UP
reset = Debouncer(reset_pin, interval=0.01)

# Initialize contact closure output
contact = digitalio.DigitalInOut(board.D9)
contact.direction = digitalio.Direction.OUTPUT

# Initialize contact closure reset output for non-latching relay
contact_reset = digitalio.DigitalInOut(board.D13)
contact_reset.direction = digitalio.Direction.OUTPUT

# Initialize local led
led = digitalio.DigitalInOut(board.D12)
led.direction = digitalio.Direction.OUTPUT

# Default network config
my_network_config = NetworkConfig()
my_network_config.ip = os.getenv("IP")
my_network_config.subnet = os.getenv("SN")
my_network_config.gateway = os.getenv("GW")
my_network_config.dns = os.getenv("DNS")

# Initialize OSC message variable
osc_message = OSCMessage()
osc_message.destination = os.getenv("DIP")
osc_message.port = os.getenv("DP")
osc_message.payload(os.getenv("DM"))
if os.getenv("EN") == "True":
    osc_message.state = True
elif os.getenv("EN") == "False":
    osc_message.state = False

# For Adafruit Ethernet FeatherWing
cs = digitalio.DigitalInOut(board.D10)
spi_bus = board.SPI()

# Initialize ethernet interface without DHCP
eth = WIZNET5K(spi_bus, cs, is_dhcp=False)

eth.ifconfig = (my_network_config.ip,
                my_network_config.subnet,
                my_network_config.gateway,
                my_network_config.dns)

MIMETypes.configure(
    default_to="text/plain",
    keep_for=[".html", ".css", ".js", ".png", ".jpg", ".jpeg", ".gif", ".ico"],
)

# Create a socket pool
pool = adafruit_connection_manager.get_radio_socketpool(eth)
server = Server(pool, "../static", debug=True)


@server.route("/")
def base(request: Request):
    checked_state = None
    if osc_message.state:
        checked_state = "checked"
    return Response(request,
                    HTML_FORM.format(
                        network="display:none;",
                        osc="display:none;",
                        checked=checked_state,
                        ip=my_network_config.ip_as_string(),
                        subnet=my_network_config.subnet_as_string(),
                        gateway=my_network_config.gateway_as_string(),
                        dns=my_network_config.dns_as_string(),
                        destinationIP=osc_message.destination,
                        destinationPort=osc_message.port,
                        message=osc_message.payload_no_types()
                    ),
                    content_type="text/html", )


@server.route("/submit-network", POST)
def submit_form_handler(request: Request):
    # Parse form data from the POST request
    form_data = request.form_data
    my_network_config.ip = form_data.get("ip")
    my_network_config.subnet = form_data.get("subnet")
    my_network_config.gateway = form_data.get("gateway")
    my_network_config.dns = form_data.get("dns")
    checked_state = None
    if osc_message.state:
        checked_state = "checked"
    update_toml()
    gc.collect()
    # We have to restart the network connection here!

    return Response(request,
                    HTML_FORM.format(
                        network="display:inline;",
                        osc="display:none;",
                        checked=checked_state,
                        ip=my_network_config.ip_as_string(),
                        subnet=my_network_config.subnet_as_string(),
                        gateway=my_network_config.gateway_as_string(),
                        dns=my_network_config.dns_as_string(),
                        destinationIP=osc_message.destination,
                        destinationPort=osc_message.port,
                        message=osc_message.payload_no_types()
                    ),
                    content_type="text/html", )


@server.route("/submit-osc", POST)
def submit_form_handler(request: Request):
    # Parse form data from the POST request
    form_data = request.form_data
    osc_message.destination = form_data.get("destIp")
    osc_message.port = form_data.get("destPort")
    osc_message.payload(form_data.get("message"))
    if form_data.get("sendOSC") == "on":
        osc_message.state = True
    else:
        osc_message.state = False
    checked_state = None
    if osc_message.state:
        checked_state = "checked"
    update_toml()
    gc.collect()

    return Response(request,
                    HTML_FORM.format(
                        network="display:none;",
                        osc="display:inline;",
                        checked=checked_state,
                        ip=my_network_config.ip_as_string(),
                        subnet=my_network_config.subnet_as_string(),
                        gateway=my_network_config.gateway_as_string(),
                        dns=my_network_config.dns_as_string(),
                        destinationIP=osc_message.destination,
                        destinationPort=osc_message.port,
                        message=osc_message.payload_no_types()
                    ),
                    content_type="text/html", )


def update_toml():
    with open("settings.toml", "w") as f:
        f.write(f'IP = "{my_network_config.ip_as_string()}"\n')
        f.write(f'SN = "{my_network_config.subnet_as_string()}"\n')
        f.write(f'GW = "{my_network_config.gateway_as_string()}"\n')
        f.write(f'DNS = "{my_network_config.dns_as_string()}"\n')
        f.write(f'DIP = "{osc_message.destination}"\n')
        f.write(f'DP = "{osc_message.port}"\n')
        f.write(f'DM = "{osc_message.payload_no_types()}"\n')
        if osc_message.state:
            f.write('EN = "True"\n')
        else:
            f.write('EN = "False"\n')


# Start the server.
try:
    server.start(str(eth.pretty_ip(eth.ip_address)))
    my_network_config.disconnected = False
except Exception:
    my_network_config.disconnected = True


async def handle_http_requests():
    while True:
        # Process any waiting requests
        if not my_network_config.disconnected:
            try:
                pool_result = server.poll()
                gc.collect()

                if pool_result == REQUEST_HANDLED_RESPONSE_SENT:
                    eth.ifconfig = (my_network_config.ip,
                                    my_network_config.subnet,
                                    my_network_config.gateway,
                                    my_network_config.dns)
                    gc.collect()
            except Exception:
                my_network_config.disconnected = True

        await async_sleep(0)


async def do_something_useful():
    HOLD_TIME = 5.0
    reset_pressed_time = None
    reset_triggered = False
    led.value = False

    while True:
        reset.update()
        button.update()

        if not reset.value:  # Button is pressed
            if reset_pressed_time is None:
                # First cycle button is detected as pressed
                reset_pressed_time = time.monotonic()
            elif not reset_triggered:
                # Check how long it has been held
                if time.monotonic() - reset_pressed_time >= HOLD_TIME:
                    print("Factory reset.")
                    my_network_config.ip = "192.168.1.100"
                    my_network_config.subnet = "255.255.255.0"
                    my_network_config.gateway = "192.168.1.1"
                    my_network_config.dns = "8.8.8.8"
                    eth.ifconfig = (my_network_config.ip,
                                    my_network_config.subnet,
                                    my_network_config.gateway,
                                    my_network_config.dns)
                    update_toml()
                    gc.collect()
                    reset_triggered = True  # Prevent continuous triggering
        else:
            # Button is released, reset state
            reset_pressed_time = None
            reset_triggered = False

        # Check for contestant button push, latching relay
        if button.fell:
            contact.value = True
            time.sleep(0.1)
            contact.value = False
            led.value = True
            if osc_message.state:
                osc_client = microosc.OSCClient(pool, osc_message.destination, osc_message.port)
                msg = microosc.OscMsg(osc_message.address, *osc_message.args, *osc_message.types)
                try:
                    osc_client.send(msg)
                except Exception:
                    pass
            time.sleep(5.0)
            contact_reset.value = True
            led.value = False
            time.sleep(0.1)
            contact_reset.value = False

        '''
        # Check for contestant button push, non-latching relay
        if button.fell:
            contact.value = True
            led.value = True
            if osc_message.state:
                osc_client = microosc.OSCClient(pool, osc_message.destination, osc_message.port)
                msg = microosc.OscMsg(osc_message.address, *osc_message.args, *osc_message.types)
                try:
                    osc_client.send(msg)
                except Exception:
                    pass
            time.sleep(5.0)
            contact.value = False
            led.value = False
            time.sleep(0.1)
        '''

        gc.collect()
        await async_sleep(0)


async def main():
    await gather(
        create_task(handle_http_requests()),
        create_task(do_something_useful()),
    )


run(main())
