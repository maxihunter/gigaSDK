import sys

file_path = "memory"
try:
    with open(file_path, 'r') as file:
        rom = 0
        ram = 0
        for line in file:
            line_parts = line.strip().split('\t')
            if line_parts[0].strip() == 'text':
                continue
            rom = int(line_parts[0]) + int(line_parts[1])
            ram = int(line_parts[1]) + int(line_parts[2])
        rom_pers = (rom / int(sys.argv[1]))*100
        ram_pers = (ram / int(sys.argv[2]))*100
        print("ROM: ", rom, '\t(using a %.2f%%)' % rom_pers)
        print("RAM: ", ram, '\t(using a %.2f%%)' % ram_pers)
        pass
except FileNotFoundError:
    print(f"Error: The file '{file_path}' was not found.")
except Exception as e:
    print(f"An error occurred: {e}")


