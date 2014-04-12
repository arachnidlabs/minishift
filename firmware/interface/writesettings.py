import hid
from mcp2210 import MCP2210, ChipSettings, SPISettings, USBSettings
import time


DEFAULT_ID = (0x04D8, 0x00DE)
OUR_ID = (0x04D8, 0xF517)
VALID_IDS = set([DEFAULT_ID, OUR_ID])
MANUFACTURER = "Arachnid Labs Ltd"
PRODUCT = "USB - SPI Interface 1.0"


chip_settings = ChipSettings.from_buffer_copy(open('chip_settings.bin').read())
transfer_settings = SPISettings.from_buffer_copy(open('transfer_settings.bin').read())
usb_settings = USBSettings.from_buffer_copy(open('usb_settings.bin').read())


def find_device():
    while True:
        for device in hid.enumerate(0, 0):
            id = (device['vendor_id'], device['product_id'])
            if id in VALID_IDS:
                return id
        time.sleep(0.1)


def get_device():
    vid, pid = find_device()
    return MCP2210(vid, pid), (vid, pid) == OUR_ID


def program_device():
    dev, ours = get_device()
    if ours:
        print "Programming device with our PID"
    else:
        print "Programming device with default PID"

    dev.manufacturer_name = MANUFACTURER
    dev.product_name = PRODUCT
    dev.boot_chip_settings = chip_settings
    dev.boot_transfer_settings = transfer_settings
    dev.boot_usb_settings = usb_settings
    dev.transfer("\x55\xaa" * 4)

    print "Device programmed!"


def main():
    program_device()


if __name__ == '__main__':
    main()
