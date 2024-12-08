# JUCE BASED PHASE VOCODER
ECE484

Sam Miller

## Overview

This repository contains the source code necesarry to implement the 

## Downloading 

## Usage
This Phase Vocoder is designed to implement a variety of features. This plugin can operate as a real time processing plugin or as a pre-processed plugin. The different adjustable items are as follows:

### Pitch Shift
This is a slider that represents the pitch shift of the phase vocoder in %. A positive pitch shifts represents a positive change in pitch (e.g. increasing the pitch), and a negative pitch shifts represents a negative change in pitch (e.g. decreasing the pitch.
This can be represented using the following  equation

$P_{out} = Pitch_{in}\times(1+Pitch_{Shift})$

It's worth noting that this only actually modifies pitch when pitch is in Pitch Shift mode.
### Window Size
This is a selector that represents the size of the FFT window that will be taken to analyze the signal. Note that the size is set to powers of 2 to optomize efficiency. Window size should be at 
least $2\times hop size$ at all times but this is not explicitley limited in code.It's also worth noting that if $Window size>hop size \times 16$ There may be some performance issues

### Hop Size
This is a selector that represents the distance of hops between FFT windows that are taken to analyze the signal. Note that the size is set to powers of 2 to optomize efficiecny. Hop size should be
less than $\frac{window size}{2}$ at all times. It's also worth noting that if $Hop size<\frac{window size}{16}$ There may be some performance issues.

### Effect
This is a selector that allows you to change the effects. A description of each mode as well as the recommended window and hop sizes are below.:
|Parameter|Pitch Shift|Robotization|Whisperation|
|---------|-----------|------------|-----------|
|Description|Modifies the pitch by the amount input in Pitch Shift|modifies the phase of each fft bin to be zero which gives a robot type effect to the vocals|modifies the phase of each fft bin to be a random value between -pi to pi|
|Window Size (samples)| 1024-4096 | <=2048 | 64-256 |
|Hop Size (fraction of Window Size)| $<=\frac{1}{8}$|$<=\frac{1}{2}$| $<=\frac{1}{2}$|

A few notes can be made about each of these parameters
- For all cases if $Hop Size<\frac{1}{32} Window Size$ There will be significant issues in real time processing due to the number of FFTs that must be taken. If you need to lower the hop size or raise the window size by this much it's recommended to pre-apply the effect.
- For Pitch shift, if $Hop Size > \frac{1}{8}$ There will be notable distortion and artifacting from the pitch shift due to the inaccuracies of predicting changes of phase in the predictive algorithm outlined in class. The exact value for this might vary depending on the signal
- For Robotization and Whisperization if $Hop Size > \frac{1}{2}$ there will be distortion as the original signal cannot be perfectly reconstructed with this large of a hop size
- For Robotization changing the hop size wil vary the pitch.


## Signal Processing and Technical Implementation
This project uses a phase vocoder based on the principles outlined in Audio Effects Theory, Implementation and Application by Joshua D. Reiss Andrew P. McPherson and DAFX: Digital Audio Effects, Second Edition edited by Udo Zolzer.

To implement the phase vocoder the following steps were taken:
- Window each sample using the Hann/Hamming window function.
- Apply an in-place FFT to the window (can use out of place but the sample must be rotated in this case).
- Itterate through each frequency bin and perform operations to modify the frequency.
- Apply an in-place inverse FFT to the window.
- If pitch shifting resample the window to expand or compress the audio to change pitch.
- Move onto the next frame by moving by hop size.

Some more information on how these steps are implemented are below.

### Step One: Copy input buffer into circular buffer.

The juce framework supports real-time processing by taking input samples in audio buffers of a size determined by the environment the plugin is running in. However, the phase vocoder needs not only the current information, but also information of at least one window size into the future, and one window size into the past. In order to do this we make a circular input and output buffer of size $2\times s_{win max}+s_{buffer}$ Where $s_{win max}$ is the max window size and $s_{buffer}$ is the size of the buffer we are reading from.

Using this buffer we copy one buffer of data into the input buffer. Circular buffers are implemented in such a way as to overwrite old data with new data so this will remove the data from two windows ago that is no longer needed.

### Step Two: Hann Windowing

To window the functions the input data is broken into windows. For each buffer windows are taken by advancing the start of the windows by the hop size. If the end of the window exceeds the edge of the buffer then it stops window. For each window a Hanning window is applied to center the functions. This makes the transitions between windows much less jarring and limits audio artifacts from the vocoder. To window the function each sample is multiplied by the below function where Where n represents the current sample of the window and N is the sample side

$ 1-cos(\frac{2\pi n}{N}) $ 

### Step Three: Take the FFT
To transform the function into the frequency domain a fast fourier transform (FFT) is used. In our case the built-in juce FFT class was used to compute the transform. This allows us to process the signals in the frequency domain. It's worth noting that this a real only frequency transform was used since we are only interested in real signals in the time domain. The function used to use this is found in the juce::dsp::fft class and is called performRealOnlyForwardTransform()

### Step Four: Apply Frequency Modulation
To modulate the frequency a differnt Algorithm is applied for each of the effects.

For Pitch shifting the pitch is calculated to find what the phase should be based on the required pitch shift. To do this we itterate through each frequency bin and calculate what it's phase should be based on the pitch shift. To do this we calculate the unwrapped phase difference of the unmodified phase. divide this by the pitch shift, then add this to the previous phase for this bin. For more information on this see the doPitchShift funciton or read more in the McPherson textbook at https://www.taylorfrancis.com/books/mono/10.1201/b17593/audio-effects-joshua-reiss-andrew-mcpherson

For robotization the phase of each sample is simply set ot 0. This flattens the pitch which gives the vocals a robotic like effect.

For whisperization the phase of each sample is instead set to a random value between -pi and pi. This removes the vowel sounds from the vocals especially when the window size is low. 

### Step Five: Apply Inverse FFT
For this simply take the inverse FFT by invoking the same juce object used in the forward FFT.  The function used to use this is found in the juce::dsp::fft class and is called performRealOnlyInverseTransform()

### Step Six: Resample the Window to Apply Pitch Shift

To actually shift the pitch we need to resample the window to actually shift the pitch. To raise the signal pitch we need to shrink the window size and to the lower the pitch we need to increase the window size. To do this the window is compressed/expanded by the pitch shift. However, in most cases 




