#include "ayu/ui/visual_effects.h"

#include "ayu/ayu_settings.h"
#include "platform/platform_specific.h"
#include "styles/style_basic.h"
#include "ui/effects/animation_value.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include "ui/style/style_core_palette.h"

#include <algorithm>
#include <array>
#include <QtGui/QPainter>
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
	state.image = path.isEmpty()
		? QImage()
		: Images::Read({ .path = path }).image;
	if (!state.image.isNull() && blur > 0) {
		state.image = Images::BlurLargeImage(std::move(state.image), blur);
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
	const auto &image = CurrentState().image;
	if (!image.isNull() && !window->rect().isEmpty()) {
		const auto target = QRectF(window->rect());
		const auto targetRatio = target.width() / target.height();
		const auto imageRatio = image.width() / float64(image.height());
		auto source = QRectF(QPointF(), QSizeF(image.size()));
		if (imageRatio > targetRatio) {
			const auto width = source.height() * targetRatio;
			source.setLeft((source.width() - width) / 2.);
			source.setWidth(width);
		} else {
			const auto height = source.width() / targetRatio;
			source.setTop((source.height() - height) / 2.);
			source.setHeight(height);
		}
		painter.save();
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.setOpacity(settings.transparentBackgroundOpacity() / 100.);
		painter.drawImage(target, image, source);
		painter.restore();
	}

	auto tint = QColor(settings.transparentTint());
	if (!tint.isValid()) {
		tint = QColor(22, 25, 34);
	}
	tint.setAlpha(image.isNull() ? 76 : 42);
	painter.fillRect(window->rect(), tint);
}

void ApplyMotionSettings() {
	anim::SetMotionStyle(ResolveMotionStyle());
}

} // namespace Ayu::VisualEffects
