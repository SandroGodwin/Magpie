// Experimental XeSS-SR adapter. Native D3D12 XeSS replaces this pass.
// Motion is estimated on a 50% grid, expanded to input resolution, and passed
// as low-resolution motion vectors together with a flat depth texture.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME XeSS Optical Flow 50% (Experimental)

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
