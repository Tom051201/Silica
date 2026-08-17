#include "silicapch.h"
#include "SCheckBox.h"

#include "Renderer.h"

namespace Silica {

	void SCheckBox::construct(const Args& args) {
		m_isChecked = args.initialCheck;
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().Background_Input);
		m_checkColor = args.checkColor.value_or(GetTheme().Accent_Primary);
		m_onCheckChanged = args.onCheckChanged;
	}

	void SCheckBox::computeDesiredSize() {
		m_desiredSize = Vec2(20.0f, 20.0f);
	}

	void SCheckBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SCheckBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw Background --
		outDrawList.addRect(allocatedGeometry, m_backgroundColor);

		// -- Draw Inner Check --
		if (m_isChecked) {
			Geometry checkGeo;
			float pad = GetTheme().Element_Padding * m_renderScale;
			checkGeo.position.x = allocatedGeometry.position.x + pad;
			checkGeo.position.y = allocatedGeometry.position.y + pad;
			checkGeo.size.x = allocatedGeometry.size.x - (pad * 2.0f);
			checkGeo.size.y = allocatedGeometry.size.y - (pad * 2.0f);
			outDrawList.addRect(checkGeo, m_checkColor);
		}
	}

	void SCheckBox::setRenderScale(float scale) {
		m_renderScale = scale;
	}

	EventReply SCheckBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (allocatedGeometry.contains(mousePos)) {
			m_isChecked = !m_isChecked;

			if (m_onCheckChanged) {
				m_onCheckChanged(m_isChecked);
			}
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

}
