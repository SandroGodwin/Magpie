//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME Frame Guidance - Depth Residual

//!PARAMETER
//!LABEL Display Gain
//!DEFAULT 8
//!MIN 0.5
//!MAX 32
//!STEP 0.5
float gain;

//!TEXTURE
Texture2D INPUT;
//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
Texture2D OUTPUT;
//!SAMPLER
//!FILTER LINEAR
SamplerState sam;
//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT
MF4 Pass1(float2 pos) { return INPUT.SampleLevel(sam, pos, 0); }
