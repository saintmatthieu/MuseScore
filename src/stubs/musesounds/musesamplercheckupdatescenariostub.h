#pragma once

#include "musesounds/imusesamplercheckupdatescenario.h"

namespace mu::musesounds
{
class MuseSamplerCheckUpdateScenarioStub
    : public IMuseSamplerCheckUpdateScenario
{
public:
  bool alreadyChecked() const { return true; }
  void checkAndShowUpdateIfNeed() {}
};
} // namespace mu::musesounds
