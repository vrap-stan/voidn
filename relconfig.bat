if exist build-Release rmdir /s /q build-Release

cmake -B build-Release -S . -G Ninja ^
 -DCMAKE_BUILD_TYPE=Release ^
 -DVCPKG_TARGET_TRIPLET=x64-windows ^
 -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
 -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
 -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDLL" ^
 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"

@REM cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DOpenImageIO_DIR=D:\work\voidn\vcpkg_installed\x64-windows\share\openimageio -DVCPKG_TARGET_TRIPLET=x64-windows
