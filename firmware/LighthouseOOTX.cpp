/** \file
 * Decode the OOTX frame.
 *
 * Described here: https://github.com/nairol/LighthouseRedox/blob/master/docs/Light%20Emissions.md
 */

#include "LighthouseOOTX.h"

LighthouseOOTX::LighthouseOOTX()
{
	reset();
	complete = 0;
	length = 0;
}

void LighthouseOOTX::reset()
{
	waiting_for_preamble = 1;
	waiting_for_length = 1;
	accumulator = 0;
	accumulator_bits = 0;
	rx_bytes = 0;
}

void LighthouseOOTX::add(unsigned bit)
{
	if (bit != 0 && bit != 1)
	{
		// something is wrong.  dump what we have received so far
		reset();
		return;
	}

	// add this bit to our incoming word
	accumulator = (accumulator << 1) | bit;
	accumulator_bits++;

	if (waiting_for_preamble)
	{
		// 17 zeros, followed by a 1 == 18 bits
		if (accumulator_bits != 18)
			return;

		if (accumulator == 0x1)
		{
			// received preamble, start on data
			// first we'll need the length
			waiting_for_preamble = 0;
			waiting_for_length = 1;
			return;
		}

		// we've received 18 bits worth of preamble,
		// but it is not a valid thing.  hold onto the
		// last 17 bits worth of data
		accumulator_bits--;
		accumulator = accumulator & 0x1FFFF;
		return;
	}

	// we're receiving data!  accumulate until we get a sync bit
	if (accumulator_bits != 17)
		return;

	if ((accumulator & 1) == 0)
	{
		// no sync bit. go back into waiting for preamble mode
		reset();
		return;
	}

	// hurrah!  the sync bit was set
	unsigned word = accumulator >> 1;
	accumulator = 0;
	accumulator_bits = 0;
	
	add_word(word);
}


void LighthouseOOTX::add_word(unsigned word)
{
	if (waiting_for_length)
	{
		length = word + 4; // add in the CRC32 length
		padding = length & 1;
		waiting_for_length = 0;
		rx_bytes = 0;

		// error!
		if (length > sizeof(bytes))
			reset();

		return;
	}

	bytes[rx_bytes++] = (word >> 8) & 0xFF;
	bytes[rx_bytes++] = (word >> 0) & 0xFF;

	if (rx_bytes < length + padding)
		return;

	// we are at the end!

	// todo: check crc32

	complete = 1;
	waiting_for_length = 1;

	// reset to wait for a preamble
	reset();
}
