import json
import os
from pygltflib import GLTF2

# Carica il file .gltf
gltf = GLTF2().load("assets/models/Buildings.gltf")
texDir = 'all_buildings'
assetName = 'village'

def getModels():
    models = []
    for mesh in gltf.meshes:
        model = mesh.name
        for meshId, primitive in enumerate(mesh.primitives):
            id = f"{model}-{meshId:02}"
            models.append({
                "id": id,
                'VD': "VDtan",
                "model": model,
                'node': model,
                "meshId": meshId,
                'asset': assetName,
                'format': 'ASSET'
            })
    return models
models = getModels()

def getTextures(dir):
    texs = os.listdir(dir)
    lines = []
    for tex in texs:
        tex = tex
        format = ''
        if tex[-5] == 'a':
            format = 'C'
        else:
            format = 'D'
        # line = '{' + f'"id": "{tex[:-4]}", "texture": "assets/textures/all_buildings/{tex}", "format": "{format}"' + '},'
        lines.append({
            'id': tex[:-4],
            'texture': f'assets/textures/{texDir}/{tex}',
            'format': format
        })
    return lines
textures = getTextures('assets/textures/' +texDir)




def get_image_uri(texture_index):
    if texture_index is None:
        return None
    texture = gltf.textures[texture_index]
    image_index = texture.source
    return gltf.images[image_index].uri if image_index is not None else None

def getMaterialsList():
    materialsList = []
    for i, material in enumerate(gltf.materials):
        # print(f"\nMateriale {i}: {material.name or '[senza nome]'}")

        # Albedo (baseColorTexture)
        albedo_index = None
        # print(material.extensions['KHR_materials_pbrSpecularGlossiness'])
        if not material.extensions:
            print(f"Warning: Material {material.name} has no extensions.")
            materialsList.append(('void', 'void', material.name))
            continue
        if 'diffuseTexture' in material.extensions['KHR_materials_pbrSpecularGlossiness'].keys():
            albedo_index = material.extensions['KHR_materials_pbrSpecularGlossiness']['diffuseTexture']['index']
        albedo_uri = get_image_uri(albedo_index)
        # print(f"  Albedo: {albedo_uri or '— nessuna'}")

        # Normal Map
        normal_index = material.normalTexture.index if material.normalTexture else None
        normal_uri = get_image_uri(normal_index)
        # print(f"  Normal map: {normal_uri or '— nessuna'}")

        materialsList.append((albedo_uri, normal_uri, material.name))
    return materialsList

def getTextureMap():
    primitiveTextureMap = {}
    materialsList = getMaterialsList()
    for mesh in gltf.meshes:
        for i, primitive in enumerate(mesh.primitives):
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

                primitiveTextureMap[f'{name}-{i:02}'] = (albedoTex, normalTex)
                # line = '{' + f'"id": "{name}",  "model": "{name}","texture": ["{albedoTex}", "{normalTex}", "void", "void"]' + '},'
                # lines.append(line)
    return primitiveTextureMap

# from scipy.spatial.transform import Rotation as R
# def quaternion_to_euler(quaternion):
#     # Crea un oggetto Rotation dal quaternione
#     rotation = R.from_quat(quaternion)
#     # Converte il quaternione in angoli di Eulero (ordine: XYZ)
#     euler_angles = rotation.as_euler('xyz', degrees=True)
#     return euler_angles

def getAllChildrenNodes(gltfNode):
    children = []
    if gltfNode.mesh:
        children.append(gltfNode.name)
    if gltfNode.children:
        for child in gltfNode.children:
            childNode = gltf.nodes[child]
            children.extend(getAllChildrenNodes(childNode))
    return children


def computeRotationFromNode(node):
    rotOld = [node.rotation[i] for i in range(4)]

    from scipy.spatial.transform import Rotation as R
    rotation = R.from_quat([rotOld[0], rotOld[2], -rotOld[1], rotOld[3]])
    eulerAngles = rotation.as_euler('xyz', degrees=True)
    # if -3.46368051 in tranfEntry['translation']:
    #     print('debug')
    rot = [0.0, 0.0, 0.0]  # Inizializza una lista di grandezza 4
    rot[0] = float(eulerAngles[0])
    rot[1] = float(-eulerAngles[2])
    rot[2] = float(eulerAngles[1])

    # rot = [0.0, 0.0, 0.0, 0.0]  # Inizializza una lista di grandezza 4
    # rot[0] = -rotOld[3]
    # rot[1] = rotOld[2]
    # rot[2] = -rotOld[1]
    # rot[3] = rotOld[0]

    return rot

def getElementsToRender():
    toRender = []
    primitiveTextureMap = getTextureMap()
    for node in gltf.nodes:
        if node.mesh is None or node.translation is None:
            continue

        mesh = gltf.meshes[node.mesh]
        for meshId, primitive in enumerate(mesh.primitives):
            primitiveId = f'{mesh.name}-{meshId:02}'

            entry = {
                'id': node.name,
                'model': primitiveId,
                'texture': [
                    primitiveTextureMap[primitiveId][0],
                    primitiveTextureMap[primitiveId][1],
                    "void",
                    "void"
                ]
            }
            if node.translation:
                entry['translate'] = [coord for coord in node.translation]
            if node.rotation:
                entry['eulerAngles'] = [x for x in computeRotationFromNode(node)]
            if node.scale:
                entry['scale'] = [coord for coord in node.scale]
            toRender.append(entry)

    return toRender

toRender = getElementsToRender()


# sections = {
#     "models": models,
#     "textures": textures,
#     "toRender": toRender
# }
# # Salva le sezioni in un file JSON
# output_file = "script/json_sections.json"
# with open(output_file, 'w') as f:
#     json.dump(sections, f, indent=4)

output_file = "script/json_sections.txt"

with open(output_file, "w") as f:
    for section in [models, textures, toRender]:
        for entry in section:
            f.write(f'{entry},\n'.replace("'", '"')
                    .replace('"eulerAngles', '\n  "eulerAngles')
                    .replace('"quaternion', '\n  "quaternion')
                    .replace('"translate', '\n  "translate')
                    .replace('"scale', '\n  "scale'))
        f.write("\n\n")