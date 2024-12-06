# JUCE BASED PHASE VOCODER
ECE484

Sam Miller

## Overview

This repository contains the source code necesarry to implement the 

## Usage
This Phase Vocoder is designed to implement a variety of features. This plugin can operate as a real time processing plugin or as a pre-processed plugin. The different adjustable items are as follows:

### Pitch Shift
This is a slider that represents the pitch shift of the phase vocoder in %. A positive pitch shifts represents a positive change in pitch (e.g. increasing the pitch), and a negative pitch shifts represents a negative change in pitch (e.g. decreasing the pitch.
This can be represented using the following  equation

$P_{out} = Pitch_{in}\times(1+Pitch_{Shift})$

### Window Size
This is a selector that represents the size of the FFT window that will be taken to analyze the signal. Note that the size is set to powers of 2 to optomize efficiency. Window size should be at 
least $2\times hop size$ at all times but this is not explicitley limited in code.

### Hop Size
This is a selector that represents the distance of hops between FFT windows that are taken to analyze the signal. Note that the size is set to powers of 2 to optomize efficiecny. Hop size should be
less than $\frac{window size}{2}$ at all times.

## Background and Theory

## Implementation

