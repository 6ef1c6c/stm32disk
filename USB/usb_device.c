#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_msc.h"
#include "usbd_desc.h"
#include "usbd_storage_if.h"

USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void)
{
    (void)USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    (void)USBD_RegisterClass(&hUsbDeviceFS, USBD_MSC_CLASS);
    (void)USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_STORAGE_fops);
    (void)USBD_Start(&hUsbDeviceFS);
}
