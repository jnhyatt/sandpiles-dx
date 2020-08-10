struct vIn
{
  float2 pos: POSITION;
};

struct fIn
{
  float4 pos: SV_POSITION;
  float2 texCoord: TEXCOORD0;
};

fIn main(vIn v)
{
  fIn o;
  o.pos = float4(v.pos, 0.0f, 1.0f);
  o.texCoord = 0.5f * (v.pos + float2(1.0f, 1.0f));
  return o;
}
