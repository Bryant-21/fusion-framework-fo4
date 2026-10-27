#pragma once
#include <cstdint>

// Tales -> FullScreenMap over F4SE messaging (receiver "B21_FullScreenMap"), plus one export
// Tales resolves with GetProcAddress. Everything is versioned JSON so either side can add
// fields without a lockstep release.
namespace fusionframework::tales_map {
    inline constexpr std::uint32_t kVersion = 1;
    inline constexpr std::uint32_t kMessageType = 0x42325444;   // 'B2TD'

    // Map -> Tales, always on the game thread. JSON: {"op":"respawnSelect","sequence":N,"id":N}
    // or {"op":"respawnCancel","sequence":N}.
    using ReplyFn = void (*)(const char* json);

    struct Message {
        std::uint32_t magic;        // kMessageType
        std::uint32_t version;      // kVersion
        const char* json;           // valid only during the Dispatch call
        std::uint32_t length;
        std::uint32_t accepted;     // out: map sets 1 when it acted on the message
        ReplyFn reply;              // respawnOpen only; must stay valid for the plugin lifetime
    };
    static_assert(sizeof(Message) == 32);

    // ops (Tales -> map):
    //   {"op":"infestations","version":1,"session":N,"world":"APPALACHIA","layers":[{"src":"infestations/fog0.dds","scale":f,"period":f}],"fogs":[{"px":f,"py":f,"r":f,"discovered":bool,"sx":f,"sy":f,"name":"..."}]}
    //   {"op":"reputation","version":1,"factions":[...],"labels":{...}}
    //   {"op":"respawnOpen","version":1,"sequence":N,"world":<WRLD formID>,"choices":[{"id":N,"name":"...","x":f,"y":f}],"labels":{...}}
    //   {"op":"respawnClose","version":1,"sequence":N}
    //   {"op":"notice","version":1,"text":"..."}

    // Exported by B21_FullScreenMap.dll: map-marker discovery state for a full FormID
    // (0 unknown, 1 sighted, 2 discovered). Safe from the game thread only.
    inline constexpr const char* kMarkerStateExport = "B21_GetMapMarkerStateV1";
    using MarkerStateFn = std::uint32_t (*)(std::uint32_t fullFormID);
}
