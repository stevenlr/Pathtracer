@ECHO OFF
CALL settings.bat
CALL "%VS_DIR%/Common7/IDE/devenv.exe" build/raytracer.exe
