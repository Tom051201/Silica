#include "silicapch.h"
#include "SColorField.h"

#include "SMenuAnchor.h"
#include "SButton.h"
#include "SBox.h"
#include "SColorPicker.h"

namespace Silica {

	void SColorField::construct(const Args& args) {
		auto colorBox = MakeWidget<SBox>({
			.explicitSize = Vec2(100.0f, 20.0f),
			.backgroundColor = args.initialColor
		});

		m_anchor = MakeWidget<SMenuAnchor>({
			.openOnHover = false,
			.anchorContent = MakeWidget<SButton>({
				.padding = { 2.0f, 2.0f },
				.child = colorBox
			}),
			.menuContent = MakeWidget<SBox>({
				.padding = { 10.0f, 10.0f },
				.borderThickness = 1.0f,
				.backgroundColor = GetTheme().Background_Popup,
				.borderColor = GetTheme().Border_Primary,
				.child = MakeWidget<SColorPicker>({
					.initialColor = args.initialColor,
					.onColorChanged = [colorBox, externalCallback = args.onColorChanged](Color c) {
						colorBox->setBackgroundColor(c);
						if (externalCallback) externalCallback(c);
					}
				})
			})
		});
	}

	void SColorField::computeDesiredSize() {
		m_anchor->computeDesiredSize();
		m_desiredSize = m_anchor->getDesiredSize();
	}

	void SColorField::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_anchor->arrangeChildren(allocatedGeometry);
	}

	void SColorField::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		m_anchor->onDraw(outDrawList, allocatedGeometry);
	}

	void SColorField::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_anchor) m_anchor->setRenderScale(scale);
	}

	EventReply SColorField::onMouseMove(const Geometry& geo, const Vec2& pos) {
		return m_anchor->onMouseMove(geo, pos);
	}

	EventReply SColorField::onMouseButtonDown(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		return m_anchor->onMouseButtonDown(geo, pos, btn);
	}

	EventReply SColorField::onMouseButtonUp(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		return m_anchor->onMouseButtonUp(geo, pos, btn);
	}

	EventReply SColorField::onMouseWheel(const Geometry& geo, const Vec2& pos, float scroll) {
		return m_anchor->onMouseWheel(geo, pos, scroll);
	}

}
