echo compileBLAS.cmd started
rem set INCLUDE
cd "C:\BLAS\OpenBLAS-v0.3.10\OpenBLAS-0.3.10"
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
dir "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build"
dir "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build"
rem set INCLUDE
cmake -G "Ninja" -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH="C:\BLAS\OpenBLAS-v0.3.10\OpenBLAS-0.3.10\out\install\x64-Release" -DCMAKE_C_COMPILER:FILEPATH=cl -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=ninja "C:\BLAS\OpenBLAS-v0.3.10\OpenBLAS-0.3.10"
cmake --build "C:\BLAS\OpenBLAS-v0.3.10\OpenBLAS-0.3.10"
echo compileBLAS.cmd completed