//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME Frame Guidance - Depth

//!PARAMETER
//!LABEL Display Gain
//!DEFAULT 1
//!MIN 0.05
//!MAX 4
//!STEP 0.05
float gain;
//!PARAMETER
//!LABEL Invert Display
//!DEFAULT 0
//!MIN 0
//!MAX 1
//!STEP 1
int invert;
//!PARAMETER
//!LABEL Use Percentile Clipping
//!DEFAULT 1
//!MIN 0
//!MAX 1
//!STEP 1
int percentileClip;

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
