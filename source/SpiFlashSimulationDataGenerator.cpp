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
#include <algorithm>
#include <vector>
#include <map>
#include <list>

#include "SpiFlashSimulationDataGenerator.h"
#include "SpiFlashAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

#include "SpiFlash.h"

SpiFlashSimulationDataGenerator::SpiFlashSimulationDataGenerator()
{
}

SpiFlashSimulationDataGenerator::~SpiFlashSimulationDataGenerator()
{
}

void SpiFlashSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SpiFlashAnalyzerSettings* settings)
{
	double target_frequency = simulation_sample_rate / 10;
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;
	if (target_frequency > 104000000)
		target_frequency = 104000000;
	mClockGenerator.Init(target_frequency, simulation_sample_rate);
	spiFlash.SetSpiMode(mSettings->mSpiMode == 3 ? SPI_MODE3 : SPI_MODE0);
	spiFlash.SetCurrentCommand(nullptr);
	spiFlash.SetCurrentBusMode(BusMode(mSettings->mBusMode));
	spiFlash.SetDefaultBusMode(BusMode(mSettings->mBusMode));

	if (settings->mChipSelect.mChannelIndex < 1000)
		mChipSelectSimulationData = mSimulationChannels.Add(settings->mChipSelect, mSimulationSampleRateHz, BIT_HIGH);
	else
		mChipSelectSimulationData = nullptr;

	mClockSimulationData = mSimulationChannels.Add(settings->mClock, mSimulationSampleRateHz,
		(settings->mSpiMode == 3) ? BIT_HIGH : BIT_LOW);
	mMosiSimulationData = mSimulationChannels.Add(settings->mMosi, mSimulationSampleRateHz, BIT_HIGH);

	if (settings->mMiso.mChannelIndex < 1000)
		mMisoSimulationData = mSimulationChannels.Add(settings->mMiso, mSimulationSampleRateHz, BIT_HIGH);
	else
		mMisoSimulationData = nullptr;

	if (settings->mD2.mChannelIndex < 1000)
		mD2SimulationData = mSimulationChannels.Add(settings->mD2, mSimulationSampleRateHz, BIT_HIGH);
	else
		mD2SimulationData = nullptr;

	if (settings->mD3.mChannelIndex < 1000)
		mD3SimulationData = mSimulationChannels.Add(settings->mD3, mSimulationSampleRateHz, BIT_HIGH);
	else
		mD3SimulationData = nullptr;

	mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(100));
}

U32 SpiFlashSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mClockSimulationData->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		GenerateNext();
	}

	*simulation_channel = mSimulationChannels.GetArray();
	return mSimulationChannels.GetCount();
}

void SpiFlashSimulationDataGenerator::GenerateNext()
{
	BusMode mode = spiFlash.GetCurrentBusMode();
	U8 b;
	U8 IdleClockState = spiFlash.IdleClockState();

	if (mPendingBitsIx >= mPendingBits.size())
	{
		// Add some random delay before new command
		mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(10 + rand() % 10));

		mPendingBits.clear();
		// Set clock in neutral state before CS is activated
		mPendingBits.push_back(CS_HIGH | IdleClockState);
		// Activate CS
		mPendingBits.push_back(CS_LOW | IdleClockState);
		// Add some delay
		mPendingBits.push_back(spiFlash.Delay(1));
		// Generate command bits
		spiFlash.GenerateRandomCommandBits(mPendingBits);
		// Make sure clock goes to idle state
		mPendingBits.push_back(CS_LOW | IdleClockState);
		// Deactivate CS
		mPendingBits.push_back(CS_HIGH | IdleClockState);
		mPendingBitsIx = 0;
	}

	// Get next bits or delay
	b = mPendingBits[mPendingBitsIx++];
	if (b & HALF_CLOCK_DELAY)
		mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(b & 0x7F));
	else
	{
		// Set all bits and move half period forward
		setBit(mChipSelectSimulationData, b & CS_HIGH);
		setBit(mClockSimulationData, b & CLOCK_HIGH);
		setBit(mMosiSimulationData, b & MOSI_HIGH);
		setBit(mMisoSimulationData, b & MISO_HIGH);
		setBit(mD2SimulationData, b & D2_HIGH);
		setBit(mD3SimulationData, b & D3_HIGH);
		mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
	}
}
