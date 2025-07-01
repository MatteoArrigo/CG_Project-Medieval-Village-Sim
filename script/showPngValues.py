from PIL import Image

def show_png_values(file_path):
    try:
        # Apri l'immagine
        img = Image.open(file_path)
        img = img.convert("RGBA")  # Converti in formato RGBA per garantire la compatibilità

        # Ottieni i dati dei pixel
        pixels = list(img.getdata())

        # Stampa i valori dei pixel
        print(f"Valori dei pixel per il file '{file_path}':")
        for i, pixel in enumerate(pixels[:100]):
            print(f"Pixel {i}: {pixel}")

    except Exception as e:
        print(f"Errore durante la lettura del file PNG: {e}")

# Esempio di utilizzo
file_path = "../assets/textures/all_buildings/build_building_01_sg.png"
show_png_values(file_path)