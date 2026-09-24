#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *invoice_number;
    const char *amount;
    const char *paid_at;
    const char *serial;
    const char *status;
} printer_receipt_t;

void printer_forward_init(void);

/* raw payload print, no formatting */
void printer_print_raw(const uint8_t *data, size_t len);

/* formatted receipt print */
void printer_print_receipt(const printer_receipt_t *receipt);

/* dynamic QRIS print */
void printer_print_qris(const char *qr_content);
