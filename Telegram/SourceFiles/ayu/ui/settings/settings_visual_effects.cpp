#include "ayu/ui/settings/settings_visual_effects.h"

#include "ayu/ayu_settings.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_main.h"
#include "boxes/abstract_box.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "lang_auto.h"
#include "settings/settings_builder.h"
#include "ui/layers/generic_box.h"
#include "ui/image/image.h"
#include "ui/vertical_list.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QFileInfo>
#include <QtGui/QColor>

namespace Settings {
namespace {

using namespace Builder;
using namespace AyuBuilder;

void ShowTintEditor(
		not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::ayu_VisualEffectsTintTitle());
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::ayu_VisualEffectsTintPlaceholder(),
				AyuSettings::getInstance().transparentTint()),
			st::boxRowPadding);
		field->setMaxLength(9);
		box->setFocusCallback([=] {
			field->setFocusFast();
		});
		Ui::AddDividerText(
			box->verticalLayout(),
			tr::ayu_VisualEffectsTintDescription());
		const auto save = [=] {
			const auto value = field->getLastText().trimmed();
			if (!QColor(value).isValid()) {
				controller->showToast(
					tr::ayu_VisualEffectsTintInvalid(tr::now));
				return;
			}
			AyuSettings::getInstance().setTransparentTint(value);
			box->closeBox();
		};
		field->submits(
		) | rpl::on_next(save, field->lifetime());
		box->addButton(tr::lng_settings_save(), save);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

void ChooseBackdrop(
		not_null<Window::SessionController*> controller) {
	const auto filter = u"Image files (*.png *.jpg *.jpeg *.webp *.heic);;"_q
		+ FileDialog::AllFilesFilter();
	FileDialog::GetOpenPath(
		Core::App().getFileDialogParent(),
		tr::ayu_VisualEffectsChooseBackground(tr::now),
		filter,
		[=](const FileDialog::OpenResult &result) {
			if (result.paths.isEmpty()) {
				return;
			}
			const auto path = result.paths.front();
			if (Images::Read({ .path = path }).image.isNull()) {
				controller->showToast(
					tr::ayu_VisualEffectsBackgroundInvalid(tr::now));
				return;
			}
			AyuSettings::getInstance().setTransparentBackgroundPath(path);
		});
}

void BuildGlass(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto &settings = AyuSettings::getInstance();

	builder.addSubsectionTitle(tr::ayu_VisualEffectsGlassHeader());
	ayu.addSettingToggle({
		.id = u"ayu/transparentMode"_q,
		.title = tr::ayu_VisualEffectsEnableGlass(),
		.getter = &AyuSettings::transparentMode,
		.setter = &AyuSettings::setTransparentMode,
		.icon = { &st::menuIconTransparent },
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentBlur"_q,
		.title = tr::ayu_VisualEffectsNativeBlur(),
		.getter = &AyuSettings::transparentBlur,
		.setter = &AyuSettings::setTransparentBlur,
		.icon = { &st::menuIconChangeColors },
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentSidebar"_q,
		.title = tr::ayu_VisualEffectsSidebar(),
		.getter = &AyuSettings::transparentSidebar,
		.setter = &AyuSettings::setTransparentSidebar,
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentChat"_q,
		.title = tr::ayu_VisualEffectsChat(),
		.getter = &AyuSettings::transparentChat,
		.setter = &AyuSettings::setTransparentChat,
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentPanels"_q,
		.title = tr::ayu_VisualEffectsPanels(),
		.getter = &AyuSettings::transparentPanels,
		.setter = &AyuSettings::setTransparentPanels,
	});
	ayu.addSlider({
		.id = u"ayu/transparentOpacity"_q,
		.title = tr::ayu_VisualEffectsOpacity(),
		.steps = 81,
		.current = settings.transparentOpacity(),
		.indexToValue = [](int index) {
			return index + 20;
		},
		.onChanged = [](int value) {
			AyuSettings::getInstance().setTransparentOpacity(value);
		},
		.formatLabel = [](int value) {
			return QString::number(value) + '%';
		},
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentAdaptiveMaterial"_q,
		.title = tr::ayu_VisualEffectsAdaptiveMaterial(),
		.getter = &AyuSettings::transparentAdaptiveMaterial,
		.setter = &AyuSettings::setTransparentAdaptiveMaterial,
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentDynamicHighlights"_q,
		.title = tr::ayu_VisualEffectsDynamicHighlights(),
		.getter = &AyuSettings::transparentDynamicHighlights,
		.setter = &AyuSettings::setTransparentDynamicHighlights,
	});
	ayu.addSettingToggle({
		.id = u"ayu/transparentRefraction"_q,
		.title = tr::ayu_VisualEffectsRefraction(),
		.getter = &AyuSettings::transparentRefraction,
		.setter = &AyuSettings::setTransparentRefraction,
	});
	ayu.addSlider({
		.id = u"ayu/transparentMaterialIntensity"_q,
		.title = tr::ayu_VisualEffectsMaterialIntensity(),
		.steps = 101,
		.current = settings.transparentMaterialIntensity(),
		.onChanged = [](int value) {
			AyuSettings::getInstance().setTransparentMaterialIntensity(value);
		},
		.formatLabel = [](int value) {
			return QString::number(value) + '%';
		},
	});
	builder.addDividerText(tr::ayu_VisualEffectsGlassDescription());
	ayu.addSectionDivider();
}

void BuildBackdrop(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto &settings = AyuSettings::getInstance();
	const auto controller = builder.controller();

	builder.addSubsectionTitle(tr::ayu_VisualEffectsBackdropHeader());
	builder.addButton({
		.id = u"ayu/transparentTint"_q,
		.title = tr::ayu_VisualEffectsTint(),
		.icon = { &st::menuIconChangeColors },
		.label = settings.transparentTintValue(),
		.onClick = [=] {
			ShowTintEditor(controller);
		},
	});
	builder.addButton({
		.id = u"ayu/transparentBackground"_q,
		.title = tr::ayu_VisualEffectsChooseBackground(),
		.icon = { &st::menuIconPhoto },
		.label = settings.transparentBackgroundPathValue(
		) | rpl::map([](const QString &path) {
			return path.isEmpty()
				? tr::ayu_VisualEffectsNoBackground(tr::now)
				: QFileInfo(path).fileName();
		}),
		.onClick = [=] {
			ChooseBackdrop(controller);
		},
	});
	builder.addButton({
		.id = u"ayu/transparentBackgroundClear"_q,
		.title = tr::ayu_VisualEffectsClearBackground(),
		.icon = { &st::menuIconDelete },
		.onClick = [] {
			AyuSettings::getInstance().setTransparentBackgroundPath(
				QString());
		},
		.shown = settings.transparentBackgroundPathValue(
		) | rpl::map([](const QString &path) {
			return !path.isEmpty();
		}),
	});
	ayu.addSlider({
		.id = u"ayu/transparentBackgroundOpacity"_q,
		.title = tr::ayu_VisualEffectsBackgroundOpacity(),
		.steps = 101,
		.current = settings.transparentBackgroundOpacity(),
		.onChanged = [](int value) {
			AyuSettings::getInstance().setTransparentBackgroundOpacity(
				value);
		},
		.formatLabel = [](int value) {
			return QString::number(value) + '%';
		},
	});
	ayu.addSlider({
		.id = u"ayu/transparentBackgroundBlur"_q,
		.title = tr::ayu_VisualEffectsBackgroundBlur(),
		.steps = 41,
		.current = settings.transparentBackgroundBlur(),
		.onFinalChanged = [](int value) {
			AyuSettings::getInstance().setTransparentBackgroundBlur(value);
		},
		.formatLabel = [](int value) {
			return QString::number(value) + u" px"_q;
		},
	});
	builder.addDividerText(tr::ayu_VisualEffectsBackdropDescription());
	ayu.addSectionDivider();
}

void BuildMotion(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto &settings = AyuSettings::getInstance();

	builder.addSubsectionTitle(tr::ayu_VisualEffectsMotionHeader());
	ayu.addSettingToggle({
		.id = u"ayu/enhancedAnimations"_q,
		.title = tr::ayu_VisualEffectsEnhancedAnimations(),
		.getter = &AyuSettings::enhancedAnimations,
		.setter = &AyuSettings::setEnhancedAnimations,
		.icon = { &st::menuIconExperimental },
	});
	ayu.addChooseButton({
		.id = u"ayu/motionStyle"_q,
		.title = tr::ayu_VisualEffectsMotionStyle(),
		.boxTitle = tr::ayu_VisualEffectsMotionStyle(),
		.initialSelection = int(settings.motionStyle()),
		.options = {
			tr::ayu_VisualEffectsMotionQuick(tr::now),
			tr::ayu_VisualEffectsMotionSmooth(tr::now),
			tr::ayu_VisualEffectsMotionDynamic(tr::now),
			tr::ayu_VisualEffectsMotionSpringy(tr::now),
			tr::ayu_VisualEffectsMotionLiquidBounce(tr::now),
		},
		.setter = [](int value) {
			AyuSettings::getInstance().setMotionStyle(
				static_cast<IntelGramMotionStyle>(value));
		},
		.icon = { &st::menuIconPowerUsage },
	});
	ayu.addSettingToggle({
		.id = u"ayu/animateWindowOpening"_q,
		.title = tr::ayu_VisualEffectsAnimateWindows(),
		.getter = &AyuSettings::animateWindowOpening,
		.setter = &AyuSettings::setAnimateWindowOpening,
	});
	builder.addDividerText(tr::ayu_VisualEffectsMotionDescription());
	builder.addDividerText(tr::ayu_VisualEffectsMotionPacksDescription());
}

const auto kMeta = BuildHelper({
	.id = AyuVisualEffects::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_VisualEffectsTitle,
	.icon = &st::menuIconTransparent,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	builder.addSkip();
	BuildGlass(builder, ayu);
	BuildBackdrop(builder, ayu);
	BuildMotion(builder, ayu);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> AyuVisualEffects::title() {
	return tr::ayu_VisualEffectsTitle();
}

AyuVisualEffects::AyuVisualEffects(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuVisualEffects::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
