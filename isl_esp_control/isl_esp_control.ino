/*
  First attempt talking to the IC
  Ben V. Brown <ralim@ralimtek.com>
*/

#include <Wire.h>
#define TIMESCALE_US 0
#define TIMESCALE_MS 1
#define TIMESCALE_S 2
#define TIMESCALE_MIN 3

// for continuous looping
bool continiousReadings = false;


/*
 * return pack voltage
 */
uint16_t readPackV()
{
  uint16_t r1 = readReg(0xA6);
  uint16_t r2 = readReg(0xA6 + 1);
  float reading = (uint16_t)(r2 << 8 | r1);

  reading *= (1.8 * 32);
  reading /= (4.095 );

  return reading;
}


/*
 * Return individual cell voltage
 */
uint16_t readCellV(uint8_t cellNo)
{
  return readRegVScale(0x90 + (2 * cellNo));
}


/*
 * Read input current
 */
uint16_t readCurrent()
{
  uint8_t r2 = readReg(0x8E);
  uint8_t r1 = readReg(0x8F);
  float reading = (((uint16_t)(r2 << 8 | r1)) & 0xFFFF);
  reading *= (1.8);
  reading /= (4095 * 2.89);
  return reading;
}


/*
 * print the system tempeature status
 * Mode, temperature,Current and Voltage overall Status
 */
void printStatus()
{
  byte stat = readReg(0x80);//get status 1
  if (stat & 0x80)
    Serial.println(F("Charge disabled as its too cold"));
  if (stat & 0x40)
    Serial.println(F("Charge disabled as its too hot"));
  if (stat & 0x20)
    Serial.println(F("Discharge disabled as its too cold"));
  if (stat & 0x10)
    Serial.println(F("Discharge disabled as its too hot"));
  if (stat & 0x08)
    Serial.println(F("UVLO"));
  if (stat & 0x04)
    Serial.println(F("Pack Shutdown - Under Voltage"));
  if (stat & 0x02)
    Serial.println(F("Overvoltage Lockout Fault"));
  if (stat & 0x01)
    Serial.println(F("Pack Shutdown - Over Voltage"));
  //Register 2 - Status
  stat = readReg(0x81);//get status 2
  if (stat & 0x80)
    Serial.println(F("Battery Fully charged"));
  if (stat & 0x20)
    Serial.println(F("Open Circuit on a Balance Lead!"));
  if (stat & 0x10)
    Serial.println(F("Cell failed!!! Battery needs replacement"));
  if (stat & 0x08)
    Serial.println(F("Short circuit tripped. Remove load to reset"));
  if (stat & 0x04)
    Serial.println(F("Overcurrent on dishcarge. Remove load to reset"));
  if (stat & 0x02)
    Serial.println(F("Overcurrent on charge. Remove charger to reset"));
  if (stat & 0x01)
    Serial.println(F("Outputs off. Battery too hot"));
  //Status 3
  stat = readReg(0x82);//get status 2
  if (stat & 0x80)
    Serial.println(F("Low Voltage Pre Charge on"));
  //if (stat & 0x40)
  //  Serial.println(F("Internal Scan is running"));
  if (stat & 0x20)
    Serial.println(F("ECC failed to correct an error in EEPROM"));
  if (stat & 0x10)
    Serial.println(F("ECC used to fix error in EEPROM"));
  if (stat & 0x08)
    Serial.println(F("Current: Discharging"));
  if (stat & 0x04)
    Serial.println(F("Current: Charging"));
  if (stat & 0x02)
    Serial.println(F("Charger Connected"));
  if (stat & 0x01)
    Serial.println(F("Load Connected"));

  //Status 4
  stat = readReg(0x83);//get status 2
  if (stat & 0x40)
    Serial.println(F("Mode: Sleep"));
  if (stat & 0x20)
    Serial.println(F("Mode: Doze"));
  if (stat & 0x10)
    Serial.println(F("Mode: Idle"));
  if (stat & 0x08)
    Serial.println(F("All Cells under voltage"));
  if (stat & 0x04)
    Serial.println(F("All Cells over voltage"));
  if (stat & 0x02)
    Serial.println(F("Temperature is too cold"));
  if (stat & 0x01)
    Serial.println(F("Temperature is too hot"));
  //cell bal status
  stat = readReg(0x84);
  for (byte i = 0; i < 8; i++)
    if (stat & (0x01 << i)) {
      Serial.print("Balancing Cell : ");
      Serial.print(String(i + 1) + "  ");
    }
}


/*
 * read the voltage of teh register (cell, over/under threshold, etc
 */
uint16_t readRegVScale(uint8_t reg)
{
  uint16_t r1 = readReg(reg);
  uint16_t r2 = (readReg(reg + 1)) & 0x0F;
  float reading = (uint16_t)(r2 << 8 | r1);

  reading *= (1.8 * 8);
  reading /= (4.095 * 3);

  return reading;
}


/*
 * Discharging short circuit voltage reference for 
 */
void setDischargeShortCircuit(uint8_t threshold, uint16_t count, uint8_t timeScale)
{
  //Threshold 000 is 16mV, each step ={16,24,32,48,64,96,128,256}
  //timescale = {us,ms,s,min}
  //default is 128mV,200uS
  uint8_t thres = 0xFF;
  switch (threshold)
  {
    case 16:
      thres = 0;
      break;
    case 24:
      thres = 1;
      break;
    case 32:
      thres = 2;
      break;
    case 48:
      thres = 3;
      break;
    case 64:
      thres = 4;
      break;
    case 96:
      thres = 5;
      break;
    case 128:
      thres = 6;
      break;
    case 256:
      thres = 7;
      break;
    default:
      thres = 0xFF;
  }
  if (thres == 0xFF)
  {
    Serial.println(F("Invalid threshold"));
    return;
  }
  
  if(!writeEEPROMTimeout(0x1A, count, timeScale, thres))
    Serial.println(" -- DSC NOT SET");
}


/*
 * Charging over current voltage reference for 
 */
void setChargeOverCurrent(uint8_t threshold, uint16_t count, uint8_t timeScale)
{
  //Threshold 000 is 1mV, each step ={1,2,4,6,8,12,16,24}
  //timescale = {us,ms,s,min}
  //default is 8mV,160ms
  uint8_t thres = 0xFF;
  switch (threshold)
  {
    case 1:
      thres = 0;
      break;
    case 2:
      thres = 1;
      break;
    case 4:
      thres = 2;
      break;
    case 6:
      thres = 3;
      break;
    case 8:
      thres = 4;
      break;
    case 12:
      thres = 5;
      break;
    case 16:
      thres = 6;
      break;
    case 24:
      thres = 7;
      break;
    default:
      thres = 0xFF;
  }
  if (thres == 0xFF)
  {
    Serial.println(F("Invalid threshold"));
    return;
  }
  
  if(!writeEEPROMTimeout(0x18, count, timeScale, thres))
    Serial.println(" -- COC NOT SET");
}

/*
 * Discharging over current voltage reference for 
 */
void setDischargeOverCurrent(uint8_t threshold, uint16_t count, uint8_t timeScale)
{
  //Threshold 000 is 4mV, each step ={4,8,16,24,32,48,64,96}
  //timescale = {us,ms,s,min}
  //default is 32mV,160ms
  uint8_t thres = 0xFF;
  switch (threshold)
  {
    case 4:
      thres = 0;
      break;
    case 8:
      thres = 1;
      break;
    case 16:
      thres = 2;
      break;
    case 24:
      thres = 3;
      break;
    case 32:
      thres = 4;
      break;
    case 48:
      thres = 5;
      break;
    case 64:
      thres = 6;
      break;
    case 96:
      thres = 7;
      break;
    default:
      thres = 0xFF;
  }
  
  if (thres == 0xFF)
  {
    Serial.println(F(" -- INVALID THRESHOLD"));
    return;
  }
  
  if(!writeEEPROMTimeout(0x16, count, timeScale, thres))
    Serial.println(" -- DOC NOT SET");
}


/*
 * Cell ballancing 2
 * only Cell balancingduring charge and  enableBalanceAtEOC are active
 */
void setFeature2(bool CellBalanceDuringDischarge, bool CellbalanceDuringCharge,
                 bool keepDFETonDuringCharge, bool keepCFETonDuringDischarge, 
                 bool shutdownOnUVLO, bool enableBalanceAtEOC)
{
  uint8_t output = 0;
  output |= CellBalanceDuringDischarge ? 1 << 7 : 0;
  output |= CellbalanceDuringCharge ? 1 << 6 : 0;
  output |= keepDFETonDuringCharge ? 1 << 5 : 0;
  output |= keepCFETonDuringDischarge ? 1 << 4 : 0;
  output |= shutdownOnUVLO ? 1 << 3 : 0;
  output |= enableBalanceAtEOC ? 1 : 0;

  if(!writeEEPROM(0x4B, output)){
    Serial.print(" -- FET CONTROL1 NOT SET");
  }
  else{
    Serial.print("Feature 2 ");
    Serial.println(readReg(0x4B & 0xFC), HEX);
  }
}


/*
 * Fet control1
 * only cell balancing and XT2 should be activated
 */
void setFeature1(bool CellFActivatesPSD, bool XT2Mode, bool TGain, bool PreChargeFETEnabled, bool disableOpenWireScan, bool OpenWireSetsPSD)
{
  uint8_t output = 0;
  output |= CellFActivatesPSD << 7;
  output |= XT2Mode << 5;
  output |= TGain << 4;
  output |= PreChargeFETEnabled << 2;
  output |= disableOpenWireScan << 1;
  output |= OpenWireSetsPSD;
  
  if(!writeEEPROM(0x4A, output)){
    Serial.print(" -- FET CONTROL1 NOT SET");
  }
  else{
    Serial.print("Feature 1 ");
    Serial.println(readReg(0x4A & 0xFC), HEX);
  }
  
}


/*
 * disable eeprom register access with 00
 */
void disableEEPROMAccess()
{
  //Set the bit to change to read/write EEPROM
  writeReg(0x89, 0x00);
}


/*
 * Enable eeprom register access with 01
 * must be disable afterwards wia the "disableepprom access"
 */
void enableEEPROMAccess()
{
  //Set the bit to change to read/write EEPROM
  writeReg(0x89, 0x01);
}

/*
 * Write the proper word to EEPROM
 * set the reserved words address
 * read the values of the buffer to be transmitted
 * transmit the buffer's first byte once (eeprom first byte reload)
 * transmit the full buffer data for full eeprom reload
*/
bool writeEEPROMWord(uint8_t reg, uint16_t value)
{
  // Do not write to addresses 58H through 7FH or locations higher than address ABH
  if ((reg > 0x58 && reg < 0x7F) || reg > 0xAB)
    return false;

  enableEEPROMAccess();

  // These are reserved areas for factory cal etc
  uint8_t buffer[4];//We need to write in pages
  uint8_t base = reg & 0xFC;
  buffer[0] = readReg(base);
  delay(1);//delay to allow EEPROM refresh
  Wire.beginTransmission(0x28);
  Wire.write(base);
  Wire.endTransmission(false);
  Wire.requestFrom(0x28, 4);
  
  for (byte i = 0; i < 4; i++) {
    while (!Wire.available());
    buffer[i] = Wire.read();
  }
  
  Wire.endTransmission(); //We have read in the buffer
  
  //Update the buffer
  buffer[reg & 0x03] = value & 0xFF;
  buffer[(reg & 0x03) + 1] = value >> 8;    // shift to the write

  Wire.beginTransmission(0x28);
  Wire.write(base);
  Wire.write(buffer[0]);
  Wire.endTransmission();
  delay(60);//pause for EEPROM write - at least 20ms

  //Special case for first byte causing EEPROM reload
  for (uint8_t i = 0; i < 4; i++)
  {
    Wire.beginTransmission(0x28);
    Wire.write(base + i);
    Wire.write(buffer[i]);
    Wire.endTransmission();
    delay(35);//pause for EEPROM write
  }
  delay(50);

  disableEEPROMAccess();
  
  //return true if the value was changed sucsessfully
  return value == readReg(reg);
}


/*
 * determine if 12 bits header was provided, if not exit
 * perform a register overwide on the lowest address register (8Bits)
 * get the voltage conversion out of the bit for confirmation
 * write the value to eeprom
*/
void writeEEPROMVoltage(uint8_t add_, uint16_t mV, uint8_t headerFourBits)
{
  uint16_t outputValue = 0;
  outputValue  = headerFourBits << 12;              // shit the bits to the left by 12 bits
  uint16_t calcValue = milliVoltsToVScaleRaw(mV);

  // to be sure one is not going over the range  (12 bits data max, lower 8 and upper 4 in different registers)
  if (calcValue > 0x0FFF)
  {
    Serial.println(F("Over Range"));
    return;
  }

  // if no return, one has exactly 12 bits of data
  // bit-wise OR operator.
  outputValue |= (calcValue & 0x0FFF);

  //Write out LSB in lowest address
  float reading = (uint16_t)(outputValue & 0x0FFF);

  // datasheet voltage conversion
  reading *= (1.8 * 8);
  reading /= (4.095 * 3);
  
  Serial.print("Setting Voltage To : " + String(reading) + " mV");

  // in case the targeted register was not set
  if(!writeEEPROMWord(add_, outputValue))
    Serial.print(" -- VOLTAGE " + String(add_) + " NOT SET");

  Serial.println();
}

/*
 * Set the cell counts of the battery
 * set the idel timeout and sleep timeout
 */
void setCellCountSleepMode(uint8_t cellCount, uint8_t idleModeTimeout, uint8_t sleepModeTimeout)
{
  //cellCount needs to be turned into the input bitmask - 4cells
  uint8_t cellMask = 0b11000011;

  
  idleModeTimeout = idleModeTimeout & 0x0F;
  sleepModeTimeout = sleepModeTimeout & 0xF0;
  
  Serial.print("Setting Cell Count to : ");
  Serial.println(cellMask, BIN);
  Serial.print("Setting IDLE/Doze Time to : ");
  Serial.println(idleModeTimeout);
  Serial.print("Setting Sleep Time to : ");
  Serial.println(sleepModeTimeout);
  
  uint16_t output = cellMask << 8;
  output |= idleModeTimeout;
  output |= sleepModeTimeout;
  
  if(!writeEEPROMWord(0x48, output))      // set the idle mode and sleep mode time out
    Serial.print(" -- SLEEP/IDLE NOT SET");
    
  if(!writeEEPROMWord(0x49, cellMask);    // set the battery cells number
    Serial.print(" -- CELL COUNT NOT SET");
}


/*
 * add the  balancing fet time out + on time scale
 */
boolean writeEEPROMTimeout(uint8_t add, uint16_t timeout, uint8_t timeScale, uint8_t headerFourBits)
{
  uint16_t outputValue = 0;
  outputValue  = headerFourBits << 12;
  outputValue |= (timeout & 0x3FF);
  outputValue |= ((timeScale & 0x03) << 10);
  
  if(!writeEEPROMWord(add, outputValue))
    return false;

  return true
}

/*
 * write the balancing on time first
 * write the balancing off time
 */
void setCellBalanceOnOffTime(uint16_t onTime, uint8_t onTimeScale, uint16_t offTime, uint8_t offTimeScale)
{
  //Sets on  and off time for balancing cycle
  if(!writeEEPROMTimeout(0x24, onTime, onTimeScale, 0))
    Serial.printLN(" -- TIME ON NOT SET");
    
  if(!writeEEPROMTimeout(0x26, offTime, offTimeScale))
    Serial.println(" -- TIME OFF NOT SET");
}


void setCellBalanceMax(uint16_t mV)
{
  writeEEPROMVoltage(0x1E, mV, 0x00);
}

void setCellBalanceMin(uint16_t mV)
{
  writeEEPROMVoltage(0x1C, mV, 0x00);
}

void setCellBalanceDifference(uint16_t mV)
{
  //If the cells are closer than this the balancing stops
  //defaults to 20mV
  writeEEPROMVoltage(0x20, mV, 0x00);
}

void setEOCThreshold(uint16_t mV)
{
  writeEEPROMVoltage(0x0C, mV, 0x00);
}

void setUVLockout(uint16_t mV)
{
  writeEEPROMVoltage(0x0A, mV, 0x00);
}

void setOVLockout(uint16_t mV)
{
  writeEEPROMVoltage(0x08, mV, 0x00);
}

void setUVRecovery(uint16_t mV)
{
  writeEEPROMVoltage(0x06, mV, 0x00);
}

void setUVThres(uint16_t mV, uint8_t LoadDetectPulseWidth)
{
  writeEEPROMVoltage(0x04, mV, LoadDetectPulseWidth);
}

void setOVRecovery(uint16_t mV)
{
  writeEEPROMVoltage(0x02, mV, 0x00);
}

void setOVThres(uint16_t mV, uint8_t ChargeDetectPulseWidth)
{
  writeEEPROMVoltage(0x00, mV, ChargeDetectPulseWidth);
}


/*
 * Do not write to addresses 58H through 7FH or locations higher than address ABH, because these addresses
 * access registers that are reserved. Writing to these locations can result in unexpected device operation
*/
void writeReg(uint8_t reg, uint8_t value)
{
  if ((reg > 0x58 && reg < 0x7F) || reg > 0xAB)
    return false;
  Wire.beginTransmission(0x28); // The Cell Balance Under-Temperature threshold
  Wire.write(reg);              // first write the register to access
  Wire.write(value);            // then write the value to pass
  Wire.endTransmission();       // send the data
}


/*
 * The EEPROM Enable bit determines read/write access to either the control registers or the EEPROM.
 * Set EEEN = 0 (default) by writing 0x00 to register 0x89 to access the control registers.
 * Set EEEN = 1 by writing 0x01 to register 0x89 to access the ISL94202 EEPROM
*/
void disableEEPROMAccess()
{
  //Set the bit to change to read/write EEPROM
  writeReg(0x89, 0x00);
}


/*
 * ISL Set up
 */
void islSetUp(){
  setOVThres(4250, 1);//Turn off charging the pack at 4.25V
  setOVRecovery(4150);//Turn back on charging once discharged to 4.15V
  setUVThres(3400, 1);//Turn off the pack output when any cell is below 3.4V
  setUVRecovery(3500);//Turn discharging back on when the lowest cell is above 3.5V
  setOVLockout(4300);//Permanently shutdown the pack if a cell goes above 4.3V
  setUVLockout(3300);//Permanently shutdown the pack if a cell falls below 3.3V
  setEOCThreshold(4200);//Mark End of Charge once any cell is at 4.2V
  setDischargeOverCurrent(8, 500, TIMESCALE_MS);//Set the discharge current limit to 8mv == 4A. Averaged over 0.5S - we fcked up-------------------------------
  setChargeOverCurrent(24, 500, TIMESCALE_MS);//Set the charge current limit to 24mV == 12A. Averaged over 0.5S - we fcked up-------------------------------------
  setDischargeShortCircuit(16, 500, TIMESCALE_US);//Set the short circuit protection to 64mV == 19.2A over 0.5mS - we fcked up------------------------------------
  setCellBalanceOnOffTime(10, TIMESCALE_S, 1, TIMESCALE_S);//Set balance time to 10S on, and 1S off
  setCellCountSleepMode(4, 15, 32);//Set the unit to 3S cell count. 10 minutes to enter lower power modes. 32 minutes to enter deep sleep
  setCellBalanceDifference(10);//Balance cells until they are within 10mV maximum error
  setCellBalanceMin(3850);//Start the blanacing when the cells are above 3.85V
  setCellBalanceMax(4300);//Stop balancing if the cells are above 4.3V (Set higher than OVThres to disable this feature).
  setFeature1(true, true, false, false, false, false);
  setFeature2(false, true, false, false, false, true);
  Serial.println(F("Please disconnect and reconnect the battery to force an EEPROM reload."));
  delay(10000);
}


/*
 *  1. Overvoltage Threshold
 *  2. Overvoltage Recovery
 *  3. Undervoltage Threshold
 *  4. Overvoltage Lockout Threshold
 *  5. Undervoltage Lockout Threshold
 *  6. End-of-Charge (EOC) Threshold
 *  7. End-of-Charge (EOC) Threshold
*/
void loop()
{
  Serial.print("Cells : ");
  
  for (uint8_t i = 0; i < 8; i++)
    Serial.println(readCellV(i));
  
  Serial.print("Current : ");
  Serial.println(readCurrent());
  
  Serial.print("Pack V: ");
  Serial.println(readPackV());
  
  Serial.print("Minimum Cell :");
  Serial.println(minCellV());
  
  Serial.print("Maximum Cell :");
  Serial.println(maxCellV());
  
  //printStatus();

  delay(5000);
}


void setup()
{
  Wire.begin();
  Serial.begin(1000000);
  Serial.println("ISL94202 Test");

  Wire.beginTransmission(0x01);
  Wire.write(00);
  Wire.endTransmission(true); // the Arduino Wire library sends the data only if "endTransmission" is called

  //False comms to unlock I2c state machine on the ic if we mess up^
  disableEEPROMAccess();

  writeReg(0x85, 0b00010000);//Set current shunt gain

  islSetUp(); //set ISL set control

  //printStatus();  // after the isl is set up, on can define to read all registers

}
