#ifndef WAV_HPP
#define WAV_HPP

#define WAV_HEADER_SIZE 44

#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>

struct WavHeader {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
    uint32_t subChunk1ID;
    uint32_t subChunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t subChunk2ID;
    uint32_t subChunk2Size;
};


struct WavFile
{
    WavHeader header;

    WavFile(std::string path, bool overwrite = false);
    ~WavFile();
    void createHeader();
    void loadHeader();
    void printHeader() const;
    void writeData(uint8_t *data, uint32_t length);

private:
    void updateSize(uint32_t size);
    std::filesystem::path mFilePath;
    std::fstream mFile;
};
#endif // WAV_HPP