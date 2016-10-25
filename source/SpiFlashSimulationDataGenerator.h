#ifndef SPIFLASH_SIMULATION_DATA_GENERATOR
#define SPIFLASH_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class SpiFlashAnalyzerSettings;

class SpiFlashSimulationDataGenerator
{
public:
	SpiFlashSimulationDataGenerator();
	~SpiFlashSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, SpiFlashAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	SpiFlashAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //SPIFLASH_SIMULATION_DATA_GENERATOR
