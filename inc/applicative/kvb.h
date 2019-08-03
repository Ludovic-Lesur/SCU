/*
 * kvb.h
 *
 *  Created on: 26 dec. 2017
 *      Author: Ludovic
 */

#ifndef KVB_H
#define KVB_H

/*** KVB macros ***/

#define KVB_PA400_TEXT		((unsigned char*) "PA 400")
#define KVB_UC512_TEXT		((unsigned char*) "UC 512")
#define KVB_888888_TEXT		((unsigned char*) "888888")

/*** KVB functions ***/

void KVB_Init(void);
void KVB_Display(unsigned char* display);
void KVB_DisplayOff(void);
void KVB_Sweep(void);
void KVB_EnableBlinkLVAL(unsigned char blink_enabled);
void KVB_EnableBlinkLSSF(unsigned char blink_enabled);
void KVB_Task(unsigned char bl_unlocked);

#endif /* KVB_H */