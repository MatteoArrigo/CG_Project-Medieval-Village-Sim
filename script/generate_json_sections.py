import json
import os
from pygltflib import GLTF2
import re

# Carica il file .gltf
gltf = GLTF2().load("assets/models/Props.gltf")
texDir = 'props'
assetName = 'village_props'

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

def get_texture_name(texture_index):
    if texture_index is None:
        return None
    # texture = gltf.textures[texture_index]
    # return texture.name
    textureId = gltf.textures[texture_index].source
    return gltf.images[textureId].name if textureId is not None else None

def getMaterialsList():
    materialsList = []
    for material in gltf.materials:
        entry = {'name': material.name}

        # print(material.extensions['KHR_materials_pbrSpecularGlossiness'])
        if not material.extensions or 'KHR_materials_pbrSpecularGlossiness' not in material.extensions:
            print(f"Warning: Material {material.name} does not support KHR_materials_pbrSpecularGlossiness")
            entry['albedo'] = 'void'
            entry['normal'] = 'void'
            entry['sg'] = 'void'
            entry['ao'] = 'void'
            entry['aoFactor'] = None
            entry['diffuseFactor'] = None
            entry['specularFactor'] = None
            entry['glossinessFactor'] = None
            materialsList.append(entry)
            continue
        if 'diffuseTexture' in material.extensions['KHR_materials_pbrSpecularGlossiness']:
            entry['albedo'] = get_texture_name(material.extensions['KHR_materials_pbrSpecularGlossiness']['diffuseTexture']['index'])
        else:
            entry['albedo'] = 'all1s'  # Default texture if no diffuse texture is provided
            print(f"Warning: Material {material.name} does not have a diffuse texture in KHR_materials_pbrSpecularGlossiness")
        # This way, the constant value of diffuseFactor is taken (multiplied by 1)

        entry['normal'] = get_texture_name(material.normalTexture.index) if material.normalTexture else 'void'
        entry['ao'] = get_texture_name(material.occlusionTexture.index) if material.occlusionTexture else 'void'
        entry['sg'] = get_texture_name(material.extensions['KHR_materials_pbrSpecularGlossiness']['specularGlossinessTexture']['index']) \
            if 'specularGlossinessTexture' in material.extensions['KHR_materials_pbrSpecularGlossiness'] else 'void'

        entry['aoFactor'] = material.occlusionTexture.strength if material.occlusionTexture else None
        entry['diffuseFactor'] = material.extensions['KHR_materials_pbrSpecularGlossiness']['diffuseFactor'][:3] \
            if 'diffuseFactor' in material.extensions['KHR_materials_pbrSpecularGlossiness'] else None
        entry['specularFactor'] = material.extensions['KHR_materials_pbrSpecularGlossiness']['specularFactor'] \
            if 'specularFactor' in material.extensions['KHR_materials_pbrSpecularGlossiness'] else None
        entry['glossinessFactor'] = material.extensions['KHR_materials_pbrSpecularGlossiness']['glossinessFactor'] \
            if 'glossinessFactor' in material.extensions['KHR_materials_pbrSpecularGlossiness'] else None

        materialsList.append(entry)
    return materialsList


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

def getElementsToRender(meshNameRegex = None):
    toRender = []
    materialsList = getMaterialsList()
    usedIdsCount = {}
    for node in gltf.nodes:
        if node.mesh is None:
        # if node.mesh is None or node.translation is None:
            continue

        mesh = gltf.meshes[node.mesh]
        if meshNameRegex is not None and not re.match(meshNameRegex, mesh.name):
            continue

        for meshId, primitive in enumerate(mesh.primitives):
            primitiveId = f'{mesh.name}-{meshId:02}'
            if primitive.material is None:
                print(f"Warning: Primitive {primitiveId} has no material - Impossible to render")
                continue
            materialIdx = primitive.material
            # print(primitiveId, materialsList[materialIdx]['albedo'])

            if primitiveId not in usedIdsCount:
                usedIdsCount[primitiveId] = 0

            entry = {
                'id': f'{primitiveId}.{usedIdsCount[primitiveId]:02}',
                'model': primitiveId,
                'texture': [
                    materialsList[materialIdx]['albedo'],
                    materialsList[materialIdx]['normal'],
                    materialsList[materialIdx]['sg'],
                    materialsList[materialIdx]['ao']
                ]
            }
            usedIdsCount[primitiveId] += 1
            if node.translation:
                entry['translate'] = [coord for coord in node.translation]
            if node.rotation:
                entry['eulerAngles'] = [x for x in computeRotationFromNode(node)]
            if node.scale:
                entry['scale'] = [coord for coord in node.scale]
            if materialsList[materialIdx]['diffuseFactor'] is not None:
                entry['diffuseFactor'] = materialsList[materialIdx]['diffuseFactor']
            if materialsList[materialIdx]['specularFactor'] is not None:
                entry['specularFactor'] = materialsList[materialIdx]['specularFactor']
            if materialsList[materialIdx]['glossinessFactor'] is not None:
                entry['glossinessFactor'] = materialsList[materialIdx]['glossinessFactor']
            if materialsList[materialIdx]['aoFactor'] is not None:
                entry['aoFactor'] = materialsList[materialIdx]['aoFactor']
            toRender.append(entry)

    return toRender

toRender = getElementsToRender('pf_fish.+')


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
    # for section in [models, toRender]:
        for entry in section:
            f.write(f'{entry},\n'.replace("'", '"')
                    .replace('"eulerAngles', '\n  "eulerAngles')
                    .replace('"quaternion', '\n  "quaternion')
                    .replace('"translate', '\n  "translate')
                    .replace('"scale', '\n  "scale')
                    .replace('"diffuseFactor', '\n  "diffuseFactor'))
        f.write("\n\n")