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
	if (cmd->mHasAddr)
	{
		uint32_t addr = rand();
		GenerateByte(U8(addr >> 16), bits);
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
		n = 1 + rand() % 2;
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
		+ Cmd1(0x92, "MFID", "Read manufacturer, Device ID DUAL I/O") + DUAL_IO + ADDR + M + OP_DATA_READ
		+ Cmd1(0x94, "MFID", "Read manufacturer, Device ID QUAD I/O") + QUAD_IO + ADDR + M + DummyBytes(2) + OP_DATA_READ
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
		+ Cmd1(0x15, "RDCR", "Read configuration register") + RegisterRead("Configuration Register")
		+ Cmd1(0xB0, "SUSP", "Erase/Program Suspend")
		+ Cmd1(0x30, "RESM", "Erase/Program Resume")
		+ Cmd1(0xC0, "SBL", "Set Burst Length") + OP_DATA_WRITE
		+ Cmd1(0xB1, "ENSO", "Enter Secured OTP")
		+ Cmd1(0xC1, "EXSO", "Exit Secured OTP")
		+ Cmd1(0x2B, "RDSCUR", "Read Security Register")
		+ Cmd1(0x2F, "WRSCUR", "Write Security Register")
		+ Cmd1(0xAB, "RES", "Read Electronic ID") + DummyBytes(3) + OP_DATA_READ
		+ Cmd1(0x32, "QPP", "Quad Input Page Program") + QUAD_DATA + ADDR + OP_DATA_WRITE
		;
}

SpiFlash::SpiFlash() : mSpiMode(SPI_MODE0), mDefBusMode(SINGLE), mCurBusMode(SINGLE), mCurrentCmd(nullptr), mActiveCmdSet(nullptr), mDataIn(false)
{
	addCommands(*this);
}

SpiFlash spiFlash;

