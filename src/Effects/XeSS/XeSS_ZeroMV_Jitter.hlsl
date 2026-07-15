// Experimental XeSS-SR adapter with metadata-only jitter.
// The native D3D12 backend replaces this pass at runtime.
//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME XeSS Zero-MV Jitter (Experimental)

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
