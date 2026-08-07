// Experimental XeSS Frame Generation marker effect. The renderer keeps this
// pass in the regular effect chain while XeSSFGPresenter owns interpolation
// and presentation through Intel's D3D12 proxy swap chain.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME XeSS Frame Generation x2 Zero-MV (No Smooth Motion)

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
