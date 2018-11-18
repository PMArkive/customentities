#include "CDetour.h"

#include <Windows.h>
#include <Detours/src/detours.h>

extern SourceHook::IHookManagerAutoGen *g_HMAGPtr;

CDetour::DetourList_t &CDetour::GetDetourList() noexcept {
	static DetourList_t _detourlist{};
	return _detourlist;
}

asmjit::JitRuntime &CDetour::JitRuntime() noexcept {
	static asmjit::JitRuntime _asmjitruntime{};
	return _asmjitruntime;
}

static void PreDelegateCall() noexcept
{
	RETURN_META(MRES_SUPERCEDE);
}

class CSHDelegate final : public SourceHook::ISHDelegate {
	public:
		CSHDelegate(const void *const pthis, const void *const dst) noexcept;
		~CSHDelegate() noexcept;
	private:
		virtual bool IsEqual(ISHDelegate *const pOtherDeleg) noexcept override final;
		virtual void DeleteThis() noexcept override final {
			//delete this;
		}
		virtual void Func() noexcept final;
	private:
		const void *const *_oldvtabl{nullptr};
		const void *_newvtabl[3]{nullptr};
};

struct PubFuncInfo_t final
{
	public:
		int _index{-1};
		CArgsBuilder _args{};
		int _ref{0};

		SourceHook::HookManagerPubFunc _pubfunc{nullptr};

		const bool operator==(const PubFuncInfo_t &other) const noexcept {
			if(_index != other._index)
				return false;

			const SourceHook::ProtoInfo *const protoinfo{other._args.GetInfo()};
			const SourceHook::ProtoInfo *const _protoinfo{_args.GetInfo()};

			if(_protoinfo->convention != protoinfo->convention)
				return false;

			if(_protoinfo->numOfParams != protoinfo->numOfParams)
				return false;

			int equal{0};
			for(int i{0}; i < protoinfo->numOfParams; i++) {
				if(_protoinfo->paramsPassInfo[i].size != protoinfo->paramsPassInfo[i].size)
					break;

				if(_protoinfo->paramsPassInfo[i].flags != protoinfo->paramsPassInfo[i].flags)
					break;

				if(_protoinfo->paramsPassInfo[i].type != protoinfo->paramsPassInfo[i].type)
					break;
				equal++;
			}
			if(equal != protoinfo->numOfParams)
				return false;

			return true;
		}

		const bool operator!=(const PubFuncInfo_t &other) const noexcept
			{ return !operator==(other); }
};

struct DelegateInfo_t final
{
	public:
		const void *_pthis{nullptr};
		const void *_dst{nullptr};
		int _ref{0};

		CSHDelegate *_delegate{nullptr};

		const bool operator==(const DelegateInfo_t &other) const noexcept {
			if(_pthis != other._pthis)
				return false;

			if(_dst != other._dst)
				return false;

			return true;
		}

		const bool operator!=(const DelegateInfo_t &other) const noexcept
			{ return !operator==(other); }
};

class CHookCache final
{
	public:
		PubFuncInfo_t &FindOrCreatePubFunc(const int index, const CArgsBuilder &args) noexcept;
		DelegateInfo_t &FindOrCreateDelegate(const void *const pthis, const void *const dst) noexcept;

		void Remove(DelegateInfo_t &info) noexcept;
		void Remove(PubFuncInfo_t &info) noexcept;

	public:
		using PubFuncList_t = std::forward_list<PubFuncInfo_t>;
		PubFuncList_t _pubfunclist{};

		using DelegateList_t = std::forward_list<DelegateInfo_t>;
		DelegateList_t _delegatelist{};
};

static CHookCache &HookCache() noexcept {
	static CHookCache _hookcache{};
	return _hookcache;
}

void CHookCache::Remove(DelegateInfo_t &info) noexcept
{
	info._ref--;
	if(info._ref == 0) {
		if(info._delegate)
			delete info._delegate;
		_delegatelist.remove(info);
		return;
	}

	//info._delegate = new CSHDelegate{info._pthis, info._dst};
}

void CHookCache::Remove(PubFuncInfo_t &info) noexcept
{
	info._ref--;
	if(info._ref == 0) {
		if(info._pubfunc)
			g_HMAGPtr->ReleaseHookMan(info._pubfunc);
		_pubfunclist.remove(info);
		return;
	}
}

PubFuncInfo_t &CHookCache::FindOrCreatePubFunc(const int index, const CArgsBuilder &args) noexcept
{
	const SourceHook::ProtoInfo *const protoinfo{args.GetInfo()};

	for(PubFuncList_t::value_type &it : _pubfunclist) {
		if(it._index != index)
			continue;

		const SourceHook::ProtoInfo *const _protoinfo{it._args.GetInfo()};

		if(_protoinfo->convention != protoinfo->convention)
			continue;

		if(_protoinfo->numOfParams != protoinfo->numOfParams)
			continue;

		int equal{0};
		for(int i{0}; i < protoinfo->numOfParams; i++) {
			if(_protoinfo->paramsPassInfo[i].size != protoinfo->paramsPassInfo[i].size)
				break;

			if(_protoinfo->paramsPassInfo[i].flags != protoinfo->paramsPassInfo[i].flags)
				break;

			if(_protoinfo->paramsPassInfo[i].type != protoinfo->paramsPassInfo[i].type)
				break;
			equal++;
		}
		if(equal != protoinfo->numOfParams)
			continue;

		it._ref++;
		return it;
	}

	PubFuncInfo_t info{};
	info._index = index;
	info._args = args;
	info._pubfunc = g_HMAGPtr->MakeHookMan(protoinfo, 0, index);
	info._ref = 1;
	_pubfunclist.push_front(std::move(info));

	return _pubfunclist.front();
}

DelegateInfo_t &CHookCache::FindOrCreateDelegate(const void *const pthis, const void *const dst) noexcept
{
	for(DelegateList_t::value_type &it : _delegatelist) {
		if(it._dst != dst)
			continue;

		if(it._pthis != pthis)
			continue;

		it._ref++;
		return it;
	}

	DelegateInfo_t info{};
	info._dst = dst;
	info._pthis = pthis;
	info._delegate = new CSHDelegate{pthis, dst};
	info._ref = 1;
	_delegatelist.push_front(std::move(info));

	return _delegatelist.front();
}

CSHDelegate::CSHDelegate(const void *const pthis, const void *const dst) noexcept
{
	_oldvtabl = MemUtils::GetVTable(this);

	_newvtabl[0] = _oldvtabl[0];
	_newvtabl[1] = _oldvtabl[1];

	asmjit::CodeHolder holder{};
	holder.init(CDetour::JitRuntime().getCodeInfo());

	asmjit::X86Assembler assem{&holder};
	assem.call(asmjit::imm_ptr(PreDelegateCall));
	if(pthis)
		assem.lea(asmjit::x86::ecx, asmjit::x86::ptr(reinterpret_cast<const size_t>(pthis)));
	assem.jmp(asmjit::imm_ptr(dst));

	CDetour::JitRuntime().add(&_newvtabl[2], &holder);

	MemUtils::SetVTable(this, _newvtabl);
}

CSHDelegate::~CSHDelegate() noexcept
{
	MemUtils::SetVTable(this, _oldvtabl);

	CDetour::JitRuntime().release(_newvtabl[2]);
}

void CSHDelegate::Func() noexcept
{
	__debugbreak();
	RETURN_META(MRES_IGNORED);
}

bool CSHDelegate::IsEqual(ISHDelegate *const pOtherDeleg) noexcept
{
	if(!pOtherDeleg)
		return false;

	return (MemUtils::GetVTable(this) == MemUtils::GetVTable(pOtherDeleg));
}

const bool CDetour::Hook(const void *const ptr, const int index, const void *pthis, const void *const dst, const CArgsBuilder &args, const bool post) noexcept
{
	Remove();

	if(!ptr || (index < 0) || !dst)
		return false;

	_pubfuncinfo = &HookCache().FindOrCreatePubFunc(index, args);
	_delegateinfo = &HookCache().FindOrCreateDelegate(pthis, dst);

	_hookid = g_SHPtr->AddHook(g_PLID, SourceHook::ISourceHook::Hook_Normal, const_cast<void *const>(ptr), 0, _pubfuncinfo->_pubfunc, _delegateinfo->_delegate, post);
	GetDetourList().push_front(this);
	return true;
}

const bool CDetour::Detour(const void **const src, const void *const pthis, const void *const dst, const void *const addr) noexcept
{
	Remove();

	if(!src || !dst || !addr)
		return false;

	*src = addr;

	_thread = GetCurrentThread();
	_src = *src;
	_dst = dst;

	if(pthis) {
		asmjit::CodeHolder holder{};
		holder.init(JitRuntime().getCodeInfo());

		asmjit::X86Assembler assem{&holder};
		assem.lea(asmjit::x86::ecx, asmjit::x86::ptr(reinterpret_cast<const size_t>(pthis)));
		assem.jmp(asmjit::imm_ptr(_dst));

		JitRuntime().add(&_dst, &holder);

		_thispatched = true;
	}

	DetourTransactionBegin();
	DetourUpdateThread(_thread);

	if(DetourAttach(const_cast<void **>(&_src), const_cast<void *const>(_dst)) != NO_ERROR) {
		Remove();
		return false;
	}

	DetourTransactionCommit();

	*src = _src;

	GetDetourList().push_front(this);
	return true;
}

void CDetour::UnDetour() noexcept
{
	if(!_thread)
		_thread = GetCurrentThread();

	if(_src && _dst) {
		DetourTransactionBegin();
		DetourUpdateThread(_thread);
		DetourDetach(const_cast<void **>(&_src), const_cast<void *const>(_dst));
		DetourTransactionCommit();
	}

	if(_thispatched && _dst)
		JitRuntime().release(_dst);

	_thread = nullptr;
	_src = nullptr;
	_dst = nullptr;
	_thispatched = false;
	_paused = false;

	GetDetourList().remove(this);
}

void CDetour::UnHook() noexcept
{
	if(_hookid)
		g_SHPtr->RemoveHookByID(_hookid);
	_hookid = 0;

	if(_delegateinfo)
		HookCache().Remove(*_delegateinfo);
	_delegateinfo = nullptr;

	if(_pubfuncinfo)
		HookCache().Remove(*_pubfuncinfo);
	_pubfuncinfo = nullptr;

	_paused = false;

	GetDetourList().remove(this);
}

void CDetour::Remove() noexcept
{
	UnDetour();
	UnHook();
}

void CDetour::Resume() noexcept
{
	if(!_paused)
		return;

	/*
	if(!_thread)
		_thread = GetCurrentThread();

	if(_src && _dst) {
		DetourTransactionBegin();
		DetourUpdateThread(_thread);
		DetourAttach(ccast<void **>(&_src), ccast<void *const>(_dst));
		DetourTransactionCommit();
	}
	*/

	if(_hookid)
		g_SHPtr->UnpauseHookByID(_hookid);

	_paused = false;
}

void CDetour::Pause() noexcept
{
	if(_paused)
		return;

	/*
	if(!_thread)
		_thread = GetCurrentThread();

	if(_src && _dst) {
		DetourTransactionBegin();
		DetourUpdateThread(_thread);
		DetourDetach(ccast<void **>(&_src), ccast<void *const>(_dst));
		DetourTransactionCommit();
	}
	*/

	if(_hookid)
		g_SHPtr->PauseHookByID(_hookid);

	_paused = true;
}

void CDetour::RemoveAll() noexcept
{
	DetourList_t::const_iterator it{GetDetourList().begin()};
	while(it != GetDetourList().end()) {
		CDetour *const detour{*it};
		it++;
		detour->Remove();
	}
	GetDetourList().clear();
}