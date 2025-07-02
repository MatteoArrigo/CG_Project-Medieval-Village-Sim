from PIL import Image

def generate_uniform_texture(width, height, rgba, output_file):
    # Crea un'immagine con dimensioni specificate e colore uniforme
    image = Image.new("RGBA", (width, height), rgba)
    # Salva l'immagine come file PNG
    image.save(output_file)

# Specifica dimensioni, colore RGBA e nome del file di output
width, height = 8, 8
rgba = (1, 1, 1, 255)
output_file = "../assets/textures/all1s.png"

generate_uniform_texture(width, height, rgba, output_file)