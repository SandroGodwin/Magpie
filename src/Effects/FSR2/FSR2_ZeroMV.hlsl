// Experimental native FSR 2.2.1 D3D11 Zero-MV adapter.
//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME FSR2 Zero-MV Full-Reactive (Experimental)

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
