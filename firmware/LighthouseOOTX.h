/** \file
 * Record the Lighthouse OOTX messages
 */
#pragma once


class LighthouseOOTX
{
public:
	LighthouseOOTX();

	void add(unsigned bit);

	bool complete;
	unsigned length; // message length in bytes
	unsigned char bytes[256];

private:
	void reset();
	void add_word(unsigned word);

	bool waiting_for_preamble;
	bool waiting_for_length;

	unsigned long accumulator;
	unsigned accumulator_bits;


	unsigned rx_bytes;
	unsigned padding; // if there is a padding byte
};
