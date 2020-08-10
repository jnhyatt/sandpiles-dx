Texture2D<uint> sandpile;

struct fIn
{
  float4 pos: SV_POSITION;
};

static float4 colors[8] =
{
  float4(0.0f, 0.0f, 0.0f, 1.0f),
  float4(0.114f, 0.043f, 0.271f, 1.0f),
  float4(0.329f, 0.075f, 0.427f, 1.0f),
  float4(0.529f, 0.129f, 0.420f, 1.0f),
  float4(0.733f, 0.212f, 0.329f, 1.0f),
  float4(0.882f, 0.337f, 0.208f, 1.0f),
  float4(0.976f, 0.549f, 0.035f, 1.0f),
  float4(0.976f, 0.788f, 0.196f, 1.0f)
};

float4 main(fIn f) : SV_TARGET
{
  uint center = sandpile.Load(int3(int(f.pos.x), int(f.pos.y), 0));
  return colors[min(7, center)];
}
