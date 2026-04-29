import math

def generate_sin_lut():
    # Константы для Fixed-Point Q16.16
    FP_SHIFT = 16
    SCALE = 1 << FP_SHIFT  # Это 65536
    TABLE_SIZE = 256

    print("/* Precomputed Sine Look-Up Table (Q16.16 format) */")
    print("static const int32_t sin_lut[256] = {")

    for i in range(TABLE_SIZE):
        # Вычисляем угол в радианах: i * 2PI / 256
        angle = (i * 2.0 * math.pi) / TABLE_SIZE
        
        # Считаем синус
        sine_val = math.sin(angle)
        
        # Переводим в fixed-point формат с округлением
        fp_sine = int(round(sine_val * SCALE))
        
        # Форматируем вывод: добавляем запятую, если это не последний элемент
        comma = "," if i < TABLE_SIZE - 1 else ""
        
        # Печатаем строку каждые 8 элементов для красоты (чтобы массив был читаемым)
        print(f"    {fp_sine:7d}{comma}", end="")
        if (i + 1) % 8 == 0:
            print()

    print("\n};")

if __name__ == "__main__":
    generate_sin_lut()