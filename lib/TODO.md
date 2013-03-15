To Do
=====

..IMPORTANT..!!!
- *** dimensional compression and PC_Uncompress probably don't play will together... once serialized, patches don't know their compression anymore, so it's possible to pass a NONE patch up into deserialize and have it treated as a DIMENSIONAL

- (?) convert PCBYTES to use PCDIMENSION* instead of holding all values as dupes
- (??) convert PCBYTES handlign to pass-by-reference instead of pass-by-value

- implement PC_PatchAvg/PC_PatchMin/PC_PatchMax as C functions against patches with dimensional and uncompressed implementations
- update pc_patch_from_patchlist to merge dimensional patchlists directly


- TESTS for pc_patch_dimensional_from_uncompressed() and pc_patch_dimensional_compress()
