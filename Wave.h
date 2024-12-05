/**
	WAVE file format.
*/

#ifndef DISKERROR_WAVE_H
#define DISKERROR_WAVE_H
#pragma once

#include <boost/endian.hpp>

using namespace boost::endian;

#define SPEAKER_FRONT_LEFT 0x00000001
#define SPEAKER_FRONT_RIGHT 0x00000002
#define SPEAKER_FRONT_CENTER 0x00000004
#define SPEAKER_LOW_FREQUENCY 0x00000008
#define SPEAKER_BACK_LEFT 0x00000010
#define SPEAKER_BACK_RIGHT 0x00000020
#define SPEAKER_FRONT_LEFT_OF_CENTER 0x00000040
#define SPEAKER_FRONT_RIGHT_OF_CENTER 0x00000080
#define SPEAKER_BACK_CENTER 0x00000100
#define SPEAKER_SIDE_LEFT 0x00000200
#define SPEAKER_SIDE_RIGHT 0x00000400
#define SPEAKER_TOP_CENTER 0x00000800
#define SPEAKER_TOP_FRONT_LEFT 0x00001000
#define SPEAKER_TOP_FRONT_CENTER 0x00002000
#define SPEAKER_TOP_FRONT_RIGHT 0x00004000
#define SPEAKER_TOP_BACK_LEFT 0x00008000
#define SPEAKER_TOP_BACK_CENTER 0x00010000
#define SPEAKER_TOP_BACK_RIGHT 0x00020000
#define SPEAKER_ALL 0x80000000

#define SPEAKER_STEREO_LEFT 0x20000000
#define SPEAKER_STEREO_RIGHT 0x40000000

#define SPEAKER_CONTROLSAMPLE_1 0x08000000
#define SPEAKER_CONTROLSAMPLE_2 0x10000000

#define SPEAKER_BITSTREAM_1_LEFT 0x00800000
#define SPEAKER_BITSTREAM_1_RIGHT 0x01000000
#define SPEAKER_BITSTREAM_2_LEFT 0x02000000
#define SPEAKER_BITSTREAM_2_RIGHT 0x04000000

// Parent class
struct ChunkID
{
	big_uint32_t id;
//	little_uint32_t size;
};

typedef struct Chunk : ChunkID
{
//	big_uint32_t    id;
	little_uint32_t size;
	char8_t         data[];
} Chunk_t;

typedef struct HeaderChunk : ChunkID
{
//	big_uint32_t    id;
	little_uint32_t size;
	big_uint32_t    type;
} HeaderChunk_t;


//	Sample header chunks:
//	RIFF Chunk:
//		id = 'RIFF'
//		size = file size -8 (minus sizeof id and size)
//		data[4] = 'WAVE'

//	RF64 Chunk:
//		id = 'RF64'
//		size = 0xFFFFFFFF meaning don't use this size member
//		data[4] = 'WAVE'


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
typedef struct Size64Chunk : ChunkID
{
//	big_uint32_t   id;
	little_int64_t size;
} ChunkSize64_t;

//	Format data structure.
//	Chunk data with type 'fmt ' and size 16 will have this structure.
typedef struct FormatData
{
	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, etc.
	little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
	little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
	little_uint32_t bytesPerSecond;    // average, only important for compressed formats
	little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
	little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
} FormatData_t;

//	Chunk data with type 'fmt ' and size 18 will have this structure.
typedef struct FormatPlusData : FormatData
{
//	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, etc.
//	little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
//	little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
//	little_uint32_t bytesPerSecond;    // average, only important for compressed formats
//	little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
//	little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
	little_uint16_t cbSize;            // extra information (after cbSize) to store
} FormatPlusData_t;

typedef struct Guid
{
	big_uint32_t data1;
	big_uint16_t data2;
	big_uint16_t data3;
	big_uint32_t data4;
	big_uint32_t data5;
} Guid_t;

//	Chunk data with type 'fmt ' and size 40 will have this structure.
typedef struct FormatExtensibleData : FormatPlusData
{
//	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, etc.
//	little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
//	little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
//	little_uint32_t bytesPerSecond;    // average, only important for compressed formats
//	little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
//	little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
//	little_uint16_t cbSize;            // extra information (after cbSize) to store
	little_uint16_t validBitsPerSample;
	little_uint32_t channelMask;
	Guid_t          subFormat;
} FormatExtensibleData_t;

//	Chunk data with type 'bext' and minimum size of 602 will have this structure.
typedef struct BroadcastAudioExtension
{
	char           Description[256];          // ASCII : Description of the sound sequence
	char           Originator[32];            // ASCII : Name of the originator
	char           OriginatorReference[32];   // ASCII : Reference of the originator
	char           OriginationDate[10];       // ASCII : yyyy:mm:dd
	char           OriginationTime[8];        // ASCII : hh:mm:ss
	little_int64_t TimeReference;             // First sample count since midnight
	little_int16_t Version;                   // Version of the BWF; unsigned binary number
	uint8_t        UMID[64];                  // Binary SMPTE UMID
	little_int16_t LoudnessValue;       // Integrated Loudness Value of the file in LUFS x100
	little_int16_t LoudnessRange;       // Maximum True Peak Level of the file expressed as dBTP x100
	little_int16_t MaxTruePeakLevel;    // Maximum True Peak Level of the file expressed as dBTP x100
	little_int16_t MaxMomentaryLoudness;  // Highest value of the Momentary Loudness Level of the file in LUFS (x100)
	little_int16_t MaxShortTermLoudness;  // Highest value of the Short-Term Loudness Level of the file in LUFS (x100)
	uint8_t        Reserved[180];     // Reserved for future use, set to NULL
	char           CodingHistory[0];   // ASCII : History coding
} BroadcastAudioExt_t;

//	Describes size of data in 'data' chunk.
//		id = 'ds64'
//		size >= 28
typedef struct Size64Data
{
	little_int64_t riffSize;       // size of RF64 block
	little_int64_t dataSize;       // size of data chunk
	little_int64_t sampleCount;    // sample count of fact chunk
	little_int32_t tableLength;    // number of valid entries in array “table”
	Size64Chunk    table[];
} Size64Data_t;


#endif /* DISKERROR_WAVE_H */
