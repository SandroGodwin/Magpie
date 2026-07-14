// Experimental XeSS-SR adapter. The native D3D12 XeSS code replaces this pass.
// Colour is shared from Magpie's D3D11 renderer; high-resolution motion vectors
// are cleared, flat depth is supplied, and jitter is fixed at zero.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME XeSS Zero-MV (Experimental)

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
