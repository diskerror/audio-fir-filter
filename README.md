# Audio FIR Filter

This is a workspace for—initially—building a low-cut FIR filter for audio files. It currently writes over the input file preserving all meta data. Audio data is read and written using the same format as the input file.

Of course it needs option handling for things like cutoff frequency, slope, high or low pass filtering, and writing to a new file. It may also include the writing of new meta data from a configuration file, like creator, title, comments, etc.

If you are not familiar with FIR filters, this algorithm can seem quite slow, particularly with a steep slope (narrow transition band). It's faster than real-time with current hardware—Mac Studio with M2 chip—even being single threaded. The largest native floating point type, long double, is used to keep the filter stable at the extreme settings as well as accuracy. A 20Hz slope works quite well. Narrower slopes have not been tested... yet. It's primary usage is to remove DC offset and very low frequency garbage from very old digital audio recordings.

## Cross Platform
The Wave.h header file uses the Boost endianess file so it is anticipated that this program can be compiled on platforms with different endianess and maintain the proper WAVE and AIFF byte order.

## Original Build
This was originall built and working on a 2023 Mac Studio with an Apple M2 Max chip and running MacOS 15.4 (Sequoia). The Boost library was installed with [**Mac Ports**](https://www.macports.org).

# Acknowlegement

The windowed sinc algorithm was written with inspiration and understanding from [**The Scientist and Engineer's Guide to Digital Signal Processing**](http://www.dspguide.com) (Second Edition) by Steven W. Smith which can be downloaded for free. It has been extremely helpful in my casual study of DSP.
