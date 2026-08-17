#include "silicapch.h"
#include "SScrollBox.h"

namespace Silica {

	void SScrollBox::construct(const Args& args) {
		m_child = args.child;
		m_scrollSpeed = args.scrollSpeed;
		m_thumbColor = args.thumbColor.value_or(GetTheme().Element_Normal);
		m_thumbDraggingColor = args.thumbDraggingColor.value_or(GetTheme().Element_Hover);
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

			float reservedSpace = (m_maxScroll > 0.0f) ? (12.0f * m_renderScale) : 0.0f;
			childGeo.size.x = allocatedGeometry.size.x - reservedSpace;
			childGeo.size.y = childDesired.y;

			m_child->arrangeChildren(childGeo);
		}
	}

	void SScrollBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_child) {
			float reservedSpace = (m_maxScroll > 0.0f) ? (12.0f * m_renderScale) : 0.0f;

			Rect myRect(
				allocatedGeometry.position.x,
				allocatedGeometry.position.x + allocatedGeometry.size.x - reservedSpace,
				allocatedGeometry.position.y,
				allocatedGeometry.position.y + allocatedGeometry.size.y
			);

			// -- Draw and Clip the Content --
			outDrawList.pushClipRect(myRect);
			m_child->onDraw(outDrawList, m_child->getAllocatedGeometry());
			outDrawList.popClipRect();


			// -- Draw Scroll Bar --
			if (m_maxScroll > 0.0f) {
				float visibleRatio = allocatedGeometry.size.y / m_child->getDesiredSize().y;

				float thumbHeight = allocatedGeometry.size.y * visibleRatio;
				float minThumbHeight = 20.0f * m_renderScale;
				if (thumbHeight < minThumbHeight) thumbHeight = minThumbHeight;

				float scrollRatio = m_scrollOffset / m_maxScroll;
				float availableTrack = allocatedGeometry.size.y - thumbHeight;
				float thumbY = allocatedGeometry.position.y + (scrollRatio * availableTrack);

				float thumbWidth = 8.0f * m_renderScale;

				Geometry thumbGeo;
				thumbGeo.position.x = allocatedGeometry.position.x + allocatedGeometry.size.x - thumbWidth;
				thumbGeo.position.y = thumbY;
				thumbGeo.size.x = thumbWidth;
				thumbGeo.size.y = thumbHeight;

				Color drawColor = m_isDraggingThumb ? m_thumbDraggingColor : m_thumbColor;
				outDrawList.addRect(thumbGeo, drawColor);
			}
		}
	}

	void SScrollBox::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_child) {
			m_child->setRenderScale(scale);
		}
	}

	EventReply SScrollBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDraggingThumb) {
			float visibleRatio = allocatedGeometry.size.y / m_child->getDesiredSize().y;
			float thumbHeight = allocatedGeometry.size.y * visibleRatio;
			float minThumbHeight = 20.0f * m_renderScale;
			if (thumbHeight < minThumbHeight) thumbHeight = minThumbHeight;
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

	EventReply SScrollBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		// -- Check If Grabbed Thumb --
		if (button == MouseButton::Left) {
			Rect thumbRect = getThumbRect(allocatedGeometry);
			if (thumbRect.contains(mousePos)) {
				m_isDraggingThumb = true;
				m_dragClickOffsetY = mousePos.y - thumbRect.top;

				SWidget::setCapturedWidget(this);
				return EventReply::handled();
			}
		}

		if (m_child) return m_child->onMouseButtonDown(m_child->getAllocatedGeometry(), mousePos, button);
		return EventReply::unhandled();
	}

	EventReply SScrollBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		// -- Check Thumb --
		if (m_isDraggingThumb && button == MouseButton::Left) {
			m_isDraggingThumb = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		if (m_child) return m_child->onMouseButtonUp(m_child->getAllocatedGeometry(), mousePos, button);
		return EventReply::unhandled();
	}

	EventReply SScrollBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (allocatedGeometry.contains(mousePos)) {
			if (m_child && m_child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = m_child->onMouseWheel(m_child->getAllocatedGeometry(), mousePos, scrollDelta);
				if (reply.isHandled) return reply;
			}

			m_scrollOffset -= scrollDelta * (m_scrollSpeed * m_renderScale);

			if (m_scrollOffset > m_maxScroll) m_scrollOffset = m_maxScroll;
			if (m_scrollOffset < 0.0f) m_scrollOffset = 0.0f;

			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	Rect SScrollBox::getThumbRect(const Geometry& allocatedGeometry) const {
		if (m_maxScroll <= 0.0f) return Rect(0, 0, 0, 0);

		float visibleRatio = allocatedGeometry.size.y / m_child->getDesiredSize().y;
		float thumbHeight = allocatedGeometry.size.y * visibleRatio;
		float minThumbHeight = 20.0f * m_renderScale;
		if (thumbHeight < minThumbHeight) thumbHeight = minThumbHeight;

		float scrollRatio = m_scrollOffset / m_maxScroll;
		float availableTrack = allocatedGeometry.size.y - thumbHeight;
		float thumbY = allocatedGeometry.position.y + (scrollRatio * availableTrack);

		float thumbWidth = 8.0f * m_renderScale;

		return Rect(
			allocatedGeometry.position.x + allocatedGeometry.size.x - thumbWidth,
			allocatedGeometry.position.x + allocatedGeometry.size.x,
			thumbY,
			thumbY + thumbHeight
		);
	}

	EventReply SScrollBox::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onDragOver(m_child->getAllocatedGeometry(), mousePos, payload);
		}

		return EventReply::unhandled();
	}

	EventReply SScrollBox::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onDrop(m_child->getAllocatedGeometry(), mousePos, payload);
		}

		return EventReply::unhandled();
	}

}
