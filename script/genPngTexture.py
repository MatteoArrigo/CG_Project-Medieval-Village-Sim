from PIL import Image

def generate_uniform_texture(width, height, rgba, output_file):
    # Crea un'immagine con dimensioni specificate e colore uniforme
    image = Image.new("RGBA", (width, height), rgba)
    # Salva l'immagine come file PNG
    image.save(output_file)

def generate_two_color_texture(width, height, color_top, color_bottom, output_file, orientation="vertical"):
    # Crea un'immagine con dimensioni specificate
    image = Image.new("RGBA", (width, height))

    # Riempie l'immagine con i colori in base all'orientamento
    for y in range(height):
        for x in range(width):
            if orientation == "vertical":
                if y < height // 2:
                    image.putpixel((x, y), color_top)
                else:
                    image.putpixel((x, y), color_bottom)
            elif orientation == "horizontal":
                if x < width // 2:
                    image.putpixel((x, y), color_top)
                else:
                    image.putpixel((x, y), color_bottom)

    # Salva l'immagine come file PNG
    image.save(output_file)

def generate_four_color_texture(width, height, color_top_left, color_top_right, color_bottom_left, color_bottom_right, output_file):
    # Crea un'immagine con dimensioni specificate
    image = Image.new("RGBA", (width, height))

    # Riempie l'immagine con i colori nei quattro angoli
    for y in range(height):
        for x in range(width):
            if x < width // 2 and y < height // 2:
                image.putpixel((x, y), color_top_left)
            elif x >= width // 2 and y < height // 2:
                image.putpixel((x, y), color_top_right)
            elif x < width // 2 and y >= height // 2:
                image.putpixel((x, y), color_bottom_left)
            else:
                image.putpixel((x, y), color_bottom_right)

    # Salva l'immagine come file PNG
    image.save(output_file)

# Specifica dimensioni, colore RGBA e nome del file di output
width, height = 1024, 1024

output_files = [
    "../assets/textures/water/uniform_posx.png",
    "../assets/textures/water/uniform_negx.png",
    "../assets/textures/water/uniform_posy.png",
    "../assets/textures/water/uniform_negy.png",
    "../assets/textures/water/uniform_posz.png",
    "../assets/textures/water/uniform_negz.png"
]

colors = [
    (255, 0, 0, 255),  # Top-left-front
    (0, 255, 0, 255),  # Top-right-front
    (0, 0, 255, 255),  # Bottom-left-front
    (255, 255, 0, 255),  # Bottom-right-front
    (0, 0, 0, 255),  # Top-right-back
    (0, 128, 128, 255),  # Bottom-right-back
    (128, 128, 128, 255),  # Bottom-left-back
    (255, 255, 255, 255),  # Top-left-back
]

for i in range(6):
    generate_uniform_texture(width, height, colors[i], output_files[i])