/** \file
 * Edge triggered input capture for the Teensy 3
 *
 * Derived from:
 *
 * PulsePosition Library for Teensy 3.1
 * High resolution input and output of PPM encoded signals
 * http://www.pjrc.com/teensy/td_libs_PulsePosition.html
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this library was funded by PJRC.COM, LLC by sales of Teensy
 * boards.  Please support PJRC's efforts to develop open source software by
 * purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "InputCapture.h"


// convert from microseconds to I/O clock ticks
#if defined(KINETISK)
#define CLOCKS_PER_MICROSECOND ((double)F_BUS / 1000000.0)
#elif defined(KINETISL)
#define CLOCKS_PER_MICROSECOND ((double)F_PLL / 2000000.0)
#endif

#define FTM0_SC_VALUE (FTM_SC_TOIE | FTM_SC_CLKS(1) | FTM_SC_PS(0))

#if defined(KINETISK)
#define CSC_CHANGE(reg, val)         ((reg)->csc = (val))
#define CSC_INTACK(reg, val)         ((reg)->csc = (val))
#define CSC_CHANGE_INTACK(reg, val)  ((reg)->csc = (val))
#define FRAME_PIN_SET()              *framePinReg = 1
#define FRAME_PIN_CLEAR()            *framePinReg = 0
#elif defined(KINETISL)
#define CSC_CHANGE(reg, val)         ({(reg)->csc = 0; while ((reg)->csc); (reg)->csc = (val);})
#define CSC_INTACK(reg, val)         ((reg)->csc = (val) | FTM_CSC_CHF)
#define CSC_CHANGE_INTACK(reg, val)  ({(reg)->csc = 0; while ((reg)->csc); (reg)->csc = (val) | FTM_CSC_CHF;})
#define FRAME_PIN_SET()              *(framePinReg + 4) = framePinMask
#define FRAME_PIN_CLEAR()            *(framePinReg + 8) = framePinMask
#endif


/**
 * Interrupt for the flexible timer module 0.
 *
 * This indicates either a timer overflow or a transition on one of
 * the inputs.
 */
void ftm0_isr(void)
{
	if (FTM0_SC & 0x80) {
		#if defined(KINETISK)
		FTM0_SC = FTM0_SC_VALUE;
		#elif defined(KINETISL)
		FTM0_SC = FTM0_SC_VALUE | FTM_SC_TOF;
		#endif
		InputCapture::overflow_count++;
		InputCapture::overflow_inc = true;
	}

	// TODO: this could be efficient by reading FTM0_STATUS
	//const uint8_t maskin = 0x18;
	const uint8_t maskin = InputCapture::channelmask;
	if ((maskin & 0x01) && (FTM0_C0SC & 0x80)) InputCapture::list[0]->isr();
	if ((maskin & 0x02) && (FTM0_C1SC & 0x80)) InputCapture::list[1]->isr();
	if ((maskin & 0x04) && (FTM0_C2SC & 0x80)) InputCapture::list[2]->isr();
	if ((maskin & 0x08) && (FTM0_C3SC & 0x80)) InputCapture::list[3]->isr();
	if ((maskin & 0x10) && (FTM0_C4SC & 0x80)) InputCapture::list[4]->isr();
	if ((maskin & 0x20) && (FTM0_C5SC & 0x80)) InputCapture::list[5]->isr();
	#if defined(KINETISK)
	if ((maskin & 0x40) && (FTM0_C6SC & 0x80)) InputCapture::list[6]->isr();
	if ((maskin & 0x80) && (FTM0_C7SC & 0x80)) InputCapture::list[7]->isr();
	#endif
	InputCapture::overflow_inc = false;
}

// some explanation regarding this C to C++ trickery can be found here:
// http://forum.pjrc.com/threads/25278-Low-Power-with-Event-based-software-architecture-brainstorm?p=43496&viewfull=1#post43496

uint16_t InputCapture::overflow_count = 0;
bool InputCapture::overflow_inc = false;
volatile uint8_t InputCapture::channelmask = 0;
InputCapture * InputCapture::list[8];

InputCapture::InputCapture()
{
}


bool InputCapture::begin(uint8_t pin, int polarity)
{
	uint32_t channel;
	volatile void *reg;

	cscEdge = (polarity == FALLING) ? 0b01001000 : 0b01000100;

	if (FTM0_MOD != 0xFFFF || (FTM0_SC & 0x7F) != FTM0_SC_VALUE) {
		FTM0_SC = 0;
		FTM0_CNT = 0;
		FTM0_MOD = 0xFFFF;
		FTM0_SC = FTM0_SC_VALUE;
		#if defined(KINETISK)
		FTM0_MODE = 0;
		#endif
	}

	switch (pin) {
	  case  6: channel = 4; reg = &FTM0_C4SC; break;
	  case  9: channel = 2; reg = &FTM0_C2SC; break;
	  case 10: channel = 3; reg = &FTM0_C3SC; break;
	  case 20: channel = 5; reg = &FTM0_C5SC; break;
	  case 22: channel = 0; reg = &FTM0_C0SC; break;
	  case 23: channel = 1; reg = &FTM0_C1SC; break;
	  #if defined(KINETISK)
	  case 21: channel = 6; reg = &FTM0_C6SC; break;
	  case  5: channel = 7; reg = &FTM0_C7SC; break;
	  #endif
	  default:
		return false;
	}

	write_index = 0;
	read_index = 0;

	ftm = (struct ftm_channel_struct *)reg;

	// Check for already installed on this pin
	if (channelmask & (1 << channel))
		return false;

	channelmask |= (1<<channel);
	list[channel] = this;

	*portConfigRegister(pin) = PORT_PCR_MUX(4);

	// input capture & interrupt on desired edge
	CSC_CHANGE(ftm, cscEdge);

	NVIC_SET_PRIORITY(IRQ_FTM0, 32);
	NVIC_ENABLE_IRQ(IRQ_FTM0);

	return true;
}

void InputCapture::isr(void)
{
	uint32_t count = overflow_count;
	uint32_t val = ftm->cv;

	CSC_INTACK(ftm, cscEdge); // input capture & interrupt on desired edge

	// if the pulse happened recently and we registered an overflow
	// on this interrupt then we assume that the pulse was in the last
	// window, not this one.
	if (val > 0xE000 && overflow_inc)
		count--;

	// update the high bits on the counter
	val |= (count << 16);

	samples[write_index++ % SAMPLE_COUNT] = val;
}


// 0 == no data, 1 == data, -1 == lost data
int InputCapture::read(uint32_t * val)
{
	__disable_irq();
	const uint32_t w = write_index;

	// fast return if no data
	if (w == read_index)
	{
		__enable_irq();
		return 0;
	}

	int rc = 1;

	if (w - read_index > SAMPLE_COUNT)
	{
		// we lost data.  catch up.
		rc = -1;
		read_index = w - SAMPLE_COUNT+1;
	}

	*val = samples[read_index++ % SAMPLE_COUNT];

	__enable_irq();

	return rc;
}
