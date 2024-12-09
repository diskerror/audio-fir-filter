# Audio FIR Filter

This is a workspace for—initially—building a low-cut FIR filter for audio files. It currently writes over the input file only preserving the 'fmt ' and 'bext' meta data. All other WAVE chunks are dropped. A 'JUNK' chunk is added so that there is space for additional chunks at the start of the file, and so that audio data starts at the 4k block.

The Wave.h header file uses the Boost endianess file so it is anticipated that this program can be compiled on platforms with different endianess and maintain the proper WAVE byte order.

Of course it needs option handling for things like cutoff frequency, slope, high or low pass filtering, and writing to a new file. It also needs to handle files larger than 4GB and file types other than WAVE.
