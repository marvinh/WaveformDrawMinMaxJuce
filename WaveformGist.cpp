//Width Of this component must be a divider of the sample rate for exmaple 44100Hz the width of this component must be at least 441 pixels
//upsample the data you want to render by at least 2x
//use min max for large views samples >= width*4 else juct interpolate

class SubWaveViewComponent : public juce::Component {
    
public:
    
    SubWaveViewComponent(const VisibleRangeDataModel & vr):visibleRange(vr) {
        
        
    }
    ~SubWaveViewComponent() {
       
    }
    
    
    void resized() override {
        
        osBuffer.setSize(1, getWidth()*4.0);
    }
    
    int gcd(int a, int b){
        if (a == 0){
            return b;
        }else{
            return gcd(b % a, a);
        }
    }
      
      
    std::tuple<float,float,bool> minmaxorder(float *p, int length){
        float mi = std::numeric_limits<float>::max();
        float ma = -std::numeric_limits<float>::max();
        int ia = 0;
        int ii = 0;
        for(int i =0; i < length; i++){
            if(p[i] <= mi)
            {
                mi = p[i];
                ii = i;
            }
            
            if(p[i] >= ma)
            {
                ma = p[i];
                ia = i;
            }
        }
        
        return std::tuple<float,float,bool> (mi,ma, ii < ia);
    }

    template<typename T>
        inline T cubicHermiteSpline(const T* buffer, T readHead, int size) noexcept
        {
            const auto iFloor = std::floor(readHead);
            auto i1 = static_cast<int>(iFloor);
            auto i0 = i1 - 1;
            auto i2 = i1 + 1;
            auto i3 = i1 + 2;
            if (i3 >= size) i3 -= size;
            if (i2 >= size) i2 -= size;
            if (i0 < 0) i0 += size;

            const auto t = readHead - iFloor;
            const auto v0 = buffer[i0];
            const auto v1 = buffer[i1];
            const auto v2 = buffer[i2];
            const auto v3 = buffer[i3];

            const auto c0 = v1;
            const auto c1 = static_cast<T>(.5) * (v2 - v0);
            const auto c2 = v0 - static_cast<T>(2.5) * v1 + static_cast<T>(2.) * v2 - static_cast<T>(.5) * v3;
            const auto c3 = static_cast<T>(1.5) * (v1 - v2) + static_cast<T>(.5) * (v3 - v0);

            return ((c3 * t + c2) * t + c1) * t + c0;
        }
    
    void paint(juce::Graphics &g) override {
        
        
        
        juce::Path path;
        
        juce::Path p2;
        
        auto start = visibleRange.getVisibleRange().getStart();
        auto length = visibleRange.getVisibleRange().getLength();
        auto end = visibleRange.getVisibleRange().getEnd();
        
        
        int vstart = rd.getNumSamples()*start;
        int vlen = rd.getNumSamples()*length;
        int vend = rd.getNumSamples()*end;
        int inc = length*16;
       
        
        double ratio =  (44100.0*length) / getWidth();
        
        
        float sourcePos = 0.0f;
        
        
        
        if(ratio > 0 && ratio <= 2.0) {
            while(sourcePos < vlen-1){
                auto pos = (int) sourcePos;
                auto alpha = (float) sourcePos - pos;
                auto invAlpha = 1.0f - alpha;
                float s = cubicHermiteSpline(rd.getChannelPointer(0), sourcePos+vstart, rd.getNumSamples());//rd.getSample(0, pos+vstart) * invAlpha + rd.getSample(0, pos + vstart + 1) * alpha;
                float m = juce::jmap(-s,-1.0f,1.0f,0.0f, (float) getHeight());
                if(sourcePos == 0){
                    path.startNewSubPath(sourcePos/vlen*getWidth(), m);
                }else{
                    path.lineTo(sourcePos/vlen*getWidth(),m);
                }
                
                sourcePos += ratio;
            }
        }else{
            
            float sourcePos = 0;
            int k = 0;
            ratio/=2.0;
            while(sourcePos < vlen-ratio){
                auto pos = (int) sourcePos;
                
                float *list = rd.getSubBlock(pos+vstart,(int)ratio).getChannelPointer(0);
                
                std::tuple<float,float,bool>  fmi = minmaxorder(list, (int)ratio);
                float mi = -std::get<0>(fmi);
                float ma = -std::get<1>(fmi);
                
                mi = juce::jmap(mi,-1.0f,1.0f,0.0f,1.0f);
                ma = juce::jmap(ma,-1.0f,1.0f,0.0f,1.0f);
                
                mi = juce::jmap(mi,0.0f,1.0f,0.0f,(float)getHeight());
                ma = juce::jmap(ma,0.0f,1.0f,0.0f,(float)getHeight());
                
                float md = (mi+ma)/2.0;
                
                bool isMinFirst = std::get<2>(fmi);
                
                float mt = sourcePos+ratio/2.0;
                float st = sourcePos+ratio;
                
                if(sourcePos == 0.0) {
                    if(isMinFirst){
                        path.startNewSubPath(sourcePos/vlen*getWidth(), mi);
                        path.lineTo(sourcePos/vlen*getWidth(), md);
                        path.lineTo(st/vlen*getWidth(), ma);
                    }else{
                        path.startNewSubPath(sourcePos/vlen*getWidth(), ma);
                        path.lineTo(mt/vlen*getWidth(), md);
                        path.lineTo(st/vlen*getWidth(), mi);
                    }
                }else{
                    if(isMinFirst){
                        path.lineTo(sourcePos/vlen*getWidth(), mi);
                        path.lineTo(mt/vlen*getWidth(), md);
                        path.lineTo(st/vlen*getWidth(), ma);
                    }else{
                        path.lineTo(sourcePos/vlen*getWidth(), ma);
                        path.lineTo(mt/vlen*getWidth(), md);
                        path.lineTo(st/vlen*getWidth(), mi);
                    }
                }
                k++;
                
                
                
                sourcePos += ratio;
            }
                    
            
                
            
        }
        
        g.setColour(juce::Colour(209,209,209));
        g.strokePath(path,juce::PathStrokeType(1.0f,juce::PathStrokeType::JointStyle::curved));
        
        
    }
    
    juce::dsp::AudioBlock<float> rd;
    VisibleRangeDataModel visibleRange;
    juce::AudioBuffer<float> osBuffer;
    
    float minBuffer[4096];
    float maxBuffer[4096];
    float residue[4096];
    
    
};
