#include "SpiFlashAnalyzer.h"
#include "SpiFlashAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SpiFlashAnalyzer::SpiFlashAnalyzer()
	: Analyzer(),
	mSettings(new SpiFlashAnalyzerSettings()),
	mSimulationInitilized(false)
{
	SetAnalyzerSettings(mSettings.get());
}

SpiFlashAnalyzer::~SpiFlashAnalyzer()
{
	KillThread();
}

void SpiFlashAnalyzer::WorkerThread()
{
	for (;;)
	{
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
