#include "Wav.h"

#include <iostream>
#include <memory.h>
#include <stdio.h>

WavFile::WavFile(std::string path, bool overwrite) : mFilePath{path}
{
    bool existingFile = std::filesystem::exists(path) && !overwrite;
    std::ios::openmode mask = std::ios::binary | std::ios::out;
    if(existingFile)
        mask |= std::ios::in;
    
    mFile.open(path, mask);
    if(!mFile)
        throw std::runtime_error("Could not open file");

    if(existingFile)
        loadHeader();
    else
        createHeader();
}

WavFile::~WavFile()
{
    if(mFile)
        mFile.close();
}

void WavFile::createHeader() {
    uint8_t *wavHeader = reinterpret_cast<uint8_t*>(&header);
    // Chunk ID
    wavHeader[0]  = 'R';
    wavHeader[1]  = 'I';
    wavHeader[2]  = 'F';
    wavHeader[3]  = 'F';
    // Chunk Size
    wavHeader[4] = 36; // 36 + Sub chunk 2 size
    wavHeader[5] = 0;
    wavHeader[6] = 0;
    wavHeader[7] = 0;
    // Format
    wavHeader[8]  = 'W';
    wavHeader[9]  = 'A';
    wavHeader[10] = 'V';
    wavHeader[11] = 'E';
    // Sub chunk 1 ID
    wavHeader[12] = 'f';
    wavHeader[13] = 'm';
    wavHeader[14] = 't';
    wavHeader[15] = ' ';
    //  Sub chunk 1 Size
    wavHeader[16] = 16;
    wavHeader[17] = 0;
    wavHeader[18] = 0;
    wavHeader[19] = 0;
    // Audio format PCM
    wavHeader[20] = 1;
    wavHeader[21] = 0;
    // Number of channels 1
    wavHeader[22] = 1;
    wavHeader[23] = 0;
    // Sample rate 9615Hz
    wavHeader[24] = 0x8F;
    wavHeader[25] = 0x25;
    wavHeader[26] = 0;
    wavHeader[27] = 0;
    // Byte rate = Sample rate * Num channels * Bytes per sample
    wavHeader[28] = 0x1E;
    wavHeader[29] = 0x4B;
    wavHeader[30] = 0;
    wavHeader[31] = 0;
    // Block align = Num channels * Bytes per sample
    wavHeader[32] = 2;
    wavHeader[33] = 0;
    // Bits per sample
    wavHeader[34] = 16;
    wavHeader[35] = 0;
    // Data sub chunk (Sub chunk 2 ID)
    wavHeader[36] = 'd';
    wavHeader[37] = 'a';
    wavHeader[38] = 't';
    wavHeader[39] = 'a';
    // Sub chunk 2 size
    wavHeader[40] = 0;
    wavHeader[41] = 0;
    wavHeader[42] = 0;
    wavHeader[43] = 0;

    mFile.write(reinterpret_cast<char*>(wavHeader), WAV_HEADER_SIZE);
}

void WavFile::loadHeader() {
    uint8_t buffer[WAV_HEADER_SIZE] = { 0 };
    mFile.read(reinterpret_cast<char*>(buffer), WAV_HEADER_SIZE);
    memcpy(&header, buffer, WAV_HEADER_SIZE);
}

void WavFile::printHeader() const {
    std::cout << "RIFF: 0x" << std::hex << header.chunkID << "\n";
    std::cout << "Chunk size: " << std::dec << header.chunkSize << "\n";
    std::cout << "Format: 0x" << std::hex << header.format << "\n";
    std::cout << "Sub chunk 1 ID: 0x" << std::hex << header.subChunk1ID << "\n";
    std::cout << "Sub chunk 1 size: " << std::dec << header.subChunk1Size << "\n";
    std::cout << "Audio format: " << std::dec << header.audioFormat << "\n";
    std::cout << "Number of channels: " << std::dec << header.numChannels << "\n";
    std::cout << "Sample rate: " << std::dec << header.sampleRate << "\n";
    std::cout << "Byte rate: " << std::dec << header.byteRate << "\n";
    std::cout << "Block align: " << std::dec << header.blockAlign << "\n";
    std::cout << "Bits per sample: " << std::dec << header.bitsPerSample << "\n";
    std::cout << "Sub chunk 2 ID: 0x" << std::hex << header.subChunk2ID << "\n";
    std::cout << "Sub chunk 2 size: " << std::dec << header.subChunk2Size << "\n";
}

void WavFile::updateSize(uint32_t size)
{
    header.chunkSize += size;
    mFile.seekp(4);
    mFile.write(reinterpret_cast<char*>(&header.chunkSize), 4);

    header.subChunk2Size += size;
    mFile.seekp(40);
    mFile.write(reinterpret_cast<char*>(&header.subChunk2Size), 4);

    mFile.seekp(0, std::ios::end);
}

void WavFile::writeData(uint8_t *data, uint32_t length)
{
    mFile.write(reinterpret_cast<char*>(data), length);
    updateSize(length);
}