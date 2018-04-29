@ECHO OFF
CALL settings.bat
MD build
PUSHD build
cl ../src/raytracer.c /W3 /O2 /nologo /D_CRT_SECURE_NO_WARNINGS /I%SDL_INCLUDE_DIR% /link /LIBPATH:%SDL_LIB_DIR% SDL2main.lib SDL2.lib /subsystem:console
POPD

