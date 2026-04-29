import sys
import os
from PIL import Image

def rgb888_to_rgb565(r, g, b):
    # Формат RGB565: 5 бит Red, 6 бит Green, 5 бит Blue
    # RRRRR GGGGGG BBBBB
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def main():
    if len(sys.argv) < 3:
        print("Usage: convert_sprite.py <input_image> <output_header>")
        return

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    if not os.path.exists(input_path):
        print(f"Error: File {input_path} not found!")
        return

    img = Image.open(input_path).convert('RGB')
    width, height = img.size
    pixels = img.load()

    # Имя переменной для C-кода (на основе имени файла без расширения)
    var_name = os.path.splitext(os.path.basename(input_path))[0].upper()
    # Заменяем дефисы или пробелы на подчеркивания для валидного имени в C
    var_name = var_name.replace('-', '_').replace(' ', '_')

    with open(output_path, 'w') as f:
        f.write(f"// Generated from {input_path}\n")
        f
        f.write(f"unsigned short _spr{var_name}[] = {{\n")

        count = 0
        for y in range(height):
            row = []
            for x in range(width):
                r, g, b = pixels[x, y]
                
                # Если пиксель черный (или очень близкий к черному), делаем прозрачным 0x0000
                if r == 0 and g == 0 and b == 0:
                    color565 = 0x0000
                else:
                    color565 = rgb888_to_rgb565(r, g, b)
                
                row.append(f"0x{color565:04X}")
                count += 1
            
            # Форматируем строку: добавляем запятые и переносы строк каждые N элементов
            line = ", ".join(row)
            f.write(f"    {line},\n")
            
        f.write("};\n")
        print(f"Success! Created {output_path} with {count} pixels.")

if __name__ == "__main__":
    main()