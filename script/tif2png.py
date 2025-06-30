from PIL import Image
from pathlib import Path

# Set the root directory to search
root_dir = Path(r'C:\Users\Matteo\Desktop\0LabsHPC\CG project\Polimi-CG-Project\assets\textures\all_buildings')

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
        with Image.open(tif_path) as img:
            if img.mode not in ["RGB", "RGBA"]:
                img = img.convert("RGB")  # Convert to a compatible mode
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