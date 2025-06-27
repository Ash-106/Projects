import wave
import struct
import heapq
from collections import defaultdict
from bitarray import bitarray
import numpy as np
from scipy.fftpack import dct

def thresholding_dct(dct_coeffs, threshold):
    truncated_coeffs = np.where(np.abs(dct_coeffs) < threshold, 0, dct_coeffs)
    return truncated_coeffs

def run_length_encoding(data):
    encoded_data = []
    count = 1
    for i in range(1, len(data)):
        if data[i] == data[i-1]:
            count += 1
        else:
            encoded_data.append((count, data[i-1]))
            count = 1
    encoded_data.append((count, data[-1]))
    return encoded_data

def build_huffman_tree(frequencies):
    heap = [[weight, [symbol, ""]] for symbol, weight in frequencies.items()]
    heapq.heapify(heap)
    while len(heap) > 1:
        lo = heapq.heappop(heap)
        hi = heapq.heappop(heap)
        for pair in lo[1:]:
            pair[1] = '0' + pair[1]
        for pair in hi[1:]:
            pair[1] = '1' + pair[1]
        heapq.heappush(heap, [lo[0] + hi[0]] + lo[1:] + hi[1:])
    return sorted(heapq.heappop(heap)[1:], key=lambda p: (len(p[-1]), p))

def huffman_encoding(data):
    frequencies = defaultdict(int)
    for item in data:
        frequencies[item] += 1
    huff_tree = build_huffman_tree(frequencies)
    huff_codes = {}
    for symbol, code in huff_tree:
        huff_codes[symbol] = code
    encoded_data = [huff_codes[item] for item in data]
    return encoded_data, huff_codes

# Read the .wav file
wav_file = wave.open('audio.wav', 'rb')
num_samples = wav_file.getnframes()
sample_width = wav_file.getsampwidth()
sample_rate = wav_file.getframerate()
print(num_samples," ", sample_width," ", sample_rate," ")
# Extract audio samples from the .wav file
samples = []
for _ in range(num_samples):
    sample = wav_file.readframes(1)
    sample_value = int.from_bytes(sample, byteorder='little', signed=True)
    # print(sample_value)
    

    samples.append(sample_value)

# Perform DCT transformation
dct_coeffs = dct(samples, norm='ortho')

# Calculate the energy of each DCT coefficient
energy = np.abs(dct_coeffs) ** 2

# Sort the energy values in descending order
sorted_energy = np.sort(energy)[::-1]

# Calculate the cumulative energy
cumulative_energy = np.cumsum(sorted_energy)

# Find the threshold value that represents 99% of the signal energy
total_energy = np.sum(energy)
threshold_energy = 0.99 * total_energy
threshold_index = np.where(cumulative_energy >= threshold_energy)[0][0]
threshold = np.sqrt(sorted_energy[threshold_index])

# Apply thresholding to the DCT coefficients
truncated_coeffs = thresholding_dct(dct_coeffs, threshold)

# Perform quantization (if desired)

# Convert coefficients to integers

# Perform run-length encoding
rle_encoded_data = run_length_encoding(truncated_coeffs)

# Perform Huffman encoding
encoded_data, huff_codes = huffman_encoding(rle_encoded_data)

# Convert Huffman codes to bitarray
bitarr = bitarray()
for code in encoded_data:
    bitarr.extend(bitarray(code))

# Write the compressed data to a binary file
with open('compressed.bin', 'wb') as file:
    bitarr.tofile(file)

# Save the Huffman codes as a dictionary (optional)
with open('huffman_codes.txt', 'w') as file:
    for symbol, code in huff_codes.items():
        file.write(f'{symbol}: {code}\n')

# Calculate the compression factor
original_length = num_samples * sample_width
compressed_length = len(encoded_data)
compression_factor =  compressed_length/original_length
print("Compression Factor:", compression_factor)

mse = np.mean((np.array(samples) - np.array(truncated_coeffs)) ** 2)
max_sample_value = 2 ** (8 * sample_width) / 2 - 1  # Maximum value for the given sample width
psnr = 10 * np.log10(max_sample_value ** 2 / mse)
print("PSNR:", psnr)
# # In this code, we calculate the Mean Squared Error (MSE) between the original audio samples and the truncated DCT coefficients (truncated_coeffs). Since the compression step involved thresholding the DCT coefficients, we consider the truncated_coeffs as the compressed data for PSNR calculation. While this is not the same as a full audio compression-decompression cycle, it allows us to demonstrate the PSNR calculation in the absence of explicit decompressed samples. However, please note that this PSNR value will not represent the actual audio compression quality since it is based on DCT coefficients rather than the full compressed audio data.





