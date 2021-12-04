#pragma once
#include <string>

namespace ASCII
{
class Image
{
public:
	
	enum class EChannels
	{
		UseImageData = 0,
		Gray,
		GrayAlpha,
		RGB,
		RGBA,
	};

	enum class ExportAs
	{
		PNG,
		JPG
	};

	Image(const std::string& inPath, EChannels inChannel = EChannels::UseImageData);
	Image(int32_t inWidth, int32_t inHeight, EChannels inChannel = EChannels::RGB);
	~Image();
	int32_t GetWidth() { return mWidth; }
	int32_t GetHeight() { return mHeight; }
	int32_t GetChannels() { return mChannels; }
	uint8_t* GetBuffer() { return mPixels; }

	void CopyRect(const Image& inImage, int32_t inSrcX, int32_t inSrcY, int32_t inDestX, int32_t inDestY, int32_t x, int32_t y);
	void ExportImage(const std::string& inExportPath, ExportAs inExportAs = ExportAs::JPG);
private:
	void LoadFromDisk(const std::string& inPath, EChannels inChannel);

	int32_t mWidth;
	int32_t mHeight;
	int32_t	mChannels;
	
	uint8_t* mPixels = nullptr;
};

}

