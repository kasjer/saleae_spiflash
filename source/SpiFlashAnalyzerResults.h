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
