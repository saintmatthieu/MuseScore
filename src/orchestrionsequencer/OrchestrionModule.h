#pragma once

#include "modularity/imodulesetup.h"

namespace dgk
{
class Orchestrion;

class OrchestrionModule : public muse::modularity::IModuleSetup
{
public:
  OrchestrionModule();

private:
  std::string moduleName() const override;
  void registerExports() override;
  void onInit(const muse::IApplication::RunMode &mode) override;

  const std::shared_ptr<Orchestrion> m_orchestrion;
};

} // namespace dgk