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

        # for pixel in pixels:
        #     if pixel[3] != 255:
        #         print(f"Pixel con alpha diverso da 255: {pixel}")
        #         break

        # Imposta il canale alpha (A) di tutti i pixel a 255
        # pixels = [(r, g, b, 255) for r, g, b, a in pixels]
        # img.putdata(pixels)

        # Salva l'immagine modificata
        # output_path = file_path.replace(".png", "_modified.png")
        # img.save(output_path)
        # print(f"Immagine salvata con canale alpha impostato a 255: {output_path}")

    except Exception as e:
        print(f"Errore durante la lettura del file PNG: {e}")

# Esempio di utilizzo
file_path = r"C:\Users\Matteo\Desktop\0LabsHPC\CG project\Polimi-CG-Project\assets\textures\water\posy.png"
show_png_values(file_path)