/*
 * tch.c
 *
 *  Created on: 15 jul. 2018
 *      Author: Ludo
 */

#include "tch.h"

#include "common.h"
#include "mapping.h"
#include "tim.h"

/*** TCH local macros ***/

// Speed under which the Tachro is off (not accurate enough).
#define TCH_SPEED_MIN_KMH	5

/*** TCH local structures ***/

// Internal state machine.
typedef enum {
	TCH_STATE_OFF,
	TCH_STATE_STEP1,
	TCH_STATE_STEP2,
	TCH_STATE_STEP3,
	TCH_STATE_STEP4,
	TCH_STATE_STEP5,
	TCH_STATE_STEP6,
} TCH_State;

/*** TCH local global variables ***/

// step_delay_us[v] = delay between each step in �s required to display v km/h.
static const unsigned int tch_step_delay_us[TCH_SPEED_MAX_KMH+1] = {0, 0, 0, 0, 0, 314278, 269123, 235314, 209051, 188062, 170904,
														     	 	156614, 144530, 134177, 125208, 117363, 110443, 104293, 98793, 93843, 89366,
																	85296, 81581, 78177, 75044, 72154, 69477, 66993, 64679, 62520, 60501,
																	58608, 56830, 55156, 53579, 52089, 50679, 49344, 48078, 46875, 45730,
																	44640, 43601, 42609, 41662, 40755, 39887, 39056, 38258, 37492, 36757,
																	36049, 35368, 34713, 34081, 33472, 32885, 32317, 31769, 31239, 30727,
																	30231, 29751, 29286, 28835, 28398, 27974, 27562, 27162, 26774, 26397,
																	26030, 25673, 25326, 24988, 24659, 24339, 24027, 23722, 23426, 23136,
																	22854, 22579, 22310, 22047, 21791, 21540, 21295, 21056, 20822, 20593,
																	20369, 20150, 19935, 19725, 19520, 19318, 19121, 18928, 18739, 18553,
																	18371, 18193, 18018, 17846, 17678, 17512, 17350, 17191, 17035, 16881,
																	16730, 16582, 16437, 16294, 16153, 16015, 15879, 15746, 15614, 15485,
																	15358, 15234, 15111, 14990, 14871, 14754, 14638, 14525, 14413, 14303,
																	14195, 14088, 13983, 13879, 13777, 13676, 13577, 13480, 13383, 13288,
																	13195, 13102, 13011, 12922, 12833, 12746, 12660, 12575, 12491, 12408,
																	12326, 12246, 12166, 12088, 12010, 11934, 11858, 11784, 11710, 11637};
static TCH_State tch_state;

/*** TCH functions ***/

/* CONFIGURE TCH CONTROL INTERFACE.
 * @param:	None.
 * @return:	None.
 */
void TCH_Init(void) {
	// Init INH outputs.
	GPIO_Configure(&GPIO_TCH_INH_A, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TCH_INH_B, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TCH_INH_C, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Init PWM outputs
	GPIO_Configure(&GPIO_TCH_PWM_A, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TCH_PWM_B, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TCH_PWM_C, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Init context.
	tch_state = TCH_STATE_OFF;
	// Init global context.
	lsmcu_ctx.lsmcu_speed_kmh = 0;
}

/* MAIN ROUTINE OF TCH CONTROL INTERFACE.
 * @param:	None.
 * @return:	None.
 */
void TCH_Task(void) {
	// Perform state machine.
	switch (tch_state) {
	case TCH_STATE_OFF:
		// All outputs off.
		GPIO_Write(&GPIO_TCH_INH_A, 0);
		GPIO_Write(&GPIO_TCH_INH_B, 0);
		GPIO_Write(&GPIO_TCH_INH_C, 0);
		GPIO_Write(&GPIO_TCH_PWM_A, 0);
		GPIO_Write(&GPIO_TCH_PWM_B, 0);
		GPIO_Write(&GPIO_TCH_PWM_C, 0);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh >= TCH_SPEED_MIN_KMH) {
			// Start timer and go to first step.
			TIM5_Start();
			TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
			TIM5_ClearUifFlag();
			tch_state = TCH_STATE_STEP1;
		}
		break;
	case TCH_STATE_STEP1:
		// Toggle required outputs.
		GPIO_Write(&GPIO_TCH_INH_A, 1);
		GPIO_Write(&GPIO_TCH_INH_B, 1);
		GPIO_Write(&GPIO_TCH_INH_C, 0);
		GPIO_Write(&GPIO_TCH_PWM_A, 1);
		GPIO_Write(&GPIO_TCH_PWM_C, 0);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh < TCH_SPEED_MIN_KMH) {
			// Stop timer and switch Tachro off.
			TIM5_Stop();
			tch_state = TCH_STATE_OFF;
		}
		else {
			// Check delay.
			if (TIM5_GetUifFlag() != 0) {
				// Clear flag, update delay and go to next step.
				TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
				TIM5_ClearUifFlag();
				tch_state = TCH_STATE_STEP2;
			}
		}
		break;
	case TCH_STATE_STEP2:
		// Toggle required outputs.
		GPIO_Write(&GPIO_TCH_INH_B, 0);
		GPIO_Write(&GPIO_TCH_INH_C, 1);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh < TCH_SPEED_MIN_KMH) {
			// Stop timer and switch Tachro off.
			TIM5_Stop();
			tch_state = TCH_STATE_OFF;
		}
		else {
			// Check delay.
			if (TIM5_GetUifFlag() != 0) {
				// Clear flag, update delay and go to next step.
				TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
				TIM5_ClearUifFlag();
				tch_state = TCH_STATE_STEP3;
			}
		}
		break;
	case TCH_STATE_STEP3:
		// Toggle required outputs.
		GPIO_Write(&GPIO_TCH_INH_A, 0);
		GPIO_Write(&GPIO_TCH_INH_B, 1);
		GPIO_Write(&GPIO_TCH_PWM_A, 0);
		GPIO_Write(&GPIO_TCH_PWM_B, 1);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh < TCH_SPEED_MIN_KMH) {
			// Stop timer and switch Tachro off.
			TIM5_Stop();
			tch_state = TCH_STATE_OFF;
		}
		else {
			// Check delay.
			if (TIM5_GetUifFlag() != 0) {
				// Clear flag, update delay and go to next step.
				TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
				TIM5_ClearUifFlag();
				tch_state = TCH_STATE_STEP4;
			}
		}
		break;
	case TCH_STATE_STEP4:
		// Toggle required outputs.
		GPIO_Write(&GPIO_TCH_INH_A, 1);
		GPIO_Write(&GPIO_TCH_INH_C, 0);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh < TCH_SPEED_MIN_KMH) {
			// Stop timer and switch Tachro off.
			TIM5_Stop();
			tch_state = TCH_STATE_OFF;
		}
		else {
			// Check delay.
			if (TIM5_GetUifFlag() != 0) {
				// Clear flag, update delay and go to next step.
				TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
				TIM5_ClearUifFlag();
				tch_state = TCH_STATE_STEP5;
			}
		}
		break;
	case TCH_STATE_STEP5:
		// Toggle required outputs.
		GPIO_Write(&GPIO_TCH_INH_B, 0);
		GPIO_Write(&GPIO_TCH_INH_C, 1);
		GPIO_Write(&GPIO_TCH_PWM_B, 0);
		GPIO_Write(&GPIO_TCH_PWM_C, 1);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh < TCH_SPEED_MIN_KMH) {
			// Stop timer and switch Tachro off.
			TIM5_Stop();
			tch_state = TCH_STATE_OFF;
		}
		else {
			// Check delay.
			if (TIM5_GetUifFlag() != 0) {
				// Clear flag, update delay and go to next step.
				TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
				TIM5_ClearUifFlag();
				tch_state = TCH_STATE_STEP6;
			}
		}
		break;
	case TCH_STATE_STEP6:
		// Toggle required outputs.
		GPIO_Write(&GPIO_TCH_INH_A, 0);
		GPIO_Write(&GPIO_TCH_INH_B, 1);
		// State evolution.
		if (lsmcu_ctx.lsmcu_speed_kmh < TCH_SPEED_MIN_KMH) {
			// Stop timer and switch Tachro off.
			TIM5_Stop();
			tch_state = TCH_STATE_OFF;
		}
		else {
			// Check delay.
			if (TIM5_GetUifFlag() != 0) {
				// Clear flag, update delay and go to next step.
				TIM5_SetDelayUs(tch_step_delay_us[lsmcu_ctx.lsmcu_speed_kmh]);
				TIM5_ClearUifFlag();
				tch_state = TCH_STATE_STEP1;
			}
		}
		break;
	default:
		// Unknown state.
		break;
	}
}
