#include "SCheckBox.h"

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SCheckBox::construct(const Args& args) {
		m_isChecked = args.initialCheck;
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().buttonPressed);
		m_checkColor = args.backgroundColor.value_or(GetTheme().accentPrimary);
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
		addRectToDrawList(outDrawList, allocatedGeometry, m_backgroundColor);

		// -- Draw the inner "Check" --
		if (m_isChecked) {
			Geometry checkGeo;
			checkGeo.position.x = allocatedGeometry.position.x + 4.0f;
			checkGeo.position.y = allocatedGeometry.position.y + 4.0f;
			checkGeo.size.x = allocatedGeometry.size.x - 8.0f;
			checkGeo.size.y = allocatedGeometry.size.y - 8.0f;

			addRectToDrawList(outDrawList, checkGeo, m_checkColor);
		}
	}

	EventReply SCheckBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (allocatedGeometry.contains(mousePos)) {
			m_isChecked = !m_isChecked;

			if (m_onCheckChanged) {
				m_onCheckChanged(m_isChecked);
			}
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	void SCheckBox::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
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