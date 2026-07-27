#pragma once

#include "rpl/producer.h"

#include <QtCore/QString>

namespace Ayu {

[[nodiscard]] bool DiscreetModeEnabled();
[[nodiscard]] rpl::producer<bool> DiscreetModeValue();
[[nodiscard]] QString VisibleClientName();
[[nodiscard]] QString VisibleClientShortName();
[[nodiscard]] QString VisibleClientUrl();

void InitializeDiscreetMode();
void SetDiscreetMode(bool enabled);

} // namespace Ayu
