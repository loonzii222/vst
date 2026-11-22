#include "../Source/ReverseEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main()
{
    noirnuit::StereoClip input; input.sampleRate=44100;
    input.left.assign(44100*3,0); input.right.assign(44100*3,0);
    for(int i=0;i<44100;i++){
        const double env=std::sin(3.141592653589793*double(i)/44100);
        input.left[44100+i]=float(0.4*env*std::sin(2*3.141592653589793*240*i/44100));
        input.right[44100+i]=float(0.37*env*std::sin(2*3.141592653589793*244*i/44100));
    }
    noirnuit::Settings p; p.durationSeconds=4.0;
    auto out=noirnuit::createReverseReverb(input,p);
    assert(out.valid()); assert(out.length()==4u*44100u);
    float peak=0; for(auto v:out.left){assert(std::isfinite(v));peak=std::max(peak,std::abs(v));}
    assert(peak>0.005f && peak<1.0f);
    p.phone=true;p.chorus=true;p.drive=true;p.tremolo=true;p.vocalMode=false;
    auto fx=noirnuit::createReverseReverb(input,p);
    assert(fx.valid());float diff=0;for(size_t i=0;i<out.length();i++)diff+=std::abs(fx.left[i]-out.left[i]);
    assert(diff>1.0f);
    noirnuit::StereoClip silent; silent.sampleRate=44100; silent.left.assign(500,0);silent.right.assign(500,0);
    auto s=noirnuit::createReverseReverb(silent,p);
    for(auto v:s.left)assert(v==0.0f);
    std::cout<<"DSP tests OK | peak="<<peak<<" | FX difference="<<diff<<"\n";
}
