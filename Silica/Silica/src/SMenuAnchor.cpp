#include "SMenuAnchor.h"

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SMenuAnchor::construct(const Args& args) {
		m_anchorContent = args.anchorContent;
		m_menuContent = args.menuContent;
		m_openOnHover = args.openOnHover;
		m_openToRight = args.openToRight;
		m_showArrow = args.showArrow;
	}

	void SMenuAnchor::computeDesiredSize() {
		if (m_anchorContent) {
			m_anchorContent->computeDesiredSize();
			m_desiredSize = m_anchorContent->getDesiredSize();
		}
		else {
			m_desiredSize = Vec2::zero();
		}

		if (m_menuContent) {
			m_menuContent->computeDesiredSize();
		}
	}

	void SMenuAnchor::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		// -- Arrange The Normal Button --
		if (m_anchorContent) {
			m_anchorContent->arrangeChildren(allocatedGeometry);
		}

		// -- Arrange The Floating Menu --
		if (m_isOpen && m_menuContent) {
			Vec2 menuDesired = m_menuContent->getDesiredSize();
			m_menuGeometry.size = menuDesired;

			if (m_openToRight) {
				m_menuGeometry.position = { allocatedGeometry.position.x + allocatedGeometry.size.x, allocatedGeometry.position.y };
			}
			else {
				m_menuGeometry.position = { allocatedGeometry.position.x, allocatedGeometry.position.y + allocatedGeometry.size.y };
			}

			m_menuContent->arrangeChildren(m_menuGeometry);

			Renderer::pushPopup(m_menuContent, m_menuGeometry, [this]() {
				m_isOpen = false;
			});
		}
	}

	void SMenuAnchor::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw The Button --
		if (m_anchorContent) {
			m_anchorContent->onDraw(outDrawList, m_anchorContent->getAllocatedGeometry());
		}

		// --- Draw The Arrow ---
		if (m_showArrow) {
			Vec2 arrowCenter = {
				allocatedGeometry.position.x + allocatedGeometry.size.x - 12.0f,
				allocatedGeometry.position.y + (allocatedGeometry.size.y * 0.5f)
			};

			drawTriangle(outDrawList, arrowCenter, 6.0f, GetTheme().textMain);
		}
	}

	EventReply SMenuAnchor::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_anchorContent) {
			m_anchorContent->onMouseMove(m_anchorContent->getAllocatedGeometry(), mousePos);
		}

		Vec2 realMouse = Renderer::s_mousePosition;
		bool isHoveringAnchorReal = allocatedGeometry.contains(realMouse);

		if (m_openOnHover && isHoveringAnchorReal && !m_isOpen) {
			m_isOpen = true;
		}

		if (m_isOpen && m_openOnHover) {
			bool isHoveringMenu = m_menuGeometry.contains(realMouse);
			if (!isHoveringAnchorReal && !isHoveringMenu) {
				m_isOpen = false;
			}
		}

		bool isHoveringVisually = allocatedGeometry.contains(mousePos);

		return isHoveringVisually ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SMenuAnchor::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (allocatedGeometry.contains(mousePos)) {
			if (!m_openOnHover) m_isOpen = !m_isOpen;
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SMenuAnchor::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_anchorContent && allocatedGeometry.contains(mousePos)) {
			return m_anchorContent->onMouseButtonUp(m_anchorContent->getAllocatedGeometry(), mousePos, button);
		}

		return EventReply::unhandled();
	}

	void SMenuAnchor::drawTriangle(DrawList& drawList, const Vec2& center, float radius, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		Vec2 p0 = { center.x - (radius * 0.5f), center.y - radius };
		Vec2 p1 = { center.x - (radius * 0.5f), center.y + radius };
		Vec2 p2 = { center.x + (radius * 0.5f), center.y };

		drawList.vertices.push_back({ p0, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ p1, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ p2, {0.0f, 0.0f}, color });

		drawList.indices.push_back(startIndex + 0);
		drawList.indices.push_back(startIndex + 1);
		drawList.indices.push_back(startIndex + 2);

		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 3;
	}

}
