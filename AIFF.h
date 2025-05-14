/**
 *	AIFF file format.
 */

#ifndef DISKERROR_AIFF_H
#define DISKERROR_AIFF_H
#pragma once

#include <boost/endian.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>

using namespace boost;
using namespace boost::endian;

#include <cmath>
#include <limits>

using namespace std;


long double bigExt80ToNativeLongDouble(const char *data)
{
	typedef struct {
		big_uint16_t sign_exponent;
		big_uint64_t mantissa;
	} big_ext80_struct_t;

	big_ext80_struct_t *parts = (big_ext80_struct_t *) data;

	const big_int16_t big_7F_16_mask = 0x7FFF;
	const big_int64_t big_7F_64_mask = 0x7FFFFFFFFFFFFFFF;

	const long double sign     = *data & 0x80 ? -1.0 : 1.0;
	const int16_t     exponent = parts->sign_exponent & big_7F_16_mask;

	//  unusual number, infinity or NaN,
	if ( exponent == big_7F_16_mask ) {
		//  infinity, check all 64 bits
		if ( ( parts->mantissa & big_7F_64_mask ) == 0 ) {
			return ( sign < 0 ) ?
			       -numeric_limits<long double>::infinity() :
			       numeric_limits<long double>::infinity();
		}

		// highest mantissa bit set means quiet NaN
		return ( *( data + 2 ) & 0x80 ) ?
		       numeric_limits<long double>::quiet_NaN() :
		       numeric_limits<long double>::signaling_NaN();
	}

	// 1 bit sign, 15 bit exponent, 64 bit mantissa
	// value = (-1) ^ s * (normalizeBit + m / 2 ^ 63) * 2 ^ (e - 16383)
	return sign * (long double) parts->mantissa * powl(2.0L, ( (long double) exponent - 16383.0L ) - 63.0L);
}


// Parent class
struct aChunkID {
	big_uint32_t id;
};

typedef struct aChunkHead : aChunkID {
//	big_uint32_t	id;
	big_uint32_t size;
} aChunkHead_t;

typedef struct aChunk : aChunkHead {
//	big_uint32_t	id;
//  big_uint32_t size;
	char8_t data[];        //	data[size]
} aChunk_t;


//	Sample header chunks:
//	FORM Chunk:
//		id = 'FORM'
//		size = file size -8 (minus sizeof id and size)
//		type = 'AIFF' or 'AIFC'

//	Format Chunk:
//		id = 'COMM'
//		size = 18
//		data = commData


//	Data format structure.
//	Chunk data with type 'COMM' and size 18 will have this structure.
typedef struct {
	big_uint16_t numChannels;     // 1 = mono, 2 = stereo, etc.
	big_uint32_t numSampleFrames; // a frame is one sample for each channel
	big_uint16_t sampleSize;      // valid bits per sample 8, 16, 24, etc.
	char         sampleRate[10];  // 32000, 44100, 48000, etc. as a big endian 80-bit extended float
} commChunk_t;

//	Chunk data with type 'COMM' and size 18 will have this structure.
typedef struct {
	big_uint16_t numChannels;     // 1 = mono, 2 = stereo, etc.
	big_uint32_t numSampleFrames; // a frame is one sample for each channel
	big_uint16_t sampleSize;      // valid bits per sample 8, 16, 24, etc.
	char         sampleRate[10];  // 32000, 44100, 48000, etc. as a big endian 80-bit extended float
	big_uint32_t compressionType; //
	char         compressionName[1];     /* variable length array, Pascal string */
} commExtChunk_t;

//  Sound data chunk. 'SSND'
//	dataBlockSize should be the chunk size - 8 for offset and blockSize members
//  dataBlockSize will be derived from numSampleFrames * numChannels * ceil(sampleSize / 8)
typedef struct {
	big_uint32_t offset;    // Determines where the first sample frame in the soundData starts. Offset is in bytes.
	big_uint32_t blockSize;
//    char8_t            data[];          //	data[size]
} ssndChunk_t;
// offset  determines where the first sample frame in the soundData starts.  offset is in bytes.
//    Most applications won't use offset and should set it to zero.  Use for a non-zero offset
//    is explained in the Block-Aligning Sound Data section below.

// blockSize  is used in conjunction with offset for block-aligning sound data.
//    It contains the size in bytes of the blocks that sound data is aligned to.
//    As with offset, most applications won't use blockSize and should set it to zero.
//    More information on blockSize is in the Block-Aligning Sound Data section below.

// soundData  contains the sample frames that make up the sound.
//    The number of sample frames in the soundData is determined by the numSampleFrames parameter in the Common Chunk.


//	Describes Broadcast Audio Extension data structure.
//		id = 'best'
//		size >= 602
//typedef struct BroadcastAudioExtension {
//    char Description[256];          // ASCII : Description of the sound sequence
//    char Originator[32];            // ASCII : Name of the originator
//    char OriginatorReference[32];   // ASCII : Reference of the originator
//    char OriginationDate[10];       // ASCII : yyyy:mm:dd
//    char OriginationTime[8];        // ASCII : hh:mm:ss
//    big_int64_t TimeReference;   // First sample count since midnight
//    big_int16_t Version;               // Version of the BWF; unsigned binary number
//    uint8_t UMID[64];                     // Binary SMPTE UMID
//    big_int16_t LoudnessValue;         // Integrated Loudness Value of the file in LUFS x100
//    big_uint16_t LoudnessRange;        // Maximum True Peak Level of the file expressed as dBTP x100
//    big_int16_t MaxTruePeakLevel;      // Maximum True Peak Level of the file expressed as dBTP x100
//    big_int16_t MaxMomentaryLoudness;  // Highest value of the Momentary Loudness Level of the file in LUFS (x100)
//    big_int16_t MaxShortTermLoudness;  // Highest value of the Short-Term Loudness Level of the file in LUFS (x100)
//    uint8_t Reserved[180];   // Reserved for future use, set to NULL
//    char CodingHistory[0];   // ASCII : History coding
//} BroadcastAudioExt_t;



#endif /* DISKERROR_AIFF_H */
