#include "imageio.h"
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <fmt/core.h>
#include <array>
#include <algorithm>  // For std::reverse

// Helper to read little-endian values
template <typename T>
static T read_le(std::istream& is) {
    T value;
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

class TgaInput final : public ImageInput {
  public:
    TgaInput() : ImageInput("tga") {}
    virtual ~TgaInput() { close(); }

    void open(ImageSpec& newspec) override {
        if (!isp) throw std::runtime_error("TGA Input: stream is not open");

        idLength = read_le<uint8_t>(*isp);
        colorMapType = read_le<uint8_t>(*isp);
        imageType = read_le<uint8_t>(*isp);

        cmFirstEntryIndex = read_le<uint16_t>(*isp);
        cmLength = read_le<uint16_t>(*isp);
        cmEntrySize = read_le<uint8_t>(*isp);

        uint16_t xOrigin_file = read_le<uint16_t>(*isp);
        (void)xOrigin_file;
        uint16_t yOrigin_file = read_le<uint16_t>(*isp);
        (void)yOrigin_file;
        width = read_le<uint16_t>(*isp);
        height = read_le<uint16_t>(*isp);
        pixelDepth = read_le<uint8_t>(*isp);
        uint8_t imageDescriptor = read_le<uint8_t>(*isp);

        if (isp->fail() || isp->tellg() < 18) {
            throw std::runtime_error("TGA Input: Failed to read full header");
        }

        attributeBits = imageDescriptor & 0x0F;
        bool originIsTopLeft = (imageDescriptor & 0x20) != 0;

        if (((imageDescriptor & 0xC0) >> 6) != 0) {
            fwarning("TGA Input: Interleaved TGA files are not fully supported.");
        }
        if ((imageDescriptor & 0x10) != 0) {
            fwarning("TGA Input: Image Descriptor bit 4 is set (reserved). Should be 0.");
        }

        if (idLength > 0) {
            isp->seekg(idLength, std::ios::cur);
            if (isp->fail()) throw std::runtime_error("TGA Input: Failed to skip Image ID field");
        }

        bool isSupportedType = false;
        switch (imageType) {
        case 1:
        case 9:
            if (this->colorMapType == 1 &&
                (cmEntrySize == 15 || cmEntrySize == 16 || cmEntrySize == 24 ||
                 cmEntrySize == 32) &&
                pixelDepth == 8) {
                isSupportedType = true;
            }
            break;
        case 2:
        case 10:
            if (this->colorMapType == 0 &&
                (pixelDepth == 15 || pixelDepth == 16 || pixelDepth == 24 || pixelDepth == 32)) {
                isSupportedType = true;
            }
            break;
        case 3:
        case 11:
            if (this->colorMapType == 0 && (pixelDepth == 8 || pixelDepth == 16)) {
                isSupportedType = true;
            }
            break;
        }

        if (!isSupportedType) {
            throw std::runtime_error(fmt::format(
                "TGA Input: Unsupported combination: imageType {}, colorMapType {}, pixelDepth {}, "
                "cmEntrySize {}.",
                (int)imageType, (int)this->colorMapType, (int)pixelDepth, (int)cmEntrySize));
        }

        if (this->colorMapType == 1) {
            loadColorMap();  // Call to member function
        }

        uint32_t specChannelCount = 0;
        uint32_t specChannelBitDepth = 0;

        if (imageType == 3 || imageType == 11) {
            specChannelCount = 1;
            specChannelBitDepth = this->pixelDepth;  // Use this->pixelDepth
        } else if (imageType == 1 || imageType == 9) {
            specChannelBitDepth = 8;
            switch (cmEntrySize) {
            case 15:
                specChannelCount = 4;
                break;
            case 16:
                specChannelCount = (this->attributeBits == 0) ? 3 : 4;  // Use this->attributeBits
                break;
            case 24:
                specChannelCount = 3;
                break;
            case 32:
                specChannelCount = 4;
                break;
            default:
                throw std::logic_error("TGA Input: Unhandled cmEntrySize in spec setup.");
            }
        } else if (imageType == 2 || imageType == 10) {
            specChannelBitDepth = 8;
            switch (this->pixelDepth) {  // Use this->pixelDepth
            case 15:
                specChannelCount = 4;
                break;
            case 16:
                specChannelCount = (this->attributeBits == 0) ? 3 : 4;  // Use this->attributeBits
                break;
            case 24:
                specChannelCount = 3;
                break;
            case 32:
                specChannelCount = 4;
                break;
            default:
                throw std::logic_error("TGA Input: Unhandled pixelDepth in spec setup.");
            }
        } else {
            throw std::logic_error("TGA Input: Unhandled imageType in spec setup.");
        }

        if (specChannelCount == 0) {
            throw std::runtime_error(
                fmt::format("TGA Input::open FATAL: specChannelCount is 0! imageType: {}, "
                            "pixelDepth: {}, cmEntrySize: {}, attributeBits: {}, colorMapType: {}",
                            this->imageType, this->pixelDepth, this->cmEntrySize,
                            this->attributeBits, this->colorMapType));
        }
        if (specChannelBitDepth == 0) {
            throw std::runtime_error(
                fmt::format("TGA Input::open FATAL: specChannelBitDepth is 0! imageType: {}, "
                            "pixelDepth: {}, cmEntrySize: {}, attributeBits: {}, colorMapType: {}",
                            this->imageType, this->pixelDepth, this->cmEntrySize,
                            this->attributeBits, this->colorMapType));
        }

        outputPixelBytes = specChannelCount * (specChannelBitDepth / 8);

        if (outputPixelBytes == 0 && (this->width > 0 || this->height > 0)) {
            throw std::runtime_error(
                fmt::format("TGA Input::open FATAL: outputPixelBytes is 0 for non-empty image! "
                            "specChannelCount: {}, specChannelBitDepth: {}",
                            specChannelCount, specChannelBitDepth));
        }

        ImageSpec::Origin imgOrigin_spec(
            ImageSpec::Origin::eLeft,
            originIsTopLeft ? ImageSpec::Origin::eTop : ImageSpec::Origin::eBottom,
            ImageSpec::Origin::eFront);

        images.emplace_back(
            ImageSpec(this->width, this->height, 1U, std::move(imgOrigin_spec), specChannelCount,
                      specChannelBitDepth, static_cast<khr_df_sample_datatype_qualifiers_e>(0),
                      KHR_DF_TRANSFER_SRGB, KHR_DF_PRIMARIES_SRGB,
                      (specChannelCount == 1 ? KHR_DF_MODEL_YUVSDA : KHR_DF_MODEL_RGBSDA),
                      KHR_DF_FLAG_ALPHA_STRAIGHT),
            ImageInputFormatType::tga);

        if (!images.empty()) {
            this->curSubimage = 0;
            this->curMiplevel = 0;
        } else {
            this->curSubimage = std::numeric_limits<uint32_t>::max();
            this->curMiplevel = std::numeric_limits<uint32_t>::max();
        }

        newspec = spec();

        if (this->imageType == 9 || this->imageType == 10 || this->imageType == 11) {
            size_t rawImageSize =
                static_cast<size_t>(this->width) * this->height * (this->pixelDepth / 8);
            if (rawImageSize == 0 && (this->width > 0 || this->height > 0)) {
                throw std::runtime_error(
                    fmt::format("TGA Input::open FATAL: rawImageSize for RLE is 0 but dimensions "
                                "are not. pixelDepth: {}",
                                this->pixelDepth));
            }
            if (rawImageSize > 0) {
                rawRleBuffer.resize(rawImageSize);
                decodeRLE(rawRleBuffer.data(), rawImageSize);  // Call to member function
            } else if (this->width > 0 && this->height > 0) {
                fwarning(
                    "TGA Input: RLE image has zero raw size despite non-zero dimensions. Skipping "
                    "RLE decode.");
            }
        }
    }

    bool seekSubimage(uint32_t subimage, uint32_t miplevel) override {
        if (images.empty()) {
            curSubimage = std::numeric_limits<uint32_t>::max();
            curMiplevel = std::numeric_limits<uint32_t>::max();
            return false;
        }
        if (subimage == 0 && miplevel == 0) {
            curSubimage = 0;
            curMiplevel = 0;
            return true;
        }
        curSubimage = std::numeric_limits<uint32_t>::max();
        curMiplevel = std::numeric_limits<uint32_t>::max();
        return false;
    }

    void readImage(void* bufferOut, size_t bufferByteCount, uint32_t subimage, uint32_t miplevel,
                   const FormatDescriptor& destFormatDesc) override {
        const ImageSpec& tgaFileSpec = spec(subimage, miplevel);

        uint32_t tga_width = tgaFileSpec.width();
        uint32_t tga_height = tgaFileSpec.height();
        uint32_t tga_channels = tgaFileSpec.format().channelCount();
        uint32_t tga_bytes_per_pixel = tgaFileSpec.format().pixelByteCount();

        uint32_t dest_channels = destFormatDesc.channelCount();
        uint32_t dest_bytes_per_pixel = destFormatDesc.pixelByteCount();

        size_t required_buffer_size =
            static_cast<size_t>(tga_width) * tga_height * dest_bytes_per_pixel;
        if (bufferByteCount < required_buffer_size) {
            throw std::runtime_error(
                fmt::format("TGA Input::readImage: Buffer too small. Provided: {}, Required: {}",
                            bufferByteCount, required_buffer_size));
        }
        if (required_buffer_size == 0 && (tga_width > 0 || tga_height > 0)) return;

        uint8_t* out_current_row_ptr = static_cast<uint8_t*>(bufferOut);
        std::vector<uint8_t> tgaScanlineBuffer(tgaFileSpec.scanlineByteCount());

        for (uint32_t y = 0; y < tga_height; ++y) {
            if (tgaScanlineBuffer.empty() && tga_width > 0) {
                throw std::runtime_error(
                    "TGA Input::readImage: tgaScanlineBuffer is empty for non-zero width image.");
            }
            if (!tgaScanlineBuffer.empty()) {
                readNativeScanline(tgaScanlineBuffer.data(), tgaScanlineBuffer.size(), y, 0,
                                   subimage, miplevel);
            }

            uint8_t* dest_pixel_ptr = out_current_row_ptr;
            const uint8_t* src_pixel_ptr = tgaScanlineBuffer.data();

            for (uint32_t x = 0; x < tga_width; ++x) {
                if (tga_channels == 3 && dest_channels == 4) {
                    dest_pixel_ptr[0] = src_pixel_ptr[0];
                    dest_pixel_ptr[1] = src_pixel_ptr[1];
                    dest_pixel_ptr[2] = src_pixel_ptr[2];
                    dest_pixel_ptr[3] = 255;
                } else if (tga_channels == 1 && dest_channels == 4) {
                    dest_pixel_ptr[0] = src_pixel_ptr[0];
                    dest_pixel_ptr[1] = src_pixel_ptr[0];
                    dest_pixel_ptr[2] = src_pixel_ptr[0];
                    dest_pixel_ptr[3] = 255;
                } else if (tga_channels == 1 && dest_channels == 3) {
                    dest_pixel_ptr[0] = src_pixel_ptr[0];
                    dest_pixel_ptr[1] = src_pixel_ptr[0];
                    dest_pixel_ptr[2] = src_pixel_ptr[0];
                } else if (tga_channels == tga_bytes_per_pixel &&
                           dest_channels == dest_bytes_per_pixel && tga_channels == dest_channels) {
                    std::memcpy(dest_pixel_ptr, src_pixel_ptr, tga_bytes_per_pixel);
                } else {
                    size_t bytes_to_copy = std::min(tga_bytes_per_pixel, dest_bytes_per_pixel);
                    std::memcpy(dest_pixel_ptr, src_pixel_ptr, bytes_to_copy);
                    if (dest_bytes_per_pixel > bytes_to_copy) {
                        std::memset(dest_pixel_ptr + bytes_to_copy, 0,
                                    dest_bytes_per_pixel - bytes_to_copy);
                        if (dest_channels == 4 && dest_bytes_per_pixel - bytes_to_copy >= 1 &&
                            bytes_to_copy < 4) {
                            dest_pixel_ptr[3] = 255;
                        }
                    }
                }
                dest_pixel_ptr += dest_bytes_per_pixel;
                src_pixel_ptr += tga_bytes_per_pixel;
            }
            out_current_row_ptr += static_cast<size_t>(tga_width) * dest_bytes_per_pixel;
        }
    }

    void readNativeScanline(void* buffer, size_t bufferByteCount, uint32_t y_target, uint32_t /*z*/,
                            uint32_t subimage, uint32_t miplevel) override {
        if (buffer == nullptr && bufferByteCount > 0) {
            throw std::runtime_error(
                "TGA Input::readNativeScanline FATAL: input buffer is nullptr but bufferByteCount "
                "> 0!");
        }

        const ImageSpec& currentSpec = spec(subimage, miplevel);
        const size_t outputScanlineBytesForThisFunc = currentSpec.scanlineByteCount();

        if (outputScanlineBytesForThisFunc == 0 && currentSpec.width() > 0) {
            throw std::runtime_error(
                fmt::format("TGA Input::readNativeScanline FATAL: currentSpec.scanlineByteCount() "
                            "is 0! Width: {}",
                            currentSpec.width()));
        }
        if (bufferByteCount < outputScanlineBytesForThisFunc)
            throw std::runtime_error(
                fmt::format("TGA Input: Scanline buffer too small. Provided: {}, Required: {}",
                            bufferByteCount, outputScanlineBytesForThisFunc));

        if (outputScanlineBytesForThisFunc == 0) return;

        uint32_t y_file = y_target;
        if (currentSpec.origin().y == ImageSpec::Origin::eBottom) {
            y_file = this->height - 1 - y_target;  // Use member height
        }

        const size_t rawPixelBytesInFile = this->pixelDepth / 8;  // Use member pixelDepth
        if (rawPixelBytesInFile == 0 && this->width > 0) {        // Use member width
            throw std::runtime_error(
                fmt::format("TGA Input::readNativeScanline FATAL: rawPixelBytesInFile is 0 but "
                            "width > 0. pixelDepth: {}",
                            this->pixelDepth));
        }
        std::vector<uint8_t> rawScanlineDataVec;
        if (this->width * rawPixelBytesInFile > 0) {
            rawScanlineDataVec.resize(this->width * rawPixelBytesInFile);
        }

        if (this->imageType == 9 || this->imageType == 10 ||
            this->imageType == 11) {  // Use member imageType
            if (this->rawRleBuffer.empty() &&
                static_cast<size_t>(this->width) * this->height * rawPixelBytesInFile > 0)
                throw std::runtime_error(
                    "TGA Input: RLE buffer not decoded or empty for non-zero size image.");
            if (!this->rawRleBuffer.empty()) {
                size_t offset = static_cast<size_t>(y_file) * this->width * rawPixelBytesInFile;
                if (offset + this->width * rawPixelBytesInFile > this->rawRleBuffer.size()) {
                    throw std::runtime_error(
                        fmt::format("TGA Input: RLE read offset out of bounds. y_file: {}, offset: "
                                    "{}, rleBufferSize: {}",
                                    y_file, offset, this->rawRleBuffer.size()));
                }
                if (!rawScanlineDataVec.empty())
                    std::memcpy(rawScanlineDataVec.data(), &this->rawRleBuffer[offset],
                                this->width * rawPixelBytesInFile);
            } else if (this->width * rawPixelBytesInFile > 0) {
                throw std::runtime_error(
                    "TGA Input: Attempting to read from empty RLE buffer for a non-zero size "
                    "scanline.");
            }
        } else {
            if (this->width * rawPixelBytesInFile > 0) {
                size_t dataStartOffset =
                    18 + this->idLength +
                    (this->colorMapType == 1
                         ? static_cast<size_t>(this->cmLength) * (this->cmEntrySize / 8)
                         : 0);
                size_t current_seek_pos = dataStartOffset + static_cast<size_t>(y_file) *
                                                                this->width * rawPixelBytesInFile;
                isp->seekg(current_seek_pos, std::ios::beg);
                if (isp->fail()) {
                    throw std::runtime_error("TGA Input: Seek failed for scanline data.");
                }
                isp->read(reinterpret_cast<char*>(rawScanlineDataVec.data()),
                          this->width * rawPixelBytesInFile);
                if (isp->gcount() !=
                    static_cast<std::streamsize>(this->width * rawPixelBytesInFile)) {
                    throw std::runtime_error("TGA Input: Failed to read scanline data");
                }
            }
        }
        processRawScanline(rawScanlineDataVec.data(), static_cast<uint8_t*>(buffer), this->width,
                           y_target);  // Call to member
    }

    void close() override {
        ImageInput::close();
        colorMap.clear();
        rawRleBuffer.clear();
    }

  private:
    // Member variable declarations
    uint8_t idLength = 0;
    uint8_t colorMapType = 0;
    uint8_t imageType = 0;
    uint16_t cmFirstEntryIndex = 0;
    uint16_t cmLength = 0;
    uint8_t cmEntrySize = 0;
    uint32_t width = 0, height = 0;
    uint8_t pixelDepth = 0;
    uint8_t attributeBits = 0;
    size_t outputPixelBytes = 0;

    std::vector<std::array<uint8_t, 4>> colorMap;
    std::vector<uint8_t> rawRleBuffer;

    // DECLARATIONS of private helper methods
    void loadColorMap();
    void decodeRLE(uint8_t* buffer, size_t bufferSize);
    void processRawScanline(const uint8_t* rawScanlineData, uint8_t* outScanline,
                            uint32_t numPixels, uint32_t y_target_from_readNativeScanline);
};

void TgaInput::loadColorMap() {
    if (this->cmLength == 0) return;
    this->colorMap.resize(this->cmLength);
    size_t entryBytes = this->cmEntrySize / 8;
    if (entryBytes == 0)
        throw std::runtime_error("TGA Input: cmEntrySize is invalid (results in 0 bytes).");

    std::vector<uint8_t> entryBuffer(entryBytes);
    size_t colorMapFileOffset = 18 + this->idLength;
    this->isp->seekg(colorMapFileOffset, std::ios::beg);
    if (this->isp->fail()) throw std::runtime_error("TGA Input: Failed to seek to color map.");

    for (uint16_t i = 0; i < this->cmLength; ++i) {
        this->isp->read(reinterpret_cast<char*>(entryBuffer.data()), entryBytes);
        if (this->isp->gcount() != static_cast<std::streamsize>(entryBytes))
            throw std::runtime_error(
                fmt::format("TGA Input: Failed to read color map entry {}/{}.", i, this->cmLength));

        uint8_t r = 0, g = 0, b = 0, a = 255;
        if (this->cmEntrySize == 15 || this->cmEntrySize == 16) {
            uint16_t pixel16 = *reinterpret_cast<uint16_t*>(entryBuffer.data());
            if (this->cmEntrySize == 15 || (this->cmEntrySize == 16 && this->attributeBits >= 1)) {
                b = (pixel16 & 0x001F);
                g = ((pixel16 & 0x03E0) >> 5);
                r = ((pixel16 & 0x7C00) >> 10);
                if (this->cmEntrySize == 16)
                    a = (pixel16 & 0x8000) ? 255 : 0;
                else
                    a = 255;

                r = (r << 3) | (r >> 2);
                g = (g << 3) | (g >> 2);
                b = (b << 3) | (b >> 2);
            } else {
                b = (pixel16 & 0x001F);
                g = ((pixel16 & 0x07E0) >> 5);
                r = ((pixel16 & 0xF800) >> 11);
                r = (r << 3) | (r >> 2);
                g = (g << 2) | (g >> 4);
                b = (b << 3) | (b >> 2);
                a = 255;
            }
        } else if (this->cmEntrySize == 24) {
            b = entryBuffer[0];
            g = entryBuffer[1];
            r = entryBuffer[2];
        } else if (this->cmEntrySize == 32) {
            b = entryBuffer[0];
            g = entryBuffer[1];
            r = entryBuffer[2];
            a = entryBuffer[3];
        }
        this->colorMap[i] = {r, g, b, a};
    }
}

void TgaInput::decodeRLE(uint8_t* buffer, size_t bufferSize) {
    if (bufferSize == 0) return;
    size_t pixelBytesInFile = this->pixelDepth / 8;
    if (pixelBytesInFile == 0)
        throw std::runtime_error("TGA Input: pixelDepth is 0, cannot decode RLE.");
    size_t outOffset = 0;

    while (outOffset < bufferSize) {
        if (outOffset > bufferSize) {
            throw std::runtime_error(
                fmt::format("TGA Input: RLE decoding offset {} exceeds buffer size {}.", outOffset,
                            bufferSize));
        }

        uint8_t packetHeader;
        this->isp->read(reinterpret_cast<char*>(&packetHeader), 1);
        if (this->isp->fail()) throw std::runtime_error("TGA Input: RLE packet header read error");

        uint8_t count = (packetHeader & 0x7F) + 1;

        if (packetHeader & 0x80) {
            std::vector<uint8_t> pixelData(pixelBytesInFile);
            this->isp->read(reinterpret_cast<char*>(pixelData.data()), pixelBytesInFile);
            if (this->isp->gcount() != static_cast<std::streamsize>(pixelBytesInFile))
                throw std::runtime_error("TGA Input: RLE pixel data read error");

            for (uint8_t i = 0; i < count; ++i) {
                if (outOffset + pixelBytesInFile > bufferSize)
                    throw std::runtime_error("TGA Input: RLE run packet overflow");
                std::memcpy(&buffer[outOffset], pixelData.data(), pixelBytesInFile);
                outOffset += pixelBytesInFile;
            }
        } else {
            size_t bytesToRead = pixelBytesInFile * count;
            if (outOffset + bytesToRead > bufferSize)
                throw std::runtime_error("TGA Input: RLE raw packet overflow");
            this->isp->read(reinterpret_cast<char*>(&buffer[outOffset]), bytesToRead);
            if (this->isp->gcount() != static_cast<std::streamsize>(bytesToRead))
                throw std::runtime_error("TGA Input: RLE raw data read error");
            outOffset += bytesToRead;
        }
    }
    if (outOffset != bufferSize) {
        fwarning(
            fmt::format("TGA Input: RLE decoded size {} does not match expected image size {}.",
                        outOffset, bufferSize));
    }
}

void TgaInput::processRawScanline(const uint8_t* rawScanlineData, uint8_t* outScanline,
                                  uint32_t numPixels, uint32_t y_target_from_readNativeScanline) {
    if (outScanline == nullptr && numPixels > 0) {
        throw std::runtime_error(
            "TGA Input::processRawScanline FATAL: outScanline is nullptr with numPixels > 0!");
    }
    if (numPixels == 0) return;

    const uint8_t* src = rawScanlineData;
    uint8_t* dst = outScanline;
    const ImageSpec& currentTgaSpec = spec();

    uint32_t tga_spec_channel_bit_length = 0;
    if (!currentTgaSpec.format().samples.empty() && currentTgaSpec.format().sameUnitAllChannels()) {
        tga_spec_channel_bit_length = currentTgaSpec.format().samples[0].bitLength + 1;
    } else if (!currentTgaSpec.format().samples.empty()) {
        tga_spec_channel_bit_length = currentTgaSpec.format().largestChannelBitLength();
    } else {
        throw std::runtime_error(
            "TGA Input (processRawScanline): Could not determine channel bit length from ImageSpec "
            "for processing.");
    }

    // bool first_pixel_processed_debug = (y_target_from_readNativeScanline != 0); // Uncomment for
    // debug

    for (uint32_t px_idx = 0; px_idx < numPixels; ++px_idx) {
        // const uint8_t* src_before_type_switch = src; // Uncomment for debug
        uint8_t r_out = 0, g_out = 0, b_out = 0, a_out = 255;
        uint8_t index_val_debug = 0;

        if (this->imageType == 1 || this->imageType == 9) {
            index_val_debug = *src;
            uint16_t actualIndex = index_val_debug;
            if (this->cmLength > 0 && actualIndex >= this->cmFirstEntryIndex &&
                (actualIndex - this->cmFirstEntryIndex) < this->cmLength) {
                const auto& cmapEntry = this->colorMap[actualIndex - this->cmFirstEntryIndex];
                r_out = cmapEntry[0];
                g_out = cmapEntry[1];
                b_out = cmapEntry[2];
                a_out = cmapEntry[3];
            } else if (this->cmLength > 0) {
                fwarning(fmt::format("TGA Input: Color map index {} out of range [{}, {}]",
                                     actualIndex, this->cmFirstEntryIndex,
                                     this->cmFirstEntryIndex + this->cmLength - 1));
                r_out = g_out = b_out = 0;
                a_out = 255;
            } else {
                r_out = g_out = b_out = index_val_debug;
                a_out = 255;
            }
            src += 1;
        } else if (this->imageType == 3 || this->imageType == 11) {
            if (this->pixelDepth == 8) {
                r_out = g_out = b_out = *src;
                src += 1;
            } else {
                uint16_t gray16 = *reinterpret_cast<const uint16_t*>(src);
                // Using khr_df_model_channels_e(0) as a placeholder for "the first/only channel"
                if (currentTgaSpec.format().channelBitLength(
                        static_cast<khr_df_model_channels_e>(0)) == 16) {
                    uint16_t* dst16 = reinterpret_cast<uint16_t*>(dst);
                    *dst16 = gray16;
                    dst += 2;
                    src += 2;
                    continue;
                } else {
                    r_out = g_out = b_out = static_cast<uint8_t>(gray16 >> 8);
                }
                src += 2;
            }
        } else {
            if (this->pixelDepth == 15 || this->pixelDepth == 16) {
                uint16_t pixel16 = *reinterpret_cast<const uint16_t*>(src);
                uint8_t r_file, g_file, b_file;
                if (this->pixelDepth == 15 ||
                    (this->pixelDepth == 16 && this->attributeBits >= 1)) {
                    b_file = (pixel16 & 0x001F);
                    g_file = (pixel16 & 0x03E0) >> 5;
                    r_file = (pixel16 & 0x7C00) >> 10;
                    if (this->pixelDepth == 16) {
                        a_out = (pixel16 & 0x8000) ? 255 : 0;
                    } else {
                        a_out = 255;
                    }
                    r_out = (r_file << 3) | (r_file >> 2);
                    g_out = (g_file << 3) | (g_file >> 2);
                    b_out = (b_file << 3) | (b_file >> 2);
                } else {
                    b_file = (pixel16 & 0x001F);
                    g_file = (pixel16 & 0x07E0) >> 5;
                    r_file = (pixel16 & 0xF800) >> 11;
                    r_out = (r_file << 3) | (r_file >> 2);
                    g_out = (g_file << 2) | (g_file >> 4);
                    b_out = (b_file << 3) | (b_file >> 2);
                    a_out = 255;
                }
                src += 2;
            } else if (this->pixelDepth == 24) {
                uint8_t b_file = src[0];
                uint8_t g_file = src[1];
                uint8_t r_file = src[2];
                r_out = r_file;
                g_out = g_file;
                b_out = b_file;
                a_out = 255;
                src += 3;
            } else if (this->pixelDepth == 32) {
                uint8_t b_file = src[0];
                uint8_t g_file = src[1];
                uint8_t r_file = src[2];
                uint8_t a_file = src[3];
                r_out = r_file;
                g_out = g_file;
                b_out = b_file;
                a_out = a_file;
                if (this->attributeBits == 0) {
                    a_out = 255;
                }
                src += 4;
            }
        }

        if (currentTgaSpec.format().channelCount() == 1) {
            *dst++ = r_out;
        } else if (currentTgaSpec.format().channelCount() == 3) {
            *dst++ = r_out;
            *dst++ = g_out;
            *dst++ = b_out;
        } else if (currentTgaSpec.format().channelCount() == 4) {
            *dst++ = r_out;
            *dst++ = g_out;
            *dst++ = b_out;
            *dst++ = a_out;
        }
    }
}

ImageInput* tgaInputCreate() { return new TgaInput; }
const char* tgaInputExtensions[] = {"tga", "vda", "icb", "vst", nullptr};