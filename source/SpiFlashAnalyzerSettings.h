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
