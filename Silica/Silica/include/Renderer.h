#pragma once

#include <stdint.h>
#include <vector>

#include "SilicaMath.h"

namespace Silica {

	struct Vertex {
		Vec2 position;
		Vec2 uv;
		uint32_t color;
	};

	struct DrawCommand {
		uint32_t indexCount;
		uint32_t startIndex;
		int32_t vertexOffset;
	};

	struct DrawList {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<DrawCommand> commands;
	};

}
