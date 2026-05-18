#include "SSlider.h"

#include <algorithm>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SSlider::construct(const Args& args) {
		m_value = std::clamp(args.initialValue, 0.0f, 1.0f);
		m_trackColor = args.trackColor.value_or(GetTheme().buttonPressed);
		m_fillColor = args.fillColor.value_or(GetTheme().accentPrimary);
		m_thumbColor = args.thumbColor.value_or(GetTheme().textDim);
		m_thumbDraggingColor = args.thumbDraggingColor.value_or(GetTheme().textMain);
		m_onValueChanged = args.onValueChanged;
	}

	void SSlider::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f, 20.0f);
	}

	void SSlider::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SSlider::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw Background Track --
		addRectToDrawList(outDrawList, allocatedGeometry, m_trackColor);

		// -- Draw Fill Track --
		Geometry fillGeo = allocatedGeometry;
		fillGeo.size.x = allocatedGeometry.size.x * m_value;
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

	void SSlider::updateValueFromMouse(float mouseX) {
		float localX = mouseX - m_allocatedGeometry.position.x;
		float percentage = localX / m_allocatedGeometry.size.x;

		m_value = std::clamp(percentage, 0.0f, 1.0f);

		if (m_onValueChanged) {
			m_onValueChanged(m_value);
		}
	}

	EventReply SSlider::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (allocatedGeometry.contains(mousePos)) {
			m_isDragging = true;
			SWidget::setCapturedWidget(this);
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SSlider::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDragging) {
			updateValueFromMouse(mousePos.x);
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SSlider::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDragging) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	void SSlider::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
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