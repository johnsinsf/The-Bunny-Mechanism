
#include "u3.h"
//#include <unistd.h>
#include "dpslibLJU3.h"
//#include "Log4.h"

extern log4cpp::Category& logger;

int dpsLJRead(HANDLE hDevice, DssObject& o, u3CalibrationInfo *caliInfo, int isDAC1Enabled) {
  uint8 *sendBuff, *recBuff;
  int sendChars, recChars, sendSize, recSize;
  int j;
  int error = 0;
  uint16 checksumTotal;
  int numChannels = 8;

  if( ((sendSize = 7+2+1+numChannels*2) % 2) != 0 )
      sendSize++;

  sendBuff = (uint8 *)malloc(sendSize*sizeof(uint8));

  if( ((recSize = 4+numChannels*2) % 2) != 0 )
      recSize++;

  recBuff = (uint8 *)malloc(recSize*sizeof(uint8));

  sendBuff[1] = (uint8)(0xF8);  //Command byte
  sendBuff[2] = (sendSize - 6)/2;  //Number of data words 
  sendBuff[3] = (uint8)(0x00);  //Extended command number

  sendBuff[6] = 0;  //Echo

  sendBuff[7] = 34;  //IOType is DAC0

  //getDacBinVoltCalibrated(caliInfo, 0, 0, &sendBuff[8]);
  sendBuff[8] = 0;

  sendBuff[9] = 26;    //IOType is PortStateRead

  //Setting AIN read commands
  for( j = 0; j < numChannels; j++ ) {
    sendBuff[10 + j*2] = 10; 
    sendBuff[11 + j*2] = j;
  }

  if( (sendSize % 2) != 0 )
    sendBuff[sendSize - 1] = 0;

  extendedChecksum(sendBuff, sendSize);

  if( (sendChars = LJUSB_Write(hDevice, sendBuff, sendSize)) < sendSize ) {
    if( sendChars == 0 )
      logger.error("write failed");
    else
      logger.error("did not write all of the buffer" );
    error = 1;
  } else {
    if( (recChars = LJUSB_Read(hDevice, recBuff, recSize)) < recSize ) {
      if( recChars == 0 ) {
        logger.error("read failed");
        error = 1;
      }
      else
        logger.error("did not read all of the expected buffer %d %d", recChars, recSize);
      error = 1;
    }
  }

  checksumTotal = extendedChecksum16(recBuff, recChars);

  if( (uint8)((checksumTotal / 256) & 0xFF) != recBuff[5] ) {
    logger.error("read buffer has bad checksum16(MSB) %d %d ", (uint8)((checksumTotal / 256 ) & 0xFF), recBuff[5]);
    error = 1;
  }
  else
  if( (uint8)(checksumTotal & 0xFF) != recBuff[4] ) {
    logger.error("read buffer has bad checksum16(LBS)" );
    error = 1;
  }
  else
  if( extendedChecksum8(recBuff) != recBuff[0] ) {
    logger.error("read buffer has bad checksum8");
    error = 1;
  }
  else
  if( recBuff[1] != (uint8)(0xF8) || recBuff[3] != (uint8)(0x00) ) {
    logger.error("read buffer has wrong command bytes");
    error = 1;
  }
  else
  if( recBuff[6] != 0 ) {
    logger.error("received errorcode %d for frame %d in response.", recBuff[6], recBuff[7]);
    error = 1;
  }
  else
  if( recChars != recSize ) {
    logger.error("wrong recSize %d %d", recChars, recSize);
    error = 1;
  }

  // process recBuff to send alerts for changes in state

  int current_state = -1;

  if( ! error ) {
    for( int i = 0; i < numChannels; i++ ) {
      current_state = recBuff[12+i];
      o.dpsids[i].inew_state = current_state;
    }
  }
  free(recBuff);
  free(sendBuff);

  return error;
}

void dpsLJPrint(int num, dpsID* ptr) {
  for(int i = 0; i < num; i++) {
    logger.info("id: %d fio:%s name: %s", ptr[i].id, ptr[i].fio, ptr[i].name.c_str());
  }
}

int dpsLJConf(HANDLE devHandle, int* isDAC1Enabled) {
  // Setup the variables we will need.
  int r = 0; // For checking return values
  BYTE sendBuffer[CONFIGU3_COMMAND_LENGTH], recBuffer[CONFIGU3_RESPONSE_LENGTH];

  if( devHandle == NULL ) {
      logger.error("U3 not open. Please connect one and try again.");
      return(-1);
  }

  // Builds the ConfigU3 command
  buildConfigU3Bytes(sendBuffer);
  
  // Write the command to the device.
  r = LJUSB_Write( devHandle, sendBuffer, CONFIGU3_COMMAND_LENGTH );
  
  if( r != CONFIGU3_COMMAND_LENGTH ) {
      logger.error("An error occurred when trying to write the buffer. The error was: %d", errno);
      return(-1);
  }
  
  // Read the result from the device.
  r = LJUSB_Read( devHandle, recBuffer, CONFIGU3_RESPONSE_LENGTH );
  
  if( r != CONFIGU3_RESPONSE_LENGTH ) {
      logger.error("An error occurred when trying to read from the U3. The error was: %d", errno);
      return(-1);
  }
  
  // Check the command for errors
  if( checkResponseForErrors(recBuffer) != 0 ){
      return(-1);
  }
  
  // Parse the response into something useful
  parseConfigU3Bytes(recBuffer);
 
  configIO_example(devHandle, isDAC1Enabled);
 
  return 0;
}

/* ------------- LabJack Related Helper Functions Definitions ------------- */

// Uses information from section 5.2.2 of the U3 User's Guide to make a ConfigU3
// packet.
// http://labjack.com/support/u3/users-guide/5.2.2
void buildConfigU3Bytes(BYTE * sendBuffer) {
  int i; // For loops
  int checksum = 0;
  
  // Build up the bytes
  //sendBuffer[0] = Checksum8
  sendBuffer[1] = 0xF8;
  sendBuffer[2] = 0x0A;
  sendBuffer[3] = 0x08;
  //sendBuffer[4] = Checksum16 (LSB)
  //sendBuffer[5] = Checksum16 (MSB)
  
  // We just want to read, so we set the WriteMask to zero, and zero out the
  // rest of the bytes.
  sendBuffer[6] = 0;
  for( i = 7; i < CONFIGU3_COMMAND_LENGTH; i++){
      sendBuffer[i] = 0;
  }
  
  // Calculate and set the checksum16
  checksum = calculateChecksum16(sendBuffer, CONFIGU3_COMMAND_LENGTH);
  sendBuffer[4] = (BYTE)( checksum & 0xff );
  sendBuffer[5] = (BYTE)( (checksum / 256) & 0xff );
  
  // Calculate and set the checksum8
  sendBuffer[0] = calculateChecksum8(sendBuffer);
  
  // The bytes have been set, and the checksum calculated. We are ready to
  // write to the U3.
}

// Checks the response for any errors.
int checkResponseForErrors(BYTE * recBuffer) {
  if(recBuffer[0] == 0xB8 && recBuffer[1] == 0xB8) {
      // If the packet is [ 0xB8, 0xB8 ], that's a bad checksum.
      logger.error("The U3 detected a bad checksum. Double check your checksum calculations and try again.");
      return -1;
  }
  else if (recBuffer[1] != 0xF8 || recBuffer[2] != 0x10 || recBuffer[3] != 0x08) {
      // Make sure the command bytes match what we expect.
      logger.error("Got the wrong command bytes back from the U3.");
      return -1;
  }
  
  // Calculate the checksums.
  int checksum16 = calculateChecksum16(recBuffer, CONFIGU3_RESPONSE_LENGTH);
  BYTE checksum8 = calculateChecksum8(recBuffer);
  
  if ( checksum8 != recBuffer[0] || recBuffer[4] != (BYTE)( checksum16 & 0xff ) || recBuffer[5] != (BYTE)( (checksum16 / 256) & 0xff ) ) {
      // Check the checksum
      logger.error("Response had invalid checksum.\n%d != %d, %d != %d, %d != %d", 
	checksum8, recBuffer[0], (BYTE)( checksum16 & 0xff ), recBuffer[4], (BYTE)( (checksum16 / 256) & 0xff ), recBuffer[5] );
      return -1;
  }
  else if ( recBuffer[6] != 0 ) {
      // Check the error code in the packet. See section 5.3 of the U3
      // User's Guide for errorcode descriptions.
      logger.error("Command returned with an errorcode = %d", recBuffer[6]);
      return -1;
  }
  
  return 0;
}

// Parses the ConfigU3 packet into something useful.
void parseConfigU3Bytes(BYTE * recBuffer){
  logger.error("Results of ConfigU3:");
  logger.error("  FirmwareVersion = %d.%02d", recBuffer[10], recBuffer[9]);
  logger.error("  BootloaderVersion = %d.%02d", recBuffer[12], recBuffer[11]);
  logger.error("  HardwareVersion = %d.%02d", recBuffer[14], recBuffer[13]);
  logger.error("  SerialNumber = %d", makeInt(recBuffer, 15));
  logger.error("  ProductID = %d", makeShort(recBuffer, 19));
  logger.error("  LocalID = %d", recBuffer[21]);
  logger.error("  TimerCounterMask = %d", recBuffer[22]);
  logger.error("  FIOAnalog = %d", recBuffer[23]);
  logger.error("  FIODireciton = %d", recBuffer[24]);
  logger.error("  FIOState = %d", recBuffer[25]);
  logger.error("  EIOAnalog = %d", recBuffer[26]);
  logger.error("  EIODirection = %d", recBuffer[27]);
  logger.error("  EIOState = %d", recBuffer[28]);
  logger.error("  CIODirection = %d", recBuffer[29]);
  logger.error("  CIOState = %d", recBuffer[30]);
  logger.error("  DAC1Enable = %d", recBuffer[31]);
  logger.error("  DAC0 = %d", recBuffer[32]);
  logger.error("  DAC1 = %d", recBuffer[33]);
  logger.error("  TimerClockConfig = %d", recBuffer[34]);
  logger.error("  TimerClockDivisor = %d", recBuffer[35]);
  logger.error("  CompatibilityOptions = %d", recBuffer[36]);
  logger.error("  VersionInfo = %d", recBuffer[37]);
  
  if( recBuffer[37] == 0 ){
      logger.error("  DeviceName = U3A");
  }
  else if(recBuffer[37] == 1) {
      logger.error("  DeviceName = U3B");
  }
  else if(recBuffer[37] == 2) {
      logger.error("  DeviceName = U3-LV");
  }
  else if(recBuffer[37] == 18) {
      logger.error("  DeviceName = U3-HV");
  }
}


/* ---------------- Buffer Helper Functions Definitions ---------------- */

// Takes a buffer and an offset, and turns into an 32-bit integer
int makeInt(BYTE * buffer, int offset){
    return (buffer[offset+3] << 24) + (buffer[offset+2] << 16) + (buffer[offset+1] << 8) + buffer[offset];
}

// Takes a buffer and an offset, and turns into an 16-bit integer
int makeShort(BYTE * buffer, int offset) {
    return (buffer[offset+1] << 8) + buffer[offset];
}

// Calculates the checksum8
BYTE calculateChecksum8(BYTE* buffer){
  int i; // For loops
  int temp; // For holding a value while we working.
  int checksum = 0;
  
  for( i = 1; i < 6; i++){
      checksum += buffer[i];
  }
  
  temp = checksum/256;
  checksum = ( checksum - 256 * temp ) + temp;
  temp = checksum/256;
  
  return (BYTE)( ( checksum - 256 * temp ) + temp );
}

// Calculates the checksum16
int calculateChecksum16(BYTE* buffer, int len){
  int i;
  int checksum = 0;
    
  for( i = 6; i < len; i++){
    checksum += buffer[i];
  }
    
  return checksum;
}


int configIO_example(HANDLE hDevice, int *isDAC1Enabled)
{
  uint8 sendBuff[12], recBuff[12];
  uint8 timerCounterConfig, fioAnalog;
  uint16 checksumTotal;
  int sendChars, recChars;

  timerCounterConfig = 64;  //Disabling timers (bits 0 and 1) and Counters
                                  //(bits 2 and 3), setting TimerCounterPinOffset
                                  //to 4 (bits 4-7)
  fioAnalog = 0;  

  sendBuff[1] = (uint8)(0xF8);  //Command byte
  sendBuff[2] = (uint8)(0x03);  //Number of data words
  sendBuff[3] = (uint8)(0x0B);  //Extended command number

  sendBuff[6] = 5;  //Writemask : Setting writemask for timerCounterConfig (bit 0)
                    //and FIOAnalog (bit 2)

  sendBuff[7] = 0;  //Reserved
  sendBuff[8] = timerCounterConfig;  //TimerCounterConfig
  sendBuff[9] = 0;  //DAC1 enable : not enabling, though could already be enabled.
                    //If U3 hardware version 1.30, DAC1 is always enabled.
  sendBuff[10] = fioAnalog;  //FIOAnalog
  sendBuff[11] = 0;  //EIOAnalog : Not setting anything
  extendedChecksum(sendBuff, 12);

  //Sending command to U3
  if( (sendChars = LJUSB_Write(hDevice, sendBuff, 12)) < 12 )
  {
      if( sendChars == 0 )
          logger.error("ConfigIO error : write failed");
      else
          logger.error("ConfigIO error : did not write all of the buffer");
      return -1;
  }

  //Reading response from U3
  if( (recChars = LJUSB_Read(hDevice, recBuff, 12)) < 12 )
  {
      if( recChars == 0 )
          logger.error("ConfigIO error : read failed");
      else
          logger.error("ConfigIO error : did not read all of the buffer");
      return -1;
  }

  checksumTotal = extendedChecksum16(recBuff, 12);
  if( (uint8)((checksumTotal / 256 ) & 0xFF) != recBuff[5] ) {
      logger.error("ConfigIO error : read buffer has bad checksum16(MSB)");
      return -1;
  }

  if( (uint8)(checksumTotal & 0xFF) != recBuff[4] ) {
      logger.error("ConfigIO error : read buffer has bad checksum16(LBS)");
      return -1;
  }

  if( extendedChecksum8(recBuff) != recBuff[0] ) {
      logger.error("ConfigIO error : read buffer has bad checksum8");
      return -1;
  }

  if( recBuff[1] != (uint8)(0xF8) || recBuff[2] != (uint8)(0x03) || recBuff[3] != (uint8)(0x0B) ) {
      logger.error("ConfigIO error : read buffer has wrong command bytes");
      return -1;
  }

  if( recBuff[6] != 0 ) {
      logger.error("ConfigIO error : read buffer received errorcode %d", recBuff[6]);
      return -1;
  }

  if( recBuff[8] != timerCounterConfig ) {
      logger.error("ConfigIO error : TimerCounterConfig did not get set correctly");
      return -1;
  }

  if( recBuff[10] != fioAnalog && recBuff[10] != (fioAnalog|(0x0F)) ) {
      logger.error("ConfigIO error : FIOAnalog(%d) did not set correctly", recBuff[10]);
      return -1;
  }

  *isDAC1Enabled = (int)recBuff[9];

  return 0;
}
