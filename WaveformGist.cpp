class SubWaveViewComponent : public juce::Component {
    
public:
    
    SubWaveViewComponent(const VisibleRangeDataModel& vr):visibleRange(vr) {
        
        
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
    
    void paint(juce::Graphics &g) override {
        
        
        
        juce::Path path;
        
        juce::Path p2;
        
        int sampleLength = rd.getNumSamples();
        float flength = visibleRange.getVisibleRange().getLength();
        int vstart = visibleRange.getVisibleRange().getStart()*sampleLength;
        int vlen = visibleRange.getVisibleRange().getLength()*sampleLength;
        int vend = visibleRange.getVisibleRange().getEnd()*sampleLength;
        float ratio =  (44100.0*flength) / getWidth();
        
        float sourcePos = 0.0f;
        
        int k = 0;
        
        if(ratio > 0 && ratio <= 2.0) {
            while(sourcePos < vlen-1){
                auto pos = (int) sourcePos;
                auto alpha = (float) sourcePos - pos;
                auto invAlpha = 1.0f - alpha;
                float s = rd.getSample(0, pos+vstart) * invAlpha + rd.getSample(0, pos + vstart + 1) * alpha;
                float m = juce::jmap(-s,-1.0f,1.0f,0.0f, (float) getHeight());
                if(sourcePos == 0){
                    path.startNewSubPath(sourcePos/vlen*getWidth(), m);
                }else{
                    path.lineTo(sourcePos/vlen*getWidth(),m);
                }
                
                sourcePos += ratio;
                k++;
            }
        }else{
            
            float sourcePos = 0;
            
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

        
        g.setColour(juce::Colour(90,150,210));
        g.strokePath(path,juce::PathStrokeType(1.0f+M_PI/8,juce::PathStrokeType::JointStyle::curved));
        
        
    }
    
    juce::dsp::AudioBlock<float> rd;
    VisibleRangeDataModel visibleRange;
    juce::AudioBuffer<float> osBuffer;
    
    float minBuffer[4096];
    float maxBuffer[4096];
    float residue[4096];
};


class SubViewComponent : public juce::Component , public DataModel::Listener,
public juce::AudioProcessorValueTreeState::Listener {
public:
    SubViewComponent(DataModel & dm, juce::AudioProcessorValueTreeState &s):
    dataModel(dm),
    state(s),
    levelMseg(dataModel.getMainMSEGModel()),
    wave(dataModel.getSubViewVisibleRange()),
    ruler(dataModel.getSubViewVisibleRange())
    {
        
        
        addAndMakeVisible(ruler);
        addAndMakeVisible(wave);
        addAndMakeVisible(levelMseg);
        renderSynth.setCurrentPlaybackSampleRate(44100.0);
        renderSynth.addVoice(new SubVoice());
        renderSynth.addSound(new SubSound());
        os2x = std::make_unique<juce::dsp::Oversampling<float>>(2);
        os2x->addOversamplingStage(juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, 0.05, -90.0, 0.05, -90.0);
        os2x->initProcessing(44100.0*4);
        
        renderData.setSize(2, 4*44100.0);
        
        renderData.clear();
        
        dataModel.addListener(*this);
        
        
        state.addParameterListener(Params::kSubLevel, this);
        state.addParameterListener(Params::kSubPitchHi, this);
        state.addParameterListener(Params::kSubPitchLow, this);
        state.addParameterListener(Params::kSubPitchDecay, this);
        state.addParameterListener(Params::kSubPitchVelocity, this);
    }
    
    ~SubViewComponent() {
        
    }
    
    void parameterChanged(const juce::String &parameterID, float newValue) override {

        if(auto * subsound = static_cast<SubSound*>(renderSynth.getSound(0).get())) {
            subsound->m = 1;
            if(parameterID == Params::kSubLevel) {
                subsound->setSubLevel(newValue);
            }else if (parameterID == Params::kSubPitchHi) {
                subsound->setSubPitchHi(newValue);
            }else if (parameterID == Params::kSubPitchLow) {
                subsound->setSubPitchLow(newValue);
            }else if (parameterID == Params::kSubPitchDecay) {
                subsound->setSubPitchDecay(newValue);
            }else if (parameterID == Params::kSubPitchVelocity) {
                subsound->setSubPitchVelocity(newValue);
            }
        }
        juce::MessageManager::callAsync([this] {
            this->refresh();
        });
    }
    
    void refreshSub() {
        
        renderData.clear();
        
        juce::MidiMessage n(juce::MidiMessage::noteOn(1, 48, 1.0f));
        juce::MidiBuffer mb(n);
        
        mb.addEvent(juce::MidiMessage::noteOff(1, 48, 1.0f), renderData.getNumSamples()/4-1);
        mb.addEvent(juce::MidiMessage::noteOn(1, 48, 1.0f), renderData.getNumSamples()/4);
        mb.addEvent(juce::MidiMessage::noteOff(1, 48, 1.0f), renderData.getNumSamples()/4*2-1);
        mb.addEvent(juce::MidiMessage::noteOn(1, 48, 1.0f), renderData.getNumSamples()/4*2);
        mb.addEvent(juce::MidiMessage::noteOff(1, 48, 1.0f), renderData.getNumSamples()/4*3-1);
        
        auto sampleLength = renderData.getNumSamples();
        
//        auto rng = dataModel.getSubViewVisibleRange();
//        auto fstart = rng.getVisibleRange().getStart();
//        auto flength = rng.getVisibleRange().getLength();
//        auto fend = rng.getVisibleRange().getEnd();
//
        renderSynth.renderNextBlock(renderData, mb,0, sampleLength);
        
    }
    
    
    void refresh() {
        refreshSub();
        juce::dsp::AudioBlock<float> rd (renderData);
        wave.rd = rd;
        wave.repaint();
    }
    void setMainMSEGChanged(std::shared_ptr<mseg_vec> m) override {
        if(auto * subsound = static_cast<SubSound*>(renderSynth.getSound(0).get())) {
            subsound->mainMSEGToUse = m;
        }
        
        juce::MessageManager::callAsync([this] {
            this->refresh();
        });
    }
    
    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colour(39,41,44));
    }
    
    
    void resized() override {
    
        auto b = getLocalBounds().reduced(proportionOfHeight(0.1f));
        ruler.setBounds(b.removeFromTop(proportionOfHeight(0.1f)).withWidth(882));
        levelMseg.setBounds(b.withWidth(882));
        wave.setBounds(b.withWidth(882));
        wave.toBack();
        refresh();
    }
    
    juce::Rectangle<int> viewArea;
    DataModel dataModel;
    juce::AudioProcessorValueTreeState & state;
    
    MSEGComponent levelMseg;
    SubWaveViewComponent wave;
    Ruler ruler;
    juce::Synthesiser renderSynth;
    juce::AudioBuffer<float> renderData;
    std::unique_ptr<juce::dsp::Oversampling<float>> os2x;


    
};
