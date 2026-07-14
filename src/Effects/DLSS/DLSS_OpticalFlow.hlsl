// DLSS-SR with colour-only half-resolution optical-flow motion vectors.
//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME DLSS Optical Flow 50% (Experimental)
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
