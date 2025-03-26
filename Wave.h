/**
 *	WAVE file format.
 */

#ifndef DISKERROR_WAVE_H
#define DISKERROR_WAVE_H
#pragma once

#include <boost/endian.hpp>
#include "mmreg.h"

using namespace boost::endian;


typedef big_uint32_t chunkID_t;
typedef big_uint32_t ckID_t;

// Parent class
struct ChunkID {
    chunkID_t id;
};

typedef struct Chunk : ChunkID {
//	chunkID_t		id;
    little_uint32_t size;
    char8_t         data[];
} Chunk_t;

typedef struct HeaderChunk : ChunkID {
//	chunkID_t		id;
    little_uint32_t size;
    big_uint32_t    type;
} HeaderChunk_t;


//	Sample header chunks:
//	RIFF Chunk:
//		id = 'RIFF'
//		size = file size -8 (minus sizeof id and size)
//		type = 'WAVE'

//	RF64 Chunk:
//		id = 'RF64'
//		size = 0xFFFFFFFF meaning don't use this size member
//		type = 'WAVE'


//	Data Chunk:
//		id = 'data'
//		size = size of data[] in bytes
//		data[samples] = sample size and type described by one of the format chunks

//	JUNK Chunk:
//		id = 'JUNK'
//		size = size of data
//		data[] =  <anything>

//	Format Chunk:
//		id = 'fmt '
//		size = 16
//		data = FormatData
//	or
//		id = 'fmt '
//		size = 40
//		data = FormatExtensibleData


//	RF64 Size Chunk: id = 'big1' (this chunk is a big one)
//		size of data in Data Chunk
typedef struct Size64Chunk : ChunkID {
//	chunkID_t		id;
    little_int64_t	size;
} ChunkSize64_t;

//	Format data structure.
//	Chunk data with type 'fmt ' and size 16 will have this structure.
typedef struct FormatData {
    little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, etc.
    little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
    little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
    little_uint32_t bytesPerSecond;    // average, only important for compressed formats
    little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
    little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
} FormatData_t;

//	Chunk data with type 'fmt ' and size 18 will have this structure.
typedef struct FormatPlusData : FormatData {
//	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, etc.
//	little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
//	little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
//	little_uint32_t bytesPerSecond;    // average, only important for compressed formats
//	little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
//	little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
    little_uint16_t cbSize;            // extra information (after cbSize) to store
} FormatPlusData_t;

//	Chunk data with type 'fmt ' and size 40 will have this structure.
typedef struct FormatExtensibleData : FormatPlusData {
//	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, etc.
//	little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
//	little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
//	little_uint32_t bytesPerSecond;    // average, only important for compressed formats
//	little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
//	little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
//	little_uint16_t cbSize;            // extra information (after cbSize) to store
    union {
        little_uint16_t wValidBitsPerSample;       /* bits of precision  */
        little_uint16_t wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        little_uint16_t wReserved;                 /* If neither applies, set to zero. */
    } Samples;
    little_uint32_t channelMask;
    GUID subFormat;
} FormatExtensibleData_t;

//	Describes Broadcast Audio Extension data structure.
//		id = 'best'
//		size >= 602
typedef struct BroadcastAudioExtension {
    char Description[256];          // ASCII : Description of the sound sequence
    char Originator[32];            // ASCII : Name of the originator
    char OriginatorReference[32];   // ASCII : Reference of the originator
    char OriginationDate[10];       // ASCII : yyyy:mm:dd
    char OriginationTime[8];        // ASCII : hh:mm:ss
    little_int64_t TimeReference;   // First sample count since midnight
    little_int16_t Version;               // Version of the BWF; unsigned binary number
    uint8_t UMID[64];                     // Binary SMPTE UMID
    little_int16_t LoudnessValue;         // Integrated Loudness Value of the file in LUFS x100
    little_uint16_t LoudnessRange;        // Maximum True Peak Level of the file expressed as dBTP x100
    little_int16_t MaxTruePeakLevel;      // Maximum True Peak Level of the file expressed as dBTP x100
    little_int16_t MaxMomentaryLoudness;  // Highest value of the Momentary Loudness Level of the file in LUFS (x100)
    little_int16_t MaxShortTermLoudness;  // Highest value of the Short-Term Loudness Level of the file in LUFS (x100)
    uint8_t Reserved[180];   // Reserved for future use, set to NULL
    char CodingHistory[0];   // ASCII : History coding
} BroadcastAudioExt_t;

//	Describes size of data in 'data' chunk.
//		id = 'ds64'
//		size >= 28
typedef struct Size64Data {
    little_int64_t riffSize;       // size of RF64 block
    little_int64_t dataSize;       // size of data chunk
    little_int64_t sampleCount;    // sample count of fact chunk
    little_int32_t tableLength;    // number of valid entries in array “table”
    Size64Chunk table[];
} Size64Data_t;


#endif /* DISKERROR_WAVE_H */
