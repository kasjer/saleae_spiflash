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
#ifndef SPIFLASH_SIMULATION_DATA_GENERATOR
#define SPIFLASH_SIMULATION_DATA_GENERATOR

#include <string>
#include <SimulationChannelDescriptor.h>
#include <AnalyzerHelpers.h>

class SpiFlashAnalyzerSettings;

class SpiFlashSimulationDataGenerator
{
	// Bit sequence generated for command
	std::vector<U8> mPendingBits;
	// Current index in mPendingBits sequence
	size_t mPendingBitsIx;

	void GenerateNext();
	void setBit(SimulationChannelDescriptor *channel, U8 high)
	{
		if (channel)
			channel->TransitionIfNeeded(high ? BIT_HIGH : BIT_LOW);
	}
public:
	SpiFlashSimulationDataGenerator();
	~SpiFlashSimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, SpiFlashAnalyzerSettings* settings);
	U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel);

protected:
	ClockGenerator mClockGenerator;
	SpiFlashAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	SimulationChannelDescriptorGroup mSimulationChannels;

	SimulationChannelDescriptor *mChipSelectSimulationData;
	SimulationChannelDescriptor *mClockSimulationData;
	SimulationChannelDescriptor *mMosiSimulationData;
	SimulationChannelDescriptor *mMisoSimulationData;
	SimulationChannelDescriptor *mD2SimulationData;
	SimulationChannelDescriptor *mD3SimulationData;
};
#endif //SPIFLASH_SIMULATION_DATA_GENERATOR
