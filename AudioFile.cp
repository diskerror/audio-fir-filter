//
// Created by Reid Woodbury.
//

#include "AudioFile.h"
#include "Wave.h"
#include "AIFF.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Diskerror {
using namespace std;
using namespace boost;
using namespace boost::endian;

//  Utility function to convert a 32-bit integer to a string of 4 characters.
string bigInt2str(big_uint32_t fc) {
    const auto c = reinterpret_cast<char*>(&fc);
    return string("") + c[0] + c[1] + c[2] + c[3];
}


typedef struct ChunkHead {
    big_uint32_t id;

    union {
        big_uint32_t    be;
        little_uint32_t le;
    } size;
} chunkHead_t;


/**
 * Find the container chunk type and size.
 * Defines all possible interpretations of the first 12 bytes, WAVE or AIFF.
 */
typedef struct {
    big_uint32_t baseID;

    union {
        big_uint32_t    be;
        little_uint32_t le;
    } size;

    big_uint32_t type;
} mediaHeader_t;


////////////////////////////////////////////////////////////////////////////////////////////////////
AudioFile::AudioFile(const filesystem::path& fPath) : file(fPath) {
    mediaHeader_t header;
    header.baseID  = 0;
    header.size.le = 0;
    header.type    = 0;

    if (!filesystem::is_regular_file(fPath)) throw runtime_error("Not a regular file.");

    this->fileStm.open(this->file.string(), ios_base::in | ios_base::out | ios_base::binary);
    if (this->fileStm.fail()) {
        throw invalid_argument("There was a problem opening the input file.");
    }

    //	Audio files always have a 12 byte header.
    this->fileStm.read(reinterpret_cast<char*>(&header), sizeof(header));
    this->fileType    = header.baseID;
    this->fileSubType = header.type;
    switch (this->fileType) {
        case 'RIFF':
            if (this->fileSubType != 'WAVE') {
                throw runtime_error("ERROR: Unknown media type.");
            }

            openRIFF();
            break;

        case 'RF64':
            if (this->fileSubType != 'WAVE') {
                throw runtime_error("ERROR: Unknown media type.");
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
                    throw runtime_error("ERROR: Unknown media type.");
            }
            break;

        default:
            throw runtime_error("ERROR: Unknow file type.");
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openRIFF() {
    FormatData_t format;

    //	Look for Chunks and save appropriatly.
    do {
        chunkHead_t chunkExam;
        chunkExam.id      = 0;
        chunkExam.size.le = 0;

        this->fileStm.read(reinterpret_cast<char*>(&chunkExam), 8);

        switch (chunkExam.id) {
            case 'fmt ':
                this->fileStm.read(reinterpret_cast<char*>(&format), sizeof(format));

                //	skip over remainder, if any
                this->fileStm.seekg(chunkExam.size.le - sizeof(format), ios_base::cur);
                break;

            case 'data':
                this->dataBlockSize = chunkExam.size.le;
                this->dataBlockStart = this->fileStm.tellg();
            //	fall through

            // skip over other chunks.
            default:
                this->fileStm.seekg(chunkExam.size.le, ios_base::cur);
                break;
        }
    }
    while (this->fileStm.good());

    this->dataType      = 'ms  ' & static_cast<big_uint32_t>(format.type);
    this->sampleRate    = format.sampleRate;
    this->bitsPerSample = static_cast<uint16_t>(ceil(format.bitsPerSample / 8.0)) * 8;
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
            this->dataEncoding = 0x00000000;
            break;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openRF64() {
    FormatData_t format;
    DataSize64_t ds64;
    ds64.dataSize = 0;

    //	Look for Chunks and save appropriatly.
    do {
        chunkHead_t chunkExam;
        chunkExam.id      = 0;
        chunkExam.size.le = 0; //	little endian

        this->fileStm.read(reinterpret_cast<char*>(&chunkExam), 8);

        switch (chunkExam.id) {
            case 'fmt ':
                this->fileStm.read(reinterpret_cast<char*>(&format), sizeof(format));

                //	skip over remainder, if any
                this->fileStm.seekg(chunkExam.size.le - sizeof(format), ios_base::cur);
                break;

            case 'ds64':
                this->fileStm.read(reinterpret_cast<char*>(&ds64), sizeof(ds64));
                this->dataBlockSize = ds64.dataSize;

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

    this->dataType      = 'ms  ' & static_cast<big_uint32_t>(format.type);
    this->sampleRate    = format.sampleRate;
    this->bitsPerSample = static_cast<uint16_t>(ceil(format.bitsPerSample / 8.0)) * 8;
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
            this->dataEncoding = 0x00000000;
            break;
    }

    this->numSamples = this->dataBlockSize / (this->bitsPerSample / 8);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openAIFF() {
    commChunk_t format;
    ssndChunk_t ssndChunk;

    //	Look for Chunks and save appropriatly.
    do {
        chunkHead_t chunkExam;
        chunkExam.id      = 0;
        chunkExam.size.be = 0; //	big endian

        this->fileStm.read(reinterpret_cast<char*>(&chunkExam), sizeof(chunkExam));

        switch (chunkExam.id) {
            case 'COMM':
                this->fileStm.read(reinterpret_cast<char*>(&format), sizeof(format));
                this->sampleRate    = static_cast<uint16_t>(bigExt80ToNativeLongDouble(format.sampleRate));
                this->bitsPerSample = static_cast<uint16_t>(ceil(format.sampleSize / 8) * 8);
                this->numChannels   = format.numChannels;
                this->numSamples    = this->numChannels * format.numSampleFrames;
                this->dataBlockSize = static_cast<int64_t>(this->numSamples * (this->bitsPerSample / 8));
                break;

            case 'SSND':
                this->fileStm.read(reinterpret_cast<char*>(&ssndChunk), 8);
                if (ssndChunk.offset != 0 && ssndChunk.blockSize != 0) {
                    throw runtime_error(
                        "Can't handle files with SSND.offset or SSND.blockSize set to other than zero.");
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

    this->dataType = 'NONE';
    //  AIFF files are always big endian and PCM integer
    this->dataEndianess = 'big ';
    this->dataEncoding  = 'PCM ';
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void AudioFile::openAIFC() {
    commExtChunk_t format;
    ssndChunk_t    ssndChunk;

    //	Look for Chunks and save appropriatly.
    do {
        chunkHead_t chunkExam;
        chunkExam.id      = 0;
        chunkExam.size.be = 0; //	big endian

        this->fileStm.read(reinterpret_cast<char*>(&chunkExam), 8);

        switch (chunkExam.id) {
            case 'COMM': //  Extended Common Chunk
                this->fileStm.read(reinterpret_cast<char*>(&format), sizeof(format));
                this->fileStm.seekg(chunkExam.size.be - sizeof(format), ios_base::cur);
                break;

            case 'SSND':
                this->fileStm.read(reinterpret_cast<char*>(&ssndChunk), 8);
                if (ssndChunk.offset != 0 && ssndChunk.blockSize != 0) {
                    throw runtime_error(
                        "Can't handle files with SSND.offset or SSND.blockSize set to other than zero.");
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

    this->sampleRate    = static_cast<uint16_t>(bigExt80ToNativeLongDouble(format.sampleRate));
    this->bitsPerSample = static_cast<uint16_t>(ceil(format.sampleSize / 8.0)) * 8;
    this->numChannels   = format.numChannels;
    this->numSamples    = this->numChannels * format.numSampleFrames;
    this->dataBlockSize = this->numSamples * (this->bitsPerSample / 8);

    switch (format.compressionType) {
        case 'NONE':
            this->dataEndianess = 'big ';
            this->dataEncoding = 'PCM ';
            break;

        case 'swot':
            this->dataEndianess = 'litl';
            this->dataEncoding = 'PCM ';
            break;

        case 'fl32':
        case 'FL32': //  endianess?
            this->dataEndianess = 'big ';
            this->dataEncoding = 'IEEE';
            break;

        default:
            this->dataEndianess = 'big ';
            this->dataEncoding = 0x00000000;
            break;
    }

    this->dataType = format.compressionType;
}


//	Maximum value storable == 2^(nBits-1) - 1
float32_t AudioFile::getSampleMaxMagnitude() const {
    //	if ( dataEncoding != 'PCM ' ) throw logic_error("Only PCM sammples have a maximum magnitude.");
    switch (this->getBitsPerSample()) {
        case 8:
            return MAX_VALUE_8_BIT; //        127

        case 16:
            return MAX_VALUE_16_BIT; //     32,767

        case 24:
            return MAX_VALUE_24_BIT; //  8,388,607

        case 32:
            return 1.0; //  Assume IEEE

        default:
            throw runtime_error("Can't handle PCM files with this bit depth.");
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char* AudioFile::ReadAllData() {
    const auto data = static_cast<unsigned char*>(calloc(this->dataBlockSize + 8, 1));
    this->fileStm.clear();
    this->fileStm.seekg(this->dataBlockStart);
    this->fileStm.read(reinterpret_cast<char*>(data), this->dataBlockSize);
    return data;
}


void AudioFile::WriteAllData(const unsigned char*data) {
    this->fileStm.clear();
    this->fileStm.seekp(this->dataBlockStart);
    this->fileStm.write(reinterpret_cast<const char*>(data), this->dataBlockSize);
}
} //  namespace Diskerror
