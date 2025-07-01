from PIL import Image
from pathlib import Path
import tifffile
from numpy import asarray

# Set the root directory to search
root_dir = Path(r'../assets/textures/props')

# Check if the root directory exists
if not root_dir.exists():
    print(f"Root directory {root_dir} does not exist.")
    exit()

# Search recursively for .tif or .tiff files
tif_files = list(root_dir.rglob("*.tif")) + list(root_dir.rglob("*.tiff"))

print(f"Found {len(tif_files)} TIFF files.")

for tif_path in tif_files:
    png_path = tif_path.with_suffix('.png')
    try:
        # Open and convert the TIFF image
        img = tifffile.imread(tif_path)
        if img.dtype == 'uint16':
            # Scale 16-bit image to 8-bit
            img = (img / 256).astype('uint8')
        img = Image.fromarray(img)
        # img = img.convert('RGBA')
        img.save(png_path, format="PNG")
        print(f"Converted: {tif_path} -> {png_path}")
        # Delete the original TIFF file
        try:
            tif_path.unlink()
            print(f"Deleted original: {tif_path}")
        except Exception as e:
            print(f"Failed to delete {tif_path}: {e}")
    except Exception as e:
        print(f"Failed to convert {tif_path}: {e}")