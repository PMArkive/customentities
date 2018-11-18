#include "memutils.h"

#include <Windows.h>

namespace MemUtils
{
	const void *const GetFuncAtOffset(const void *const ptr, const int offset) noexcept
	{
		const int relative{*reinterpret_cast<const int *const>(reinterpret_cast<const unsigned long>(ptr) + (offset + 1))};
		return reinterpret_cast<const void *const>(reinterpret_cast<const unsigned long>(ptr) + (offset + 1) + 4 + relative);
	}

	const void *const *const GetVTable(const void *const ptr) noexcept
	{
		const void *const *const vtable{*static_cast<const void *const *const *const>(ptr)};
		return vtable;
	}

	const void *const GetVFunc(const void *const ptr, const int index) noexcept
	{
		const void *const *const vtable{GetVTable(ptr)};
		return vtable[index];
	}

	const int GetVFuncIndex(const void *const func) noexcept
	{
		const unsigned char *addr{*reinterpret_cast<const unsigned char *const *const>(&func)};
		if(*addr == 0xE9)
			addr += (5 + *reinterpret_cast<const unsigned long *const>(addr + 1));

		if(addr[0] == 0x8B && addr[1] == 0x44 && addr[2] == 0x24 && addr[3] == 0x04 && addr[4] == 0x8B && addr[5] == 0x00)
			addr += 6;
		else if(addr[0] == 0x8B && addr[1] == 0x01)
			addr += 2;
		else if(addr[0] == 0x48 && addr[1] == 0x8B && addr[2] == 0x01)
			addr += 3;
		else
			return -1;

		if(*addr == 0xFF || *addr == 0x8B) {
			*addr++;
			if(*addr == 0x60 || *addr == 0x40) {
				*addr++;
				return (*addr / sizeof(const void *const));
			} else if(*addr == 0xA0) {
				addr++;
				return (*(reinterpret_cast<const unsigned int *const>(addr)) / sizeof(const void *const));
			} else if(*addr == 0x20 || *addr == 0x0)
				return 0;
			else
				return -1;
		}

		return -1;
	}

	void SetVTable(void *const ptr, const void *const *const vtable) noexcept
	{
		const void *const **const oldvtable{&*static_cast<const void *const **const>(ptr)};

		unsigned long old{0};
		VirtualProtect(oldvtable, sizeof(oldvtable), PAGE_EXECUTE_READWRITE, &old);

		*oldvtable = vtable;

		VirtualProtect(oldvtable, sizeof(oldvtable), old, nullptr);
	}
};