#pragma once

class QPainter;
class QWidget;

namespace Ayu::VisualEffects {

void PrepareWindow(QWidget *window);
void RefreshWindow(QWidget *window);
void HandlePaletteChanged(QWidget *window);
void PaintWindow(QWidget *window, QPainter &painter);
void ApplyMotionSettings();

} // namespace Ayu::VisualEffects
