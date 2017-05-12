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
#ifndef SPIFLASH_H
#define SPIFLASH_H

#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <list>

#include <LogicPublicTypes.h>

struct BitField
{
	std::string mFieldName;
	U8 mUpperBit;
	U8 mLowerBit;
	BitField(U8 pos, const char *name) : mUpperBit(pos), mLowerBit(pos), mFieldName(name) {}
	BitField(U8 upper, U8 lower, const char *name) : mUpperBit(upper), mLowerBit(lower), mFieldName(name) {}
	U32 GetValue(U64 reg) const { return U32((reg & ((1 << (mUpperBit + 1)) - 1)) >> mLowerBit); }
};

typedef BitField Bit;

class RegisterData
{
	std::vector<BitField> mBits;
	std::string mName;
	U8 mLen;
	U64 mValue;
public:
	RegisterData(const char *name, U8 len = 8) : mName(name), mLen(len) {}
	RegisterData(const RegisterData &o) : mBits(o.mBits), mName(o.mName), mLen(o.mLen) {}
	void SetLength(U8 len) { mLen = len; }
	const std::string GetName() const { return mName; }
	size_t GetBitfieldCount(void) { return mBits.size(); }
	const BitField &at(size_t ix) { return mBits.at(ix); }

	void AddBitField(const BitField &field)
	{
		mBits.push_back(field);
	}
	void AddBitRange(U8 n1, U8 n2, const char *name)
	{
		AddBitField(BitField(n1, n2, name));
	}
	void AddBit(U8 n, const char *name)
	{
		AddBitRange(n, n, name);
	}
	bool operator==(const std::string name)
	{
		return mName.compare(name) == 0;
	}
};

enum SpiMode
{
	SPI_MODE0,
	SPI_MODE1,
	SPI_MODE2,
	SPI_MODE3
};

enum BusMode
{
	UNDEFINED = 0,
	SINGLE = 1,
	DUAL = 2,
	QUAD = 4,
};

enum CmdMode
{
	CM_1 = BusMode::SINGLE,
	CM_2 = BusMode::DUAL,
	CM_4 = BusMode::QUAD,
	CM_12 = (BusMode::SINGLE | BusMode::DUAL),
	CM_14 = (BusMode::SINGLE | BusMode::QUAD),
	CM_124 = (BusMode::SINGLE | BusMode::DUAL | BusMode::QUAD),
	CM_ALL = CM_124,
};

enum GeneratedData
{
	CS_LOW = 0,
	CLOCK_LOW = 0,
	MOSI_LOW = 0,
	MISO_LOW = 0,
	D2_LOW = 0,
	D3_LOW = 0,
	CS_HIGH = 1,
	CLOCK_HIGH = 2,
	MOSI_HIGH = 4,
	MISO_HIGH = 8,
	D2_HIGH = 16,
	D3_HIGH = 32,
	HALF_CLOCK_DELAY = 128
};

enum CmdFeature
{
	ADDR,
	M,
	DUAL_IO,
	DUAL_DATA,
	QUAD_IO,
	QUAD_DATA,
	SET_SINGLE,
	SET_DUAL,
	SET_QUAD,
};

enum CmdOp
{
	OP_NO_DATA,
	OP_REG_READ,
	OP_REG_WRITE,
	OP_DATA_READ,
	OP_DATA_WRITE,
};

struct DummyBytes
{
	U8 mCnt;
	DummyBytes(U8 cnt) : mCnt(cnt) {}
};

struct DummyCycles
{
	U8 mCnt;
	DummyCycles(U8 cnt) : mCnt(cnt) {}
};

struct RegisterOp
{
	std::string mName;
	CmdOp mOp;
	RegisterOp(const char *name, CmdOp op) : mName(name), mOp(op) {}
};

static inline RegisterData *Register(const char *name, U8 length)
{
	return new RegisterData(name, length);
}

static inline RegisterOp RegisterRead(const char *name)
{
	return RegisterOp(name, CmdOp::OP_REG_READ);
}

static inline RegisterOp RegisterWrite(const char *name)
{
	return RegisterOp(name, CmdOp::OP_REG_WRITE);
}

class SpiCmdData
{
public:
	U8 mCode;
	CmdMode mMode;
	CmdOp mCmdOp;
	bool mHasAddr;
	bool mContinuousRead;
	bool mDummyBytes;
	bool mDummyCycles;
	U8 mDummyCount;
	U8 mModeChange;
	U8 mModeArgs;
	U8 mModeData;
	std::vector<std::string> mNames;
	std::vector<RegisterData *> mRegs;
public:
	SpiCmdData(U8 code, CmdMode mode, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) : mCode(code), mMode(mode),
		mCmdOp(OP_NO_DATA), mHasAddr(false), mDummyBytes(false), mDummyCycles(false), mContinuousRead(false),
		mModeChange(0), mModeArgs(0), mModeData(0)
	{
		mNames.push_back(std::string(n1));
		if (n2)
			mNames.push_back(std::string(n2));
		if (n3)
			mNames.push_back(std::string(n3));
	}
	~SpiCmdData() {}

	U8 GetCode() const { return mCode; }

	bool IsSingle() const { return (mMode & CmdMode::CM_1) != 0; }
	bool IsDual() const { return (mMode & CmdMode::CM_2) != 0; }
	bool IsQuad() const { return (mMode & CmdMode::CM_4) != 0; }
	bool IsValidForMode(BusMode mode) const { return (mMode & mode) != 0; }

	void AddName(const char *name) { mNames.push_back(name); }
	void AddReg(RegisterData *reg) { mRegs.push_back(reg); }
	RegisterData *GetRegister(size_t ix) { return mRegs.size() ? mRegs.at(ix % mRegs.size()) : nullptr; }
	size_t RegisterCount() const { return mRegs.size(); }

	void Set(CmdFeature feature)
	{
		switch (feature)
		{
		case ADDR:
			mHasAddr = true;
			break;
		case M:
			mContinuousRead = true;
			break;
		case DUAL_IO:
			mModeArgs = 2;
			break;
		case DUAL_DATA:
			mModeData = 2;
			break;
		case QUAD_IO:
			mModeArgs = 4;
			break;
		case QUAD_DATA:
			mModeData = 4;
			break;
		case SET_SINGLE:
			mModeChange = CM_1;
			break;
		case SET_DUAL:
			mModeChange = CM_2;
			break;
		case SET_QUAD:
			mModeChange = CM_4;
			break;
		default:
			break;
		}
	}
	void Set(CmdOp op) { mCmdOp = op; }
	void Set(const DummyBytes &db) { mDummyCount = db.mCnt; mDummyBytes = true; mDummyCycles = false; }
	void Set(const DummyCycles &db) { mDummyCount = db.mCnt; mDummyBytes = false; mDummyCycles = true; }
};

class CmdSet
{
	typedef std::map<U16, SpiCmdData *> CommandMap;
	std::vector<RegisterData *> mRegisters;
	CommandMap mCommandMap;
	std::vector<std::auto_ptr<SpiCmdData>> mCommands;
	CmdSet *mParent;
	std::string mName;
	int mId;
public:
	int GetId() const { return mId; }
	const std::string &GetName() const { return mName; }
	CmdSet(int id, const std::string name, CmdSet *parent = nullptr) : mId(id), mName(name), mParent(parent) {}
	void AddRegister(RegisterData *reg) { mRegisters.push_back(reg); }
	RegisterData *GetRegister(const char *name)
	{
		std::vector<RegisterData *>::iterator i;
		for (i = mRegisters.begin(); i != mRegisters.end(); ++i)
			if ((*i)->GetName().compare(name) == 0)
				break;

		if (i != mRegisters.end())
			return *i;

		AddRegister(new RegisterData(name, 8));
		return mRegisters.back();
	}
	void AddRegisterField(const BitField &field)
	{
		if (mRegisters.size())
			mRegisters.back()->AddBitField(field);
	}

	void AddCommand(SpiCmdData *cmd)
	{
		mCommands.push_back(std::auto_ptr<SpiCmdData>(cmd));
		if (cmd->IsSingle())
			mCommandMap[(uint16_t)cmd->GetCode()] = cmd;
		if (cmd->IsDual())
			mCommandMap[0x200 + cmd->GetCode()] = cmd;
		if (cmd->IsQuad())
			mCommandMap[0x400 + cmd->GetCode()] = cmd;
	}
	void SetParent(CmdSet *parent) { mParent = parent; }
	void GetValidCommands(BusMode busMode, std::vector<U8> &cmds) const
	{
		// Get commands from parnet first
		if (mParent)
			mParent->GetValidCommands(busMode, cmds);

		// Add commands that are valid for specifed bus mode
		for (size_t i = 0; i < mCommands.size(); ++i)
			if (mCommands[i]->IsValidForMode(busMode))
				cmds.push_back(mCommands[i]->GetCode());

		// If parent existed, sort and remove duplicates
		if (mParent)
		{
			std::sort(cmds.begin(), cmds.end());
			std::unique(cmds.begin(), cmds.end());
		}
	}
	SpiCmdData *GetCommand(BusMode mode, U8 code)
	{
		int key;
		switch (mode)
		{
		default:
		case SINGLE:
			key = code;
			break;
		case DUAL:
			key = 0x200 + code;
			break;
		case QUAD:
			key = 0x400 + code;
			break;
		}
		CommandMap::iterator i = mCommandMap.find(key);
		if (i != mCommandMap.end())
		{
			return i->second;
		}
		else if (mParent)
			return mParent->GetCommand(mode, code);
		else
		{
			return nullptr;
		}
	}
};

struct CommandSet
{
	int mId;
	int mParentId;
	std::string mName;
public:
	CommandSet(int id, const char *name, int parentId = -1) : mId(id), mName(name), mParentId(parentId) {}
};

class SpiFlash
{
	typedef std::vector<CmdSet *> CommandSets;
	CommandSets mCmdSets;
	CmdSet *mActiveCmdSet;
	SpiCmdData *mCurrentCmd;

	BusMode mCurBusMode;
	BusMode mDefBusMode;
	SpiMode mSpiMode;
	bool mDataIn;
public:
	void SetSpiMode(SpiMode mode) { mSpiMode = mode; }
	U8 IdleClockState() const { return mSpiMode < 2 ? CLOCK_LOW : CLOCK_HIGH; }
	U8 Delay(U8 halfClocks) { return halfClocks + HALF_CLOCK_DELAY; }
	SpiFlash();
	void SetDefaultBusMode(BusMode mode) { mDefBusMode = mode; }
	void SetCurrentBusMode(BusMode mode) { if (mode) mCurBusMode = mode; }
	BusMode GetCurrentBusMode() const { return mCurBusMode; }
	BusMode GetDefaultBusMode() const { return mDefBusMode; }
	void GenerateByte(U8 b, std::vector<U8> &bits);
	void GenerateCommandBits(SpiCmdData *cmd, std::vector<U8> &bits);
	void GenerateRandomCommandBits(std::vector<U8> &bits);
	SpiCmdData *GetCurrentCommand() const { return mCurrentCmd; }

	void SetCurrentCommand(SpiCmdData *cmd) { mCurrentCmd = cmd; }
	SpiCmdData *GetCommand(BusMode mode, U8 code)
	{
		return mActiveCmdSet ? mActiveCmdSet->GetCommand(mode, code) : nullptr;
	}
	CmdSet *GetCommandSet(uint8_t id)
	{
		CommandSets::iterator i;
		for (i = mCmdSets.begin(); i != mCmdSets.end(); ++i)
			if ((*i)->GetId() == id)
				return *i;
		return nullptr;
	}
	const std::vector<CmdSet *> &getCommandSets() const { return mCmdSets; }
	void GetValidCommands(std::vector<U8> &cmds) const
	{
		cmds.clear();
		if (mActiveCmdSet)
			mActiveCmdSet->GetValidCommands(mCurBusMode, cmds);
	}
	void SelectCmdSet(uint8_t id)
	{
		mActiveCmdSet = GetCommandSet(id);
	}

	SpiFlash &operator+(const CommandSet &cmdSet)
	{
		mActiveCmdSet = new CmdSet(cmdSet.mId, cmdSet.mName, GetCommandSet(cmdSet.mParentId));

		mCmdSets.push_back(mActiveCmdSet);

		return *this;
	}
	SpiFlash &operator+(RegisterData *reg)
	{
		if (mActiveCmdSet)
			mActiveCmdSet->AddRegister(reg);
		else
			delete reg;
		return *this;
	}
	SpiFlash &operator+(const RegisterOp &reg)
	{
		if (mActiveCmdSet && mCurrentCmd)
		{
			RegisterData *regData = mActiveCmdSet->GetRegister(reg.mName.c_str());
			if (regData)
			{
				mCurrentCmd->AddReg(regData);
				mCurrentCmd->Set(reg.mOp);
			}
		}
		return *this;
	}
	SpiFlash &operator+(CmdFeature feature)
	{
		if (mCurrentCmd)
			mCurrentCmd->Set(feature);

		return *this;
	}
	SpiFlash &operator+(CmdOp op)
	{
		if (mCurrentCmd)
			mCurrentCmd->Set(op);

		return *this;
	}
	SpiFlash &operator+(const DummyBytes &db)
	{
		if (mCurrentCmd)
			mCurrentCmd->Set(db);

		return *this;
	}
	SpiFlash &operator+(const DummyCycles &dc)
	{
		if (mCurrentCmd)
			mCurrentCmd->Set(dc);
	}
	SpiFlash &operator+(const BitField &field)
	{
		if (mActiveCmdSet)
			mActiveCmdSet->AddRegisterField(field);
		return *this;
	}
	SpiFlash &operator+(SpiCmdData *cmd)
	{
		if (mActiveCmdSet)
		{
			mActiveCmdSet->AddCommand(cmd);
			mCurrentCmd = cmd;
		}
		else
		{
			delete cmd;
		}

		return *this;
	}
};

static SpiCmdData *Cmd(U8 ins, CmdMode mode, const char *n1, const char *n2, const char *n3)
{
	return new SpiCmdData(ins, mode, n1, n2, n3);
}

static SpiCmdData *Cmd1(U8 ins, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) { return Cmd(ins, CM_1, n1, n2, n3); }
static SpiCmdData *Cmd2(U8 ins, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) { return Cmd(ins, CM_2, n1, n2, n3); }
static SpiCmdData *Cmd12(U8 ins, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) { return Cmd(ins, CM_12, n1, n2, n3); }
static SpiCmdData *Cmd4(U8 ins, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) { return Cmd(ins, CM_4, n1, n2, n3); }
static SpiCmdData *Cmd14(U8 ins, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) { return Cmd(ins, CM_14, n1, n2, n3); }
static SpiCmdData *Cmd124(U8 ins, const char *n1, const char *n2 = nullptr, const char *n3 = nullptr) { return Cmd(ins, CM_124, n1, n2, n3); }

extern SpiFlash spiFlash;

#endif //SPIFLASH_H
