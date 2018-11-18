#ifndef _EXTENSION_H_
#define _EXTENSION_H_

#pragma once

#include <smsdk_ext.h>

class __single_inheritance __declspec(novtable) IEntityFactoryDictionary;
class __single_inheritance __declspec(novtable) IServerTools;

extern IEntityFactoryDictionary *entityfactory;
extern IServerTools *servertools;
extern IGameConfig *mainconfig;

class Extension final : public SDKExtension, public IConCommandBaseAccessor
{
	private:
		virtual bool SDK_OnLoad(char *const error, const size_t maxlen, const bool late) noexcept override final;
		virtual void SDK_OnUnload() noexcept override final;
		//virtual void SDK_OnAllLoaded();
		//virtual void SDK_OnPauseChange(bool paused);
		//virtual bool QueryRunning(char *error, size_t maxlen);
	private:
	#ifdef SMEXT_CONF_METAMOD
		virtual bool SDK_OnMetamodLoad(ISmmAPI *const ismm, char *const error, const size_t maxlen, const bool late) noexcept override final;
		virtual bool SDK_OnMetamodUnload(char *const error, const size_t maxlen) noexcept override final;
		//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
	#endif
	private:
		virtual bool RegisterConCommandBase(ConCommandBase *const pVar) override final;
};

#endif