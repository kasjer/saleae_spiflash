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


	Channel mChipSelect;
	Channel mClock;
	Channel mMosi;
	Channel mMiso;
	Channel mD2;
	Channel mD3;
	U32 mManufacturer;
	U32 mAddressLength;
	U32 mSpiMode;
	U32 mBusMode;

protected:
	std::auto_ptr<AnalyzerSettingInterfaceNumberList> mManufacturerInterface;
	std::auto_ptr<AnalyzerSettingInterfaceNumberList> mAddressLengthInterface;
	std::auto_ptr<AnalyzerSettingInterfaceNumberList> mSpiModeInterface;
	std::auto_ptr<AnalyzerSettingInterfaceNumberList> mBusModeInterface;

	std::auto_ptr<AnalyzerSettingInterfaceChannel> mChipSelectInterface;
	std::auto_ptr<AnalyzerSettingInterfaceChannel> mClockInterface;
	std::auto_ptr<AnalyzerSettingInterfaceChannel> mMosiInterface;
	std::auto_ptr<AnalyzerSettingInterfaceChannel> mMisoInterface;
	std::auto_ptr<AnalyzerSettingInterfaceChannel> mD2Interface;
	std::auto_ptr<AnalyzerSettingInterfaceChannel> mD3Interface;
};

#endif //SPIFLASH_ANALYZER_SETTINGS
