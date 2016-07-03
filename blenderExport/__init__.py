bl_info = {
    "name": "3DY File format",
    "author": "Yvain Raeymaekers",
    "blender": (2, 6, 0),
    "api": 41226,
    "location": "File > Import-Export",
    "description": ("Export to 3dy : meshes"),
    "warning": "Bug free edition.",
    "category": "Import-Export"}
    
import json
import bpy
import mathutils
import os
import shutil
import re
from array import array
from bpy_extras.io_utils import ExportHelper
from bpy.props import *

import time, sys

def updateProgress(job_title, progress):
    length = 30 # modify this to change the length
    block = int(round(length*progress))
    msg = "\r{0}: [{1}] {2}%".format(job_title, "#"*block + "-"*(length-block), round(progress*100, 2))
    if progress >= 1: msg += " DONE\r\n"
    sys.stdout.write(msg)
    sys.stdout.flush()

def writeDataBlockToFile(array, binary_file):
    start_offset = binary_file.tell()
    array.tofile(binary_file)
    end_offset = binary_file.tell()
    #print(end_offset - start_offset)
    return {'Address':start_offset, 'Size':end_offset - start_offset}    

def writeMeshField(json_mesh_fields, field_name, components, field_array, binary_file):    
    json_blob = writeDataBlockToFile(field_array, binary_file)
    
    if field_array.typecode == 'f':
        scalar_type = 'Float'
    elif field_array.typecode == 'H':
        scalar_type = 'UnsignedShort'
    else: #field_array.typecode == 'B'
        scalar_type = 'UnsignedByte'

    json_mesh_field = {'Name': field_name, 'Components':components, 'DataBlock': json_blob, 'Type':scalar_type}    
    json_mesh_fields.append(json_mesh_field)

def getBoneIndicesAndWeights(mesh_vertex_groups, vertex, bone_name_to_index):
    bones = []
    for vertex_group in vertex.groups:
        group_name = mesh_vertex_groups[vertex_group.group].name
        bone_index = bone_name_to_index.get(group_name)
        if bone_index is None:
            continue
        bone_weight = vertex_group.weight
        bones.append((bone_index, bone_weight))
        
    if len(bones) < 4:
        dummy_elem_count = 4 - len(bones)
        for i in range(dummy_elem_count):
            bones.append((0, 0))
    elif len(bones) > 4:
        bones.sort(key=lambda index_weight_pair: index_weight_pair[1])
        bones = bones[-4:]
    
    bone_indices, bone_weights = zip(*bones)
    weight_sum = sum(bone_weights)
    if weight_sum > 0:
        bone_weights = [w/weight_sum for w in bone_weights]
        
    return (bone_indices, bone_weights)
    
def writeMesh(binary_file, mesh, mesh_vertex_groups, bone_name_to_index):

    # Be sure tessface & co are available!
    #if not mesh.tessfaces and mesh.polygons:
    #    mesh.calc_tessface()
        
    active_uv_layer = mesh.uv_layers.active
    if active_uv_layer:
        has_uv = True
    else:
        has_uv = False
        
    has_bones = (bone_name_to_index is not None)
    #has_uv = False    
    #if has_uv:
        #mesh.calc_tangents(active_uv_layer.name)
        #bpy.context.scene.update() 
    
    output_positions = array('f', [])
    output_normals = array('f', [])
    output_tangents = array('f', [])
    output_binormals = array('f', [])
    output_uvs = array('f', [])
    output_bone_weights = array('f', [])
    output_bone_indices = array('H', []) #TODO filter used bone
    
    loop_idx = 0
    vertex_count=0
    for face in mesh.polygons:
        smooth = face.use_smooth
        normal = face.normal[:]
        
        iter_vertices = (0, 1, 2)
        if len(face.vertices) >= 4:
            iter_vertices = (0, 1, 2, 2, 3, 0)
        for i in iter_vertices:
            vidx = face.vertices[i]
            vertex = mesh.vertices[vidx]            
            if smooth:
                normal = vertex.normal[:]

            output_positions.extend(vertex.co[:])            
            output_normals.extend(normal[:])
            if has_bones:
                bone_indices, bone_weights = getBoneIndicesAndWeights(mesh_vertex_groups, vertex, bone_name_to_index)
                #print(bone_weights[:])
                output_bone_indices.extend(bone_indices[:])
                output_bone_weights.extend(bone_weights[:])
                
            if has_uv:
                #output_tangents.extend(mesh.loops[loop_idx+i].bitangent[:])
                #output_binormals.extend(mesh.loops[loop_idx].bitangent[:])
                output_uvs.extend(active_uv_layer.data[loop_idx+i].uv.to_2d()[:])                
                #print(active_uv_layer.data[1].uv.to_2d()[:])
                #print(active_uv_layer.data[2].uv.to_2d()[:])
                #print(active_uv_layer.data[3].uv.to_2d()[:])
            vertex_count += 1
        loop_idx += len(face.vertices)
    json_mesh = {'Id':mesh.name, 'TriangleCount':len(mesh.polygons), 'VertexCount':vertex_count} 
    #print(loop_idx*3*4)    
    json_mesh_fields = []    
    writeMeshField(json_mesh_fields, 'position', 3, output_positions, binary_file)
    writeMeshField(json_mesh_fields, 'normal', 3, output_normals, binary_file)
    if has_uv:
        #writeMeshField(json_mesh_fields, 'tangent', 3, output_tangents, binary_file)
        #writeMeshField(json_mesh_fields, 'binormal', 3, output_binormals, binary_file)
        writeMeshField(json_mesh_fields, 'uv', 2, output_uvs, binary_file)    
        
    if has_bones:
        writeMeshField(json_mesh_fields, 'bone_indices', 4, output_bone_indices, binary_file)
        writeMeshField(json_mesh_fields, 'bone_weights', 4, output_bone_weights, binary_file)    
    json_mesh['Fields'] = json_mesh_fields
    
    return json_mesh

    
    
def writeMatrix(mat):
    result = []
    for column_index in range(4):
        for line_index in range(3):
            result.append(mat[line_index][column_index])
    return result

def writeArmature(armature_object, armature_data):        
    bone_name_to_index = {}
    json_bones = []
    i = 0
    for bone in armature_data.bones:
        json_parent = bone.parent.name if (bone.parent is not None) else None
        json_children = [child.name for child in bone.children]
        pose = armature_object.pose.bones[bone.name]
        json_bone_pose = {'Location':pose.location[:], 'RotationQuaternion':pose.rotation_quaternion[:], 'Scale':pose.scale[:], 'PoseMatrix':writeMatrix(pose.matrix)}
        json_bone = {'Name':bone.name, 'Index':i, 'SkeletonToBoneMatrix':writeMatrix(bone.matrix_local), 'Pose':json_bone_pose, 'Parent':json_parent, 'Children':json_children}
        json_bones.append(json_bone)
        bone_name_to_index[bone.name] = i
        i += 1
    json_armature = {'Name': armature_object.name, 'Bones':json_bones, 'WorldToSkeletonMatrix':writeMatrix(armature_object.matrix_world), }
    
    return (json_armature, bone_name_to_index)

def writeArmatures():
    armatures_bone_indices = {}
    json_armatures = []
    for object in bpy.context.scene.objects:
        if object.type != 'ARMATURE':                    
                continue
    
        json_armature, bone_name_to_index = writeArmature(object, object.data)
        armatures_bone_indices[json_armature['Name']] = bone_name_to_index
        json_armatures.append(json_armature)
    return (json_armatures, armatures_bone_indices)
    
def hasMesh(object):
    return hasattr(object.data, "polygons")

def writeInputValue(input):
    if hasattr(input.default_value, '__getitem__'):
        return input.default_value[:] 
    else: 
        return input.default_value

def toScalingMatrix(vector):
    return mathutils.Matrix(((vector.x, 0.0, 0.0), (0.0, vector.y, 0.0), (0.0, 0.0, vector.z))).to_4x4()
        
def writeTexImageNodeProperties(node, json_node, collected_textures):
    transform = node.texture_mapping
    mat_loc = mathutils.Matrix.Translation(transform.translation)
    mat_sca = toScalingMatrix(transform.scale)
    mat_rot = transform.rotation.to_matrix().to_4x4()
    mat_out = mat_loc * mat_rot * mat_sca
    json_node["TransformMatrix"] = writeMatrix(mat_out)
    json_node["Image"] = node.image.name
    collected_textures.add(node.image.name)
   
def writeMathNodeProperties(node, json_node):
    json_node["Clamp"] = node.use_clamp
    json_node["Operation"] = node.operation

def writeVectMathNodeProperties(node, json_node):
    json_node["Operation"] = node.operation    
    
def writeNode(node, collected_textures):
    json_node = {}
    
    json_inputs = []
    for input in node.inputs:
        json_links = [{'Node':link.from_node.name, 'Slot':link.from_socket.identifier } for link in input.links]
        if hasattr(input, 'default_value'):                            
            json_inputs.append({'Name':input.identifier, 'Links':json_links, 'DefaultValue':writeInputValue(input) })
        else:
            json_inputs.append({'Name':input.identifier, 'Links':json_links })
        
    json_outputs = []    
    for output in node.outputs:
        json_links = [{'Node':link.from_node.name, 'Slot':link.from_socket.identifier } for link in output.links]
        json_outputs.append({'Name':output.identifier, 'Links':json_links })
        
    json_node = {'Name': node.name, 'Type': node.type, 'InputSlots':json_inputs, 'OutputSlots':json_outputs}
    
    if node.type == 'TEX_IMAGE':
        writeTexImageNodeProperties(node, json_node, collected_textures)
    elif node.type == 'VECT_MATH':
        writeVectMathNodeProperties(node, json_node)
    elif node.type == 'MATH':
        writeMathNodeProperties(node, json_node)
        
    return json_node
    
def writeMaterials(collected_textures):
    json_materials = []
    already_written_materials = set()
    for object in bpy.context.scene.objects:
        if not object.is_visible(bpy.context.scene):
            continue
        if not hasattr(object.data, "materials"):
            continue
        if not len(object.data.materials):
            continue
        material = object.data.materials[0]
        if material.node_tree is None:
            continue
        if (material.name in already_written_materials):
            continue
        
        json_nodes = []        
        for node in material.node_tree.nodes:
            json_nodes.append(writeNode(node, collected_textures))
        json_materials.append({'Name':material.name, 'Nodes':json_nodes})
        already_written_materials.add(material.name)
    return json_materials

    
def writeTexture(texture_name, output_folder_path):
    image = bpy.data.images[texture_name]
    scene=bpy.context.scene
    scene.render.image_settings.file_format=image.file_format
    texture_path = output_folder_path+texture_name    
    image.save_render(texture_path,scene)    
    return {'Name':texture_name, 'Path':texture_path}
    
def writeTextures(texture_names, output_folder_path):
    json_textures = []
    for texture_name in texture_names:
        try:
            json_textures.append(writeTexture(texture_name, output_folder_path))
        except Exception as e:
            print("Texture export failed: "+texture_name+'\n'+str(e))
    return json_textures
    
def writeEnvironment(output_folder_path):
    try:
        env_image=bpy.context.scene.world.node_tree.nodes['Environment Texture'].image
        env_path = bpy.path.abspath(env_image.filepath, library=env_image.library)
        shutil.copy2(env_path, output_folder_path)
        json_environment = {'Name':env_image.name, 'Path':output_folder_path+env_image.name }
    except Exception as e:
        print("Environment texture export failed: "+str(e))        
        json_environment = None
    return json_environment

def writeLights():
    json_lights = []
    for object in bpy.context.scene.objects:
        if object.type != 'LAMP':                    
            continue   
                
        light_data = object.data
        
        json_light = {'Name':object.name, 'Type':light_data.type, 'Size':light_data.shadow_soft_size}
        json_light['Color'] = light_data.node_tree.nodes['Emission'].inputs[0].default_value[:]
        json_light['Strength'] = light_data.node_tree.nodes['Emission'].inputs[1].default_value
        json_light['WorldToLocalMatrix'] = writeMatrix(object.matrix_world)
        
        if light_data.type == 'AREA':
            json_light['SizeX'] = light_data.size
            if light_data.shape == 'RECTANGLE':
                json_light['SizeY'] = light_data.size_y
            else:
                json_light['SizeY'] = light_data.size
        elif light_data.type == 'SPOT':
            json_light['Angle'] = light_data.spot_size
            json_light['AngleBlend'] = light_data.spot_blend
        else:
            json_light['Size'] = light_data.shadow_soft_size
            
        json_lights.append(json_light)
        
    return json_lights

bone_path_regex = re.compile(r'pose\.bones\["(.*)"\]\.(location|scale|rotation_quaternion)')
transform_path_regex = re.compile(r'(location|scale|rotation_quaternion|rotation_euler)')

def convertDataPath(fcurve):
    match = bone_path_regex.search(fcurve.data_path)
    if match:
        return 'bone/'+match.group(1) +'/'+match.group(2)+'/'+str(fcurve.array_index)
        
    match = transform_path_regex.search(fcurve.data_path)
    if match:
        return 'transform/'+match.group(1) +'/'+str(fcurve.array_index)
    
    return None

def writeAnimationCurves(curves):    
    json_curves = []
    for fcurve in curves:

        property_path = convertDataPath(fcurve)
        if property_path is None:
            continue
        
        json_keyframes = []
        for keyframe in fcurve.keyframe_points:
            json_keyframes.append({'X':keyframe.co.x, 'Y':keyframe.co.y})
        
        json_curve = {'TargetPropertyPath':property_path, 'Keyframes':json_keyframes }
        json_curves.append(json_curve)
    return json_curves

def isValidMeshObject(object):
    return object.is_visible(bpy.context.scene) and hasMesh(object)
    
def isValidAnimatedObject(object):
    if object.animation_data is None:
        return False
            
    if object.animation_data.action is None:
        return False
    
    if object.type == 'ARMATURE':
        return True
    return isValidMeshObject(object)
    
def writeActions():
    json_actions = []
    for object in bpy.context.scene.objects:
        if isValidAnimatedObject(object): 
            curves = object.animation_data.action.fcurves
            json_curves = writeAnimationCurves(curves)
            json_action = {'TargetObject':object.name, 'Curves':json_curves }
            if len(json_curves) > 0:
                json_actions.append(json_action)
    return json_actions
    
def getMeshArmature(object):   
    armature_name = None
    for modifier in object.modifiers:
        if modifier.type == 'ARMATURE' and modifier.object is not None:
            armature_name = modifier.object.name
            break
    return armature_name

def writeObjectTransform(object):    
    json_transform = {'Location':object.location[:], 'RotationType':object.rotation_mode, 'RotationEuler':object.rotation_euler[:], 'RotationQuaternion':object.rotation_quaternion[:], 'Scale':object.scale[:] }
    return json_transform
    
class Export3DY(bpy.types.Operator, ExportHelper):
    bl_idname = "export.3dy"
    bl_label = "Export to 3DY"
    filename_ext = ".3dy"
    filter_glob = StringProperty(default="*.3dy", options={'HIDDEN'})    
    filepath = StringProperty(subtype='FILE_PATH')
    
    def execute(self, context):
        output_path = self.filepath+'/'
        if not os.path.exists(self.filepath):
            os.makedirs(self.filepath)
        
        binary_file = open(output_path+'data.bin', "wb")
        
        textures = set()        
        json_lights = writeLights()
        json_armatures, armatures_bone_indices = writeArmatures()
        json_materials = writeMaterials(textures)
        json_environment = writeEnvironment(output_path)
        json_textures = writeTextures(textures, output_path)
        json_actions = writeActions()
        
        json_surfaces = []
        bpy.ops.wm.console_toggle()
        #bpy.ops.object.make_single_user(type='ALL', object=True, obdata=True)
        object_count = sum(map(hasMesh, bpy.context.scene.objects))
        i=0
        for object in bpy.context.scene.objects:
            if not isValidMeshObject(object):
                continue
            
            armature_name = getMeshArmature(object)
            bone_name_to_index = armatures_bone_indices[armature_name] if (armature_name is not None) else None
            
            mesh = object.data ##to_mesh(bpy.context.scene, True, "PREVIEW")#
            json_mesh = writeMesh(binary_file, mesh, object.vertex_groups, bone_name_to_index)
            if len(object.data.materials) != 0:
                material_name = object.data.materials[0].name
            else:
                material_name = ""
            json_surface = {'Name':object.name, 'Mesh':json_mesh, 'Material':material_name, 'Transform':writeObjectTransform(object), 'WorldToLocalMatrix':writeMatrix(object.matrix_world), 'Skeleton':armature_name}
            json_surfaces.append(json_surface)
            i+= 1
            updateProgress("progress", i/object_count)            
            
        binary_file_size = binary_file.tell()
        binary_file.close()
        
        root = {'Surfaces':json_surfaces} 
        root['Textures'] = json_textures
        root['Materials'] = json_materials
        root['Environment'] = json_environment
        root['Lights'] = json_lights
        root['Skeletons'] = json_armatures
        root['Actions'] = json_actions
        with open(output_path+'structure.json', 'w') as json_file:
            json.dump(root, json_file, indent=1)
        return {'FINISHED'}
    
def menu_func_export(self, context):
    self.layout.operator(Export3DY.bl_idname, text="3DY (.3dy)")

def register():
    bpy.utils.register_module(__name__)    
    bpy.types.INFO_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

# test call
#bpy.ops.export.panda3d_egg('INVOKE_DEFAULT')
