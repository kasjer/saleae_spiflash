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
