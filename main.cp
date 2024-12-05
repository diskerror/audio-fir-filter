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

float_t dither()
{
	static random_device                      rd;
	static mt19937                            gen(rd());
	static uniform_real_distribution<float_t> low(-1, 0);
	static uniform_real_distribution<float_t> hi(0, 1);
	return low(gen) + hi(gen); // This creates triangular dither.
}

//void convolv(uint16_t chan,
//			 uint16_t channelCount,
//			 uint32_t sampleQuantity,
//			 int32_t Mo2,
//			 WindowedSinc sinc,
//			 int16_t *samples,
//			 float_t *output,
//			 uint32_t progressCount
//			)
//{
//	//	for each input sample
//	for (uint32_t samp = chan; samp < sampleQuantity; samp += channelCount)
//	{
//		//	accumulator
//		long double acc = 0.0;
//
//		//	for each sinc member
//		for (int32_t m = -Mo2; m <= Mo2; m++)
//		{
//			int32_t mc = m * channelCount;
//
//			if (((int32_t) samp + mc) < 0 || (samp + mc) >= sampleQuantity) { continue; }
//
////		acc = samples[samp + mc] * sinc[m] + acc;
//			acc = fma(samples[samp + mc], sinc[m], acc);
//		}
//		//	output[samp][chan] = acc
//		output[samp] = (float_t) acc;
//
//		//	progress bar, do only every 100 samples
//		if (progressCount % 4181 == 0)
//		{
//			float_t progress         = (float_t) progressCount / sampleQuantity;
//			uint8_t progressPosition = floor((float_t) PROGRESS_WIDTH * progress);
//
//			cout << "\r" << "["
//				 << string(progressPosition, '=') << ">" << string(PROGRESS_WIDTH - progressPosition, ' ')
//				 << "] " << progress * 100.0 << " %           " << flush;
//		}
//		progressCount++;
//	}
//}

int main(int argc, char **argv)
{
	auto exitVal = EXIT_SUCCESS;
	
	try
	{
		//  Check for input parameter.
		if (argc < 2)
		{
			println("Applies high-pass FIR filter to WAV file with cutoff at 20Hz.");
			return exitVal;
		}
		
		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20Hz and slope of 20Hz means that 30Hz is 0dB and 10Hz is approximately -80dB with ripple.
		//	Ripple is only in the stop band.
		const double freq  = 20;    //	Fc = freq / sampleRate
		const double slope = 20;    //	transition ~= slope / sampleRate
		
		
		HeaderChunk_t          header;
		Chunk_t                chunkExam;
		FormatExtensibleData_t format;
		uint32_t               formatSize;
		BroadcastAudioExt_t    bExt;
		uint32_t               bextSize;
		uint32_t               dataSize;
//		char                   *allChunks;		//	No. There could be chunks after data chunk.
//		uint32_t               allChunksSize;
		char                   *data = nullptr;
		
		WindowedSinc<long double> sinc;
		
		
		//  Loop over file names.
		for (uint32_t a = 1; a < argc; a++)
		{
			string   inputFileName(argv[a]);
			fs::path filePath(inputFileName);
			
			ifstream inputStream(inputFileName.c_str(), ios_base::in | ios_base::binary);
			if (!inputStream)
			{
				throw invalid_argument("There was a problem opening the input file.");
			}
			
			if (fs::file_size(filePath) < 1024)
			{
				cout << inputFileName << endl << "  File is too small." << endl;
				inputStream.close();
				continue;
			}
			
			//	WAVE files always have a 12 byte header
			inputStream.read(reinterpret_cast<char *>(&header), 12);
			if (header.id != 'RIFF' || header.type != 'WAVE')
			{
				cout << inputFileName << endl << " is not a WAVE file." << endl;
				inputStream.close();
				continue;
			}
			
			//	Look for Chunks and save appropriatly.
			while (inputStream.good() && !inputStream.eof())
			{
				inputStream.read((char *) &chunkExam, 8);
				
				switch (chunkExam.id)
				{
					case 'fmt ':
						formatSize = chunkExam.size;
						inputStream.read(reinterpret_cast<char *>(&format), chunkExam.size);
						
						////////////////////////////////////////////////////////////////////////////////
						//////////////////     ONLY 16-BIT PCM SAMPLES FOR NOW    //////////////////////
						if (format.type != 1 || format.bitsPerSample != 16)
						{
							cout << inputFileName << endl << "  WAVE file must be 16-bit PCM format." << endl;
							inputStream.close();
							continue;
						}

//						if (format.channelCount > 4)
//						{
//							cout << inputFileName << endl << "  WAVE file must have 4 channels or less." << endl;
//							inputStream.close();
//							continue;
//						}
						break;
					
					case 'bext':
						bextSize = chunkExam.size;
						inputStream.read(reinterpret_cast<char *>(&bExt), chunkExam.size);
						break;
					
					case 'data':
					{
						dataSize      = chunkExam.size;
//						allChunksSize = inputStream.tellg();
						
						//	read data into data buffer
						if (data != nullptr) { delete data; }
						data = (char *) calloc(dataSize, 1);
						inputStream.read(data, dataSize);
					}
						break;
						
						// skip over JUNK and fact (and what else?).
					default:
						inputStream.seekg((uint32_t) inputStream.tellg() + chunkExam.size);
						break;
				}
			}
			
//			inputStream.seekg(ios_base::beg);
//			allChunks = (char *) calloc(allChunksSize, 1);
//			inputStream.read(allChunks, allChunksSize);
			
			inputStream.close();
			
			//	reinterpret data as 16-bit samples
			uint16_t bytesPerSample = 2;
			uint32_t sampleQuantity = dataSize / bytesPerSample;
			auto     *samples       = reinterpret_cast<little_int16_t *>(data); // little_int16_t
			
			sinc.SetSinc((freq / format.sampleRate), (slope / format.sampleRate));
			sinc.ApplyBlackman();
			sinc.MakeIntoHighPass();
			
			auto Mo2 = sinc.Get_M() / 2;
			
			//	define temporary output buffer[samps][chan], befor normalizing and reducing to file bits
			auto output = (float_t *) calloc(sampleQuantity, sizeof(float_t));
			
			uint32_t progressCount = 0;
			cout << fixed << setprecision(1) << flush;

//			if (format.channelCount == 1)
//			{
//				convolv(1, 1, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//			}
//			else if (format.channelCount == 2)
//			{
//				thread L(convolv, 1, 2, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				thread R(convolv, 2, 2, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				L.join();
//				R.join();
//			}
//			else if (format.channelCount == 3)
//			{
//				thread L(convolv, 1, 3, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				thread C(convolv, 2, 3, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				thread R(convolv, 3, 3, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				L.join();
//				C.join();
//				R.join();
//			}
//			else if (format.channelCount == 4)
//			{
//				thread L(convolv, 1, 4, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				thread C(convolv, 2, 4, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				thread R(convolv, 3, 4, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				thread S(convolv, 4, 4, sampleQuantity, Mo2, std::ref(sinc), samples, output, std::ref(progressCount));
//				L.join();
//				C.join();
//				R.join();
//				S.join();
//			}
			
			//	for each channel
			for (uint16_t chan = 0; chan < format.channelCount; chan++)
			{
				//	for each input sample in channel
				for (uint32_t samp = chan; samp < sampleQuantity; samp += format.channelCount)
				{
					//	accumulator
					long double acc = 0.0;
					
					//	for each sinc member
					for (int32_t m = -Mo2; m <= Mo2; m++)
					{
						int32_t mc = m * format.channelCount;
						
						if (((int32_t) samp + mc) < 0 || (samp + mc) >= sampleQuantity) { continue; }

//						acc = samples[samp + chan + m] * (sinc[m]) + acc;
						acc = fma(samples[samp + mc], sinc[m], acc);
					}
					//	output[samp][chan] = acc
					output[samp] = (float_t) acc;
					
					//	progress bar, do only every x samples
					if (progressCount % 4181 == 0)
					{
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
			for (uint32_t samp = 0; samp < sampleQuantity; samp++)
			{
				//	find max absolute value
				maxVal = mMax(maxVal, mAbs(output[samp]));
			}
			
			//	again, for each sample of all channels apply adjustment
			//	target_amplitude / max
			auto adjustment = (float_t) INT16_MAX / maxVal;
			
			//	and convert output buffer into writable buffer, using input buffer
			for (uint32_t samp = 0; samp < sampleQuantity; samp++)
			{
				samples[samp] = (little_int16_t) (int16_t) round(fma(output[samp], adjustment, dither()));
			}
			
			delete output;
			
			//  Open output stream.
			string   tempOutputName = inputFileName + "TEMPORARY";
			ofstream outputStream(tempOutputName.c_str(), ios_base::out | ios_base::binary);

//			Write header.
//			We know most of the details from the input file chunks.
//			Only the 'data', 'bext', 'fmt ', and header chunks are saved.
			header.size = dataSize + 8 + bextSize + 8 + formatSize + 8 + sizeof(header.type);
			outputStream.write(reinterpret_cast<char *>(&header), 12);
			
			outputStream.write("fmt ", 4);
			outputStream.write(reinterpret_cast<char *>(&formatSize), 4);
			outputStream.write(reinterpret_cast<char *>(&format), formatSize);
			
			outputStream.write("bext", 4);
			outputStream.write(reinterpret_cast<char *>(&bextSize), 4);
			outputStream.write(reinterpret_cast<char *>(&bExt), bextSize);
			
			outputStream.write("data", 4);
			outputStream.write(reinterpret_cast<char *>(&dataSize), 4);

//			outputStream.write(allChunks, allChunksSize);
			outputStream.write(data, dataSize);
			
			outputStream.close();
			
			delete data;
			data     = nullptr;
			dataSize = 0;
			
//			delete allChunks;
//			allChunks     = nullptr;
//			allChunksSize = 0;

//			remove(inputFileName.c_str());
			rename(tempOutputName.c_str(), (inputFileName).c_str());
			
		} //  Loop over file names.
	} // try
	catch (exception &e)
	{
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}
	
	return exitVal;
	
} // main
