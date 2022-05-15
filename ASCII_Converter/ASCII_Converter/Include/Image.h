#pragma once
#include <string>
#include <vector>
#include "dynamic_bitset.hpp"

namespace ASCII
{
	struct GifHeader;

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
		GIF,
		PNG,
		JPG
	};

	Image(const std::string& inPath, EChannels inChannel = EChannels::UseImageData);
	Image(int32_t inWidth, int32_t inHeight, EChannels inChannel = EChannels::RGB, int32_t inFrames = 1);
	~Image();
	int32_t GetWidth() const { return mWidth; }
	int32_t GetHeight() const { return mHeight; }
	int32_t GetChannels() const { return mChannels; }
	uint8_t* GetBuffer() { return mPixels; }
	
	const int32_t* GetDelays() { return mDelays; }
	void SetDelays(const int32_t* inDelays, int32_t frames);
	

	void CopyRect(const Image& inImage, int32_t inSrcX, int32_t inSrcY, int32_t inDestX, int32_t inDestY, int32_t x, int32_t y);
	void ExportImage(const std::string& inExportPath, ExportAs inExportAs = ExportAs::JPG, int Quality = 100) const;
	int32_t GetFrames() const { return mFrames; }
private:
	void LoadFromDisk(const std::string& inPath, EChannels inChannel);
	void SaveAnimatedGifToFile(const std::string& inExportPath) const;

	void ProcessFrame(size_t frame, std::ofstream& outfile, const sul::dynamic_bitset<>& inBitfield) const;


	void WriteGraphicControlExtention(size_t& frame, std::ofstream& outfile) const;
	void WriteApplicationExentionBlock(std::ofstream& outfile) const;
	void WriteImageDescriptor(std::ofstream& outfile) const;
	void Compress(std::vector<int>& indexStream, size_t frame, sul::dynamic_bitset<>& inBitField) const;

	void SaveBitField(const sul::dynamic_bitset<>& bitField, std::ofstream& outfile) const;
	void BuildBitField(std::vector<std::vector<int>> codeStreams, sul::dynamic_bitset<>& bitField) const;

	std::vector<int> BuildIndexStream(const size_t& frame, GifHeader& header) const;

	int32_t		mWidth;
	int32_t		mHeight;
	int32_t		mChannels;
	int32_t		mFrames = 1;
	uint8_t*	mPixels = nullptr;
	int32_t*	mDelays = nullptr;
};

}

