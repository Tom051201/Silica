#include "SSliderInt.h"

#include <algorithm>
#include <cmath>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SSliderInt::construct(const Args& args) {
		m_min = args.minValue;
		m_max = std::max(args.minValue, args.maxValue);
		m_value = std::clamp(args.initialValue, m_min, m_max);

		m_trackColor = args.trackColor.value_or(GetTheme().buttonPressed);
		m_fillColor = args.fillColor.value_or(GetTheme().accentPrimary);
		m_thumbColor = args.thumbColor.value_or(GetTheme().textDim);
		m_thumbDraggingColor = args.thumbDraggingColor.value_or(GetTheme().textMain);
		m_onValueChanged = args.onValueChanged;
	}

	void SSliderInt::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f, 20.0f);
	}

	void SSliderInt::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SSliderInt::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Calculate Visual Percentage --
		float percentage = 0.0f;
		if (m_max > m_min) percentage = static_cast<float>(m_value - m_min) / static_cast<float>(m_max - m_min);

		// -- Draw Background Track --
		addRectToDrawList(outDrawList, allocatedGeometry, m_trackColor);

		// -- Draw Fill Track --
		Geometry fillGeo = allocatedGeometry;
		fillGeo.size.x = allocatedGeometry.size.x * percentage;
		addRectToDrawList(outDrawList, fillGeo, m_fillColor);

		// -- Draw Thumb Handle --
		if (fillGeo.size.x > 2.0f) {
			Geometry thumbGeo;
			thumbGeo.position.x = fillGeo.position.x + fillGeo.size.x - 2.0f;
			thumbGeo.position.y = fillGeo.position.y;
			thumbGeo.size.x = 4.0f;
			thumbGeo.size.y = fillGeo.size.y;

			Color thumbColor = m_isDragging ? m_thumbDraggingColor : m_thumbColor;
			addRectToDrawList(outDrawList, thumbGeo, thumbColor);
		}
	}

	void SSliderInt::updateValueFromMouse(float mouseX) {
		if (m_max <= m_min) return;

		float localX = mouseX - m_allocatedGeometry.position.x;
		float percentage = std::clamp(localX / m_allocatedGeometry.size.x, 0.0f, 1.0f);

		int newValue = m_min + static_cast<int>(std::round(percentage * (m_max - m_min)));
		newValue = std::clamp(newValue, m_min, m_max);

		if (m_value != newValue) {
			m_value = newValue;
			if (m_onValueChanged) {
				m_onValueChanged(m_value);
			}
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
		if (allocatedGeometry.contains(mousePos)) {
			m_isDragging = true;
			SWidget::setCapturedWidget(this);
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SSliderInt::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_isDragging) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	void SSliderInt::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color });

		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 1); drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 2); drawList.indices.push_back(startIndex + 3);
		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 6;
	}

}
