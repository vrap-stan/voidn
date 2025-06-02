if exist build-Debug rmdir /s /q build-Debug

cmake -B build-Debug -S . -G Ninja ^
 -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
 -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
 -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDebugDLL" ^
 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"