#include "Application.h"
#include "stb_image.h"
#include "Image.h"
#include "JobSystem.h"
#include <iostream>

const std::string scale = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'.";

float Luminance(uint8_t* pixel)
{
	return (pixel[0] * 0.3f) + (pixel[1] * 0.59f) + (pixel[2] * 0.11f);
}

// TODO profile
// TODO speedup
// TODO Extract colors from gif
// TODO Extract colors from image
// TODO Support Transparency

namespace ASCII
{

	Application::Application() :
		mImageMap("Assets/ImageMap.bmp")
	{
		JobSystem::Initialize();
		int32_t ASCIISize = 16;
		std::cin.get();
		//Image gif("Assets/789839.gif");
		Image gif("Assets/rick-roll.gif");
		int FramesToRender = gif.GetFrames();
		Image exportImage(gif.GetWidth() * ASCIISize, gif.GetHeight() * ASCIISize, Image::EChannels::RGB, FramesToRender);
		exportImage.SetDelays(gif.GetDelays(), FramesToRender);

		uint8_t* buffer = gif.GetBuffer();
		float bucket = 255.f / static_cast<float>(scale.size());
		for (size_t frame = 0; frame < FramesToRender; frame++)
		{
			for (size_t y = 0; y < gif.GetHeight(); y++)
			{
				for (size_t x = 0; x < gif.GetWidth(); x++)
				{
					if(gif.GetChannels() == 4 && buffer[((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels() + 3] < 255)
						continue;
					float luminance = Luminance(&buffer[((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels()]);
					int index = static_cast<int>(floorf(luminance / bucket));
					unsigned char asciiChar = scale[index];
					int yIndex = asciiChar / ASCIISize;
					int xIndex = asciiChar % ASCIISize;

					exportImage.CopyRect(mImageMap, xIndex * ASCIISize, yIndex * ASCIISize, x * ASCIISize, (y + frame * gif.GetHeight()) * ASCIISize, ASCIISize, ASCIISize);
				}
			}
		}
		//gif.ExportImage("sampleTest.gif", Image::ExportAs::GIF, 100);
		exportImage.ExportImage("Output/rick-roll.gif", Image::ExportAs::GIF, 100);
		//exportImage.ExportImage("Output/Test.gif", Image::ExportAs::GIF, 100);
	}
}
