//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_AUDIOFILE_H
#define DISKERROR_AUDIOFILE_H


#include <filesystem>
#include <fstream>
#include <vector>
#include <boost/cstdfloat.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/limits.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>

using namespace std;
using namespace boost;
using namespace boost::endian;

/**
 *	class AudioFile
 *	Open, read, and write existing audio files.
 *	The file size and type cannot be changed.
 */
class AudioFile 
{
	fstream   fileStm;

    // These properties are used as four character codes.
	big_uint32_t fileType      = 0x00000000; // RIFF, RF64, FORM, FORM
	big_uint32_t fileSubType   = 0x00000000; // WAVE, WAVE, AIFF, AIFC
	big_uint32_t dataEncoding  = 0x00000000; // 'PCM ' integer, 'IEEE' float
	big_uint32_t dataEndianess = 0x00000000; // 'big ' or 'litl'

    big_uint32_t dataType   = 0x00000000;
	uint16_t numChannels    = 0;
	uint32_t sampleRate	    = 0;
	uint16_t bitsPerSample  = 0;
	uint64_t numSamples     = 0;
	uint64_t dataBlockStart = 0;
	uint64_t dataBlockSize  = 0;

	void	openFile();

	void	openRIFF();	//	RIFF/WAVE < 4G
	void	openRF64();	//	RF64/WAVE > 4G
	void	openAIFF();	//	FORM/AIFF < 4G
	void	openAIFC();	//	FORM/AIFC   ?

//    void    ReverseCopyBytes( unsigned char * , unsigned char * , uint64_t );
    static void ReverseCopy4Bytes( unsigned char * , unsigned char * );

    void assertDataFormat();

public:
    // Constructor
    //  Requires a valid filesystem path object.
	AudioFile(const filesystem::path);

    // Factory methods
    //  Requires a valid file name.
	static AudioFile Make(string sPath) { return AudioFile(filesystem::path(sPath)); };
	static AudioFile Make(char* cPath)  { return AudioFile(filesystem::path(cPath)); };

    // Destructor
	~AudioFile() { };

    // Exposing these members because of their useful methods.
	const filesystem::path file;
	vector<float32_t>      samples;

	inline big_uint32_t	GetDataEncoding()  { return this->dataEncoding; };
	inline big_uint32_t	GetDataEndianess() { return this->dataEndianess; };

	inline uint16_t	GetNumChannels()   { return this->numChannels; };
	inline uint32_t	GetSampleRate()    { return this->sampleRate; };
	inline uint16_t	GetBitsPerSample() { return this->bitsPerSample; };
	inline uint64_t	GetNumSamples()    { return this->numSamples; };
	inline uint64_t	GetDataBlockSize() { return this->dataBlockSize; };

	unsigned char* ReadRawData();
	void           WriteRawData(unsigned char*);

	void ReadSamples();
	void WriteSamples();
	
	inline float& operator[] (uint64_t s) { return samples[s]; }
};

#endif // DISKERROR_AUDIOFILE_H
