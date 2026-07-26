#include "ayu/ui/visual_effects.h"

#include "ayu/ayu_settings.h"
#include "platform/platform_specific.h"
#include "ui/effects/animation_value.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include "ui/style/style_core_palette.h"

#include "styles/style_ayu_styles.h"
#include "styles/style_basic.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <QtGui/QBrush>
#include <QtGui/QCursor>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QRadialGradient>
#include <QtGui/QScreen>
#include <QtWidgets/QWidget>

namespace Ayu::VisualEffects {
namespace {

struct PaletteEntry {
	const char *name;
	style::color color;
	int alphaOffset = 0;
};

struct State {
	QByteArray opaquePalette;
	QByteArray glassPalette;
	QString imagePath;
	int imageBlur = -1;
	QImage image;
	QColor imageAverage;
	QImage grain;
	QSize coverSize;
	QImage cover;
	bool applyingPalette = false;
};

[[nodiscard]] State &CurrentState() {
	static auto result = State();
	return result;
}

[[nodiscard]] int SurfaceAlpha(int offset = 0) {
	const auto opacity = AyuSettings::getInstance().transparentOpacity();
	return std::clamp((255 * opacity / 100) + offset, 0, 255);
}

[[nodiscard]] float64 ColorLuminance(const QColor &color) {
	const auto linear = [](int channel) {
		const auto value = channel / 255.;
		return (value <= 0.04045)
			? (value / 12.92)
			: std::pow((value + 0.055) / 1.055, 2.4);
	};
	return (0.2126 * linear(color.red()))
		+ (0.7152 * linear(color.green()))
		+ (0.0722 * linear(color.blue()));
}

[[nodiscard]] QColor BlendColors(
		const QColor &from,
		const QColor &to,
		float64 amount) {
	amount = std::clamp(amount, 0., 1.);
	const auto blend = [=](int first, int second) {
		return int(std::round(first + (second - first) * amount));
	};
	return QColor(
		blend(from.red(), to.red()),
		blend(from.green(), to.green()),
		blend(from.blue(), to.blue()));
}

[[nodiscard]] QImage CreateGrain() {
	const auto side = st::liquidGlassNoiseTileSize;
	auto result = QImage(
		side,
		side,
		QImage::Format_ARGB32_Premultiplied);
	auto seed = uint32(0x7f4a7c15);
	for (auto y = 0; y != side; ++y) {
		for (auto x = 0; x != side; ++x) {
			seed = (seed * uint32(1664525)) + uint32(1013904223);
			const auto light = ((seed >> 30) & 1) != 0;
			const auto alpha = 3 + int((seed >> 27) & 3);
			result.setPixelColor(
				x,
				y,
				QColor(
					light ? 255 : 0,
					light ? 255 : 0,
					light ? 255 : 0,
					alpha));
		}
	}
	return result;
}

[[nodiscard]] const QImage &CoverImage(
		State &state,
		QSize size) {
	if (state.image.isNull() || size.isEmpty()) {
		state.coverSize = QSize();
		state.cover = QImage();
		return state.cover;
	}
	if (state.coverSize == size && !state.cover.isNull()) {
		return state.cover;
	}
	state.coverSize = size;
	const auto scaled = state.image.scaled(
		size,
		Qt::KeepAspectRatioByExpanding,
		Qt::SmoothTransformation);
	state.cover = scaled.copy(
		std::max((scaled.width() - size.width()) / 2, 0),
		std::max((scaled.height() - size.height()) / 2, 0),
		size.width(),
		size.height());
	return state.cover;
}

[[nodiscard]] QColor AdaptiveTint(
		const AyuSettings &settings,
		const State &state) {
	auto tint = QColor(settings.transparentTint());
	if (!tint.isValid()) {
		tint = QColor(22, 25, 34);
	}
	if (!settings.transparentAdaptiveMaterial()) {
		return tint;
	}
	const auto sample = state.imageAverage.isValid()
		? state.imageAverage
		: st::windowBg->c;
	const auto luminance = ColorLuminance(sample);
	const auto sampled = BlendColors(tint, sample, 0.18);
	const auto contrast = (luminance > 0.58)
		? QColor(12, 15, 22)
		: QColor(238, 244, 255);
	return BlendColors(sampled, contrast, luminance > 0.58 ? 0.13 : 0.06);
}

[[nodiscard]] QPainterPath WindowPath(const QRectF &rect) {
	auto result = QPainterPath();
	result.addRoundedRect(
		rect,
		st::liquidGlassCornerRadius,
		st::liquidGlassCornerRadius);
	return result;
}

[[nodiscard]] QPainterPath EdgePath(const QRectF &rect) {
	auto result = WindowPath(rect);
	const auto inset = st::liquidGlassEdgeThickness;
	const auto inner = rect.adjusted(inset, inset, -inset, -inset);
	if (!inner.isEmpty()) {
		auto innerPath = QPainterPath();
		innerPath.addRoundedRect(
			inner,
			std::max(st::liquidGlassCornerRadius - inset, 0),
			std::max(st::liquidGlassCornerRadius - inset, 0));
		result = result.subtracted(innerPath);
	}
	return result;
}

void SetEntryAlpha(const PaletteEntry &entry) {
	auto color = entry.color->c;
	const auto requested = SurfaceAlpha(entry.alphaOffset);
	color.setAlpha(color.alpha() * requested / 255);
	style::main_palette::setColor(
		QLatin1String(entry.name),
		color.red(),
		color.green(),
		color.blue(),
		color.alpha());
}

void ApplyEntries(gsl::span<const PaletteEntry> entries) {
	for (const auto &entry : entries) {
		SetEntryAlpha(entry);
	}
}

void ApplyGlassPalette(bool force) {
	auto &state = CurrentState();
	if (state.applyingPalette) {
		return;
	}
	const auto enabled = AyuSettings::getInstance().transparentMode();
	const auto current = style::main_palette::save();
	if (enabled && !force && current == state.glassPalette) {
		return;
	}
	if (!enabled) {
		if (!state.opaquePalette.isEmpty()
			&& current != state.opaquePalette) {
			state.applyingPalette = true;
			style::main_palette::load(state.opaquePalette);
			state.glassPalette.clear();
			style::NotifyPaletteChanged();
			state.applyingPalette = false;
		}
		state.opaquePalette.clear();
		state.glassPalette.clear();
		return;
	}
	if (state.opaquePalette.isEmpty()
		|| (!state.glassPalette.isEmpty()
			&& current != state.glassPalette)) {
		state.opaquePalette = current;
	}

	state.applyingPalette = true;
	style::main_palette::load(state.opaquePalette);

	const auto common = std::array{
		PaletteEntry{ "windowBg", st::windowBg },
		PaletteEntry{ "windowBgOver", st::windowBgOver, 24 },
		PaletteEntry{ "windowBgRipple", st::windowBgRipple, 36 },
		PaletteEntry{ "lightButtonBg", st::lightButtonBg, 10 },
		PaletteEntry{ "lightButtonBgOver", st::lightButtonBgOver, 28 },
		PaletteEntry{ "lightButtonBgRipple", st::lightButtonBgRipple, 40 },
		PaletteEntry{ "filterInputActiveBg", st::filterInputActiveBg, 14 },
		PaletteEntry{ "filterInputInactiveBg", st::filterInputInactiveBg, 22 },
		PaletteEntry{ "titleBg", st::titleBg, 12 },
		PaletteEntry{ "titleBgActive", st::titleBgActive, 12 },
		PaletteEntry{ "titleButtonBg", st::titleButtonBg, 12 },
		PaletteEntry{ "titleButtonBgActive", st::titleButtonBgActive, 12 },
	};
	ApplyEntries(common);

	const auto &settings = AyuSettings::getInstance();
	if (settings.transparentSidebar()) {
		const auto sidebar = std::array{
			PaletteEntry{ "dialogsBg", st::dialogsBg },
			PaletteEntry{ "dialogsBgOver", st::dialogsBgOver, 24 },
			PaletteEntry{ "dialogsRippleBg", st::dialogsRippleBg, 36 },
			PaletteEntry{ "searchedBarBg", st::searchedBarBg, 18 },
			PaletteEntry{ "mainMenuBg", st::mainMenuBg, 6 },
		};
		ApplyEntries(sidebar);
	}
	if (settings.transparentChat()) {
		const auto chat = std::array{
			PaletteEntry{ "topBarBg", st::topBarBg, 8 },
			PaletteEntry{ "msgInBg", st::msgInBg, 34 },
			PaletteEntry{ "msgOutBg", st::msgOutBg, 34 },
			PaletteEntry{ "historyComposeAreaBg", st::historyComposeAreaBg, 16 },
			PaletteEntry{ "historyPinnedBg", st::historyPinnedBg, 16 },
			PaletteEntry{ "historyReplyBg", st::historyReplyBg, 16 },
			PaletteEntry{ "historyComposeButtonBg", st::historyComposeButtonBg, 16 },
			PaletteEntry{ "historyComposeButtonBgOver", st::historyComposeButtonBgOver, 30 },
			PaletteEntry{ "historyComposeButtonBgRipple", st::historyComposeButtonBgRipple, 42 },
			PaletteEntry{ "historyToDownBg", st::historyToDownBg, 24 },
			PaletteEntry{ "historyToDownBgOver", st::historyToDownBgOver, 36 },
			PaletteEntry{ "historyToDownBgRipple", st::historyToDownBgRipple, 48 },
		};
		ApplyEntries(chat);
	}
	if (settings.transparentPanels()) {
		const auto panels = std::array{
			PaletteEntry{ "menuBg", st::menuBg, 8 },
			PaletteEntry{ "menuBgOver", st::menuBgOver, 26 },
			PaletteEntry{ "menuBgRipple", st::menuBgRipple, 38 },
			PaletteEntry{ "boxBg", st::boxBg, 8 },
			PaletteEntry{ "boxSearchBg", st::boxSearchBg, 16 },
			PaletteEntry{ "boxDividerBg", st::boxDividerBg, 24 },
			PaletteEntry{ "contactsBg", st::contactsBg, 8 },
			PaletteEntry{ "contactsBgOver", st::contactsBgOver, 26 },
			PaletteEntry{ "emojiPanBg", st::emojiPanBg, 8 },
			PaletteEntry{ "emojiPanCategories", st::emojiPanCategories, 18 },
			PaletteEntry{ "emojiPanHeaderBg", st::emojiPanHeaderBg, 20 },
			PaletteEntry{ "mediaPlayerBg", st::mediaPlayerBg, 8 },
		};
		ApplyEntries(panels);
	}

	state.glassPalette = style::main_palette::save();
	style::NotifyPaletteChanged();
	state.applyingPalette = false;
}

void RefreshImage() {
	auto &state = CurrentState();
	const auto &settings = AyuSettings::getInstance();
	const auto path = settings.transparentBackgroundPath();
	const auto blur = settings.transparentBackgroundBlur();
	if (state.imagePath == path && state.imageBlur == blur) {
		return;
	}
	state.imagePath = path;
	state.imageBlur = blur;
	state.coverSize = QSize();
	state.cover = QImage();
	state.image = path.isEmpty()
		? QImage()
		: Images::Read({ .path = path }).image;
	if (!state.image.isNull() && blur > 0) {
		state.image = Images::BlurLargeImage(std::move(state.image), blur);
	}
	if (state.image.isNull()) {
		state.imageAverage = QColor();
	} else {
		const auto sample = state.image.scaled(
			1,
			1,
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation);
		state.imageAverage = sample.pixelColor(0, 0);
	}
	if (state.grain.isNull()) {
		state.grain = CreateGrain();
	}
}

[[nodiscard]] anim::MotionStyle ResolveMotionStyle() {
	const auto &settings = AyuSettings::getInstance();
	if (!settings.enhancedAnimations()) {
		return anim::MotionStyle::Default;
	}
	switch (settings.motionStyle()) {
	case IntelGramMotionStyle::Quick:
		return anim::MotionStyle::Quick;
	case IntelGramMotionStyle::Smooth:
		return anim::MotionStyle::Smooth;
	case IntelGramMotionStyle::Dynamic:
		return anim::MotionStyle::Dynamic;
	case IntelGramMotionStyle::Springy:
		return anim::MotionStyle::Springy;
	case IntelGramMotionStyle::LiquidBounce:
		return anim::MotionStyle::LiquidBounce;
	}
	Unexpected("IntelGramMotionStyle value.");
}

} // namespace

void PrepareWindow(QWidget *window) {
	window->setAttribute(Qt::WA_TranslucentBackground);
	window->setAutoFillBackground(false);
}

void RefreshWindow(QWidget *window) {
	ApplyMotionSettings();
	ApplyGlassPalette(true);
	RefreshImage();
	const auto &settings = AyuSettings::getInstance();
	Platform::SetWindowVisualEffect(
		window,
		settings.transparentMode(),
		settings.transparentBlur());
	window->update();
}

void HandlePaletteChanged(QWidget *window) {
	ApplyGlassPalette(false);
	Platform::SetWindowVisualEffect(
		window,
		AyuSettings::getInstance().transparentMode(),
		AyuSettings::getInstance().transparentBlur());
	window->update();
}

void PaintWindow(QWidget *window, QPainter &painter) {
	const auto &settings = AyuSettings::getInstance();
	if (!settings.transparentMode()) {
		painter.fillRect(window->rect(), st::windowBg);
		return;
	}

	RefreshImage();
	auto &state = CurrentState();
	const auto &image = state.image;
	const auto target = QRectF(window->rect());
	if (target.isEmpty()) {
		return;
	}
	const auto &cover = CoverImage(state, window->size());
	if (!image.isNull()) {
		painter.save();
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.setOpacity(settings.transparentBackgroundOpacity() / 100.);
		painter.drawImage(target, cover);
		painter.restore();
	}

	const auto intensity = settings.transparentMaterialIntensity() / 100.;
	const auto sample = state.imageAverage.isValid()
		? state.imageAverage
		: st::windowBg->c;
	const auto luminance = ColorLuminance(sample);
	auto tint = AdaptiveTint(settings, state);
	const auto tintAlphaBase = image.isNull() ? 52 : 28;
	const auto tintAlphaRange = image.isNull() ? 46 : 36;
	const auto luminanceAdjustment = settings.transparentAdaptiveMaterial()
		? int(std::round((luminance - 0.5) * 22.))
		: 0;
	tint.setAlpha(std::clamp(
		tintAlphaBase
			+ int(std::round(tintAlphaRange * intensity))
			+ luminanceAdjustment,
		18,
		112));
	painter.fillRect(target, tint);

	const auto phase = settings.transparentDynamicHighlights()
			&& !anim::Disabled()
		? (crl::now() % crl::time(120000)) / 1000.
		: 0.;
	const auto cursor = window->mapFromGlobal(QCursor::pos());
	const auto cursorInside = target.contains(QPointF(cursor));
	const auto focus = cursorInside
		? QPointF(cursor)
		: QPointF(
			target.center().x() + std::sin(phase * 0.21) * target.width() * 0.18,
			target.center().y() + std::cos(phase * 0.17) * target.height() * 0.12);
	const auto light = QColor(255, 255, 255);
	const auto cool = BlendColors(
		QColor(104, 194, 255),
		sample,
		0.22);

	{
		const auto radius = std::max(
			float64(st::liquidGlassHighlightRadius),
			std::hypot(target.width(), target.height()) * 0.36);
		auto diffusion = QRadialGradient(focus, radius);
		auto center = light;
		center.setAlpha(int(std::round((12. + 18. * intensity)
			* (luminance > 0.72 ? 0.55 : 1.))));
		auto middle = cool;
		middle.setAlpha(int(std::round(7. + 11. * intensity)));
		diffusion.setColorAt(0., center);
		diffusion.setColorAt(0.46, middle);
		diffusion.setColorAt(1., Qt::transparent);
		painter.fillRect(target, diffusion);
	}

	if (settings.transparentDynamicHighlights() && !anim::Disabled()) {
		const auto travel = std::fmod(phase * 0.055, 1.4) - 0.2;
		const auto start = QPointF(
			target.left() + target.width() * (travel - 0.24),
			target.top());
		const auto end = QPointF(
			start.x() + target.width() * 0.4,
			target.bottom());
		auto reflection = QLinearGradient(start, end);
		auto highlight = light;
		highlight.setAlpha(int(std::round(14. * intensity)));
		reflection.setColorAt(0., Qt::transparent);
		reflection.setColorAt(0.46, Qt::transparent);
		reflection.setColorAt(0.5, highlight);
		reflection.setColorAt(0.54, Qt::transparent);
		reflection.setColorAt(1., Qt::transparent);
		painter.fillRect(target, reflection);
	}

	if (settings.transparentRefraction()) {
		const auto edge = EdgePath(target.adjusted(0.5, 0.5, -0.5, -0.5));
		if (!image.isNull()) {
			const auto offset = st::liquidGlassRefractionOffset * intensity;
			const auto shift = QPointF(
				std::sin(phase * 0.43) * offset,
				std::cos(phase * 0.37) * offset);
			painter.save();
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			painter.setClipPath(edge);
			painter.setOpacity(0.16 + (0.2 * intensity));
			painter.drawImage(target.translated(shift), cover);
			painter.restore();
		}
		auto edgeMaterial = QLinearGradient(
			target.topLeft(),
			target.bottomRight());
		auto nearLight = light;
		nearLight.setAlpha(int(std::round(30. + 34. * intensity)));
		auto quietLight = light;
		quietLight.setAlpha(int(std::round(5. + 8. * intensity)));
		auto farLight = cool;
		farLight.setAlpha(int(std::round(15. + 18. * intensity)));
		edgeMaterial.setColorAt(0., nearLight);
		edgeMaterial.setColorAt(0.42, quietLight);
		edgeMaterial.setColorAt(1., farLight);
		painter.fillPath(edge, edgeMaterial);
	}

	{
		auto topReflection = QLinearGradient(
			target.topLeft(),
			QPointF(target.left(), target.top() + target.height() * 0.32));
		auto top = light;
		top.setAlpha(int(std::round(19. + 18. * intensity)));
		topReflection.setColorAt(0., top);
		topReflection.setColorAt(0.18, QColor(255, 255, 255, 7));
		topReflection.setColorAt(1., Qt::transparent);
		painter.save();
		painter.setClipPath(WindowPath(target));
		painter.fillRect(target, topReflection);
		painter.restore();
	}

	if (!state.grain.isNull()) {
		painter.save();
		painter.setOpacity(0.55 + (0.35 * intensity));
		painter.fillRect(target, QBrush(state.grain));
		painter.restore();
	}

	{
		const auto outline = target.adjusted(0.5, 0.5, -0.5, -0.5);
		auto outer = light;
		outer.setAlpha(int(std::round(34. + 35. * intensity)));
		painter.save();
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(Qt::NoBrush);
		painter.setPen(QPen(outer, 1.));
		painter.drawRoundedRect(
			outline,
			st::liquidGlassCornerRadius,
			st::liquidGlassCornerRadius);
		auto inner = BlendColors(sample, QColor(255, 255, 255), 0.38);
		inner.setAlpha(int(std::round(10. + 16. * intensity)));
		painter.setPen(QPen(inner, 1.));
		const auto inset = target.adjusted(
			st::liquidGlassEdgeThickness,
			st::liquidGlassEdgeThickness,
			-st::liquidGlassEdgeThickness,
			-st::liquidGlassEdgeThickness);
		painter.drawRoundedRect(
			inset,
			std::max(
				st::liquidGlassCornerRadius
					- st::liquidGlassEdgeThickness,
				0),
			std::max(
				st::liquidGlassCornerRadius
					- st::liquidGlassEdgeThickness,
				0));
		painter.restore();
	}
}

void ApplyMotionSettings() {
	anim::SetMotionStyle(ResolveMotionStyle());
}

bool WindowAnimationEnabled(QWidget *window) {
	const auto &settings = AyuSettings::getInstance();
	return settings.transparentMode()
		&& settings.transparentDynamicHighlights()
		&& !anim::Disabled()
		&& window->isVisible()
		&& window->isActiveWindow();
}

int WindowAnimationRefreshInterval(QWidget *window) {
	const auto screen = window->screen();
	const auto refreshRate = screen
		? screen->refreshRate()
		: 60.;
	const auto capped = std::clamp(refreshRate, 30., 120.);
	return std::max(int(std::round(1000. / capped)), 8);
}

} // namespace Ayu::VisualEffects
