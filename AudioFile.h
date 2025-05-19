//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_AUDIOFILE_H
#define DISKERROR_AUDIOFILE_H


#include <cfloat>
#include <filesystem>
#include <fstream>
#include <random>
#include <vector>
#include <boost/cstdfloat.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/limits.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>

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

	// These properties are used as four character codes.
	big_uint32_t fileType      = 0x00000000; // RIFF, RF64, FORM, FORM
	big_uint32_t fileSubType   = 0x00000000; // WAVE, WAVE, AIFF, AIFC
	big_uint32_t dataEncoding  = 0x00000000; // 'PCM ' integer, 'IEEE' float
	big_uint32_t dataEndianess = 0x00000000; // 'big ' or 'litl'

	big_uint32_t dataType       = 0x00000000;
	uint16_t     numChannels    = 0;
	uint32_t     sampleRate     = 0;
	uint16_t     bitsPerSample  = 0;
	uint64_t     numSamples     = 0;
	uint64_t     dataBlockStart = 0;
	uint64_t     dataBlockSize  = 0;

	void openRIFF();    //	RIFF/WAVE < 4G
	void openRF64();    //	RF64/WAVE > 4G
	void openAIFF();    //	FORM/AIFF < 4G
	void openAIFC();    //	FORM/AIFC   ?

//    void    ReverseCopyBytes( unsigned char * , unsigned char * , uint64_t );
	static void ReverseCopy4Bytes(unsigned char *, unsigned char *);

	void assertDataFormat();

public:
	constexpr static float MAX_VALUE_8_BIT  = 127.0;
	constexpr static float MAX_VALUE_16_BIT = 32767.0;
	constexpr static float MAX_VALUE_24_BIT = 8388607.0;

	// Constructor
	explicit AudioFile(filesystem::path);

	explicit AudioFile(const string &sPath) : AudioFile(filesystem::path(sPath)) {};

	explicit AudioFile(char *cPath) : AudioFile(filesystem::path(cPath)) {};

	// Destructor
	~AudioFile() = default;

	// Exposing these members because of their useful methods.
	const filesystem::path file;
	vector<float32_t>      samples;

	inline big_uint32_t GetDataEncoding() { return this->dataEncoding; };

	inline big_uint32_t GetDataEndianess() { return this->dataEndianess; };

	inline uint16_t GetNumChannels() { return this->numChannels; };

	inline uint32_t GetSampleRate() { return this->sampleRate; };

	inline uint16_t GetBitsPerSample() { return this->bitsPerSample; };

	inline uint64_t GetNumSamples() { return this->numSamples; };

	inline uint64_t GetDataBlockSize() { return this->dataBlockSize; };

	unsigned char *ReadRawData();

	void WriteRawData(unsigned char *);

	void ReadSamples();

	void WriteSamples();

	inline float_t &operator[](uint64_t s) { return samples[s]; }

	// This returns a random number with values -1.0 to 1.0 in triangular distribution.
	inline static float_t dither()
	{
		static random_device                      rd;
		static mt19937                            gen(rd());
		static uniform_real_distribution<float_t> low(-1, 0);
		static uniform_real_distribution<float_t> hi(0, 1);
		return low(gen) + hi(gen);
	}
};

} // namespace Diskerror

#endif // DISKERROR_AUDIOFILE_H
