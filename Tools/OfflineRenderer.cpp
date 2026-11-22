// Standalone WAV renderer for testing the same real DSP engine without a DAW or JUCE.
// Supports uncompressed mono/stereo PCM16 or float32 WAV. Produces stereo PCM16 WAV.
#include "../Source/ReverseEngine.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

namespace
{
uint16_t r16(std::istream& i) { unsigned char b[2]{}; i.read(reinterpret_cast<char*>(b), 2); return uint16_t(b[0] | (b[1] << 8)); }
uint32_t r32(std::istream& i) { auto a=r16(i); auto b=r16(i); return uint32_t(a | (uint32_t(b) << 16)); }
void w16(std::ostream& o, uint16_t v) { const char b[]={char(v & 255),char((v>>8)&255)}; o.write(b,2); }
void w32(std::ostream& o, uint32_t v) { w16(o,uint16_t(v)); w16(o,uint16_t(v>>16)); }
noirnuit::StereoClip readWav(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Fichier introuvable: " + path);
    char header[4]{}; f.read(header,4); if (std::strncmp(header,"RIFF",4)) throw std::runtime_error("WAV RIFF requis");
    (void) r32(f); f.read(header,4); if (std::strncmp(header,"WAVE",4)) throw std::runtime_error("WAV invalide");
    uint16_t codec=0, channels=0, bits=0; uint32_t sr=0; std::vector<unsigned char> bytes;
    while (f)
    {
        if (!f.read(header,4)) break;
        uint32_t sz=r32(f); const auto chunkStart=f.tellg();
        if (!std::strncmp(header,"fmt ",4))
        {
            codec=r16(f); channels=r16(f); sr=r32(f); (void)r32(f); (void)r16(f); bits=r16(f);
        }
        if (!std::strncmp(header,"data",4))
        { bytes.resize(sz); f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(sz)); }
        f.clear(); f.seekg(chunkStart + std::streamoff(sz + (sz&1u)));
    }
    if (channels < 1 || channels > 2 || sr<8000 || sr>384000 || !((codec==1 && bits==16)||(codec==3 && bits==32)))
        throw std::runtime_error("WAV attendu: PCM16 ou Float32, mono/stereo, 8-384kHz");
    const auto bytesPerSample=bits/8;
    const auto frames=bytes.size()/(channels*bytesPerSample);
    noirnuit::StereoClip clip; clip.sampleRate=static_cast<int>(sr);
    clip.left.resize(frames); clip.right.resize(frames);
    for (size_t i=0; i<frames; ++i)
    {
        auto parse=[&](int ch){
            const size_t p=(i*channels+static_cast<size_t>(ch))*bytesPerSample;
            if (codec==1) { const int16_t s=static_cast<int16_t>(uint16_t(bytes[p] | (uint16_t(bytes[p+1])<<8))); return float(s)/32768.0f; }
            uint32_t raw=uint32_t(bytes[p]) | (uint32_t(bytes[p+1])<<8) | (uint32_t(bytes[p+2])<<16) | (uint32_t(bytes[p+3])<<24);
            float fl=0; std::memcpy(&fl,&raw,sizeof(float)); return std::isfinite(fl)?fl:0.0f;
        };
        clip.left[i]=parse(0); clip.right[i]=parse(channels==2?1:0);
    }
    return clip;
}
void writeWav(const std::string& path, const noirnuit::StereoClip& clip)
{
    std::ofstream f(path, std::ios::binary); if (!f) throw std::runtime_error("Impossible de creer le fichier");
    const auto n=static_cast<uint32_t>(clip.length());
    const uint32_t dataSize=n*4;
    f.write("RIFF",4); w32(f,36+dataSize); f.write("WAVEfmt ",8); w32(f,16);
    w16(f,1); w16(f,2); w32(f,static_cast<uint32_t>(clip.sampleRate)); w32(f,clip.sampleRate*4u); w16(f,4); w16(f,16);
    f.write("data",4); w32(f,dataSize);
    for (size_t k=0;k<clip.length();++k)
    {
        auto to16=[](float v){return static_cast<int16_t>(std::round(std::max(-1.0f,std::min(1.0f,v))*32767.0f));};
        w16(f,static_cast<uint16_t>(to16(clip.left[k]))); w16(f,static_cast<uint16_t>(to16(clip.right[k])));
    }
}
}
int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "NOIRNUIT REVERSE / offline renderer\nUsage: noirnuit-render source.wav reverse.wav [BPM=120] [bars=4] [preset=Vocal]\nPresets: Vocal, Dream, Dark, Glitch\n";
        return 0;
    }
    try
    {
        auto source=readWav(argv[1]);
        noirnuit::Settings p;
        if(argc>3) p.bpm=std::stod(argv[3]);
        const double bars= argc>4 ? std::stod(argv[4]) : 4.0;
        p.durationSeconds=bars * 4.0 * 60.0 / std::max(30.0,std::min(300.0,p.bpm));
        const std::string preset=argc>5?argv[5]:"Vocal";
        if(preset=="Dream") {p.chorus=true;p.decay=0.95f;p.width=1.45f;}
        if(preset=="Dark") {p.phone=true;p.tone=0.28f;p.width=0.8f;}
        if(preset=="Glitch") {p.drive=true;p.tremolo=true;p.decay=0.56f;}
        auto out=noirnuit::createReverseReverb(source,p);
        writeWav(argv[2],out);
        std::cout << "OK : " << argv[2] << " (" << out.length() << " echantillons, " << out.sampleRate << " Hz)\n";
    }
    catch (const std::exception& e) {std::cerr << "Erreur: " << e.what() << '\n'; return 1;}
    return 0;
}
