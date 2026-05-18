#include "SScrollBox.h"

#include "Theme.h"

namespace Silica {

	void SScrollBox::construct(const Args& args) {
		m_child = args.child;
		m_scrollSpeed = args.scrollSpeed;
		m_thumbColor = args.thumbColor.value_or(GetTheme().buttonNormal);
		m_thumbDraggingColor = args.thumbDraggingColor.value_or(GetTheme().buttonHover);
	}

	void SScrollBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}
	}

	void SScrollBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_child) {
			Vec2 childDesired = m_child->getDesiredSize();

			// -- Calculate How Far Allowed To Scroll --
			m_maxScroll = childDesired.y - allocatedGeometry.size.y;
			if (m_maxScroll < 0.0f) m_maxScroll = 0.0f;

			// -- Clamp Current Scroll --
			if (m_scrollOffset > m_maxScroll) m_scrollOffset = m_maxScroll;
			if (m_scrollOffset < 0.0f) m_scrollOffset = 0.0f;

			// -- Shift Child Up --
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x;
			childGeo.position.y = allocatedGeometry.position.y - m_scrollOffset;
			childGeo.size.x = allocatedGeometry.size.x;
			childGeo.size.y = childDesired.y;

			m_child->arrangeChildren(childGeo);
		}
	}

	void SScrollBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_child) {
			Rect myRect(
				allocatedGeometry.position.x, allocatedGeometry.position.x + allocatedGeometry.size.x,
				allocatedGeometry.position.y, allocatedGeometry.position.y + allocatedGeometry.size.y
			);

			// -- Draw and Clip the Content --
			outDrawList.pushClipRect(myRect);
			m_child->onDraw(outDrawList, m_child->getAllocatedGeometry());
			outDrawList.popClipRect();


			// -- Draw Scroll Bar --
			if (m_maxScroll > 0.0f) {
				float visibleRatio = allocatedGeometry.size.y / m_child->getDesiredSize().y;

				float thumbHeight = allocatedGeometry.size.y * visibleRatio;
				if (thumbHeight < 20.0f) thumbHeight = 20.0f;

				float scrollRatio = m_scrollOffset / m_maxScroll;
				float availableTrack = allocatedGeometry.size.y - thumbHeight;
				float thumbY = allocatedGeometry.position.y + (scrollRatio * availableTrack);

				Geometry thumbGeo;
				thumbGeo.position.x = allocatedGeometry.position.x + allocatedGeometry.size.x - 8.0f;
				thumbGeo.position.y = thumbY;
				thumbGeo.size.x = 8.0f;
				thumbGeo.size.y = thumbHeight;

				Color drawColor = m_isDraggingThumb ? m_thumbDraggingColor : m_thumbColor;
				addRectToDrawList(outDrawList, thumbGeo, drawColor);
			}
		}
	}

	EventReply SScrollBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (allocatedGeometry.contains(mousePos)) {
			if (m_child && m_child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = m_child->onMouseWheel(m_child->getAllocatedGeometry(), mousePos, scrollDelta);
				if (reply.isHandled) return reply;
			}

			m_scrollOffset -= scrollDelta * m_scrollSpeed;

			if (m_scrollOffset > m_maxScroll) m_scrollOffset = m_maxScroll;
			if (m_scrollOffset < 0.0f) m_scrollOffset = 0.0f;

			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SScrollBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDraggingThumb) {
			float visibleRatio = allocatedGeometry.size.y / m_child->getDesiredSize().y;
			float thumbHeight = allocatedGeometry.size.y * visibleRatio;
			if (thumbHeight < 20.0f) thumbHeight = 20.0f;
			float availableTrack = allocatedGeometry.size.y - thumbHeight;

			if (availableTrack > 0.0f) {
				float targetThumbY = mousePos.y - m_dragClickOffsetY;

				if (targetThumbY < allocatedGeometry.position.y) targetThumbY = allocatedGeometry.position.y;
				if (targetThumbY > allocatedGeometry.position.y + availableTrack) targetThumbY = allocatedGeometry.position.y + availableTrack;

				float scrollRatio = (targetThumbY - allocatedGeometry.position.y) / availableTrack;
				m_scrollOffset = scrollRatio * m_maxScroll;
			}
			return EventReply::handled();
		}

		if (m_child) return m_child->onMouseMove(m_child->getAllocatedGeometry(), mousePos);
		return EventReply::unhandled();
	}

	EventReply SScrollBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		Rect thumbRect = getThumbRect(allocatedGeometry);
		if (thumbRect.contains(mousePos)) {
			m_isDraggingThumb = true;
			m_dragClickOffsetY = mousePos.y - thumbRect.top;

			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		if (m_child) return m_child->onMouseButtonDown(m_child->getAllocatedGeometry(), mousePos);
		return EventReply::unhandled();
	}

	EventReply SScrollBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDraggingThumb) {
			m_isDraggingThumb = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		if (m_child) return m_child->onMouseButtonUp(m_child->getAllocatedGeometry(), mousePos);
		return EventReply::unhandled();
	}

	void SScrollBox::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
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

	Rect SScrollBox::getThumbRect(const Geometry& allocatedGeometry) const {
		if (m_maxScroll <= 0.0f) return Rect(0, 0, 0, 0);

		float visibleRatio = allocatedGeometry.size.y / m_child->getDesiredSize().y;
		float thumbHeight = allocatedGeometry.size.y * visibleRatio;
		if (thumbHeight < 20.0f) thumbHeight = 20.0f;

		float scrollRatio = m_scrollOffset / m_maxScroll;
		float availableTrack = allocatedGeometry.size.y - thumbHeight;
		float thumbY = allocatedGeometry.position.y + (scrollRatio * availableTrack);

		return Rect(
			allocatedGeometry.position.x + allocatedGeometry.size.x - 8.0f,
			allocatedGeometry.position.x + allocatedGeometry.size.x,
			thumbY,
			thumbY + thumbHeight
		);
	}

}