// register_types.cpp - Entry point for ONNXRuntime.gd GDExtension

#include "register_types.h"

// Autowrapper-generated module registration
#include "autowrapper/module.h"
#include "ort_errors.h"
#include "ort_adapters.hpp"

#include <godot_cpp/core/class_db.hpp>

static void onnxruntime_gd_initialize(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register error diagnostics class
    godot::ClassDB::register_class<godot::OrtErrors>();

    // Register autowrapper-generated classes
    gdext_initialize_module_auto(p_level);

    // Register high-level adapters class
    godot::ClassDB::register_class<godot::OrtAdapters>();
}

static void onnxruntime_gd_uninitialize(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    gdext_uninitialize_module_auto(p_level);
}

#include <locale>

extern "C" {
    GDExtensionBool GDE_EXPORT gdext_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {
        try {
            std::locale::global(std::locale::classic());
        } catch (...) {
        }

        const godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization
        );

        init_obj.register_initializer(onnxruntime_gd_initialize);
        init_obj.register_terminator(onnxruntime_gd_uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
