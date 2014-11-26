/* 
 * File:   tactile_driver.h
 * Author: jgoldberg
 *
 * Created on April 14, 2014, 12:15 PM
 */

#include "uart.h"
#include <stdio.h>

#ifndef UART_H
#define	UART_H

//#ifndef TACTILE_DRIVER_H
//#define	TACTILE_DRIVER_H

#define TACTILE_TX_IDLE          0xFF
#define TACTILE_RX_IDLE          0xFF
#define TACTILE_MODE_A           0x41
#define TACTILE_MODE_B           0x42
#define TACTILE_MODE_C           0x43
#define TACTILE_MODE_D           0x44
#define TACTILE_MODE_E           0x45
#define TACTILE_MODE_F           0x46
#define TACTILE_MODE_G           0x47
#define TACTILE_MODE_T           0x54


// buffer lengths
#define SMALL_BUFFER            3
#define LARGE_BUFFER            110

void tactileInit();
void checkFrameSize();
void checkTactileBuffer();
void handleSkinRequest(unsigned char length,unsigned char *frame);
unsigned char sendTactileCommand(unsigned char length,unsigned char *frame);
void skinDataReceived(unsigned char rx_byte);
void handleSkinData(unsigned char length, unsigned char *data);
//#ifdef	__cplusplus
//extern "C" {
//#endif




//#ifdef	__cplusplus
//}
//#endif

#endif	/* TACTILE_DRIVER_H */

