with open('C:/Users/parallelno/Downloads/bytecode_generator.bin', 'wb') as f:
    for i in range(256):
        f.write(bytes([i, 0, 0]))
