#include "Image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "Utility.h"

#include <cassert>
#include <stdint.h>
#include <iomanip>
#include <cstring>
#include <bitset>

#include <iostream>

#define USE_LZW_COMPRESSION 1

namespace ASCII 
{
	constexpr int masks[8]{ 0b0000'0001, 0b0000'0010, 0b0000'0100, 0b0000'1000, 0b0001'0000, 0b0010'0000, 0b0100'0000, 0b1000'0000 };
	struct GraphicControlExtention
	{
		uint8_t mIntroducer = 0x21;
		uint8_t mGCL = 0xF9;
		uint8_t mSize = 0x04;
		uint8_t mPackedField = 0x0000'0000;
		uint16_t mDelay = 0x00;
		//uint16_t mDelay = 0x6400;
		uint8_t mTCI = 0x00;
		uint8_t mBlockTerminator = 0x00;
	};

	std::ostream& operator<<(std::ostream& os, const GraphicControlExtention& inHeader)
	{
		os.write((char*)&inHeader.mIntroducer, 1);
		os.write((char*)&inHeader.mGCL, 1);
		os.write((char*)&inHeader.mSize, 1);
		os.write((char*)&inHeader.mPackedField, 1);
		os.write((char*)&inHeader.mDelay, 2);
		os.write((char*)&inHeader.mTCI, 1);
		os.write((char*)&inHeader.mBlockTerminator, 1);

		return os;
	}

	struct AEB
	{
		uint8_t mBlock[3] ={ 0x21, 0xFF, 0x0B };
		uint8_t mNesspace[11]{ 0x4E, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45, 0x32, 0x2E, 0x30 };
		uint8_t mBytes = 0x03;
		uint8_t mData[3] = { 0x01, 0x00, 0x00 };
		uint8_t mTerminator = { 0x00 };
	};

	std::ostream& operator<<(std::ostream& os, const AEB& inHeader)
	{
		os.write((char*)&inHeader.mBlock, 3);
		os.write((char*)&inHeader.mNesspace, 11);
		os.write((char*)&inHeader.mBytes, 1);
		os.write((char*)&inHeader.mData, 3);
		os.write((char*)&inHeader.mTerminator, 1);

		return os;
	}

	struct ImageData
	{
		uint8_t mLZW = 0x02;
		struct SubBlock
		{
			uint8_t mBytes;
			std::vector<uint8_t> mData;
		};
		std::vector<SubBlock> mBlocks;
		uint8_t mBlockTerinator = 0x00;
	};

	std::ostream& operator<<(std::ostream& os, const ImageData& inDescriptor)
	{
		os.write((char*)&inDescriptor.mLZW, 1);
		for (const ImageData::SubBlock& block : inDescriptor.mBlocks)
		{
			os.write((char*)&block.mBytes, 1);
			os.write((char*)block.mData.data(), block.mData.size());
		}
		os.write((char*)&inDescriptor.mBlockTerinator, 1);
		return os;
	}

	struct ImageDescriptor
	{
		uint8_t mIS = 0x2C;
		uint16_t mleft = 0x00;
		uint16_t mRight = 0x00;
		uint16_t mWidth = 10;
		uint16_t mheight = 10;
		uint8_t	mPackedField = 0x00;
	};

	std::ostream& operator<<(std::ostream& os, const ImageDescriptor& inDescriptor)
	{
		os.write((char*)&inDescriptor.mIS, 1);
		os.write((char*)&inDescriptor.mleft, 2);
		os.write((char*)&inDescriptor.mRight, 2);
		os.write((char*)&inDescriptor.mWidth, 2);
		os.write((char*)&inDescriptor.mheight, 2);
		os.write((char*)&inDescriptor.mPackedField, 1);

		//os << inDescriptor.mIS << inDescriptor.mleft << inDescriptor.mRight << inDescriptor.mWidth << inDescriptor.mheight << inDescriptor.mPackedField;
		return os;
	}

	struct GifHeader
	{
		uint8_t		mDesriptor[6]{'G','I','F','8','9','a'};
		uint16_t	mWidth = 10;
		uint16_t	mHeight = 10;
		uint8_t		mPackedField = 0x91;
		uint8_t		mBackgroundColorIndex = 0;
		uint8_t		mPixelAspectRatio = 0;
		uint8_t		mColorTable[12]{ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 }; // BLACK , RED, BLUE, WHITE
	};

	std::ostream& operator<<(std::ostream& os, const GifHeader& inHeader)
	{
		os.write((char*)&inHeader.mDesriptor, 6);
		os.write((char*)&inHeader.mWidth, 2);
		os.write((char*)&inHeader.mHeight, 2);
		os.write((char*)&inHeader.mPackedField, 1);
		os.write((char*)&inHeader.mBackgroundColorIndex, 1);
		os.write((char*)&inHeader.mPixelAspectRatio, 1);
		os.write((char*)&inHeader.mColorTable, 12);
		return os;
	}

Image::Image(const std::string& inPath, EChannels inChannel):
	mChannels{ static_cast<int>(inChannel) }
{
	LoadFromDisk(inPath, inChannel);
}

Image::Image(int32_t inWidth, int32_t inHeight, EChannels inChannel, int32_t inFrames) :
	mChannels{ static_cast<int>(inChannel) },
	mWidth(inWidth),
	mHeight(inHeight),
	mFrames(inFrames)
{
	mPixels = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * mChannels * (mHeight * inFrames) * mWidth));
}

Image::~Image()
{
	free(mPixels);
	if (mDelays)
		delete[] mDelays;
}

void Image::SetDelays(const int32_t* inDelays, int32_t frames)
{
	assert(frames == mFrames && "Frame count does not match");
	mDelays = new int32_t[frames];
	memcpy(mDelays, inDelays, frames * sizeof(int32_t));
}

void Image::CopyRect(const Image& inSrc, int32_t inSrcX, int32_t inSrcY, int32_t inDestX, int32_t inDestY, int32_t x, int32_t y)
{
	// TODO figure out what to do with different channels
	assert(mChannels == inSrc.mChannels && "Channels must match");
	size_t bytesToCopy = (size_t)x * sizeof(uint8_t) * (size_t)mChannels;
	
	for (size_t i = 0; i < y; i++)
	{
		uint8_t* dest = mPixels + (inDestX + (inDestY + i) * mWidth) * mChannels;
		uint8_t* src = inSrc.mPixels + (inSrcX + (inSrcY + i) * inSrc.mWidth) * inSrc.mChannels;
		memcpy(dest, src, bytesToCopy);
	}

		
}

void Image::ExportImage(const std::string& inExportPath, ExportAs inExportAs, int Quality) const
{
	// sizeof(uint8_t) * mWidth * mChannels
	switch (inExportAs)
	{
	case ASCII::Image::ExportAs::PNG:
		stbi_write_png(inExportPath.c_str(), GetWidth(), GetHeight() * mFrames, mChannels, mPixels, 0);
		break;
	case ASCII::Image::ExportAs::JPG:
		stbi_write_jpg(inExportPath.c_str(), GetWidth(), GetHeight() * mFrames, mChannels, mPixels, Quality);
		break;
	case ExportAs::GIF:
		SaveAnimatedGifToFile(inExportPath);
		break;
	}
	
}

void Image::LoadFromDisk(const std::string& inPath, EChannels inChannel)
{
	if(inPath.find(".gif") != std::string::npos)
	{
		std::string buffer = ReadFileToBuffer(inPath);
		mPixels = stbi_load_gif_from_memory((unsigned char*)buffer.c_str(), buffer.size(), &mDelays, &mWidth, &mHeight, &mFrames, &mChannels, mChannels);
	}
	else
	{
		mPixels = stbi_load(inPath.c_str(), &mWidth, &mHeight, &mChannels, mChannels);
	}
	assert(mPixels != nullptr && "Image failed to load.");
}

void Image::SaveAnimatedGifToFile(const std::string& inExportPath) const
{
	std::ofstream outfile(inExportPath, std::ios::binary);
	GifHeader header;
	header.mWidth = mWidth;
	header.mHeight = mHeight;
	outfile << header;

	for (size_t frame = 0; frame < mFrames; frame++)
		ProcessFrame(frame, outfile, header);

	uint8_t terminator = 0x3B;
	outfile.write((char*)&terminator, 1);
	outfile.close();
}

void Image::WriteImageDescriptor(std::ofstream& outfile) const
{
	ImageDescriptor desc;
	desc.mWidth = mWidth;
	desc.mheight = mHeight;
	outfile << desc;
}


std::vector<int> Image::BuildIndexStream(size_t& frame, GifHeader& header) const
{
	std::vector<int> indexStream{};
	for (int y = 0; y < mHeight; ++y)
	{
		for (int x = 0; x < mWidth; ++x)
		{
			size_t index = ((y + frame * mHeight) * mWidth + x) * mChannels;

			for (int i = 0; i < mChannels; ++i)
				mPixels[index + i] = mPixels[index + i] < 122 ? 0 : 255;
			if (mChannels == 4 && mPixels[index + 3] < 255)
				indexStream.push_back(2);
			else if (memcmp(&mPixels[index], &header.mColorTable[0], 3) == 0)
				indexStream.push_back(0);
			//else if (memcmp(&mPixels[index], &header.mColorTable[3], 3) == 0)
			//	indexStream.push_back(1);
			else if (memcmp(&mPixels[index], &header.mColorTable[6], 3) == 0)
				indexStream.push_back(2);
			else
				indexStream.push_back(3);
		}
	}
	return indexStream;
}

void Image::CompressAndSave(std::vector<int>& indexStream, size_t& frame, std::ofstream& outfile) const
{
#if USE_LZW_COMPRESSION
	std::vector<std::vector<int>> codeStreams{ {} };
	std::vector<std::vector<int>> initialCodeTable{ { 0 },{ 1 },{ 2 },{ 3 },{ 4 },{ 5 } };
	std::vector<std::vector<std::vector<int>>> codeTables{ initialCodeTable };

	int tableIndex{ 0 };
	codeStreams[tableIndex] = initialCodeTable[4];
	std::vector<int> indexBuffer{ indexStream.front() };


	for (int i = 1; i < indexStream.size(); i++)
	{
		int K = indexStream[i];

		std::vector<int> indexBufferPlusK{ indexBuffer };
		indexBufferPlusK.push_back(K);

		bool isInCodeTable = false;
		for (const std::vector<int>& row : codeTables[tableIndex])
		{
			if (row.size() != indexBufferPlusK.size())
				continue;

			isInCodeTable = std::equal(indexBufferPlusK.begin(), indexBufferPlusK.end(), row.begin(), row.end());
			if (isInCodeTable)
				break;
		}

		if (isInCodeTable)
		{
			indexBuffer = indexBufferPlusK;
		}
		else
		{
			codeTables[tableIndex].push_back(indexBufferPlusK);

			for (int index = 0; index < codeTables[tableIndex].size(); ++index)
			{
				if (codeTables[tableIndex][index].size() != indexBuffer.size())
					continue;

				if (std::equal(indexBuffer.begin(), indexBuffer.end(), codeTables[tableIndex][index].begin(), codeTables[tableIndex][index].end()))
				{
					codeStreams[tableIndex].push_back(index);
					break;
				}
			}

			indexBuffer.clear();
			indexBuffer.push_back(K);

			if (codeTables[tableIndex].size() == 0xFFF)
			{
				codeStreams[tableIndex].insert(codeStreams[tableIndex].end(), codeTables[tableIndex][5].begin(), codeTables[tableIndex][5].end());
				++tableIndex;
				codeStreams.push_back({});
				codeStreams[tableIndex] = initialCodeTable[4];

				codeTables.push_back(initialCodeTable);
			}
		}
	}

	for (int index = 0; index < codeTables[tableIndex].size(); ++index)
	{
		if (codeTables[tableIndex][index].size() != indexBuffer.size())
			continue;

		if (std::equal(indexBuffer.begin(), indexBuffer.end(), codeTables[tableIndex][index].begin(), codeTables[tableIndex][index].end()))
		{
			codeStreams[tableIndex].push_back(index);
			break;
		}
	}

	codeStreams[tableIndex].insert(codeStreams[tableIndex].end(), codeTables[tableIndex][5].begin(), codeTables[tableIndex][5].end());

	std::cout << "\tFinished CodeStream of Frame: " << frame << "\n";

	sul::dynamic_bitset<> bitField{};
	BuildBitField(codeStreams, bitField);
	SaveBitField(bitField, outfile);

#else
	ImageData data;
	indexStream.insert(indexStream.begin(), 4);
	indexStream.push_back(5);
	int subBlocksBytesRead = 0;
	int remainingSize = indexStream.size();
	do
	{
		if (remainingSize - 0xFF < 0)
		{
			ImageData::SubBlock block2;
			block2.mBytes = remainingSize;
			for (int j = 0; j < remainingSize; ++j)
			{
				block2.mData.push_back(indexStream[j + subBlocksBytesRead]);
			}
			data.mBlocks.push_back(std::move(block2));
			break;
		}
		ImageData::SubBlock block;
		block.mBytes = 0xFF;

		for (int i = 0; i < 0xFF; ++i)
		{
			block.mData.push_back(indexStream[i + subBlocksBytesRead]);
		}
		data.mBlocks.push_back(std::move(block));

		remainingSize -= 0xFF;
		subBlocksBytesRead += 0xFF;
	} while (remainingSize >= 0);
	outfile << data;

#endif // USE_LZW_COMPRESSION
}

void Image::SaveBitField(sul::dynamic_bitset<>& bitField, std::ofstream& outfile) const
{
	ImageData data;
	int remainingSize = bitField.num_blocks();
	int subBlocksBytesRead = 0;
	do
	{
		if (remainingSize - 0xFF < 0)
		{
			ImageData::SubBlock block2;
			block2.mBytes = remainingSize;
			for (int j = 0; j < remainingSize; ++j)
			{
				block2.mData.push_back(*(bitField.data() + j + subBlocksBytesRead));
			}
			data.mBlocks.push_back(std::move(block2));
			break;
		}
		ImageData::SubBlock block;
		block.mBytes = 0xFF;

		for (int i = 0; i < 0xFF; ++i)
		{
			block.mData.push_back(*(bitField.data() + i + subBlocksBytesRead));
	}
		data.mBlocks.push_back(std::move(block));

		remainingSize -= 0xFF;
		subBlocksBytesRead += 0xFF;
	} while (remainingSize >= 0);
	outfile << data;
}

void Image::BuildBitField(std::vector<std::vector<int>> codeStreams, sul::dynamic_bitset<>& bitField) const
{
	for (const std::vector<int>& stream : codeStreams)
	{
		int codeSize = 3;
		int32_t bytesToRead = 1 << (codeSize - 1);
		int bytesRead = 0;
		for (int i = 0; i < stream.size(); ++i)
		{

			bool codeSizeChanged = false;
			if (bytesRead == bytesToRead)
			{
				codeSizeChanged = true;
				bytesRead = 0;
				codeSize++;
				bytesToRead = 1 << (codeSize - 1);
			}

			bytesRead++;


			std::string s = std::bitset<32>(stream[i]).to_string(); // string conversion

			if (codeSizeChanged)
			{
				int remainder{ bitField.size() % 8 };
				if (remainder + codeSize < 8 && remainder != 0)
				{
					for (int i = 0; i < 8 - remainder - codeSize; ++i)
						bitField.push_back(0);
				}
			}

			for (int bit = 0; bit < codeSize; ++bit)
			{
				bitField.push_back(s[s.size() - 1 - bit] == '1' ? 1 : 0);
	}
}

		int remainder{ bitField.size() % 8 };
		for (int i = 0; i < 8 - remainder; ++i)
			bitField.push_back(0);
	}
}

void Image::ProcessFrame(size_t& frame, std::ofstream& outfile, GifHeader& header) const
{
	std::cout << "Started Frame: " << frame << "\n";
	
	WriteApplicationExentionBlock(outfile);
	WriteGraphicControlExtention(frame, outfile);
	WriteImageDescriptor(outfile);
	std::vector<int> indexStream = BuildIndexStream(frame, header);
	CompressAndSave(indexStream, frame, outfile);
	std::cout << "\tFinished BitField of Frame: " << frame << "\n";
}
	void Image::WriteGraphicControlExtention(size_t& frame, std::ofstream& outfile) const
	{
		GraphicControlExtention gce;
		gce.mDelay = mDelays[frame] / 10;
		if (mChannels == 4)
		{
			gce.mPackedField = 0b0000'0001;
			gce.mTCI = 1;
		}

		outfile << gce;
	}

	void Image::WriteApplicationExentionBlock(std::ofstream& outfile) const
	{
		AEB applicationExtentionBlock;
		outfile << applicationExtentionBlock;
	}
}
