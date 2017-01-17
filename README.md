# SPI flash protocol analyzer for Saleae

Protocol analyzer for SPI flash decodes single, dual and quad commands.
Several manufacturer specific command sets can be used.
Most flashes can work in SPI mode 0 and 3, analyzer can detect this set it automatically or manually.

# Features
- SPI0 and SPI3 mode
- Single, dual and quad mode
- Commands for changing between single and quad or dual mode detected
- Continues read mode detected
- Register bit fields decoded
- Following manufacturers command sets supported:
  - Winbond
  - Macronix
  - GigaDevice
  - Adesto

# Installation

Simply put SpiFlash.dll in the Saleae *Analyzers* folder (typically: C:\Program Files\Saleae LLC\Analyzers).

# To be implemented
- More manufacturers (Micron, Microchip, Issi, Cypress)
- 32 addressing mode
- Custom dummy cycles (now only default manufacturer specific dummy cycle count is used, but some flashes have commands with dummy cycles that can be changed)
- Make CS optional (this could allow to analyze with higher bit rate)

## To consider
Concatenate same commands:
- read status that returns same result (usually busy) could be shown as single frame
- series of reads could be shown as single frame when consecutive addresses are used
