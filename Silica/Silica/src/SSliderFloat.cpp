#include "SSliderFloat.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SSliderFloat::construct(const Args& args) {
		m_min = args.minValue;
		m_max = std::max(args.minValue, args.maxValue);
		m_value = std::clamp(args.initialValue, m_min, m_max);
		m_snapStep = args.snapStep;
		m_showText = args.showText;
		m_format = args.format;
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_trackColor = args.trackColor.value_or(GetTheme().Element_Pressed);
		m_fillColor = args.fillColor.value_or(GetTheme().Accent_Primary);
		m_thumbColor = args.thumbColor.value_or(GetTheme().Text_Dim);
		m_thumbDraggingColor = args.thumbDraggingColor.value_or(GetTheme().Text_Main);
		m_onValueChanged = args.onValueChanged;
	}

	void SSliderFloat::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f * m_renderScale, 20.0f * m_renderScale);
	}

	void SSliderFloat::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SSliderFloat::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		float percentage = 0.0f;
		if (m_max > m_min) percentage = std::clamp((m_value - m_min) / (m_max - m_min), 0.0f, 1.0f);

		// -- Draw Background Track --
		outDrawList.addRect(allocatedGeometry, m_trackColor);

		// -- Calculate Slider Constraints --
		float thumbWidth = 4.0f * m_renderScale;
		float thumbHalf = thumbWidth * 0.5f;
		float travelRange = allocatedGeometry.size.x - thumbWidth;
		float thumbLeftX = allocatedGeometry.position.x + (travelRange * percentage);

		// -- Draw Fill Track --
		Geometry fillGeo = allocatedGeometry;
		fillGeo.size.x = (thumbLeftX + thumbHalf) - allocatedGeometry.position.x;
		outDrawList.addRect(fillGeo, m_fillColor);

		// -- Draw Thumb Handle --
		Geometry thumbGeo;
		thumbGeo.position.x = thumbLeftX;
		thumbGeo.position.y = allocatedGeometry.position.y;
		thumbGeo.size.x = thumbWidth;
		thumbGeo.size.y = allocatedGeometry.size.y;

		Color thumbColor = m_isDragging ? m_thumbDraggingColor : m_thumbColor;
		outDrawList.addRect(thumbGeo, thumbColor);

		// -- Draw Centered Value Text --
		if (m_showText && m_font) {
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), m_format.c_str(), m_value);
			std::string text = buffer;

			float textWidth = 0.0f;
			for (char c : text) {
				textWidth += m_font->getGlyph(c).advanceX * m_renderScale;
			}

			float centerY = allocatedGeometry.position.y + (allocatedGeometry.size.y * 0.5f);

			Vec2 textPos = {
				allocatedGeometry.position.x + (allocatedGeometry.size.x - textWidth) * 0.5f,
				centerY + (4.0f * m_renderScale)
			};

			outDrawList.addText(m_font, text, textPos, Silica::GetTheme().Text_Main, m_renderScale);
		}
	}

	EventReply SSliderFloat::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDragging) {
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SSliderFloat::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (allocatedGeometry.contains(mousePos)) {
			m_isDragging = true;
			SWidget::setCapturedWidget(this);
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SSliderFloat::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (m_isDragging) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	void SSliderFloat::updateValueFromMouse(float mouseX) {
		if (m_max <= m_min) return;

		float thumbWidth = 4.0f * m_renderScale;
		float thumbHalf = thumbWidth * 0.5f;
		float localX = mouseX - (m_allocatedGeometry.position.x + thumbHalf);
		float travelRange = m_allocatedGeometry.size.x - thumbWidth;

		float percentage = 0.0f;
		if (travelRange > 0.0f) {
			percentage = std::clamp(localX / travelRange, 0.0f, 1.0f);
		}

		float newValue = m_min + (percentage * (m_max - m_min));
		bool isShiftHeld = Platform::isKeyDown(Key::LeftShift) || Platform::isKeyDown(Key::RightShift);

		// -- Apply Snapping --
		if (m_snapStep > 0.0f && isShiftHeld) {
			newValue = m_min + std::round((newValue - m_min) / m_snapStep) * m_snapStep;
		}

		newValue = std::clamp(newValue, m_min, m_max);

		if (m_value != newValue) {
			m_value = newValue;
			if (m_onValueChanged) {
				m_onValueChanged(m_value);
			}
		}
	}

	EventReply SSliderFloat::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (allocatedGeometry.contains(mousePos)) {
			bool isShiftHeld = Platform::isKeyDown(Key::LeftShift) || Platform::isKeyDown(Key::RightShift);
			float step = (m_snapStep > 0.0f) ? m_snapStep : 1.0f;
			float delta = isShiftHeld ? (step * 5.0f) : step;

			float newValue = m_value + (scrollDelta * delta);

			if (m_snapStep > 0.0f) {
				newValue = m_min + std::round((newValue - m_min) / m_snapStep) * m_snapStep;
			}

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
