//
// Created by Reid Woodbury.
//

#include "AudioFile.h"
#include "Wave.h"
#include "AIFF.h"

#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>

//  Utility function to convert a 32-bit integer to a string of 4 characters.
string bigInt2str(big_uint32_t fc)
{
	auto c = (char*) &fc;
	return string("") + c[0] + c[1] + c[2] + c[3];
}

typedef struct ChunkHead {
	big_uint32_t id;
	union {
    	big_uint32_t   be;
    	little_int32_t le;
    }            size;
} chunkHead_t;

/**
 * Find the container chunk type and size.
 * Defines all possible interpretations of the first 12 bytes, WAVE or AIFF.
 */
typedef struct {
	big_uint32_t	baseID;
	union {
    	big_uint32_t   be;
    	little_int32_t le;
    }	            size;
	big_uint32_t	type;
} mediaHeader_t;


////////////////////////////////////////////////////////////////////////////////////////////////////
AudioFile::AudioFile(const filesystem::path fPath): file(fPath) {
	mediaHeader_t header;
	header.baseID  = 0;
	header.size.le = 0;
	header.type    = 0;

	this->fileStm.open(this->file.string(), ios_base::in | ios_base::out | ios_base::binary);
	if (this->fileStm.fail()) {
		throw invalid_argument("There was a problem opening the input file.");
	}

	//	Audio files always have a 12 byte header.
	this->fileStm.read((char *) &header, sizeof(header));
	this->fileType    = header.baseID;
	this->fileSubType = header.type;
	switch (this->fileType) {
		case 'RIFF':
			if(this->fileSubType != 'WAVE') {
				throw runtime_error("ERROR: Unknown media file type.");
			}

			openRIFF();
			break;
		
		case 'RF64':
			if(this->fileSubType != 'WAVE') {
				throw runtime_error("ERROR: Unknown media file type.");
			}
			
			openRF64();
			break;
		
		case 'FORM':
		    switch (this->fileSubType) {
            	case 'AIFF':
            	    this->openAIFF();
            	    break;

            	case 'AIFC':
            	    this->openAIFC();
            	    break;

            	default:
            	    throw runtime_error("ERROR: Unknown media file type.");
		    }
			break;
		
		default:
				throw runtime_error("ERROR: Unknow file type.");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openRIFF()
{
	FormatData_t format;

	//	Look for Chunks and save appropriatly.
	do {
		chunkHead_t chunkExam;
		chunkExam.id      = 0;
		chunkExam.size.le = 0;

		this->fileStm.read((char *) &chunkExam, 8);
		
		switch (chunkExam.id) {
			case 'fmt ':
				this->fileStm.read((char *) &format, sizeof(format));

				//	skip over remainder, if any
				this->fileStm.seekg(chunkExam.size.le - sizeof(format), ios_base::cur);
				break;

			case 'data':
				this->dataBlockSize  = chunkExam.size.le;
				this->dataBlockStart = this->fileStm.tellg();
				//	fall through

				// skip over other chunks.
			default:
				this->fileStm.seekg(chunkExam.size.le, ios_base::cur);
				break;
		}
	}
	while (this->fileStm.good());

	this->dataType      = 'ms  ' & (big_uint32_t) format.type;
	this->sampleRate    = format.sampleRate;
	this->bitsPerSample = ceil((float)format.bitsPerSample / 8.0 ) * 8;
	this->numChannels   = format.channelCount;
	this->numSamples    = this->dataBlockSize / (this->bitsPerSample / 8);

    //  Always little endian for RIFF
	this->dataEndianess = 'litl';

	switch (format.type) {
		case 1:
			this->dataEncoding = 'PCM ';
			break;
		case 3:
			this->dataEncoding = 'IEEE';
			break;
		default:
			this->dataEndianess = 'litl';
			this->dataEncoding  = 0x00000000;
			break;
	}
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openRF64()
{
	FormatData_t format;
	DataSize64_t ds64;
	ds64.dataSize = 0;

	//	Look for Chunks and save appropriatly.
	do {
		chunkHead_t  chunkExam;
		chunkExam.id      = 0;
		chunkExam.size.le = 0;	//	little endian

		this->fileStm.read((char *) &chunkExam, 8);

		switch (chunkExam.id) {
			case 'fmt ':
				this->fileStm.read((char *) &format, sizeof(format));

				//	skip over remainder, if any
				this->fileStm.seekg(chunkExam.size.le - sizeof(format), ios_base::cur);
				break;

			case 'ds64':
				this->fileStm.read((char *) &ds64, sizeof(ds64));
				this->dataBlockSize	= ds64.dataSize;
				
				//	skip over remainder, if any
				this->fileStm.seekg(chunkExam.size.le - sizeof(ds64), ios_base::cur);
				break;

			case 'data':
				if (this->dataBlockSize == 0) {
					throw runtime_error("The 'ds64' chunk must come before the 'data' chunk.");
				}
				
				this->dataBlockStart = this->fileStm.tellg();
				this->fileStm.seekg(this->dataBlockSize, ios_base::cur);
				break;

				// skip over other chunks.
			default:
				this->fileStm.seekg(chunkExam.size.le, ios_base::cur);
				break;
		}
	}
	while (!this->fileStm);
	
	this->dataType      = 'ms  ' & (big_uint32_t) format.type;
	this->sampleRate    = format.sampleRate;
	this->bitsPerSample = ceil((float_t)format.bitsPerSample / 8.0 ) * 8;
	this->numChannels   = format.channelCount;

    //  Always little endian for RF64
	this->dataEndianess = 'litl';

	switch (format.type) {
		case 1:
			this->dataEncoding = 'PCM ';
			break;
		case 3:
			this->dataEncoding = 'IEEE';
			break;
		default:
			this->dataEncoding  = 0x00000000;
			break;
	}
	
	this->numSamples        = this->dataBlockSize / (this->bitsPerSample / 8);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openAIFF()
{
	commChunk_t format;
	ssndChunk_t ssndChunk;

	//	Look for Chunks and save appropriatly.
	do {
		chunkHead_t chunkExam;
		chunkExam.id      = 0;
		chunkExam.size.be = 0;	//	big endian

		this->fileStm.read((char *) &chunkExam, sizeof(chunkExam));

		switch (chunkExam.id) {
			case 'COMM':
				this->fileStm.read((char *) &format, sizeof(format));
				this->sampleRate    = (uint16_t) bigExt80ToNativeLongDouble(format.sampleRate);
				this->bitsPerSample	= ceil((float_t)format.sampleSize / 8.0 ) * 8;
				this->numChannels   = format.numChannels;
				this->numSamples    = this->numChannels * format.numSampleFrames;
				this->dataBlockSize = this->numSamples * (this->bitsPerSample / 8.0);
				break;

			case 'SSND':
				this->fileStm.read((char *) &ssndChunk, 8);
				if(ssndChunk.offset != 0 && ssndChunk.blockSize != 0) {
                    throw runtime_error("Can't handle files with SSND.offset or SSND.blockSize set to other than zero.");
				}
				this->dataBlockStart = this->fileStm.tellg();
				this->fileStm.seekg(chunkExam.size.be - 8, ios_base::cur);
				break;

				// skip over other chunks.
			default:
				this->fileStm.seekg(chunkExam.size.be, ios_base::cur);
				break;
		}
	}
	while (this->fileStm.good());
	
	this->dataType      = 'NONE';
	//  AIFF files are always big endian and PCM integer
	this->dataEndianess = 'big ';
	this->dataEncoding  = 'PCM ';
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openAIFC()
{
	commExtChunk_t format;
	ssndChunk_t ssndChunk;

	//	Look for Chunks and save appropriatly.
	do {
		chunkHead_t chunkExam;
		chunkExam.id      = 0;
		chunkExam.size.be = 0;	//	big endian

		this->fileStm.read((char *) &chunkExam, 8);

		switch (chunkExam.id) {
			case 'COMM':  //  Extended Common Chunk
				this->fileStm.read((char *) &format, sizeof(format));
				this->fileStm.seekg(chunkExam.size.be - sizeof(format), ios_base::cur);
				break;

			case 'SSND':
				this->fileStm.read((char *) &ssndChunk, 8);
				if(ssndChunk.offset != 0 && ssndChunk.blockSize != 0) {
                    throw runtime_error("Can't handle files with SSND.offset or SSND.blockSize set to other than zero.");
				}
				this->dataBlockStart = this->fileStm.tellg();
				this->fileStm.seekg(chunkExam.size.be - 8, ios_base::cur);
				break;

				// skip over other chunks.
			default:
				this->fileStm.seekg(chunkExam.size.be, ios_base::cur);
				break;
		}
	}
	while (this->fileStm.good());

	this->sampleRate    = (uint16_t) bigExt80ToNativeLongDouble(format.sampleRate);
	this->bitsPerSample	= ceil((float_t)format.sampleSize / 8.0 ) * 8;
	this->numChannels   = format.numChannels;
	this->numSamples    = this->numChannels * format.numSampleFrames;
	this->dataBlockSize = this->numSamples * (this->bitsPerSample / 8);

	switch (format.compressionType) {
        case 'NONE':
			this->dataEndianess = 'big ';
			this->dataEncoding  = 'PCM ';
        	break;

        case 'swot' :
			this->dataEndianess = 'litl';
			this->dataEncoding  = 'PCM ';
        	break;

        case 'fl32':
        case 'FL32':  //  endianess?
			this->dataEndianess = 'big ';
			this->dataEncoding  = 'IEEE';
        	break;

        default:
			this->dataEndianess = 'big ';
			this->dataEncoding  = 0x00000000;
        	break;
	}

	this->dataType = format.compressionType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char* AudioFile::ReadRawData()
{
	auto data = (unsigned char*) calloc(this->dataBlockSize + 8, 1);
	this->fileStm.clear();
	this->fileStm.seekg(this->dataBlockStart);
	this->fileStm.read((char*)data, this->dataBlockSize);
	return data;
}

void AudioFile::WriteRawData(unsigned char* data)
{
	this->fileStm.clear();
	this->fileStm.seekp(this->dataBlockStart);
	this->fileStm.write((char*)data, this->dataBlockSize);
}


// void AudioFile::ReverseCopyBytes(unsigned char * dest, unsigned char * src, uint64_t count)
// {
//    for(uint64_t d = 0, s = count - 1; d < count; ++d, --s) {
//        dest[d] = src[s];
//    }
// }
void AudioFile::ReverseCopy4Bytes(unsigned char * dest, unsigned char * src)
{
	dest[0] = src[3];
	dest[1] = src[2];
	dest[2] = src[1];
	dest[3] = src[0];
}


void AudioFile::assertDataFormat() {
	if ( this->dataEncoding != 'PCM ' && this->dataEncoding != 'IEEE' ) {
		throw runtime_error("ERROR: This method only handles PCM (integer) or IEEE (32-bit float) data in audio files.");
	}
	
	if ( this->dataEncoding == 'PCM ' && this->bitsPerSample >= 32 ) {
		throw runtime_error("ERROR: Cannot convert 32-bit integer (PCM) data to 32-bit float without loss of resolution.");
	}

	if ( this->bitsPerSample > 32 ) {
		throw runtime_error("ERROR: Cannot read data without loss of resolution.");
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * ReadSamples
 * This method converts 8-, 16-, 24-bit integers (PCM), and returns 32-bit machine native floats (IEEE).
 */
void AudioFile::ReadSamples()
{
    this->assertDataFormat();
	auto dataBlock = this->ReadRawData();
	auto tempInt8  = (uint8_t*) dataBlock;
	auto tempFloat = (float*) dataBlock;
	
	uint_fast64_t s;	//	Index variable for samples.
	unsigned char* dPtr;
	
	this->samples.resize(this->numSamples);
	this->samples.shrink_to_fit();

	if ( this->bitsPerSample == 8 ) {
		for(s = 0; s < this->numSamples; ++s) {
			this->samples[s] = (float) tempInt8[s] - 127;
		}
	}
	else if ( this->bitsPerSample > 32 ) {
		throw runtime_error("ERROR: Cannot read data without loss of resolution.");
	}
	else {
		switch (this->dataEndianess) {
			case 'litl':
				switch (this->bitsPerSample) {
					case 16:
					for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=2) {
						this->samples[s] = (float) load_little_s16(dPtr);
					}
					break;
					
					case 24:
					for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=3) {
						this->samples[s] = (float) load_little_s24(dPtr);
					}
					break;
					
					//  Data is 32-bit float.
					case 32:
					#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
						for(s = 0; s < this->numSamples; ++s) {
							this->samples[s] = (float) tempFloat[s];
						}
					#else   //  __ORDER_BIG_ENDIAN__
						for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=4) {
							this->ReverseCopy4Bytes((unsigned char*)&this->samples[s], dPtr);
						}
					#endif
					break;
					
					default:
						throw runtime_error("Should not get here.");
				}
				break;

			case 'big ':
			    switch (this->bitsPerSample) {
					case 16:
					for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=2) {
						this->samples[s] = (float) load_big_s16(dPtr);
					}
					break;
					
					case 24:
					for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=3) {
						this->samples[s] = (float) load_big_s24(dPtr);
					}
					break;
					
					//  Data is 32-bit float.
					case 32:
					#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
						for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=4) {
							this->ReverseCopy4Bytes((unsigned char*)&this->samples[s], dPtr);
						}
					#else   //  __ORDER_BIG_ENDIAN__
						for(s = 0; s < this->numSamples;) {
							this->samples[s] = (float) tempFloat[s];
						}
					#endif
                    break;
					
					default:
						throw runtime_error("Should not get here.");
			    }
				break;
			
			default:
				throw runtime_error("Should not get here.");
		}
	}
	
	delete dataBlock;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//	Assumes data will be the same size as the buffer that was read.
void AudioFile::WriteSamples()
{
    this->assertDataFormat();
	auto dataBlock = (unsigned char*) calloc(this->dataBlockSize, 1);
	auto tempInt8  = (uint8_t*) dataBlock;

	uint_fast64_t s;	//	Index variable for samples.
	unsigned char* dPtr;

	if ( this->bitsPerSample == 8 ) {
		for(s = 0; s < this->numSamples; ++s) {
			tempInt8[s] = (uint8_t) this->samples[s] + 127;
		}
	}
	else {
		switch (this->dataEndianess) {
			case 'litl':
			    switch (this->bitsPerSample) {
                    case 16:
					for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=2) {
						store_little_s16((unsigned char*) dPtr, (int16_t) this->samples[s]);
                    }
                    break;

                    case 24:
                    for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=3) {
						store_little_s24((unsigned char*) dPtr, (int32_t) this->samples[s]);
                    }
                    break;

                    case 32:
                    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    	delete dataBlock;
                    	dataBlock = (unsigned char*) this->samples.data();
                    #else   //  __ORDER_BIG_ENDIAN__
						for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=4) {
							this->ReverseCopy4Bytes(dPtr, (unsigned char*)&this->samples[s]);
						}
                    #endif
                    break;
					
					default:
						//  Should not get here.
						throw new exception();
			    } 
				break;

			case 'big ':
			    switch (this->bitsPerSample) {
                    case 16:
					for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=2) {
						store_big_s16(dPtr, (int16_t) this->samples[s]);
                    }
                    break;

                    case 24:
                    for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=3) {
						store_big_s24(dPtr, (int32_t) this->samples[s]);
                    }
                    break;

                    case 32:
                    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
						for(s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr+=4) {
							this->ReverseCopy4Bytes(dPtr, (unsigned char*)&this->samples[s]);
						}
                    #else   //  __ORDER_BIG_ENDIAN__
                    	delete dataBlock;
                    	dataBlock = (unsigned char*) this->samples.data();
                    #endif
                    break;
					
					default:
				        throw runtime_error("Should not get here.");
			    }
				break;
					
			default:
				throw runtime_error("Should not get here.");
		}
	}

	WriteRawData(dataBlock);
	if (dataBlock != (unsigned char*) samples.data()) delete dataBlock;
}
