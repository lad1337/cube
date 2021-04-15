import logging
import socket
import signal

from pyhap.const import CATEGORY_LIGHTBULB
from pyhap.accessory import Accessory, Bridge
from pyhap.accessory_driver import AccessoryDriver

logging.basicConfig(level=logging.INFO, format="[%(module)s] %(message)s")

TARGET_IP = "10.0.13.50"
TARGET_PORT = 1234


class Light(Accessory):
    """Fake lightbulb, logs what the client sets."""

    category = CATEGORY_LIGHTBULB

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        serv_light = self.add_preload_service("Lightbulb", ["Brightness"])
        self.char_on = serv_light.configure_char("On", setter_callback=self.set_bulb)
        self.char_brightness = serv_light.configure_char(
            "Brightness", setter_callback=self.set_brightness
        )

    def set_bulb(self, value):
        logging.info("Bulb on value: %s", value)

        out = ",".join(map(str, [0, 0, 0, value]))
        socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(
            out.encode("utf-8"), (TARGET_IP, TARGET_PORT)
        )

    def set_brightness(self, value):
        logging.info("Bulb brightness value: %s", value)


def get_bridge(driver):
    bridge = Bridge(driver, "Bridge")
    bridge.add_accessory(Light(driver, "Cube"))
    return bridge


driver = AccessoryDriver(port=51826, persist_file="cube.state")
driver.add_accessory(accessory=get_bridge(driver))
signal.signal(signal.SIGTERM, driver.signal_handler)
driver.start()
