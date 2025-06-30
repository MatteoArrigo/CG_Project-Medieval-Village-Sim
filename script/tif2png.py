from PIL import Image
from pathlib import Path
import os

# Set the root directory to search
root_dir = Path("../assets/textures")

# Search recursively for .tif or .tiff files
tif_files = list(root_dir.rglob("*.tif")) + list(root_dir.rglob("*.tiff"))

print(f"Found {len(tif_files)} TIFF files.")

for tif_path in tif_files:
    png_path = tif_path.with_suffix('.png')

    try:
        # Open and convert the TIFF image
        with Image.open(tif_path) as img:
            img.save(png_path, format="PNG")
        print(f"Converted: {tif_path} -> {png_path}")

        # Delete the original TIFF file
        os.remove(tif_path)
        print(f"Deleted original: {tif_path}")

    except Exception as e:
        print(f"Failed to convert {tif_path}: {e}")
