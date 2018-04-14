@ECHO OFF
CALL settings.bat
MD build
PUSHD build
cl ../src/raytracer.c /Zi /W3 /nologo /D_CRT_SECURE_NO_WARNINGS
POPD

