#pragma once

#include <ostream>
#include <stack>
#include <stdint.h>
#include <matrix3.h>

class ChunkWriter
{
public:
	ChunkWriter(std::filebuf *filebuf);

	void beginChunk(uint32_t id);
	void endChunk();
	void beginMiniChunk(byte id);
	void endMiniChunk();

	bool isEmpty() const;
	size_t tellp();

	template<typename T>
	void write(const T &value)
	{
		write(value, sizeof(value));
	}

	template<typename T>
	void write(const T &value, const size_t &size)
	{
		m_oStream.write(reinterpret_cast<const char*>(&value), size);
	}

	void writeName(const std::string &name);
	void writeName(const wchar_t* str);
	void writeMatrix(const Matrix3 &matrix);

	template<typename T>
	void writeValues(T value, int count)
	{
		for (int i = 0; i < count; ++i)
		{
			write(value);
		}
	}

	void fillBytes(int count);

	template<typename T>
	void writeAt(const size_t& position, const T& value)
	{
		writeAt(position, value, sizeof(value));
	}

	template<typename T>
	void writeAt(const size_t& position, const T& value, const size_t& size)
	{
		auto curPos = m_oStream.tellp();
		m_oStream.seekp(position);
		write(value, size);
		m_oStream.seekp(curPos);
	}

protected:
	std::ostream m_oStream;
	std::stack<std::pair<size_t, uint32_t>> m_chunks;
};
