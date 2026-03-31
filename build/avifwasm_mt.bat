@echo off

call "C:\Users\%USERNAME%\emsdk\emsdk_env.bat"

if not exist aom (
	git clone https://aomedia.googlesource.com/aom --depth 1
	cd aom
) else (
	cd aom
	git pull
)

mkdir build-aom
cd build-aom
call emcmake cmake ..\ ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DAOM_TARGET_CPU=generic ^
  -DCONFIG_RUNTIME_CPU_DETECT=0 ^
  -DENABLE_NASM=0 ^
  -DENABLE_DOCS=0 ^
  -DENABLE_TESTS=0 ^
  -DENABLE_EXAMPLES=0 ^
  -DENABLE_TOOLS=0 ^
  -DCONFIG_WEBM_IO=0 ^
  -DCONFIG_AV1_ENCODER=1 ^
  -DCONFIG_AV1_DECODER=0 ^
  -DCMAKE_CXX_FLAGS="-pthread -msimd128" ^
  -DCMAKE_C_FLAGS="-pthread -msimd128"



cmake --build . --parallel 4 --config Release
cmake --install . --config Release --prefix ../aom-wasm-install

cd ..\..\
if not exist libavif (
	git clone https://github.com/AOMediaCodec/libavif.git --depth 1
	cd libavif
) else (
	cd libavif
	git pull
)

mkdir build-avif
cd build-avif

call emcmake cmake .. ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_FLAGS="-pthread -msimd128" ^
  -DCMAKE_CXX_FLAGS="-pthread -msimd128" ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DAVIF_BUILD_APPS=OFF -DAVIF_BUILD_TESTS=OFF -DAVIF_BUILD_EXAMPLES=OFF ^
  -DAVIF_CODEC_AOM=SYSTEM -DAVIF_CODEC_DAV1D=OFF -DAVIF_LIBYUV=OFF ^
  -DCMAKE_SKIP_INSTALL_RULES=ON ^
  -DCMAKE_FIND_ROOT_PATH="../../aom/aom-wasm-install" ^
  -Daom_DIR="../../aom/aom-wasm-install/lib/cmake/aom" ^
  -DAOM_INCLUDE_DIR="../../aom/aom-wasm-install/include" ^
  -DAOM_LIBRARY="../../aom/aom-wasm-install/lib/libaom.a"

cmake --build . --parallel 4 --config Release
copy ..\..\avif_wasm.c .\

call emcc ^
  avif_wasm.c ^
  -I ..\include ^
  libavif.a ^
  libavif_internal.a ^
  C:\Users\Sputchik\aom\aom-wasm-install\lib\libaom.a ^
  -O3 -pthread -msimd128 ^
  -s WASM=1 ^
  -s USE_PTHREADS=1 ^
  -s PTHREAD_POOL_SIZE=8 ^
  -s STACK_SIZE=1048576 ^
  -s DEFAULT_PTHREAD_STACK_SIZE=1048576 ^
  -s MODULARIZE=1 ^
  -s EXPORT_NAME="createAvifModuleMt" ^
  -s ALLOW_MEMORY_GROWTH=1 ^
  -s EXPORTED_FUNCTIONS="['_avif_encode_rgba_ex','_avif_free_buffer','_malloc','_free']" ^
  -s EXPORTED_RUNTIME_METHODS="['cwrap','HEAPU8','HEAPU32']" ^
  -o avif_wasm_mt.js

copy avif_wasm_mt.js ..\..\avif_wasm_mt.js
copy avif_wasm_mt.wasm ..\..\avif_wasm_mt.wasm

cd "%~dp0"