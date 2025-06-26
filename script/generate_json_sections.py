import json
import os

def parse_primitives(file_path):
    models = []
    with open(file_path, 'r') as file:
        lines = file.readlines()
        current_mesh = None
        mesh_id = None

        for line in lines:
            line = line.strip()
            if line.startswith("Mesh"):
                # Parse mesh information
                parts = line.split(":")
                mesh_id = int(parts[0].split()[1])
                mesh_name = parts[1].strip()
                current_mesh = {"id": mesh_name, "meshId": mesh_id, "primitives": []}
                models.append(current_mesh)
            elif line.startswith("Primitive"):
                # Parse primitive information
                primitive_id = int(line.split()[1].strip(":"))
                current_mesh["primitives"].append({"primitiveId": primitive_id, "attributes": []})
            elif line.startswith("Attribute"):
                # Parse attribute information
                attribute = line.split(":")[1].strip()
                current_mesh["primitives"][-1]["attributes"].append(attribute)

    return models

def generate_textures(dir):
    texs = os.listdir(dir)
    lines = []
    for tex in texs:
        tex = tex
        format = ''
        if tex[-5] == 'a':
            format = 'C'
        else:
            format = 'D'
        line = '{' + f'"id": "{tex[:-4]}", "texture": "../assets/textures/all_buildings/{tex}", "format": "{format}"' + '},'
        lines.append(line)
    return lines

def main():
    file_path = 'list_primitives.txt'
    models = parse_primitives(file_path)
    # print(json.dumps(models, indent=2))
    with open('prova.txt', 'w') as f:
        for model in models:
            for i in range(len(model["primitives"])):
                line = '{' + f'"id": "{model["id"]}", "VD": "VDtan", "model": "{model["id"]}", "node": "{model["id"]}", "meshId": {i}, "asset": "village", "format": "ASSET"' + '},'
                line = line.replace('\n', '')
                f.write(line + '\n')




from pygltflib import GLTF2

# Carica il file .gltf
gltf = GLTF2().load("../assets/models/Buildings.gltf")

# Funzione per trovare il file URI a partire da un indice texture
def get_image_uri(texture_index):
    if texture_index is None:
        return None
    texture = gltf.textures[texture_index]
    image_index = texture.source
    return gltf.images[image_index].uri if image_index is not None else None


materialsList = []
for i, material in enumerate(gltf.materials):
    # print(f"\nMateriale {i}: {material.name or '[senza nome]'}")

    # Albedo (baseColorTexture)
    albedo_index = None
    # print(material.extensions['KHR_materials_pbrSpecularGlossiness'])
    if 'diffuseTexture' in material.extensions['KHR_materials_pbrSpecularGlossiness'].keys():
        albedo_index = material.extensions['KHR_materials_pbrSpecularGlossiness']['diffuseTexture']['index']
    albedo_uri = get_image_uri(albedo_index)
    # print(f"  Albedo: {albedo_uri or '— nessuna'}")

    # Normal Map
    normal_index = material.normalTexture.index if material.normalTexture else None
    normal_uri = get_image_uri(normal_index)
    # print(f"  Normal map: {normal_uri or '— nessuna'}")

    materialsList.append((albedo_uri, normal_uri))


# print(len(materialsList))

primitiveTextureMap = {}
for mesh in gltf.meshes:
    for primitive in mesh.primitives:
        if primitive.material is not None:
            idx = primitive.material
            name = mesh.name
            if materialsList[idx][0] is not None:
                albedoTex = materialsList[idx][0][:-4]
            else:
                albedoTex = 'void'
            if materialsList[idx][1] is not None:
                normalTex = materialsList[idx][1][:-4]
            else:
                normalTex = 'void'

            primitiveTextureMap[name] = (albedoTex, normalTex)
            # line = '{' + f'"id": "{name}",  "model": "{name}","texture": ["{albedoTex}", "{normalTex}", "void", "void"]' + '},'
            # lines.append(line)

# lines = generate_textures('assets/textures/all_buildings')
# with open('prova.txt', 'w') as f:
    # for line in lines:
#         f.write(line + '\n')
# with open('textures.json', 'w') as f:
#     f.write('[' + '\n')
#     for line in lines:
#         f.write(line + '\n')
#     f.write(']')

nodeNames = [node.name for node in gltf.nodes]

from scipy.spatial.transform import Rotation as R
def quaternion_to_euler(quaternion):
    # Crea un oggetto Rotation dal quaternione
    rotation = R.from_quat(quaternion)
    # Converte il quaternione in angoli di Eulero (ordine: XYZ)
    euler_angles = rotation.as_euler('xyz', degrees=True)
    return euler_angles

toRenderTmp = {}
for node in gltf.nodes:
    if node.name == 'Buildings':
        continue
    if node.children and (node.translation or node.rotation or node.scale):
        children = [nodeNames[i] for i in node.children if 'collider' not in nodeNames[i]]
        if not children:
            continue

        toRenderTmp[node.name] = {'children': children}
        if node.translation:
            trans = [coord for coord in node.translation]
            trans[0] *= -1
            toRenderTmp[node.name]['translation'] = trans
        if node.rotation:
            rot = [node.rotation[i] for i in range(4)]
            # rot[1] *= -1
            # rot[2] *= -1
            euler_angles = quaternion_to_euler(rot)
            toRenderTmp[node.name]['rotation'] = euler_angles
        if node.scale:
            toRenderTmp[node.name]['scale'] = node.scale

usedIds = {}
lines = []
for groupName, groupData in toRenderTmp.items():
    for model in groupData["children"]:
        if model[:-2] not in usedIds:
            usedIds[model[:-2]] = 1
            id = model
        else:
            child_suffix = usedIds[model[:-2]]  # Prendi le ultime 2 cifre di `model` come intero
            incremented_suffix = child_suffix + 1  # Incrementa il valore
            id = f"{model[:-2]}{incremented_suffix:02}"  # Riforma il nome con il suffisso incrementato
            usedIds[model[:-2]] = child_suffix + 1
        # print(id, model)

        if model not in primitiveTextureMap:
            print(f"Warning: {model} not found in primitiveTextureMap")
            continue

        line = '{' + f'"id": "{id}", "model": "{model}", "texture": ["{primitiveTextureMap[model][0]}", "{primitiveTextureMap[model][1]}", "void", "void"]'
        if 'translation' in groupData:
            line += f',\n  "translate": {groupData["translation"]}'
        if 'rotation' in groupData:
            line += f',\n  "eulerAngles": [{groupData["rotation"][0]}, {groupData["rotation"][1]}, {groupData["rotation"][2]}]'
        if 'scale' in groupData:
            line += f',\n  "scale": {groupData["scale"]}'
        line += '},'
        lines.append(line)

with open('toRender.json', 'w') as f:
    f.write('[' + '\n')
    for line in lines:
        f.write(line + '\n')
    f.write(']')




