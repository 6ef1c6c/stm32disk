#include "stm32f1xx_hal.h"
#include "usbd_core.h"
#include "usbd_desc.h"

PCD_HandleTypeDef hpcd_USB_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;

void HAL_PCD_MspInit(PCD_HandleTypeDef *pcd)
{
    GPIO_InitTypeDef gpio = {0};
    if (pcd->Instance == USB) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USB_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio);
        HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
        HAL_NVIC_SetPriority(USBWakeUp_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USBWakeUp_IRQn);
    }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *pcd)
{
    if (pcd->Instance == USB) {
        __HAL_RCC_USB_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
        HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
        HAL_NVIC_DisableIRQ(USBWakeUp_IRQn);
    }
}

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    hpcd_USB_FS.Instance = USB;
    hpcd_USB_FS.Init.dev_endpoints = 4;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.ep0_mps = DEP0CTL_MPS_64;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
    pdev->pData = &hpcd_USB_FS;
    hpcd_USB_FS.pData = pdev;

    if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK) return USBD_FAIL;
    (void)HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x00, PCD_SNG_BUF, 0x18);
    (void)HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x80, PCD_SNG_BUF, 0x58);
    (void)HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x81, PCD_SNG_BUF, 0x98);
    (void)HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x01, PCD_SNG_BUF, 0xD8);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{ (void)pdev; return (HAL_PCD_DeInit(&hpcd_USB_FS) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{ (void)pdev; return (HAL_PCD_Start(&hpcd_USB_FS) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{ (void)pdev; return (HAL_PCD_Stop(&hpcd_USB_FS) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t addr, uint8_t type, uint16_t mps)
{ (void)pdev; return (HAL_PCD_EP_Open(&hpcd_USB_FS, addr, mps, type) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t addr)
{ (void)pdev; return (HAL_PCD_EP_Close(&hpcd_USB_FS, addr) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t addr)
{ (void)pdev; return (HAL_PCD_EP_Flush(&hpcd_USB_FS, addr) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t addr)
{ (void)pdev; return (HAL_PCD_EP_SetStall(&hpcd_USB_FS, addr) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t addr)
{ (void)pdev; return (HAL_PCD_EP_ClrStall(&hpcd_USB_FS, addr) == HAL_OK) ? USBD_OK : USBD_FAIL; }
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t addr)
{
    (void)pdev;
    return ((addr & 0x80u) != 0u) ? hpcd_USB_FS.IN_ep[addr & 0x7Fu].is_stall
                                  : hpcd_USB_FS.OUT_ep[addr & 0x7Fu].is_stall;
}
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t addr)
{ (void)pdev; return (HAL_PCD_SetAddress(&hpcd_USB_FS, addr) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t addr, uint8_t *buf, uint32_t len)
{ (void)pdev; return (HAL_PCD_EP_Transmit(&hpcd_USB_FS, addr, buf, len) == HAL_OK) ? USBD_OK : USBD_FAIL; }
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t addr, uint8_t *buf, uint32_t len)
{ (void)pdev; return (HAL_PCD_EP_Receive(&hpcd_USB_FS, addr, buf, len) == HAL_OK) ? USBD_OK : USBD_FAIL; }
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t addr)
{ (void)pdev; return HAL_PCD_EP_GetRxCount(&hpcd_USB_FS, addr); }
void USBD_LL_Delay(uint32_t ms) { HAL_Delay(ms); }

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_SetupStage((USBD_HandleTypeDef *)pcd->pData, (uint8_t *)pcd->Setup); }
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *pcd, uint8_t epnum)
{ USBD_LL_DataOutStage((USBD_HandleTypeDef *)pcd->pData, epnum, pcd->OUT_ep[epnum].xfer_buff); }
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *pcd, uint8_t epnum)
{ USBD_LL_DataInStage((USBD_HandleTypeDef *)pcd->pData, epnum, pcd->IN_ep[epnum].xfer_buff); }
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_SOF((USBD_HandleTypeDef *)pcd->pData); }
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_SetSpeed((USBD_HandleTypeDef *)pcd->pData, USBD_SPEED_FULL); USBD_LL_Reset((USBD_HandleTypeDef *)pcd->pData); }
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_Suspend((USBD_HandleTypeDef *)pcd->pData); }
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_Resume((USBD_HandleTypeDef *)pcd->pData); }
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_DevConnected((USBD_HandleTypeDef *)pcd->pData); }
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *pcd)
{ USBD_LL_DevDisconnected((USBD_HandleTypeDef *)pcd->pData); }

void USB_LP_CAN1_RX0_IRQHandler(void) { HAL_PCD_IRQHandler(&hpcd_USB_FS); }
void USBWakeUp_IRQHandler(void) { HAL_PCD_IRQHandler(&hpcd_USB_FS); }
