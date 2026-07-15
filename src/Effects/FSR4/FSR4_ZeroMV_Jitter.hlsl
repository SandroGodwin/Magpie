// Experimental FSR 4.1.1 ML upscaler adapter with metadata-only jitter.
// Zero motion and the existing virtual auxiliary inputs remain unchanged.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME FSR4 Zero-MV Jitter (Experimental)

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
