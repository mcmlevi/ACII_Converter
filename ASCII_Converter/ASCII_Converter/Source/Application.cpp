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
		Image CopyBuffer(ASCIISize, ASCIISize);
		Image gif("Assets/nyan-cat.gif");
		int FramesToRender = gif.GetFrames();
		
		uint8_t emptyColor[3]{ 0,0,0 };
		const size_t colorSize = 3 * sizeof(uint8_t);

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
					size_t index = ((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels();

					float luminance = Luminance(&buffer[index]);
					int luminecenceIndex = static_cast<int>(floorf(luminance / bucket));
					unsigned char asciiChar = scale[luminecenceIndex];
					int yIndex = asciiChar / ASCIISize;
					int xIndex = asciiChar % ASCIISize;

					CopyBuffer.CopyRect(mImageMap, xIndex * ASCIISize, yIndex * ASCIISize, 0, 0, ASCIISize, ASCIISize);
					for (size_t CoppyBufferY = 0; CoppyBufferY < ASCIISize; CoppyBufferY++)
					{
						for (size_t CoppyBufferX = 0; CoppyBufferX < ASCIISize; CoppyBufferX++)
						{
							const size_t copyBufferIndex = (CoppyBufferY * ASCIISize + CoppyBufferX) * CopyBuffer.GetChannels();
							uint8_t* Copybuffer = CopyBuffer.GetBuffer();
							
							if (memcmp(emptyColor, Copybuffer + copyBufferIndex, colorSize) == 0)
							{
								memcpy(Copybuffer + copyBufferIndex, buffer, colorSize);
							}
							else
							{
								memcpy(Copybuffer + copyBufferIndex, buffer + index, colorSize);
							}
						}

						exportImage.CopyRect(CopyBuffer, 0, 0, x * ASCIISize, ((y * ASCIISize) + frame * exportImage.GetHeight()), ASCIISize, ASCIISize);
					}
				}
			}
		}
		
		exportImage.SetColorTableFromImage(gif);
		exportImage.ExportImage("Output/test.gif", Image::ExportAs::GIF, 100);
	};
}
