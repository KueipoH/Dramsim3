import random

random.seed(0)

# Parameters
low = 0  # min of random number
high = 20  # max of random number (number of users)
row_number_of_index_array = 1024 * 5  # (src, dst) pair
the_number_of_items = 20 * 80

# Generate random 'src' and 'dst' arrays and form Index_Array
Index_Array = []

for _ in range(row_number_of_index_array):
    src = random.randint(0, the_number_of_items - 1)
    dst = random.randint(low, high)
    Index_Array.append((src, dst))

# Sort Index_Array by dst
Index_Array.sort(key=lambda x: x[1])

# Export Index_Array to a file
output_file = "sorted_index_array.txt"

with open(output_file, "w") as f:
    for pair in Index_Array:
        f.write(f"{pair[0]} {pair[1]}\n")

print(f"Sorted Index_Array exported to '{output_file}'")
