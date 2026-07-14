// NVIDIA VideoSuperRes native backend replaces this placeholder pass.
//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME RTX Video VSR Upscale High

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
