#include "fusionframework/Game.h"
#include "fusionframework/PapyrusCompat.h"

namespace fusionframework::papyrus {
namespace {
bool PackObject(RE::BSScript::IVirtualMachine& vm, std::uint32_t typeID,
                const void* form, RE::BSScript::Variable& value)
{
    RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> type;
    if (!form || !vm.GetScriptObjectType(typeID, type) || !type) {
        return false;
    }
    auto& handles = vm.GetObjectHandlePolicy();
    const auto handle = handles.GetHandleForObject(typeID, form);
    if (handle == handles.EmptyHandle()) {
        return false;
    }
    RE::BSTSmartPointer<RE::BSScript::Object> object;
    // Native results must match the declared type, even when the reference has derived scripts attached.
    if (!vm.FindBoundObject(handle, type->name.c_str(), false, object, true)) {
        // OG163 and AE221 use slot 0x17; CommonLib's overload order selects incompatible slot 0x16.
        using CreateObject = bool (*)(RE::BSScript::IVirtualMachine*, const RE::BSFixedString&,
                                      RE::BSTSmartPointer<RE::BSScript::Object>&);
        const auto vtable = *reinterpret_cast<const std::uintptr_t* const*>(&vm);
        if (!reinterpret_cast<CreateObject>(vtable[0x17])(&vm, type->name, object) || !object) {
            return false;
        }
        vm.GetObjectBindPolicy().BindObject(object, handle);
    }
    if (!object) {
        return false;
    }
    value = std::move(object);
    return true;
}
}

bool PackForm(RE::BSScript::IVirtualMachine& vm, std::uint32_t typeID,
              const RE::TESForm* form, RE::BSScript::Variable& value)
{
    return PackObject(vm, typeID, form, value);
}

bool PackActiveEffect(RE::BSScript::IVirtualMachine& vm, const RE::ActiveEffect* effect,
                      RE::BSScript::Variable& value)
{
    return PackObject(vm, RE::BSScript::GetVMTypeID<RE::ActiveEffect>(), effect, value);
}
}

namespace RE::BSScript {
template <>
void PackVariable<TESForm>(Variable& value, const volatile TESForm* form)
{
    value = nullptr;
    if (!form) {
        return;
    }
    const auto* game = GameVM::GetSingleton();
    const auto vm = game ? game->GetVM() : nullptr;
    if (!vm || !fusionframework::papyrus::PackForm(*vm, GetVMTypeID<TESForm>(), const_cast<const TESForm*>(form), value)) {
        spdlog::error("Papyrus: failed to pack Form result");
    }
}

template <>
void PackVariable<ActorValueInfo>(Variable& value, const volatile ActorValueInfo* form)
{
    value = nullptr;
    if (!form) {
        return;
    }
    const auto* game = GameVM::GetSingleton();
    const auto vm = game ? game->GetVM() : nullptr;
    if (!vm || !fusionframework::papyrus::PackForm(*vm, GetVMTypeID<ActorValueInfo>(), const_cast<const ActorValueInfo*>(form), value)) {
        spdlog::error("Papyrus: failed to pack ActorValue result");
    }
}

template <>
void PackVariable<TESObjectREFR>(Variable& value, const volatile TESObjectREFR* form)
{
    value = nullptr;
    if (!form) {
        return;
    }
    const auto* game = GameVM::GetSingleton();
    const auto vm = game ? game->GetVM() : nullptr;
    if (!vm || !fusionframework::papyrus::PackForm(*vm, GetVMTypeID<TESObjectREFR>(), const_cast<const TESObjectREFR*>(form), value)) {
        spdlog::error("Papyrus: failed to pack ObjectReference result");
    }
}

template <>
void PackVariable<TESNPC>(Variable& value, const volatile TESNPC* form)
{
    value = nullptr;
    if (!form) {
        return;
    }
    const auto* game = GameVM::GetSingleton();
    const auto vm = game ? game->GetVM() : nullptr;
    if (!vm || !fusionframework::papyrus::PackForm(*vm, GetVMTypeID<TESNPC>(), const_cast<const TESNPC*>(form), value)) {
        spdlog::error("Papyrus: failed to pack ActorBase result");
    }
}

template <>
void PackVariable<TESObjectARMO>(Variable& value, const volatile TESObjectARMO* form)
{
    value = nullptr;
    if (!form) {
        return;
    }
    const auto* game = GameVM::GetSingleton();
    const auto vm = game ? game->GetVM() : nullptr;
    if (!vm || !fusionframework::papyrus::PackForm(*vm, GetVMTypeID<TESObjectARMO>(), const_cast<const TESObjectARMO*>(form), value)) {
        spdlog::error("Papyrus: failed to pack Armor result");
    }
}

template <>
void PackVariable<BGSHeadPart>(Variable& value, const volatile BGSHeadPart* form)
{
    value = nullptr;
    if (!form) {
        return;
    }
    const auto* game = GameVM::GetSingleton();
    const auto vm = game ? game->GetVM() : nullptr;
    if (!vm || !fusionframework::papyrus::PackForm(*vm, GetVMTypeID<BGSHeadPart>(), const_cast<const BGSHeadPart*>(form), value)) {
        spdlog::error("Papyrus: failed to pack HeadPart result");
    }
}
}

namespace RE::BSScript {
namespace {
template <class T>
T UnpackNoneSafeArray(const Variable& value)
{
    T result;
    if (!value.is<Array>()) {
        return result;
    }
    const auto array = get<Array>(value);
    if (!array) {
        return result;
    }
    result.reserve(array->elements.size());
    for (const auto& element : array->elements) {
        result.push_back(detail::UnpackVariable<typename T::value_type>(element));
    }
    return result;
}
}

template <>
std::vector<float> UnpackVariable<std::vector<float>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<float>>(value);
}

template <>
std::vector<std::int32_t> UnpackVariable<std::vector<std::int32_t>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<std::int32_t>>(value);
}

template <>
std::vector<std::string> UnpackVariable<std::vector<std::string>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<std::string>>(value);
}

template <>
std::vector<TESForm*> UnpackVariable<std::vector<TESForm*>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<TESForm*>>(value);
}

template <>
std::vector<TESObjectREFR*> UnpackVariable<std::vector<TESObjectREFR*>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<TESObjectREFR*>>(value);
}

template <>
std::vector<Actor*> UnpackVariable<std::vector<Actor*>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<Actor*>>(value);
}

template <>
std::vector<TESWorldSpace*> UnpackVariable<std::vector<TESWorldSpace*>>(const Variable& value)
{
    return UnpackNoneSafeArray<std::vector<TESWorldSpace*>>(value);
}
}
