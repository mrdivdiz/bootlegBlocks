import sys

def to_fp(float_val):
    """
    Имитация макроса #define TO_FP(x) ((int32_t)((x) * FP_ONE))
    где FP_ONE = 1 << 16 (65536)
    """
    # Константа сдвига (2^16)
    FP_SHIFT = 16
    FP_ONE = 1 << FP_SHIFT
    
    # Вычисляем целое значение
    # Используем round(), чтобы избежать ошибок точности float при конвертации
    fp_value = int(round(float_val * FP_ONE))
    
    # Обработка знака для корректного отображения в Hex (имитация int32_t)
    if fp_value < 0:
        # Превращаем отрицательное число в unsigned 32-bit representation
        fp_value = (1 << 32) + fp_value
        
    return fp_value

def main():
    print("--- MRE Fixed-Point Converter (Q16.16) ---")
    print("Введите float число (например, 2.0 или -1.5).")
    print("Для выхода введите 'exit' или нажмите Ctrl+C.\n")

    while True:
        try:
            user_input = input("Input Float: ").strip().lower()
            
            if user_input == 'exit':
                break
            
            # Преобразуем ввод в float
            f_val = float(user_input)
            
            # Считаем значение
            result = to_fp(f_val)
            
            # Вывод результатов
            print(f"  [Decimal] : {result}")
            print(f"  [Hex/Int32]: {hex(result).upper()}")
            print("-" * 40)
            
        except ValueError:
            print("Ошибка: Пожалуйста, введите корректное число.")
        except EOFError:
            break
        except KeyboardInterrupt:
            print("\nВыход...")
            break

if __name__ == "__main__":
    main()