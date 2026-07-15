// Experimental FSR 4.1.1 ML upscaler adapter. Motion is estimated on a
// half-size grid and expanded to render resolution before D3D12 dispatch.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME FSR4 Optical Flow 50% (Experimental)

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
