/*
 * 	reconfig_hbicap.h
 *
 *  Created on: 20/02/2024
 *      Author: Daniel Vazquez
 */

#ifndef RECONFIG_HBICAP_H_
#define RECONFIG_HBICAP_H_

#include "xil_types.h"
#include "xdevcfg.h"
#include "xaxicdma.h"

/** @name Register offset definitions
 *   Register accesses are 32-bit.
 * @{
 */
#define XAXIHBICAP_GIER_OFFSET 	0x01C
#define XAXIHBICAP_IPISR_OFFSET 0x020
#define XAXIHBICAP_IPIER_OFFSET 0x028
#define XAXIHBICAP_SZ_OFFSET 	0x108
#define XAXIHBICAP_CR_OFFSET 	0x10C
#define XAXIHBICAP_SR_OFFSET 	0x110
#define XAXIHBICAP_WFV_OFFSET 	0x114
#define XAXIHBICAP_RFO_OFFSET 	0x118
#define XAXIHBICAP_ASR_OFFSET 	0x11C

// Constant definitions
#define HBICAP_WRITE_SIZE 256

/************************** Function Prototypes ******************************/

/****************************************************************************/
/**
*
* Initializes HBICAP interface and CDMA
*
* @param PcapInstancePtr is a pointer to the PCAP instance to be configured
* @param DmaInstancePtr is a pointer to the CDMA instance to be configured
* @param PcapDeviceId is the ID of the DEVCFG device
* @param DmaDeviceId is the ID of the CDMA device
*
* @return   XST_SUCCESS else XST_FAILURE.
*
*****************************************************************************/
int HBICAP_Initialize(XDcfg *PcapInstancePtr, XAxiCdma *DmaInstancePtr, u16 PcapDeviceId, u32 DmaDeviceId);

/****************************************************************************/
/**
*
* Writes PBS file using HBICAP interface
*
* @param DmaInstancePtr is a pointer to the CDMA instance.
* @param DeviceCtrlAddr is the base address of the control interface of the HBICAP IP
* @param DeviceMemAddr is the base address of the memory interface of the HBICAP IP
* @param addr_start is a pointer to the frame that is to be written to the device
* @param addr_end is the value of the last memory position of the PBS
* @param x0, y0, xf, yf are the coordinates of the region to be reconfigured
* @param erase_bram is a boolean to erase the content of the BRAMs
*
* @return   XST_SUCCESS else XST_FAILURE.
*
*****************************************************************************/
int HBICAP_RAM_write(XAxiCdma *DmaInstancePtr, u32 DeviceCtrlAddr, u32 DeviceMemAddr, u32 *addr_start, u32 addr_end, u32 x0, u32 y0, u32 xf, u32 yf, u32 erase_bram);

/****************************************************************************/
/**
*
* Reads PBS file using HBICAP interface
*
* @param DmaInstancePtr is a pointer to the CDMA instance.
* @param DeviceCtrlAddr is the base address of the control interface of the HBICAP IP
* @param DeviceMemAddr is the base address of the memory interface of the HBICAP IP
* @param addr_start is a pointer to the memory addres that will store data read from the device
* @param x0, y0, xf, yf are the coordinates of the region to be reconfigured
*
* @return   XST_SUCCESS else XST_FAILURE.
*
*****************************************************************************/
int HBICAP_RAM_read(XAxiCdma *DmaInstancePtr, u32 DeviceCtrlAddr, u32 DeviceMemAddr, u32 **addr_start, u32 x0, u32 y0, u32 xf, u32 yf);

#endif /* RECONFIG_HBICAP_H_ */
