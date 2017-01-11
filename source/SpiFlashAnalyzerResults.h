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
#ifndef SPIFLASH_ANALYZER_RESULTS
#define SPIFLASH_ANALYZER_RESULTS

#include <AnalyzerResults.h>

enum FrameType
{
	FT_OUT_BYTE,
	FT_OUT_ADDR24,
	FT_OUT_ADDR32,
	FT_IN_BYTE,
	FT_CMD,
	FT_CMD_BYTE,
	FT_DUMMY,
	FT_IN_OUT,
	FT_M,
};

class SpiFlashAnalyzer;
class SpiFlashAnalyzerSettings;

class SpiFlashAnalyzerResults : public AnalyzerResults
{
public:
	SpiFlashAnalyzerResults( SpiFlashAnalyzer* analyzer, SpiFlashAnalyzerSettings* settings );
	virtual ~SpiFlashAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	SpiFlashAnalyzerSettings* mSettings;
	SpiFlashAnalyzer* mAnalyzer;
};

#endif //SPIFLASH_ANALYZER_RESULTS
