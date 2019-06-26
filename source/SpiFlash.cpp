/*
MIT License

Copyright(c) 2017 Jerzy Kasenberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <cstdlib>
#include "SpiFlash.h"

void SpiFlash::GenerateByte(U8 b, std::vector<U8> &bits)
{
	// Bits for all the lines at once 0 - CS, 1 - CLK, 2-5 data bits
	U8 lines = 0;
	int n;
	// Mask for extracting bits x    1b    2b  x    4b
	static const U8 mask[5] = { 0, 0x80, 0xC0, 0, 0xF0 };
	// row 0 for output, row 1 for input
	// CS is on bit 0
	// CLK on bit 1
	// Data line are on bits 2-5 hence shifts
	static const U8 shif[2][5] = { { 0, 5, 4, 0, 2 }, {0, 4, 4, 0, 2} };

	for (int i = 0; i < 8; )
	{
		n = 0;
		// Extract as many bits as bus mode wants, they will be left aligned
		lines = b & mask[mCurBusMode];
		// shift right to bits 2-5 (for quad), 2-3 (for dual), o 2 or 3 for single mode
		lines >>= shif[mDataIn][mCurBusMode];

		b <<= mCurBusMode;
		i += mCurBusMode;
		// Add lines to history, CS is low all the time
		bits.push_back(CS_LOW | lines | CLOCK_LOW);
		// Add same lines with clock high
		bits.push_back(CS_LOW | lines | CLOCK_HIGH);
	}
}

void SpiFlash::GenerateCommandBits(SpiCmdData *cmd, std::vector<U8> &bits)
{
	int n;
	mDataIn = false;
	// Add some delay before CS goes low
	bits.push_back(Delay(3 + rand() % 10));

	// generate cmd bits if mActiveCmd in not set
	if (mCurrentCmd == nullptr)
	{
		GenerateByte(cmd->GetCode(), bits);
	}

	// if bus witdh changes after command code change current bus mode
	if (cmd->mModeArgs)
		mCurBusMode = BusMode(cmd->mModeArgs);

	// generate address
	if (cmd->mAddressBits)
	{
		uint32_t addr = rand();
		U32 addressBits = (cmd->mAddressBits != 0xFF) ? cmd->mAddressBits : mAddressBits;
		if (addressBits > 24)
			GenerateByte(U8(addr >> 24), bits);
		if (addressBits > 16)
			GenerateByte(U8(addr >> 16), bits);
		if (addressBits > 8)
			GenerateByte(U8(addr >> 8), bits);
		GenerateByte(U8(addr), bits);
	}

	// for continuous read mode command generate M bits
	if (cmd->mContinuousRead)
	{
		// 3/4 times stay in continuous read mode
		if (rand() % 4)
		{
			GenerateByte(0xAF, bits);
			mCurrentCmd = cmd;
		}
		else
		{
			// 1/4 times go to command mode again
			GenerateByte(0xFF, bits);
			mCurrentCmd = nullptr;
		}
	}

	// Add some dummy bytes if needed
	if (cmd->mDummyBytes)
	{
		for (int i = 0; i < cmd->mDummyCount; ++i)
			GenerateByte(0xFF, bits);
	}
	// or maybe some dummy cycles
	else if (cmd->mDummyCycles)
	{
		for (int i = 0; i < cmd->mDummyCount; ++i)
		{
			// Add lines to history
			bits.push_back(CS_LOW | MOSI_HIGH | MISO_HIGH | D2_HIGH | D3_HIGH | CLOCK_LOW);
			// Add same lines with clock high
			bits.push_back(CS_LOW | MOSI_HIGH | MISO_HIGH | D2_HIGH | D3_HIGH | CLOCK_HIGH);
		}
	}

	// Switch do other bus mode if this is 1-1-2 or 1-1-4 command
	if (cmd->mModeData)
		mCurBusMode = BusMode(cmd->mModeData);

	// Generate data for commands that have it
	mDataIn = (cmd->mCmdOp == OP_REG_READ || cmd->mCmdOp == OP_DATA_READ);
	switch (cmd->mCmdOp)
	{
	case OP_REG_READ:
	case OP_REG_WRITE:
		// For register read or write just one or to bytes
		n = (int)cmd->RegisterCount();
		break;
	case OP_DATA_READ:
	case OP_DATA_WRITE:
		// For data read or write generate up to 50 bytes
		n = 1 + rand() % 50;
		break;
	default:
		// Commands does not have any additional data
		n = 0;
	}

	// Genereate data
	for (int i = 0; i < n; ++i)
		GenerateByte(U8(rand()), bits);

	// If command changed bus mode update it
	// (comands like Enter/Exit QPI mode)
	if (cmd->mModeChange)
		mDefBusMode = BusMode(cmd->mModeChange);

	// Switch to default bus mode
	mCurBusMode = mDefBusMode;
}

void SpiFlash::GenerateRandomCommandBits(std::vector<U8> &bits)
{
	std::vector<U8> cmds;

	if (mCurrentCmd)
		GenerateCommandBits(mCurrentCmd, bits);
	else
	{
		GetValidCommands(cmds);
		SpiCmdData *cmd = mActiveCmdSet->GetCommand(mCurBusMode, cmds[rand() % cmds.size()]);
		if (cmd)
			GenerateCommandBits(cmd, bits);
	}
}

void addCommands(SpiFlash &spiFlash)
{
	spiFlash
		+ CommandSet(0, "not set")
		+ Register("Status Register-1", 8) + Bit(7, "SRP0") + Bit(1, "WEL") + Bit(0, "BUSY")
		+ Register("Status Register-2", 8) + Bit(7, "SUS") + Bit(1, "QE") + Bit(0, "SRP1")

		+ Cmd14(0x06, "WREN", "Write Enable")
		+ Cmd14(0x04, "WRDI", "Write Disable")
		+ Cmd14(0x05, "RDSR", "Read status register-1") + RegisterRead("Status Register-1")
		+ Cmd14(0x35, "RS2", "Read status register-2") + RegisterWrite("Status Register-2")
		+ Cmd14(0x01, "WS1", "Write status register-1") + RegisterWrite("Status Register-1") + RegisterWrite("Status Register-2") +
		+ Cmd14(0x31, "WS2", "Write status register-2") + RegisterWrite("Status Register-2")
		+ Cmd1(0x03, "R", "Read Data") + ADDR + OP_DATA_READ
		+ Cmd1(0x0B, "R", "Fast Read") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x3B, "R", "R 1-1-2", "Fast Read Dual Ouput") + ADDR + DummyBytes(1) + DUAL_DATA + OP_DATA_READ
		+ Cmd1(0x6B, "R", "R 1-1-4", "Fast Read Quad Output") + ADDR + DummyBytes(1) + QUAD_DATA + OP_DATA_READ
		+ Cmd1(0xBB, "R", "R 1-2-2", "Fast Read Dual I/O") + DUAL_IO + ADDR + M + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0xEB, "R", "R 1-4-4", "Fast Read Quad I/O") + QUAD_IO + ADDR + M + DummyBytes(2) + OP_DATA_READ
		+ Cmd14(0x02, "PP", "Page Program") + ADDR + OP_DATA_WRITE
		+ Cmd14(0x20, "SE", "Sector erase") + ADDR
		+ Cmd14(0x52, "BE", "Block erase") + ADDR
		+ Cmd14(0xD8, "BE", "BE64", "64KB Block erase") + ADDR
		+ Cmd14(0x60, "CE", "Chip erase")
		+ Cmd14(0xC7, "CE", "Chip erase")
		+ Cmd1(0x5A, "SFDP", "Read SFDP Register") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd14(0x75, "SUSP", "Erase/Program Suspend")
		+ Cmd14(0x7A, "RESM", "Erase/Program Resume")
		+ Cmd14(0xB9, "DN", "Power Down")
		+ Cmd14(0x9F, "JID", "Read JEDEC ID") + OP_DATA_READ
		+ Cmd1(0x90, "MFID", "Read manufacturer, Device ID") + ADDR + OP_DATA_READ
		+ Cmd14(0x66, "RSTEN", "Enable Reset")
		+ Cmd14(0x99, "RST", "Reset")
		+ Cmd14(0xAB, "UP", "Release Power Down") + DummyBytes(3) + OP_DATA_READ
		+ CommandSet(0xEF, "Winbond", 0)
		+ Register("Status Register-1", 8) + Bit(7, "SRP0") + Bit(6, "TPB") + Bit(5, "TP") + Bit(4, 2, "BPB") + Bit(1, "WEL") + Bit(0, "BUSY")
		+ Register("Status Register-2", 8) + Bit(7, "SUS") + Bit(6, "CMP") + Bit(5, 3, "LB") + Bit(1, "QE") + Bit(0, "SRP1")
		+ Register("Status Register-3", 8) + Bit(7, "HOLD/RESET") + Bit(6, 5, "DRV") + Bit(2, "WPS")

		+ Cmd14(0x50, "WRENVSR", "Write Enable for Volatile Status Register")
		+ Cmd14(0x35, "RS2", "Read status register-2") + RegisterRead("Status Register-2")
		+ Cmd14(0x15, "RS3", "Read status register-3") + RegisterRead("Status Register-3")
		+ Cmd14(0x01, "WS1", "Write status register-1") + RegisterWrite("Status Register-1") + RegisterWrite("Status Register-2")
		+ Cmd14(0x31, "WS2", "Write status register-2") + RegisterWrite("Status Register-2")
		+ Cmd14(0x11, "WS3", "Write status register-3") + RegisterWrite("Status Register-3")
		+ Cmd4(0xEB, "R", "R 1-4-4", "Fast Read Quad I/O") + QUAD_IO + ADDR + M + OP_DATA_READ
		+ Cmd14(0xE7, "R", "R 1-4-4", "Word Read Quad I/O") + QUAD_IO + ADDR + M + DummyBytes(1) + OP_DATA_READ
		+ Cmd14(0xE3, "R", "R 1-4-4", "Octal Word Read Quad I/O") + QUAD_IO + ADDR + M + OP_DATA_READ
		+ Cmd14(0x36, "Individual Block/Sector Lock") + ADDR
		+ Cmd14(0x39, "Individual Block/Sector Unlock") + ADDR
		+ Cmd14(0x3D, "Read Block/Sector Lock") + ADDR
		+ Cmd14(0x7E, "Global Block/Sector Lock")
		+ Cmd14(0x98, "Global Block/Sector Unlock")

		+ Cmd1(0x77, "Set Burst with Wrap") + QUAD_IO + DummyBytes(3) + OP_DATA_WRITE
		+ Cmd1(0x32, "QPP", "Quad Input Page Program") + QUAD_DATA + ADDR + OP_DATA_WRITE
		+ Cmd1(0x92, "MFID", "Read manufacturer, Device ID DUAL I/O") + DUAL_IO + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x94, "MFID", "Read manufacturer, Device ID QUAD I/O") + QUAD_IO + ADDR + DummyBytes(3) + OP_DATA_READ
		+ Cmd1(0x4B, "ID", "Read Unique ID number") + DummyBytes(4) + OP_DATA_READ
		+ Cmd1(0x44, "Erase Security Registers") + ADDR
		+ Cmd1(0x42, "Program Security Registers") + ADDR + OP_DATA_WRITE
		+ Cmd1(0x48, "Read Security Registers") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x38, "*4", "QPI", "Enter QPI Mode") + SET_QUAD

		+ Cmd4(0x0B, "R", "R 4-4-4", "Fast Read") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd4(0xC0, "SRP", "Set Read Parameters") + OP_DATA_WRITE
		+ Cmd4(0x0C, "BRW", "Burst Read with Wrap") + ADDR + M + DummyBytes(1) + OP_DATA_READ

		+ Cmd14(0xff, "*1", "Exit QPI Mode") + SET_SINGLE
		+ CommandSet(0xC2, "Macronix", 0)
		+ Register("Status Register-1", 8) + Bit(7, "SRWD") + Bit(6, "QE") + Bit(5, 2, "BPB") + Bit(1, "WEL") + Bit(0, "WIP")
		+ Register("Configuration Register-1", 8) + Bit(6, "DC") + Bit(3, "TB")
		+ Register("Configuration Register-2", 8) + Bit(1, "L/H")
		+ Register("Security Register", 8) + Bit(6, "E_FAIL") + Bit(5, "P_FAIL") + Bit(3, "ESB") + Bit(2, "PSB") + Bit(1, "LDSO") + Bit(0, "SOTP")
		+ Cmd1(0x01, "WSRS", "Write status register") + RegisterWrite("Status Register-1") + RegisterWrite("Configuration Register-1") + RegisterWrite("Configuration Register-2")
		+ Cmd1(0x05, "RDSR", "Read status register-1") + RegisterRead("Status Register-1")
		+ Cmd1(0x15, "RDCR", "Read configuration register") + RegisterRead("Configuration Register-1") + RegisterRead("Configuration Register-2")
		+ Cmd1(0xB0, "SUSP", "Erase/Program Suspend")
		+ Cmd1(0x30, "RESM", "Erase/Program Resume")
		+ Cmd1(0xC0, "SBL", "Set Burst Length") + OP_DATA_WRITE
		+ Cmd1(0xB1, "ENSO", "Enter Secured OTP")
		+ Cmd1(0xC1, "EXSO", "Exit Secured OTP")
		+ Cmd1(0x2B, "RDSCUR", "Read Security Register") + RegisterRead("Security Register")
		+ Cmd1(0x2F, "WRSCUR", "Write Security Register") + RegisterWrite("Security Register")
		+ Cmd1(0xAB, "RES", "Read Electronic ID") + DummyBytes(3) + OP_DATA_READ
		+ Cmd1(0x32, "QPP", "Quad Input Page Program") + QUAD_DATA + ADDR + OP_DATA_WRITE
		+ Cmd1(0x38, "QPP", "Quad I/O Page Program") + QUAD_IO + ADDR + OP_DATA_WRITE

		+ CommandSet(0xC8, "GigaDevice", 0xEF)
		+ Register("Status Register-1", 8) + Bit(7, "SRP0") + Bit(6, 2, "BPB") + Bit(1, "WEL") + Bit(0, "BUSY")
		+ Register("Status Register-2", 8) + Bit(7, "SUS1") + Bit(6, "CMP") + Bit(5, 3, "LB") + Bit(2, "SUS2") + Bit(1, "QE") + Bit(0, "SRP1")
		+ CommandSet(0x1F, "Adesto", 0)
		+ Cmd14(0xB1, "ENSO", "Enter Secured OTP")
		+ Cmd14(0xC1, "EXSO", "Exit Secured OTP")
		+ Cmd14(0x2B, "RDSCUR", "Read Security Register")
		+ Cmd14(0x2F, "WRSCUR", "Write Security Register")
		+ Cmd1(0x38, "*4", "QPI", "Enter QPI Mode") + SET_QUAD
		+ Cmd4(0xff, "*1", "Exit QPI Mode") + SET_SINGLE
		+ Cmd4(0x0C, "BRW", "Burst Read with Wrap") + ADDR + M + DummyBytes(1) + OP_DATA_READ
		+ Cmd4(0xC0, "SRP", "Set Read Parameters") + OP_DATA_WRITE
		+ Cmd14(0x33, "QPP", "Quad Input Page Program") + QUAD_DATA + ADDR + OP_DATA_WRITE
		+ Cmd1(0x94, "MFID", "Read manufacturer, Device ID QUAD I/O") + QUAD_IO + ADDR + DummyBytes(3) + OP_DATA_READ
		+ Cmd14(0xE7, "R", "R 1-4-4", "Word Read Quad I/O") + QUAD_IO + ADDR + M + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x77, "Set Burst with Wrap") + QUAD_IO + DummyBytes(3) + OP_DATA_WRITE

		+ CommandSet(0x01, "Cypress", 0)
		+ Register("Status Register-1", 8) + Bit(7, "SRP0") + Bit(6, "TPB") + Bit(5, "TP") + Bit(4, 2, "BPB") + Bit(1, "WEL") + Bit(0, "BUSY")
		+ Register("Status Register-2", 8) + Bit(7, "SUS") + Bit(6, "CMP") + Bit(5, 3, "LB") + Bit(1, "QE") + Bit(0, "SRP1")
		+ Register("Status Register-3", 8) + Bit(7, "RFU") + Bit(6, 5, "W6:5") + Bit(4, "W4") + Bit(3, 0, "LC")
		+ Cmd1(0x05, "RDSR1", "Read Status Register-1") + RegisterRead("Status Register-1")
		+ Cmd1(0x50, "WRENVSR", "Write Enable for Volatile Status Register")
		+ Cmd1(0x01, "WS1", "Write status registers") + RegisterWrite("Status Register-1") + RegisterWrite("Status Register-2") + RegisterWrite("Status Register-3") +
		+ Cmd1(0x07, "RDSR2", "Read Status Register-2") + RegisterRead("Status Register-2")
		+ Cmd1(0x35, "RDCR", "Read Configuration Register") + RegisterWrite("Status Register-2")
		+ Cmd1(0x33, "RDSR3", "Read Status register-3") + RegisterRead("Status Register-3")
		+ Cmd1(0x01, "WRR", "Write Status Registers") + RegisterWrite("Status Register-1")
		+ Cmd1(0xB9, "BRAC", "Bank Register Access")
		+ Cmd1(0x17, "BRWR", "Bank Register Write")
		+ Cmd1(0x18, "ECCRD", "ECC Statuc Register Read") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x14, "ABRD", "Auto Boot Register Read")
		+ Cmd1(0x14, "ABWR", "Auto Boot Register Write")
		+ Cmd1(0x43, "PNVDLR", "Programm NVDLR") + OP_DATA_WRITE
		+ Cmd1(0x4A, "WVDLR", "Write VDLR") + OP_DATA_WRITE
		+ Cmd1(0x41, "DLPRD", "Data Learning Patter Read") + OP_DATA_READ
		+ Cmd1(0x30, "CLSR", "Clear Status Register")
		+ Cmd1(0x77, "Set Burst with Wrap") + QUAD_IO + DummyBytes(3) + OP_DATA_WRITE
		+ Cmd1(0x39, "Set Block/Pointer protection") + ADDR
		+ Cmd1(0x48, "Read Security Registers") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x44, "Erase Security Registers") + ADDR
		+ Cmd1(0x42, "Program Security Registers") + ADDR + OP_DATA_WRITE

		+ CommandSet(0x9D, "Issi", 0xEF)
		+ Register("Function Register", 8) + Bit(7, "IRL3") + Bit(6, "IRL2") + Bit(5, "IRL1") + Bit(4, 2, "IRL0") + Bit(3, "ESUS") + Bit(2, "PSUS")
		+ Cmd1(0x48, "Read Function Register") + RegisterRead("Function Register")
		+ Cmd1(0x42, "Write Function Register") + RegisterWrite("Function Register")
		+ Cmd1(0x68, "IRRD", "Read Information Row") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd1(0x62, "IRP", "Information Row Program") + ADDR + OP_DATA_WRITE
		+ Cmd1(0x26, "SECUNLOCK", "Sector Unlock") + ADDR
		+ Cmd1(0x24, "SECLOCK", "Sector Lock") + ADDR
		+ Cmd1(0xD7, "SE", "Sector erase") + ADDR
		/* 0x38 Differes from Winbond */
		+ Cmd1(0x38, "QPP", "Quad Input Page Program") + QUAD_DATA + ADDR + OP_DATA_WRITE
		+ Cmd1(0xB0, "SUSP", "Erase/Program Suspend")
		+ Cmd1(0x30, "RESM", "Erase/Program Resume")

		+ CommandSet(0x20, "Micron", 0)
		+ Register("Nonvolatile Configuration Register", 16) + Bit(15, 12, "DCC") + Bit(11, 9, "XIPMODE") + Bit(8, 6, "ODS") + Bit(4, "Reset/Hold") + Bit(3, "QUAD") + Bit(2, "DUAL")
		+ Register("Volatile Configuration Register", 8) + Bit(7, 4, "DCC") + Bit(3, "XIP") + Bit(1, 0, "Wrap")
		+ Register("Enhanced Volatile Configuration Register", 8) + Bit(7, "QUAD") + Bit(6, "DUAL") + Bit(4, "Reset/Hold") + Bit(3, "VPPACC") + Bit(2, 0, "ODS")
		+ Register("Flag Status Register", 8) + Bit(7, "RDY") + Bit(6, "Erase suspend") + Bit(5, "Erase fail") + Bit(4, "Program fail") + Bit(3, "VPP fail") + Bit(2, "Program suspend") + Bit(1, "Protection fail")
		+ Register("Lock Register", 8) + Bit(1, "SLD") + Bit(0, "SWL")

		+ Cmd24(0xAF, "Multiple I/O READ ID") /* TODO: */
		+ Cmd124(0x5A, "SFDP", "Read SFDP Register") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd124(0x0B, "R", "Fast Read") + ADDR + DummyBytes(1) + OP_DATA_READ
		+ Cmd12(0x3B, "R", "R 1-1-2", "Fast Read Dual Ouput") + ADDR + DummyBytes(1) + DUAL_DATA + OP_DATA_READ
		+ Cmd12(0xBB, "R", "R 1-2-2", "Fast Read Dual I/O") + DUAL_IO + ADDR + M + DummyBytes(1) + OP_DATA_READ
		+ Cmd14(0x6B, "R", "R 1-1-4", "Fast Read Quad Output") + ADDR + DummyBytes(1) + QUAD_DATA + OP_DATA_READ
		+ Cmd14(0xEB, "R", "R 1-4-4", "Fast Read Quad I/O") + QUAD_IO + ADDR + M + DummyBytes(2) + OP_DATA_READ

		+ Cmd124(0x06, "WREN", "Write Enable")
		+ Cmd124(0x04, "WRDI", "Write Disable")
		+ Cmd124(0x05, "RDSR", "Read status register") + RegisterRead("Status Register-1")
		+ Cmd124(0x01, "WS1", "Write status register") + RegisterWrite("Status Register-1")
		+ Cmd124(0xE8, "RDLR", "Read lock register") + RegisterRead("Lock Register")
		+ Cmd124(0xE5, "WRLR", "Write lock register") + RegisterWrite("Lock Register")
		+ Cmd124(0x70, "RDFSR", "Read flag status register") + RegisterRead("Flag Status Register")
		+ Cmd124(0x50, "WRFSR", "Write flag status register") + RegisterWrite("Flag Status Register")
		+ Cmd124(0xB5, "RDNVCR", "Read nonvolatile configuration register") + RegisterRead("Nonvolatile Configuration Register")
		+ Cmd124(0xB1, "WRNVCR", "Write nonvolatile configuration register") + RegisterWrite("Nonvolatile Configuration Register")
		+ Cmd124(0x85, "RDVCR", "Read volatile configuration register") + RegisterRead("Volatile Configuration Register")
		+ Cmd124(0x81, "WRVCR", "Write volatile configuration register") + RegisterWrite("Volatile Configuration Register")
		+ Cmd124(0x85, "RDEVCR", "Read enhanced volatile configuration register") + RegisterRead("Enhanced Volatile Configuration Register")
		+ Cmd124(0x81, "WREVCR", "Write enhanced volatile configuration register") + RegisterWrite("Enhanced Volatile Configuration Register")

		+ Cmd124(0x02, "PP", "Page Program") + ADDR + OP_DATA_WRITE
		+ Cmd12(0xA2, "DPP", "Dual Input Fast Program") + DUAL_DATA + ADDR + OP_DATA_WRITE
		+ Cmd12(0xD2, "DPP", "Extended Dual Input Fast Program") + DUAL_IO + ADDR + OP_DATA_WRITE

		+ Cmd14(0x32, "QPP", "Quad Input Fast program") + DUAL_DATA + ADDR + OP_DATA_WRITE
		+ Cmd14(0x12, "QPP", "Extended Quad Input Fast Program") + DUAL_IO + ADDR + OP_DATA_WRITE

		+ Cmd124(0x20, "SSE", "Subsector erase") + ADDR
		+ Cmd124(0xD8, "SE", "Sector erase") + ADDR
		+ Cmd124(0xC7, "BE", "Bulk erase") + ADDR
		+ Cmd124(0x75, "SUSP", "Erase/Program Suspend")
		+ Cmd124(0x7A, "RESM", "Erase/Program Resume")

		+ Cmd124(0x75, "ROTP", "Read OTP Array") + OP_DATA_READ
		+ Cmd124(0x7A, "POTP", "Program OTP Array") + OP_DATA_WRITE

		+ CommandSet(0xBF, "Microchip", 0xEF)
		+ Register("Status Register", 8) + Bit(7, "BUSY") + Bit(5, "SEC") + Bit(4, "WPLD") + Bit(3, "WSP") + Bit(2, "WSE") + Bit(1, "WEL") + Bit(0, "BUSY")
		+ Register("Configuration Register", 8) + Bit(7, "WPEN") + Bit(3, "BPNV") + Bit(1, "IOC")
		+ Cmd1(0x05, "RDSR", "Read status register") + RegisterRead("Status Register")
		+ Cmd4(0x05, "RDSR", "Read status register") + DummyBytes(1) + RegisterRead("Status Register")
		+ Cmd1(0x35, "RDCR", "Read configuration register") + RegisterWrite("Configuration Register")
		+ Cmd4(0x35, "RDCR", "Read configuration register") + DummyBytes(1) + RegisterWrite("Configuration Register")
		+ Cmd4(0x0B, "R", "Fast Read") + ADDR + DummyBytes(3) + OP_DATA_READ
		+ Cmd1(0xEB, "R", "R 1-4-4", "Fast Read Quad I/O") + QUAD_IO + ADDR + M + DummyBytes(3) + OP_DATA_READ
		+ Cmd14(0xC0, "SB", "Set Burst Length") + OP_DATA_WRITE
		+ Cmd4(0x0C, "RBSQI", "Burst Read with Wrap") + ADDR + M + DummyBytes(3) + OP_DATA_READ
		+ Cmd1(0xEC, "RBSPI", "Burst Read with Wrap") + ADDR + M + DummyBytes(3) + OP_DATA_READ
		+ Cmd14(0xB0, "SUSP", "Erase/Program Suspend")
		+ Cmd14(0x30, "RESM", "Erase/Program Resume")

		+ Cmd1(0x72, "RBPR", "Read Block Protection Register") + OP_DATA_READ
		+ Cmd4(0x72, "RBPR", "Read Block Protection Register") + DummyBytes(1) + OP_DATA_READ
		+ Cmd14(0x8D, "LBPR", "Lock Down Block Protection Register")
		+ Cmd14(0xE8, "nVWLDR", "Non-volatile Write Lock Down Register") + OP_DATA_WRITE
		+ Cmd14(0x98, "ULBPR", "Global Block Protection Unlock")
		+ Cmd1(0x88, "RSID", "Read Security ID") + ADDR2 + DummyBytes(1) + OP_DATA_READ
		+ Cmd4(0x88, "RSID", "Read Security ID") + ADDR2 + DummyBytes(3) + OP_DATA_READ
		+ Cmd14(0xA5, "PSID", "Program User Security ID Area") + ADDR2 + OP_DATA_WRITE
		+ Cmd14(0x85, "LSID", "Lockout Security ID Programming")

		;
}

SpiFlash::SpiFlash() : mSpiMode(SPI_MODE0), mDefBusMode(SINGLE), mCurBusMode(SINGLE), mCurrentCmd(nullptr), mActiveCmdSet(nullptr), mDataIn(false), mAddressBits(24)
{
	addCommands(*this);
}

SpiFlash spiFlash;

