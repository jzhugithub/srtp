#include "featuretracker1.h"
#include "videoprocessor1.h"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>

using namespace std;
using namespace cv;

int main()
{
	VideoProcessor processor;
	FeatureTracker tracker;
	processor.setInput(0);
	processor.setFrameProcessor(&tracker);
	processor.displayOutput("Tracker Features");
	processor.setDelay(50);//ms
	processor.run();

	return 0;
};