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

def clamp(x, a, b):
    return max(min(x,b), a)
    
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
        #writeMatrix(pose.matrix)
        json_bone = {'Name':bone.name, 'Index':i, 'SkeletonToBoneMatrix':writeMatrix(bone.matrix_local), 'Pose':writeObjectTransform(pose), 'Parent':json_parent, 'Children':json_children}
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
    
def writeMixRgbNodeProperties(node, json_node):
    json_node["Clamp"] = node.use_clamp
    json_node["Operation"] = node.blend_type

def writeVectMathNodeProperties(node, json_node):
    json_node["Operation"] = node.operation    

def writeColorRampNodeProperties(node, json_node, binary_file):
    colors = array('H', [])
    channel_count = 4 if node.outputs['Alpha'].links else 3
    for i in range(0, 256):
        color = node.color_ramp.evaluate(float(i)/255.0)[:]
        color_in_ushort = [int(val*65535) for val in color]
        colors.extend(color_in_ushort[:channel_count])
    json_node['Colors'] = {'DataBlock': writeDataBlockToFile(colors, binary_file), 'Type':'UnsignedShort', 'Channels':channel_count}   
 
def writeCurveRgbNodeProperties(node, json_node, binary_file):
    red_curve = array('H', [])
    green_curve = array('H', [])
    blue_curve = array('H', [])
    curves = node.mapping.curves
    
    for i in range(0, 256):
        t = float(i)/255.0
        red = curves[0].evaluate(curves[3].evaluate(t))
        green = curves[1].evaluate(curves[3].evaluate(t))
        blue = curves[2].evaluate(curves[3].evaluate(t))
        
        red_curve.extend([clamp(int(red*65535), 0, 65535)])
        green_curve.extend([clamp(int(green*65535), 0, 65535)])
        blue_curve.extend([clamp(int(blue*65535), 0, 65535)])

    json_node['RedCurve'] = {'DataBlock': writeDataBlockToFile(red_curve, binary_file), 'Type':'UnsignedShort'}
    json_node['GreenCurve'] = {'DataBlock': writeDataBlockToFile(green_curve, binary_file), 'Type':'UnsignedShort'}  
    json_node['BlueCurve'] = {'DataBlock': writeDataBlockToFile(blue_curve, binary_file), 'Type':'UnsignedShort'}
    
def forwardGroupOutputLink(group_name, node_link):
    group_node = node_link.from_node
    #print(group_node.name)
    group_output_node = group_node.node_tree.nodes['Group Output']
    group_output_node_input = next(input for input in group_output_node.inputs if input.identifier==node_link.from_socket.identifier)
    link = group_output_node_input.links[0]
    return [{'Node':group_name+group_node.name+link.from_node.name, 'Slot':link.from_socket.identifier }]
    
def forwardGroupInputLink(group_node, group_name, node_link):
    group_input_node = node_link.from_node
    group_node_input = next(input for input in group_node.inputs if input.identifier==node_link.from_socket.identifier)
    print(group_node_input.name)
    if len(group_node_input.links)==0:
        return [],group_node_input
    link = group_node_input.links[0]
    group_name_outside_of_group = group_name[:-len(group_node.name)]
    return [{'Node':group_name_outside_of_group+link.from_node.name, 'Slot':link.from_socket.identifier }],group_node_input
    
def writeNode(node, group, group_name, json_nodes, collected_textures, binary_file):
    if node.type == 'GROUP_INPUT' or node.type == 'GROUP_OUTPUT':
        return    
    
    if node.type == 'GROUP':
        for child_node in node.node_tree.nodes:
            writeNode(child_node, node, group_name+node.name, json_nodes, collected_textures, binary_file)
        return
    
    json_inputs = []
    for input in node.inputs:
        group_input = input
        if len(input.links)==1 and input.links[0].from_node.type == 'GROUP':
            json_links = forwardGroupOutputLink(group_name, input.links[0])
        elif len(input.links)==1 and input.links[0].from_node.type == 'GROUP_INPUT':
            json_links,group_input = forwardGroupInputLink(group, group_name, input.links[0])
        else:
            json_links = [{'Node':group_name+link.from_node.name, 'Slot':link.from_socket.identifier } for link in input.links]
        if hasattr(input, 'default_value'):                            
            json_inputs.append({'Name':input.identifier, 'Links':json_links, 'DefaultValue':writeInputValue(group_input) })
        else:
            json_inputs.append({'Name':input.identifier, 'Links':json_links })
        
    #json_outputs = []    
    #for output in node.outputs:
    #    json_links = [{'Node':link.from_node.name, 'Slot':link.from_socket.identifier } for link in output.links]
    #    json_outputs.append({'Name':output.identifier, 'Links':json_links })
        
    #json_node = {'Name': group_name+node.name, 'Type': node.type, 'InputSlots':json_inputs, 'OutputSlots':json_outputs}
    json_node = {'Name': group_name+node.name, 'Type': node.type, 'InputSlots':json_inputs, 'OutputSlots':[]}
    
    if node.type == 'TEX_IMAGE':
        writeTexImageNodeProperties(node, json_node, collected_textures)
    elif node.type == 'VECT_MATH':
        writeVectMathNodeProperties(node, json_node)
    elif node.type == 'MATH':
        writeMathNodeProperties(node, json_node)
    elif node.type == 'MIX_RGB':
        writeMixRgbNodeProperties(node, json_node)
    elif node.type == 'VALTORGB':
        writeColorRampNodeProperties(node, json_node, binary_file)
    elif node.type == 'CURVE_RGB':
        writeCurveRgbNodeProperties(node, json_node, binary_file)
        
    json_nodes.append(json_node)
        
def writeMaterials(collected_textures, binary_file):
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
            writeNode(node, None, '', json_nodes, collected_textures, binary_file)
        json_materials.append({'Name':material.name, 'Nodes':json_nodes})
        already_written_materials.add(material.name)
    return json_materials

    
def writeTexture(texture_name, output_folder_path):
    image = bpy.data.images[texture_name]
    scene=bpy.context.scene
    scene.render.image_settings.file_format=image.file_format
    if image.file_format=='PNG':
        scene.render.image_settings.color_mode='RGBA'
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
        if object.type != 'LAMP' or not object.is_visible(bpy.context.scene):                    
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

def convertDataPath(fcurve, object):
    match = bone_path_regex.search(fcurve.data_path)
    if match:
        return 'bone/'+match.group(1) +'/'+match.group(2)+'/'+str(fcurve.array_index)
        
    match = transform_path_regex.search(fcurve.data_path)
    if match and object.type != 'ARMATURE': #TODO animate skeleton tranform
        return 'transform/'+match.group(1) +'/'+str(fcurve.array_index)
    
    return None

def isConstantCurve(fcurve):
    return len(fcurve.keyframe_points)==1 or (len(fcurve.keyframe_points) == 2 and fcurve.keyframe_points[0].co.y == fcurve.keyframe_points[1].co.y)
    
def writeAnimationCurves(curves, object):    
    json_curves = []
    for fcurve in curves:

        property_path = convertDataPath(fcurve, object)
        if property_path is None:
            continue
        
        if isConstantCurve(fcurve):
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
            json_curves = writeAnimationCurves(curves, object)
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

def getTopParent(object):
    while object.parent != None:
        object = object.parent
    return object
 
def writeObjectTransform(object):    
    json_transform = {'Location':object.location[:], 'RotationType':object.rotation_mode, 'RotationEuler':object.rotation_euler[:], 'RotationQuaternion':object.rotation_quaternion[:], 'Scale':object.scale[:] }
    if object.rotation_mode == 'QUATERNION':
        json_transform['Rotation'] = object.rotation_quaternion[:]
    elif object.rotation_mode == 'AXIS_ANGLE':
        json_transform['Rotation'] = object.rotation_axis_angle[:]
    else:
        json_transform['Rotation'] = object.rotation_euler[:] + tuple([0.0])
    return json_transform

def writeTransformHierarchyNodes(object_names, json_nodes):
    created_nodes = []
    for object_name in object_names:
        object = bpy.data.objects[object_name]
        children_names = [child.name for child in object.children]
        parent_name = object.parent.name if (object.parent is not None) else None
        json_node = {'ObjectName':object.name, 'ParentToNodeMatrix':writeMatrix(object.matrix_parent_inverse), 'Transform':writeObjectTransform(object), 'Parent':parent_name, 'Children':children_names }
        json_nodes.append(json_node)
        created_nodes.append(json_node)
        
    for json_node in created_nodes:
        writeTransformHierarchyNodes(json_node['Children'], json_nodes)
    
def writeTransformHierarchy():
    root_children = set()
    for object in bpy.context.scene.objects:
        if object.is_visible(bpy.context.scene) and (object.type == 'ARMATURE' or object.type == 'MESH'):                    
            root_children.add(getTopParent(object).name)
            
    json_nodes = []
    json_root_transform = {'Location':[0,0,0], 'RotationType':'XYZ', 'Rotation':[0,0,0,0], 'Scale':[1,1,1] }
    json_nodes.append({'ObjectName':'Root', 'ParentToNodeMatrix':writeMatrix(mathutils.Matrix.Identity(4)), 'Transform':json_root_transform, 'Parent':None, 'Children':list(root_children) })
    writeTransformHierarchyNodes(root_children, json_nodes)
    json_hierarchy = {'Nodes':json_nodes}
    return json_hierarchy

def getSurfaceCenter(surface):
    center = mathutils.Vector((0.0, 0.0, 0.0))    
    for i in range(0,8):
        bbox_corner = mathutils.Vector(surface.bound_box[i][:])
        center += bbox_corner;       
        
    return (center/8.0)[:]
    
def writeSurfaces(armatures_bone_indices, binary_file):
    json_surfaces = []
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
        json_surface = {'Name':object.name, 'CenterInLocal':getSurfaceCenter(object), 'Mesh':json_mesh, 'Material':material_name, 'WorldToLocalMatrix':writeMatrix(object.matrix_world), 'Skeleton':armature_name}
        json_surfaces.append(json_surface)
        i+= 1
        updateProgress("progress", i/object_count)
    return json_surfaces

    
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
        bpy.ops.wm.console_toggle()
        
        json_root = {} 
        textures = set()        
        json_skeletons, armatures_bone_indices = writeArmatures()
        json_root['Skeletons'] = json_skeletons
        json_root['Lights'] = writeLights()        
        json_root['Materials'] = writeMaterials(textures, binary_file)
        json_root['Environment'] = writeEnvironment(output_path)
        json_root['Textures'] = writeTextures(textures, output_path)
        json_root['Actions'] = writeActions()
        json_root['TransformHierarchy'] = writeTransformHierarchy()        
        json_root['Surfaces'] = writeSurfaces(armatures_bone_indices, binary_file)

        binary_file_size = binary_file.tell()
        binary_file.close()
        
        with open(output_path+'structure.json', 'w') as json_file:
            json.dump(json_root, json_file, indent=1)
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
