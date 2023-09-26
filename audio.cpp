#include "ps3eye.hpp"
#include "ps3mic.h"

#define CAM_SX 640
#define CAM_SY 480
#define CAM_FPS 60 // Frames per second. At 320x240 this can go as high as 187 fps.

#define CAM_GRAYSCALE false // If true the camera will output a grayscale image instead of rgb color.

#define DO_SUM true // Calculates and draws the sum of all four microphones. The effect of this is that the summed channel amplifies sounds coming from the front, while reducing ambient noise and sounds coming from the sides. Try it out by making hissing sounds from different directions towards the camera!

#define NUM_CHANNELS (DO_SUM ? 5 : 4) // The number of channels to record. Normally this would be four, but when DO_SUM is enabled, this becomes five.

#define AUDIO_HISTORY_LENGTH .5f // Length of the audio history buffer in seconds.

using namespace ps3eye;

static std::string errorString;

static uint8_t imageData[CAM_SX * CAM_SY * 4];

struct MyAudioCallback : AudioCallback
{
	std::vector<float> histories[NUM_CHANNELS];
	int maxHistoryFrames = 0;
	int numHistoryFrames = 0;
	int firstHistoryFrame = 0;
	
	MyAudioCallback(const float seconds)
	{
		maxHistoryFrames = seconds * PS3EYEMic::kSampleRate;
		
		for (int i = 0; i < NUM_CHANNELS; ++i)
		{
			histories[i].resize(maxHistoryFrames, 0.f);
		}
	}
	
	virtual void handleAudioData(const AudioFrame * frames, const int numFrames) override
	{
		//printf("received %d frames!\n", numFrames);
		
		for (int i = 0; i < numFrames; ++i)
		{
			for (int c = 0; c < 4; ++c)
			{
				auto & history = histories[c];
				
				const float value = frames[i].channel[c] / float(1 << 15);
				
				history[firstHistoryFrame] = value;
			}
			
		#if DO_SUM
			histories[4][firstHistoryFrame] =
				(
					histories[0][firstHistoryFrame] +
					histories[1][firstHistoryFrame] +
					histories[2][firstHistoryFrame] +
					histories[3][firstHistoryFrame]
				) / 4.f;
		#endif
			
			if (numHistoryFrames < maxHistoryFrames)
				numHistoryFrames++;
			
			firstHistoryFrame++;
			
			if (firstHistoryFrame == maxHistoryFrames)
				firstHistoryFrame = 0;
		}
	}
};

static void drawAudioHistory(const MyAudioCallback & audioCallback)
{
	for (int c = 0; c < NUM_CHANNELS; ++c)
	{
		auto & history = audioCallback.histories[c];

		for (int i = audioCallback.firstHistoryFrame, n = 0; n + 1 < audioCallback.maxHistoryFrames; ++n)
		{
			const int index1 = i;
			const int index2 = i + 1 == audioCallback.maxHistoryFrames ? 0 : i + 1;

			const float value1 = history[index1];
			const float value2 = history[index2];
			const float strokeSize = .4f;

			printf("%f, %f \n", value1, value2);
			//

			i = index2;
		}
	}
}

int main(int argc, char * argv[])
{
	auto devices = ps3eye::list_devices();

	if (devices.empty())
	{
		printf("no PS3 eye camera found\n");
		return 0;
	}
	auto eye = devices[0];
	devices.clear();
	
	if (eye != nullptr)
	{
		if (!eye->init(ps3eye::res_VGA, 60, ps3eye::fmt_BGR))
		{
			printf("failed to initialize PS3 eye camera\n");
			return 0;
		}
		else
		{
			eye->start();
		}
	}
	
	PS3EYEMic mic;
	MyAudioCallback audioCallback(AUDIO_HISTORY_LENGTH);
	
	if (eye != nullptr)
	{
		auto init = mic.init(eye->device(), &audioCallback);
		if(!init){
			printf("failed to initialize PS3 eye mic\n");
			return 0;
		}
	}
	
	while (1) {

	}

	mic.shut();
	
	if (eye != nullptr)
	{
		eye->stop();
		eye = nullptr;
	}
		
	return 0;
}
