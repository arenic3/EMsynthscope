#include "Filter.h"

void LowPassFilter::setCutoff(float cutoff) {
	cutoff_ = cutoff;
}

void LowPassFilter::setThreshold(float threshold) {
	threshold_ = threshold;
}

float LowPassFilter::process(float input) {
	alpha = 1.0f / (1.0f + (sampleRate / (2.0f * M_PI * cutoff_)));
	
    float stage1Output = alpha * input + (1.0f - alpha) * previousOutput1;
    previousOutput1 = stage1Output;
    
	float output = alpha * stage1Output + (1.0f - alpha) * previousOutput2;
    previousOutput2 = output;

    if (fabs(output) > threshold_) {
        output = threshold_ * (output / fabs(output));
    }

    return output;
}