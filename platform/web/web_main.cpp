/**************************************************************************/
/*  web_main.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "display_server_web.h"
#include "godot_js.h"
#include "os_web.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/profiling/profiling.h"
#include "main/main.h"

#ifdef TOOLS_ENABLED
#include "core/io/file_access.h"
#include "editor/web_tools_editor_plugin.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h" // SceneTree only forward declares it.
#endif

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>

#include <cstdlib>
#include <cstring>

// TODO(davnotdev)
// If WebGPU is enabled, we need to obtain our instance and device as soon as possible to avoid aync code.
#if defined(WEBGPU_ENABLED) && defined(WEBGPU_BACKEND_EMDAWN) && !defined(PROXY_TO_PTHREAD_ENABLED)
#include "drivers/webgpu/webgpu_web_preinit.h"
#define WEB_WEBGPU_PREINIT
#endif

static OS_Web *os = nullptr;
#ifndef PROXY_TO_PTHREAD_ENABLED
static uint64_t target_ticks = 0;
#endif

static bool main_started = false;
static bool shutdown_complete = false;

void exit_callback() {
	if (!shutdown_complete) {
		return; // Still waiting.
	}
	if (main_started) {
		Main::cleanup();
		main_started = false;
	}
	int exit_code = OS_Web::get_singleton()->get_exit_code();
	memdelete(os);
	os = nullptr;
	godot_cleanup_profiler();
	emscripten_force_exit(exit_code); // Exit runtime.
}

void cleanup_after_sync() {
	shutdown_complete = true;
}

void main_loop_callback() {
#ifndef PROXY_TO_PTHREAD_ENABLED
	uint64_t current_ticks = os->get_ticks_usec();
#endif

	bool force_draw = DisplayServerWeb::get_singleton()->check_size_force_redraw();
	if (force_draw) {
		Main::force_redraw();
#ifndef PROXY_TO_PTHREAD_ENABLED
	} else if (current_ticks < target_ticks) {
		return; // Skip frame.
#endif
	}

#ifndef PROXY_TO_PTHREAD_ENABLED
	int max_fps = Engine::get_singleton()->get_max_fps();
	if (max_fps > 0) {
		if (current_ticks - target_ticks > 1000000) {
			// When the window loses focus, we stop getting updates and accumulate delay.
			// For this reason, if the difference is too big, we reset target ticks to the current ticks.
			target_ticks = current_ticks;
		}
		target_ticks += (uint64_t)(1000000 / max_fps);
	}
#endif

	if (os->main_loop_iterate()) {
		emscripten_cancel_main_loop(); // Cancel current loop and set the cleanup one.
		emscripten_set_main_loop(exit_callback, -1, false);
		godot_js_os_finish_async(cleanup_after_sync);
	}
}

void print_web_header() {
	// Emscripten.
	char *emscripten_version_char = godot_js_emscripten_get_version();
	String emscripten_version = vformat("Emscripten %s", emscripten_version_char);
	// `free()` is used here because it's not memory that was allocated by Godot.
	free(emscripten_version_char);

	// Build features.
	String thread_support = OS::get_singleton()->has_feature("threads")
			? "multi-threaded"
			: "single-threaded";
	String extensions_support = OS::get_singleton()->has_feature("web_extensions")
			? "GDExtension support"
			: "no GDExtension support";

	Vector<String> build_configuration = { emscripten_version, thread_support, extensions_support };
	print_line(vformat("Build configuration: %s.", String(", ").join(build_configuration)));
}

static int _godot_web_main_start(int argc, char *argv[]) {
#ifdef TOOLS_ENABLED
	WebToolsEditorPlugin::initialize();
#endif

	// We must override main when testing is enabled
	TEST_MAIN_OVERRIDE

	Error err = Main::setup(argv[0], argc - 1, &argv[1]);

	// Proper shutdown in case of setup failure.
	if (err != OK) {
		// Will only exit after sync.
		emscripten_set_main_loop(exit_callback, -1, false);
		godot_js_os_finish_async(cleanup_after_sync);
		if (err == ERR_HELP) { // Returned by --help and --version, so success.
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	print_web_header();

	main_started = true;

	// Ease up compatibility.
	ResourceLoader::set_abort_on_missing_resources(false);

	int ret = Main::start();
	os->set_exit_code(ret);
	os->get_main_loop()->initialize();
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_project_manager_hint() && FileAccess::exists("/tmp/preload.zip")) {
		PackedStringArray ps;
		ps.push_back("/tmp/preload.zip");
		SceneTree::get_singleton()->get_root()->emit_signal(SNAME("files_dropped"), ps);
	}
#endif
	emscripten_set_main_loop(main_loop_callback, -1, false);
	// Immediately run the first iteration.
	// NOTE: With `WEB_WEBGPU_PREINIT`, this runs from the WebGPU device callback rather than inside an animation frame.
	main_loop_callback();

	return os->get_exit_code();
}

#ifdef WEB_WEBGPU_PREINIT
static int web_main_argc = 0;
static char **web_main_argv = nullptr;

static void _webgpu_preinit_done(void *) {
	_godot_web_main_start(web_main_argc, web_main_argv);
	emscripten_runtime_keepalive_pop();
}
#endif

/// When calling main, it is assumed FS is setup and synced.
extern EMSCRIPTEN_KEEPALIVE int godot_web_main(int argc, char *argv[]) {
	godot_init_profiler();

	os = new OS_Web();

#ifdef WEB_WEBGPU_PREINIT
	web_main_argc = argc;
	web_main_argv = (char **)malloc(sizeof(char *) * (argc + 1));
	for (int i = 0; i < argc; i++) {
		web_main_argv[i] = strdup(argv[i]);
	}
	web_main_argv[argc] = nullptr;

	emscripten_runtime_keepalive_push();
	webgpu_web_preinit_start(_webgpu_preinit_done, nullptr);
	return EXIT_SUCCESS;
#else
	return _godot_web_main_start(argc, argv);
#endif
}
