# -*- cmake -*-
include(Prebuilt)

set(LLCONVEXDECOMP_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  
if (INSTALL_PROPRIETARY AND NOT STANDALONE)
  use_prebuilt_binary(llconvexdecomposition)
  set(LLCONVEXDECOMP_LIBRARY llconvexdecomposition)
else (INSTALL_PROPRIETARY AND NOT STANDALONE)
  use_prebuilt_binary(llconvexdecompositionstub)
  
  # NickyD/Wolfpup HACD based convex decomp
	set(LLCONVEXDECOMP_LIBRARY nd_hacdConvexDecomposition hacd)

endif (INSTALL_PROPRIETARY AND NOT STANDALONE)
