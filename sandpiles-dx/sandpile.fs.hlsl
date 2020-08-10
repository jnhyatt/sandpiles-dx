Texture2D<uint> sandpile;

struct fIn
{
  float4 pos: SV_POSITION;
};

uint main(fIn f) : SV_TARGET
{
  uint center = sandpile.Load(int3(int(f.pos.x), int(f.pos.y), 0));
  uint up = sandpile.Load(int3(int(f.pos.x), int(f.pos.y - 1), 0));
  uint upRight = sandpile.Load(int3(int(f.pos.x + 1), int(f.pos.y - 1), 0));
  uint right = sandpile.Load(int3(int(f.pos.x + 1), int(f.pos.y), 0));
  uint downRight = sandpile.Load(int3(int(f.pos.x + 1), int(f.pos.y + 1), 0));
  uint down = sandpile.Load(int3(int(f.pos.x), int(f.pos.y + 1), 0));
  uint downLeft = sandpile.Load(int3(int(f.pos.x - 1), int(f.pos.y + 1), 0));
  uint left = sandpile.Load(int3(int(f.pos.x - 1), int(f.pos.y), 0));
  uint upLeft = sandpile.Load(int3(int(f.pos.x - 1), int(f.pos.y - 1), 0));

  uint incUp =        uint(up >= 8u);
  uint incUpRight =   uint(upRight >= 8u);
  uint incRight =     uint(right >= 8u);
  uint incDownRight = uint(downRight >= 8u);
  uint incDown =      uint(down >= 8u);
  uint incDownLeft =  uint(downLeft >= 8u);
  uint incLeft =      uint(left >= 8u);
  uint incUpLeft =    uint(upLeft >= 8u);

  uint inc = incUp + incUpRight + incRight + incDownRight + incDown + incDownLeft + incLeft + incUpLeft;

  uint dec = 8u * uint(center >= 8u);

  return center + inc - dec;
}
