import math
import sys

def generate_sin_lut(table_size=1024, var_name="fx_sin_lut"):
    # Q16.16 fixed-point scale.
    FP_SHIFT = 16
    SCALE = 1 << FP_SHIFT  # 65536

    print(f"/* Precomputed Sine Look-Up Table (Q16.16 format), {table_size} entries */")
    print(f"const int32 {var_name}[{table_size}] = {{")

    for i in range(table_size):
        angle = (i * 2.0 * math.pi) / table_size
        fp_sine = int(round(math.sin(angle) * SCALE))
        comma = "," if i < table_size - 1 else ""
        # 8 entries per line for readability.
        print(f"    {fp_sine:7d}{comma}", end="")
        if (i + 1) % 8 == 0:
            print()

    print("\n};")

if __name__ == "__main__":
    # Default = 1024 entries, used by the PR-3 fixed-point migration in
    # main.c.  Pass an integer to override (e.g. `python gen_sin_LUT.py 256`).
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 1024
    generate_sin_lut(n)