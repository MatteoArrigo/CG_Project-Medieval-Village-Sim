import json
import os
from pygltflib import GLTF2

# Carica il file .gltf
gltf = GLTF2().load("assets/models/Buildings.gltf")

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
                'asset': 'village',
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
            'texture': f'assets/textures/all_buildings/{tex}',
            'format': format
        })
    return lines
textures = getTextures('assets/textures/all_buildings')




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
    return primitiveTextureMap

# from scipy.spatial.transform import Rotation as R
# def quaternion_to_euler(quaternion):
#     # Crea un oggetto Rotation dal quaternione
#     rotation = R.from_quat(quaternion)
#     # Converte il quaternione in angoli di Eulero (ordine: XYZ)
#     euler_angles = rotation.as_euler('xyz', degrees=True)
#     return euler_angles

def getElementsToRender():
    nodeNames = [node.name for node in gltf.nodes]
    toRenderTmp = {}
    for node in gltf.nodes:
        if node.translation or node.rotation or node.scale:
            children = [nodeNames[i] for i in node.children if 'collider' not in nodeNames[i]]
            if not children and not node.mesh:
                    continue

            if node.mesh:
                children.append(node.name)

            if node.name not in toRenderTmp:
                toRenderTmp[node.name] = {'children': children}
                toRenderTmp[node.name]['transf'] = []
            tranfEntry = {}
            if node.translation:
                trans = [coord for coord in node.translation]
                # trans[0] *= -1
                tranfEntry['translation'] = trans
            if node.rotation:
                rotOld = [node.rotation[i] for i in range(4)]
                rot = [0.0, 0.0, 0.0, 1.0]  # Inizializza una lista di grandezza 4
                rot[0] = -rotOld[3]
                rot[1] = rotOld[2]
                rot[2] = -rotOld[1]
                rot[3] = rotOld[0]
                # rot[0] = -rotOld[1]
                # rot[1] = rotOld[0]
                # rot[2] = rotOld[3]
                # rot[3] = -rotOld[2]
                # rot = quaternion_to_euler(rot)
                tranfEntry['rotation'] = rot
            if node.scale:
                tranfEntry['scale'] = node.scale
            toRenderTmp[node.name]['transf'].append(tranfEntry)

    usedIdsCount = {}
    toRender = []
    primitiveTextureMap = getTextureMap()
    for groupName, groupData in toRenderTmp.items():

        for meshName in groupData["children"]:
            if meshName[:-2] not in usedIdsCount:
                usedIdsCount[meshName[:-2]] = 1
                id = meshName
            else:
                child_suffix = usedIdsCount[meshName[:-2]]  # Prendi le ultime 2 cifre di `meshName` come intero
                incremented_suffix = child_suffix + 1  # Incrementa il valore
                id = f"{meshName[:-2]}{incremented_suffix:02}"  # Riforma il nome con il suffisso incrementato
                usedIdsCount[meshName[:-2]] = child_suffix + 1
            # print(id, meshName)

            if meshName not in primitiveTextureMap:
                print(f"Warning: {meshName} not found in primitiveTextureMap")
                continue

            primitives = [model for model in models if model['model'] == meshName]
            if(len(primitives) == 0):
                print(f"Warning: {meshName} not found in models")
                continue

            for i, transf in enumerate(groupData['transf']):
                for primitiveEntry in primitives:
                    entry = {
                       'id': id+f'-{i:02}'+primitiveEntry['id'][-3:],
                       'model': primitiveEntry['id'],
                       'texture': [
                           primitiveTextureMap[meshName][0],
                           primitiveTextureMap[meshName][1],
                           "void",
                           "void"
                       ]
                    }
                    if 'translation' in transf:
                        entry['translate'] = transf["translation"]
                    if 'rotation' in transf:
                        entry['quaternion'] = [
                            transf["rotation"][0],
                            transf["rotation"][1],
                            transf["rotation"][2],
                            transf["rotation"][3]
                        ]
                    if 'scale' in transf:
                        entry['scale'] = transf["scale"]
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
                    .replace('"quat', '\n  "quat')
                    .replace('"translate', '\n  "translate')
                    .replace('"scale', '\n  "scale'))
        f.write("\n\n")