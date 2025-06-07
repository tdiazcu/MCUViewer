
/*********************************************************************
 *                    ????????????????????????                        *
 **********************************************************************
 *                                                                    *
 *            .........................................               *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 *             NGSPICE NgspiceDLL.h and Ngspice_Const.h               *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 *                  ..........................                        *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 * All rights reserved.                                               *
 *                                                                    *
 * Redistribution and use in source and binary forms                  *
 * without modification is permitted provided that the following      *
 * condition is met:                                                  *
 *                                                                    *
 * Redistributions of source code or binary must retain the above     *
 * copyright notice, this condition and the following disclaimer.     *
 *                                                                    *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
 * DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
 * DAMAGE.                                                            *
 *                                                                    *
 **********************************************************************/

#ifndef NGSPICE_H	//  Avoid multiple inclusion
#define NGSPICE_H

#include "stdint.h"

#define NGSPICE_HSS_FLAG_TIMESTAMP_US (1uL << 0)

#define NGSPICE_TIF_JTAG 0
#define NGSPICE_TIF_SWD  1

#define NGSPICE_DEVICE_MAX_NUM_FLASH_BANKS 16

/*********************************************************************
 *
 *       SWO Commands
 */
#define NGSPICE_SWO_CMD_START				 0	// Parameter: NGSPICE_SWO_START_INFO *
#define NGSPICE_SWO_CMD_STOP				 1
#define NGSPICE_SWO_CMD_FLUSH				 2	// Parameter: U32* Number of bytes to flush
#define NGSPICE_SWO_CMD_GET_SPEED_INFO		 3
#define NGSPICE_SWO_CMD_GET_NUM_BYTES		 10
#define NGSPICE_SWO_CMD_SET_BUFFERSIZE_HOST 20
#define NGSPICE_SWO_CMD_SET_BUFFERSIZE_EMU	 21

/*********************************************************************
 *
 *       SWO Interfaces
 */
#define NGSPICE_SWO_IF_UART	   0
#define NGSPICE_SWO_IF_MANCHESTER 1  // Not supported in current version
#define NGSPICE_SWO_IF_TRACE	   2  // Only used internally, in case SWO ITM data is merged into trace stream

#if defined(__cplusplus)
extern "C"
{  // Make sure we have C-declarations in C++ programs
#endif

	typedef void NGSPICE_LOG(const char* sErr);

	typedef struct
	{
		uint32_t SerialNumber;		   // This is the serial number reported in the discovery process, which is the "true serial number" for newer J-Links and 123456 for older J-Links.
		unsigned Connection;		   // Either NGSPICE_HOSTIF_USB = 1 or NGSPICE_HOSTIF_IP = 2
		uint32_t USBAddr;			   // USB Addr. Default is 0, values of 0..3 are permitted (Only filled if for J-Links connected via USB)
		uint8_t aIPAddr[16];		   // IP Addr. in case emulator is connected via IP. For IP4 (current version), only the first 4 bytes are used.
		int Time;					   // J-Link via IP only: Time period [ms] after which we have received the UDP discover answer from emulator (-1 if emulator is connected over USB)
		uint64_t Time_us;			   // J-Link via IP only: Time period [us] after which we have received the UDP discover answer from emulator (-1 if emulator is connected over USB)
		uint32_t HWVersion;			   // J-Link via IP only: Hardware version of J-Link
		uint8_t abMACAddr[6];		   // J-Link via IP only: MAC Addr
		char acProduct[32];			   // Product name
		char acNickName[32];		   // J-Link via IP only: Nickname of J-Link
		char acFWString[112];		   // J-Link via IP only: Firmware string of J-Link
		char IsDHCPAssignedIP;		   // J-Link via IP only: Is J-Link configured for IP address reception via DHCP?
		char IsDHCPAssignedIPIsValid;  // J-Link via IP only
		char NumIPConnections;		   // J-Link via IP only: Number of IP connections which are currently established to this J-Link
		char NumIPConnectionsIsValid;  // J-Link via IP only
		uint8_t aPadding[34];		   // Pad struct size to 264 bytes
	} NGSPICE_EMU_CONNECT_INFO;	   // In general, unused fields are zeroed.

	typedef struct
	{
		uint32_t SizeOfStruct;	// Required. Use SizeofStruct = sizeof(NGSPICE_DEVICE_SELECT_INFO)
		uint32_t CoreIndex;
	} NGSPICE_DEVICE_SELECT_INFO;

	typedef struct
	{
		uint32_t Addr;
		uint32_t NumBytes;
		uint32_t Flags;	 // Future use. SBZ.
		uint32_t Dummy;	 // Future use. SBZ.
	} NGSPICE_HSS_MEM_BLOCK_DESC;

	typedef struct
	{
		//
		// Needed for algos that are implemented via RAMCode + PCode or PCode only
		// For these algos, pAlgoInfo is == NULL and needs to be constructed at runtime
		//
		const void* pRAMCodeTurbo_LE;
		const void* pRAMCodeTurbo_BE;
		const void* pRAMCode_LE;
		const void* pRAMCode_BE;
		uint32_t SizeRAMCodeTurbo_LE;
		uint32_t SizeRAMCodeTurbo_BE;
		uint32_t SizeRAMCode_LE;
		uint32_t SizeRAMCode_BE;
		const void* pPCode;	 // PCode for flash bank
		uint32_t SizePCode;
	} NGSPICE_FLASH_BANK_INFO_EXT;

	typedef struct
	{
		uint32_t Addr;
		uint32_t Size;
	} NGSPICE_FLASH_AREA_INFO;

	typedef struct
	{
		uint32_t Addr;
		uint32_t Size;
	} NGSPICE_RAM_AREA_INFO;

	typedef struct
	{
		const char* sBankName;
		const char* sAlgoFile;
		uint32_t AlgoType;	// Really of type MAIN_FLASH_ALGO_TYPE but to avoid mutual inclusion, we choose U32 here for now...
		uint32_t BaseAddr;
		const void* paBlockInfo;  // For some algos, e.g. for SPIFI, this is just a default block info that may vary from target to target, so we need to request the actual one from the target via the RAMCode, at runtime
		const void* pAlgoInfo;
	} NGSPICE_FLASH_BANK_INFO;

	typedef struct
	{
		const uint8_t* pPCode;	// Pointer to PCode.
		uint32_t NumBytes;		// Length of PCode in bytes.
	} NGSPICE_PCODE_INFO;

	typedef NGSPICE_FLASH_AREA_INFO FLASH_AREA_INFO;
	typedef NGSPICE_RAM_AREA_INFO RAM_AREA_INFO;

	typedef struct
	{
		//
		// IMPORTANT!!!!:
		// New elements must be added at the end of the struct.
		// Otherwise, positions of existing elements in the struct would change and make the API binary incompatible.
		//
		uint32_t SizeOfStruct;	// Required. Use SizeofStruct = sizeof(NGSPICE_DEVICE_INFO)
		const char* sName;
		uint32_t CoreId;
		uint32_t FlashAddr;														// Start address of first flash area
		uint32_t RAMAddr;														// Start address of first RAM area
		char EndianMode;														// 0=Little, 1=Big, 2=Both
		uint32_t FlashSize;														// Total flash size in bytes (flash may contain gaps. For exact address & size of each region, please refer to aFlashArea)
		uint32_t RAMSize;														// Total RAM size in bytes  (RAM may contain gaps. For exact address & size of each region, please refer to aRAMArea)
		const char* sManu;														// Device manufacturer
		NGSPICE_FLASH_AREA_INFO aFlashArea[32];									// Region size of 0 bytes marks the end of the list
		NGSPICE_RAM_AREA_INFO aRAMArea[32];										// Region size of 0 bytes marks the end of the list
		uint32_t Core;															// NGSPICE_CORE_... value
		NGSPICE_FLASH_BANK_INFO aFlashBank[NGSPICE_DEVICE_MAX_NUM_FLASH_BANKS];	// Only interesting for J-Flash. Other applications can safely ignore this
		NGSPICE_PCODE_INFO aPCodes[10];											// Only interesting for J-Flash. Other applications can safely ignore this. Currently, we support 5 different PCodes. We have allocated 5 elements as buffer for future versions.
		//
		// Available since extension for Flash banks without direct algo info linking
		//
		NGSPICE_FLASH_BANK_INFO_EXT aFlashBankExt[NGSPICE_DEVICE_MAX_NUM_FLASH_BANKS];  // Only interesting for J-Flash. Other applications can safely ignore this
		const char* sNote;															   // Contains a link to a device specific wiki article (if available). Can be used as additional user / display information.
		const char* sDeviceFamily;													   // The device family of a specific device is not fixed and may change. Can be used as additional user / display information.
	} NGSPICE_DEVICE_INFO;

	const char* NGSPICE_Open(void);
	void NGSPICE_Close(void);
	char NGSPICE_IsOpen(void);
	int32_t NGSPICE_Connect(void);
	int32_t NGSPICE_ReadMemEx(uint32_t Addr, uint32_t NumBytes, void* pData, uint32_t Flags);
	int32_t NGSPICE_WriteMemEx(uint32_t Addr, uint32_t NumBytes, const void* p, uint32_t Flags);
	int32_t NGSPICE_EMU_GetList(int32_t HostIFs, NGSPICE_EMU_CONNECT_INFO* paConnectInfo, int32_t MaxInfos);
	int32_t NGSPICE_EMU_SelectByUSBSN(uint32_t SerialNo);
	int32_t NGSPICE_ExecCommand(const char* pIn, char* pOut, int32_t BufferSize);
	int32_t NGSPICE_TIF_Select(int32_t int32_terface);
	void NGSPICE_SetSpeed(uint32_t Speed);
	uint16_t NGSPICE_GetSpeed(void);
	int32_t NGSPICE_DEVICE_SelectDialog(void* hParent, uint32_t Flags, NGSPICE_DEVICE_SELECT_INFO* pInfo);
	int32_t NGSPICE_DEVICE_GetInfo(int32_t DeviceIndex, NGSPICE_DEVICE_INFO* pDeviceInfo);

	int32_t NGSPICE_HSS_Start(NGSPICE_HSS_MEM_BLOCK_DESC* paDesc, int32_t NumBlocks, int32_t Period_us, int32_t Flags);
	int32_t NGSPICE_HSS_Stop(void);
	int32_t NGSPICE_HSS_Read(void* pBuffer, uint32_t BufferSize);

	int32_t NGSPICE_SWO_Control(uint32_t Cmd, void* pData);

	int32_t NGSPICE_SWO_Config(const char* sConfig);
	int32_t NGSPICE_SWO_DisableTarget(uint32_t PortMask);
	int32_t NGSPICE_SWO_EnableTarget(uint32_t CPUSpeed, uint32_t SWOSpeed, int32_t Mode, uint32_t PortMask);
	int32_t NGSPICE_SWO_GetCompatibleSpeeds(uint32_t CPUSpeed, uint32_t MaxSWOSpeed, uint32_t* paSWOSpeed, uint32_t NumEntries);
	int32_t NGSPICE_SWO_Read(uint8_t* pData, uint32_t Offset, uint32_t* pNumBytes);

#if defined(__cplusplus)
} /* Make sure we have C-declarations in C++ programs */
#endif

#endif