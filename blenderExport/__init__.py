""" Part of the YABEE
"""

bl_info = {
    "name": "3DY File format",
    "author": "Yvain Raeymaekers",
    "blender": (2, 6, 0),
    "api": 41226,
    "location": "File > Import-Export",
    "description": ("Export to 3dy : meshes"),
    "warning": "Bug free edition.",
    "category": "Import-Export"}
    
from array import array
import json
import bpy
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
    json_mesh_field = {'Name': field_name, 'Components':components, 'DataBlock': json_blob}    
    json_mesh_fields.append(json_mesh_field)
    
def writeMesh(binary_file, mesh):

    # Be sure tessface & co are available!
    #if not mesh.tessfaces and mesh.polygons:
    #    mesh.calc_tessface()
        
    active_uv_layer = mesh.uv_layers.active
    if active_uv_layer:
        has_uv = True
    else:
        has_uv = False
    has_uv = False    
    if has_uv:
        mesh.calc_tangents(active_uv_layer.name)
        
    
    output_positions = array('f', [])
    output_normals = array('f', [])
    output_tangents = array('f', [])
    output_binormals = array('f', [])
    output_uvs = array('f', [])
    
    loop_idx = 0
    for face in mesh.polygons:
        smooth = face.use_smooth
        normal = face.normal[:]
        
        iter_vertices = face.vertices
        if len(face.vertices) >= 4:
            iter_vertices = face.vertices[0],face.vertices[1],face.vertices[2],face.vertices[2], face.vertices[3], face.vertices[0] 
        for vidx in iter_vertices:
            vertex = mesh.vertices[vidx]            
            if smooth:
                normal = vertex.normal[:]

            output_positions.extend(vertex.co[:])            
            output_normals.extend(normal[:])
            if has_uv:
                output_tangents.extend(mesh.loops[loop_idx].tangent[:])
                output_binormals.extend(mesh.loops[loop_idx].bitangent[:])
                output_uvs.extend(active_uv_layer.data[loop_idx].uv.to_2d()[:])
            loop_idx += 1
    json_mesh = {'Id':mesh.name, 'TriangleCount':len(mesh.polygons), 'VertexCount':loop_idx} 
    #print(loop_idx*3*4)    
    json_mesh_fields = []    
    writeMeshField(json_mesh_fields, 'position', 3, output_positions, binary_file)
    writeMeshField(json_mesh_fields, 'normal', 3, output_normals, binary_file)
    if has_uv:
        writeMeshField(json_mesh_fields, 'tangent', 3, output_tangents, binary_file)
        writeMeshField(json_mesh_fields, 'binormal', 3, output_binormals, binary_file)
        writeMeshField(json_mesh_fields, 'uv', 2, output_uvs, binary_file)    
    json_mesh['Fields'] = json_mesh_fields
    
    return json_mesh

def writeMatrix(mat):
    result = []
    for column_index in range(4):
        for line_index in range(3):
            result.append(mat[line_index][column_index])
    return result
    
def hasMesh(object):
    return hasattr(object.data, "polygons")
    
class Export3DY(bpy.types.Operator, ExportHelper):
    bl_idname = "export.3dy"
    bl_label = "Export to 3DY"
    filename_ext = ".3dy"

    filter_glob = StringProperty(
            default="*.3dy",
            options={'HIDDEN'},
            )
    
    def execute(self, context):
        filepath = 'D:/test'
        binary_file = open(filepath+'.bin', "wb")
        json_surfaces = []
        bpy.ops.wm.console_toggle()
        bpy.ops.object.make_single_user(type='ALL', object=True, obdata=True)
        object_count = sum(map(hasMesh, bpy.context.scene.objects))
        i=0
        for object in bpy.context.scene.objects:
            if not hasMesh(object):                    
                    continue
            
            #object.modifiers.new('triangulate_for_export', 'TRIANGULATE') 
            #bpy.context.scene.objects.active = object
            #bpy.ops.object.modifier_apply(modifier = 'triangulate_for_export')

            mesh = object.to_mesh(bpy.context.scene, True, "PREVIEW")
            json_mesh = writeMesh(binary_file, mesh)
            json_surface = {'Name':object.name, 'Mesh':json_mesh, 'WorldToLocalMatrix':writeMatrix(object.matrix_world) }
            json_surfaces.append(json_surface)
            i+= 1
            updateProgress("progress", i/object_count)            
            
        binary_file_size = binary_file.tell()
        binary_file.close()
        
        root = {'DataBlocksFile':{'Path':'test.bin', 'Bytes':binary_file_size }, 'Surfaces':json_surfaces} 
        with open(filepath+'.json', 'w') as json_file:
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
