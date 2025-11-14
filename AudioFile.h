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

/**
 *	class AudioFile
 *	Open, read, and write existing audio files.
 *	The file size and type cannot be changed.
 */
class AudioFile {
	fstream fileStm;

protected:
	// These properties are used as four character codes.
	fourcc_t fileType      = 0x00000000; // RIFF, RF64, FORM
	fourcc_t fileSubType   = 0x00000000; // WAVE, AIFF, AIFC
	fourcc_t dataEncoding  = 0x00000000; // 'PCM ' integers, 'IEEE' floats
	fourcc_t dataEndianess = 0x00000000; // 'big ' or 'litl'

	fourcc_t dataType       = 0x00000000;
	uint16_t numChannels    = 0;
	uint32_t sampleRate     = 0;
	uint16_t bitsPerSample  = 0;
	int64_t  numSamples     = 0;
	int64_t  dataBlockStart = 0;
	int64_t  dataBlockSize  = 0;

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

	const filesystem::path file;

	fourcc_t getDataEncoding() const { return this->dataEncoding; };

	bool is_pcm() const { return this->dataEncoding == static_cast<big_uint32_t>('PCM '); }

	big_uint32_t GetDataEndianess() const { return this->dataEndianess; };

	uint16_t getNumChannels() const { return this->numChannels; };

	uint32_t getSampleRate() const { return this->sampleRate; };

	uint16_t getBitsPerSample() const { return this->bitsPerSample; };

	int64_t getNumFrames() const { return this->numSamples; };

	int64_t getDataSize() const { return this->dataBlockSize; };

	float32_t getSampleMaxMagnitude() const;

	unsigned char* ReadAllData();

	void WriteAllData(const unsigned char*);
};

} // namespace Diskerror

#endif // DISKERROR_AUDIOFILE_H
