#include "DragDrop.h"

namespace Silica {

	DragDropPayload DragDrop::s_payload;
	bool DragDrop::s_isDragging = false;

	void DragDrop::beginDrag(const std::string& type, std::any data, const std::string& tooltip, FontAtlas* font) {
		s_payload = { type, data, tooltip, font };
		s_isDragging = true;
	}

	void DragDrop::endDrag() {
		s_payload = {};
		s_isDragging = false;
	}

	bool DragDrop::isDragging() {
		return s_isDragging;
	}

	bool DragDrop::isDraggingType(const std::string& type) {
		return s_isDragging && (s_payload.type == type);
	}

	const DragDropPayload& DragDrop::getPayload() {
		return s_payload;
	}

}
