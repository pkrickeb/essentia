#include "mute.h"

using namespace essentia;
using namespace standard;

const char* Mute::name = "Mute";
const char* Mute::category = "Statistics";
const char* Mute::description = DOC("This algorithm mutes an audio signal.");

void Mute::compute() {
  
  const std::vector<Real>& signal = _signal.get();
  std::vector<Real>& muted = _muted.get();

  int size = signal.size();
  muted.resize(size);
  for (int i = 0; i < size; ++i) {
    muted[0] = 0;
  }
}
