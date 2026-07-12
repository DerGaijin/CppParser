#pragma once

#include "Definitions.h"
#include "String.h"
#include "Array.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>


namespace CE
{
	using int64 = int64_t;
	using int32 = int32_t;
	using uint32 = uint32_t;
	using uint8 = uint8_t;

	template<typename T>
	using SharedPtr = std::shared_ptr<T>;

	template<typename K, typename V>
	using Map = std::map<K, V>;

	template<typename K, typename V>
	using MultiMap = std::multimap<K, V>;
}
