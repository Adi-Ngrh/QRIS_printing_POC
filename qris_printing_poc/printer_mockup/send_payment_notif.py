"""
fake payload (run with --mode flag to switch between 3 print modes, receipt(0), qris(1), raw(2))
"""

import argparse
import json
import random
import string
import time

import paho.mqtt.publish as publish

BROKER_HOST = "localhost"
BROKER_PORT = 1883
DEVICE_IMEI = "3"


def random_invoice_number():
    return "".join(random.choices(string.hexdigits.lower(), k=32))


def random_serial():
    return "INVC-" + "".join(random.choices(string.ascii_uppercase, k=5))


def crc16_ccitt(data):
    crc = 0xFFFF
    for byte in data.encode():
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def tlv(tag, value):
    return f"{tag}{len(value):02d}{value}"


def build_fake_qris(amount, invoice_number):
    merchant_info = tlv("00", "ID.CO.QRIS.WWW") + tlv("01", "ID2024000000001")
    additional_data = tlv("01", invoice_number)

    payload = (
        tlv("00", "01")
        + tlv("01", "12")
        + tlv("26", merchant_info)
        + tlv("52", "0000")
        + tlv("53", "360")
        + tlv("54", amount)
        + tlv("58", "ID")
        + tlv("59", "MOCKUP MERCHANT")
        + tlv("60", "JAKARTA")
        + tlv("62", additional_data)
        + "6304"
    )
    return payload + f"{crc16_ccitt(payload):04X}"


def build_payload(amount, mode):
    amount_str = f"{amount:.2f}"
    invoice_number = random_invoice_number()

    return {
        "amount": amount_str,
        "invoice_number": invoice_number,
        "paid_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        "serial": random_serial(),
        "status": "pending",
        "qr_content": build_fake_qris(amount_str, invoice_number),
        "print_mode": mode,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--amount", type=float, default=250000.00)
    parser.add_argument("--mode", type=int, choices=[0, 1, 2], default=2, help="0=raw, 1=receipt, 2=qris")
    args = parser.parse_args()

    topic = f"topic-notif-{DEVICE_IMEI}"
    payload = build_payload(args.amount, args.mode)

    publish.single(topic, json.dumps(payload), hostname=BROKER_HOST, port=BROKER_PORT)
    print(f"published to {topic}: {payload}")
