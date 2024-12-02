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


    inputBuffer.setSize(getTotalNumInputChannels(), 3 * samplesPerBlock);
    inputBuffer.clear();

    //Set the size of the window buffer, we set it as twice the samples per block to make sure it can store 2 blocks of data
    outputBuffer.setSize(getTotalNumInputChannels(), 3*samplesPerBlock);
    outputBuffer.clear();

    //Output data is twice the size since it has real and comples
    fftComplex.resize(s_fft);
    std::fill(fftComplex.begin(), fftComplex.end(), 0);

    

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

//Note the array must contain indexes between bufferstart and bufferend
void ECE484PhaseVocoderAudioProcessor::copyBuffertoVector(juce::AudioBuffer<float>& buffer, int bufStart, std::vector<float> Vector, int vecStart, int numSamples, int channel) {
    auto* bufferData = buffer.getReadPointer(channel);
    int bufferSize = buffer.getNumSamples();

    for (int i = 0; i < numSamples; i++) {
        Vector[vecStart+i]=bufferData[bufStart + i];
    }

}

void ECE484PhaseVocoderAudioProcessor::hannWindow(std::vector<float>& Vector, int size) {
    for (int i = 0; i < size; i++) {
        //multiply each index by it's sinusoidal equivilaent
        Vector[i] = Vector[i] * 0.5 * (1 - cos(i * 2 * 3.1415) / size);

    }

}

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

//Update the buffer by filling it with the full size of the vector
void ECE484PhaseVocoderAudioProcessor::updateCircBuffer(float* input, int inputStart, int inputEnd, juce::AudioBuffer<float>& circBuffer, int& writePosition, int channel) {

    //i is only used to itterate the vector write posisiton is used for the buffer
    auto* circData = circBuffer.getWritePointer(channel);
    int s_circBuffer = circBuffer.getNumSamples();
    //The way we have this set up the first half of the data is overlapping with past data, the second half of the data
    //Shouldn't overlap anything and will be overlapped by future data. As such they should be treated differently


    for (int i = inputStart; i <inputEnd ; i++) {
        //Update the write posisiton
        writePosition++;
        if (writePosition >= s_circBuffer) {
            writePosition = 0;
        }
        //Add the vector to what's already in that write position

        circData[writePosition] = input[i];


    }
   

}

void ECE484PhaseVocoderAudioProcessor::addCircBuffer(float* input, int inputStart, int inputEnd, juce::AudioBuffer<float>& circBuffer, int& writePosition, int channel) {

    //i is only used to itterate the vector write posisiton is used for the buffer
    auto* circData = circBuffer.getWritePointer(channel);
    int s_circBuffer = circBuffer.getNumSamples();
    //The way we have this set up the first half of the data is overlapping with past data, the second half of the data
    //Shouldn't overlap anything and will be overlapped by future data. As such they should be treated differently


    for (int i = inputStart; i < inputEnd; i++) {
        //Update the write posisiton
        writePosition++;
        if (writePosition >= s_circBuffer) {
            writePosition = 0;
        }
        //Add the vector to what's already in that write position

        circData[writePosition] += input[i];


    }


}
//Update a set number of samples from the circular buffer. Note we update half a buffer back in time because we need to make sure all the phase vocoder data is there
void ECE484PhaseVocoderAudioProcessor::updateOutputBuffer(std::vector<float> Window, int windowStart, juce::AudioBuffer<float>& outputBuffer, int outputStart,  int numSamples, int channel) {
    
    auto* outputData = outputBuffer.getWritePointer(channel);
    int s_output_size = outputBuffer.getNumSamples();
    

    for (int i = 0; i < numSamples; i++) {
        outputData[i + outputStart] = Window[i + windowStart]+ outputData[i + outputStart];
    }

}

void ECE484PhaseVocoderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    //For each sample we need to

    //1. Shift the input data down by one audio buffer size

    //2. Window the data starting from any windows that reach into this sample time.

    //3. Perform FFT and IFFT of existing window using window data and put this in FFT data

    //4. Perform any phase vocoder actions

    //Overlap and add 
    
     //Itterate through each window 
    for (int channel = 0; channel < totalNumInputChannels; ++channel){
        auto* bufferData = buffer.getWritePointer(channel);
        int bufferSize = buffer.getNumSamples();

        //Shift the data down by one buffer size, contains a total of 3 buffers, the center buffer is the one we operate on
        juce::AudioBuffer<float> copy = inputBuffer;
        inputBuffer.copyFrom(channel, 0, copy, channel, bufferSize, 2 * bufferSize);

        //Copy the inputBuffer in from the input data
        inputBuffer.copyFrom(channel, 2 * bufferSize, buffer, channel, 0, bufferSize);
        
        //we want to start where the window doesn't reach into the middle of the three windows at all
        int window;
        for (window = 0; window * s_hop + s_win < bufferSize; window++);

        //Now we itterate through window
        for (window; window * s_hop < 2 * bufferSize; window++) {

            copyBuffertoVector(inputBuffer,window*s_hop,fftComplex,0,s_win,channel);
            //Copy a window worth of data into the window data from the buffer
                
            //apply the hann windowing
            //hannWindow(fftComplex, s_win);

            //shift data to the left, this is to ensure phase is flat
            //circularShift(fftComplex, s_win, s_win / 2);


            //Now we perform the FFT on the data stored in same location
           // forwardFFT.performFrequencyOnlyForwardTransform(fftComplex.data());

            //Now we go through each bin and do frequency processing
            for (int bin = 0; bin < s_win; bin++) {
                //Put the 2 values into a complex constructor so it's easier to modify
                //td::complex<float> complexData (fftComplex[2*bin], fftComplex[2*bin+1]);

                //Find the phase and magnitude
                //float angle = arg(complexData);
                //float magnitude = abs(complexData);


                //Do processing right now this is robotization

                /* ------------------------TODO Processing--------------------------*/
                //angle = 0;

                /*------------------------TODO Processing--------------------------*/

                //Convert back to complex number
                //complexData = std::polar(magnitude, angle);

                //Store back into the original vector
                //fftComplex[2 * bin] = complexData.real();
                //fftComplex[2 * bin + 1] = complexData.imag();

            }
                //Takes the inverse transform
                //forwardFFT.performRealOnlyInverseTransform(fftComplex.data());

                //Inverse shift the data so it's centered again
                //circularShift(fftComplex, s_win, s_win / 2);

                //Update the Circular buffer with 
                //Copy window data back into the output buffer
                

                updateOutputBuffer(fftComplex,0,outputBuffer,window*s_hop,s_win,channel);
            
            //This condition is for if the window goes out of the sample range
            //else{
            //    winWritten = (s_hop * window + s_win)- numSamples;

            //    copyAudiotoVector(bufferData, fftComplex, 0, winWritten);

            //    //If not just update the normal amount from the output buffer
            //    if (s_hop * window + s_hop <= numSamples) {
            //        updateOutputBuffer(buffer, window * s_hop, s_hop - outWritten, channel);
            //        outWritten = 0;

            //    }
            //    else {
            //        outWritten = (s_hop * window + s_hop) - numSamples;
            //        updateOutputBuffer(buffer, window * s_hop, outWritten, channel);
            //    }

            //    

            //}
            
        }

        buffer.copyFrom(channel, 0, outputBuffer, channel, bufferSize, bufferSize);
        
    }
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
    settings.pitchShift = layout.getRawParameterValue("Pitch Shift in Semitones")->load();
    settings.effect = layout.getRawParameterValue("Effect")->load();


    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout
ECE484PhaseVocoderAudioProcessor::createParamaterLayout() {

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        "Pitch Shift in Semitones",
        "Pitch Shift in Semitones",
        juce::NormalisableRange<float>(-5.f,5.f,1.f),
        0.0f));


    juce::StringArray stringArray;
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
