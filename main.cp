//
// Created by Reid Woodbury.
//

#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <print>
#include <random>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <thread>


#include "Wave.h"
#include "WindowedSinc.h"

using namespace std;
namespace fs = filesystem;

#ifndef mAbs
#define mAbs(x) (((x) >= 0) ? (x) : -(x))
#endif

#ifndef mMax
#define mMax(x, y) (((x) > (y)) ? (x) : (y))
#endif

#define PROGRESS_WIDTH 70.0

inline float_t dither()
{
	static random_device                      rd;
	static mt19937                            gen(rd());
	static uniform_real_distribution<float_t> low(-1, 0);
	static uniform_real_distribution<float_t> hi(0, 1);
	return low(gen) + hi(gen); // This creates triangular dither.
}

int main(int argc, char **argv)
{
	auto    exitVal     = EXIT_SUCCESS;
	char    *data       = nullptr;
	float_t *tempOutput = nullptr;

	try {
		//  Check for input parameter.
		if (argc < 2) {
			println("Applies high-pass FIR filter to WAV file with cutoff at 20Hz.");
			return exitVal;
		}

		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20Hz and slope of 20Hz means that 30Hz is 0dB and 10Hz is approximately -80dB with ripple.
		//	Ripple is only in the stop band.
		const double freq  = 20;    //	Fc = freq / sampleRate
		const double slope = 20;    //	transition ~= slope / sampleRate


		//  Loop over file names.
		for (uint32_t a = 1; a < argc; a++) {
			HeaderChunk_t header;

			FormatData_t format;
			Chunk_t      formatChunk;
			formatChunk.id   = 'fmt ';
			formatChunk.size = sizeof(format);    //	16

			BroadcastAudioExt_t bExt;
			Chunk_t             bextChunk;
			bextChunk.id   = 'bext';
			bextChunk.size = sizeof(bExt);    //	602

			Chunk_t dataChunk;
			dataChunk.id   = 'data';
			dataChunk.size = 0;
			streampos dataStart = 0;


			string   inputFileName(argv[a]);
			fs::path filePath(inputFileName);

			ifstream inputStream(inputFileName, ios_base::in | ios_base::binary);
			if (!inputStream) {
				throw invalid_argument("There was a problem opening the input file.");
			}

			if (fs::file_size(filePath) < 1024) {
				cout << inputFileName << endl << "  File is too small." << endl;
				inputStream.close();
				continue;
			}

			//	WAVE files always have a 12 byte header
			inputStream.read((char *) &header, 12);
			if (header.id != 'RIFF' || header.type != 'WAVE') {
				cout << inputFileName << endl << " is not a WAVE file." << endl;
				inputStream.close();
				continue;
			}

			//	Look for Chunks and save appropriatly.
			do {
				Chunk_t chunkExam;
				chunkExam.id   = 0;
				chunkExam.size = 0;

				inputStream.read((char *) &chunkExam, 8);

				switch (chunkExam.id) {
					case 'fmt ':
						inputStream.read((char *) &format, formatChunk.size);
						//	skip over remainder, if any
						inputStream.seekg(chunkExam.size - formatChunk.size, ios_base::cur);
						break;

					case 'bext':
						inputStream.read((char *) &bExt, bextChunk.size);
						//	skip over remainder, if any
						inputStream.seekg(chunkExam.size - bextChunk.size, ios_base::cur);
						break;

					case 'data':
						dataChunk.size = chunkExam.size;
						data = new char[dataChunk.size];
						dataStart = inputStream.tellg();
						break;

						// skip over JUNK and fact (and what else?).
					default:
						inputStream.seekg(chunkExam.size, ios_base::cur);
						break;
				}
			} while (inputStream.good());

			////////////////////////////////////////////////////////////////////////////////
			//////////////////     ONLY 16-BIT PCM SAMPLES FOR NOW    //////////////////////
			if (format.type != 1 || format.bitsPerSample != 16) {
				cout << inputFileName << endl << "  WAVE file must be 16-bit PCM format." << endl;
				inputStream.close();
				continue;
			}

			inputStream.seekg(dataStart);
			inputStream.read(data, dataChunk.size);

			inputStream.close();

			//	reinterpret data as 16-bit samples
			uint16_t bytesPerSample = 2;
			uint32_t sampleQuantity = dataChunk.size / bytesPerSample;
			auto     *samples       = (little_int16_t *) data; // little_int16_t

			WindowedSinc sinc((freq / format.sampleRate), (slope / format.sampleRate));
			sinc.ApplyBlackman();
			sinc.MakeIntoHighPass();

			auto Mo2 = (int32_t) sinc.Get_M() / 2;

			//	define temporary output buffer[samps][chan], befor normalizing and reducing to file bits
			tempOutput = new float_t[sampleQuantity];

			uint32_t progressCount = 0;
			cout << fixed << setprecision(1) << flush;

			//	for each channel
			for (uint16_t chan = 0; chan < format.channelCount; chan++) {
				//	for each input sample in channel
				for (uint32_t samp = chan; samp < sampleQuantity; samp += format.channelCount) {
					//	accumulator
					long double acc = 0.0;

					//	for each sinc member
					for (int32_t m = -Mo2; m <= Mo2; m++) {
						int32_t mc  = m * format.channelCount;
						int64_t smc = samp + mc;

						if (smc < 0 || smc >= sampleQuantity) { continue; }

//						acc = samples[samp + chan * m] * (sinc[m]) + acc;
						acc = fmal(samples[smc], sinc[m], acc);
					}
					//	tempOutput[samp][chan] = acc
					tempOutput[samp] = (float_t) acc;

					//	progress bar, do only every x samples
					if (progressCount % 4181 == 0) {
						auto progress         = (float_t) progressCount / (float_t) sampleQuantity;
						auto progressPosition = (uint8_t) round((float_t) PROGRESS_WIDTH * progress);

						cout << "\r" << "["
						     << string(progressPosition, '=') << ">" << string(PROGRESS_WIDTH - progressPosition, ' ')
						     << "] " << (progress * 100) + 0.05 << " %           " << flush;
					}
					progressCount++;
				}
			}
			cout << "\r" << "[" << std::string(PROGRESS_WIDTH + 1, '=') << "] 100.0 %                 " << endl;
			std::cout.flush();


			//	normalize
			float_t maxVal = 0;

			//	for each sample of all channels
			for (uint32_t samp = 0; samp < sampleQuantity; samp++) {
				//	find max absolute value
				maxVal = mMax(maxVal, mAbs(tempOutput[samp]));
			}

			//	again, for each sample of all channels apply adjustment
			//	target_amplitude / max
			auto adjustment = (float_t) INT16_MAX / maxVal;

			//	and convert output buffer into writable buffer, using input buffer
			for (uint32_t samp = 0; samp < sampleQuantity; samp++) {
				//  Speculation (is this correct?):
				//  This truncates rather than rounds to prevent added dither from outputting values greater than INT16_MAX.
				samples[samp] = (little_int16_t) fma(tempOutput[samp], adjustment, dither());
			}

			delete tempOutput;
			tempOutput = nullptr;

			//  Open output stream.
			//  Safe save. Create new temp file, then delete old file and rename the new one.
			string   tempOutputName = inputFileName + "TEMPORARY";
			ofstream outputStream(tempOutputName, ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);
			if (!outputStream.is_open()) {
				throw runtime_error(inputFileName + " Couldn't open the file for output");
			}

			//	Write header, format, and broadcast extension chunks with
			//	JUNK padding so that audio data starts at the 4k block boundary.
			header.size = 4088 + dataChunk.size;    //	4096 minus space for header ID and size.
			outputStream.write((char *) &header, 12);

			outputStream.write((char *) &formatChunk, 8);
			outputStream.write((char *) &format, formatChunk.size);

			outputStream.write((char *) &bextChunk, 8);
			outputStream.write((char *) &bExt, bextChunk.size);

			Chunk_t junkChunk;
			junkChunk.id   = 'JUNK';
			//  header(12), format(8 + 16), bext(8 + 602), junk ID & size(8), data ID & size(8)
			junkChunk.size = 4096 - (12 + (8 + 16) + (8 + 602) + 8 + 8);
			outputStream.write((char *) &junkChunk, 8);
			outputStream.seekp(junkChunk.size, ios_base::cur);

			outputStream.write((char *) &dataChunk, 8);
			outputStream.write(data, dataChunk.size);

			outputStream.close();

			delete data;
			data = nullptr;

			//  Finnish safe save.
			remove(inputFileName.c_str());
			rename(tempOutputName.c_str(), (inputFileName).c_str());

		} //  Loop over file names.
	} // try
	catch (exception &e) {
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}

	delete data;
	delete tempOutput;

	return exitVal;

} // main
