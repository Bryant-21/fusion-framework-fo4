#include "fusionframework/Game.h"
#include "fusionframework/SourceTemplates.h"

namespace fusionframework::forms {
TemplateKey TemplateSource(RE::Actor* actor, TemplateCategory category)
{
    if (!actor) return {};
    const auto index = static_cast<std::size_t>(category);
    auto* base = actor->GetNPC();
    if (actor->extraList) {
        if (auto* extra = actor->extraList->GetByType<RE::ExtraLeveledCreature>()) {
            auto* selected = extra->templates[index] ? extra->templates[index] : extra->originalBase;
            base = selected ? selected->As<RE::TESNPC>() : nullptr;
        }
    }
    // Runs inside hot engine hooks: a hop limit instead of a visited set (template chains are a few deep).
    for (int hops = 0; base && hops < 64; ++hops) {
        const auto flag = static_cast<RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG>(1U << index);
        if (!base->actorData.templateUseFlags.all(flag)) return {base, actor->race};
        auto* next = base->templateForms ? base->templateForms[index] : nullptr;
        if (!next) next = base->baseTemplateForm;
        // A leveled list must already have been resolved by CalcTemplateForRef, never by the converter.
        base = next ? next->As<RE::TESNPC>() : nullptr;
    }
    return {};
}
}
