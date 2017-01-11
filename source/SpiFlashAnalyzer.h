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
#ifndef SPIFLASH_ANALYZER_H
#define SPIFLASH_ANALYZER_H

#include <Analyzer.h>
#include "SpiFlashAnalyzerResults.h"
#include "SpiFlashSimulationDataGenerator.h"

#include "SpiFlash.h"

class SpiFlashAnalyzerSettings;
class ANALYZER_EXPORT SpiFlashAnalyzer : public Analyzer2
{
public:
	SpiFlashAnalyzer();
	virtual ~SpiFlashAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
	virtual U32 GetMinimumSampleRateHz();

	virtual const char *GetAnalyzerName() const;
	virtual bool NeedsRerun();

	AnalyzerChannelData *GetAnalyzerChannelData(Channel& channel);
protected: //vars
	std::auto_ptr<SpiFlashAnalyzerSettings> mSettings;
	std::auto_ptr<SpiFlashAnalyzerResults> mResults;
	AnalyzerChannelData *mSerial;

	SpiFlashSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	AnalyzerChannelData *mChipSelect;
	AnalyzerChannelData *mClock;
	AnalyzerChannelData *mMosi;
	AnalyzerChannelData *mMiso;
	AnalyzerChannelData *mD2;
	AnalyzerChannelData *mD3;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
	BusMode mCurrentBusMode;
	BusMode mDefaultBusMode;
	bool mDirIn;

	// Starting sample, CS activated
	U64 mCommandStart;
	// Ending sample, CS deactivated
	U64 mCommandEnd;
	BitState mClockIdleState;
	// Continues read mode active after CS is activated
	SpiCmdData *mLockedCmd;
private:
	void AddFrame(U64 start, U64 end, U64 d1, U64 d2, U8 type, U8 flags);
	void Setup();
	void AdvanceToCommandStart();
	void AdvanceDataToAbsPosition(U64 AbsolutePosition);
	void SetupResults();
	void AnalyzeCommandBits();
	void UpdateBusMode(BusMode busMode) { if (busMode) mCurrentBusMode = busMode; }
	U8 GetBits(BusMode busMode, bool dirIn);
	int ExtractBits(U64 &start, U64 &end, U32 &val, U8 bitCount);
	int ExtractMosiMiso(U64 &start, U64 &end, U8 &mosi, U8 &miso);

	void CacheClock(int num, U64 limit = 0);
	void CacheDropOlderClocks(U64 limit);

	U64 mCachedClocks[64];
	U8 mCachedClockCount;

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);

#endif //SPIFLASH_ANALYZER_H
