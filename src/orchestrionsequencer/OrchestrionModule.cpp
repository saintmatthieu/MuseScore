#include "OrchestrionModule.h"
#include "internal/ComputerKeyboardMidiController.h"
#include "internal/Orchestrion.h"

namespace dgk
{
OrchestrionModule::OrchestrionModule()
    : m_orchestrion(std::make_shared<Orchestrion>())
{
}

std::string OrchestrionModule::moduleName() const { return "Orchestrion"; }

void OrchestrionModule::registerExports()
{
  ioc()->registerExport<IOrchestrion>(moduleName(), m_orchestrion);
  ioc()->registerExport<IComputerKeyboardMidiController>(
      moduleName(), new ComputerKeyboardMidiController());
}

void OrchestrionModule::onInit(const muse::IApplication::RunMode &)
{
  m_orchestrion->init();
}
} // namespace dgk