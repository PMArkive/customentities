#ifndef _CCUSTOMWORLD_H_
#define _CCUSTOMWORLD_H_

#pragma once

#include <CCustomBaseEntity.h>

class CCustomWorld : public CCustomBaseEntity
{
	public:
		using CCustomBaseEntity::CCustomBaseEntity;

	private:
		virtual void Spawn() noexcept override final;
};

#endif