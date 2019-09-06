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
#include "SpiFlashAnalyzer.h"
#include "SpiFlashAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include "SpiFlashAnalyzerResults.h"

#include "SpiFlash.h"

SpiFlashAnalyzer::SpiFlashAnalyzer()
	: Analyzer2(),
	mSettings(new SpiFlashAnalyzerSettings()),
	mSimulationInitilized(false)
{
	SetAnalyzerSettings(mSettings.get());
}

SpiFlashAnalyzer::~SpiFlashAnalyzer()
{
	KillThread();
}

AnalyzerChannelData *SpiFlashAnalyzer::GetAnalyzerChannelData(Channel& channel)
{
	if (channel == UNDEFINED_CHANNEL)
		return nullptr;
	else
		return Analyzer::GetAnalyzerChannelData(channel);
}

void SpiFlashAnalyzer::SetupResults()
{
	mResults.reset(new SpiFlashAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());

	// Whole command goes to CS or clock
	if (mSettings->mChipSelect != UNDEFINED_CHANNEL)
		mResults->AddChannelBubblesWillAppearOn(mSettings->mChipSelect);
	else
		mResults->AddChannelBubblesWillAppearOn(mSettings->mClock);

	if (mSettings->mMosi != UNDEFINED_CHANNEL)
		mResults->AddChannelBubblesWillAppearOn(mSettings->mMosi);

	if (mSettings->mMiso != UNDEFINED_CHANNEL)
		mResults->AddChannelBubblesWillAppearOn(mSettings->mMiso);
}

void SpiFlashAnalyzer::AddFrame(U64 start, U64 end, U64 d1, U64 d2, U8 type, U8 flags)
{
	Frame f;
	f.mStartingSampleInclusive = S64(start);
	f.mEndingSampleInclusive = S64(end);
	f.mData1 = d1;
	f.mData2 = d2;
	f.mFlags = flags;
	f.mType = type;

	mResults->AddFrame(f);
	mResults->CommitResults();
}

// TODO: Remove this once there is no going back in time
U64 pos;

void SpiFlashAnalyzer::Setup()
{
	mLockedCmd = nullptr;
	mChipSelect = GetAnalyzerChannelData(mSettings->mChipSelect);
	mClock = GetAnalyzerChannelData(mSettings->mClock);
	mMosi = GetAnalyzerChannelData(mSettings->mMosi);
	mMiso = GetAnalyzerChannelData(mSettings->mMiso);
	mD2 = GetAnalyzerChannelData(mSettings->mD2);
	mD3 = GetAnalyzerChannelData(mSettings->mD3);
	if (mSettings->mSpiMode == 0)
		mClockIdleState = BIT_LOW;
	else if (mSettings->mSpiMode == 3)
		mClockIdleState = BIT_HIGH;

	mDefaultBusMode = BusMode(mSettings->mBusMode);
	mCurrentBusMode = mDefaultBusMode;
	// Continues read mode selected as starting point
	U8 manufacturer = (U8)(mSettings->mContinuousRead >> 8);
	U8 code = (U8)mSettings->mContinuousRead;
	CmdSet *cmdSet = spiFlash.GetCommandSet(manufacturer);
	if (cmdSet)
	{
		SpiCmdData *cmd = cmdSet->GetCommand(mCurrentBusMode, code);
		if (cmd != NULL)
		{
			mLockedCmd = cmd;
			mCurrentBusMode = BusMode(cmd->mModeData);
		}
	}
	mCachedClockCount = 0;
	pos = 0;
}

void SpiFlashAnalyzer::AdvanceDataToAbsPosition(U64 AbsolutePosition)
{
	if (pos > AbsolutePosition)
	{
		return;
	}
	pos = AbsolutePosition;
	if (mMosi) mMosi->AdvanceToAbsPosition(AbsolutePosition);
	if (mMiso) mMiso->AdvanceToAbsPosition(AbsolutePosition);
	if (mD2) mD2->AdvanceToAbsPosition(AbsolutePosition);
	if (mD3) mD3->AdvanceToAbsPosition(AbsolutePosition);
}

void SpiFlashAnalyzer::CacheDropOlderClocks(U64 limit)
{
	int i;

	for (i = 0; i < mCachedClockCount; ++i)
	{
		if (mCachedClocks[i] >> 1 >= limit)
		{
			if (i > 0)
			{
				memmove(mCachedClocks, mCachedClocks + i,
					(mCachedClockCount - i) * sizeof(mCachedClocks[0]));
			}
			break;
		}
	}
	mCachedClockCount -= i;
}

void SpiFlashAnalyzer::CacheClock(int num, U64 lowerLimit)
{
	if (lowerLimit)
		CacheDropOlderClocks(lowerLimit);

	// No cached clocks, move clock forward
	if (mClock->GetSampleNumber() < lowerLimit)
		mClock->AdvanceToAbsPosition(lowerLimit);

	while (mCachedClockCount < num && mClock->DoMoreTransitionsExistInCurrentData())
	{
		mClock->AdvanceToNextEdge();
		if (mClock->GetBitState() == BIT_HIGH)
			mCachedClocks[mCachedClockCount++] = (mClock->GetSampleNumber() << 1) + 1;
		else
			mCachedClocks[mCachedClockCount++] = mClock->GetSampleNumber() << 1;
	}
}

void SpiFlashAnalyzer::AdvanceToCommandStart()
{
	// If CS is present just move to next falling edge
	if (mChipSelect != NULL)
	{
		if (mChipSelect->GetBitState() == BIT_HIGH)
		{
			mChipSelect->AdvanceToNextEdge();
		}
		else
		{
			mChipSelect->AdvanceToNextEdge();
			mChipSelect->AdvanceToNextEdge();
		}
		mCommandStart = mChipSelect->GetSampleNumber();

		CacheClock(16, mCommandStart);
		if (mCachedClockCount > 0)
		{
			bool clockHigh = mCachedClocks[0] & 1;
			// If mode is 0 or 3 and clock state is not matching mark error
			if ((mSettings->mSpiMode == 0 && !clockHigh) ||
				(mSettings->mSpiMode == 3 && clockHigh))
			{
				mResults->AddMarker(mCommandStart, AnalyzerResults::ErrorSquare, mSettings->mClock);
			}
			else if (mSettings->mSpiMode == 0xFF)
				// For auto mode take current clock state as idle state
				mClockIdleState = clockHigh ? BIT_HIGH : BIT_LOW;
		}

		// Command ends at next rising edge of CS or at the end of data
		if (mChipSelect->DoMoreTransitionsExistInCurrentData())
		{
			mChipSelect->AdvanceToNextEdge();
			mCommandEnd = mChipSelect->GetSampleNumber();
		}
		else
			mCommandEnd = ~0;
	}
	else
	{
		// TODO: Rethink clocks !!!
		// Hardware generated clock should have some pattern
		U64 edges[10];

		if (mSettings->mSpiMode == 0 && mClock->GetBitState() == BIT_HIGH)
			mClock->AdvanceToNextEdge();
		else if (mSettings->mSpiMode == 3 && mClock->GetBitState() == BIT_LOW)
			mClock->AdvanceToNextEdge();
		else
			mClockIdleState = BIT_LOW;

		U64 sample = mClock->GetSampleNumber();
		// Assume that clock is in idle now
		mClock->AdvanceToNextEdge();
		edges[0] = mClock->GetSampleNumber(); // rising edge
		mClock->AdvanceToNextEdge();
		edges[1] = mClock->GetSampleNumber(); // falling edge
		while (true)
		{
			mClock->AdvanceToNextEdge();
			edges[2] = mClock->GetSampleNumber(); // rising edge
			mClock->AdvanceToNextEdge();
			edges[3] = mClock->GetSampleNumber(); // falling edge
			int d1 = int(edges[1] - edges[0]);
			int d2 = int(edges[2] - edges[1]);
			int d3 = int(edges[3] - edges[2]);
			if (d3 == 0)
				return;
			// If positive pulses differe more then 10 % and 2 samples
			// or negative puls differes from positive more than 30 % and 2 samples
			// lets move to place where clock is more stable
			if ((abs(d1 - d3) > 2 && (abs(d1 - d2) > d1 / 10)) ||
				(abs(d2 - d1) > 2 && (abs(d1 - d2) > d1 / 30)))
			{
				edges[0] = edges[2];
				edges[1] = edges[3];
				continue;
			}
			mClock->AdvanceToAbsPosition(edges[0]);
			mCommandStart = mClock->GetSampleNumber();
			break;
		}
	}
	AdvanceDataToAbsPosition(mCommandStart);
}

U8 SpiFlashAnalyzer::GetBits(BusMode busMode, bool dirIn)
{
	U8 b = 0;

	if (busMode == SINGLE)
	{
		if (dirIn)
		{
			if (mMiso)
				b = mMiso->GetBitState() == BIT_HIGH ? 1 : 0;
		}
		else
		{
			if (mMosi)
				b = mMosi->GetBitState() == BIT_HIGH ? 1 : 0;
		}
	}
	else
	{
		if (mMosi)
			b = mMosi->GetBitState() == BIT_HIGH ? 1 : 0;
		if (mMiso)
			b |= mMiso->GetBitState() == BIT_HIGH ? 2 : 0;
	}
	if (busMode == QUAD)
	{
		if (mD2)
			b |= mD2->GetBitState() == BIT_HIGH ? 4 : 0;
		if (mD3)
			b |= mD3->GetBitState() == BIT_HIGH ? 8 : 0;
	}

	return b;
}

int SpiFlashAnalyzer::ExtractBits(U64 &start, U64 &end, U32 &val, U8 neededBits)
{
	BusMode busMode = mCurrentBusMode;
	U8 bitCount = 0;
	val = 0;
	int i;
	int clockEdgesPerByte = 2 * neededBits / busMode;

	CacheClock(clockEdgesPerByte, mCommandStart);

	// Start time of first clock edge (rising or falling)
	start = mCachedClocks[0] >> 1;

	// Not enough clocks to form a byte, and those clocks are in active CS?
	if (mCachedClockCount < clockEdgesPerByte || (mCachedClocks[clockEdgesPerByte - 1] >> 1) > mCommandEnd)
	{
		if (mCachedClockCount)
		{
			end = mCachedClocks[mCachedClockCount - 1] >> 1;
			if (end > mCommandEnd)
				end = mCommandEnd;
		}
		return -1;
	}

	// Let i point to rising edge time in table
	i = (mCachedClocks[0] & 1) ? 0 : 1;

	while (bitCount < neededBits)
	{
		AdvanceDataToAbsPosition(mCachedClocks[i] >> 1);
		mResults->AddMarker(mCachedClocks[i] >> 1, AnalyzerResults::UpArrow, mSettings->mClock);
		val <<= busMode;
		val |= GetBits(busMode, mDirIn);
		bitCount += busMode;
		i += 2;
	}
	end = mCachedClocks[clockEdgesPerByte - 1] >> 1;
	CacheDropOlderClocks(end + 1);

	return 0;
}

int SpiFlashAnalyzer::ExtractMosiMiso(U64 &start, U64 &end, U8 &mosi, U8 &miso)
{
	BusMode busMode = mCurrentBusMode;
	U8 bitCount;
	int i;
	int ret = 0;
	int clocksPerByte = 8 * 2;
	mosi = 0;
	miso = 0;

	CacheClock(clocksPerByte, mCommandStart);

	// Start time of first clock edge (rising or falling)
	start = mCachedClocks[0] >> 1;

	// Not enough clocks to form a byte, and those clocks are in active CS?
	if (mCachedClockCount < clocksPerByte || (mCachedClocks[clocksPerByte - 1] >> 1) > mCommandEnd)
	{
		end = mCachedClocks[mCachedClockCount - 1] >> 1;
		if (end > mCommandEnd)
			end = mCommandEnd;
		ret = -1;
	}
	else
	{
		// Let i point to rising edge time in table
		i = (mCachedClocks[0] & 1) ? 0 : 1;

		for (bitCount = 0; bitCount < 8; ++bitCount, i += 2)
		{
			AdvanceDataToAbsPosition(mCachedClocks[i] >> 1);
			if (mMosi)
				mosi = (mosi << 1) + (mMosi->GetBitState() == BIT_HIGH ? 1 : 0);
			if (mMiso)
				miso = (miso << 1) + (mMiso->GetBitState() == BIT_HIGH ? 2 : 0);
		}
		end = mCachedClocks[clocksPerByte - 1] >> 1;
	}
	CacheDropOlderClocks(end + 1);

	return ret;
}

void SpiFlashAnalyzer::AnalyzeCommandBits()
{
	int b;

	union
	{
		SpiCmdData *data;
		intptr_t code;
	} cmd;
	U64 cmdExtra;

	U32 val;

	U32 addr = 0;

	U64 start;
	U64 end;

	U8 m;

	cmd.data = nullptr;
	mResults->CommitPacketAndStartNewPacket();

	mDirIn = false;

	do
	{
		cmdExtra = 0;
		if (mLockedCmd != nullptr)
			cmd.data = mLockedCmd;
		else
		{
			b = ExtractBits(start, end, val, 8);
			if (b < 0)
			{
				// Not enough bits for decoding command set value that is more then byte
				// but not enough for valid pointer
				cmd.code = 0x100;
				break;
			}

			cmd.data = spiFlash.GetCommand(mCurrentBusMode, U8(val));
			if (cmd.data == nullptr)
				cmd.code = (int)val;

			// Add command to MOSI line
			AddFrame(start, end, val, reinterpret_cast<U64>(cmd.data), FT_CMD_BYTE, 0);
		}

		if (cmd.code > 0x100)
		{
			UpdateBusMode((BusMode)cmd.data->mModeArgs);
			if (cmd.data->mAddressBits)
			{
				U32 addressLength = (cmd.data->mAddressBits != 0xFF) ? cmd.data->mAddressBits  : mSettings->mAddressLength;
				addr = 0;
				if (ExtractBits(start, end, addr, addressLength) < 0)
					break;
				AddFrame(start, end, addr, 0, FT_OUT_ADDR24, 0);
				cmdExtra = U64(addr) << 24;
			}
			if (cmd.data->mContinuousRead)
			{
				if (ExtractBits(start, end, val, 8) < 0)
					break;
				m = U8(val);
				mLockedCmd = ((m & 0x30) == 0x20) ? cmd.data : nullptr;
				AddFrame(start, end, val, 0, FT_M, 0);
			}

			U64 dummyStart = 0;
			U64 dummyEnd = 0;

			if (cmd.data->mDummyBytes)
			{
				if (ExtractBits(dummyStart, dummyEnd, val, cmd.data->mDummyCount * 8) < 0)
					break;
			}
			else if (cmd.data->mDummyCycles)
			{
				if (ExtractBits(dummyStart, dummyEnd, val, cmd.data->mDummyCycles) < 0)
					break;
			}

			// Dummy cycles or byte found
			if (dummyEnd)
			{
				AddFrame(dummyStart, dummyEnd, val, 0, FT_DUMMY, 0);
				end = dummyEnd;
			}

			// Change bus mode if command require change for data phase
			UpdateBusMode(BusMode(cmd.data->mModeData));

			switch (cmd.data->mCmdOp)
			{
			case OP_DATA_WRITE:
				while (ExtractBits(start, end, val, 8) >= 0)
				{
					AddFrame(start, end, val, 0, FT_OUT_BYTE, 0);
					cmdExtra++;
				}
				break;
			case OP_DATA_READ:
				mDirIn = true;
				while (ExtractBits(start, end, val, 8) >= 0)
				{
					AddFrame(start, end, 0, val, FT_IN_BYTE, 0);
					cmdExtra++;
				}
				break;
			case OP_REG_WRITE:
				while (ExtractBits(start, end, val, 8) >= 0)
				{
					AddFrame(start, end, val,
						reinterpret_cast<U64>(cmd.data->GetRegister(size_t(cmdExtra))), FT_OUT_REG, 0);
					cmdExtra++;
				}
				break;
			case OP_REG_READ:
				mDirIn = true;
				while (ExtractBits(start, end, val, 8) >= 0)
				{
					AddFrame(start, end, reinterpret_cast<U64>(cmd.data->GetRegister(size_t(cmdExtra))),
						val, FT_IN_REG, 0);
					cmdExtra++;
				}
				break;
			}
			// Commands like Enter QPI or Exit QPI change bus mode
			if (cmd.data->mModeChange)
				mDefaultBusMode = BusMode(cmd.data->mModeChange);
		}
		else if (cmd.code < 0x100)
		{
			U8 miso, mosi;
			while (ExtractMosiMiso(start, end, mosi, miso) >= 0)
				AddFrame(start, end, mosi, miso, FT_IN_OUT, 0);
		}
	} while (0);

	if (cmd.code != 0x100)
	{
		AddFrame(mCommandStart, mCommandEnd, cmdExtra, reinterpret_cast<U64>(cmd.data), FT_CMD, 0);
		ReportProgress(mCommandEnd);
	}


	// Set default bus mode
	mCurrentBusMode = mDefaultBusMode;
}

void SpiFlashAnalyzer::WorkerThread()
{
	Setup();

	for (;;)
	{
		AdvanceToCommandStart();
		AnalyzeCommandBits();
		CheckIfThreadShouldExit();
	}
}

bool SpiFlashAnalyzer::NeedsRerun()
{
	return false;
}

U32 SpiFlashAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (mSimulationInitilized == false)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 SpiFlashAnalyzer::GetMinimumSampleRateHz()
{
	return 100000;
}

const char* SpiFlashAnalyzer::GetAnalyzerName() const
{
	return "SPI Flash";
}

const char* GetAnalyzerName()
{
	return "SPI Flash";
}

Analyzer* CreateAnalyzer()
{
	return new SpiFlashAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
