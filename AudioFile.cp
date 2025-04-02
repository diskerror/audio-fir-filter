//
// Created by Reid Woodbury.
//

//	#include <climits>
//	#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <boost/endian.hpp>

using namespace boost::endian;
using namespace std;

#include "AudioFile.h"

string fourcc2str(big_uint32_t fc)
{
	auto c = (char*) &(fc);
	return string("") + c[0] + c[1] + c[2] + c[3];
}

/**
 * Define all possible interpretations of the first 12 bytes, WAVE or AIFF.
 */
typedef union {
	big_uint32_t	big;
	little_int32_t	little;
} h_sizeUnion_t;

typedef struct {
	big_uint32_t	baseID;
	h_sizeUnion_t	size;
	big_uint32_t	type;
} audio_header_t;


AudioFile::AudioFile(string fNameIn) {
	fileName = fNameIn;
	openFile();
}

AudioFile::AudioFile(char * arg) {
	fileName = arg;
	openFile();
}

void AudioFile::openFile() {
	audio_header_t header;
	header.baseID	= 0;
	header.size.little = 0;
	header.type		= 0;

// 	filesystem::path filePath(fileName);
// 	fileSize = filesystem::file_size(filePath);
	
	this->fileObj.open(fileName, ios_base::in | ios_base::out | ios_base::binary);
	if (this->fileObj.fail()) {
		throw invalid_argument("There was a problem opening the input file.");
	}

	//	Audio files always have a 12 byte header.
	this->fileObj.read((char *) &header, sizeof(header));
	fileType = header.baseID;
	switch (fileType) {
		case 'RIFF':
// 			if((header.size.little + 8) != fileSize) {
// 				cerr << "WARNING: File chunk size does not correspond to actual file size." << endl;
// 			}

			if(header.type != 'WAVE') {
				throw runtime_error("ERROR: Unknow sound media file type.");
			}

			openRIFF();
			break;
		
		case 'RF64':
// 			if((header.size.little) != 0xFFFFFFFF) {
// 				cerr << "WARNING: File chunk size should be 0xFFFFFF." << endl;
// 			}

			if(header.type != 'WAVE') {
				throw runtime_error("ERROR: Unknow sound media file type.");
			}
			
			openRF64();
			break;
		
		case 'FORM':
// 			if((header.size.big + 8) != fileSize) {
// 				cerr << "WARNING: File chunk size does not correspond to actual file size." << endl;
// 			}

			if(header.type != 'AIFF') {
				throw runtime_error("ERROR: Unknow sound media file type.");
			}
			
			openFORM();
			break;
		
		default:
				throw runtime_error("ERROR: Unknow file type.");
	}

	if((this->bytesPerSample >= 1 && this->bytesPerSample <= 3 && this->dataType != 1)
	    || (this->bytesPerSample == 4 && this->dataType != 3)
	    || this->bytesPerSample < 1 || this->bytesPerSample > 4)
	{
	    throw runtime_error("Audio file must be 8-, 16-, 24-bit PCM or 32-bit IEEE float format.");
	}
}


void AudioFile::openRIFF()
{
	FormatData_t format;
	
	//	Look for Chunks and save appropriatly.
	do {
		ChunkHead_t chunkExam;
		chunkExam.id   = 0;
		chunkExam.size = 0;

		this->fileObj.read((char *) &chunkExam, 8);
		
		switch (chunkExam.id) {
			case 'fmt ':
				this->fileObj.read((char *) &format, 16);
			
				//	skip over remainder, if any
				this->fileObj.seekg(chunkExam.size - 16, ios_base::cur);
				break;

			case 'data':
				this->dataBlockSize  = chunkExam.size;
				this->dataBlockStart = this->fileObj.tellg();
				//	fall through

				// skip over other chunks.
			default:
				this->fileObj.seekg(chunkExam.size, ios_base::cur);
				break;
		}
	}
	while (this->fileObj.good());

	this->dataType		    = format.type;
	this->sampleRate        = (float) format.sampleRate;
	this->bytesPerSample	= ceil((float_t)format.bitsPerSample / 8.0 );
	this->numChannels	    = format.channelCount;
	this->numSamples        = this->dataBlockSize / this->bytesPerSample;
}


void AudioFile::openRF64()
{
	FormatData_t format;
	DataSize64_t ds64;
	ds64.dataSize = 0;
	
	//	Look for Chunks and save appropriatly.
	do {
		ChunkHead_t  chunkExam;
		chunkExam.id   = 0;
		chunkExam.size = 0;

		this->fileObj.read((char *) &chunkExam, 8);

		switch (chunkExam.id) {
			case 'fmt ':
				this->fileObj.read((char *) &format, 16);

				//	skip over remainder, if any
				this->fileObj.seekg(chunkExam.size - 16, ios_base::cur);
				break;

			case 'ds64':
				this->fileObj.read((char *) &ds64, 28);
				this->dataBlockSize	= ds64.dataSize;
				
				//	skip over remainder, if any
				this->fileObj.seekg(chunkExam.size - 28, ios_base::cur);
				break;

			case 'data':
				if (this->dataBlockSize == 0) {
					throw runtime_error("The 'ds64' chunk must come before the 'data' chunk.");
				}
				
				this->dataBlockStart = this->fileObj.tellg();
				this->fileObj.seekg(this->dataBlockSize, ios_base::cur);
				break;

				// skip over other chunks.
			default:
				this->fileObj.seekg(chunkExam.size, ios_base::cur);
				break;
		}
	}
	while (!this->fileObj);
	
	this->dataBlockSize     = ds64.dataSize;
	this->bytesPerSample    = ceil((float_t)format.bitsPerSample / 8.0 );
	this->numChannels       = format.channelCount;
	this->numSamples        = this->dataBlockSize / this->bytesPerSample;
}


void AudioFile::openFORM()
{
	throw runtime_error("ERROR: Media file type FORM/AIFF not handled yet.");
}


char* AudioFile::ReadRawData()
{
	char *data = (char*) calloc(this->dataBlockSize, 1);
	this->fileObj.clear();
	this->fileObj.seekg(this->dataBlockStart);
	this->fileObj.read(data, this->dataBlockSize);
	return data;
}

void AudioFile::WriteRawData(char* data)
{
	this->fileObj.clear();
	this->fileObj.seekp(this->dataBlockStart);
	this->fileObj.write(data, this->dataBlockSize);
}


//	Only WAVE files atm.
void AudioFile::ReadSamples()
{
	auto dataBlock = ReadRawData();
	auto tempInt8    = (uint8_t *) dataBlock;
	auto tempInt16   = (little_int16_t *) dataBlock;
	auto tempInt24   = (little_int24_t *) dataBlock;
	auto tempFloat32 = (little_float32_t *) dataBlock;
	uint_fast64_t s;	//	Index variable for samples.
	
	samples.resize(this->numSamples);
	samples.shrink_to_fit();
	
	switch (this->bytesPerSample) {
		case 1:
		for(s = 0; s < this->numSamples; ++s) {
			samples[s] = (float) tempInt8[s];
		}
		break;
		
		case 2:
		for(s = 0; s < this->numSamples; ++s) {
			samples[s] = (float) tempInt16[s];
		}
		break;
		
		case 3:
		for(s = 0; s < this->numSamples; ++s) {
			samples[s] = (float) tempInt24[s];
		}
		break;
		
		case 4:
		for(s = 0; s < this->numSamples; ++s) {
			samples[s] = (float) tempFloat32[s];
		}
		break;
		
		default:
			cerr << fileName << endl << "  WAVE file must be 8, 16, 24, or 32-bit PCM format." << endl;
			throw new exception();
	}
	
	delete dataBlock;
}

//	Assumes this is the same size as the buffer that was read.
void AudioFile::WriteSamples()
{
	auto dataBlock = (char*) calloc(this->dataBlockSize, 1);
	auto tempInt8     = (uint8_t*) dataBlock;
	auto tempLInt16   = (little_int16_t*) dataBlock;
	auto tempLInt24   = (little_int24_t*) dataBlock;
	auto tempLFloat32 = (little_float32_t*) dataBlock;
	auto tempBInt16   = (big_int16_t*) dataBlock;
	auto tempBInt24   = (big_int24_t*) dataBlock;
	auto tempBFloat32 = (big_float32_t*) dataBlock;
	uint_fast64_t s;	//	Index variable for samples.
	
	if ( this->bytesPerSample == 1 ) {
		for(s = 0; s < this->numSamples; ++s) {
			tempInt8[s] = (uint8_t) this->samples[s] + 127;
		}
	}
	else {
		switch (fileType) {
			case 'RIFF':
			case 'RF64':
			    switch (this->bytesPerSample) {
                    case 2:
					for(s = 0; s < this->numSamples; ++s) {
						tempLInt16[s] = (little_int16_t) this->samples[s];
                    }
                    break;

                    case 3:
                    for(s = 0; s < this->numSamples; ++s) {
                        tempLInt24[s] = (little_int24_t) this->samples[s];
                    }
                    break;

                    case 4:
                    for(s = 0; s < this->numSamples; ++s) {
                        tempLFloat32[s] = (little_float32_t) this->samples[s];
                    }
                    break;
			    }
				break;
			
			case 'FORM':
			    switch (this->bytesPerSample) {
                    case 2:
					for(s = 0; s < this->numSamples; ++s) {
						tempBInt16[s] = (big_int16_t) this->samples[s];
                    }
                    break;

                    case 3:
                    for(s = 0; s < this->numSamples; ++s) {
                        tempBInt24[s] = (big_int24_t) this->samples[s];
                    }
                    break;

                    case 4:
                    for(s = 0; s < this->numSamples; ++s) {
                        tempBFloat32[s] = (big_float32_t) this->samples[s];
                    }
                    break;
			    }
				break;
		}
	}

	WriteRawData(dataBlock);
	delete dataBlock;
}
