// Experimental FSR 4.1.1 ML upscaler adapter. The native D3D12 backend
// replaces this pass and supplies zero motion plus virtual auxiliary inputs.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME FSR4 Zero-MV (Experimental)

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
MF4 Pass1(float2 pos) { return INPUT.SampleLevel(sam, pos, 0); }
