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

// TODO support timings
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
		Image gif("Assets/nyan-cat.gif");
		int FramesToRender = gif.GetFrames();

		Image exportImage(gif.GetWidth() * ASCIISize, gif.GetHeight() * ASCIISize, Image::EChannels::RGB, FramesToRender);
		exportImage.SetDelays(gif.GetDelays(), FramesToRender);

		uint8_t* buffer = gif.GetBuffer();

		const uint8_t backGroundColor[3]{ buffer[0], buffer[1], buffer[2] };
		const uint8_t emptyColor[3]{ 0,0,0 };
		const size_t colorSize = 3 * sizeof(uint8_t);

		for (size_t y = 0; y < mImageMap.GetHeight(); y++)
		{
			for (size_t x = 0; x < mImageMap.GetWidth(); x++)
			{
				const size_t index = (y * mImageMap.GetWidth() + x) * mImageMap.GetChannels();
				if (memcmp(emptyColor, mImageMap.GetBuffer() + index, colorSize) == 0)
				{
					memcpy(mImageMap.GetBuffer() + index, backGroundColor, colorSize);
				}
			}
		}

		float bucket = 255.f / static_cast<float>(scale.size());
		for (size_t frame = 0; frame < FramesToRender; frame++)
		{
			for (size_t y = 0; y < gif.GetHeight(); y++)
			{
				for (size_t x = 0; x < gif.GetWidth(); x++)
				{
					const size_t index = ((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels();

					float luminance = Luminance(&buffer[index]);
					int luminecenceIndex = static_cast<int>(floorf(luminance / bucket));
					unsigned char asciiChar = scale[luminecenceIndex];
					int yIndex = asciiChar / ASCIISize;
					int xIndex = asciiChar % ASCIISize;

					exportImage.CopyRect(mImageMap, xIndex * ASCIISize, yIndex * ASCIISize, x * ASCIISize, y * ASCIISize + frame * exportImage.GetHeight(), ASCIISize, ASCIISize);

					const size_t yOffset = y * ASCIISize + frame * exportImage.GetHeight();
					const size_t xOffset = x * ASCIISize;

					for (size_t CoppyBufferY = 0; CoppyBufferY < ASCIISize; CoppyBufferY++)
					{
						for (size_t CoppyBufferX = 0; CoppyBufferX < ASCIISize; CoppyBufferX++)
						{
							const size_t copyBufferindex = (( yOffset + CoppyBufferY) * exportImage.GetWidth() + xOffset + CoppyBufferX) * exportImage.GetChannels();
							
							if (memcmp(backGroundColor, exportImage.GetBuffer() + copyBufferindex, colorSize) != 0)
								memcpy(exportImage.GetBuffer() + copyBufferindex, buffer + index, colorSize);
						}
					}
				}
			}
		}
		
		exportImage.SetColorTableFromImage(gif);
		exportImage.ExportImage("Output/test.gif", Image::ExportAs::GIF, 100);
	};
}
