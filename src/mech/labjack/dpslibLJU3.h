#ifndef __DssDpsLibLJ
#define __DssDpsLibLJ

//#include <errno.h>
//#include <string.h>
//#include <time.h>

#include "Mech.h"

#define MAXDPSIDS 256

// Defines how long the command is
#define CONFIGU3_COMMAND_LENGTH 26

// Defines how long the response is
#define CONFIGU3_RESPONSE_LENGTH 38

/* Buffer Helper Functions Protypes */
// Takes a buffer and an offset, and turns it into a 32-bit integer
int makeInt(BYTE * buffer, int offset);

// Takes a buffer and an offset, and turns it into a 16-bit integer
int makeShort(BYTE * buffer, int offset);

// Takes a buffer and calculates the checksum8 of it.
BYTE calculateChecksum8(BYTE* buffer);

// Takes a buffer and length, and calculates the checksum16 of the buffer.
int calculateChecksum16(BYTE* buffer, int len);

/* LabJack Related Helper Functions Protoypes */

// Demonstrates how to build the ConfigU3 packet.
void buildConfigU3Bytes(BYTE * sendBuffer);

// Demonstrates how to check a response for errors.
int checkResponseForErrors(BYTE * recBuffer);

// Demonstrates how to parse the response of ConfigU3.
void parseConfigU3Bytes(BYTE * recBuffer);

int dpsLJConf(HANDLE devHandle, int* isDAC1Enabled);
dpsID* dpsLJLoad(long &num);

void dpsLJPrint(int num, dpsID* ptr);
int  dpsLJRead(HANDLE hDevice, DssObject& o, u3CalibrationInfo *caliInfo, int isDAC1Enabled);

int  configIO_example(HANDLE hDevice, int *isDAC1Enabled);

#endif
