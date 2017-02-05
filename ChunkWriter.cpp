#include "ChunkWriter.hpp"

#include <fstream>

ChunkWriter::ChunkWriter(std::filebuf *filebuf)
	:m_oStream(filebuf)
{

}

void ChunkWriter::beginChunk(uint32_t id)
{
	write(id);
	if(m_chunks.size() > 0)
	{
		auto &parent = m_chunks.top();
		parent.second = 1<<31;	
	}
	m_chunks.push(std::make_pair(m_oStream.tellp(), 0));
	write(uint32_t());
}

void ChunkWriter::endChunk()
{
	auto chunk = m_chunks.top();
	writeAt(chunk.first, static_cast<uint32_t>(static_cast<size_t>(m_oStream.tellp())-chunk.first-4)+chunk.second);
	m_chunks.pop();
}

void ChunkWriter::beginMiniChunk(byte id)
{
	write(id);
	m_chunks.push(std::make_pair(m_oStream.tellp(),0));
	write(byte(0));
}

void ChunkWriter::endMiniChunk()
{
	auto miniChunk = m_chunks.top();
	writeAt(miniChunk.first, static_cast<byte>(static_cast<size_t>(m_oStream.tellp())-miniChunk.first-1));
	m_chunks.pop();
}

bool ChunkWriter::isEmpty() const
{
	return m_chunks.empty();
}

size_t ChunkWriter::tellp()
{
	return m_oStream.tellp();
}

void ChunkWriter::writeName(const std::string& name)
{
	write(name, name.length());
	write(short(0),1);
}

void ChunkWriter::writeName(const wchar_t* str)
{
	//Converting the name from wchar_t to regular char*
	char nameMB[1024];
	WideCharToMultiByte(CP_ACP, 0, str, -1, nameMB, wcslen(str), nullptr,nullptr);
	write(nameMB, wcslen(str));
	write(short(0),1);
}

void ChunkWriter::writeMatrix(const Matrix3& matrix)
{	
	Point3 row;
	/*
	for (int i = 0; i < 4; ++i)
	{
		row = matrix.GetRow(i);
		write(row.x);
		write(row.y);
		write(row.z);
		write(0.f);
	}*/
	Point4 column;
	for(int i=0;i<3;++i)
	{
		column = matrix.GetColumn(i);
		write(column.x);
		write(column.y);
		write(column.z);
		write(column.w);
	}
}

void ChunkWriter::fillBytes(int count)
{
	writeValues(byte(0),count);
}