// Experimental DLSS-SR adapter with subpixel jitter metadata.
// Native NGX D3D11 code replaces this pass at runtime.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME DLSS Zero-MV Jitter (Experimental)

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
