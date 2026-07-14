// Experimental FSR 3.1.5 upscaler-only adapter. The native D3D12 backend
// replaces this pass and receives explicit virtual temporal inputs.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME FSR3 Zero-MV (Experimental)

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
