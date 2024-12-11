# Audio FIR Filter

This is a workspace for—initially—building a low-cut FIR filter for audio files. It currently writes over the input file only preserving the 'fmt ' and 'bext' meta data. All other WAVE chunks are dropped. A 'JUNK' chunk is added so that there is space for additional chunks at the start of the file, and so that audio data starts at the secoond 4k block boundry.

Of course it needs option handling for things like cutoff frequency, slope, high or low pass filtering, and writing to a new file. It also needs to handle files larger than 4GB and file types other than WAVE. It also needs to handle bit-depths other than just 16-bit. It might also be appropriate to handle other audio file types and conversion of bit depths.

If you are not familiar with FIR filters, this algorithm can seem quite slow, particularly with a steep slope (narrow transition band). It's faster than real-time with current hardware, even being single threaded. The largest native floating point type is used to keep the filter stable at the extreme settings as well as accuracy. A 20Hz slope works quite well. Narrower slopes have not been tested... yet. It's primary usage is to remove DC offset and very low frequency garbage from very old digital audio recordings.

## Cross Platform
The Wave.h header file uses the Boost endianess file so it is anticipated that this program can be compiled on platforms with different endianess and maintain the proper WAVE byte order.

## Original Build
This was originall built and working on a 2023 Mac Studio with an Apple M2 Max chip and running MacOS 15.1 (Sequoia). The Boost library was installed with [Mac Ports](https://www.macports.org).

# Acknowlegement

The windowed sinc algorithm was written with inspiration and understanding from [**The Scientist and Engineer's Guide to Digital Signal Processing**](http://www.dspguide.com) (Second Edition) by Steven W. Smith which can be downloaded for free. It has been extremely helpful in my study of DSP.
