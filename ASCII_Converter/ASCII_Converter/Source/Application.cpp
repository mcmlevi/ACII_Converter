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

void Blit(const std::bitset<256>& bitset, uint8_t* buffer, Color color, size_t index, int width)
{
	for (int y = 0; y < 16; ++y)
	{
		for (int x = 0; x < 16; ++x)
		{
			size_t globalIndex = index + (x + y * width) * 3;
			uint32_t should = bitset.test(x + y * 16);
			buffer[globalIndex + 0] = color[0] * should;
			buffer[globalIndex + 1] = color[1] * should;
			buffer[globalIndex + 2] = color[2] * should;
		}
	}
}

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

		const float bucketInverse = 1.0f / (255.f / static_cast<float>(scale.size()));
		auto sampleCache = std::vector<Sample>(gif.GetWidth());

		const size_t bytesPerFrame = exportImage.GetHeight() * exportImage.GetChannels() * exportImage.GetWidth();
		std::atomic<int> jobCounter{};
		for (int frame = 0; frame < FramesToRender; frame++)
		{

			JobSystem::Execute([&, frame]()
			{
				for (int y = 0; y < gif.GetHeight(); y++)
				{
					for (int x = 0; x < gif.GetWidth(); ++x)
					{
						const size_t index = ((y + frame * gif.GetHeight()) * gif.GetWidth() + x) * gif.GetChannels();
						Color c = { gif.GetBuffer()[index], gif.GetBuffer()[index + 1], gif.GetBuffer()[index + 2] };
						uint8_t sampleIndex = static_cast<uint8_t>(Luminance(c[0], c[1], c[2]) * bucketInverse);
						Blit(ascii_bits[sampleIndex], exportImage.GetBuffer(), c, (x * 16 + y * 16 * exportImage.GetWidth()) * exportImage.GetChannels() + frame * bytesPerFrame, exportImage.GetWidth());
					}
				}
			}, &jobCounter);

			
		}
		JobSystem::Wait(&jobCounter);
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
