/* Nand Flash 4Mbit (512 Kbytes) - S71295 (SST) */

#include <SPI.h>

char buf[16];

//#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define FLASH_SSn 4
//#else
  //#define FLASH_SSn 15
//#endif

// ======================================================================================= //

void setup()
{
  Serial.begin(38400);

  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  pinMode(FLASH_SSn, OUTPUT); digitalWrite(FLASH_SSn, HIGH);
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    pinMode(11, INPUT); // Conflicts with SPI MOSI...do not use
    pinMode(13, INPUT); // Conflicts with SPI CLOCK...do not use
  #endif
  digitalWrite(FLASH_SSn, LOW);
  SPI.setBitOrder(MSBFIRST);

  flashInit();
  flashIDread();
     
  Serial.println("");
  Serial.println("Checking Non-Empty Sectors...");
  for (long x=0; x<128; x++)
  {
    flashReadInit((4096UL*x));
    
    boolean sectorEmpty = true;
    for (int q=0; q<4096; q++)
    {
      if (flashReadNext() != 0xFF) { sectorEmpty = false; break; }
    }
    
    if (!sectorEmpty)
    {
      Serial.print("Sector: ");
      if ((x+1) < 10) Serial.print("00"); else if ((x+1) < 100) Serial.print("0");
      Serial.println(x+1, DEC);
    }
    flashReadFinish();
  }
}

// ======================================================================================= //
// ======================================================================================= //
// ======================================================================================= //

void loop() 
{
  uint32_t address = 4;
  uint8_t data = 24;
  //flashWriteByte(address,data);
  flashReadInit(address);
  uint8_t rea = flashReadNext();
  flashReadFinish();
  Serial.println(rea);
}


// ======================================================================================= //
// ======================================================================================= //
// ======================================================================================= //

inline void volatile nop(void) { asm __volatile__ ("nop"); }

// ======================================================================================= //

void flashEnable()    { SPI.setBitOrder(MSBFIRST); nop(); }
void flashDisable()   { SPI.setBitOrder(LSBFIRST); nop(); }

// ======================================================================================= //

void flashWaitUntilDone()
{
  uint8_t data = 0;
  while (1)
  {
    digitalWrite(FLASH_SSn,LOW);
    (void) SPI.transfer(0x05);
    data = SPI.transfer(0);
    digitalWrite(FLASH_SSn,HIGH);
    if (!bitRead(data,0)) break;
    nop();
  }
}

// ======================================================================================= //

void flashInit()
{
  flashEnable();
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x50); //enable write status register instruction
  digitalWrite(FLASH_SSn,HIGH);
  delay(50);
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x01); //write the status register instruction
  SPI.transfer(0x00);//value to write to register - xx0000xx will remove all block protection
  digitalWrite(FLASH_SSn,HIGH);
  delay(50);
  flashDisable();
}

// ======================================================================================= //

void flashIDread()
{
  uint8_t id, mtype, dev;
  flashEnable();
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x9F); // Read ID command
  id = SPI.transfer(0);
  mtype = SPI.transfer(0);
  dev = SPI.transfer(0);
  char buf[16] = {0};
  sprintf(buf, "%02X %02X %02X", id, mtype, dev);
  Serial.print("SPI ID ");
  Serial.println(buf);
  digitalWrite(FLASH_SSn,HIGH);
  flashDisable();
}

// ======================================================================================= //

void flashTotalErase()
{
  flashEnable();
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x06);//write enable instruction
  digitalWrite(FLASH_SSn,HIGH);
  nop();
  digitalWrite(FLASH_SSn, LOW); 
  (void) SPI.transfer(0x60); // Erase Chip //
  digitalWrite(FLASH_SSn, HIGH);
  flashWaitUntilDone();
  flashDisable();
}

// ======================================================================================= //

void flashSetAddress(uint32_t addr)
{
  (void) SPI.transfer(addr >> 16);
  (void) SPI.transfer(addr >> 8);  
  (void) SPI.transfer(addr);
}

// ======================================================================================= //

void flashReadInit(uint32_t address)
{
  flashEnable();
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x03); // Read Memory - 25/33 Mhz //
  flashSetAddress(address);
}
uint8_t flashReadNext() { return SPI.transfer(0); }
void flashReadFinish()
{
  digitalWrite(FLASH_SSn,HIGH);
  flashDisable();
}

// ======================================================================================= //

void flashWriteByte(uint32_t address, uint8_t data)
{
  flashEnable();
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x06);//write enable instruction
  digitalWrite(FLASH_SSn,HIGH);
  nop();
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x02); // Write Byte //
  flashSetAddress(address);
  (void) SPI.transfer(data);
  digitalWrite(FLASH_SSn,HIGH);
  flashWaitUntilDone();
  flashDisable();
}

// ======================================================================================= //

void flashSectorErase(uint8_t sectorAddress)
{
  flashEnable();
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x06);//write enable instruction
  digitalWrite(FLASH_SSn,HIGH);
  nop();
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x20); // Erase 4KB Sector //
  flashSetAddress(4096UL*long(sectorAddress));
  digitalWrite(FLASH_SSn,HIGH);
  flashWaitUntilDone();
  flashDisable();
}
