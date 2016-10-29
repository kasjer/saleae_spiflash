#include "SpiFlashAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

SpiFlashAnalyzerSettings::SpiFlashAnalyzerSettings() :
	mChipSelect(UNDEFINED_CHANNEL),
	mClock(UNDEFINED_CHANNEL),
	mMosi(UNDEFINED_CHANNEL),
	mMiso(UNDEFINED_CHANNEL),
	mD2(UNDEFINED_CHANNEL),
	mD3(UNDEFINED_CHANNEL),
	mManufacturer(0xEF),
	mAddressLength(24),
	mSpiMode(0)
{
	mChipSelectInterface.reset(new AnalyzerSettingInterfaceChannel());
	mChipSelectInterface->SetTitleAndTooltip("CS", "Select Chip select line");
	mChipSelectInterface->SetChannel(mChipSelect);

	mClockInterface.reset(new AnalyzerSettingInterfaceChannel());
	mClockInterface->SetTitleAndTooltip("Clock", "Select Clock line");
	mClockInterface->SetChannel(mClock);

	mMosiInterface.reset(new AnalyzerSettingInterfaceChannel());
	mMosiInterface->SetTitleAndTooltip("MOSI", "Select MOSI line");
	mMosiInterface->SetChannel(mMosi);

	mMisoInterface.reset(new AnalyzerSettingInterfaceChannel());
	mMisoInterface->SetTitleAndTooltip("MISO", "Select MISO line");
	mMisoInterface->SetChannel(mMiso);

	mD2Interface.reset(new AnalyzerSettingInterfaceChannel());
	mD2Interface->SetTitleAndTooltip("D2", "Select D2 line");
	mD2Interface->SetSelectionOfNoneIsAllowed(true);
	mD2Interface->SetChannel(mD2);

	mD3Interface.reset(new AnalyzerSettingInterfaceChannel());
	mD3Interface->SetTitleAndTooltip("D3", "Select D3 line");
	mD3Interface->SetSelectionOfNoneIsAllowed(true);
	mD3Interface->SetChannel(mD3);

	mManufacturerInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mManufacturerInterface->SetTitleAndTooltip("Manufacturer", "Select flash manufacturer");
	mManufacturerInterface->AddNumber(0x1F, "Adesto", "");
	mManufacturerInterface->AddNumber(0x01, "Cypress", "");
	mManufacturerInterface->AddNumber(0xC8, "GigaDevice", "");
	mManufacturerInterface->AddNumber(0x9D, "ISSI", "");
	mManufacturerInterface->AddNumber(0xC2, "Macronix", "");
	mManufacturerInterface->AddNumber(0xBF, "Microchip", "");
	mManufacturerInterface->AddNumber(0x20, "Micron", "");
	mManufacturerInterface->AddNumber(0xEF, "Winbond", "");
	mManufacturerInterface->SetNumber(mManufacturer);

	mAddressLengthInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mAddressLengthInterface->SetTitleAndTooltip("Address", "Select address length");
	mAddressLengthInterface->AddNumber(24, "24 bits", "");
	mAddressLengthInterface->AddNumber(32, "32 bits", "");
	mAddressLengthInterface->SetNumber(mAddressLength);

	mSpiModeInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mSpiModeInterface->SetTitleAndTooltip("SPI Mode", "Select SPI mode");
	mSpiModeInterface->AddNumber(0, "Auto", "");
	mSpiModeInterface->AddNumber(1, "SPI Mode 0", "");
	mSpiModeInterface->AddNumber(2, "SPI Mode 3", "");
	mSpiModeInterface->SetNumber(mSpiMode);

	AddInterface(mChipSelectInterface.get());
	AddInterface(mClockInterface.get());
	AddInterface(mMosiInterface.get());
	AddInterface(mMisoInterface.get());
	AddInterface(mD2Interface.get());
	AddInterface(mD3Interface.get());
	AddInterface(mManufacturerInterface.get());
	AddInterface(mAddressLengthInterface.get());
	AddInterface(mSpiModeInterface.get());

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "text", "txt");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();

	AddChannel(mChipSelect, "Chip Select", false);
	AddChannel(mClock, "Clock", false);
	AddChannel(mMosi, "MOSI", false);
	AddChannel(mMiso, "MISO", false);
	AddChannel(mD2, "D2", false);
	AddChannel(mD3, "D3", false);
}

SpiFlashAnalyzerSettings::~SpiFlashAnalyzerSettings()
{
}

bool SpiFlashAnalyzerSettings::SetSettingsFromInterfaces()
{
	mManufacturer = U32(mManufacturerInterface->GetNumber());
	mAddressLength = U32(mAddressLengthInterface->GetNumber());
	mSpiMode = U32(mSpiModeInterface->GetNumber());
	mChipSelect = mChipSelectInterface->GetChannel();
	mClock = mClockInterface->GetChannel();
	mMosi = mMosiInterface->GetChannel();
	mMiso = mMisoInterface->GetChannel();
	mD2 = mD2Interface->GetChannel();
	mD3 = mD3Interface->GetChannel();

	ClearChannels();

	AddChannel(mChipSelect, "Chip Select", true);
	AddChannel(mClock, "Clock", true);
	AddChannel(mMosi, "MOSI", true);
	AddChannel(mMiso, "MISO", true);
	AddChannel(mD2, "D2", true);
	AddChannel(mD3, "D3", true);

	return true;
}

void SpiFlashAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mManufacturerInterface->SetNumber(mManufacturer);
	mAddressLengthInterface->SetNumber(mAddressLength);
	mSpiModeInterface->SetNumber(mSpiMode);
	mChipSelectInterface->SetChannel(mChipSelect);
	mClockInterface->SetChannel(mClock);
	mMosiInterface->SetChannel(mMosi);
	mMisoInterface->SetChannel(mMiso);
	mD2Interface->SetChannel(mD2);
	mD3Interface->SetChannel(mD3);
}

void SpiFlashAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString(settings);

	text_archive >> mManufacturer;
	text_archive >> mAddressLength;
	text_archive >> mSpiMode;
	text_archive >> mChipSelect;
	text_archive >> mClock;
	text_archive >> mMosi;
	text_archive >> mMiso;
	text_archive >> mD2;
	text_archive >> mD3;

	ClearChannels();
	AddChannel(mChipSelect, "Chip Select", true);
	AddChannel(mClock, "Clock", true);
	AddChannel(mMosi, "MOSI", true);
	AddChannel(mMiso, "MISO", true);
	AddChannel(mD2, "D2", true);
	AddChannel(mD3, "D3", true);

	UpdateInterfacesFromSettings();
}

const char* SpiFlashAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mManufacturer;
	text_archive << mAddressLength;
	text_archive << mSpiMode;
	text_archive << mChipSelect;
	text_archive << mClock;
	text_archive << mMosi;
	text_archive << mMiso;
	text_archive << mD2;
	text_archive << mD3;

	return SetReturnString(text_archive.GetString());
}
