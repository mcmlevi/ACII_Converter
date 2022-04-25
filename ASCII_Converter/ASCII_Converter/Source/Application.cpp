#include "Application.h"
#include "stb_image.h"
#include "Image.h"

const std::string scale = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'.";

float Luminance(uint8_t* pixel)
{
	return (pixel[0] * 0.3f) + (pixel[1] * 0.59f) + (pixel[2] * 0.11f);
}

// TODO Cleanup gif saving code
// TODO make a submit on github
// TODO profile
// TODO speedup
// TODO Extract colors from gif
// TODO Extract colors from image
// TODO Support Transparency

namespace ASCII
{

	Application::Application() :
		mImageMap("Assets/ImageMapx8t.png")
	{
		int32_t ASCIISize = 8;
		//Image exportImage(256, 256, Image::EChannels::RGB);
		//exportImage.CopyRect(mImageMap, 0, 0, 0, 0, 256, 256);
		//exportImage.ExportImage("test.jpg");
		//for (size_t y = 0; y < exportImage.GetHeight(); y++)
		//{
		//	for (size_t x = 0; x < exportImage.GetWidth(); x++)
		//	{

		//	}
		//}

		//char buffer[13];
		//for (size_t frame = 0; frame < 88; frame++)
		//{
		//	sprintf(buffer, "IMG%05u.bmp", frame);
		//	Image test("Assets/twitchLogo/" + std::string(buffer));
		//	Image exportImage(test.GetWidth() * 16, test.GetHeight() * 16, Image::EChannels::RGB);

		//	uint8_t* buffer = test.GetBuffer();
		//	float bucket = 255.f / static_cast<float>(scale.size());
		//	for (size_t y = 0; y < test.GetHeight(); y++)
		//	{
		//		for (size_t x = 0; x < test.GetWidth(); x++)
		//		{
		//			float luminance = Luminance(&buffer[(y * test.GetWidth() + x) * test.GetChannels()]);
		//			int index = static_cast<int>(floorf(luminance / bucket));
		//			unsigned char asciiChar = scale[index];
		//			int yIndex = asciiChar / 16;
		//			int xIndex = asciiChar % 16;

		//			exportImage.CopyRect(mImageMap, xIndex * 16, yIndex * 16, x * 16, y * 16, 16, 16);
		//		}
		//	}
		//	exportImage.ExportImage("gif" + std::to_string(frame) + ".jpg");
		//}

		Image gif("Assets/cat.gif");
		int FramesToRender = gif.GetFrames();
		Image exportImage(gif.GetWidth() * ASCIISize, gif.GetHeight() * ASCIISize, Image::EChannels::RGBA, FramesToRender);
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
		//gif.ExportImage("STBI_ISSUE_NoASCII.gif", Image::ExportAs::GIF, 100);
		exportImage.ExportImage("CatGifTest.gif", Image::ExportAs::GIF, 100);
		//gif.ExportImage("STBI_ISSUE_.jpg", Image::ExportAs::JPG, 100);
		//exportImage.ExportImage("animationTest.gif", Image::ExportAs::GIF, 100);

	}
}
