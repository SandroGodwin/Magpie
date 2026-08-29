//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME Frame Guidance - Motion

//!PARAMETER
//!LABEL Display Gain
//!DEFAULT 0.08
//!MIN 0.005
//!MAX 1
//!STEP 0.005
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
