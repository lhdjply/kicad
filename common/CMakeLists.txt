# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

set( INC_AFTER ${INC_AFTER} ${NGSPICE_INCLUDE_DIR} )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./widgets
    ./dialog_about
    ${CMAKE_SOURCE_DIR}/resources/bitmaps_png
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${INC_AFTER}
    )

# Get the GAL Target
add_subdirectory( gal )

# Only for win32 cross compilation using MXE
add_compile_definitions( $<$<AND:$<BOOL:${WIN32}>,$<BOOL:${MSYS}>>:GLEW_STATIC> )


# The build version string defaults to the value in the KiCadVersion.cmake file.
# If being built inside a git repository, the git tag and commit hash are used to create
# a new version string instead. The user can supply an additional string to be appended
# to the end inside the KICAD_VERSION_EXTRA variable
set( KICAD_VERSION_EXTRA "" CACHE STRING
    "User defined configuration string to append to KiCad version." )

# Generate version header file.
add_custom_target(
    version_header ALL
    COMMAND ${CMAKE_COMMAND}
        -DKICAD_VERSION_EXTRA=${KICAD_VERSION_EXTRA}
        -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.h
        -DTEXT_OUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.txt
        -DSRC_PATH=${PROJECT_SOURCE_DIR}
        -DKICAD_CMAKE_MODULE_PATH=${KICAD_CMAKE_MODULE_PATH}
        -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/WriteVersionHeader.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS ${CMAKE_BINARY_DIR}/kicad_build_version.h
    DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/WriteVersionHeader.cmake
    COMMENT "Generating version string header"
    )

# A shared library subsetted from common which restricts what can go into
# a single_top link image.  By not linking to common, we control what does
# statically go into single_top link images.  My current thinking is that only
# wxWidgets should be a shared link from single top, everything else should be
# statically bound into it. Otherwise you will have DSO loading probs. After it
# sets the LIB PATHS however, we want the *.kiface modules to use shared linking.
add_library( singletop STATIC EXCLUDE_FROM_ALL
    eda_doc.cpp
    kiway.cpp
    kiway_holder.cpp )


# A shared library used by multiple *.kiface files and one or two program
# launchers.  Object files can migrate into here over time, but only if they are
# surely needed and certainly used from more than one place without recompilation.
# Functions and data all need to use the #include <import_export.h> and be declared
# as APIEXPORT
set( KICOMMON_SRCS
    # Fonts
    newstroke_font.cpp
    font/fontconfig.cpp
    font/version_info.cpp
    # Gal
    gal/color4d.cpp
    # Jobs
    jobs/job.cpp
    jobs/job_export_pcb_drill.cpp
    jobs/job_export_pcb_dxf.cpp
    jobs/job_export_pcb_gerber.cpp
    jobs/job_export_pcb_gerbers.cpp
    jobs/job_export_pcb_ipc2581.cpp
    jobs/job_export_pcb_pdf.cpp
    jobs/job_export_pcb_pos.cpp
    jobs/job_export_pcb_svg.cpp
    jobs/job_export_pcb_3d.cpp
    jobs/job_export_sch_bom.cpp
    jobs/job_export_sch_netlist.cpp
    jobs/job_export_sch_plot.cpp
    jobs/job_export_sch_pythonbom.cpp
    jobs/job_fp_export_svg.cpp
    jobs/job_fp_upgrade.cpp
    jobs/job_pcb_drc.cpp
    jobs/job_sch_erc.cpp
    jobs/job_sym_export_svg.cpp
    jobs/job_sym_upgrade.cpp

    kicad_curl/kicad_curl.cpp
    kicad_curl/kicad_curl_easy.cpp

    widgets/progress_reporter_base.cpp
    widgets/std_bitmap_button.cpp

    advanced_config.cpp
    asset_archive.cpp
    array_axis.cpp
    array_options.cpp
    bitmap_info.cpp
    build_version.cpp
    config_params.cpp
    confirm.cpp
    dsnlexer.cpp
    eda_pattern_match.cpp
    eda_units.cpp
    exceptions.cpp
    kiid.cpp
    layer_id.cpp
    lib_id.cpp
    locale_io.cpp
    lset.cpp
    markup_parser.cpp
    netclass.cpp
    page_info.cpp
    paths.cpp
    richio.cpp
    string_utils.cpp
    trace_helpers.cpp
    wildcards_and_files_ext.cpp
    wx_filename.cpp

    io/kicad/kicad_io_utils.cpp    # needed by richio
    )

add_library( kicommon SHARED
    ${KICOMMON_SRCS}
    )

target_link_libraries( kicommon
    core
    kimath
    kiplatform
    nlohmann_json
    fmt::fmt
    CURL::libcurl
    ${wxWidgets_LIBRARIES}
    ${LIBGIT2_LIBRARIES}

    # needed by kiid to allow linking for Boost for the UUID against bcrypt (msys2 only)
    ${EXTRA_LIBS}

    # outline font support
   	${FREETYPE_LIBRARIES}
   	${HarfBuzz_LIBRARIES}
	${Fontconfig_LIBRARIES}
    )

include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
include( ${KICAD_CMAKE_MODULE_PATH}/CreateGitVersionHeader.cmake )
create_git_version_header(${CMAKE_SOURCE_DIR})

# Extract the major and minor build version as a string
string( REGEX MATCH
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)"
        KICAD_MAJOR_MINOR_PATCH_VERSION
        "${KICAD_VERSION}"
    )

set_target_properties( kicommon PROPERTIES
    OUTPUT_NAME     kicommon
    SOVERSION      ${KICAD_MAJOR_MINOR_PATCH_VERSION}
    )

install( TARGETS kicommon
    RUNTIME DESTINATION ${KICAD_LIB}
    LIBRARY DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( kicommon PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_LIB_DIR}"
        INSTALL_NAME_DIR "${OSX_BUNDLE_BUILD_LIB_DIR}"
    )
    set_target_properties( kicommon PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks" )
    set_target_properties( kicommon PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )
endif()

target_compile_definitions( kicommon PRIVATE KICOMMON_DLL=1 )

target_include_directories( kicommon
    PUBLIC
        .
        ${CMAKE_BINARY_DIR}
        $<TARGET_PROPERTY:pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )

add_dependencies( kicommon pegtl version_header )

if( MSVC )
    target_sources( kicommon PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/kicommon-dll.rc )
endif()

set( COMMON_ABOUT_DLG_SRCS
    dialog_about/AboutDialog_main.cpp
    dialog_about/dialog_about.cpp
    dialog_about/dialog_about_base.cpp
    )

set( COMMON_GIT_DLG_SRCS
    dialogs/git/dialog_git_commit.cpp
    dialogs/git/dialog_git_switch.cpp
    dialogs/git/dialog_git_auth.cpp
    dialogs/git/dialog_git_repository.cpp
    dialogs/git/dialog_git_repository_base.cpp
    dialogs/git/panel_git_repos.cpp
    dialogs/git/panel_git_repos_base.cpp

    )

set( COMMON_DLG_SRCS
    ${COMMON_GIT_DLG_SRCS}
    dialogs/dialog_assign_netclass.cpp
    dialogs/dialog_assign_netclass_base.cpp
    dialogs/dialog_book_reporter.cpp
    dialogs/dialog_book_reporter_base.cpp
    dialogs/dialog_color_picker.cpp
    dialogs/dialog_color_picker_base.cpp
    dialogs/dialog_configure_paths.cpp
    dialogs/dialog_configure_paths_base.cpp
    dialogs/dialog_display_html_text_base.cpp
    dialogs/dialog_edit_library_tables.cpp
    dialogs/dialog_global_lib_table_config.cpp
    dialogs/dialog_global_lib_table_config_base.cpp
    dialogs/dialog_grid_settings.cpp
    dialogs/dialog_grid_settings_base.cpp
    dialogs/dialog_hotkey_list.cpp
    dialogs/dialog_HTML_reporter_base.cpp
    dialogs/dialog_import_choose_project.cpp
    dialogs/dialog_import_choose_project_base.cpp
    dialogs/dialog_locked_items_query.cpp
    dialogs/dialog_locked_items_query_base.cpp
    dialogs/dialog_migrate_settings.cpp
    dialogs/dialog_migrate_settings_base.cpp
    dialogs/dialog_page_settings_base.cpp
    dialogs/dialog_paste_special.cpp
    dialogs/dialog_paste_special_base.cpp
    dialogs/dialog_plugin_options.cpp
    dialogs/dialog_plugin_options_base.cpp
    dialogs/dialog_text_entry_base.cpp
    dialogs/dialog_page_settings.cpp
    dialogs/dialog_print_generic.cpp
    dialogs/dialog_print_generic_base.cpp
    dialogs/dialog_text_entry.cpp
    dialogs/dialog_unit_entry.cpp
    dialogs/dialog_unit_entry_base.cpp
    dialogs/eda_list_dialog.cpp
    dialogs/eda_list_dialog_base.cpp
    dialogs/eda_reorderable_list_dialog.cpp
    dialogs/eda_reorderable_list_dialog_base.cpp
    dialogs/eda_view_switcher.cpp
    dialogs/eda_view_switcher_base.cpp
    dialogs/hotkey_cycle_popup.cpp
    dialogs/html_message_box.cpp
    dialogs/panel_color_settings_base.cpp
    dialogs/panel_color_settings.cpp
    dialogs/panel_common_settings.cpp
    dialogs/panel_common_settings_base.cpp
    dialogs/panel_gal_display_options.cpp
    dialogs/panel_hotkeys_editor.cpp
    dialogs/panel_image_editor.cpp
    dialogs/panel_image_editor_base.cpp
    dialogs/panel_grid_settings.cpp
    dialogs/panel_grid_settings_base.cpp
    dialogs/panel_mouse_settings.cpp
    dialogs/panel_mouse_settings_base.cpp
    dialogs/panel_packages_and_updates.cpp
    dialogs/panel_packages_and_updates_base.cpp
    dialogs/panel_setup_netclasses.cpp
    dialogs/panel_setup_netclasses_base.cpp
    dialogs/panel_setup_severities.cpp
    dialogs/panel_text_variables.cpp
    dialogs/panel_text_variables_base.cpp
    )

if( KICAD_USE_SENTRY )
    list( APPEND COMMON_DLG_SRCS
    dialogs/panel_data_collection.cpp
    dialogs/panel_data_collection_base.cpp )
endif()

set( COMMON_WIDGET_SRCS
    widgets/app_progress_dialog.cpp
    widgets/bitmap_button.cpp
    widgets/bitmap_toggle.cpp
    widgets/button_row_panel.cpp
    widgets/color_swatch.cpp
    widgets/font_choice.cpp
    widgets/footprint_choice.cpp
    widgets/footprint_diff_widget.cpp
    widgets/footprint_preview_widget.cpp
    widgets/footprint_select_widget.cpp
    widgets/gal_options_panel.cpp
    widgets/gal_options_panel_base.cpp
    widgets/grid_bitmap_toggle.cpp
    widgets/grid_button.cpp
    widgets/grid_color_swatch_helpers.cpp
    widgets/grid_combobox.cpp
    widgets/grid_icon_text_helpers.cpp
    widgets/grid_text_button_helpers.cpp
    widgets/grid_text_helpers.cpp
    widgets/html_window.cpp
    widgets/indicator_icon.cpp
    widgets/wx_infobar.cpp
    widgets/kistatusbar.cpp
    widgets/layer_box_selector.cpp
    widgets/lib_tree.cpp
    widgets/mathplot.cpp
    widgets/msgpanel.cpp
    widgets/number_badge.cpp
    widgets/paged_dialog.cpp
    widgets/properties_panel.cpp
    widgets/search_pane.cpp
    widgets/search_pane_base.cpp
    widgets/search_pane_tab.cpp
    widgets/split_button.cpp
    widgets/stepped_slider.cpp
    widgets/text_ctrl_eval.cpp
    widgets/ui_common.cpp
    widgets/unit_binder.cpp
    widgets/widget_save_restore.cpp
    widgets/widget_hotkey_list.cpp
    widgets/wx_aui_art_providers.cpp
    widgets/wx_aui_utils.cpp
    widgets/wx_busy_indicator.cpp
    widgets/wx_collapsible_pane.cpp
    widgets/wx_combobox.cpp
    widgets/wx_dataviewctrl.cpp
    widgets/wx_ellipsized_static_text.cpp
    widgets/wx_grid.cpp
    widgets/wx_html_report_box.cpp
    widgets/wx_html_report_panel.cpp
    widgets/wx_html_report_panel_base.cpp
    widgets/wx_listbox.cpp
    widgets/wx_panel.cpp
    widgets/wx_progress_reporters.cpp
    widgets/wx_splitter_window.cpp
    widgets/wx_treebook.cpp
    )

set( COMMON_DRAWING_SHEET_SRCS
    drawing_sheet/ds_data_item.cpp
    drawing_sheet/ds_data_model.cpp
    drawing_sheet/ds_data_model_io.cpp
    drawing_sheet/drawing_sheet_default_description.cpp
    drawing_sheet/ds_draw_item.cpp
    drawing_sheet/ds_proxy_undo_item.cpp
    drawing_sheet/ds_proxy_view_item.cpp
    drawing_sheet/drawing_sheet_parser.cpp
    )

set( COMMON_PREVIEW_ITEMS_SRCS
    preview_items/arc_assistant.cpp
    preview_items/arc_geom_manager.cpp
    preview_items/centreline_rect_item.cpp
    preview_items/draw_context.cpp
    preview_items/polygon_geom_manager.cpp
    preview_items/polygon_item.cpp
    preview_items/preview_utils.cpp
    preview_items/ruler_item.cpp
    preview_items/selection_area.cpp
    preview_items/simple_overlay_item.cpp
    preview_items/two_point_assistant.cpp
    )

set( PLOTTERS_CONTROL_SRCS
    plotters/plotter.cpp
    plotters/DXF_plotter.cpp
    plotters/GERBER_plotter.cpp
    plotters/HPGL_plotter.cpp
    plotters/PDF_plotter.cpp
    plotters/PS_plotter.cpp
    plotters/SVG_plotter.cpp
    plotters/common_plot_functions.cpp
    )

set( COMMON_IO_SRCS
    io/io_base.cpp
    io/io_utils.cpp

    # Altium
    io/altium/altium_binary_parser.cpp
    io/altium/altium_ascii_parser.cpp
    io/altium/altium_parser_utils.cpp
    io/altium/altium_props_utils.cpp

    # Cadstar
    io/cadstar/cadstar_archive_parser.cpp
    io/cadstar/cadstar_parts_lib_parser.cpp

    # Eagle
    io/eagle/eagle_parser.cpp

    # EasyEDA
    io/easyeda/easyeda_parser_base.cpp
    io/easyeda/easyeda_parser_structs.cpp

    # EasyEDA pro
    io/easyedapro/easyedapro_parser.cpp
    io/easyedapro/easyedapro_import_utils.cpp
    )

set( COMMON_IMPORT_GFX_SRCS
    import_gfx/graphics_import_mgr.cpp
    import_gfx/graphics_importer.cpp
    import_gfx/graphics_importer_buffer.cpp
    import_gfx/dxf_import_plugin.cpp
    import_gfx/svg_import_plugin.cpp
    )

set( COMMON_GIT_SRCS
    git/git_add_to_index_handler.cpp
    git/git_clone_handler.cpp
    git/git_commit_handler.cpp
    git/git_pull_handler.cpp
    git/git_push_handler.cpp
    git/git_remove_from_index_handler.cpp
    git/git_resolve_conflict_handler.cpp
    git/git_revert_handler.cpp
    git/git_sync_handler.cpp
    git/kicad_git_common.cpp
    git/kicad_git_errors.cpp
    )

set( COMMON_SRCS
    ${LIB_KICAD_SRCS}
    ${COMMON_ABOUT_DLG_SRCS}
    ${COMMON_DLG_SRCS}
    ${COMMON_WIDGET_SRCS}
    ${COMMON_DRAWING_SHEET_SRCS}
    ${COMMON_PREVIEW_ITEMS_SRCS}
    ${PLOTTERS_CONTROL_SRCS}
    ${COMMON_IO_SRCS}
	${FONT_SRCS}
    ${COMMON_IMPORT_GFX_SRCS}
    ${COMMON_GIT_SRCS}
    jobs/job_dispatcher.cpp
    background_jobs_monitor.cpp
    base_screen.cpp
    bin_mod.cpp
    bitmap.cpp
    bitmap_base.cpp
    bitmap_store.cpp
    board_printout.cpp
    cli_progress_reporter.cpp
    commit.cpp
    common.cpp
    dialog_shim.cpp
    dpi_scaling_common.cpp
    draw_panel_gal.cpp
    gal_display_options_common.cpp
    gr_text.cpp
    eda_base_frame.cpp
    eda_dde.cpp
    eda_doc.cpp
    eda_draw_frame.cpp
    eda_item.cpp
    eda_shape.cpp
    eda_text.cpp
    eda_tools.cpp
    env_paths.cpp
    env_vars.cpp
    executable_names.cpp
    filename_resolver.cpp
    file_history.cpp
    filter_reader.cpp
    footprint_filter.cpp
    footprint_info.cpp
    gbr_metadata.cpp
    gestfich.cpp
    gr_basic.cpp
    grid_tricks.cpp
    hotkey_store.cpp
    hotkeys_basic.cpp
    kiface_base.cpp
    kiway.cpp
    kiway_express.cpp
    kiway_holder.cpp
    kiway_player.cpp
    launch_ext.cpp
    lib_table_base.cpp
    lib_table_grid_tricks.cpp
    lib_tree_model.cpp
    lib_tree_model_adapter.cpp
    marker_base.cpp
    notifications_manager.cpp
    origin_transforms.cpp
    printout.cpp
    project.cpp
    ptree.cpp
    rc_item.cpp
    refdes_utils.cpp
    render_settings.cpp
    reporter.cpp
    scintilla_tricks.cpp
    search_stack.cpp
    searchhelpfilefullpath.cpp
    status_popup.cpp
    string_utf8_map.cpp
    stroke_params.cpp
    systemdirsappend.cpp
    template_fieldnames.cpp
    textentry_tricks.cpp
    title_block.cpp
    undo_redo_container.cpp
    validators.cpp
    drawing_sheet/ds_painter.cpp
    xnode.cpp
    view/wx_view_controls.cpp
    )

if( TRUE OR NOT USE_KIWAY_DLLS )
#if( NOT USE_KIWAY_DLLS )
    # We DO NOT want pgm_base.cpp linked into the KIFACE, only into the KIWAY.
    # Check the map files to verify eda_pgm.o not being linked in.
    list( APPEND COMMON_SRCS pgm_base.cpp )
    if( KICAD_USE_SENTRY )
        set_property(SOURCE pgm_base.cpp APPEND PROPERTY COMPILE_DEFINITIONS KICAD_SENTRY_DSN="${KICAD_SENTRY_DSN}")
    endif()
endif()

set( COMMON_SRCS
    ${COMMON_SRCS}

    origin_viewitem.cpp

    tool/action_manager.cpp
    tool/action_menu.cpp
    tool/action_toolbar.cpp
    tool/actions.cpp
    tool/common_control.cpp
    tool/common_tools.cpp
    tool/conditional_menu.cpp
    tool/edit_constraints.cpp
    tool/edit_points.cpp
    tool/editor_conditions.cpp
    tool/grid_helper.cpp
    tool/grid_menu.cpp
    tool/picker_tool.cpp
    tool/properties_tool.cpp
    tool/selection.cpp
    tool/selection_tool.cpp
    tool/selection_conditions.cpp
    tool/tool_action.cpp
    tool/tool_base.cpp
    tool/tool_dispatcher.cpp
    tool/tool_event.cpp
    tool/tools_holder.cpp
    tool/tool_interactive.cpp
    tool/tool_manager.cpp
    tool/tool_menu.cpp
    tool/zoom_menu.cpp
    tool/zoom_tool.cpp

    settings/app_settings.cpp
    settings/aui_settings.cpp
    settings/bom_settings.cpp
    settings/color_settings.cpp
    settings/cvpcb_settings.cpp
    settings/common_settings.cpp
    settings/grid_settings.cpp
    settings/json_settings.cpp
    settings/nested_settings.cpp
    settings/parameters.cpp
    settings/settings_manager.cpp
    settings/kicad_settings.cpp

    project/board_project_settings.cpp
    project/net_settings.cpp
    project/project_archiver.cpp
    project/project_file.cpp
    project/project_local_settings.cpp

    properties/color4d_variant.cpp
    properties/eda_angle_variant.cpp
    properties/pg_cell_renderer.cpp
    properties/pg_editors.cpp
    properties/pg_properties.cpp
    properties/property_mgr.cpp
    properties/std_optional_variants.cpp

    database/database_connection.cpp
    database/database_lib_settings.cpp

    http_lib/http_lib_connection.cpp
    http_lib/http_lib_settings.cpp

    )

add_library( common STATIC
    ${COMMON_SRCS}
    )

add_dependencies( common version_header )
add_dependencies( common compoundfilereader )  # used by altium_parser.cpp

target_link_libraries( common
    libcontext
    kimath
    kiplatform
    kicommon
    core
    fmt::fmt
    gal
    nanosvg
    dxflib_qcad
    tinyspline_lib
    scripting
    nlohmann_json
    pybind11::embed
    compoundfilereader
    Boost::headers
    # Database support needs these two
    nanodbc # for now; maybe hoist out of common
    Boost::locale
    ${wxWidgets_LIBRARIES}
    ${EXTRA_LIBS}
    # outline font support
   	${FREETYPE_LIBRARIES}
   	${HarfBuzz_LIBRARIES}
	${Fontconfig_LIBRARIES}
    )

if( KICAD_USE_SENTRY )
    target_link_libraries( common
        sentry )
endif()

target_include_directories( common
    PUBLIC
        .
        ${CMAKE_BINARY_DIR}
        $<TARGET_PROPERTY:argparse::argparse,INTERFACE_INCLUDE_DIRECTORIES>
    )

# text markup support
add_dependencies( common pegtl )
target_include_directories( common PUBLIC
    $<TARGET_PROPERTY:pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_include_directories( common SYSTEM PUBLIC
    $<TARGET_PROPERTY:nanodbc,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_include_directories( common PUBLIC
    $<TARGET_PROPERTY:magic_enum,INTERFACE_INCLUDE_DIRECTORIES>
    )


set( PCB_COMMON_SRCS
    fp_lib_table.cpp
    hash_eda.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_base_frame.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbexpr_evaluator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbexpr_functions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_commit.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_connected_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop_parameters.cpp     #needed by board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/router/pns_meander.cpp               #needed by board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_dimension.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_shape.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_group.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_marker.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/fix_board_shape.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netinfo_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netinfo_list.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pad.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_target.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_reference_image.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_field.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_text.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_textbox.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/project_pcb.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/board_stackup.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_track.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_generator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/collectors.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_algo.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_items.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_data.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/from_to_cache.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/convert_shape_list_to_polygon.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_engine.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_cache_generator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule_condition.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule_parser.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_test_provider.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint_editor_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/generators_mgr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/kicad_clipboard.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/kicad_netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/legacy_netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pad_custom_shape_functions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_draw_panel_gal.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/pcb_netlist.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_origin_transforms.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_painter.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_plot_params.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_screen.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_view.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbnew_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/ratsnest/ratsnest_data.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/ratsnest/ratsnest_view_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/sel_layer.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop_parameters.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop_utils.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone_settings.cpp

    # IO files
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/pcb_io.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/pcb_io_mgr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/kicad_legacy/pcb_io_kicad_legacy.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/eagle/pcb_io_eagle.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/geda/pcb_io_geda.cpp

    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_grid_helper.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_actions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_editor_conditions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_viewer_tools.cpp

    widgets/net_selector.cpp
)

# add -DPCBNEW to compilation of these PCBNEW sources
set_source_files_properties( ${PCB_COMMON_SRCS} PROPERTIES
    COMPILE_DEFINITIONS "PCBNEW"
    )

add_library( pcbcommon STATIC ${PCB_COMMON_SRCS} )

target_include_directories( pcbcommon PRIVATE
    )

target_link_libraries( pcbcommon PUBLIC
    core
    common
    delaunator
    kimath
    kiplatform
)

message( STATUS "Including 3Dconnexion SpaceMouse navigation support in pcbcommon" )
add_subdirectory( ../pcbnew/navlib ./navlib)
target_link_libraries( pcbcommon PUBLIC pcbnew_navlib)

add_dependencies( pcbcommon delaunator )

# The lemon grammar for the numeric evaluator
generate_lemon_grammar(
    common
    libeval
    libeval/numeric_evaluator.cpp
    libeval/grammar.lemon
    )

# The lemon grammar for the expression compiler
generate_lemon_grammar(
    common
    libeval_compiler
    libeval_compiler/libeval_compiler.cpp
    libeval_compiler/grammar.lemon
    )

# auto-generate stroke_params_lexer.h and stroke_params_keywords.cpp
# Called twice one for common and one for gal, to ensure the files are created
# on all devel tools ( Linux and msys2 )
# works on Linux:
make_lexer_export(
    kicommon
    stroke_params.keywords
    stroke_params_lexer.h
    stroke_params_keywords.cpp
    STROKEPARAMS_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate netlist_lexer.h and netlist_keywords.cpp
make_lexer_export(
    kicommon
    netlist.keywords
    netlist_lexer.h
    netlist_keywords.cpp
    NL_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate pcb_plot_params_lexer.h and pcb_plot_params_keywords.cpp
make_lexer(
    pcbcommon
    pcb_plot_params.keywords
    pcb_plot_params_lexer.h
    pcb_plot_params_keywords.cpp
    PCBPLOTPARAMS_T
    )

# auto-generate drc_rules_lexer.h and drc_rules_keywords.cpp
make_lexer_export(
    kicommon
    drc_rules.keywords
    drc_rules_lexer.h
    drc_rules_keywords.cpp
    DRCRULE_T
    KICOMMON_API
    kicommon.h
    )


# auto-generate pcbnew_sexpr.h and pcbnew_sexpr.cpp
make_lexer(
    pcbcommon
    pcb.keywords
    pcb_lexer.h
    pcb_keywords.cpp
    PCB_KEYS_T
    )

# auto-generate s-expression library table code.
make_lexer_export(
    kicommon
    lib_table.keywords
    lib_table_lexer.h
    lib_table_keywords.cpp
    LIB_TABLE_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate s-expression template fieldnames lexer and keywords.
make_lexer_export(
    kicommon
    template_fieldnames.keywords
    template_fieldnames_lexer.h
    template_fieldnames_keywords.cpp
    TFIELD_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate page layout reader s-expression page_layout_reader_lexer.h
# and title_block_reader_keywords.cpp.
make_lexer_export(
    kicommon
    drawing_sheet/drawing_sheet.keywords
    drawing_sheet/drawing_sheet_lexer.h
    drawing_sheet/drawing_sheet_keywords.cpp
    DRAWINGSHEET_T
    KICOMMON_API
    kicommon.h
    )

# This one gets made only when testing.
# to build it, first enable #define STAND_ALONE at top of dsnlexer.cpp
add_executable( dsntest EXCLUDE_FROM_ALL dsnlexer.cpp )
target_link_libraries( dsntest common ${wxWidgets_LIBRARIES} rt )
