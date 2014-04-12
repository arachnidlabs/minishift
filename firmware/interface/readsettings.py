from mcp2210 import MCP2210


def main():
    dev = MCP2210(0x04d8, 0xf517)

    f = open('chip_settings.bin', 'w')
    f.write(str(buffer(dev.boot_chip_settings)))
    f.close()

    f = open('transfer_settings.bin', 'w')
    f.write(str(buffer(dev.boot_transfer_settings)))
    f.close()

    f = open('usb_settings.bin', 'w')
    f.write(str(buffer(dev.boot_usb_settings)))
    f.close()


if __name__ == '__main__':
    main()
