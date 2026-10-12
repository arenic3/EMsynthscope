#ifndef FILTER_H
#define FILTER_H
#include <cmath>


class Filter {
protected:
	float sampleRate;
	float alpha;
	float previousOutput1;
    float previousOutput2;
    float cutoff_;
    float threshold_;
    
public:
	Filter();
    Filter(float rate)
        : sampleRate(rate), alpha(0.0f), previousOutput1(0.0f), previousOutput2(0.0f),
          cutoff_(20.0f), threshold_(1.0f) {}
    	
    virtual ~Filter() {}
    
    virtual float process(float input) = 0;
    virtual void setCutoff(float cutoff) = 0;
    virtual void setThreshold(float threshold) = 0;
    
};

class LowPassFilter : public Filter {
public:
	LowPassFilter(float rate) : Filter(rate) {}
	float process(float input) override;
	virtual void setCutoff(float cutoff) override;
    virtual void setThreshold(float threshold) override;
};

#endif