#ifndef SPIFLASH_SIMULATION_DATA_GENERATOR
#define SPIFLASH_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class SpiFlashAnalyzerSettings;

class SpiFlashSimulationDataGenerator
{
	void GenerateNext();
public:
	SpiFlashSimulationDataGenerator();
	~SpiFlashSimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, SpiFlashAnalyzerSettings* settings);
	U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel);

protected:
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
