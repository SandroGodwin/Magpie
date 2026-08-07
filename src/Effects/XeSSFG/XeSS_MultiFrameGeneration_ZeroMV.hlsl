// Experimental XeSS Multi-Frame Generation marker effect. The requested
// multiplier is clamped to the maximum reported by the active GPU/driver.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME XeSS Multi-Frame Generation x2-x4 Zero-MV (No Smooth Motion)

//!PARAMETER
//!LABEL Frame Multiplier
//!DEFAULT 2
//!MIN 2
//!MAX 4
//!STEP 1
float multiplier;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
Texture2D OUTPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT

MF4 Pass1(float2 pos) {
	return INPUT.SampleLevel(sam, pos, 0);
}
