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
		Image gif("Assets/SomeFile.gif");
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
							size_t copyBufferIndex = (CoppyBufferY * ASCIISize + CoppyBufferX) * CopyBuffer.GetChannels();
							uint8_t* buffer = CopyBuffer.GetBuffer();
							uint8_t R = buffer[copyBufferIndex];
							uint8_t G = buffer[copyBufferIndex + 1];
							uint8_t B = buffer[copyBufferIndex + 2];
							if (R != 0 || G != 0 || B != 0)
							{
								CopyBuffer.GetBuffer()[copyBufferIndex] = gif.GetBuffer()[index];
								CopyBuffer.GetBuffer()[copyBufferIndex + 1] = gif.GetBuffer()[index + 1];
								CopyBuffer.GetBuffer()[copyBufferIndex + 2] = gif.GetBuffer()[index + 2];
							}
							else
							{
								CopyBuffer.GetBuffer()[copyBufferIndex] = gif.GetBuffer()[0];
								CopyBuffer.GetBuffer()[copyBufferIndex + 1] = gif.GetBuffer()[1];
								CopyBuffer.GetBuffer()[copyBufferIndex + 2] = gif.GetBuffer()[2];
							}
						}
					
					exportImage.CopyRect(CopyBuffer, 0, 0, x * ASCIISize, ((y * ASCIISize) + frame * exportImage.GetHeight()), ASCIISize, ASCIISize);
				}
			}
		}
		exportImage.BuildColorTable();
		exportImage.ExportImage("Output/blob.gif", Image::ExportAs::GIF, 100);
	}
}
