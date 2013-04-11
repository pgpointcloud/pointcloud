To Do
=====

..IMPORTANT..!!!
- *** dimensional compression and PC_Uncompress probably don't play will together... once serialized, patches don't know their compression anymore, so it's possible to pass a NONE patch up into deserialize and have it treated as a DIMENSIONAL

- (?) convert PCBYTES to use PCDIMENSION* instead of holding all values as dupes
- (??) convert PCBYTES handling to pass-by-reference instead of pass-by-value

- implement PC_PatchAvg/PC_PatchMin/PC_PatchMax as C functions against patches with dimensional and uncompressed implementations
- update pc_patch_from_patchlist to merge dimensional patchlists directly
- TESTS for pc_patch_dimensional_from_uncompressed() and pc_patch_dimensional_compress()

- Add Min/Max values to dimensional compression serialization (???)


Use Cases to Support
--------------------

- frustrum filtering
- raster overlaying
- filtering on attribute values
- filtering on spatial polygons (in *and* out)
- find the k nearest points to point P
- gridding/binning ("avg intensity per cell", "max z per cell", "agv red per cell", "rgb into grid/picture")
- reprojection / re-schema

More Functions
--------------

- PC_FilterEquals(patch, dimension, value) returns patch
- PC_FilterLessThan(patch, dimension, value) returns patch
- PC_FilterGreaterThan(patch, dimension, value) returns patch
- PC_FilterBetween(patch, dimension, valuemin, valuemax) returns patch
- PC_Filter(patch, dimension, expression) returns patch
