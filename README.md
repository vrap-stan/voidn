# VCPP

VRAPOINT C++ 테스트 프로젝트

### - 2025

#### 05.26

KTX 직접 빌드 (ktx.exe를 함수형태로 사용할 수 있도록 ktxdll 빌드) 후 연결

#### 05.22

인텔 OpenImageDenoise 연결
구글 드라코 연결 (Autodesk FBX 연결)

---

#### 인텔 oidn 빌드 및 환경 구성

thirdparty/oidn에 oidn빌드파일들이 들어있어야 함
예시:

> thirdparty/oidn/bin/OpenImageDenoise_core.dll, ...
> thirdparty/oidn/include/OpenImageDenoise/oidn.h, ...
> thirdparty/oidn/lib/OpenImageDenoise_core.lib, ...

설정대로라면 debug_configure.bat 실행 후 f5실행 시 다음이 실행되어야 함

> 1.  파일 빌드
> 2.  디버거 어태치되어 main.cpp에서 브레이크포인트 실행 가능
>
> - 빌드는 clang++, 디버깅은 gdb를 이용. OIIO가 vcpkg를 통해 설치되어 있어야하고, 환경변수에 VCPKG_ROOT이 해당 폴더로 연결되어있어야 cmake 가능

빌드 후 실행을 위해서 build 폳더에 다음 파일들을 넣어주어야 함 (OpenImageDenoise를 스태틱빌드하면 필요없긴 할텐데)

> OpenImageDenoise.dll
> OpenImageDenoise_core.dll
> OpenImageDenoise_device_cpu.dll
> OpenImageDenoise_device_cuda.dll

#### 기억나는 대로 적어보는 필요한 프로그램들

- OIDN : OIDN 빌드과정 참조
- MinGW 중 g++, 경로는 기본 (C:/MinGW) : 디버깅 위한 gdb
- 디버그빌드(F5)는 cmake, ninja, clang으로 진행
- 릴리즈는 VS로 진행되더라 (cd build && cmake .. && cmake --build . --config Release만 실행)
- Autodesk FBX SDK를 설치해야함
- [KTX프로그램을 따로 빌드](https://github.com/vrap-stan/ktxdll), 함수 형태로 사용가능하도록 변경.

* KTX에서 필요한 다음 파일들 링크
  > astcenc-avx2-static.lib
  > fmt.lib // ktx는 v10, OIIO는 v11을 사용하므로 이름을 fmtv10.lib으로 변경하여 가져옴
  > ktxdll.lib
  > imageio.lib

- KTX내부적으로 Pathcch 사용함 (윈도우에서 경로 파싱)
