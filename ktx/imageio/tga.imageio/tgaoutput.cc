#include "imageio.h"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <vector>

// Helper to write little-endian values
template <typename T>
static void write_le(std::ostream& os, T value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

class TgaOutput final : public ImageOutput {
  public:
    TgaOutput() : ImageOutput("tga") {}
    virtual ~TgaOutput() { close(); }

    void open(const std::string& name, const ImageSpec& newspec,
              OpenMode /*mode Create only for now*/) override {
        imageSpec = newspec;

        uint32_t channels = imageSpec.format().channelCount();
        uint32_t bitDepth = 0;  // Initialize
        if (!imageSpec.format().samples.empty() && imageSpec.format().sameUnitAllChannels()) {
            bitDepth = imageSpec.format().samples[0].bitLength + 1;
        } else if (!imageSpec.format().samples.empty()) {
            bitDepth = imageSpec.format().largestChannelBitLength();
        } else {
            throw std::runtime_error(
                "TGA Output: Cannot determine channel bit depth from ImageSpec.");
        }

        if (!((channels == 1 && (bitDepth == 8 || bitDepth == 16)) ||
              (channels == 3 && bitDepth == 8) || (channels == 4 && bitDepth == 8))) {
            throw std::runtime_error(
                "TGA Output: Unsupported ImageSpec format. "
                "Supports L8, L16, RGB8, RGBA8.");
        }

        file.open(name, std::ios::binary | std::ios::trunc);
        if (!file) throw std::runtime_error("TGA Output: Failed to open file " + name);

        uint8_t header_arr[18] = {};  // Use an array for direct writing

        header_arr[0] = 0;  // idLength
        header_arr[1] = 0;  // colorMapType (0 = no color map)

        if (channels == 1)
            header_arr[2] = 3;
        else if (channels == 3 || channels == 4)
            header_arr[2] = 2;

        // header_arr[3-7] are already 0 (Color Map Spec)

        // Image Spec - directly write to array then to file
        uint16_t val16;
        val16 = 0;  // xOrigin
        std::memcpy(&header_arr[8], &val16, sizeof(uint16_t));
        val16 = 0;  // yOrigin
        std::memcpy(&header_arr[10], &val16, sizeof(uint16_t));

        val16 = static_cast<uint16_t>(imageSpec.width());
        std::memcpy(&header_arr[12], &val16, sizeof(uint16_t));
        val16 = static_cast<uint16_t>(imageSpec.height());
        std::memcpy(&header_arr[14], &val16, sizeof(uint16_t));

        header_arr[16] = static_cast<uint8_t>(channels * bitDepth);

        uint8_t attributeBits = 0;
        if (channels == 4 && bitDepth == 8) attributeBits = 8;

        // TGA Image Descriptor:
        // Bits 0-3: Number of attribute bits per pixel.
        // Bit 4: Reserved. Must be zero. (Not origin)
        // Bit 5: Screen origin bit. 0 = Origin in lower left-hand corner. 1 = Origin in upper
        // left-hand corner. Bits 6-7: Interleaving flag.
        uint8_t imageDescriptor = attributeBits & 0x0F;
        if (imageSpec.origin().y == ImageSpec::Origin::eTop) {
            imageDescriptor |= (1 << 5);  // Set bit 5 for top-left origin
        }
        header_arr[17] = imageDescriptor;

        file.write(reinterpret_cast<char*>(header_arr), sizeof(header_arr));
        if (file.fail()) throw std::runtime_error("TGA Output: Failed to write header.");
    }

    void writeImage(const FormatDescriptor& format, const void* data, stride_t xstride,
                    stride_t ystride, stride_t /*zstride*/, ProgressCallback /*progress_callback*/,
                    void* /*progress_callback_data*/) override {
        if (!file.is_open()) throw std::runtime_error("TGA Output: File not open.");

        uint32_t width_spec = imageSpec.width();
        uint32_t height_spec = imageSpec.height();
        uint32_t channels_in = format.channelCount();
        uint32_t channels_out = imageSpec.format().channelCount();

        uint32_t bitdepth_in = 0;
        if (!format.samples.empty() && format.sameUnitAllChannels()) {
            bitdepth_in = format.samples[0].bitLength + 1;
        } else if (!format.samples.empty()) {
            bitdepth_in = format.largestChannelBitLength();
        } else {
            throw std::runtime_error(
                "TGA Output: Cannot determine input channel bit depth from FormatDescriptor.");
        }

        uint32_t bitdepth_out = 0;
        if (!imageSpec.format().samples.empty() && imageSpec.format().sameUnitAllChannels()) {
            bitdepth_out = imageSpec.format().samples[0].bitLength + 1;
        } else if (!imageSpec.format().samples.empty()) {
            bitdepth_out = imageSpec.format().largestChannelBitLength();
        } else {
            throw std::runtime_error(
                "TGA Output: Cannot determine output channel bit depth from ImageSpec.");
        }

        if (!((channels_in == 1 && channels_out == 1 && (bitdepth_in == 8 || bitdepth_in == 16) &&
               (bitdepth_out == 8 || bitdepth_out == 16)) ||
              (channels_in == channels_out && bitdepth_in == bitdepth_out))) {
            throw std::runtime_error(
                "TGA Output: Input format does not match TGA spec requirements. "
                "Reformat data before calling writeImage or ensure ImageSpec matches input data "
                "type.");
        }

        size_t bytes_per_pixel_out = (channels_out * bitdepth_out) / 8;
        size_t scanline_bytes_out = width_spec * bytes_per_pixel_out;
        std::vector<uint8_t> scanline_buffer(scanline_bytes_out);

        if (xstride == AutoStride) xstride = (channels_in * bitdepth_in) / 8;
        if (ystride == AutoStride) ystride = width_spec * xstride;

        const uint8_t* p_data = static_cast<const uint8_t*>(data);

        bool tga_is_top_left = (imageSpec.origin().y == ImageSpec::Origin::eTop);

        for (uint32_t y = 0; y < height_spec; ++y) {
            const uint8_t* src_scanline;
            // TGA origin bit (bit 5) refers to how the image data is stored.
            // If TGA is top-left, we write input scanlines 0 to H-1 in order.
            // If TGA is bottom-left, we write input scanlines H-1 to 0 in order to achieve visual
            // top-left. Our ImageSpec.origin reflects the *logical* origin of the data provided to
            // us. For TGA output, the file's on-disk scanline order is determined by its header's
            // origin bit.

            if (tga_is_top_left) {                    // TGA file itself will be top-left
                src_scanline = p_data + y * ystride;  // Read input scanlines normally
            } else {                                  // TGA file itself will be bottom-left
                src_scanline =
                    p_data + (height_spec - 1 - y) *
                                 ystride;  // Read input scanlines in reverse for bottom-left TGA
            }

            uint8_t* dst_scanline_ptr = scanline_buffer.data();

            if (channels_out == 1) {
                if (bitdepth_in == 8 && bitdepth_out == 16) {
                    uint16_t* dst16 = reinterpret_cast<uint16_t*>(dst_scanline_ptr);
                    for (uint32_t x = 0; x < width_spec; ++x) {
                        uint8_t val = *(src_scanline + x * xstride);
                        dst16[x] = static_cast<uint16_t>(val) << 8 | val;
                    }
                } else if (bitdepth_in == 16 && bitdepth_out == 8) {
                    const uint16_t* src16 = reinterpret_cast<const uint16_t*>(src_scanline);
                    for (uint32_t x = 0; x < width_spec; ++x) {
                        // Assuming xstride for 16-bit is per pixel (2 bytes)
                        dst_scanline_ptr[x] = static_cast<uint8_t>(src16[x * (xstride / 2)] >> 8);
                    }
                } else {
                    std::memcpy(dst_scanline_ptr, src_scanline, scanline_bytes_out);
                }
            } else if (channels_out == 3) {
                for (uint32_t x = 0; x < width_spec; ++x) {
                    const uint8_t* _src_pixel = src_scanline + x * xstride;
                    uint8_t* _dst_pixel = dst_scanline_ptr + x * bytes_per_pixel_out;
                    _dst_pixel[0] = _src_pixel[2];  // B
                    _dst_pixel[1] = _src_pixel[1];  // G
                    _dst_pixel[2] = _src_pixel[0];  // R
                }
            } else if (channels_out == 4) {
                for (uint32_t x = 0; x < width_spec; ++x) {
                    const uint8_t* _src_pixel = src_scanline + x * xstride;
                    uint8_t* _dst_pixel = dst_scanline_ptr + x * bytes_per_pixel_out;
                    _dst_pixel[0] = _src_pixel[2];  // B
                    _dst_pixel[1] = _src_pixel[1];  // G
                    _dst_pixel[2] = _src_pixel[0];  // R
                    _dst_pixel[3] = _src_pixel[3];  // A
                }
            }

            file.write(reinterpret_cast<const char*>(scanline_buffer.data()), scanline_bytes_out);
            if (file.fail()) throw std::runtime_error("TGA Output: Failed to write scanline data.");
        }
    }

    void close() override {
        if (file.is_open()) {
            file.flush();
            file.close();
        }
    }

  private:
    std::ofstream file;
};

ImageOutput* tgaOutputCreate() { return new TgaOutput; }
const char* tgaOutputExtensions[] = {"tga", "vda", "icb", "vst", nullptr};