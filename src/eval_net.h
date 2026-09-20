#ifndef EVAL_NET_H
#define EVAL_NET_H

#include "board.h"
#include <string>

// 768 STM-relative piece-square features -> 32 -> 32 -> 1.
// Output is white-positive, same sign convention as the PST eval.
namespace EvalNet {

constexpr int kInput  = 768;
constexpr int kHidden = 32;

bool load(const std::string& path);
bool loaded();
const std::string& path();
int  evaluate(const Board& board);

}  // namespace EvalNet

#endif
