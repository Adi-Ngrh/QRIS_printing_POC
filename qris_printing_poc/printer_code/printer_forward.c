#include <stdio.h>
#include <string.h>
#include "lwip/sockets.h"
#include "printer_forward.h"

#define ESCPRESSO_HOST "10.170.161.230"
#define ESCPRESSO_PORT 9100

// ESC @ : initialize printer
static const uint8_t CMD_INIT[]         = { 0x1B, 0x40 };    
// ESC a 1 : center alignment                     
static const uint8_t CMD_ALIGN_CENTER[] = { 0x1B, 0x61, 0x01 };    
// ESC a 0 : left alignment               
static const uint8_t CMD_ALIGN_LEFT[]   = { 0x1B, 0x61, 0x00 };  
// ESC E 1 : bold on
static const uint8_t CMD_BOLD_ON[]      = { 0x1B, 0x45, 0x01 };        
// ESC E 0 : bold off
static const uint8_t CMD_BOLD_OFF[]     = { 0x1B, 0x45, 0x00 };     
// LF x3 + GS V B 0 : feed lines and partial cut              
static const uint8_t CMD_FEED_CUT[]     = { 0x0A, 0x0A, 0x0A, 0x1D, 0x56, 0x42, 0x00 }; 

static int s_sock;

static void printer_connect(void)
{
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(ESCPRESSO_PORT),
    };
    inet_pton(AF_INET, ESCPRESSO_HOST, &addr.sin_addr);

    s_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    connect(s_sock, (struct sockaddr *)&addr, sizeof(addr));
}

/*
* @brief send data to printer
*
* use lwip TCP/IP stack
*/
static void printer_send(const uint8_t *data, size_t len)
{
    send(s_sock, data, len, 0);
}

static void printer_send_str(const char *s)
{
    printer_send((const uint8_t *)s, strlen(s));
}

void printer_forward_init(void)
{
    printer_connect();
}

/*
* @brief print raw payload
*/
void printer_print_raw(const uint8_t *data, size_t len)
{
    printer_send(CMD_INIT, sizeof(CMD_INIT));
    printer_send(data, len);
    printer_send_str("\n");
}

/*
* @brief print formatted payload
*
* @param receipt : raw payload
*/
void printer_print_receipt(const printer_receipt_t *receipt)
{
    char line[64];

    printer_send(CMD_INIT, sizeof(CMD_INIT));
    printer_send(CMD_ALIGN_CENTER, sizeof(CMD_ALIGN_CENTER));
    printer_send(CMD_BOLD_ON, sizeof(CMD_BOLD_ON));
    printer_send_str("PAYMENT RECEIPT\n");
    printer_send(CMD_BOLD_OFF, sizeof(CMD_BOLD_OFF));

    printer_send(CMD_ALIGN_LEFT, sizeof(CMD_ALIGN_LEFT));

    snprintf(line, sizeof(line), "Invoice : %s\n", receipt->invoice_number);
    printer_send_str(line);

    snprintf(line, sizeof(line), "Serial  : %s\n", receipt->serial);
    printer_send_str(line);

    snprintf(line, sizeof(line), "Paid at : %s\n", receipt->paid_at);
    printer_send_str(line);

    snprintf(line, sizeof(line), "Amount  : %s\n", receipt->amount);
    printer_send_str(line);

    snprintf(line, sizeof(line), "Status  : %s\n", receipt->status);
    printer_send_str(line);

    printer_send(CMD_FEED_CUT, sizeof(CMD_FEED_CUT));
}

/*
* @brief generate and print QR
*
* @param qr_content : QR string
*/
void printer_print_qris(const char *qr_content)
{
    size_t data_len = strlen(qr_content);
    size_t store_len = data_len + 3;
    uint8_t pL = store_len & 0xFF;
    uint8_t pH = (store_len >> 8) & 0xFF;

    // GS ( k pL pH cn fn [parameters]
    // select QR model 2
    uint8_t cmd_model[]     = { 0x1D, 0x28, 0x6B, 0x04, 0x00, 0x31, 0x41, 0x32, 0x00 }; 
    // set QR module size
    uint8_t cmd_size[]      = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x43, 0x06 };       
    // set QR error-correction level
    uint8_t cmd_ec_level[]  = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45, 0x31 };  
    // store QR data (header, data bytes follow)     
    uint8_t cmd_store_hdr[] = { 0x1D, 0x28, 0x6B, pL, pH, 0x31, 0x50, 0x30 };  
    // print the stored QR data         
    uint8_t cmd_print[]     = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30 };       

    printer_send(CMD_INIT, sizeof(CMD_INIT));
    printer_send(CMD_ALIGN_CENTER, sizeof(CMD_ALIGN_CENTER));

    printer_send(cmd_model, sizeof(cmd_model));
    printer_send(cmd_size, sizeof(cmd_size));
    printer_send(cmd_ec_level, sizeof(cmd_ec_level));

    printer_send(cmd_store_hdr, sizeof(cmd_store_hdr));
    printer_send_str(qr_content);

    printer_send(cmd_print, sizeof(cmd_print));
    printer_send(CMD_FEED_CUT, sizeof(CMD_FEED_CUT));
}
