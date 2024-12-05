/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ECE484PhaseVocoderAudioProcessor::ECE484PhaseVocoderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

ECE484PhaseVocoderAudioProcessor::~ECE484PhaseVocoderAudioProcessor()
{
}

//==============================================================================
const juce::String ECE484PhaseVocoderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ECE484PhaseVocoderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ECE484PhaseVocoderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ECE484PhaseVocoderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ECE484PhaseVocoderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ECE484PhaseVocoderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ECE484PhaseVocoderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ECE484PhaseVocoderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ECE484PhaseVocoderAudioProcessor::getProgramName (int index)
{
    return {};
}

void ECE484PhaseVocoderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ECE484PhaseVocoderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    //Initialize some information about the playback
    //Window Size
    //We set this to be an arbitrarily large number as the sampleRate can change
    //And we don't want to resize our buffers
    int s_IOBuf = 2*s_win_max+samplesPerBlock;
    // = 0.25 * sampleRate;

    inputBuffer.setSize(getTotalNumInputChannels(), s_IOBuf);
    inputBuffer.clear();

    outputBuffer.setSize(getTotalNumInputChannels(), s_IOBuf);
    outputBuffer.clear();

    //Set the size of the previous phase info to the max window size
    phaseBefore.setSize(getTotalNumInputChannels(), s_win_max);
    phaseBefore.clear();

    phaseAfter.setSize(getTotalNumInputChannels(), s_win_max);
    phaseAfter.clear();


    //Make the output write position a full sample ahead of the input write position
    //This ensures that we have lots of time to process the audio
    inWrite = 0;
    outWrite = 0;
    inRead = 0;

    outRead = inWrite-s_win_max;
    if (outRead < 0) {
        outRead += s_IOBuf;
    }

    //Output data is twice the size since it has real and comples
     

    

}

void ECE484PhaseVocoderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ECE484PhaseVocoderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


//Multiply the window by the hann window
void ECE484PhaseVocoderAudioProcessor::hannWindow(std::vector<float>& Vector, int size) {
    unsigned long i;
    double phase = 0, delta;

    delta = 2.0f * M_PI / (double)size;

    for (i = 0; i < size; i++)
    { 
        Vector[i] = Vector[i] * (float)(0.5 * (1.0 - cos(phase)));
        phase += delta;
    }

}

//Shift the whole buffer by the set amount
void ECE484PhaseVocoderAudioProcessor::circularShift(std::vector<float>& Vector, int size,unsigned int shift) {
    std::vector<float> copy = Vector;

    int shiftedIndex;
    for (int i = 0; i < size; i++) {
        //Apply the shift
        shiftedIndex = i + shift;
        //Take the modulo as this normalizes this back to the actual size range
        shiftedIndex %= size;
        Vector[i] = copy[shiftedIndex];
    }
}

//Convert real vector to vector of complex numbers
std::vector<std::complex<float>> realToComplex(std::vector<float> Real) {
    std::vector<std::complex<float>> Complex;
    Complex.resize(Real.size());

    for (int i = 0; i < Real.size(); i++) {
        Complex[i].real(Real[i]);

    }
    return Complex;

}

std::vector<float> complexToReal(std::vector<std::complex<float>> Complex) {

    std::vector<float> Real;
    Real.resize(Complex.size());

    for (int i = 0; i < Real.size(); i++) {
        Real[i] = abs(Complex[i]);

    }
    return Real;

}

void ECE484PhaseVocoderAudioProcessor::updateCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int writePosition, int channel) {

    int circBufferSize = circBuffer.getNumSamples();

    if (writePosition + numSamples < circBufferSize) {
        circBuffer.copyFrom(channel, writePosition, Data, numSamples);
    }
    else {
        int toEnd = circBufferSize - writePosition;
        int fromStart = numSamples - toEnd;
        circBuffer.copyFrom(channel, writePosition, Data, toEnd);
        circBuffer.copyFrom(channel, 0, Data+toEnd, fromStart);
    }

}

void ECE484PhaseVocoderAudioProcessor::addToCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int writePosition, int channel,float gain) {

    int circBufferSize = circBuffer.getNumSamples();

    if (writePosition + numSamples < circBufferSize) {
        circBuffer.addFrom(channel, writePosition, Data, numSamples,gain);
    }
    else {
        int toEnd = circBufferSize - writePosition;
        int fromStart = numSamples - toEnd;
        circBuffer.addFrom(channel, writePosition, Data, toEnd,gain);
        circBuffer.addFrom(channel, 0, Data + toEnd, fromStart,gain);
    }

}
void ECE484PhaseVocoderAudioProcessor::clearCircBuffer(juce::AudioBuffer<float>& circBuffer,int numSamples, int writePosition, int channel) {

    int circBufferSize = circBuffer.getNumSamples();

    if (writePosition + numSamples < circBufferSize) {
        circBuffer.clear(channel,writePosition,numSamples);
    }
    else {
        int toEnd = circBufferSize - writePosition;
        int fromStart = numSamples - toEnd;
        circBuffer.clear(channel, writePosition, toEnd);
        circBuffer.clear(channel, 0, fromStart);
    }

}

void ECE484PhaseVocoderAudioProcessor::updateFromCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int readPosition, int channel) {

    int circBufferSize = circBuffer.getNumSamples();
    auto* circBufferData = circBuffer.getWritePointer(channel);

    for (int i = 0; i < numSamples; i++) {
        Data[i] = circBufferData[readPosition];
        readPosition++;
        if (readPosition >= circBufferSize)
            readPosition = 0;

    }

}

void ECE484PhaseVocoderAudioProcessor::updateBufferIndex(int& index, int increment, unsigned int size) {
    index += increment;
    while (index > (int)size) {
        index -= size;
    }
    
    while (index < 0) {
        index += size;
    }
            
        
    

}
std::complex<float> ECE484PhaseVocoderAudioProcessor::doRobitization(std::complex<float> input) {

    float angle = arg(input);
    float magnitude = abs(input);


    //Set phase to zero
    angle = 0;


    ////Convert back to complex number
    return std::polar(magnitude, angle);

}
std::complex<float> ECE484PhaseVocoderAudioProcessor::doWhisperization(std::complex<float> input) {

    float angle = arg(input);
    float magnitude = abs(input);


    //take a random value from 0 to 1 
    float random = juce::Random::getSystemRandom().nextFloat();

    angle = 2*M_PI*random;

    ////Convert back to complex number
    return std::polar(magnitude, angle);

}
/* Princarg function only really used by do Pitch shift wraps function to -pi to pi*/
float princArg(float phase) {
    while (phase > M_PI)
        phase -= 2 * M_PI;

    while (phase < -M_PI)
        phase += 2 * M_PI;

    return phase;

}

std::complex<float> ECE484PhaseVocoderAudioProcessor::doPitchShift(std::complex<float> input, float pitchShift, float& phaseBefore, float& phaseAfter, int an_hop, int bin, int s_win){
    float unwrappedPhase = arg(input);
    float magnitude = abs(input);

    //Calculate the frequency in per samples
    float frequency = 2.0f * M_PI * (float)bin / (float)s_win;
    //Calculate omega in time
    float Omega = frequency * (float)an_hop;

    //Calculate the difference between the phases
    float dPhi = unwrappedPhase - phaseBefore;

    //Calculate the true phase by unwrapping the phase
    float delPhi = Omega + princArg(dPhi - Omega);

    //Set the lastPhase to this phase
    phaseBefore = unwrappedPhase;

    //Multiply the phase by the pitchshift to find the modified phase

    phaseAfter=princArg(phaseAfter+delPhi*pitchShift);

    //Now we just convert back to a complex number and return it
    return std::polar(magnitude, phaseAfter);
}
std::vector<float> ECE484PhaseVocoderAudioProcessor::compressWindow(std::vector<float>& originalWindow, float compress) {
    int s_win = originalWindow.size();
    int s_win_compress = floor(s_win / compress);

    std::vector<float> outputVector;

    outputVector.resize(s_win_compress);

    float in_i;
    int whole_i;
    float frac_i;


    
    for (int out_i = 0; out_i < s_win_compress;out_i++ ) {
        in_i = out_i * compress;
        whole_i = floor(in_i);
        frac_i = in_i - (float)whole_i;

        outputVector[out_i] = (1 - frac_i) * originalWindow[whole_i] + frac_i * originalWindow[(whole_i+1)%s_win];

    }
    return outputVector;
}


void ECE484PhaseVocoderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    
    
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    int numSamples = buffer.getNumSamples();
    int inputSamples = inputBuffer.getNumSamples();
    int outputSamples = outputBuffer.getNumSamples();



    
    //Get Current Plugin Settings
    Pluginsettings currentSettings = getPluginSettings(layout);

    //Set this factor so we scale based on the values
    int hop_syn = currentSettings.s_hop;
    int hop_an = hop_syn;
    if (currentSettings.effect == Shift) {
        hop_an = round(hop_syn /(1+ currentSettings.pitchShift));
    }
    float trueShift = (float)hop_syn / hop_an;

    
    float ftfactor = currentSettings.s_hop * 2.0 / currentSettings. s_win;

    //Initialize fft size (normally just window size, multiplied by 2 because the fft is real)
    const int s_fft{ 2 * currentSettings.s_win };

    //Initialize an FFT object
    juce::dsp::FFT forwardFFT{ (int)log2(currentSettings.s_win) };

    //Initialize the FFT processing vector
    std::vector<float> fftmagnitude;
    fftmagnitude.resize(s_fft);
    std::fill(fftmagnitude.begin(), fftmagnitude.end(), 0);


    //Modify the size of the last phase vector so that it has all the relavent information



    
    //Clear any unused channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    //For each sample we need to

    //1. Shift the input data down by one window size

    //2. Window the data starting from any windows that reach into this sample time.

    //3. Perform FFT and IFFT of existing window using window data and put this in FFT data

    //4. Perform any phase vocoder actions

    //5. Overlap and add 
    
    //6. Copy from the overlap and added data back to the audio buffer
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel){

        //Get a pointer with the new data
        auto* bufferData = buffer.getWritePointer(channel);
        auto* unmodifiedPhases = phaseBefore.getWritePointer(channel);
        auto* modifiedPhases = phaseAfter.getWritePointer(channel);



        //Use this to know how many samples have not yet been processed
        int unwindowedSamples = inWrite-inRead;
        //Use this to normalize the index since these are circular indexes which can cause inRead to be larger than inWrite

        updateBufferIndex(unwindowedSamples, 0, inputSamples);

        //Update input Buffer with new data and index
        updateCircBuffer(bufferData,numSamples,inputBuffer,inWrite,channel);    
        updateBufferIndex(inWrite, numSamples, inputSamples);

        
       //Now we itterate through window and perform fft on all complete windows we use the unwindowed samples variable
       //To ensure we start far enough back
        int samples;
        for (samples=0; samples - unwindowedSamples + currentSettings.s_win < numSamples;samples+=hop_an) {
            
            //Need to resize the FFT vector as it may have changed sizes when being resampled
            fftmagnitude.resize(s_fft);

           
            //Copy a window worth of data into the window data from the buffer and update the index
            updateFromCircBuffer(fftmagnitude.data(),currentSettings.s_win,inputBuffer,inRead, channel);          
            updateBufferIndex(inRead, hop_an, inputSamples);
            
                
            //apply the hann windowing
            hannWindow(fftmagnitude, currentSettings.s_win);


            //shift data to the left, this is to ensure phase is flat. This actually causes the inverse effect as the FFT algorithm is different from
            // The one in class
            //circularShift(fftmagnitude, currentSettings.s_win, currentSettings.s_win / 2);

            //Perform the forward FFT
            forwardFFT.performRealOnlyForwardTransform(fftmagnitude.data());

            //We need to initialize a vector to store previous phases for the pitch shifting


            //Now we go through each bin and do frequency processing
            for (int bin = 0; bin < currentSettings.s_win; bin++) {
                std::complex<float> fftComplex {fftmagnitude[2 * bin], fftmagnitude[2 * bin + 1]};



                switch (currentSettings.effect) {
                case Shift:
                    fftComplex = doPitchShift(fftComplex, trueShift, unmodifiedPhases[bin],modifiedPhases[bin], hop_an, bin, currentSettings.s_win);
                    break;

                case Robitization:
                    fftComplex=doRobitization(fftComplex);
                    break;
                
                case Whisperization:
                    fftComplex= doWhisperization(fftComplex);
                    break;
                }

               

                fftmagnitude[2 * bin] = fftComplex.real();
                fftmagnitude[2 * bin+1] = fftComplex.imag();


            }
            //Takes the inverse FFT
            forwardFFT.performRealOnlyInverseTransform(fftmagnitude.data());

            //If we're pitch shifting we need to resample the window
            int size = fftmagnitude.size();
            
            if (currentSettings.effect == Shift) {
                

                fftmagnitude= compressWindow(fftmagnitude, trueShift);
            }
            size = fftmagnitude.size();
            //shift data to the left, this is to ensure phase is flat. This actually causes the inverse effect as the FFT algorithm is different from
            // The one in class
            //circularShift(fftmagnitude, currentSettings.s_win, currentSettings.s_win / 2);

            //Copy window data back into the output buffer
            addToCircBuffer(fftmagnitude.data(), fftmagnitude.size(), outputBuffer, outWrite, channel, ftfactor);
            updateBufferIndex(outWrite, hop_an, outputSamples);
        }
        
        
       

        //Update the output buffer, note that this is placed well after the input buffer so that all processing can take place first
        updateFromCircBuffer(bufferData, numSamples, outputBuffer, outRead, channel);
        //Clear the parts of the buffer that have been read
        clearCircBuffer(outputBuffer, numSamples,outRead,channel);
        
        //Update the out read buffer index
        updateBufferIndex(outRead, numSamples, outputSamples);


        //If we're not on our last channel we want to reset the samples
        if (channel < totalNumInputChannels - 1) {
            updateBufferIndex(outRead, -numSamples, outputSamples);
            updateBufferIndex(outWrite, -samples, outputSamples);
            updateBufferIndex(inRead, -samples, inputSamples);
            updateBufferIndex(inWrite, -numSamples, inputSamples);
        }
    }

    //Update read and write positions after all is said and done


}

//==============================================================================
bool ECE484PhaseVocoderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ECE484PhaseVocoderAudioProcessor::createEditor()
{
    //return new ECE484PhaseVocoderAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ECE484PhaseVocoderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = layout.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ECE484PhaseVocoderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(layout.state.getType()))
            layout.replaceState(juce::ValueTree::fromXml(*xmlState));
}

Pluginsettings getPluginSettings(juce::AudioProcessorValueTreeState& layout) {

    Pluginsettings settings;
    settings.pitchShift = layout.getRawParameterValue("Pitch Shift in %")->load();
    settings.effect = layout.getRawParameterValue("Effect")->load();
    int hopIndex = layout.getRawParameterValue("Hop Size")->load();
    int winIndex = layout.getRawParameterValue("Window Size")->load();
    
    //Conversion of the hop sizes

    settings.s_hop = 2 << (hopIndex + 2);
    settings.s_win= 2 << (winIndex + 2);
    


    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout
ECE484PhaseVocoderAudioProcessor::createParamaterLayout() {

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        "Pitch Shift in %",
        "Pitch Shift in %",
        juce::NormalisableRange<float>(-1.f, 1.f, 0.001f),
        0.0f));

    juce::StringArray stringArray;
    for (int power = 2; power < 12; power++) {
        std::string stringToAdd=std::to_string((2 << power))+" samples";
        stringArray.add(stringToAdd);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("Hop Size", "Hop Size", stringArray,6));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Window Size", "Window Size", stringArray, 8));

    stringArray.clear();
    stringArray.add("Pitch Shift");
    stringArray.add("Robitization");
    stringArray.add("Whisperization");

    layout.add(std::make_unique<juce::AudioParameterChoice>("Effect", "Effect", stringArray, 0));
    return layout;

}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ECE484PhaseVocoderAudioProcessor();
}
