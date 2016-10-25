#ifndef SPIFLASH_ANALYZER_H
#define SPIFLASH_ANALYZER_H

#include <Analyzer.h>
#include "SpiFlashAnalyzerResults.h"
#include "SpiFlashSimulationDataGenerator.h"

class SpiFlashAnalyzerSettings;
class ANALYZER_EXPORT SpiFlashAnalyzer : public Analyzer
{
public:
	SpiFlashAnalyzer();
	virtual ~SpiFlashAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< SpiFlashAnalyzerSettings > mSettings;
	std::auto_ptr< SpiFlashAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	SpiFlashSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SPIFLASH_ANALYZER_H
