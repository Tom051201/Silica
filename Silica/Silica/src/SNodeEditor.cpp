#include "silicapch.h"
#include "SNodeEditor.h"

#include "Renderer.h"

namespace Silica {

	void SNodeEditor::construct(const Args& args) {
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_onBackgroundContextClick = args.onBackgroundContextClick;
		m_onNodeContextClick = args.onNodeContextClick;
	}

	void SNodeEditor::computeDesiredSize() {
		m_desiredSize = Vec2(400, 400);
	}

	void SNodeEditor::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float effectiveScale = m_renderScale * m_zoom;

		if (m_activeContextMenu) {
			m_activeContextMenu->computeDesiredSize();
			Vec2 desired = m_activeContextMenu->getDesiredSize();
			m_contextMenuGeometry.size = { desired.x * m_renderScale, desired.y * m_renderScale };

			m_activeContextMenu->arrangeChildren(m_contextMenuGeometry);

			Renderer::pushPopup(m_activeContextMenu, m_contextMenuGeometry, [this]() {
				m_activeContextMenu = nullptr;
			});
		}

		// -- Layout Embedded Pin Widgets --
		for (auto& node : m_nodes) {
			float maxInTextWidth = 0.0f;
			float maxWidgetWidth = 0.0f;
			float maxOutTextWidth = 0.0f;

			for (auto& pin : node.inputs) {
				float textW = getTextWidth(pin.name) / effectiveScale;
				maxInTextWidth = std::max(maxInTextWidth, textW);

				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					pin.inlineWidget->computeDesiredSize();
					float widgetW = pin.inlineWidget->getDesiredSize().x;
					maxWidgetWidth = std::max(maxWidgetWidth, widgetW);
				}
			}

			for (auto& pin : node.outputs) {
				float textW = getTextWidth(pin.name) / effectiveScale;
				maxOutTextWidth = std::max(maxOutTextWidth, textW);
			}

			float titleWidth = getTextWidth(node.title) / effectiveScale;

			float contentWidth = 12.0f + maxInTextWidth;
			if (maxWidgetWidth > 0.0f) {
				contentWidth += 10.0f + maxWidgetWidth;
			}
			contentWidth += 40.0f + maxOutTextWidth + 12.0f;
			node.size.x = std::max({ 140.0f, titleWidth + 30.0f, contentWidth });

			Vec2 screenPos = {
				allocatedGeometry.position.x + m_panOffset.x + (node.position.x * effectiveScale),
				allocatedGeometry.position.y + m_panOffset.y + (node.position.y * effectiveScale)
			};

			float pinY = screenPos.y + (40.0f * effectiveScale);
			float widgetStartX = 12.0f + maxInTextWidth + 10.0f;

			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					Vec2 desiredSize = pin.inlineWidget->getDesiredSize();
					pin.inlineWidget->setRenderScale(effectiveScale);

					Vec2 scaledSize = { desiredSize.x * effectiveScale, desiredSize.y * effectiveScale };

					Geometry widgetGeo;
					widgetGeo.position = { screenPos.x + (widgetStartX * effectiveScale), pinY - (scaledSize.y * 0.5f) };
					widgetGeo.size = scaledSize;
					pin.inlineWidget->arrangeChildren(widgetGeo);
				}
				pinY += (30.0f * effectiveScale);
			}
		}
	}

	void SNodeEditor::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		float effectiveScale = m_renderScale * m_zoom;

		// -- Viewport Bounds for Culling --
		float viewLeft = allocatedGeometry.position.x;
		float viewRight = viewLeft + allocatedGeometry.size.x;
		float viewTop = allocatedGeometry.position.y;
		float viewBottom = viewTop + allocatedGeometry.size.y;

		// -- Draw Background And Grid --
		outDrawList.addRect(allocatedGeometry, Silica::GetTheme().NodeEditor_Background);

		outDrawList.pushClipRect(Rect(
			allocatedGeometry.position.x, allocatedGeometry.position.x + allocatedGeometry.size.x,
			allocatedGeometry.position.y, allocatedGeometry.position.y + allocatedGeometry.size.y
		));

		float gridSize = 50.0f * effectiveScale;
		float offsetX = fmod(m_panOffset.x, gridSize);
		float offsetY = fmod(m_panOffset.y, gridSize);

		for (float x = offsetX; x < allocatedGeometry.size.x; x += gridSize) {
			float snapX = std::round(allocatedGeometry.position.x + x);
			outDrawList.addThickLine({ snapX, allocatedGeometry.position.y }, { snapX, allocatedGeometry.position.y + allocatedGeometry.size.y }, 1.0f, Silica::GetTheme().NodeEditor_GridLine);
		}
		for (float y = offsetY; y < allocatedGeometry.size.y; y += gridSize) {
			float snapY = std::round(allocatedGeometry.position.y + y);
			outDrawList.addThickLine({ allocatedGeometry.position.x, snapY }, { allocatedGeometry.position.x + allocatedGeometry.size.x, snapY }, 1.0f, Silica::GetTheme().NodeEditor_GridLine);
		}

		// -- Pre-Compute Pin Screen Positions --
		for (auto& node : const_cast<SNodeEditor*>(this)->m_nodes) {
			float maxPins = (float)std::max(node.inputs.size(), node.outputs.size());
			node.size.y = 40.0f + (maxPins * 30.0f) + 10.0f;

			Vec2 screenPos = {
				allocatedGeometry.position.x + m_panOffset.x + (node.position.x * effectiveScale),
				allocatedGeometry.position.y + m_panOffset.y + (node.position.y * effectiveScale)
			};

			float pinY = screenPos.y + (40.0f * effectiveScale);
			for (auto& pin : node.inputs) {
				pin.screenPosition = { screenPos.x, pinY };
				pinY += (30.0f * effectiveScale);
			}

			pinY = screenPos.y + (40.0f * effectiveScale);
			for (auto& pin : node.outputs) {
				pin.screenPosition = { screenPos.x + (node.size.x * effectiveScale), pinY };
				pinY += (30.0f * effectiveScale);
			}
		}

		// -- Draw Established Links --
		for (const auto& link : m_links) {
			NodePin* p1 = const_cast<SNodeEditor*>(this)->findPin(link.startPin);
			NodePin* p2 = const_cast<SNodeEditor*>(this)->findPin(link.endPin);
			if (p1 && p2) {
				Vec2 cp1 = { p1->screenPosition.x + (50.0f * effectiveScale), p1->screenPosition.y };
				Vec2 cp2 = { p2->screenPosition.x - (50.0f * effectiveScale), p2->screenPosition.y };

				float targetThickness = (link.id == m_selectedLinkID ? 5.0f : 3.0f) * effectiveScale;
				float renderThickness = std::max(2.0f, targetThickness);

//				Color wireColor = (link.id == m_selectedLinkID) ? Color(255, 165, 0, 255) : link.color;
				Color wireColor = (link.id == m_selectedLinkID) ? Silica::GetTheme().NodeEditor_Selected : link.color;
				outDrawList.addBezierCurve(p1->screenPosition, cp1, cp2, p2->screenPosition, renderThickness, wireColor);
			}
		}

		// -- Draw Nodes --
		for (auto& node : const_cast<SNodeEditor*>(this)->m_nodes) {
			Vec2 screenPos = {
				allocatedGeometry.position.x + m_panOffset.x + (node.position.x * effectiveScale),
				allocatedGeometry.position.y + m_panOffset.y + (node.position.y * effectiveScale)
			};

			// -- Cull Off-Screen Nodes --
			float nodeRight = screenPos.x + (node.size.x * effectiveScale);
			float nodeBottom = screenPos.y + (node.size.y * effectiveScale);
			if (nodeRight < viewLeft || screenPos.x > viewRight || nodeBottom < viewTop || screenPos.y > viewBottom) {
				continue;
			}

			// -- Node Body And Header Backgrounds --
			Geometry nodeGeo = { screenPos, {node.size.x * effectiveScale, node.size.y * effectiveScale} };
			Geometry headerGeo = { screenPos, {node.size.x * effectiveScale, 24.0f * effectiveScale} };

			if (node.id == m_selectedNodeID) {
				float outline = std::max(2.0f, 3.0f * effectiveScale);

				Geometry outlineGeo = { {screenPos.x - outline, screenPos.y - outline}, {nodeGeo.size.x + (outline * 2.0f), nodeGeo.size.y + (outline * 2.0f)} };
				outDrawList.addRect(outlineGeo, Silica::GetTheme().NodeEditor_Selected);
			}

			outDrawList.addRect(nodeGeo, Silica::GetTheme().NodeEditor_NodeBody);
			outDrawList.addRect(headerGeo, node.headerColor);

			// -- Title --
			outDrawList.addText(m_font, node.title, { screenPos.x + (8.0f * effectiveScale), screenPos.y + (16.0f * effectiveScale) }, Silica::GetTheme().Text_Main , effectiveScale);

			// -- Draw Inputs --
			float pinRadius = 4.0f * effectiveScale;
			float pinSize = 8.0f * effectiveScale;
			for (auto& pin : node.inputs) {
				outDrawList.addRect({ {pin.screenPosition.x - pinRadius, pin.screenPosition.y - pinRadius}, {pinSize, pinSize} }, pin.color);
				outDrawList.addText(m_font, pin.name, { pin.screenPosition.x + (12.0f * effectiveScale), pin.screenPosition.y + pinRadius }, GetTheme().Text_Main, effectiveScale);

				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					// -- Cull Off-Screen Inline Widgets --
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					if (widgetGeo.position.x + widgetGeo.size.x > viewLeft && widgetGeo.position.x < viewRight &&
						widgetGeo.position.y + widgetGeo.size.y > viewTop && widgetGeo.position.y < viewBottom) {

						pin.inlineWidget->onDraw(outDrawList, widgetGeo);
					}
				}
			}

			// -- Draw Outputs --
			for (auto& pin : node.outputs) {
				outDrawList.addRect({ {pin.screenPosition.x - pinRadius, pin.screenPosition.y - pinRadius}, {pinSize, pinSize} }, pin.color);
				float textWidth = getTextWidth(pin.name);
				outDrawList.addText(m_font, pin.name, { pin.screenPosition.x - textWidth - (12.0f * effectiveScale), pin.screenPosition.y + pinRadius }, GetTheme().Text_Main, effectiveScale);
			}
		}

		// -- Draw Currently Dragging Wire --
		if (m_draggingPinID != -1) {
			NodePin* startPin = const_cast<SNodeEditor*>(this)->findPin(m_draggingPinID);
			if (startPin) {
				Vec2 cp1 = { startPin->screenPosition.x + (startPin->type == PinType::Output ? 50.0f * effectiveScale : -50.0f * effectiveScale), startPin->screenPosition.y };
				Vec2 cp2 = { m_dragWireEndPos.x + (startPin->type == PinType::Output ? -50.0f * effectiveScale : 50.0f * effectiveScale), m_dragWireEndPos.y };

				float targetThickness = 5.0f * effectiveScale;
				float renderThickness = std::max(2.0f, targetThickness);

				Color wireColor = Silica::GetTheme().NodeEditor_Selected;
				if (targetThickness < 2.0f) {
					float alphaScale = targetThickness / 2.0f;
					wireColor = Color(wireColor.r(), wireColor.g(), wireColor.b(), static_cast<uint8_t>(wireColor.a() * alphaScale));
				}

				outDrawList.addBezierCurve(startPin->screenPosition, cp1, cp2, m_dragWireEndPos, renderThickness, wireColor);
			}
		}

		outDrawList.popClipRect();
	}

	EventReply SNodeEditor::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (!allocatedGeometry.contains(mousePos)) return EventReply::unhandled();
		float effectiveScale = m_renderScale * m_zoom;

		// -- Forward To Inline Widgets First --
		for (auto& node : m_nodes) {
			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					if (widgetGeo.contains(mousePos)) {
						EventReply reply = pin.inlineWidget->onMouseButtonDown(widgetGeo, mousePos, button);
						if (reply.isHandled) return reply;
					}
				}
			}
		}

		// -- Panning --
		if (button == MouseButton::Right) {
			SWidget::setCapturedWidget(this);
			m_lastMousePos = mousePos;
			m_rightClickStartPos = mousePos;
			m_isPanning = true;
			return EventReply::handled();
		}

		if (button == MouseButton::Left) {
			SWidget::setFocusedWidget(this);

			// -- Check Pin Click --
			PinID hitPin = hitTestPins(mousePos);
			if (hitPin != -1) {
				NodePin* clickedPin = findPin(hitPin);

				if (clickedPin) {
					if (clickedPin->type == PinType::Input) {
						auto it = std::find_if(m_links.begin(), m_links.end(), [hitPin](const GraphLink& l) {
							return l.endPin == hitPin;
							});

						if (it != m_links.end()) {
							m_draggingPinID = it->startPin;
							m_dragWireEndPos = mousePos;
							m_links.erase(it);

							return EventReply::handled();
						}
					}

					m_draggingPinID = hitPin;
					m_dragWireEndPos = mousePos;
				}

				return EventReply::handled();
			}

			// -- Check Node Header --
			NodeID hitNode = hitTestNodes(mousePos);
			if (hitNode != -1) {
				m_selectedNodeID = hitNode;
				m_selectedLinkID = -1;
				m_draggingNodeID = hitNode;

				GraphNode* node = findNode(hitNode);
				Vec2 screenPos = {
					allocatedGeometry.position.x + m_panOffset.x + (node->position.x * effectiveScale),
					allocatedGeometry.position.y + m_panOffset.y + (node->position.y * effectiveScale)
				};
				m_nodeDragOffset = { mousePos.x - screenPos.x, mousePos.y - screenPos.y };

				for (size_t i = 0; i < m_nodes.size(); ++i) {
					if (m_nodes[i].id == hitNode) {
						GraphNode temp = m_nodes[i];
						m_nodes.erase(m_nodes.begin() + i);
						m_nodes.push_back(temp);
						break;
					}
				}
				return EventReply::handled();
			}

			// -- Check Wires --
			LinkID hitLink = hitTestLinks(mousePos);
			if (hitLink != -1) {
				m_selectedLinkID = hitLink;
				m_selectedNodeID = -1;
				return EventReply::handled();
			}

			// -- Clicked Empty Space --
			m_selectedNodeID = -1;
			m_selectedLinkID = -1;
			return EventReply::handled();

		}

		return EventReply::unhandled();
	}

	EventReply SNodeEditor::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		bool handled = false;
		float effectiveScale = m_renderScale * m_zoom;

		// -- Forward To Inline Widgets First --
		for (auto& node : m_nodes) {
			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					EventReply reply = pin.inlineWidget->onMouseMove(widgetGeo, mousePos);
					if (reply.isHandled) {
						handled = true;
					}
				}
			}
		}

		if (handled) return EventReply::handled();

		// -- Handle Panning --
		if (m_isPanning) {
			m_panOffset.x += (mousePos.x - m_lastMousePos.x);
			m_panOffset.y += (mousePos.y - m_lastMousePos.y);
			handled = true;
		}

		// -- Handle Node Dragging --
		if (m_draggingNodeID != -1) {
			GraphNode* node = findNode(m_draggingNodeID);
			if (node) {
				node->position.x = (mousePos.x - allocatedGeometry.position.x - m_panOffset.x - m_nodeDragOffset.x) / effectiveScale;
				node->position.y = (mousePos.y - allocatedGeometry.position.y - m_panOffset.y - m_nodeDragOffset.y) / effectiveScale;
			}
			handled = true;
		}

		// -- Handle Wire Dragging --
		if (m_draggingPinID != -1) {
			m_dragWireEndPos = mousePos;
			handled = true;
		}

		// -- Update Last Mouse Position --
		if (handled) {
			m_lastMousePos = mousePos;
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SNodeEditor::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		bool widgetHandled = false;

		// -- Forward To Inline Widgets First --
		for (auto& node : m_nodes) {
			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					EventReply reply = pin.inlineWidget->onMouseButtonUp(widgetGeo, mousePos, button);
					if (reply.isHandled) {
						widgetHandled = true;
					}
				}
			}
		}

		if (widgetHandled) return EventReply::handled();

		if (button == MouseButton::Left) {
			m_draggingNodeID = -1;

			if (m_draggingPinID != -1) {
				PinID droppedPin = hitTestPins(mousePos);
				if (droppedPin != -1 && droppedPin != m_draggingPinID) {
					GraphNode* node1 = nullptr;
					GraphNode* node2 = nullptr;
					NodePin* p1 = findPin(m_draggingPinID, &node1);
					NodePin* p2 = findPin(droppedPin, &node2);

					if (p1 && p2 && p1->type != p2->type && node1 != node2) {
						PinID outPin = (p1->type == PinType::Output) ? p1->id : p2->id;
						PinID inPin = (p1->type == PinType::Input) ? p1->id : p2->id;
						addLink((LinkID)m_links.size() + 1, outPin, inPin, p1->color);
					}
				}
				m_draggingPinID = -1;
			}
		}

		if (button == MouseButton::Right) {
			m_isPanning = false;
			float distSq = (mousePos.x - m_rightClickStartPos.x) * (mousePos.x - m_rightClickStartPos.x) + (mousePos.y - m_rightClickStartPos.y) * (mousePos.y - m_rightClickStartPos.y);

			if (distSq < 9.0f) {
				NodeID hitNode = hitTestNodes(mousePos);

				if (hitNode != -1 && m_onNodeContextClick) {
					m_activeContextMenu = m_onNodeContextClick(hitNode, mousePos);
					if (m_activeContextMenu) {
						m_activeContextMenu->setRenderScale(m_renderScale);
						m_activeContextMenu->computeDesiredSize();

						Vec2 desired = m_activeContextMenu->getDesiredSize();
						m_contextMenuGeometry = { mousePos, {desired.x * m_renderScale, desired.y * m_renderScale} };
					}
				}
				else if (hitNode == -1 && m_onBackgroundContextClick) {
					m_activeContextMenu = m_onBackgroundContextClick(mousePos);
					if (m_activeContextMenu) {
						m_activeContextMenu->setRenderScale(m_renderScale);
						m_activeContextMenu->computeDesiredSize();

						Vec2 desired = m_activeContextMenu->getDesiredSize();
						m_contextMenuGeometry = { mousePos, {desired.x * m_renderScale, desired.y * m_renderScale} };
					}
				}
			}

			if (m_draggingPinID == -1 && m_draggingNodeID == -1) {
				SWidget::setCapturedWidget(nullptr);
			}

			return EventReply::handled();
		}

		if (!m_isPanning && m_draggingNodeID == -1 && m_draggingPinID == -1) {
			SWidget::setCapturedWidget(nullptr);
		}

		return EventReply::handled();
	}

	EventReply SNodeEditor::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (!allocatedGeometry.contains(mousePos)) return EventReply::unhandled();

		// -- Forward To Inline Widgets First --
		for (auto& node : m_nodes) {
			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					if (widgetGeo.contains(mousePos)) {
						EventReply reply = pin.inlineWidget->onMouseWheel(widgetGeo, mousePos, scrollDelta);
						if (reply.isHandled) return reply;
					}
				}
			}
		}

		float zoomSpeed = 0.1f;
		float oldZoom = m_zoom;

		m_zoom = std::clamp(m_zoom + (scrollDelta * zoomSpeed), 0.3f, 2.0f);

		float oldEffectiveScale = m_renderScale * oldZoom;
		float newEffectiveScale = m_renderScale * m_zoom;

		Vec2 mouseLocal = { mousePos.x - allocatedGeometry.position.x, mousePos.y - allocatedGeometry.position.y };
		Vec2 canvasPos = { (mouseLocal.x - m_panOffset.x) / oldEffectiveScale, (mouseLocal.y - m_panOffset.y) / oldEffectiveScale };

		m_panOffset.x = mouseLocal.x - (canvasPos.x * newEffectiveScale);
		m_panOffset.y = mouseLocal.y - (canvasPos.y * newEffectiveScale);

		return EventReply::handled();
	}

	EventReply SNodeEditor::onKeyDown(Key key) {
		if (key == Key::Delete || key == Key::Backspace) {

			// -- Delete Selected Link --
			if (m_selectedLinkID != -1) {
				m_links.erase(std::remove_if(m_links.begin(), m_links.end(), [this](const GraphLink& l) {
					return l.id == m_selectedLinkID;
					}), m_links.end());

				m_selectedLinkID = -1;
				return EventReply::handled();
			}

			// -- Delete Selected Node --
			if (m_selectedNodeID != -1) {
				m_links.erase(std::remove_if(m_links.begin(), m_links.end(), [this](const GraphLink& l) {
					GraphNode* n1 = nullptr; findPin(l.startPin, &n1);
					GraphNode* n2 = nullptr; findPin(l.endPin, &n2);
					return (n1 && n1->id == m_selectedNodeID) || (n2 && n2->id == m_selectedNodeID);
					}), m_links.end());

				m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), [this](const GraphNode& n) {
					return n.id == m_selectedNodeID;
					}), m_nodes.end());

				m_selectedNodeID = -1;
				return EventReply::handled();
			}
		}

		return EventReply::unhandled();
	}

	void SNodeEditor::addNode(const GraphNode& node) {
		m_nodes.push_back(node);
	}

	void SNodeEditor::addLink(LinkID id, PinID start, PinID end, Color color) {
		m_links.push_back({ id, start, end, color });
	}

	GraphNode* SNodeEditor::findNode(NodeID id) {
		for (auto& n : m_nodes) {
			if (n.id == id) return &n;
		}

		return nullptr;
	}

	NodePin* SNodeEditor::findPin(PinID id, GraphNode** outNode) {
		for (auto& n : m_nodes) {
			for (auto& p : n.inputs) {
				if (p.id == id) {
					if (outNode) *outNode = &n;
					return &p;
				}
			}
			for (auto& p : n.outputs) {
				if (p.id == id) {
					if (outNode) *outNode = &n;
					return &p;
				}
			}
		}

		return nullptr;
	}

	PinID SNodeEditor::hitTestPins(const Vec2& mousePos) {
		float effectiveScale = m_renderScale * m_zoom;
		float hitRadius = std::max(4.0f, 10.0f * effectiveScale);

		for (auto& n : m_nodes) {
			for (auto& p : n.inputs) {
				if (std::abs(mousePos.x - p.screenPosition.x) < hitRadius &&
					std::abs(mousePos.y - p.screenPosition.y) < hitRadius) {
					return p.id;
				}
			}
			for (auto& p : n.outputs) {
				if (std::abs(mousePos.x - p.screenPosition.x) < hitRadius &&
					std::abs(mousePos.y - p.screenPosition.y) < hitRadius) {
					return p.id;
				}
			}
		}

		return -1;
	}

	NodeID SNodeEditor::hitTestNodes(const Vec2& mousePos) {
		float effectiveScale = m_renderScale * m_zoom;

		for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
			Vec2 screenPos = {
				m_allocatedGeometry.position.x + m_panOffset.x + (it->position.x * effectiveScale),
				m_allocatedGeometry.position.y + m_panOffset.y + (it->position.y * effectiveScale)
			};

			Rect nodeRect(
				screenPos.x,
				screenPos.x + (it->size.x * effectiveScale),
				screenPos.y,
				screenPos.y + (it->size.y * effectiveScale)
			);

			if (nodeRect.contains(mousePos)) return it->id;
		}

		return -1;
	}

	LinkID SNodeEditor::hitTestLinks(const Vec2& mousePos) {
		float effectiveScale = m_renderScale * m_zoom;
		float hitRadiusSq = (5.0f * effectiveScale) * (5.0f * effectiveScale);

		for (const auto& link : m_links) {
			NodePin* p1 = findPin(link.startPin);
			NodePin* p2 = findPin(link.endPin);
			if (!p1 || !p2) continue;

			Vec2 cp1 = { p1->screenPosition.x + (50.0f * effectiveScale), p1->screenPosition.y };
			Vec2 cp2 = { p2->screenPosition.x - (50.0f * effectiveScale), p2->screenPosition.y };

			int segments = 20;
			Vec2 lastPoint = p1->screenPosition;

			for (int i = 1; i <= segments; ++i) {
				float t = (float)i / (float)segments;
				float u = 1.0f - t;
				float tt = t * t, uu = u * u, uuu = uu * u, ttt = tt * t;

				Vec2 currentPoint = {
					uuu * p1->screenPosition.x + 3 * uu * t * cp1.x + 3 * u * tt * cp2.x + ttt * p2->screenPosition.x,
					uuu * p1->screenPosition.y + 3 * uu * t * cp1.y + 3 * u * tt * cp2.y + ttt * p2->screenPosition.y
				};

				float l2 = (currentPoint.x - lastPoint.x) * (currentPoint.x - lastPoint.x) + (currentPoint.y - lastPoint.y) * (currentPoint.y - lastPoint.y);
				if (l2 > 0.0f) {
					float t_proj = std::clamp(((mousePos.x - lastPoint.x) * (currentPoint.x - lastPoint.x) + (mousePos.y - lastPoint.y) * (currentPoint.y - lastPoint.y)) / l2, 0.0f, 1.0f);
					Vec2 proj = { lastPoint.x + t_proj * (currentPoint.x - lastPoint.x), lastPoint.y + t_proj * (currentPoint.y - lastPoint.y) };
					float distSq = (mousePos.x - proj.x) * (mousePos.x - proj.x) + (mousePos.y - proj.y) * (mousePos.y - proj.y);

					if (distSq < hitRadiusSq) return link.id;
				}
				lastPoint = currentPoint;
			}
		}

		return -1;
	}

	const std::vector<GraphNode>& SNodeEditor::getNodes() const {
		return m_nodes;
	}

	const std::vector<GraphLink>& SNodeEditor::getLinks() const {
		return m_links;
	}

	Vec2 SNodeEditor::screenToCanvas(const Vec2& screenPos) const {
		float effectiveScale = m_renderScale * m_zoom;
		Vec2 localPos = { screenPos.x - m_allocatedGeometry.position.x, screenPos.y - m_allocatedGeometry.position.y };
		return { (localPos.x - m_panOffset.x) / effectiveScale, (localPos.y - m_panOffset.y) / effectiveScale };
	}

	bool SNodeEditor::isPinConnected(PinID pinId) const {
		for (const auto& link : m_links) {
			if (link.startPin == pinId || link.endPin == pinId) return true;
		}
		return false;
	}

	void SNodeEditor::clear() {
		m_nodes.clear(); m_links.clear();
		m_selectedNodeID = -1; m_selectedLinkID = -1;
		m_draggingNodeID = -1; m_draggingPinID = -1;
	}

	void SNodeEditor::saveGraph(const std::filesystem::path& filepath) {
		std::ofstream out(filepath);
		if (!out.is_open()) return;

		out << "[SNodeEditor_v1]\n";

		// -- Save Camera State --
		out << "Pan " << m_panOffset.x << " " << m_panOffset.y << "\n";
		out << "Zoom " << m_zoom << "\n";

		// -- Save Nodes --
		for (const auto& node : m_nodes) {
			out << "[Node]\n";
			out << "ID " << node.id << "\n";
			out << "Title " << node.title << "\n";
			out << "Pos " << node.position.x << " " << node.position.y << "\n";
			out << "Size " << node.size.x << " " << node.size.y << "\n";
			out << "Color " << (int)node.headerColor.r() << " " << (int)node.headerColor.g() << " " << (int)node.headerColor.b() << " " << (int)node.headerColor.a() << "\n";

			// -- Save Pins --
			for (const auto& pin : node.inputs) {
				out << "In " << pin.id << " "
					<< (int)pin.color.r() << " " << (int)pin.color.g() << " "
					<< (int)pin.color.b() << " " << (int)pin.color.a() << " " << pin.name << "\n";
			}
			for (const auto& pin : node.outputs) {
				out << "Out " << pin.id << " "
					<< (int)pin.color.r() << " " << (int)pin.color.g() << " "
					<< (int)pin.color.b() << " " << (int)pin.color.a() << " " << pin.name << "\n";
			}
		}

		// -- Save Links --
		for (const auto& link : m_links) {
			out << "[Link]\n";
			out << "ID " << link.id << "\n";
			out << "Start " << link.startPin << "\n";
			out << "End " << link.endPin << "\n";
			out << "Color " << (int)link.color.r() << " " << (int)link.color.g() << " " << (int)link.color.b() << " " << (int)link.color.a() << "\n";
		}

		out.close();
	}

	void SNodeEditor::loadGraph(const std::filesystem::path& filepath) {
		std::ifstream in(filepath);
		if (!in.is_open()) return;

		std::string line;
		std::getline(in, line);
		if (line != "[SNodeEditor_v1]") return;

		GraphNode* currentNode = nullptr;

		while (std::getline(in, line)) {
			if (line.empty()) continue;

			std::istringstream iss(line);
			std::string token;
			iss >> token;

			if (token == "Pan") {
				iss >> m_panOffset.x >> m_panOffset.y;
			}
			else if (token == "Zoom") {
				iss >> m_zoom;
			}
			else if (token == "[Node]") {
				currentNode = nullptr;
			}
			else if (token == "ID") {
				NodeID id;
				iss >> id;
				currentNode = findNode(id);
			}
			else if (currentNode) {
				if (token == "Pos") {
					iss >> currentNode->position.x >> currentNode->position.y;
				}
			}
		}
	}

	float SNodeEditor::getTextWidth(const std::string& text) const {
		if (!m_font) return 0.0f;
		float effectiveScale = m_renderScale * m_zoom;
		float width = 0.0f;
		for (char c : text) width += m_font->getGlyph(c).advanceX;
		return width * effectiveScale;
	}

	EventReply SNodeEditor::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (!allocatedGeometry.contains(mousePos)) return EventReply::unhandled();

		// -- Forward To Inline Widgets First --
		for (auto& node : m_nodes) {
			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					if (widgetGeo.contains(mousePos)) {
						EventReply reply = pin.inlineWidget->onDragOver(widgetGeo, mousePos, payload);
						if (reply.isHandled) return reply;
					}
				}
			}
		}

		return EventReply::unhandled();
	}

	EventReply SNodeEditor::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (!allocatedGeometry.contains(mousePos)) return EventReply::unhandled();

		// -- Forward To Inline Widgets First --
		for (auto& node : m_nodes) {
			for (auto& pin : node.inputs) {
				if (pin.inlineWidget && !isPinConnected(pin.id)) {
					const Geometry& widgetGeo = pin.inlineWidget->getAllocatedGeometry();
					if (widgetGeo.contains(mousePos)) {
						EventReply reply = pin.inlineWidget->onDrop(widgetGeo, mousePos, payload);
						if (reply.isHandled) return reply;
					}
				}
			}
		}

		return EventReply::unhandled();
	}

}
