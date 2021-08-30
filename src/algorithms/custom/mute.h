#ifndef ESSENTIA_MUTE_H
#define ESSENTIA_MUTE_H

#include "algorithm.h"

namespace essentia {
namespace standard {

class Mute : public Algorithm {
 
 protected:
  Input<std::vector<Real> > _signal;
  Output<std::vector<Real> > _muted;

 public:
  Mute() {
    declareInput(_signal, "signal", "the input signal");
    declareOutput(_muted, "signal", "the muted signal");
  }

  void declareParameters() {}  

  void compute();

  static const char* name;
  static const char* category;
  static const char* description;

};

} // namespace standard
} // namespace essentia

#endif // ESSENTIA_MUTE_H
