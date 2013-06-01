TEMPLATE = subdirs
CONFIG += ordered

# Libraries
SUBDIRS += NURBS
SUBDIRS += DynamicVoxel
SUBDIRS += Voxeler
SUBDIRS += TopoBlenderLib
SUBDIRS += Reconstruction
SUBDIRS += GlSplatRendererLib

# Plugins
SUBDIRS += resample
SUBDIRS += segment
SUBDIRS += dynamic_voxel
SUBDIRS += voxel_resampler
SUBDIRS += nurbs_plugin
SUBDIRS += visiblity_resampler

SUBDIRS += topo-blend  # Main UI for topo-blending

# Dependecy map
nurbs_plugin.depends = NURBS
dynamic_voxel.depends = DynamicVoxel
TopoBlenderLib.depends = GlSplatRendererLib
topo-blend.depends = GlSplatRendererLib NURBS DynamicVoxel TopoBlenderLib 
