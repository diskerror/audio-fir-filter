//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_AUDIOFILE_H
#define DISKERROR_AUDIOFILE_H


#include <filesystem>
#include <fstream>

#include <boost/cstdfloat.hpp>
#include <boost/endian.hpp>

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
    big_uint32_t fileType      = 0x00000000; // RIFF, RF64, FORM
    big_uint32_t fileSubType   = 0x00000000; // WAVE, AIFF, AIFC
    big_uint32_t dataEncoding  = 0x00000000; // 'PCM ' integers, 'IEEE' floats
    big_uint32_t dataEndianess = 0x00000000; // 'big ' or 'litl'

    big_uint32_t dataType       = 0x00000000;
    uint16_t     numChannels    = 0;
    uint32_t     sampleRate     = 0;
    uint16_t     bitsPerSample  = 0;
    int64_t      numSamples     = 0;
    int64_t      dataBlockStart = 0;
    int64_t      dataBlockSize  = 0;

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

    big_uint32_t GetDataEncoding() const { return this->dataEncoding; };

    bool is_pcm() const { return this->dataEncoding == static_cast<big_uint32_t>('PCM '); }

    big_uint32_t GetDataEndianess() const { return this->dataEndianess; };

    uint16_t GetNumChannels() const { return this->numChannels; };

    uint32_t GetSampleRate() const { return this->sampleRate; };

    uint16_t GetBitsPerSample() const { return this->bitsPerSample; };

    int64_t GetNumSamples() const { return this->numSamples; };

    int64_t GetDataBlockSize() const { return this->dataBlockSize; };

    float32_t GetSampleMaxMagnitude() const;

    unsigned char* ReadRawData();

    void WriteRawData(const unsigned char*);
};
} // namespace Diskerror

#endif // DISKERROR_AUDIOFILE_H
