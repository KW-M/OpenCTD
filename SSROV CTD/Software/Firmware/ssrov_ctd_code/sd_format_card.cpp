#include "sd_format_card.hpp"

/*
 * This program will format SD/SDHC/SDXC cards.
 * Warning all data will be deleted!
 *
 * This program attempts to match the format
 * generated by SDFormatter available here:
 *
 * http://www.sdcard.org/consumers/formatter/
 *
 * For very small cards this program uses FAT16
 * and the above SDFormatter uses FAT12.
 */
#include "SdFat.h"
#include "sdios.h"

/*
  Change the value of SD_CS_PIN if you are using SPI
  and your hardware does not use the default value, SS.
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(12)

//==============================================================================
// Serial output stream
ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------
uint32_t cardSectorCount = 0;
uint8_t sectorBuffer[512];
//------------------------------------------------------------------------------
// SdCardFactory constructs and initializes the appropriate card.
SdCardFactory cardFactory;
// Pointer to generic SD card.
SdCard *m_card = nullptr;
//------------------------------------------------------------------------------
#define sdError(msg)                            \
    {                                           \
        cout << F("error: ") << F(msg) << endl; \
        sdErrorHalt();                          \
    }
//------------------------------------------------------------------------------
void sdErrorHalt()
{
    if (!m_card)
    {
        cout << F("Invalid SD_CONFIG") << endl;
    }
    else if (m_card->errorCode())
    {
        if (m_card->errorCode() == SD_CARD_ERROR_CMD0)
        {
            cout << F("No card, wrong chip select pin, or wiring error?") << endl;
        }
        cout << F("SD errorCode: ") << hex << showbase;
        printSdErrorSymbol(&Serial, m_card->errorCode());
        cout << F(" = ") << int(m_card->errorCode()) << endl;
        cout << F("SD errorData = ") << int(m_card->errorData()) << endl;
    }
    exit(0);
}
//------------------------------------------------------------------------------
void clearSerialInput()
{
    uint32_t m = micros();
    do
    {
        if (Serial.read() >= 0)
        {
            m = micros();
        }
    } while (micros() - m < 10000);
}
//------------------------------------------------------------------------------
// flash erase all data
uint32_t const ERASE_SIZE = 262144L;
void eraseCard()
{
    cout << endl
         << F("Erasing\n");
    uint32_t firstBlock = 0;
    uint32_t lastBlock;
    uint16_t n = 0;

    do
    {
        lastBlock = firstBlock + ERASE_SIZE - 1;
        if (lastBlock >= cardSectorCount)
        {
            lastBlock = cardSectorCount - 1;
        }
        if (!m_card->erase(firstBlock, lastBlock))
        {
            sdError("erase failed");
        }
        cout << '.';
        if ((n++) % 64 == 63)
        {
            cout << endl;
        }
        firstBlock += ERASE_SIZE;
    } while (firstBlock < cardSectorCount);
    cout << endl;

    if (!m_card->readSector(0, sectorBuffer))
    {
        sdError("readBlock");
    }
    cout << hex << showbase << setfill('0') << internal;
    cout << F("All data set to ") << setw(4) << int(sectorBuffer[0]) << endl;
    cout << dec << noshowbase << setfill(' ') << right;
    cout << F("Erase done\n");
}
//------------------------------------------------------------------------------
void formatCard()
{
    ExFatFormatter exFatFormatter;
    FatFormatter fatFormatter;

    // Format exFAT if larger than 32GB.
    bool rtn = cardSectorCount > 67108864 ? exFatFormatter.format(m_card, sectorBuffer, &Serial) : fatFormatter.format(m_card, sectorBuffer, &Serial);

    if (!rtn)
    {
        sdErrorHalt();
    }
    cout << F("Run the SdInfo example for format details.") << endl;
}
//------------------------------------------------------------------------------

void format_sd_card()
{

    //// ----- Sd FAT Example Code: ------
    char c;
    cout << F(
                "\n"
                "This program can erase and/or format SD/SDHC/SDXC cards.\n"
                "\n"
                "Erase uses the card's fast flash erase command.\n"
                "Flash erase sets all data to 0X00 for most cards\n"
                "and 0XFF for a few vendor's cards.\n"
                "\n"
                "Cards up to 2 GiB (GiB = 2^30 bytes) will be formated FAT16.\n"
                "Cards larger than 2 GiB and up to 32 GiB will be formatted\n"
                "FAT32. Cards larger than 32 GiB will be formatted exFAT.\n"
                "\n"
                "Warning, all data on the card will be erased.\n"
                "Enter 'Y' to continue: ")
         << endl;
    while (!Serial.available())
    {
        yield();
    }
    c = Serial.read();
    c = toupper(c);
    // Read any existing Serial data.
    clearSerialInput();
    cout << c << endl;
    if (c != 'Y')
    {
        cout << F("Quiting, you did not enter 'Y'.\n");
        return;
    }

    // Select and initialize proper card driver.
    m_card = cardFactory.newCard(SdSpiConfig(current_card_chip_select, DEDICATED_SPI, SPI_CLOCK));
    if (!m_card || m_card->errorCode())
    {
        sdError("card init failed.");
        return;
    }

    cardSectorCount = m_card->sectorCount();
    if (!cardSectorCount)
    {
        sdError("Get sector count failed.");
        return;
    }

    cout << F("\nCard size: ") << cardSectorCount * 5.12e-7;
    cout << F(" GB (GB = 1E9 bytes)\n");
    cout << F("Card size: ") << cardSectorCount / 2097152.0;
    cout << F(" GiB (GiB = 2^30 bytes)\n");

    cout << F("Card will be formated ");
    if (cardSectorCount > 67108864)
    {
        cout << F("exFAT\n");
    }
    else if (cardSectorCount > 4194304)
    {
        cout << F("FAT32\n");
    }
    else
    {
        cout << F("FAT16\n");
    }
    cout << F(
                "\n"
                "Options are:\n"
                "F - erase and then format the card. (recommended)\n"
                "E - erase the card and skip formatting.\n"
                "Q - quick format the card without erase.\n"
                "\n"
                "Enter option: ")
         << endl;

    while (!Serial.available())
    {
        yield();
    }
    c = Serial.read();
    clearSerialInput(); // Read any existing Serial data.

    c = toupper(c);
    cout << c << endl;
    if (!strchr("EFQ", c))
    {
        cout << F("Quiting, invalid option entered.") << endl;
        return;
    }
    // clean up existing sd related stuff:
    if (datalogFile)
        datalogFile.close();
    sd.end();
    // do the erase and /or format
    if (c == 'E' || c == 'F')
    {
        eraseCard();
    }
    if (c == 'F' || c == 'Q')
    {
        formatCard();
    }

    cout << F("Done. !!!! Please Restart the CTD !!!!") << endl;
}
