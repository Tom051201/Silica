#include "SBorderLayout.h"

#include <algorithm>

namespace Silica {

	void SBorderLayout::construct(const Args& args) {
		m_topBar = args.topBar;
		m_contentArea = args.contentArea;
	}

	void SBorderLayout::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		if (m_topBar) {
			m_topBar->computeDesiredSize();
			m_desiredSize.y += m_topBar->getDesiredSize().y;
			m_desiredSize.x = std::max(m_desiredSize.x, m_topBar->getDesiredSize().x);
		}

		if (m_contentArea) {
			m_contentArea->computeDesiredSize();
			m_desiredSize.y += m_contentArea->getDesiredSize().y;
			m_desiredSize.x = std::max(m_desiredSize.x, m_contentArea->getDesiredSize().x);
		}
	}

	void SBorderLayout::arrangeChildren(const Geometry & allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float topHeight = 0.0f;

		if (m_topBar) {
			topHeight = m_topBar->getDesiredSize().y;
			Geometry topGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, topHeight} };
			m_topBar->arrangeChildren(topGeo);
		}

		if (m_contentArea) {
			Geometry contentGeo = {
				{allocatedGeometry.position.x, allocatedGeometry.position.y + topHeight},
				{allocatedGeometry.size.x, std::max(0.0f, allocatedGeometry.size.y - topHeight)}
			};
			m_contentArea->arrangeChildren(contentGeo);
		}
	}

	void SBorderLayout::onDraw(DrawList & outDrawList, const Geometry & allocatedGeometry) const {
		if (m_topBar) m_topBar->onDraw(outDrawList, m_topBar->getAllocatedGeometry());
		if (m_contentArea) m_contentArea->onDraw(outDrawList, m_contentArea->getAllocatedGeometry());
	}

	EventReply SBorderLayout::onMouseMove(const Geometry & allocatedGeometry, const Vec2 & mousePos) {
		if (m_topBar && m_topBar->onMouseMove(m_topBar->getAllocatedGeometry(), mousePos).isHandled) return EventReply::handled();
		if (m_contentArea) return m_contentArea->onMouseMove(m_contentArea->getAllocatedGeometry(), mousePos);
		return EventReply::unhandled();
	}

	EventReply SBorderLayout::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_topBar && m_topBar->onMouseButtonDown(m_topBar->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		if (m_contentArea) return m_contentArea->onMouseButtonDown(m_contentArea->getAllocatedGeometry(), mousePos, button);
		return EventReply::unhandled();
	}

	EventReply SBorderLayout::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_topBar && m_topBar->onMouseButtonUp(m_topBar->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		if (m_contentArea) return m_contentArea->onMouseButtonUp(m_contentArea->getAllocatedGeometry(), mousePos, button);
		return EventReply::unhandled();
	}

	EventReply SBorderLayout::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_topBar && m_topBar->onMouseWheel(m_topBar->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
		if (m_contentArea) return m_contentArea->onMouseWheel(m_contentArea->getAllocatedGeometry(), mousePos, scrollDelta);
		return EventReply::unhandled();
	}

	EventReply SBorderLayout::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_topBar && m_topBar->onDragOver(m_topBar->getAllocatedGeometry(), mousePos, payload).isHandled) return EventReply::handled();
		if (m_contentArea) return m_contentArea->onDragOver(m_contentArea->getAllocatedGeometry(), mousePos, payload);
		return EventReply::unhandled();
	}

	EventReply SBorderLayout::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_topBar && m_topBar->onDrop(m_topBar->getAllocatedGeometry(), mousePos, payload).isHandled) return EventReply::handled();
		if (m_contentArea) return m_contentArea->onDrop(m_contentArea->getAllocatedGeometry(), mousePos, payload);
		return EventReply::unhandled();
	}

}
