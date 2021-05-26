package org.eink_onyx_reflections;

public enum UpdateMode {
	None,
	DU,                         // black/white screen update
	GU,                         // 16 level gray partial update
	GU_FAST,                    // fast GU update mode, deprecated
	GC,                         // 16 level gray full screen update
	GCC,
	ANIMATION,                  // black/white screen update, faster than DU, but lower quality
	ANIMATION_QUALITY,          // optimized animation mode with dither
	ANIMATION_MONO,
	ANIMATION_X,
	GC4,                        // 4 level gray full screen update, deprecated
	REGAL,                      // optimized 16 level partial update mode for text pages
	REGAL_D,                    // same as REGAL now ???
	DU_QUALITY,                 // optimized DU mode with dither
	HAND_WRITING_REPAINT_MODE
}
