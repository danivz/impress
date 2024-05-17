#include "reconfig_hbicap.h"
#include "xstatus.h"
#include "xil_assert.h"
#include "xaxicdma.h"
#include "reconfig_pcap.h"

// FPGA description file
#include "xc7z020.h"
#include "series7.h"


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
int HBICAP_Initialize(XDcfg *PcapInstancePtr, XAxiCdma *DmaInstancePtr, u16 PcapDeviceId, u32 DmaDeviceId)
{
    int Status;

    // PCAP/ICAP configuration
    XDcfg_Config *PcapConfigPtr;

    // Initialize the Device Configuration Interface driver
    PcapConfigPtr = XDcfg_LookupConfig(PcapDeviceId);

    // This is where the virtual address would be used, this example uses physical address
    Status = XDcfg_CfgInitialize(PcapInstancePtr, PcapConfigPtr, PcapConfigPtr->BaseAddr);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    Status = XDcfg_SelfTest(PcapInstancePtr);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    // Select ICAP interface for partial reconfiguration
    XDcfg_SelectIcapInterface(PcapInstancePtr);

    // DMA configuration
    XAxiCdma_Config *DmaConfigPtr;

    // Initialize the Device Configuration Interface driver
    DmaConfigPtr = XAxiCdma_LookupConfig(DmaDeviceId);

    // This is where the virtual address would be used, this example uses physical address
    Status = XAxiCdma_CfgInitialize(DmaInstancePtr, DmaConfigPtr, DmaConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    // Disable interrupts
    XAxiCdma_IntrDisable(DmaInstancePtr, XAXICDMA_XR_IRQ_ALL_MASK);

    return XST_SUCCESS;
}

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
int HBICAP_RAM_write(XAxiCdma *DmaInstancePtr, u32 DeviceCtrlAddr, u32 DeviceMemAddr, u32 *addr_start, u32 addr_end, u32 x0, u32 y0, u32 xf, u32 yf, u32 erase_bram)
{
    u32 Index =0;
    u32 Packet;
    u32 Data;
    u32 TotalWords;
    int Status;
    static u32 WriteBuffer[HBICAP_WRITE_SIZE];

    Xil_AssertNonvoid(DmaInstancePtr != NULL);
    Xil_AssertNonvoid(addr_start != NULL);

    // Bus Width, DUMMY and SYNC
    WriteBuffer[Index++] = PCAP_DUMMY_PACKET;
    WriteBuffer[Index++] = PCAP_BW_SYNC;
    WriteBuffer[Index++] = PCAP_BW_DETECT;
    WriteBuffer[Index++] = PCAP_DUMMY_PACKET;
    WriteBuffer[Index++] = PCAP_SYNC_PACKET;
    WriteBuffer[Index++] = PCAP_NOOP_PACKET;
    WriteBuffer[Index++] = PCAP_NOOP_PACKET;

    // Reset CRC
    Packet = PCAP_Type1Write(PCAP_CMD) | 1;
    Data = PCAP_CMD_RCRC;
    WriteBuffer[Index++] = Packet;
    WriteBuffer[Index++] = Data;
    WriteBuffer[Index++] = PCAP_NOOP_PACKET;
    WriteBuffer[Index++] = PCAP_NOOP_PACKET;

    // ID register
    Packet = PCAP_Type1Write(PCAP_IDCODE) | 1;
    Data = PCAP_IDCODE_NUMBER;
    WriteBuffer[Index++] = Packet;
    WriteBuffer[Index++] = Data;

    // Repeat for each clock region
    u32 *addr_send = addr_start;
    int x, y;
    for(y = y0; y <= yf; y++)
    {
        // Setup CMD register - write configuration
        Packet = PCAP_Type1Write(PCAP_CMD) | 1;
        Data = PCAP_CMD_WCFG;
        WriteBuffer[Index++] = Packet;
        WriteBuffer[Index++] = Data;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Setup FAR
        Packet = PCAP_Type1Write(PCAP_FAR) | 1;
        Data = PCAP_SetupFar7S((fpga[y][x0][0] & (0xFF << 24))>>24, PCAP_FAR_CLB_BLOCK, (fpga[y][x0][0] & (0xFF << 16))>>16, x0, 0);
        WriteBuffer[Index++] = Packet;
        WriteBuffer[Index++] = Data;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Setup Packet header
        TotalWords = 0;
        for(x = x0; x <= xf; x++)
        {
            TotalWords += (fpga[y][x][0] & 0xFFFF) * NUM_FRAME_WORDS;
        }
        TotalWords += NUM_FRAME_WORDS; // We add a padding frame

        if (TotalWords < PCAP_TYPE_1_PACKET_MAX_WORDS)
        {
            // Create Type 1 Packet
            Packet = PCAP_Type1Write(PCAP_FDRI) | TotalWords;
            WriteBuffer[Index++] = Packet;
        }
        else
        {
            // Create Type 2 Packet
            Packet = PCAP_Type1Write(PCAP_FDRI);
            WriteBuffer[Index++] = Packet;

            Packet = PCAP_TYPE_2_WRITE | TotalWords;
            WriteBuffer[Index++] = Packet;
        }

        // Write header data.
        // Configure size of the transfer
        if(erase_bram == PCAP_BRAM_DONOTHING) Xil_Out32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SZ_OFFSET), Index + TotalWords + 8);
        else Xil_Out32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SZ_OFFSET), Index + TotalWords + 8 + 8 + 12928);

        // Write to HBICAP
        Xil_DCacheFlushRange((INTPTR)WriteBuffer, Index*4);
        Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) WriteBuffer, (UINTPTR) DeviceMemAddr, Index*sizeof(u32), NULL, NULL);
        if(Status != XST_SUCCESS) return XST_FAILURE;
        while(XAxiCdma_IsBusy(DmaInstancePtr));

        // Write the frame data.
        Xil_DCacheFlushRange((UINTPTR) addr_send, TotalWords*4);
        Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) addr_send, (UINTPTR) DeviceMemAddr, TotalWords*sizeof(u32), NULL, NULL);
        if(Status != XST_SUCCESS) return XST_FAILURE;
        while(XAxiCdma_IsBusy(DmaInstancePtr));

        // Reset command buffer index
        Index = 0;

        // Increment initial address
        addr_send = (u32*) ((u32) addr_send + (TotalWords  * BYTES_PER_WORD_OF_FRAME));

        // Security check. We add a padding frame to the addr end
        if((u32)addr_send > (addr_end + NUM_FRAME_WORDS*BYTES_PER_WORD_OF_FRAME))
        {
            return XST_FAILURE;
        }
    }

    // Erase BRAM contents if required
    if (erase_bram == PCAP_BRAM_ERASE) {
        u32 null_frame[128*NUM_FRAME_WORDS];
        for (Index = 0; Index < 128*NUM_FRAME_WORDS; Index++) null_frame[Index] = 0;
        Index = 0;
        // Repeat for each clock region
        for (y = y0; y <= yf; y++)
        {
            // Repeat for each column
            for (x = x0; x <= xf; x++)
            {
                // Check if the column is a BRAM column
                if (((fpga_bram[y][x] & 0xFFFF0000)>>16) == BRAM_CONTENT) {
                    // Setup CMD register - write configuration
                    Packet = PCAP_Type1Write(PCAP_CMD) | 1;
                    Data = PCAP_CMD_WCFG;
                    WriteBuffer[Index++] = Packet;
                    WriteBuffer[Index++] = Data;
                    WriteBuffer[Index++] = PCAP_NOOP_PACKET;

                    // Setup FAR
                    Packet = PCAP_Type1Write(PCAP_FAR) | 1;
                    Data = PCAP_SetupFar7S((fpga[y][x][0] & (0xFF << 24))>>24, PCAP_FAR_BRAM_BLOCK, (fpga[y][x][0] & (0xFF << 16))>>16, fpga_bram[y][x] & 0xFFFF, 0);
                    WriteBuffer[Index++] = Packet;
                    WriteBuffer[Index++] = Data;
                    WriteBuffer[Index++] = PCAP_NOOP_PACKET;

                    // Setup Packet header
                    TotalWords = 128 * NUM_FRAME_WORDS;
                    if (TotalWords < PCAP_TYPE_1_PACKET_MAX_WORDS)
                    {
                        // Create Type 1 Packet
                        Packet = PCAP_Type1Write(PCAP_FDRI) | TotalWords;
                        WriteBuffer[Index++] = Packet;
                    }
                    else
                    {
                        // Create Type 2 Packet
                        Packet = PCAP_Type1Write(PCAP_FDRI);
                        WriteBuffer[Index++] = Packet;

                        Packet = PCAP_TYPE_2_WRITE | TotalWords;
                        WriteBuffer[Index++] = Packet;
                    }

                    // Write header data.
                    // Write to HBICAP
                    Xil_DCacheFlushRange((INTPTR) WriteBuffer, Index*4);
                    Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) WriteBuffer, (UINTPTR) DeviceMemAddr, Index*sizeof(u32), NULL, NULL);
                    if(Status != XST_SUCCESS) return XST_FAILURE;
                    while(XAxiCdma_IsBusy(DmaInstancePtr));

                    // Write the frame data.
                    Xil_DCacheFlushRange((UINTPTR) null_frame, TotalWords*4);
                    Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) null_frame, (UINTPTR) DeviceMemAddr, TotalWords*sizeof(u32), NULL, NULL);
                    if(Status != XST_SUCCESS) return XST_FAILURE;
                    while(XAxiCdma_IsBusy(DmaInstancePtr));

                    // Reset command buffer index
                    Index = 0;
                }
            }
        }
    }

    // Add CRC
    Packet = PCAP_Type1Write(PCAP_CMD) | 1;
    Data = PCAP_CMD_RCRC;
    WriteBuffer[Index++] = Packet;
    WriteBuffer[Index++] = Data;
    WriteBuffer[Index++] = PCAP_NOOP_PACKET;
    WriteBuffer[Index++] = PCAP_NOOP_PACKET;

    // DESYNC
    Packet = (PCAP_Type1Write(PCAP_CMD) | 1);
    Data = PCAP_CMD_DESYNCH;
    WriteBuffer[Index++] = Packet;
    WriteBuffer[Index++] = Data;
    WriteBuffer[Index++] = PCAP_DUMMY_PACKET;
    WriteBuffer[Index++] = PCAP_DUMMY_PACKET;

    // Write the end of transaction data
    Xil_DCacheFlushRange((INTPTR) WriteBuffer, Index*4);
    Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) WriteBuffer, (UINTPTR) DeviceMemAddr, Index*sizeof(u32), NULL, NULL);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    while(XAxiCdma_IsBusy(DmaInstancePtr));
    while((Xil_In32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SR_OFFSET)) & 0x1) != 0x1);

    return XST_SUCCESS;
}

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
int HBICAP_RAM_read(XAxiCdma *DmaInstancePtr, u32 DeviceCtrlAddr, u32 DeviceMemAddr, u32 **addr_start, u32 x0, u32 y0, u32 xf, u32 yf)
{
    u32 Packet;
    u32 Data;
    u32 Status;
    static u32 WriteBuffer[HBICAP_WRITE_SIZE];
    u32 Index = 0;
    u32 TotalWords = 0;

    Xil_AssertNonvoid(DmaInstancePtr != NULL);
    Xil_AssertNonvoid(*addr_start != NULL);

    // Repeat for each clock region
    int x, y;
    for(y = y0; y <= yf; y++)
    {
        // First HBICAP transaction (WRITE)
        // Bus Width, DUMMY and SYNC
        WriteBuffer[Index++] = PCAP_DUMMY_PACKET;
        WriteBuffer[Index++] = PCAP_BW_SYNC;
        WriteBuffer[Index++] = PCAP_BW_DETECT;
        WriteBuffer[Index++] = PCAP_DUMMY_PACKET;
        WriteBuffer[Index++] = PCAP_SYNC_PACKET;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Reset CRC
        Packet = PCAP_Type1Write(PCAP_CMD) | 1;
        Data = PCAP_CMD_RCRC;
        WriteBuffer[Index++] = Packet;
        WriteBuffer[Index++] = Data;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Setup CMD register to read configuration
        Packet = PCAP_Type1Write(PCAP_CMD) | 1;
        WriteBuffer[Index++] = Packet;
        WriteBuffer[Index++] = PCAP_CMD_RCFG;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Setup FAR register
        Packet = PCAP_Type1Write(PCAP_FAR) | 1;
        Data = PCAP_SetupFar7S((fpga[y][x0][0] & (0xFF << 24))>>24, PCAP_FAR_CLB_BLOCK, (fpga[y][x0][0] & (0xFF << 16))>>16, x0, 0);
        WriteBuffer[Index++] = Packet;
        WriteBuffer[Index++] = Data;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Set up packet header
        TotalWords = NUM_FRAME_WORDS;
        for(x = x0; x <= xf; x++)
        {
            TotalWords += (fpga[y][x][0] & 0xFFFF) * NUM_FRAME_WORDS;
        }
        if (TotalWords < PCAP_TYPE_1_PACKET_MAX_WORDS)
        {
            // Create Type 1 Packet
            Packet = PCAP_Type1Read(PCAP_FDRO) | TotalWords;
            WriteBuffer[Index++] = Packet;
        }
        else
        {
            // Create Type 2 Packet
            Packet = PCAP_Type1Read(PCAP_FDRO);
            WriteBuffer[Index++] = Packet;

            Packet = PCAP_TYPE_2_READ | TotalWords;
            WriteBuffer[Index++] = Packet;
        }
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;
        WriteBuffer[Index++] = PCAP_NOOP_PACKET;

        // Configure size of the transfers
        Xil_Out32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SZ_OFFSET), Index);

        // Write to HBICAP
        Xil_DCacheFlushRange((INTPTR)WriteBuffer, Index*4);
        Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) WriteBuffer, (UINTPTR) DeviceMemAddr, Index*sizeof(u32), NULL, NULL);
        if(Status != XST_SUCCESS) return XST_FAILURE;
        while(XAxiCdma_IsBusy(DmaInstancePtr));
        while((Xil_In32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SR_OFFSET)) & 0x1) != 0x1);

        // Second HBICAP transaction (READ)
        Xil_Out32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SZ_OFFSET), TotalWords);
        Xil_Out32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_CR_OFFSET), 0x00000002);
        Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) DeviceMemAddr, (UINTPTR) *addr_start, (TotalWords)*sizeof(u32), NULL, NULL);
        if(Status != XST_SUCCESS) return XST_FAILURE;
        while(XAxiCdma_IsBusy(DmaInstancePtr));
        while((Xil_In32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_CR_OFFSET)) & 0x2) == 0x2);

        // Third HBICAP transaction (WRITE)
        // DESYNC
        Index = 0;
        Packet = (PCAP_Type1Write(PCAP_CMD) | 1);
        Data = PCAP_CMD_DESYNCH;
        WriteBuffer[Index++] = Packet;
        WriteBuffer[Index++] = Data;
        WriteBuffer[Index++] = PCAP_DUMMY_PACKET;
        WriteBuffer[Index++] = PCAP_DUMMY_PACKET;

        // Write to HBICAP
        Xil_Out32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SZ_OFFSET), Index);
        Status = XAxiCdma_SimpleTransfer(DmaInstancePtr, (UINTPTR) WriteBuffer, (UINTPTR) DeviceMemAddr, Index*sizeof(u32), NULL, NULL);
        if(Status != XST_SUCCESS) return XST_FAILURE;
        while(XAxiCdma_IsBusy(DmaInstancePtr));
        while((Xil_In32((UINTPTR)(DeviceCtrlAddr + XAXIHBICAP_SR_OFFSET)) & 0x1) != 0x1);
        // End of HBICAP transactions

        // Erase NULL frame
        memmove(*addr_start, *addr_start+NUM_FRAME_WORDS, (TotalWords-NUM_FRAME_WORDS)*BYTES_PER_WORD_OF_FRAME);
        // Reset command buffer index
        Index = 0;
        // Increment initial address
        *addr_start = (u32*) ((u32) *addr_start + (TotalWords- (u32) NUM_FRAME_WORDS) * BYTES_PER_WORD_OF_FRAME);
    }

    return XST_SUCCESS;
}
