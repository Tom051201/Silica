#include "SDockSpace.h"

#include <functional>
#include <algorithm>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SDockSpace::construct(const Args& args) {
		m_font = args.font;
		m_rootNode = std::make_shared<SDockNode>();
		m_rootNode->tabs = args.initialTabs;
		m_onUndockWindow = args.onUndockWindow;
		m_titleBarColor = args.titleBarColor.value_or(GetTheme().backgroundPanel);
	}

	void SDockSpace::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		std::function<void(DockNodePtr)> computeNode = [&](DockNodePtr node) {
			if (!node) return;

			if (node->splitDirection == SplitDirection::None) {
				for (auto& tab : node->tabs) {
					if (tab.content) tab.content->computeDesiredSize();
				}
			}
			else {
				computeNode(node->child[0]);
				computeNode(node->child[1]);
			}
		};

		computeNode(m_rootNode);
	}

	void SDockSpace::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		if (m_rootNode) {
			arrangeNode(m_rootNode, allocatedGeometry);
		}
	}

	void SDockSpace::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		addRectToDrawList(outDrawList, allocatedGeometry, GetTheme().backgroundDarkWorkspace);

		if (m_rootNode) {
			drawNode(m_rootNode, outDrawList);
		}

		// -- Draw Drag & Drop Preview Overlay --
		if (m_previewNode && m_previewZone != DockZone::None) {
			Geometry pGeo = m_previewNode->allocatedGeometry;

			if (m_previewZone == DockZone::Left) pGeo.size.x /= 2.0f;
			else if (m_previewZone == DockZone::Right) { pGeo.size.x /= 2.0f; pGeo.position.x += pGeo.size.x; }
			else if (m_previewZone == DockZone::Top) pGeo.size.y /= 2.0f;
			else if (m_previewZone == DockZone::Bottom) { pGeo.size.y /= 2.0f; pGeo.position.y += pGeo.size.y; }

			addRectToDrawList(outDrawList, pGeo, Color(70, 130, 180, 128));
		}
	}

	EventReply SDockSpace::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		// -- Handle Tab Tearing --
		if (m_pressedTabNode) {
			if (std::abs(mousePos.x - m_pressedMousePos.x) > 5.0f || std::abs(mousePos.y - m_pressedMousePos.y) > 5.0f) {
				SWidget::setCapturedWidget(nullptr);
				undockNode(m_pressedTabNode, m_pressedTabIndex, mousePos);
				m_pressedTabNode = nullptr;
			}
			return EventReply::handled();
		}

		// -- Handle Splitter Dragging --
		if (m_draggingNode) {
			if (m_draggingNode->splitDirection == SplitDirection::Horizontal) {
				Platform::setCursor(Platform::Cursor::ResizeEW);
				float localX = mousePos.x - m_draggingNode->allocatedGeometry.position.x;
				float totalWidth = m_draggingNode->allocatedGeometry.size.x - m_splitterThickness;
				m_draggingNode->splitRatio = std::clamp(localX / totalWidth, 0.05f, 0.95f);
			}
			else if (m_draggingNode->splitDirection == SplitDirection::Vertical) {
				Platform::setCursor(Platform::Cursor::ResizeNS);
				float localY = mousePos.y - m_draggingNode->allocatedGeometry.position.y;
				float totalHeight = m_draggingNode->allocatedGeometry.size.y - m_splitterThickness;
				m_draggingNode->splitRatio = std::clamp(localY / totalHeight, 0.05f, 0.95f);
			}
			return EventReply::handled();
		}

		// -- Check Hovering Splitter --
		m_hoveredNode = hitTestSplitter(m_rootNode, mousePos);
		if (m_hoveredNode) {
			if (m_hoveredNode->splitterRect.contains(mousePos)) {
				Platform::setCursor(m_hoveredNode->splitDirection == SplitDirection::Horizontal ? Platform::Cursor::ResizeEW : Platform::Cursor::ResizeNS);
			}

			return EventReply::handled();
		}

		// -- Route Hover Events to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeMove = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content) {
					return node->tabs[node->activeTab].content->onMouseMove(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos);
				}
			}
			else {
				EventReply rep = routeMove(node->child[1]);
				if (rep.isHandled) return rep;
				return routeMove(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeMove(m_rootNode);
	}

	EventReply SDockSpace::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		auto hitTab = hitTestTab(m_rootNode, mousePos);
		if (hitTab.first) {
			m_pressedTabNode = hitTab.first;
			m_pressedTabIndex = hitTab.second;
			m_pressedMousePos = mousePos;
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		// -- Check if Grabbed a Splitter --
		m_draggingNode = hitTestSplitter(m_rootNode, mousePos);
		if (m_draggingNode) {
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		// -- Route The Click to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeDown = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onMouseButtonDown(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, button);
				}
			}
			else {
				EventReply rep = routeDown(node->child[1]);
				if (rep.isHandled) return rep;
				return routeDown(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeDown(m_rootNode);
	}

	EventReply SDockSpace::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_pressedTabNode) {
			m_pressedTabNode->activeTab = m_pressedTabIndex;
			m_pressedTabNode = nullptr;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		if (m_draggingNode) {
			m_draggingNode = nullptr;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		std::function<EventReply(DockNodePtr)> routeUp = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onMouseButtonUp(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, button);
				}
			}
			else {
				EventReply rep = routeUp(node->child[1]);
				if (rep.isHandled) return rep;
				return routeUp(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeUp(m_rootNode);
	}

	EventReply SDockSpace::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		std::function<EventReply(DockNodePtr)> routeWheel = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onMouseWheel(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, scrollDelta);
				}
			}
			else {
				EventReply rep = routeWheel(node->child[1]);
				if (rep.isHandled) return rep;
				return routeWheel(node->child[0]);
			}
			return EventReply::unhandled();
		};
		return routeWheel(m_rootNode);
	}

	void SDockSpace::splitNode(DockNodePtr node, SplitDirection dir, float ratio, std::string title, WidgetPtr newContent, bool insertFirst) {
		if (!node || node->splitDirection != SplitDirection::None) return;

		node->splitDirection = dir;
		node->splitRatio = ratio;
		node->child[0] = std::make_shared<SDockNode>();
		node->child[1] = std::make_shared<SDockNode>();

		if (insertFirst) {
			node->child[0]->tabs.push_back({ title, newContent, Rect() });
			node->child[1]->tabs = std::move(node->tabs);
			node->child[1]->activeTab = node->activeTab;
		}
		else {
			node->child[0]->tabs = std::move(node->tabs);
			node->child[0]->activeTab = node->activeTab;
			node->child[1]->tabs.push_back({ title, newContent, Rect() });
		}
		node->tabs.clear();
	}

	void SDockSpace::updateDragDropPreview(const Vec2& mousePos, bool isDragging) {
		if (!isDragging) {
			m_previewNode = nullptr;
			m_previewZone = DockZone::None;
			return;
		}

		m_previewNode = hitTestContentNode(m_rootNode, mousePos);
		if (m_previewNode) {
			float localX = (mousePos.x - m_previewNode->allocatedGeometry.position.x) / m_previewNode->allocatedGeometry.size.x;
			float localY = (mousePos.y - m_previewNode->allocatedGeometry.position.y) / m_previewNode->allocatedGeometry.size.y;

			if (localX < 0.25f) m_previewZone = DockZone::Left;
			else if (localX > 0.75f) m_previewZone = DockZone::Right;
			else if (localY < 0.25f) m_previewZone = DockZone::Top;
			else if (localY > 0.75f) m_previewZone = DockZone::Bottom;
			else m_previewZone = DockZone::Center;
		}
	}

	bool SDockSpace::processDrop(std::string title, WidgetPtr draggedContent) {
		if (m_previewNode && m_previewZone != DockZone::None) {

			if (m_previewZone == DockZone::Center || (m_previewNode->tabs.empty() && m_previewNode->splitDirection == SplitDirection::None)) {
				m_previewNode->tabs.push_back({ title, draggedContent, Rect() });
				m_previewNode->activeTab = (int)m_previewNode->tabs.size() - 1;
				m_previewNode = nullptr;
				m_previewZone = DockZone::None;
				return true;
			}

			SplitDirection dir = (m_previewZone == DockZone::Left || m_previewZone == DockZone::Right) ? SplitDirection::Horizontal : SplitDirection::Vertical;
			bool insertFirst = (m_previewZone == DockZone::Left || m_previewZone == DockZone::Top);

			splitNode(m_previewNode, dir, 0.5f, title, draggedContent, insertFirst);

			m_previewNode = nullptr;
			m_previewZone = DockZone::None;
			return true;
		}
		return false;
	}

	void SDockSpace::arrangeNode(DockNodePtr node, const Geometry& geo) {
		if (!node) return;

		node->allocatedGeometry = geo;

		if (node->splitDirection == SplitDirection::None) {
			node->titleBarRect = Rect(geo.position.x, geo.position.x + geo.size.x, geo.position.y, geo.position.y + 20.0f);

			float tabWidth = 120.0f;
			for (size_t i = 0; i < node->tabs.size(); ++i) {
				node->tabs[i].hitRect = Rect(geo.position.x + i * tabWidth, geo.position.x + (i + 1) * tabWidth, geo.position.y, geo.position.y + 20.0f);
			}

			Geometry contentGeo;
			contentGeo.position = { geo.position.x, geo.position.y + 20.0f };
			contentGeo.size = { geo.size.x, geo.size.y - 20.0f };
			if (contentGeo.size.y < 0) contentGeo.size.y = 0;

			if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content) {
				node->tabs[node->activeTab].content->arrangeChildren(contentGeo);
			}
		}
		// -- Left Right --
		else if (node->splitDirection == SplitDirection::Horizontal) {
			float totalWidth = geo.size.x - m_splitterThickness;
			float leftWidth = totalWidth * node->splitRatio;
			float rightWidth = totalWidth - leftWidth;

			Geometry leftGeo = { geo.position, {leftWidth, geo.size.y} };
			Geometry rightGeo = { {geo.position.x + leftWidth + m_splitterThickness, geo.position.y}, {rightWidth, geo.size.y} };

			node->splitterRect = Rect(leftGeo.position.x + leftWidth, leftGeo.position.x + leftWidth + m_splitterThickness, geo.position.y, geo.position.y + geo.size.y);

			if (node->child[0]) arrangeNode(node->child[0], leftGeo);
			if (node->child[1]) arrangeNode(node->child[1], rightGeo);
		}
		// -- Top Bottom --
		else if (node->splitDirection == SplitDirection::Vertical) {
			float totalHeight = geo.size.y - m_splitterThickness;
			float topHeight = totalHeight * node->splitRatio;
			float bottomHeight = totalHeight - topHeight;

			Geometry topGeo = { geo.position, {geo.size.x, topHeight} };
			Geometry bottomGeo = { {geo.position.x, geo.position.y + topHeight + m_splitterThickness}, {geo.size.x, bottomHeight} };

			node->splitterRect = Rect(geo.position.x, geo.position.x + geo.size.x, topGeo.position.y + topHeight, topGeo.position.y + topHeight + m_splitterThickness);

			if (node->child[0]) arrangeNode(node->child[0], topGeo);
			if (node->child[1]) arrangeNode(node->child[1], bottomGeo);
		}
	}

	void SDockSpace::drawNode(const DockNodePtr& node, DrawList& drawList) const {
		if (!node) return;

		if (node->splitDirection == SplitDirection::None) {
			if (!node->tabs.empty()) {
				// -- Draw Entire Title Bar Background --
				Geometry tbGeo = { {node->titleBarRect.left, node->titleBarRect.top}, {node->titleBarRect.right - node->titleBarRect.left, node->titleBarRect.bottom - node->titleBarRect.top} };
				addRectToDrawList(drawList, tbGeo, GetTheme().backgroundDarkWorkspace);

				// -- Draw Each Tab --
				for (size_t i = 0; i < node->tabs.size(); ++i) {
					Geometry tGeo = { {node->tabs[i].hitRect.left, node->tabs[i].hitRect.top}, {node->tabs[i].hitRect.right - node->tabs[i].hitRect.left, node->tabs[i].hitRect.bottom - node->tabs[i].hitRect.top} };
					Color tColor = (i == node->activeTab) ? m_titleBarColor : GetTheme().backgroundDarkWorkspace;

					if (i != node->activeTab) {
						tColor = Color(
							std::max(0, (int)tColor.r() - 20),
							std::max(0, (int)tColor.g() - 20),
							std::max(0, (int)tColor.b() - 20),
							tColor.a()
						);
					}

					addRectToDrawList(drawList, tGeo, tColor);

					// -- Draw Tab Title --
					if (m_font && !node->tabs[i].title.empty()) {
						float cursorX = tGeo.position.x + 10.0f;
						float baselineY = tGeo.position.y + 15.0f;
						for (char c : node->tabs[i].title) {
							const Glyph& g = m_font->getGlyph(c);
							if (g.size.x > 0 && g.size.y > 0) {
								float x0 = cursorX + g.offset.x;
								float y0 = baselineY + g.offset.y;
								float x1 = x0 + g.size.x;
								float y1 = y0 + g.size.y;
								uint32_t startIndex = (uint32_t)drawList.vertices.size();
								drawList.vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, GetTheme().textMain });
								drawList.vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, GetTheme().textMain });
								drawList.vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, GetTheme().textMain });
								drawList.vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, GetTheme().textMain });
								drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 1); drawList.indices.push_back(startIndex + 2);
								drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 2); drawList.indices.push_back(startIndex + 3);
								if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
								drawList.commands.back().indexCount += 6;
							}
							cursorX += g.advanceX;
						}
					}
				}

				if (node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content) {
					node->tabs[node->activeTab].content->onDraw(drawList, node->tabs[node->activeTab].content->getAllocatedGeometry());
				}
			}
		}
		else {
			Geometry splitterGeo = { {node->splitterRect.left, node->splitterRect.top}, {node->splitterRect.right - node->splitterRect.left, node->splitterRect.bottom - node->splitterRect.top} };
			Color sColor = GetTheme().backgroundWindow;
			if (m_draggingNode == node) sColor = GetTheme().accentPrimary;
			else if (m_hoveredNode == node) sColor = Color(120, 120, 120, 255);
			addRectToDrawList(drawList, splitterGeo, sColor);

			// -- Recurse --
			if (node->child[0]) drawNode(node->child[0], drawList);
			if (node->child[1]) drawNode(node->child[1], drawList);
		}
	}

	void SDockSpace::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
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

	DockNodePtr SDockSpace::hitTestSplitter(const DockNodePtr& node, const Vec2& mousePos) {
		if (!node || node->splitDirection == SplitDirection::None) return nullptr;

		Rect hitRect = node->splitterRect;
		if (node->splitDirection == SplitDirection::Horizontal) {
			hitRect.left -= 6.0f;
			hitRect.right += 6.0f;
		}
		else {
			hitRect.top -= 6.0f;
			hitRect.bottom += 6.0f;
		}

		if (hitRect.contains(mousePos)) return node;

		auto leftHit = hitTestSplitter(node->child[0], mousePos);
		if (leftHit) return leftHit;

		return hitTestSplitter(node->child[1], mousePos);
	}

	DockNodePtr SDockSpace::hitTestContentNode(const DockNodePtr& node, const Vec2& mousePos) {
		if (!node || !node->allocatedGeometry.contains(mousePos)) return nullptr;

		if (node->splitDirection == SplitDirection::None) return node;

		auto leftHit = hitTestContentNode(node->child[0], mousePos);
		if (leftHit) return leftHit;

		return hitTestContentNode(node->child[1], mousePos);
	}

	std::pair<DockNodePtr, int> SDockSpace::hitTestTab(const DockNodePtr& node, const Vec2& mousePos) {
		if (!node) return { nullptr, -1 };
		if (node->splitDirection == SplitDirection::None) {
			for (size_t i = 0; i < node->tabs.size(); ++i) {
				if (node->tabs[i].hitRect.contains(mousePos)) return { node, (int)i };
			}
			return { nullptr, -1 };
		}
		auto leftHit = hitTestTab(node->child[0], mousePos);
		if (leftHit.first) return leftHit;
		return hitTestTab(node->child[1], mousePos);
	}

	bool SDockSpace::removeLeafNode(DockNodePtr parent, DockNodePtr target) {
		if (!parent || parent->splitDirection == SplitDirection::None) return false;

		if (parent->child[0] == target) {
			DockNodePtr survivor = parent->child[1];
			*parent = *survivor;
			return true;
		}
		if (parent->child[1] == target) {
			DockNodePtr survivor = parent->child[0];
			*parent = *survivor;
			return true;
		}

		if (removeLeafNode(parent->child[0], target)) return true;
		if (removeLeafNode(parent->child[1], target)) return true;
		return false;
	}

	void SDockSpace::undockNode(DockNodePtr node, int tabIndex, const Vec2& mousePos) {
		if (!node || tabIndex < 0 || tabIndex >= node->tabs.size()) return;

		DockTab savedTab = node->tabs[tabIndex];
		node->tabs.erase(node->tabs.begin() + tabIndex);
		if (node->activeTab >= node->tabs.size()) node->activeTab = std::max(0, (int)node->tabs.size() - 1);

		if (node->tabs.empty()) {
			if (node == m_rootNode) { /* Empty root, do nothing */ }
			else { removeLeafNode(m_rootNode, node); }
		}

		if (m_onUndockWindow) m_onUndockWindow(savedTab.title, savedTab.content, mousePos);
	}

}
