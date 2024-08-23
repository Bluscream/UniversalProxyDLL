import sys
import os
import subprocess

try:
    import pefile
except ImportError as e:
    print("To run this script, please install the 'pefile' package: 'pip install pefile'.")
    sys.exit(1)

if len(sys.argv) < 2:
    print("Argument error")
    print("Usage: python create_export_list.py <dllfile1.dll> <dllfile2.dll> ...")
    sys.exit(1)

# Initialize an empty list to hold the results
results = []
names = []

for dllpath in sys.argv[1:]:
    if not os.path.exists(dllpath):
        print(f"DLL not found: {dllpath}")
        continue

    d = [pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]]
    pe = pefile.PE(dllpath, fast_load=True)
    pe.parse_data_directories(directories=d)

    exports = [[e.ordinal, e.name] for e in pe.DIRECTORY_ENTRY_EXPORT.symbols]
    exports.sort(key=lambda e: e[0])  # Sort by ordinal

    export_names = []
    for export in exports:
        if export[1] is None:  # Ignore exports without names
            continue
        export_names.append(export[1])

    export_str = ""
    num_columns = 50
    index = 0
    for export_name in export_names:
        if type(export_name) is not int:
            export_str += "Export(" + str(index) + ", " + export_name.decode("utf-8") + ") "
            index += 1
            if index % num_columns == 0:
                export_str += "\n"

    dllname = os.path.basename(dllpath).replace('.dll', '')
    names.append(f"* {dllname}")
    # Format the result for each DLL
    result_line = f"// {dllname}\n{export_str}\n\n"
    results.append(result_line)

# Write the results to a single file
output_filename = "multiple_dll_exports.txt"
with open(output_filename, "a") as file:
    file.writelines(results)
output_filename = "README.md"
with open(output_filename, "w") as file:
    file.writelines(results)