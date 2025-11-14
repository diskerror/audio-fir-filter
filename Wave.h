/**
 *	WAVE file format.
 */

#ifndef DISKERROR_WAVE_H
#define DISKERROR_WAVE_H
#pragma once

#include <boost/endian.hpp>
using namespace boost::endian;

//	Four-character code.
#ifndef HAS_FOURCC_T
#define HAS_FOURCC_T
typedef big_uint32_t fourcc_t;
#endif

// Parent class
struct ChunkID {
    big_uint32_t id;
};

//typedef struct ChunkHead : ChunkID {
////	big_uint32_t	id;
//    little_uint32_t size;
//} ChunkHead_t;
//
//typedef struct Chunk : ChunkHead {
////	big_uint32_t	id;
////  little_uint32_t size;
//    char8_t         data[];		//	data[size]
//} Chunk_t;
//

//	Sample header chunks:
//	RIFF Chunk:
//		id = 'RIFF'
//		size = file size -8 (minus sizeof id and size)
//		type = 'WAVE'

//	RF64 Chunk:
//		id = 'RF64'
//		size = 0xFFFFFFFF   meaning don't use this size member
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


//	Format data structure.
//	Chunk data with type 'fmt ' and size 16 will have this structure.
typedef struct FormatData {
    little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, WAVE_FORMAT_IEEE_FLOAT = 0x0003 etc.
    little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
    little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
    little_uint32_t bytesPerSecond;    // average, only important for compressed formats
    little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
    little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
} FormatData_t;

//	Chunk data with type 'fmt ' and size 18 will have this structure.
typedef struct FormatPlusData : FormatData {
//	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, WAVE_FORMAT_IEEE_FLOAT = 0x0003 etc.
//	little_uint16_t channelCount;      // 1 = mono, 2 = stereo, etc.
//	little_uint32_t sampleRate;        // 32000, 44100, 48000, etc.
//	little_uint32_t bytesPerSecond;    // average, only important for compressed formats
//	little_uint16_t blockAlignment;    // container size (in bytes) of one set of samples
//	little_uint16_t bitsPerSample;     // valid bits per sample 16, 20 or 24, etc.
    little_uint16_t cbSize;            // extra information (after cbSize) to store
} FormatPlusData_t;

// typedef struct _GUID {
//   unsigned long  Data1;
//   unsigned short Data2;
//   unsigned short Data3;
//   unsigned char  Data4[8];
// } GUID;
typedef struct GUID {
  little_uint32_t	Data1;
  little_uint16_t	Data2;
  little_uint16_t	Data3;
  little_uint32_t	Data4;
  little_uint32_t	Data5;
} GUID_t;
// // For a WAVE_FORMAT_X tag = 0xXYZW the WAVEFORMATEXTENSIBLE SubType GUID is
// // 0000XYZW-0000-0010-8000-00AA00389B71
// #define DEFINE_WAVEFORMATEX_GUID(x) (USHORT)(x), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
// 

//	Chunk data with type 'fmt ' and size 40 will have this structure.
typedef struct FormatExtensibleData : FormatPlusData {
//	little_uint16_t type;              // WAVE_FORMAT_PCM = 0x0001, WAVE_FORMAT_IEEE_FLOAT = 0x0003 etc..
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
    GUID_t			subFormat;
} FormatExtensibleData_t;

//	Describes Broadcast Audio Extension data structure.
//		id = 'bext'
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

//	RF64 Size Chunk: id = 'big1' (this chunk is a big one)
//		size of data in Data Chunk
typedef struct ChunkSize64 : ChunkID {
//	chunkID_t		id;
    little_int64_t	size;
} ChunkSize64_t;

//	Describes size of data in 'data' chunk.
//		id = 'ds64'
//		size >= 28
typedef struct DataSize64Chunk {
    little_int64_t riffSize;       // size of RF64 block
    little_int64_t dataSize;       // size of data chunk
    little_int64_t sampleCount;    // sample count of fact chunk
    little_int32_t tableLength;    // number of valid entries in array “table”
    ChunkSize64_t  table[];
} DataSize64_t;


#endif /* DISKERROR_WAVE_H */
