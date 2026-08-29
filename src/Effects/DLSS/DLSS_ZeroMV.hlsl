// Experimental DLSS-SR adapter. Native NGX D3D11 code replaces this pass.
// It receives cleared depth and low-resolution motion-vector textures and is
// not equivalent to an in-engine DLSS integration.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME DLSS SR_Experimental

//!PARAMETER
//!LABEL Use Motion Vectors
//!DEFAULT 1
//!MIN 0
//!MAX 1
//!STEP 1
float useMotionVectors;

//!PARAMETER
//!LABEL Use Estimated Depth (Experimental)
//!DEFAULT 0
//!MIN 0
//!MAX 1
//!STEP 1
float useEstimatedDepth;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
Texture2D OUTPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT

MF4 Pass1(float2 pos) {
	return INPUT.SampleLevel(sam, pos, 0);
}
