#include "Image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "Utility.h"
#include "JobSystem.h"

#include <cassert>
#include <stdint.h>
#include <iomanip>
#include <cstring>
#include <bitset>

#include <iostream>
#include <algorithm>

namespace ASCII 
{
	size_t sHash(const std::vector<uint8_t>& inVecToHash, int inStart, int inEnd, size_t initialHash = 0)
	{
		std::hash<int> hasher;
		size_t result = initialHash;
		for (size_t i = inStart; i < inEnd; ++i)
			result ^= hasher(inVecToHash[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
		return result;
	}

	size_t sHash(const std::vector<int>& inVecToHash, int inStart, int inEnd, size_t initialHash = 0)
	{
		std::hash<int> hasher;
		size_t result = initialHash;
		for (size_t i = inStart; i < inEnd; ++i)
			result ^= hasher(inVecToHash[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
		return result;
	}

	ASCII::Image::ImageHash sGenerateImageHash(const uint8_t* inBuffer, uint8_t inIndex = 0) 
	{
		return { static_cast<uint32_t>(inIndex) << 24 | static_cast<uint32_t>(inBuffer[0] << 16) | static_cast<uint32_t>(inBuffer[1]) << 8 | static_cast<uint32_t>(inBuffer[2]) };
	}

	size_t sCountBits(size_t number)
	{
		return (size_t)std::ceil(log2(number));
	}

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
		uint8_t	mPackedField = 0x91;
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
		//uint8_t		mPackedField = 0x91;
		uint8_t		mPackedField = 0;
		uint8_t		mBackgroundColorIndex = 0;
		uint8_t		mPixelAspectRatio = 0;
		//std::vector<uint8_t> mColorTable;
		//uint8_t		mColorTable[12]{ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 }; // BLACK , RED, BLUE, WHITE
	};

	std::ostream& operator<<(std::ostream& os, const GifHeader& inHeader)
	{
		os.write((char*)&inHeader.mDesriptor, 6);
		os.write((char*)&inHeader.mWidth, 2);
		os.write((char*)&inHeader.mHeight, 2);
		os.write((char*)&inHeader.mPackedField, 1);
		os.write((char*)&inHeader.mBackgroundColorIndex, 1);
		os.write((char*)&inHeader.mPixelAspectRatio, 1);
		//os.write((char*)inHeader.mColorTable.data(), inHeader.mColorTable.size());
		//os.write((char*)inHeader.mColorTable, 12);
		return os;
	}

	//////////////////////////////////////////////////////////////////////////
	
	Image::IndexStreamGenerator::IndexStreamGenerator(const Image& inImage, int inFrame):
		mImage(inImage), mFrame(inFrame)
	{
	}

	bool Image::IndexStreamGenerator::Next(uint8_t& outNext)
	{
		if (mCurrentIndex >= mImage.mHeight * mImage.mWidth)
			return false;

		const size_t y = mCurrentIndex / mImage.mWidth;
		const size_t x = mCurrentIndex % mImage.mWidth;
		
		++mCurrentIndex;

		const size_t index = ((y + mFrame * mImage.mHeight) * mImage.mWidth + x) * mImage.mChannels;
		const ImageHash hash = sGenerateImageHash(mImage.mPixels + index);

		auto it = mImage.mColorTableList[mFrame].find({ hash });

		assert(it != mImage.mColorTableList[mFrame].end() && "hash not found in table");
		
		outNext =  (it->Hash & 0xFF000000) >> 24;
		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	Image::Image(const std::string& inPath, EChannels inChannel) :
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
		const size_t bytesToCopy = (size_t)x * sizeof(uint8_t) * (size_t)mChannels;
		
		for (size_t i = 0; i < y; i++)
		{
			uint8_t* dest = mPixels + (inDestX + (inDestY + i) * mWidth) * mChannels;
			uint8_t* src = inSrc.mPixels + (inSrcX + (inSrcY + i) * inSrc.mWidth) * inSrc.mChannels;
			memcpy(dest, src, bytesToCopy);
		}
	}

	void Image::ExportImage(const std::string& inExportPath, ExportAs inExportAs, int Quality) const
	{
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
			char* packedField = buffer.data() + 10;

			uint8_t colorResolutionFlag = 0b01110000;
			uint8_t colorTableSizeFlag = 0b00000111;

			mPixels = stbi_load_gif_from_memory((unsigned char*)buffer.c_str(), buffer.size(), &mDelays, &mWidth, &mHeight, &mFrames, &mChannels, mChannels);
			BuildColorTable();
		}
		else
		{
			mPixels = stbi_load(inPath.c_str(), &mWidth, &mHeight, &mChannels, mChannels);
		}
		assert(mPixels != nullptr && "Image failed to load.");
	}
	
	void Image::BuildColorTable()
	{
		mColorTableList.resize(mFrames);
		for (size_t frame = 0; frame < mFrames; frame++)
		{
			int colorsFound = 0;
			for (size_t y = 0; y < mHeight; y++)
			{
				for (size_t x = 0; x < mWidth; x++)
				{
					uint8_t* pixel = mPixels + ((y + frame * mHeight) * mWidth + x) * mChannels;
					const ImageHash hash = sGenerateImageHash(pixel, colorsFound);
					auto it = mColorTableList[frame].find(hash);
					if (it == mColorTableList[frame].end())
					{
						mColorTableList[frame].insert(hash);
						++colorsFound;
					}
				}
			}
		}
	}

	void Image::SetColorTableFromImage(const Image& inOther)
	{
		mColorTableList = inOther.mColorTableList;
	}

	void Image::SaveAnimatedGifToFile(const std::string& inExportPath) const
	{
		std::ofstream outfile(inExportPath, std::ios::binary);
		GifHeader header;
		header.mWidth = mWidth;
		header.mHeight = mHeight;
		outfile << header;
		
		const int maxJobSize = 32;
		std::atomic<int> jobCounter{};
		int jobsLeft = mFrames;
		int jobsStarted{};
		while (jobsLeft > 0)
		{
			std::vector<sul::dynamic_bitset<>> codeStreams;
			codeStreams.resize(std::min(maxJobSize, mFrames));

			for (size_t frame = 0; frame < std::min(maxJobSize, jobsLeft); frame++)
			{
				JobSystem::Execute([&codeStreams, frame, jobsStarted, &header, this]()
				{
					Compress(frame + jobsStarted, codeStreams[frame]);
				}, &jobCounter);
			}

			JobSystem::Wait(&jobCounter);

			for (size_t frame = 0; frame < std::min(maxJobSize, jobsLeft); frame++)
				ProcessFrame(frame + jobsStarted, outfile, codeStreams[frame]);

			jobsStarted += std::min(maxJobSize, jobsLeft);
			jobsLeft -= std::min(maxJobSize, jobsLeft);

			std::cout << jobsLeft << " jobs left out of: " << mFrames << " " << 100.f - (((float)jobsLeft / mFrames) * 100.f) << "% completed\n";
		}
	
		uint8_t terminator = 0x3B;
		outfile.write((char*)&terminator, 1);
		outfile.close();
	}
	
	void Image::WriteImageDescriptor(size_t frame, std::ofstream& outfile) const
	{
		size_t bitsNeeded = sCountBits(mColorTableList[frame].size());
		int entries = static_cast<int>(1 << bitsNeeded);

		ImageDescriptor desc;
		desc.mWidth = mWidth;
		desc.mheight = mHeight;
		desc.mPackedField = 0;
		desc.mPackedField |= 1 << 7;
		desc.mPackedField |= (bitsNeeded - 1);
		outfile << desc;
		
		std::vector<uint32_t> colorsToPrint;
		colorsToPrint.resize(entries);
		for (const auto& it : mColorTableList[frame])
			colorsToPrint[(it.Hash & 0xFF000000) >> 24] = it.Hash;
		
		for (const auto& elem : colorsToPrint)
			outfile << static_cast<uint8_t>((elem & 0x00FF0000) >> 16) << static_cast<uint8_t>((elem & 0x0000FF00) >> 8) << static_cast<uint8_t>((elem & 0x000000FF));
	}

	void Image::Compress(size_t frame, sul::dynamic_bitset<>& inBitField) const
	{
		const size_t lwzMinCodeSize = sCountBits(mColorTableList[frame].size());
		int EOICode = (1 << lwzMinCodeSize) + 1;
		int clearClode = EOICode - 1;
		std::vector<std::vector<int>> initialCodeTable{};
		initialCodeTable.resize(EOICode + 1);
		for (int i = 0 ; i <= EOICode; ++i)
			initialCodeTable[i].push_back({i});

		robin_hood::unordered_map<size_t, int> codeTableMap;
		auto resetCodeTableMap = [&codeTableMap, &initialCodeTable]()
		{
			codeTableMap.clear();
			int index = 0;
			for (const std::vector<int>& codes : initialCodeTable)
			{
				size_t hash = sHash(codes, 0, codes.size());
				codeTableMap[hash] = index++;
			}
		};

		resetCodeTableMap();
		int codeSize = sCountBits(mColorTableList[frame].size()) + 1;
		
		IndexStreamGenerator indexGenerator(*this, frame);
		
		uint8_t currentIndexStreamItem = 0;
		indexGenerator.Next(currentIndexStreamItem);

		std::vector<uint8_t> indexBuffer{ currentIndexStreamItem };
		
		for (int codePos = 0; codePos < codeSize; ++codePos)
			inBitField.push_back(clearClode & (0b1 << codePos));
		
		size_t previousIndexBufferHash = 0;
		while(indexGenerator.Next(currentIndexStreamItem))
		{
			const int K = currentIndexStreamItem;
	
			size_t startOffset = previousIndexBufferHash == 0 ? 0 : indexBuffer.size();
			
			indexBuffer.push_back(K);

			size_t indexBufferPlusKHash = sHash(indexBuffer, startOffset, indexBuffer.size(), previousIndexBufferHash);

			auto indexBufferEntry = codeTableMap.find(indexBufferPlusKHash);
			bool isInCodeTable = indexBufferEntry != codeTableMap.end();
	
			if (!isInCodeTable)
			{
				size_t indexBufferHash = previousIndexBufferHash;
				if (indexBufferHash == 0)
					indexBufferHash = sHash(indexBuffer, 0, indexBuffer.size() - 1);

				auto it = codeTableMap.insert({ indexBufferPlusKHash, (int)codeTableMap.size()});
				
				auto indexBufferEntry = codeTableMap.find(indexBufferHash);
				//if (indexBufferEntry == codeTableMap.end())
				//	codeTableMap[indexBufferHash] = (int)codeTableMap.size();

				assert(indexBufferEntry != codeTableMap.end() && "Entry not found");
				for (int codePos = 0; codePos < codeSize; ++codePos)
					inBitField.push_back(indexBufferEntry->second & (0b1 << codePos));
	
				if (codeTableMap.size() > (1 << codeSize))
					++codeSize;
	
				indexBuffer.clear();
				previousIndexBufferHash = 0;
				indexBuffer.push_back(K);
	
				const int maxCodeSize = 12;
				if (codeSize == maxCodeSize)
				{
					for (int codePos = 0; codePos < codeSize; ++codePos)
						inBitField.push_back(clearClode & (0b1 << codePos));
					codeSize = sCountBits(mColorTableList[frame].size()) + 1;
	
					resetCodeTableMap();
				}
			}
			else
			{
				previousIndexBufferHash = indexBufferPlusKHash;
			}
		}
		
		size_t indexBufferHash = sHash(indexBuffer, 0, indexBuffer.size());
		auto indexBufferEntry = codeTableMap.find(indexBufferHash);
		for (int codePos = 0; codePos < codeSize; ++codePos)
			inBitField.push_back(indexBufferEntry->second & (0b1 << codePos));
	
		for (int codePos = 0; codePos < codeSize; ++codePos)
			inBitField.push_back(EOICode & (0b1 << codePos));
	
		int remainder{ inBitField.size() % 8 };
		for (int i = 0; i < 8 - remainder; ++i)
			inBitField.push_back(0);
	}
	
	void Image::SaveBitField(size_t frame, const sul::dynamic_bitset<>& bitField, std::ofstream& outfile) const
	{
		ImageData data;
		data.mLZW = sCountBits(mColorTableList[frame].size());
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
		} while (remainingSize > 0);
		outfile << data;
	}

	void Image::ProcessFrame(size_t frame, std::ofstream& outfile, const sul::dynamic_bitset<>& inBitfield) const
	{
		WriteApplicationExentionBlock(outfile);
		WriteGraphicControlExtention(frame, outfile);
		WriteImageDescriptor(frame, outfile);
		SaveBitField(frame, inBitfield, outfile);
	}

	void Image::WriteGraphicControlExtention(size_t frame, std::ofstream& outfile) const
	{
		GraphicControlExtention gce;
		gce.mDelay = mDelays[frame] / 10;
		//if (mChannels == 4)
		//{
		//	gce.mPackedField = 0b0000'0001;
		//	gce.mTCI = 1;
		//}

		outfile << gce;
	}

	void Image::WriteApplicationExentionBlock(std::ofstream& outfile) const
	{
		AEB applicationExtentionBlock;
		outfile << applicationExtentionBlock;
	}
}
