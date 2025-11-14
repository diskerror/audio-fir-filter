//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_AUDIOFILE_H
#define DISKERROR_AUDIOFILE_H


#include <filesystem>
#include <fstream>

#include <boost/cstdfloat.hpp>
#include <boost/endian/arithmetic.hpp>

#include "AIFF.h"
#include "Wave.h"

namespace Diskerror {

using namespace std;
using namespace boost;
using namespace boost::endian;

//	Currently only WAVE and AIFF (AIFC) is supported.
//	RIFF or RF64 is determined by total file size.
typedef enum {
	BASE_TYPE_UNKNOWN = 0,
	BASE_TYPE_WAVE, //	RIFF or RF64: WAVE, little-endian (and BWF)
	BASE_TYPE_AIFF, //	FORM: AIFF or AIFC, big-endian
	BASE_TYPE_WAVX, //	RIFX: WAVE, big-endian
	BASE_TYPE_8SVX, //	FORM: 8SVX, Amiga IFF 8-bit, big-endian
	BASE_TYPE_MAUD  //	FORM: MAUD, Amiga IFF multi-channel, big-endian
} baseAudioFileType_t;

typedef struct {
	fourcc_t id;
	//	RIFF or RF64: WAVE, little-endian (and BWF)
	//	FORM: AIFF or AIFC, big-endian (except 'swot')
	union {
		little_uint32_t lSize;
		big_uint32_t    bSize;
	};

	fourcc_t type;
} audioFileHeader_t;

typedef union {
	struct {
		fourcc_t id;

		union {
			little_uint32_t lSize;
			big_uint32_t    bSize;
		};

		char rawData[];
	};

	fmt_t  fmt_; //	WAVE format
	COMM_t COMM; //	AIFF format

//	additional AIFF chunks
	//	minf.size==16?
	SSND_t     SSND;
	aChunk_t   elm1; //	elm1.size==426, filler like JUNK?
	MARK_t     MARK;
	INST_t     INST;
	aiffData_t MIDI;
	aiffData_t AESD;
	APPL_t     APPL;
	COMT_t     COMT;
	text_t     NAME;
	text_t     AUTH;
	text_t     copy;
	text_t     ANNO;
	SAXL_t     SAXL;

//	additional WAVE chunks
	waveData_t data;
	ds64_t     ds64;
	waveData_t JUNK;
	bext_t     bext;
	big1_t     big1;
	fact_t     fact;
	cue_t      cue_;
	plst_t     plst;
	listHead_t list;
	smpl_t     smpl;
	inst_t     inst;
} chunks_t;

class chunkVector : public vector<chunks_t*> {
public:
	~chunkVector() { for (auto& chunk : *this) { delete chunk; } }
	uint32_t getTotalSize(bool is_littleEndian); //	Defined in AudioFile.cp
};

/**
 *	class AudioFile
 *	Open, read, and write existing audio files.
 *	The file size and type cannot be changed.
 */
class AudioFile {
	fstream fileAccess;

protected:
	// These properties are used as four character codes.
	fourcc_t fileType      = 0x00000000; // RIFF, RF64, FORM
	fourcc_t fileSubType   = 0x00000000; // WAVE, AIFF, AIFC
	fourcc_t dataEncoding  = 0x00000000; // 'PCM ' integers, 'IEEE' floats
	fourcc_t dataEndianess = 0x00000000; // 'big ' or 'litl'

	fourcc_t dataType      = 0x00000000;
	uint16_t numChannels   = 0;
	uint32_t sampleRate    = 0;
	uint16_t bitsPerSample = 0;
	int64_t  numSamples    = 0;
	int64_t  dataStart     = 0;
	int64_t  dataSize      = 0;

	void openRIFF(); //	RIFF/WAVE < 4G
	void openRF64(); //	RF64/WAVE > 4G
	void openAIFF(); //	FORM/AIFF < 4G
	void openAIFC(); //	FORM/AIFC   ?

public:
	constexpr static float32_t MAX_VALUE_8_BIT  = 127.0;
	constexpr static float32_t MAX_VALUE_16_BIT = 32767.0;
	constexpr static float32_t MAX_VALUE_24_BIT = 8388607.0;

	// Constructor
	explicit AudioFile(const filesystem::path& fPath);

	// Destructor
	~AudioFile() = default;

	const filesystem::path filePath;


	bool is_pcm() const { return this->dataEncoding == static_cast<big_uint32_t>('PCM '); }
	big_uint32_t GetDataEndianess() const { return this->dataEndianess; };

	uint16_t     getNumChannels() const { return this->numChannels; };
	uint32_t     getSampleRate() const { return this->sampleRate; };
	uint16_t     getBitsPerSample() const { return this->bitsPerSample; };
	int64_t      getNumFrames() const { return this->numSamples; };
	int64_t      getDataSize() const { return this->dataSize; };
	fourcc_t     getDataEncoding() const { return this->dataEncoding; };

	float64_t getSampleMaxMagnitude() const;

	unsigned char* ReadAllData();

	void WriteAllData(const unsigned char*);
};

} // namespace Diskerror

#endif // DISKERROR_AUDIOFILE_H
