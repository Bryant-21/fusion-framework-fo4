#pragma once
// Every translation unit of a consumer must see these specializations (include from the pch):
// the framework compiles their definitions into the DLL, and a TU that instantiated the
// dependency's generic packer instead would be an ODR violation.
#include <RE/Fallout.h>

namespace fusionframework::papyrus {
bool PackForm(RE::BSScript::IVirtualMachine& vm, std::uint32_t typeID,
              const RE::TESForm* form, RE::BSScript::Variable& value);
bool PackActiveEffect(RE::BSScript::IVirtualMachine& vm, const RE::ActiveEffect* effect,
                      RE::BSScript::Variable& value);
}

namespace RE::BSScript {
// Preserve native Form[] return metadata while replacing the dependency's unsafe element packer.
template <>
void PackVariable<TESForm>(Variable& value, const volatile TESForm* form);

template <>
void PackVariable<ActorValueInfo>(Variable& value, const volatile ActorValueInfo* form);

template <>
void PackVariable<TESObjectREFR>(Variable& value, const volatile TESObjectREFR* form);

template <>
void PackVariable<TESNPC>(Variable& value, const volatile TESNPC* form);

template <>
void PackVariable<TESObjectARMO>(Variable& value, const volatile TESObjectARMO* form);

template <>
void PackVariable<BGSHeadPart>(Variable& value, const volatile BGSHeadPart* form);

// A None Papyrus array keeps its array type with a null pointer, which the dependency's unpacker
// dereferences. Every std::vector type a B21 native takes as a parameter must be listed here.
template <>
std::vector<float> UnpackVariable<std::vector<float>>(const Variable& value);

template <>
std::vector<std::int32_t> UnpackVariable<std::vector<std::int32_t>>(const Variable& value);

template <>
std::vector<std::string> UnpackVariable<std::vector<std::string>>(const Variable& value);

template <>
std::vector<TESForm*> UnpackVariable<std::vector<TESForm*>>(const Variable& value);

template <>
std::vector<TESObjectREFR*> UnpackVariable<std::vector<TESObjectREFR*>>(const Variable& value);

template <>
std::vector<Actor*> UnpackVariable<std::vector<Actor*>>(const Variable& value);

template <>
std::vector<TESWorldSpace*> UnpackVariable<std::vector<TESWorldSpace*>>(const Variable& value);
}
