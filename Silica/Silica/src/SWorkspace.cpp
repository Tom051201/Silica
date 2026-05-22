#include "SWorkspace.h"

#include <algorithm>

namespace Silica {

	void SWorkspace::construct(const Args& args) {
		m_font = args.font;

		m_dockSpace = MakeWidget<SDockSpace>({
			.initialTabs = { { args.initialTitle, args.initialContent, Rect() } },
			.font = m_font,
			.onUndockWindow = [this](std::string title, WidgetPtr content, Vec2 mousePos) {
				auto newWin = MakeWidget<SWindow>({
					.title = title,
					.initialPosition = { mousePos.x - 150.0f, mousePos.y - 15.0f },
					.initialSize = { 300.0f, 200.0f },
					.font = m_font,
					.content = content
				});
				newWin->startDragging(mousePos);
				addFloatingWindow(newWin);
			}
		});
	}

	void SWorkspace::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		m_dockSpace->computeDesiredSize();
		for (auto& win : m_floatingWindows) win->computeDesiredSize();
	}

	void SWorkspace::arrangeChildren(const Geometry & allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_dockSpace->arrangeChildren(allocatedGeometry);
		for (auto& win : m_floatingWindows) win->arrangeChildren(allocatedGeometry);
	}

	void SWorkspace::onDraw(DrawList & outDrawList, const Geometry & allocatedGeometry) const {
		// -- Draw DockSpace First (Background) --
		m_dockSpace->onDraw(outDrawList, allocatedGeometry);

		// -- Draw Floating Windows On Top --
		for (auto& win : m_floatingWindows) {
			win->onDraw(outDrawList, win->getAllocatedGeometry());
		}
	}

	EventReply SWorkspace::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		EventReply reply = EventReply::unhandled();

		for (int i = (int)m_floatingWindows.size() - 1; i >= 0; --i) {
			if (m_floatingWindows[i]->onMouseMove(m_floatingWindows[i]->getAllocatedGeometry(), mousePos).isHandled) {
				reply = EventReply::handled();
				break;
			}
		}

		if (!reply.isHandled) {
			reply = m_dockSpace->onMouseMove(m_dockSpace->getAllocatedGeometry(), mousePos);
		}
		return reply;
	}

	EventReply SWorkspace::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		for (int i = (int)m_floatingWindows.size() - 1; i >= 0; --i) {
			auto win = m_floatingWindows[i];
			if (win->onMouseButtonDown(win->getAllocatedGeometry(), mousePos).isHandled) {
				m_floatingWindows.erase(m_floatingWindows.begin() + i);
				m_floatingWindows.push_back(win);
				return EventReply::handled();
			}
		}

		// -- Fallback to DockSpace --
		return m_dockSpace->onMouseButtonDown(m_dockSpace->getAllocatedGeometry(), mousePos);
	}

	EventReply SWorkspace::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		EventReply reply = EventReply::unhandled();

		for (int i = (int)m_floatingWindows.size() - 1; i >= 0; --i) {
			if (m_floatingWindows[i]->onMouseButtonUp(m_floatingWindows[i]->getAllocatedGeometry(), mousePos).isHandled) {
				reply = EventReply::handled();
				break;
			}
		}
		if (!reply.isHandled) reply = m_dockSpace->onMouseButtonUp(m_dockSpace->getAllocatedGeometry(), mousePos);
		return reply;
	}

	EventReply SWorkspace::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (auto it = m_floatingWindows.rbegin(); it != m_floatingWindows.rend(); ++it) {
			if ((*it)->onMouseWheel((*it)->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
		}
		return m_dockSpace->onMouseWheel(m_dockSpace->getAllocatedGeometry(), mousePos, scrollDelta);
	}

	void SWorkspace::addFloatingWindow(std::shared_ptr<SWindow> window) {
		window->onDragMove = [this](Vec2 mousePos) {
			m_dockSpace->updateDragDropPreview(mousePos, true);
		};

		window->onDragEnd = [this, window](Vec2 mousePos) {
			if (m_dockSpace->processDrop(window->getTitle(), window->getContent())) {
				window->setContent(nullptr);
				auto it = std::find(m_floatingWindows.begin(), m_floatingWindows.end(), window);
				if (it != m_floatingWindows.end()) m_floatingWindows.erase(it);
			}
			m_dockSpace->updateDragDropPreview(mousePos, false);
		};

		m_floatingWindows.push_back(window);
	}

}
