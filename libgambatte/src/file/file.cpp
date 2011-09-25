/***************************************************************************
Copyright (C) 2007 by Nach
http://nsrt.edgeemu.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 2 for more details.

You should have received a copy of the GNU General Public License
version 2 along with this program; if not, write to the
Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
***************************************************************************/

#include "file.h"
#include <algorithm>

using namespace std;

static const unsigned int MAX_FILE_NAME = 512;

namespace gambatte {

File::File(const char *filename) : stream(filename, ios::in | ios::binary), is_zip(false), fsize(0), count(0)
{
  mem.active = false;
  if (stream)
  {
    stream.seekg(0, ios::end);
    fsize = stream.tellg();
    stream.seekg(0, ios::beg);
  }
}

File::File(const void *buffer, std::size_t size) : is_zip(false)
{
  mem.active = true;
  mem.ptr = 0;
  mem.size = size;
  mem.data.reserve(size);
  std::copy(reinterpret_cast<const uint8_t*>(buffer),
      reinterpret_cast<const uint8_t*>(buffer) + size,
      mem.data.begin());
}

File::~File()
{
  if (!mem.active)
  {
    close();
  }
}

void File::rewind()
{
  if (mem.active)
  {
    mem.ptr = 0;
  }
  else if (is_open())
  {
    stream.seekg(0, ios::beg);
  }
}

bool File::is_open()
{
  if (mem.active) return true;
  return(stream.is_open());
}

void File::close()
{
  if (is_open())
  {
    stream.close();
  }
}

void File::read(char *buffer, size_t amount)
{
  if (mem.active)
  {
    size_t max_read = std::min(amount, mem.size - mem.ptr);
    std::copy(reinterpret_cast<const char*>(&mem.data[mem.ptr]),
        reinterpret_cast<const char*>(&mem.data[mem.ptr + max_read]),
        buffer);
    mem.ptr += max_read;
    mem.count = max_read;
  }
  else
  {
    if (is_open())
    {
      stream.read(buffer, amount);
      count = stream.gcount();
    }
    else
    {
      count = 0;
    }
  }
}

}
