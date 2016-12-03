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

#define SAMPLE_COUNT 64

struct ftm_channel_struct {
	volatile uint32_t csc;
	volatile uint32_t cv;
};

class InputCapture
{
public:
	InputCapture();

	// rxPin can be 5,6,9,10,20,21,22,23
	// can polarity be both?
	bool begin(uint8_t rxPin, int polarity=FALLING);

	// 0 == no data, 1 == data, -1 == data, but lost samples
	int read(uint32_t * val);

	friend void ftm0_isr(void);

private:
	void isr(void);
	struct ftm_channel_struct *ftm;

	uint32_t samples[SAMPLE_COUNT];
	volatile uint32_t write_index;
	uint32_t read_index;

	uint8_t cscEdge;

	// track which channels we have installed
	static uint16_t overflow_count;
	static volatile uint8_t channelmask;
	static bool overflow_inc;
	static InputCapture *list[8];
};


