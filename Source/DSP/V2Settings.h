#pragma once
#include <array>
namespace hpeq::dsp::v2
{
enum Index { Mode,Page,Width,LowWidth,MidWidth,HighWidth,LowSplit,HighSplit,MonoBass,Generator,Generate,Guard,Floor,
             SpaceType,Space,Size,Depth,Damping,Predelay,Position,Spread,Balance,Source,Profile,Motion,Unit,Loop,Rate,Nodes,Mix,Output,Mono,NodeStart };
constexpr int PositionOn=NodeStart+16*7;
constexpr int Restart=PositionOn+1,EditNode=Restart+1;
constexpr int kControlCount=EditNode+1;
using Controls=std::array<float,kControlCount>;
}
