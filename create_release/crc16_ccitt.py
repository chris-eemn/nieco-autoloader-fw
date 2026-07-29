"""CRC16-CCITT (poly 0x1021, init 0xFFFF, no reflection, no final XOR -- "CCITT-FALSE").

Must match bootloader/crc16_ccitt.c exactly, byte for byte, since the bootloader verifies
this checksum against the value packer.py writes into the header.
"""

CRC16_CCITT_POLYNOMIAL = 0x1021
CRC16_CCITT_INITIAL_VALUE = 0xFFFF


def crc16_ccitt(data: bytes, running_crc: int = CRC16_CCITT_INITIAL_VALUE) -> int:
    crc = running_crc
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ CRC16_CCITT_POLYNOMIAL) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc
