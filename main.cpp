// stl
#include <vector>
#include <iostream>

// 3rd-party
#include <OpenImageIO/imageio.h>
#include <OpenImageDenoise/oidn.hpp>

namespace OIIO = OpenImageIO_v3_0;

int main()
{

    std::cout << "[프로그램 시작]" << std::endl;

    const std::string input_filename = "input.hdr";
    const std::string output_filename = "output.hdr";

    // 입력 이미지 열기
    auto input = OIIO::ImageInput::open(input_filename);
    if (!input)
    {
        std::cerr << "입력 파일을 열 수 없습니다: " << input_filename << std::endl;
        ;
        return 1;
    }

    const OIIO::ImageSpec &spec = input->spec();
    const int width = spec.width;
    const int height = spec.height;
    const int channels = spec.nchannels;

    if (channels != 3)
    {
        std::cerr << "3채널(RGB) 이미지만 지원됩니다." << std::endl;
        ;
        return 1;
    }

    std::vector<float> color(width * height * channels);
    if (!input->read_image(0, 0, 0, channels, OIIO::TypeDesc::FLOAT, color.data()))
    {
        std::cerr << "이미지를 읽는 데 실패했습니다." << std::endl;
        ;
        return 1;
    }
    input->close();

    // 출력 버퍼 생성
    // std::vector<float> output(color.size());

    // OIDN 디바이스 생성 및 커밋
    oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CUDA);
    device.commit();

    // 필터 생성 및 설정
    oidn::BufferRef colorBuf = device.newBuffer(width * height * channels * sizeof(float));
    colorBuf.write(0, color.size() * sizeof(float), color.data());
    oidn::FilterRef filter = device.newFilter("RT"); // 일반적인 레이 트레이싱 필터
    filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);
    filter.setImage("output", colorBuf, oidn::Format::Float3, width, height);
    filter.set("hdr", true);
    filter.commit();

    float *colorPtr = (float *)colorBuf.getData();

    // 필터 실행
    filter.execute();

    // 오류 확인
    const char *errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
    {
        std::cerr << "OIDN 오류: " << errorMessage << std::endl;
        ;
        return 1;
    }

    // 결과 이미지 저장
    auto output_file = OIIO::ImageOutput::create(output_filename);
    if (!output_file)
    {
        std::cerr << "출력 파일을 생성할 수 없습니다: " << output_filename << std::endl;
        ;
        return 1;
    }

    OIIO::ImageSpec out_spec(width, height, channels, OIIO::TypeDesc::FLOAT);
    if (!output_file->open(output_filename, out_spec))
    {
        std::cerr << "출력 파일을 여는 데 실패했습니다: " << output_filename << std::endl;
        ;
        return 1;
    }

    if (!output_file->write_image(OIIO::TypeDesc::FLOAT, colorPtr))
    {
        std::cerr << "이미지를 쓰는 데 실패했습니다." << std::endl;
        ;
        return 1;
    }
    output_file->close();

    std::cout << "노이즈 제거된 이미지를 저장했습니다: " << output_filename << std::endl;

    return 0;
}
