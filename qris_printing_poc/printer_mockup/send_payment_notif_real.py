"""
Same as send_payment_notif.py, except qr_content comes from a real call to
QoT's /qr-generate endpoint instead of a locally-fabricated fake string.
Requires qot-staging.eloku.com to actually be reachable.
"""

import argparse
import hashlib
import hmac
import json
import random
import string
import time

import paho.mqtt.publish as publish
import requests

BROKER_HOST = "localhost"
BROKER_PORT = 1883
DEVICE_IMEI = "3"

QOT_BASE_URL = "https://qot-staging.eloku.com/api/v1"


def random_invoice_number():
    return "".join(random.choices(string.hexdigits.lower(), k=32))


def random_serial():
    return "INVC-" + "".join(random.choices(string.ascii_uppercase, k=5))


def register_device(imei):
    resp = requests.post(f"{QOT_BASE_URL}/admin/device", json={"imei": imei})
    resp.raise_for_status()
    return resp.json()["device"]  # {"secret": ..., "topic": ...}


def sign_request(secret, method, path, body, timestamp):
    canonical_body = json.dumps(body, separators=(",", ":"))
    body_hash = hashlib.sha256(canonical_body.encode()).hexdigest()
    string_to_sign = f"{method}:{path}:{body_hash}:{timestamp}"
    return hmac.new(secret.encode(), string_to_sign.encode(), hashlib.sha256).hexdigest()


def generate_qr(imei, secret, total_amount):
    body = {"total_amount": total_amount, "imei": imei}
    timestamp = str(int(time.time()))

    headers = {
        "X-SIGNATURE": sign_request(secret, "POST", "/api/v1/qr-generate", body, timestamp),
        "X-TIMESTAMP": timestamp,
    }

    resp = requests.post(f"{QOT_BASE_URL}/qr-generate", json=body, headers=headers)
    resp.raise_for_status()
    return resp.json()["qr"]


def build_payload(amount):
    device = register_device(DEVICE_IMEI)
    qr = generate_qr(DEVICE_IMEI, device["secret"], amount)

    return {
        "amount": f"{amount:.2f}",
        "invoice_number": random_invoice_number(),
        "paid_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        "serial": random_serial(),
        "status": "pending",
        "qr_content": qr["qr_content"],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--amount", type=float, default=250000.00)
    args = parser.parse_args()

    topic = f"topic-notif-{DEVICE_IMEI}"
    payload = build_payload(args.amount)

    publish.single(topic, json.dumps(payload), hostname=BROKER_HOST, port=BROKER_PORT)
    print(f"published to {topic}: {payload}")
