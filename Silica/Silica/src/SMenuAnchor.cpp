#include "silicapch.h"
#include "SMenuAnchor.h"
#include "Renderer.h"

namespace Silica {

	static SMenuAnchor* s_activeGroupedAnchor = nullptr;

	void SMenuAnchor::construct(const Args& args) {
		m_anchorContent = args.anchorContent;
		m_menuContent = args.menuContent;
		m_openOnHover = args.openOnHover;
		m_openOnRightClick = args.openOnRightClick;
		m_openToRight = args.openToRight;
		m_showArrow = args.showArrow;
		m_openAtMousePos = args.openAtMousePos;
		m_arrowNormal = args.arrowNormal.value_or(GetTheme().Text_Dim);
		m_arrowHover = args.arrowHover.value_or(GetTheme().Text_Main);
		m_hoverGroup = args.hoverGroup.value_or("");
	}

	void SMenuAnchor::computeDesiredSize() {
		if (m_anchorContent) {
			m_anchorContent->computeDesiredSize();
			m_desiredSize = m_anchorContent->getDesiredSize();
		}
		else {
			m_desiredSize = Vec2::zero();
		}

		if (m_showArrow) {
			m_desiredSize.x += 16.0f * m_renderScale;
		}

		if (m_menuContent) {
			m_menuContent->computeDesiredSize();
		}
	}

	void SMenuAnchor::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		// -- Arrange The Normal Button --
		if (m_anchorContent) {
			Geometry anchorGeo = allocatedGeometry;
			if (m_showArrow) anchorGeo.size.x -= 16.0f * m_renderScale;
			m_anchorContent->arrangeChildren(anchorGeo);
		}

		// -- Arrange The Floating Menu --
		if (m_isOpen && m_menuContent) {
			Vec2 menuDesired = m_menuContent->getDesiredSize();
			m_menuGeometry.size = menuDesired;

			Vec2 originPos = m_openAtMousePos ? m_clickPos : allocatedGeometry.position;

			if (m_openToRight) {
				m_menuGeometry.position = { originPos.x + (m_openAtMousePos ? 0.0f : allocatedGeometry.size.x), originPos.y };
			}
			else {
				m_menuGeometry.position = { originPos.x, originPos.y + (m_openAtMousePos ? 0.0f : allocatedGeometry.size.y) };
			}

			m_menuContent->arrangeChildren(m_menuGeometry);

			Renderer::pushPopup(m_menuContent, m_menuGeometry, [this]() {
				m_isOpen = false;
				if (!m_hoverGroup.empty() && s_activeGroupedAnchor == this) {
					s_activeGroupedAnchor = nullptr;
				}
			});
		}
	}

	void SMenuAnchor::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw The Button --
		if (m_anchorContent) {
			m_anchorContent->onDraw(outDrawList, m_anchorContent->getAllocatedGeometry());
		}

		// -- Draw The Arrow --
		if (m_showArrow) {
			Vec2 arrowCenter = {
				allocatedGeometry.position.x + allocatedGeometry.size.x - (8.0f * m_renderScale),
				allocatedGeometry.position.y + allocatedGeometry.size.y * 0.5f
			};

			Color arrowColor = m_isHovered ? m_arrowHover : m_arrowNormal;
			drawTriangle(outDrawList, arrowCenter, 5.0f * m_renderScale, arrowColor);
		}
	}

	void SMenuAnchor::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_anchorContent) m_anchorContent->setRenderScale(scale);
		if (m_menuContent) m_menuContent->setRenderScale(scale);
	}

	EventReply SMenuAnchor::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		Vec2 realMouse = Renderer::getMousePosition();
		bool isHoveringAnchorReal = allocatedGeometry.contains(realMouse);

		// Generic Group Gliding
		if (!m_hoverGroup.empty() && isHoveringAnchorReal && !m_isOpen && s_activeGroupedAnchor != nullptr && s_activeGroupedAnchor != this) {
			if (s_activeGroupedAnchor->m_hoverGroup == m_hoverGroup) {
				s_activeGroupedAnchor->closeMenu();
				s_activeGroupedAnchor = this;
				m_isOpen = true;
			}
		}

		if (m_openOnHover && isHoveringAnchorReal && !m_isOpen) {
			m_isOpen = true;
		}

		if (m_isOpen && m_openOnHover) {
			bool isHoveringMenu = m_menuGeometry.contains(realMouse);
			if (!isHoveringAnchorReal && !isHoveringMenu) {
				m_isOpen = false;
			}
		}

		m_isHovered = allocatedGeometry.contains(mousePos) || m_isOpen;

		Vec2 childMousePos = mousePos;
		if (m_isOpen) {
			// Force the button visually into a hover state while the menu is open
			childMousePos = { allocatedGeometry.position.x + 1.0f, allocatedGeometry.position.y + 1.0f };
		}

		if (m_anchorContent) {
			m_anchorContent->onMouseMove(m_anchorContent->getAllocatedGeometry(), childMousePos);
		}

		// DO NOT return the child's reply! If we are open, the child returns 'handled' 
		// because of the fake coordinates, which breaks all sibling widgets in the layout!
		return allocatedGeometry.contains(mousePos) ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SMenuAnchor::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left && button != MouseButton::Right) return EventReply::unhandled();

		EventReply childReply = EventReply::unhandled();

		if (m_anchorContent) {
			childReply = m_anchorContent->onMouseButtonDown(m_anchorContent->getAllocatedGeometry(), mousePos, button);
		}

		if (allocatedGeometry.contains(mousePos)) {
			if (m_openOnRightClick && button == MouseButton::Right && childReply.isHandled) {
				return childReply;
			}

			if (!m_openOnHover) {
				if (m_openOnRightClick && button == MouseButton::Right) {
					m_isOpen = !m_isOpen;
					m_clickPos = mousePos;
				}
				else if (!m_openOnRightClick && button == MouseButton::Left) {
					m_isOpen = !m_isOpen;
					m_clickPos = mousePos;

					if (!m_hoverGroup.empty()) {
						if (m_isOpen) {
							if (s_activeGroupedAnchor && s_activeGroupedAnchor != this) {
								s_activeGroupedAnchor->closeMenu();
							}
							s_activeGroupedAnchor = this;
						}
						else {
							if (s_activeGroupedAnchor == this) s_activeGroupedAnchor = nullptr;
						}
					}
				}
			}

			return childReply.isHandled ? childReply : EventReply::handled();
		}

		if (m_isOpen && !m_menuGeometry.contains(mousePos) && !allocatedGeometry.contains(mousePos)) {
			m_isOpen = false;
			if (!m_hoverGroup.empty() && s_activeGroupedAnchor == this) {
				s_activeGroupedAnchor = nullptr;
			}
		}

		return childReply;
	}

	EventReply SMenuAnchor::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_anchorContent) {
			EventReply reply = m_anchorContent->onMouseButtonUp(m_anchorContent->getAllocatedGeometry(), mousePos, button);
			if (reply.isHandled) return reply;
		}
		return EventReply::unhandled();
	}

	EventReply SMenuAnchor::onMouseWheel(const Geometry& geom, const Vec2& pos, float delta) {
		if (m_anchorContent) {
			return m_anchorContent->onMouseWheel(m_anchorContent->getAllocatedGeometry(), pos, delta);
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

	void SMenuAnchor::closeMenu() {
		m_isOpen = false;
	}

	bool SMenuAnchor::isOpen() const {
		return m_isOpen;
	}

	EventReply SMenuAnchor::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_anchorContent && m_anchorContent->getAllocatedGeometry().contains(mousePos)) {
			return m_anchorContent->onDragOver(m_anchorContent->getAllocatedGeometry(), mousePos, payload);
		}
		return EventReply::unhandled();
	}

	EventReply SMenuAnchor::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_anchorContent && m_anchorContent->getAllocatedGeometry().contains(mousePos)) {
			return m_anchorContent->onDrop(m_anchorContent->getAllocatedGeometry(), mousePos, payload);
		}
		return EventReply::unhandled();
	}

}
