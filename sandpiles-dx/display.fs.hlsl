Texture2D colorMap;
SamplerState samplerState;

struct fIn
{
  float4 pos: SV_POSITION;
  float2 texCoord: TEXCOORD0;
};

float4 main(fIn f) : SV_TARGET
{
  return colorMap.Sample(samplerState, f.texCoord);
}
