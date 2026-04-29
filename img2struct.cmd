@echo off
set INPUT_IMG=ruka.png
set OUTPUT_H=sprite_data.h
python convert_sprite.py "%INPUT_IMG%" "%OUTPUT_H%"

if %errorlevel% equ 0 (
    echo Done!
) else (
    echo Conversion failed!
)

pause