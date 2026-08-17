#include "silicapch.h"
#include "SSliderInt.h"

#include "Renderer.h"

namespace Silica {

	void SSliderInt::construct(const Args& args) {
		m_min = args.minValue;
		m_max = std::max(args.minValue, args.maxValue);
		m_snapStep = std::max(1, args.snapStep);
		m_value = std::clamp(args.initialValue, m_min, m_max);
		m_value = (int)(m_min + (std::round((float)(m_value - m_min) / m_snapStep) * m_snapStep));
		m_value = std::clamp(m_value, m_min, m_max);
		m_showText = args.showText;
		m_format = args.format;
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_trackColor = args.trackColor.value_or(GetTheme().Element_Pressed);
		m_fillColor = args.fillColor.value_or(GetTheme().Accent_Primary);
		m_thumbColor = args.thumbColor.value_or(GetTheme().Text_Dim);
		m_thumbDraggingColor = args.thumbDraggingColor.value_or(GetTheme().Text_Main);
		m_onValueChanged = args.onValueChanged;
	}

	void SSliderInt::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f * m_renderScale, 20.0f * m_renderScale);
	}

	void SSliderInt::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SSliderInt::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Calculate Visual Percentage --
		float percentage = 0.0f;
		if (m_max > m_min) percentage = static_cast<float>(m_value - m_min) / static_cast<float>(m_max - m_min);

		// -- Draw Background Track --
		outDrawList.addRect(allocatedGeometry, m_trackColor);

		// -- Geometry Setup --
		float thumbWidth = 4.0f * m_renderScale;
		float thumbHalf = thumbWidth * 0.5f;
		float travelRange = allocatedGeometry.size.x - thumbWidth;
		float thumbLeftX = allocatedGeometry.position.x + (travelRange * percentage);

		// -- Draw Fill Track --
		Geometry fillGeo = allocatedGeometry;
		fillGeo.size.x = (thumbLeftX + thumbHalf) - allocatedGeometry.position.x;
		outDrawList.addRect(fillGeo, m_fillColor);

		// -- Draw Thumb Handle --
		Geometry thumbGeo = { {thumbLeftX, allocatedGeometry.position.y}, {thumbWidth, allocatedGeometry.size.y} };
		Color thumbColor = m_isDragging ? m_thumbDraggingColor : m_thumbColor;
		outDrawList.addRect(thumbGeo, thumbColor);

		// -- Draw Text --
		if (m_showText && m_font) {
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), m_format.c_str(), m_value);
			std::string text = buffer;

			float textWidth = 0.0f;
			for (char c : text) textWidth += m_font->getGlyph(c).advanceX * m_renderScale;

			float centerY = allocatedGeometry.position.y + (allocatedGeometry.size.y * 0.5f);
			Vec2 textPos = {
				allocatedGeometry.position.x + (allocatedGeometry.size.x - textWidth) * 0.5f,
				centerY + (4.0f * m_renderScale)
			};
			outDrawList.addText(m_font, text, textPos, Silica::GetTheme().Text_Main, m_renderScale);
		}
	}

	EventReply SSliderInt::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDragging) {
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SSliderInt::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (allocatedGeometry.contains(mousePos)) {
			m_isDragging = true;
			SWidget::setCapturedWidget(this);
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SSliderInt::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (m_isDragging) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	void SSliderInt::updateValueFromMouse(float mouseX) {
		if (m_max <= m_min) return;

		float thumbWidth = 4.0f * m_renderScale;
		float thumbHalf = thumbWidth * 0.5f;
		float localX = mouseX - (m_allocatedGeometry.position.x + thumbHalf);
		float travelRange = m_allocatedGeometry.size.x - thumbWidth;

		float percentage = (travelRange > 0.0f) ? std::clamp(localX / travelRange, 0.0f, 1.0f) : 0.0f;

		int newValue = m_min + static_cast<int>(std::round(percentage * (m_max - m_min)));

		// -- Snapping Logic --
		bool isShiftHeld = Platform::isKeyDown(Key::LeftShift) || Platform::isKeyDown(Key::RightShift);
		if (isShiftHeld) {
			newValue = (int)(m_min + (std::round((float)(newValue - m_min) / m_snapStep) * m_snapStep));
		}

		newValue = std::clamp(newValue, m_min, m_max);

		if (m_value != newValue) {
			m_value = newValue;
			if (m_onValueChanged) {
				m_onValueChanged(m_value);
			}
		}
	}

	EventReply SSliderInt::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (allocatedGeometry.contains(mousePos)) {
			bool isShiftHeld = Platform::isKeyDown(Key::LeftShift) || Platform::isKeyDown(Key::RightShift);
			int step = isShiftHeld ? m_snapStep : 1;
			int newValue = m_value + (static_cast<int>(scrollDelta) * step);

			newValue = std::clamp(newValue, m_min, m_max);

			if (newValue != m_value) {
				m_value = newValue;
				if (m_onValueChanged) {
					m_onValueChanged(m_value);
				}
				return EventReply::handled();
			}
		}
		return EventReply::unhandled();
	}

}
