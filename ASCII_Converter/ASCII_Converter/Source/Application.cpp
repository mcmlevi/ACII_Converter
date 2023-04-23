#include "Application.h"
#include "stb_image.h"
#include "Image.h"
#include "JobSystem.h"
#include <iostream>
#include <bitset>
#include <array>

const std::string scale = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'.";
std::array<std::bitset<256>, 69> ascii_bits;
[[nodiscard]] float Luminance(uint8_t* pixel)
{
	return (pixel[0] * 0.3f) + (pixel[1] * 0.59f) + (pixel[2] * 0.11f);
}

[[nodiscard]] float Luminance(uint8_t r, uint8_t g, uint8_t b)
{
	return (r * 0.3f) + (g * 0.59f) + (b * 0.11f);
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

		uint8_t* buffer = exportImage.GetBuffer();

		InitializeTable();

		const float bucket = 255.f / static_cast<float>(scale.size());
		for (int frame = 0; frame < FramesToRender; frame++)
		{
			const auto sample = [frame, &gif](int x, int y) -> std::tuple<uint8_t, uint8_t, uint8_t> {
				x /= 16;
				y /= 16;

				const size_t index = ((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels();
				return { gif.GetBuffer()[index], gif.GetBuffer()[index + 1], gif.GetBuffer()[index + 2] };
			};
			
			const auto calculateLuminanceIndex = [&bucket, &sample](int x, int y, int& r, int& g, int& b)
			{
				const auto sampleColor = sample(x, y);
				r = std::get<0>(sampleColor);
				g = std::get<1>(sampleColor);
				b = std::get<2>(sampleColor);
				const float luminance = Luminance(r, g, b);
				return  std::min(static_cast<int>(floorf(luminance / bucket)), static_cast<int>(scale.size() - 1));
			};

			for (int y = 0; y < exportImage.GetHeight(); y++)
			{
				int luminanceIndex = -1;
				int r, g, b;
				for (int x = 0; x < exportImage.GetWidth(); x++)
				{
					if (x % ASCIISize == 0)
						luminanceIndex = calculateLuminanceIndex(x, y, r, g, b);
					 
					const int isVisible = ascii_bits[luminanceIndex].test((x % ASCIISize) + (y % ASCIISize) * ASCIISize);
					const int index = (((y + frame * exportImage.GetHeight()) * exportImage.GetWidth()) + x) * exportImage.GetChannels();
					buffer[index + 0] = r * isVisible;
					buffer[index + 1] = g * isVisible;
					buffer[index + 2] = b * isVisible;
					//exportImage.GetBuffer()[]

				//	const size_t index = ((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels();

				//	// if it's the background color we can use ASCII char 0 to copy a square of the full background color
				//	if(memcmp(backGroundColor, buffer + index, colorSize) == 0)
				//	{
				//		exportImage.CopyRect(mImageMap, 0, 0, x * ASCIISize, y * ASCIISize + frame * exportImage.GetHeight(), ASCIISize, ASCIISize);
				//	}
				//	else
				//	{
				//		float luminance = Luminance(&buffer[index]);
				//		int luminecenceIndex = std::min(static_cast<int>(floorf(luminance / bucket)), static_cast<int>(scale.size() - 1));
				//		unsigned char asciiChar = scale[luminecenceIndex];
				//		int yIndex = asciiChar / ASCIISize;
				//		int xIndex = asciiChar % ASCIISize;

				//		exportImage.CopyRect(mImageMap, xIndex * ASCIISize, yIndex * ASCIISize, x * ASCIISize, y * ASCIISize + frame * exportImage.GetHeight(), ASCIISize, ASCIISize);

				//		const size_t yOffset = y * ASCIISize + frame * exportImage.GetHeight();
				//		const size_t xOffset = x * ASCIISize;

				//		for (size_t CoppyBufferY = 0; CoppyBufferY < ASCIISize; CoppyBufferY++)
				//		{
				//			for (size_t CoppyBufferX = 0; CoppyBufferX < ASCIISize; CoppyBufferX++)
				//			{
				//				const size_t copyBufferindex = ((yOffset + CoppyBufferY) * exportImage.GetWidth() + xOffset + CoppyBufferX) * exportImage.GetChannels();

				//				if (memcmp(backGroundColor, exportImage.GetBuffer() + copyBufferindex, colorSize) != 0)
				//					memcpy(exportImage.GetBuffer() + copyBufferindex, buffer + index, colorSize);
				//			}
				//		}
				//	}
				}
			}
		}
		
		exportImage.SetColorTableFromImage(gif);
		exportImage.ExportImage("Output/test.gif", Image::ExportAs::GIF, 100);
	};

	void Application::InitializeTable()
	{
		const int32_t ASCIISize = 16;
		const uint8_t black_color[3]{ 0, 0, 0 };
		for (int i = 0; i < scale.size(); ++i)
		{
			const char character = scale[i];
			for (size_t y = (character / ASCIISize) * ASCIISize; y < (character / ASCIISize + 1) * ASCIISize; y++)
			{
				for (size_t x = (character % ASCIISize) * ASCIISize; x < (character % ASCIISize + 1) * ASCIISize; x++)
				{
					const bool isBackground = *(mImageMap.GetBuffer() + (y * mImageMap.GetWidth() + x) * mImageMap.GetChannels()) == 0;
					ascii_bits[i].set((y % ASCIISize) * ASCIISize + (x % ASCIISize), !isBackground);
				}
			}

		}
	}

}
