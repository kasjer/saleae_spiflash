#include "SpiFlashSimulationDataGenerator.h"
#include "SpiFlashAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

SpiFlashSimulationDataGenerator::SpiFlashSimulationDataGenerator()
{
}

SpiFlashSimulationDataGenerator::~SpiFlashSimulationDataGenerator()
{
}

void SpiFlashSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SpiFlashAnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mChipSelectSimulationData = mSimulationChannels.Add(settings->mChipSelect, mSimulationSampleRateHz, BIT_HIGH);
	mClockSimulationData = mSimulationChannels.Add(settings->mClock, mSimulationSampleRateHz, BIT_HIGH);
	mMosiSimulationData = mSimulationChannels.Add(settings->mMosi, mSimulationSampleRateHz, BIT_HIGH);
	mMisoSimulationData = mSimulationChannels.Add(settings->mMiso, mSimulationSampleRateHz, BIT_HIGH);
	mD2SimulationData = mSimulationChannels.Add(settings->mD2, mSimulationSampleRateHz, BIT_HIGH);
	mD3SimulationData = mSimulationChannels.Add(settings->mD3, mSimulationSampleRateHz, BIT_HIGH);
}

U32 SpiFlashSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mChipSelectSimulationData->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		GenerateNext();
	}

	*simulation_channel = mSimulationChannels.GetArray();
	return mSimulationChannels.GetCount();
}

void SpiFlashSimulationDataGenerator::GenerateNext()
{

}
