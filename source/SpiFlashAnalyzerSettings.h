#ifndef SPIFLASH_ANALYZER_SETTINGS
#define SPIFLASH_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class SpiFlashAnalyzerSettings : public AnalyzerSettings
{
public:
	SpiFlashAnalyzerSettings();
	virtual ~SpiFlashAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();


	Channel mInputChannel;
	U32 mBitRate;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
};

#endif //SPIFLASH_ANALYZER_SETTINGS
