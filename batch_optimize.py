#!/usr/bin/env python3
"""
CannaVille Pro - Blender Batch Optimization Script
Converts raw 3D models to optimized GLB files with LOD levels
Usage: blender --background --python tools/batch_optimize.py -- raw_path output_path
"""

import bpy
import sys
import pathlib
import bmesh
import mathutils

def clear_scene():
    """Clear all objects from the scene"""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

def import_model(filepath):
    """Import model based on file extension"""
    filepath = pathlib.Path(filepath)
    
    if filepath.suffix.lower() == '.fbx':
        bpy.ops.import_scene.fbx(filepath=str(filepath))
    elif filepath.suffix.lower() == '.obj':
        bpy.ops.import_scene.obj(filepath=str(filepath))
    elif filepath.suffix.lower() in ['.gltf', '.glb']:
        bpy.ops.import_scene.gltf(filepath=str(filepath))
    elif filepath.suffix.lower() == '.3ds':
        bpy.ops.import_scene.autodesk_3ds(filepath=str(filepath))
    else:
        print(f"Unsupported file format: {filepath.suffix}")
        return None
    
    return bpy.context.selected_objects[0] if bpy.context.selected_objects else None

def optimize_materials(obj):
    """Optimize materials for PBR workflow"""
    if not obj.data.materials:
        return
    
    for material in obj.data.materials:
        if material and material.use_nodes:
            # Ensure PBR setup
            nodes = material.node_tree.nodes
            principled = None
            
            for node in nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    principled = node
                    break
            
            if not principled:
                # Create principled BSDF if it doesn't exist
                principled = nodes.new(type='ShaderNodeBsdfPrincipled')
                output = nodes.get('Material Output')
                if output:
                    material.node_tree.links.new(principled.outputs['BSDF'], output.inputs['Surface'])

def create_lod_levels(obj, output_path, base_name):
    """Create LOD levels with different polygon counts"""
    lod_configs = [
        {'ratio': 1.0, 'name': 'LOD0'},
        {'ratio': 0.35, 'name': 'LOD1'},
        {'ratio': 0.05, 'name': 'LOD2'}
    ]
    
    for config in lod_configs:
        # Clear selection
        bpy.ops.object.select_all(action='DESELECT')
        
        # Duplicate object
        dup = obj.copy()
        dup.data = obj.data.copy()
        bpy.context.collection.objects.link(dup)
        
        # Select and make active
        dup.select_set(True)
        bpy.context.view_layer.objects.active = dup
        
        # Apply decimation if not LOD0
        if config['ratio'] < 1.0:
            # Add decimate modifier
            decimate = dup.modifiers.new(name='Decimate', type='DECIMATE')
            decimate.ratio = config['ratio']
            
            # Apply modifier
            bpy.context.view_layer.objects.active = dup
            bpy.ops.object.modifier_apply(modifier='Decimate')
        
        # Optimize materials
        optimize_materials(dup)
        
        # Export GLB
        output_file = output_path / f"{base_name}_{config['name']}.glb"
        
        bpy.ops.export_scene.gltf(
            filepath=str(output_file),
            use_selection=True,
            export_format='GLB',
            export_draco_mesh_compression_enable=True,
            export_draco_mesh_compression_level=6,
            export_materials='PBR',
            export_image_format='JPEG',
            export_texture_dir='',
            export_yup=False  # Use Z-up for Three.js
        )
        
        print(f"Exported: {output_file}")
        
        # Remove duplicate
        bpy.data.objects.remove(dup, do_unlink=True)

def process_cannabis_plant(obj, output_path, stage):
    """Special processing for cannabis plants"""
    # Ensure proper scale (1 unit = 1 meter)
    if stage == 'seedling':
        obj.scale = (0.2, 0.2, 0.2)
    elif stage == 'vegetative':
        obj.scale = (0.6, 0.6, 0.6)
    elif stage == 'flowering':
        obj.scale = (1.0, 1.0, 1.0)
    elif stage == 'harvest':
        obj.scale = (1.0, 1.0, 1.0)
    
    # Apply scale
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    
    create_lod_levels(obj, output_path, f"cannabis_{stage}")

def process_hydro_component(obj, output_path, component_name):
    """Special processing for hydroponic components"""
    # Ensure proper scale based on real-world measurements
    if component_name == 'bucket':
        # 5-gallon bucket: 0.305m diameter x 0.37m height
        obj.scale = (0.305, 0.305, 0.37)
    elif component_name == 'led_board':
        # LED quantum board: 0.60m x 0.60m x 0.05m
        obj.scale = (0.60, 0.60, 0.05)
    elif component_name == 'tray':
        # Aluminum tray: 1.15m x 1.15m x 0.06m
        obj.scale = (1.15, 1.15, 0.06)
    
    # Apply scale
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    
    create_lod_levels(obj, output_path, component_name)

def create_pvc_network():
    """Create PVC pipe network from primitives"""
    # Main rail: 1.05m length, 0.032m diameter
    bpy.ops.mesh.primitive_cylinder_add(
        radius=0.016,  # 0.032m diameter = 0.016m radius
        depth=1.05,
        location=(0, 0, 0)
    )
    main_rail = bpy.context.active_object
    main_rail.name = "PVC_Main_Rail"
    
    # T-junctions every 0.27m
    for i in range(4):  # 3 T-junctions + end caps
        x_pos = -0.405 + (i * 0.27)  # Center the rail
        
        # Vertical connector
        bpy.ops.mesh.primitive_cylinder_add(
            radius=0.016,
            depth=0.1,
            location=(x_pos, 0, -0.05),
            rotation=(1.5708, 0, 0)  # 90 degrees in radians
        )
        connector = bpy.context.active_object
        connector.name = f"PVC_Connector_{i}"
        
        # Join to main rail
        bpy.ops.object.select_all(action='DESELECT')
        main_rail.select_set(True)
        connector.select_set(True)
        bpy.context.view_layer.objects.active = main_rail
        bpy.ops.object.join()
    
    return main_rail

def create_outdoor_tile():
    """Create outdoor grass tile with soil holes"""
    # Create 1m x 1m grass plane
    bpy.ops.mesh.primitive_plane_add(size=1.0, location=(0, 0, 0))
    grass_tile = bpy.context.active_object
    grass_tile.name = "Grass_Tile"
    
    # Subdivide for detail
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.subdivide(number_cuts=10)
    
    # Add subtle vertex noise
    bpy.ops.mesh.noise(factor=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    
    # Create 5x5 grid of holes
    for x in range(5):
        for y in range(5):
            # Calculate position (0.6m spacing, centered)
            pos_x = -1.2 + (x * 0.6)
            pos_y = -1.2 + (y * 0.6)
            
            # Create hole cylinder for boolean
            bpy.ops.mesh.primitive_cylinder_add(
                radius=0.15,  # 0.30m diameter = 0.15m radius
                depth=0.25,
                location=(pos_x, pos_y, -0.125)
            )
            hole = bpy.context.active_object
            hole.name = f"Hole_{x}_{y}"
            
            # Boolean difference
            bpy.ops.object.select_all(action='DESELECT')
            grass_tile.select_set(True)
            bpy.context.view_layer.objects.active = grass_tile
            
            bool_mod = grass_tile.modifiers.new(name=f"Boolean_{x}_{y}", type='BOOLEAN')
            bool_mod.operation = 'DIFFERENCE'
            bool_mod.object = hole
            
            # Apply modifier
            bpy.ops.object.modifier_apply(modifier=f"Boolean_{x}_{y}")
            
            # Remove hole object
            bpy.data.objects.remove(hole, do_unlink=True)
    
    return grass_tile

def main():
    """Main processing function"""
    if len(sys.argv) < 7:  # blender args + -- + our args
        print("Usage: blender --background --python batch_optimize.py -- raw_path output_path")
        return
    
    # Get arguments after --
    args = sys.argv[sys.argv.index("--") + 1:]
    raw_path = pathlib.Path(args[0])
    output_path = pathlib.Path(args[1])
    
    # Ensure output directory exists
    output_path.mkdir(parents=True, exist_ok=True)
    
    print(f"Processing models from: {raw_path}")
    print(f"Output directory: {output_path}")
    
    # Process raw model files
    supported_formats = {'.fbx', '.obj', '.gltf', '.glb', '.3ds'}
    
    for model_file in raw_path.rglob("*"):
        if model_file.suffix.lower() in supported_formats:
            print(f"\nProcessing: {model_file}")
            
            # Clear scene
            clear_scene()
            
            # Import model
            obj = import_model(model_file)
            if not obj:
                continue
            
            # Convert Y-up to Z-up for Three.js
            obj.rotation_euler[0] = -1.5708  # -90 degrees in radians
            bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
            
            # Determine processing type based on filename
            filename = model_file.stem.lower()
            
            if 'cannabis' in filename or 'plant' in filename:
                # Determine growth stage
                if 'seedling' in filename:
                    stage = 'seedling'
                elif 'veg' in filename or 'vegetative' in filename:
                    stage = 'vegetative'
                elif 'flower' in filename or 'bloom' in filename:
                    stage = 'flowering'
                elif 'harvest' in filename:
                    stage = 'harvest'
                else:
                    stage = 'vegetative'  # default
                
                process_cannabis_plant(obj, output_path, stage)
                
            elif 'bucket' in filename:
                process_hydro_component(obj, output_path, 'bucket')
                
            elif 'led' in filename or 'light' in filename:
                process_hydro_component(obj, output_path, 'led_board')
                
            elif 'tray' in filename:
                process_hydro_component(obj, output_path, 'tray')
                
            else:
                # Generic processing
                create_lod_levels(obj, output_path, model_file.stem)
    
    # Create procedural assets
    print("\nCreating procedural assets...")
    
    # PVC network
    clear_scene()
    pvc = create_pvc_network()
    create_lod_levels(pvc, output_path, "pvc_network")
    
    # Outdoor tile
    clear_scene()
    tile = create_outdoor_tile()
    create_lod_levels(tile, output_path, "outdoor_tile")
    
    print("\nBatch optimization complete!")

if __name__ == "__main__":
    main()

