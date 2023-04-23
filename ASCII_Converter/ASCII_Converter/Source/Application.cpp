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

[[nodiscard]] uint32_t GetMask(uint8_t inValue)
{
	return inValue == 1 ? 0xFFFFFFFF : 0x0;
}

using Color = std::array<uint32_t, 3>;

struct Sample
{
	uint32_t color;
	uint8_t sampleIndex;
};

namespace ASCII
{

	Application::Application() :
		mImageMap("Assets/ImageMap.bmp")
	{
		JobSystem::Initialize();
		int32_t ASCIISize = 16;
		Image gif("Assets/nyan-cat.gif");
		int FramesToRender = gif.GetFrames();

		Image exportImage(gif.GetWidth() * ASCIISize, gif.GetHeight() * ASCIISize, Image::EChannels::RGBA, FramesToRender);
		exportImage.SetDelays(gif.GetDelays(), FramesToRender);

		uint8_t* buffer = exportImage.GetBuffer();

		InitializeTable();

		const float bucketInverse = 1.0f / (255.f / static_cast<float>(scale.size()));
		auto sampleCache = std::vector<Sample>(gif.GetWidth());
		for (int frame = 0; frame < FramesToRender; frame++)
		{
			const auto sample = [frame, &gif](int x, int y) -> Color {

				const size_t index = ((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels();
				return { gif.GetBuffer()[index], gif.GetBuffer()[index + 1], gif.GetBuffer()[index + 2] };
			};

			for (int y = 0; y < exportImage.GetHeight(); y++)
			{
				if (y % ASCIISize == 0)
				{
					for (int x = 0; x < gif.GetWidth(); ++x)
					{
						const auto c = sample(x, y / ASCIISize);
						sampleCache[x] = { .color = 255 << 24 | c[2] << 16 | c[1] << 8 | c[0] , .sampleIndex = static_cast<uint8_t>(Luminance(c[0], c[1], c[2]) * bucketInverse)};
					}
				}

				for (int x = 0; x < exportImage.GetWidth(); x += 16)
				{
					const auto& sample = sampleCache[x / ASCIISize];
					for (int x2 = 0 ; x2 < ASCIISize ; ++x2)
					{
						const uint8_t isVisible = ascii_bits[sample.sampleIndex].test(((x + x2) % ASCIISize) + (y % ASCIISize) * ASCIISize);
						const int index = (((y + frame * exportImage.GetHeight()) * exportImage.GetWidth()) + x + x2) * exportImage.GetChannels();
						uint32_t mask = GetMask(isVisible);
						uint32_t result = sample.color & mask;
						memcpy(buffer + index, &result, 4);
					}					
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
