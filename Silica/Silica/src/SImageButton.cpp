#include "SImageButton.h"

namespace Silica {

	void SImageButton::construct(const Args& args) {
		m_textureID = args.textureID;
		m_desiredSize = args.desiredSize;

		m_normalTint = args.normalTint.value_or(Color(200, 200, 200, 255));
		m_hoverTint = args.hoverTint.value_or(Color(255, 255, 255, 255));
		m_pressedTint = args.pressedTint.value_or(Color(120, 120, 120, 255));

		m_onClick = args.onClick;
	}

	void SImageButton::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SImageButton::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_textureID == 0) return;

		Color currentTint = m_isPressed ? m_pressedTint : (m_isHovered ? m_hoverTint : m_normalTint);
		if (currentTint.a() == 0) return;

		outDrawList.pushTextureID(m_textureID);
		uint32_t startIndex = (uint32_t)outDrawList.vertices.size();

		outDrawList.vertices.push_back({ {allocatedGeometry.position.x, allocatedGeometry.position.y}, {0.0f, 0.0f}, currentTint });
		outDrawList.vertices.push_back({ {allocatedGeometry.position.x + allocatedGeometry.size.x, allocatedGeometry.position.y}, {1.0f, 0.0f}, currentTint });
		outDrawList.vertices.push_back({ {allocatedGeometry.position.x + allocatedGeometry.size.x, allocatedGeometry.position.y + allocatedGeometry.size.y}, {1.0f, 1.0f}, currentTint });
		outDrawList.vertices.push_back({ {allocatedGeometry.position.x, allocatedGeometry.position.y + allocatedGeometry.size.y}, {0.0f, 1.0f}, currentTint });

		outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 1); outDrawList.indices.push_back(startIndex + 2);
		outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 2); outDrawList.indices.push_back(startIndex + 3);

		if (outDrawList.commands.empty()) outDrawList.commands.push_back({ 0, 0, 0 });
		outDrawList.commands.back().indexCount += 6;

		outDrawList.popTextureID();
	}

	EventReply SImageButton::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		m_isHovered = allocatedGeometry.contains(mousePos);

		if (!m_isHovered) {
			m_isPressed = false;
		}

		return m_isHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SImageButton::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (allocatedGeometry.contains(mousePos)) {
			m_isPressed = true;
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SImageButton::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (m_isPressed || SWidget::getCapturedWidget() == this) {
			m_isPressed = false;
			SWidget::setCapturedWidget(nullptr);

			if (allocatedGeometry.contains(mousePos) && m_onClick) {
				return m_onClick();
			}
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

}
