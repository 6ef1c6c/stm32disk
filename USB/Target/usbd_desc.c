#include "usbd_desc.h"

#define USB_VID     0x1209u
#define USB_PID     0xD15Cu
#define USB_LANGID  0x0409u

static uint8_t s_device_desc[USB_LEN_DEV_DESC] = {
    0x12, USB_DESC_TYPE_DEVICE,
    0x00, 0x02, 0x00, 0x00, 0x00,
    USB_MAX_EP0_SIZE,
    LOBYTE(USB_VID), HIBYTE(USB_VID),
    LOBYTE(USB_PID), HIBYTE(USB_PID),
    0x00, 0x01,
    1, 2, 3,
    0x01
};

static uint8_t s_lang_id_desc[USB_LEN_LANGID_STR_DESC] = {
    USB_LEN_LANGID_STR_DESC, USB_DESC_TYPE_STRING,
    LOBYTE(USB_LANGID), HIBYTE(USB_LANGID)
};

static uint8_t s_string[64];

static uint8_t *get_string(const char *src, uint16_t *length)
{
    uint16_t n = 0;
    while (src[n] != '\0' && n < 31u) n++;
    s_string[0] = (uint8_t)(2u * n + 2u);
    s_string[1] = USB_DESC_TYPE_STRING;
    for (uint16_t i = 0; i < n; ++i) {
        s_string[2u + 2u * i] = (uint8_t)src[i];
        s_string[3u + 2u * i] = 0;
    }
    *length = s_string[0];
    return s_string;
}

static uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(s_device_desc); return s_device_desc; }
static uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(s_lang_id_desc); return s_lang_id_desc; }
static uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; return get_string("Generic", length); }
static uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; return get_string("Mass Storage", length); }
static uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; return get_string("00000001", length); }
static uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; return get_string("MSC", length); }
static uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; return get_string("MSC", length); }

USBD_DescriptorsTypeDef FS_Desc = {
    USBD_FS_DeviceDescriptor,
    USBD_FS_LangIDStrDescriptor,
    USBD_FS_ManufacturerStrDescriptor,
    USBD_FS_ProductStrDescriptor,
    USBD_FS_SerialStrDescriptor,
    USBD_FS_ConfigStrDescriptor,
    USBD_FS_InterfaceStrDescriptor
};
