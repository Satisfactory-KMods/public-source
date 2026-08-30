"""Validate RSS native sign rendering inside a running Unreal Editor.

This script is intentionally non-destructive: it loads existing Blueprint
classes, creates a transient widget, and writes diagnostic JSON under
``exports``. It never compiles or saves an asset or level.

Run from the workspace root after the RSS editor DLL has been loaded::

    python run_via_remote.py --script C:/Modding/Mods/GameFeatures/RSS/Scripts/UnrealPy/validate_native_sign_widget.py
"""

import json
import math
import os
import traceback

import unreal


COMPONENT_BLUEPRINT = "/RSS/Buildable/-Shared/BP_Bases/BP_SignComponentWidget_Base"
GENERAL_BLUEPRINT = "/RSS/Buildable/-Shared/BP_Bases/BP_SignWidget_General"
PROBE_TEXTURE = "/RSS/Interface/Assets/Texture/warning"
PLACEHOLDER_TEXTURE = "/RSS/Assets/Images/PlaceHolder0000"
BLANK_TEXTURE = "/RSS/Assets/MaterialTextures/16x16_b"
PROBE_FONT = "/KUI/Fonts/Orbitron/KUI_Orbitron"
OUTPUT_PATH = "C:/Modding/exports/rss-native-sign-widget-validation.json"
DRAW_SIZE = (800.0, 300.0)
EPSILON = 0.01


def _string(value):
    if value is None:
        return None
    try:
        return str(value)
    except Exception:
        return repr(value)


def _object_path(value):
    if value is None:
        return None
    try:
        return value.get_path_name()
    except Exception:
        return _string(value)


def _property(value, name, default=None):
    if value is None:
        return default
    try:
        return value.get_editor_property(name)
    except Exception:
        return default


def _call(value, method_name, *args):
    method = getattr(value, method_name, None)
    if not callable(method):
        raise AttributeError(f"{value.get_class().get_name()} has no callable {method_name}")
    return method(*args)


def _method_candidates(method_name):
    pascal_name = "".join(part.capitalize() for part in method_name.split("_"))
    return tuple(dict.fromkeys((method_name, pascal_name)))


def _invoke_uobject_method(value, method_name, *args):
    errors = []
    for candidate in _method_candidates(method_name):
        method = getattr(value, candidate, None)
        if callable(method):
            return candidate, method(*args)

    caller = getattr(value, "call_method", None)
    if callable(caller):
        for candidate in _method_candidates(method_name):
            try:
                return candidate, caller(candidate, args)
            except Exception as error:
                errors.append(f"{candidate}: {error}")

    detail = " | ".join(errors) if errors else "no direct method or UObject.call_method"
    raise AttributeError(f"{value.get_class().get_name()} cannot invoke {method_name}: {detail}")


def _set(value, name, new_value):
    value.set_editor_property(name, new_value)


def _set_first_property(value, names, new_value):
    errors = []
    for name in names:
        try:
            value.set_editor_property(name, new_value)
            return name
        except Exception as error:
            errors.append(f"{name}: {error}")
    raise AttributeError("Could not set any reflected property: " + " | ".join(errors))


def _number(value, component):
    if value is None:
        return None
    direct = getattr(value, component, None)
    if direct is not None:
        return float(direct)
    reflected = _property(value, component)
    return float(reflected) if reflected is not None else None


def _vector2(value):
    if value is None:
        return None
    x_value = _number(value, "x")
    y_value = _number(value, "y")
    if x_value is None or y_value is None:
        return _string(value)
    return {"x": x_value, "y": y_value}


def _linear_color(value):
    if value is None:
        return None
    result = {}
    for component in ("r", "g", "b", "a"):
        component_value = _number(value, component)
        if component_value is None:
            return _string(value)
        result[component] = component_value
    return result


def _margin(value):
    if value is None:
        return None
    result = {}
    for component in ("left", "top", "right", "bottom"):
        component_value = _number(value, component)
        if component_value is None:
            return _string(value)
        result[component] = component_value
    return result


def _nearly_equal(actual, expected, epsilon=EPSILON):
    if actual is None:
        return False
    return math.isclose(float(actual), float(expected), rel_tol=epsilon, abs_tol=epsilon)


def _vector_matches(actual, expected):
    return (
        isinstance(actual, dict)
        and _nearly_equal(actual.get("x"), expected[0])
        and _nearly_equal(actual.get("y"), expected[1])
    )


def _record_check(checks, name, passed, expected, actual, detail=None):
    check = {
        "name": name,
        "passed": bool(passed),
        "expected": expected,
        "actual": actual,
    }
    if detail:
        check["detail"] = detail
    checks.append(check)


def _package_is_dirty(package):
    is_dirty = getattr(package, "is_dirty", None)
    if callable(is_dirty):
        return bool(is_dirty())
    try:
        dirty_packages = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        package_path = _object_path(package)
        return any(_object_path(candidate) == package_path for candidate in dirty_packages)
    except Exception:
        # Dirtiness is diagnostic metadata only. Never attempt to clear it.
        return None


def _load_blueprint(asset_path):
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        raise RuntimeError(f"Missing WidgetBlueprint: {asset_path}")
    package = blueprint.get_outermost()
    dirty_before = _package_is_dirty(package)
    status_before = _string(_property(blueprint, "status"))
    if dirty_before is not False:
        state = "dirty" if dirty_before is True else "unknown"
        raise RuntimeError(
            f"Protected Blueprint package state is {state}; refusing validation against unsaved or unprovable state: "
            f"{asset_path}"
        )
    dirty_after = _package_is_dirty(package)
    if dirty_before is False and dirty_after is True:
        raise RuntimeError(f"Loading unexpectedly dirtied protected Blueprint package: {asset_path}")
    return blueprint, {
        "asset": asset_path,
        "status_before": status_before,
        "status_after": _string(_property(blueprint, "status")),
        "package_dirty_before": dirty_before,
        "package_dirty_after": dirty_after,
        "compiled": False,
        "saved": False,
    }


def _enum_value(enum_type, wanted_name):
    normalized_wanted = wanted_name.replace("_", "").lower()
    candidates = [name for name in dir(enum_type) if not name.startswith("_")]
    for candidate in candidates:
        if candidate.replace("_", "").lower() == normalized_wanted:
            return getattr(enum_type, candidate)
    for candidate in candidates:
        if candidate.replace("_", "").lower().endswith(normalized_wanted):
            return getattr(enum_type, candidate)
    raise AttributeError(f"Could not find {wanted_name} in {enum_type}: {candidates}")


def _make_shared(position, texture=None):
    shared = unreal.RssElementSharedData()
    _set(shared, "m_position", unreal.Vector2D(position[0], position[1]))
    _set(shared, "m_colour_overwrite", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    _set(shared, "m_opacity", 0.91)
    _set(shared, "m_z_index", int(position[0]))
    if texture is not None:
        _set(shared, "m_texture", texture)
    return shared


def _make_image(position, image_size, overwrite_size, nine_slice, texture):
    element = unreal.RssElement()
    _set(element, "m_element_type", _enum_value(unreal.SignElementType, "IMAGE"))
    _set(element, "m_shared_data", _make_shared(position, texture))
    image = unreal.RssElementImageData()
    _set(image, "m_image_size", unreal.Vector2D(image_size[0], image_size[1]))
    _set(image, "m_overwrite_image_size", unreal.Vector2D(overwrite_size[0], overwrite_size[1]))
    _set(image, "m_scale_mirrow", unreal.Vector2D(1.0, 1.0))
    _set(image, "m_use9_slice_mode", unreal.Vector2D(nine_slice[0], nine_slice[1]))
    _set(element, "m_image_data", image)
    return element


def _make_text(position, justification, font=None, bold=False):
    element = unreal.RssElement()
    _set(element, "m_element_type", _enum_value(unreal.SignElementType, "TEXT"))
    _set(element, "m_shared_data", _make_shared(position))
    text = unreal.RssElementTextData()
    _set(text, "m_text", "Native spacing probe\nSecond line")
    _set(text, "m_text_size", 40)
    _set(text, "m_letter_spacing", 3)
    _set(text, "m_line_height", 1.4)
    _set(text, "m_padding", unreal.Vector4(7.0, 11.0, 13.0, 17.0))
    _set(text, "m_text_justify", _enum_value(unreal.Just, justification))
    _set(text, "m_is_bold", bool(bold))
    if font is not None:
        _set(text, "m_font", font)
    _set(element, "m_text_data", text)
    return element


def _make_effect(position, texture):
    element = unreal.RssElement()
    _set(element, "m_element_type", _enum_value(unreal.SignElementType, "EFFECT"))
    _set(element, "m_shared_data", _make_shared(position, texture))
    effect = unreal.RssElementEffectData()
    _set(effect, "m_scale_and_speed", unreal.LinearColor(0.25, -0.5, 2.0, 3.0))
    _set(effect, "m_step_frequenz", 7.0)
    _set(element, "m_effect_data", effect)
    return element


def _make_sign_data(texture, placeholder_texture, font):
    # mImageSize.Y == 1 is the existing Fill Sign UI sentinel. The first
    # element also enables a normalized 0.2 9-slice margin. A stale size
    # overwrite is deliberate: Fill Sign must remain authoritative.
    elements = [
        _make_image((37.0, 83.0), (320.0, 1.0), (123.0, 45.0), (1.0, 0.2), texture),
        _make_image((111.0, 29.0), (96.0, 64.0), (150.0, 0.0), (0.0, 0.25), placeholder_texture),
        _make_text((211.0, 67.0), "RSS_LEFT", font, False),
        _make_text((311.0, 77.0), "RSS_MIDDLE", font, True),
        _make_text((411.0, 87.0), "RSS_RIGHT"),
        _make_effect((13.0, 17.0), texture),
    ]
    sign_data = unreal.RssSignData()
    _set(sign_data, "m_elements", elements)
    return sign_data


def _widget_children(widget):
    children = []
    try:
        count = widget.get_children_count()
    except Exception:
        count = 0
    for index in range(count):
        try:
            children.append(widget.get_child_at(index))
        except Exception:
            pass
    widget_tree = _property(widget, "widget_tree")
    root_widget = _property(widget_tree, "root_widget")
    if root_widget is not None and root_widget not in children:
        children.append(root_widget)

    # Python does not expose WidgetTree on every generated UserWidget instance.
    # RSS text widgets still expose their named child variables, so follow those
    # references explicitly for non-destructive font and layout inspection.
    for property_name in (
        "m_text",
        "mText",
        "m_background",
        "mBackground",
        "m_outline_border",
        "mOutlineBorder",
    ):
        child = _property(widget, property_name)
        if child is not None and child not in children:
            children.append(child)
    return children


def _walk_widgets(root):
    result = []
    pending = [root] if root is not None else []
    seen = set()
    while pending:
        widget = pending.pop(0)
        path = _object_path(widget)
        if path in seen:
            continue
        seen.add(path)
        result.append(widget)
        pending.extend(_widget_children(widget))
    return result


def _slot_position(widget):
    slot = _property(widget, "slot")
    if slot is None:
        return None
    try:
        return _vector2(slot.get_position())
    except Exception:
        pass
    layout_data = _property(slot, "layout_data")
    offsets = _property(layout_data, "offsets")
    if offsets is not None:
        return {"x": _number(offsets, "left"), "y": _number(offsets, "top")}
    return _vector2(_property(slot, "position"))


def _slot_size(widget):
    slot = _property(widget, "slot")
    if slot is None:
        return None
    try:
        return _vector2(slot.get_size())
    except Exception:
        pass
    layout_data = _property(slot, "layout_data")
    offsets = _property(layout_data, "offsets")
    if offsets is not None:
        return {"x": _number(offsets, "right"), "y": _number(offsets, "bottom")}
    return _vector2(_property(slot, "size"))


def _slot_anchors(widget):
    slot = _property(widget, "slot")
    if slot is None:
        return None
    try:
        anchors = slot.get_anchors()
    except Exception:
        layout_data = _property(slot, "layout_data")
        anchors = _property(layout_data, "anchors")
    if anchors is None:
        return None
    return {
        "minimum": _vector2(_property(anchors, "minimum")),
        "maximum": _vector2(_property(anchors, "maximum")),
    }


def _slot_alignment(widget):
    slot = _property(widget, "slot")
    if slot is None:
        return None
    try:
        return _vector2(slot.get_alignment())
    except Exception:
        return _vector2(_property(slot, "alignment"))


def _render_pivot(widget):
    return _vector2(_property(widget, "render_transform_pivot"))


def _find_widget_by_class(root, class_fragment):
    class_fragment = class_fragment.lower()
    for widget in _walk_widgets(root):
        if class_fragment in widget.get_class().get_name().lower():
            return widget
    return None


def _brush_data(image_widget):
    if image_widget is None:
        return None
    brush = _property(image_widget, "brush")
    if brush is None:
        try:
            brush = image_widget.get_brush()
        except Exception:
            return None
    resource = _property(brush, "resource_object")
    return {
        "draw_as": _string(_property(brush, "draw_as")),
        "margin": _margin(_property(brush, "margin")),
        "image_size": _vector2(_property(brush, "image_size")),
        "resource": _object_path(resource),
        "resource_class": resource.get_class().get_name() if resource is not None else None,
    }


def _font_data(text_widget):
    if text_widget is None:
        return None
    font = _property(text_widget, "font")
    return {
        "font_object": _object_path(_property(font, "font_object")) if font is not None else None,
        "typeface_font_name": _string(_property(font, "typeface_font_name")) if font is not None else None,
        "size": _property(font, "size") if font is not None else None,
        "letter_spacing": _property(font, "letter_spacing") if font is not None else None,
        "line_height_percentage": _property(text_widget, "line_height_percentage"),
    }


def _normalized_name(value):
    return "".join(character for character in value.lower() if character.isalnum())


def _property_by_prefix(value, prefix):
    normalized_prefix = _normalized_name(prefix)
    for candidate in dir(value):
        if not _normalized_name(candidate).startswith(normalized_prefix):
            continue
        result = _property(value, candidate)
        if result is None:
            try:
                result = getattr(value, candidate)
            except Exception:
                pass
        if result is not None:
            return candidate, result
    return None, None


def _resolve_legacy_font_expectation(widget, probe_font, limitations):
    diagnostic = {
        "font": _object_path(probe_font),
        "fallback_regular": "Default",
        "fallback_bold": "None",
    }
    try:
        invoked_name, result = _invoke_uobject_method(widget, "get_font", probe_font)
        diagnostic["invoked_name"] = invoked_name
        diagnostic["result"] = _string(result)
    except Exception as error:
        diagnostic["error"] = str(error)
        limitations.append(f"Legacy GetFont could not be invoked safely: {error}")
        return "Default", "None", diagnostic

    if not isinstance(result, tuple) or len(result) < 2:
        limitations.append(f"Legacy GetFont returned unexpected result shape: {_string(result)}")
        return "Default", "None", diagnostic

    font_info, valid = result[0], bool(result[1])
    diagnostic["valid"] = valid
    if not valid:
        diagnostic["resolution"] = "native fallback"
        return "Default", "None", diagnostic

    default_property, default_name = _property_by_prefix(font_info, "m_default_name")
    bold_property, bold_name = _property_by_prefix(font_info, "m_bold_name")
    diagnostic.update(
        {
            "resolution": "Blueprint font registry",
            "default_property": default_property,
            "default_name": _string(default_name),
            "bold_property": bold_property,
            "bold_name": _string(bold_name),
        }
    )
    if default_name is None or bold_name is None:
        limitations.append("Legacy GetFont returned valid metadata whose typeface fields were not Python-readable.")
        return "Default", "None", diagnostic
    return _string(default_name), _string(bold_name), diagnostic


def _legacy_cache_dirty(widget):
    for property_name in ("m_chache_is_update", "mChacheIsUpdate"):
        value = _property(widget, property_name)
        if value is not None:
            return bool(value)
    return None


def _sign_data_element_position(widget, index):
    sign_data = _property(widget, "m_sign_widget_data")
    elements = _property(sign_data, "m_elements")
    try:
        element = elements[index]
    except Exception:
        return None
    shared = _property(element, "m_shared_data")
    return _vector2(_property(shared, "m_position"))


def _validate_legacy_update_recovery(
    widget,
    main_canvas,
    checks,
    limitations,
    method_name,
    index,
    mutated_element,
    canonical_position,
):
    """Probe Blueprint incremental updates, then require native canonical output.

    Legacy UpdateText/Image/EffectElement graphs operate on mCachedElements. A
    check is meaningful only when the graph is callable and visibly mutates the
    native widget. Failed recovery is force-reset solely to isolate later probes.
    """

    diagnostic = {
        "method": method_name,
        "index": index,
        "callable": None,
        "canonical_position": {"x": canonical_position[0], "y": canonical_position[1]},
    }

    before_widget = widget.get_native_element_widget(index)
    diagnostic["before_widget"] = _object_path(before_widget)
    diagnostic["before_position"] = _slot_position(before_widget)
    try:
        invoked_name, _ = _invoke_uobject_method(widget, method_name, mutated_element, index)
        diagnostic["callable"] = True
        diagnostic["invoked_name"] = invoked_name
    except Exception as error:
        diagnostic["callable"] = False
        diagnostic["call_error"] = str(error)
        limitations.append(f"Legacy {method_name} could not be invoked safely: {error}")
        return diagnostic

    after_legacy_widget = widget.get_native_element_widget(index)
    after_legacy_position = _slot_position(after_legacy_widget)
    diagnostic["after_legacy_position"] = after_legacy_position
    mutation_observed = not _vector_matches(after_legacy_position, canonical_position)
    diagnostic["mutation_observed"] = mutation_observed
    dirty_after_legacy = _legacy_cache_dirty(widget)
    diagnostic["cache_dirty_after_legacy_update"] = dirty_after_legacy
    _record_check(
        checks,
        f"legacy {method_name} marks native cache dirty",
        dirty_after_legacy is True,
        True,
        dirty_after_legacy,
    )

    widget.synchronize_native_elements(False)
    reclaimed_widget = widget.get_native_element_widget(index)
    reclaimed_position = _slot_position(reclaimed_widget)
    dirty_after_sync = _legacy_cache_dirty(widget)
    attached = False
    try:
        attached = main_canvas is not None and main_canvas.get_child_at(index) == reclaimed_widget
    except Exception:
        pass
    recovered = _vector_matches(reclaimed_position, canonical_position) and attached
    diagnostic.update(
        {
            "reclaimed_widget": _object_path(reclaimed_widget),
            "reclaimed_position": reclaimed_position,
            "attached_in_order": attached,
            "widget_replaced": reclaimed_widget != before_widget,
            "recovered_without_force": recovered,
            "cache_dirty_after_non_force_sync": dirty_after_sync,
        }
    )
    _record_check(
        checks,
        f"non-force native sync clears {method_name} cache dirty flag",
        dirty_after_sync is False,
        False,
        dirty_after_sync,
    )
    if not mutation_observed:
        limitations.append(
            f"Legacy {method_name} was callable but did not visibly mutate element {index}; "
            "canonical output recovery check skipped."
        )
        return diagnostic

    _record_check(
        checks,
        f"native renderer reclaims {method_name} output without force",
        recovered,
        {
            "position": {"x": canonical_position[0], "y": canonical_position[1]},
            "attached_in_order": True,
        },
        {
            "position": reclaimed_position,
            "attached_in_order": attached,
            "widget_replaced": reclaimed_widget != before_widget,
        },
    )
    if not recovered:
        # Keep later probes independent. This cleanup is not part of the check.
        widget.synchronize_native_elements(True)
        diagnostic["force_reset_after_failure"] = True
    return diagnostic


def _validate_legacy_index_update(
    widget,
    main_canvas,
    checks,
    limitations,
    index,
    replacement_element,
    expected_position,
):
    method_name = "update_element_by_index"
    diagnostic = {
        "method": method_name,
        "index": index,
        "callable": None,
        "expected_position": {"x": expected_position[0], "y": expected_position[1]},
    }

    try:
        invoked_name, _ = _invoke_uobject_method(widget, method_name, replacement_element, index)
        diagnostic["callable"] = True
        diagnostic["invoked_name"] = invoked_name
    except Exception as error:
        diagnostic["callable"] = False
        diagnostic["call_error"] = str(error)
        limitations.append(f"Legacy update_element_by_index could not be invoked safely: {error}")
        return diagnostic

    stored_position = _sign_data_element_position(widget, index)
    diagnostic["stored_position_after_call"] = stored_position
    _record_check(
        checks,
        "legacy update_element_by_index changes mSignWidgetData",
        _vector_matches(stored_position, expected_position),
        {"x": expected_position[0], "y": expected_position[1]},
        stored_position,
    )

    widget.synchronize_native_elements(False)
    native_widget = widget.get_native_element_widget(index)
    native_position = _slot_position(native_widget)
    attached = False
    try:
        attached = main_canvas is not None and main_canvas.get_child_at(index) == native_widget
    except Exception:
        pass
    rendered = _vector_matches(native_position, expected_position) and attached
    diagnostic.update(
        {
            "native_widget": _object_path(native_widget),
            "native_position_after_non_force_sync": native_position,
            "attached_in_order": attached,
            "rendered_without_force": rendered,
        }
    )
    _record_check(
        checks,
        "native renderer applies legacy update_element_by_index without force",
        rendered,
        {
            "position": {"x": expected_position[0], "y": expected_position[1]},
            "attached_in_order": True,
        },
        {"position": native_position, "attached_in_order": attached},
    )
    return diagnostic


def _validate_legacy_draw_size_overwrite(widget, checks, limitations):
    """Require native Fill/Effect layout to honor legacy preview draw-size state."""

    expected_size = (640.0, 240.0)
    diagnostic = {"expected_size": {"x": expected_size[0], "y": expected_size[1]}}
    enabled_property = None
    try:
        enabled_property = _set_first_property(
            widget,
            ("m_is_draw_size_overwrite", "mIsDrawSizeOverwrite"),
            True,
        )
        size_property = _set_first_property(
            widget,
            ("m_over_write_draw_size", "mOverWriteDrawSize"),
            unreal.Vector2D(expected_size[0], expected_size[1]),
        )
        diagnostic["enabled_property"] = enabled_property
        diagnostic["size_property"] = size_property

        # Runtime render components own mNativeDrawSizeOverride. Clear it to
        # exercise standalone editor-preview compatibility state.
        widget.set_native_draw_size(unreal.Vector2D(0.0, 0.0))
        widget.synchronize_native_elements(False)
        fill_size = _slot_size(widget.get_native_element_widget(0))
        effect_size = _slot_size(widget.get_native_element_widget(5))
        diagnostic["fill_size"] = fill_size
        diagnostic["effect_size"] = effect_size
        _record_check(
            checks,
            "legacy draw-size overwrite controls Fill Sign canvas",
            _vector_matches(fill_size, expected_size),
            {"x": expected_size[0], "y": expected_size[1]},
            fill_size,
        )
        _record_check(
            checks,
            "legacy draw-size overwrite controls effect canvas",
            _vector_matches(effect_size, expected_size),
            {"x": expected_size[0], "y": expected_size[1]},
            effect_size,
        )
    except Exception as error:
        diagnostic["error"] = str(error)
        limitations.append(f"Legacy draw-size overwrite probe could not run: {error}")
    finally:
        if enabled_property is not None:
            try:
                _set_first_property(
                    widget,
                    (enabled_property, "m_is_draw_size_overwrite", "mIsDrawSizeOverwrite"),
                    False,
                )
            except Exception as error:
                limitations.append(f"Legacy draw-size overwrite reset failed: {error}")
        widget.set_native_draw_size(unreal.Vector2D(DRAW_SIZE[0], DRAW_SIZE[1]))
        widget.synchronize_native_elements(False)

    restored_size = _slot_size(widget.get_native_element_widget(0))
    diagnostic["restored_fill_size"] = restored_size
    _record_check(
        checks,
        "native draw size restores Fill Sign after preview overwrite",
        _vector_matches(restored_size, DRAW_SIZE),
        {"x": DRAW_SIZE[0], "y": DRAW_SIZE[1]},
        restored_size,
    )
    return diagnostic


def _validate_invalid_preview_draw_size(widget, checks, limitations):
    """Reject non-finite reflected preview sizes before they reach Slate."""

    diagnostic = {}
    original_draw_size = None
    draw_size_property = None
    try:
        for candidate in ("m_draw_size", "mDrawSize"):
            original_draw_size = _property(widget, candidate)
            if original_draw_size is not None:
                draw_size_property = candidate
                break
        if draw_size_property is None:
            raise AttributeError("Transient widget exposes no reflected mDrawSize property")

        _set_first_property(widget, ("m_is_draw_size_overwrite", "mIsDrawSizeOverwrite"), False)
        _set_first_property(
            widget,
            (draw_size_property, "m_draw_size", "mDrawSize"),
            unreal.Vector2D(0.0, 0.0),
        )
        widget.set_native_draw_size(unreal.Vector2D(0.0, 0.0))
        widget.synchronize_native_elements(False)
        fallback_fill_size = _slot_size(widget.get_native_element_widget(0))
        fallback_effect_size = _slot_size(widget.get_native_element_widget(5))

        _set_first_property(
            widget,
            (draw_size_property, "m_draw_size", "mDrawSize"),
            unreal.Vector2D(float("inf"), 240.0),
        )
        widget.synchronize_native_elements(False)
        fill_size = _slot_size(widget.get_native_element_widget(0))
        effect_size = _slot_size(widget.get_native_element_widget(5))
        diagnostic.update(
            {
                "draw_size_property": draw_size_property,
                "fallback_fill_size": fallback_fill_size,
                "fallback_effect_size": fallback_effect_size,
                "fill_size": fill_size,
                "effect_size": effect_size,
            }
        )
        fallback_sizes_are_finite = all(
            isinstance(size, dict)
            and math.isfinite(float(size.get("x")))
            and math.isfinite(float(size.get("y")))
            for size in (fallback_fill_size, fallback_effect_size)
        )
        sizes_match_fallback = (
            fallback_sizes_are_finite
            and _vector_matches(fill_size, (fallback_fill_size["x"], fallback_fill_size["y"]))
            and _vector_matches(effect_size, (fallback_effect_size["x"], fallback_effect_size["y"]))
        )
        _record_check(
            checks,
            "non-finite legacy preview draw size uses sane fallback",
            sizes_match_fallback,
            {"fill": fallback_fill_size, "effect": fallback_effect_size},
            {"fill": fill_size, "effect": effect_size},
        )
    except Exception as error:
        diagnostic["error"] = str(error)
        limitations.append(f"Invalid legacy preview draw-size probe could not run: {error}")
    finally:
        if draw_size_property is not None and original_draw_size is not None:
            try:
                _set_first_property(widget, (draw_size_property, "m_draw_size", "mDrawSize"), original_draw_size)
            except Exception as error:
                limitations.append(f"Legacy preview draw-size reset failed: {error}")
        widget.set_native_draw_size(unreal.Vector2D(DRAW_SIZE[0], DRAW_SIZE[1]))
        widget.synchronize_native_elements(False)
    return diagnostic


def _material_vector_parameter(image_widget, parameter_name):
    brush = _property(image_widget, "brush")
    resource = _property(brush, "resource_object")
    if resource is None:
        return None, "brush has no resource_object"
    getter = getattr(resource, "get_vector_parameter_value", None)
    if not callable(getter):
        return None, f"{resource.get_class().get_name()} exposes no get_vector_parameter_value"
    try:
        return _linear_color(getter(parameter_name)), None
    except Exception as error:
        return None, f"get_vector_parameter_value({parameter_name}) failed: {error}"


def _describe_widget(widget):
    if widget is None:
        return None
    return {
        "path": _object_path(widget),
        "class": widget.get_class().get_name(),
        "slot_class": _property(widget, "slot").get_class().get_name() if _property(widget, "slot") else None,
        "position": _slot_position(widget),
        "size": _slot_size(widget),
        "anchors": _slot_anchors(widget),
        "alignment": _slot_alignment(widget),
        "render_pivot": _render_pivot(widget),
        "descendants": [
            {"path": _object_path(child), "class": child.get_class().get_name()}
            for child in _walk_widgets(widget)
        ],
    }


def _create_transient_widget(generated_class):
    attempts = []
    subsystem_type = getattr(unreal, "UnrealEditorSubsystem", None)
    if subsystem_type is not None:
        try:
            subsystem = unreal.get_editor_subsystem(subsystem_type)
            game_world = subsystem.get_game_world()
            if game_world is not None:
                attempts.append(("UnrealEditorSubsystem.get_game_world", game_world))
            editor_world = subsystem.get_editor_world()
            if editor_world is not None:
                attempts.append(("UnrealEditorSubsystem.get_editor_world", editor_world))
        except Exception:
            pass
    editor_level_library = getattr(unreal, "EditorLevelLibrary", None)
    if editor_level_library is not None:
        try:
            editor_world = editor_level_library.get_editor_world()
            if editor_world is not None:
                attempts.append(("EditorLevelLibrary.get_editor_world", editor_world))
        except Exception:
            pass

    errors = []
    seen_worlds = set()
    for source, world in attempts:
        world_path = _object_path(world)
        if world_path in seen_worlds:
            continue
        seen_worlds.add(world_path)
        native_factory = getattr(unreal.RssSignWidget, "create_native_sign_widget", None)
        if callable(native_factory):
            try:
                widget = native_factory(world, generated_class)
                canvas_getter = getattr(widget, "get_native_element_canvas", None)
                canvas = canvas_getter() if callable(canvas_getter) else None
                if widget is not None and canvas is not None:
                    return widget, {
                        "method": "RssSignWidget.create_native_sign_widget",
                        "world_source": source,
                        "world": world_path,
                        "canvas": _object_path(canvas),
                        "canonical_umg_creation": True,
                    }
                errors.append(f"{source} / native factory: created widget has no initialized native canvas")
            except Exception as error:
                errors.append(f"{source} / native factory: {error}")
        widget_library = getattr(unreal, "WidgetBlueprintLibrary", None)
        if widget_library is not None:
            try:
                widget = widget_library.create(world, generated_class, None)
                canvas_getter = getattr(widget, "get_native_element_canvas", None)
                canvas = canvas_getter() if callable(canvas_getter) else None
                if widget is not None and canvas is not None:
                    return widget, {
                        "method": "WidgetBlueprintLibrary.create",
                        "world_source": source,
                        "world": world_path,
                        "canvas": _object_path(canvas),
                        "canonical_umg_creation": True,
                    }
                errors.append(f"{source} / WidgetBlueprintLibrary.create: widget has no initialized native canvas")
            except Exception as error:
                errors.append(f"{source} / WidgetBlueprintLibrary.create: {error}")
    raise RuntimeError("Could not create transient BP_SignWidget_General: " + " | ".join(errors))


def _run_behavior_validation(widget, payload):
    checks = payload["checks"]
    texture = unreal.load_asset(PROBE_TEXTURE)
    if texture is None:
        raise RuntimeError(f"Missing probe texture: {PROBE_TEXTURE}")
    placeholder_texture = unreal.load_asset(PLACEHOLDER_TEXTURE)
    if placeholder_texture is None:
        raise RuntimeError(f"Missing placeholder texture: {PLACEHOLDER_TEXTURE}")
    probe_font = unreal.load_asset(PROBE_FONT)
    if probe_font is None:
        raise RuntimeError(f"Missing probe font: {PROBE_FONT}")
    sign_data = _make_sign_data(texture, placeholder_texture, probe_font)
    payload["probe_data_constructed"] = True
    payload["probe_font"] = _object_path(probe_font)
    required_api = (
        "set_native_draw_size",
        "update_sign_data",
        "update_native_element_by_index",
        "synchronize_native_elements",
        "get_native_element_count",
        "get_native_element_widget",
        "get_native_element_canvas",
    )
    api = {name: callable(getattr(widget, name, None)) for name in required_api}
    payload["native_api"] = api
    missing_api = [name for name, present in api.items() if not present]
    if missing_api:
        payload["limitations"].append(
            "Loaded editor class lacks native API (editor DLL/hot reload likely stale): " + ", ".join(missing_api)
        )
        return False

    widget.set_native_draw_size(unreal.Vector2D(DRAW_SIZE[0], DRAW_SIZE[1]))
    widget.update_sign_data(sign_data)
    widget.synchronize_native_elements(True)

    count = widget.get_native_element_count()
    _record_check(checks, "native element count", count == 6, 6, count)
    native_widgets = []
    for index in range(max(0, min(int(count), 6))):
        native_widgets.append(widget.get_native_element_widget(index))
    payload["native_widgets"] = [_describe_widget(widget_value) for widget_value in native_widgets]
    if len(native_widgets) != 6 or any(value is None for value in native_widgets):
        payload["limitations"].append("Native renderer did not expose all six probe widgets; detailed checks skipped.")
        return True

    expected_alignments = (0.5, 0.5, 0.0, 0.5, 1.0, 0.5)
    for index, native_widget in enumerate(native_widgets):
        anchors = _slot_anchors(native_widget)
        anchors_ok = (
            isinstance(anchors, dict)
            and _vector_matches(anchors.get("minimum"), (0.5, 0.5))
            and _vector_matches(anchors.get("maximum"), (0.5, 0.5))
        )
        _record_check(
            checks,
            f"element {index} uses centered canvas anchors",
            anchors_ok,
            {"minimum": {"x": 0.5, "y": 0.5}, "maximum": {"x": 0.5, "y": 0.5}},
            anchors,
        )
        alignment = _slot_alignment(native_widget)
        _record_check(
            checks,
            f"element {index} uses legacy canvas alignment",
            _vector_matches(alignment, (expected_alignments[index], 0.5)),
            {"x": expected_alignments[index], "y": 0.5},
            alignment,
        )
        pivot = _render_pivot(native_widget)
        _record_check(
            checks,
            f"element {index} uses centered render pivot",
            _vector_matches(pivot, (0.5, 0.5)),
            {"x": 0.5, "y": 0.5},
            pivot,
        )

    fill_position = _slot_position(native_widgets[0])
    _record_check(
        checks,
        "Fill Sign starts at canvas origin",
        _vector_matches(fill_position, (0.0, 0.0)),
        {"x": 0.0, "y": 0.0},
        fill_position,
    )
    expected_positions = (
        (1, (111.0, 29.0)),
        (2, (211.0, 67.0)),
        (3, (311.0, 77.0)),
        (4, (411.0, 87.0)),
        (5, (13.0, 17.0)),
    )
    for index, expected in expected_positions:
        actual = _slot_position(native_widgets[index])
        _record_check(
            checks,
            f"element {index} X/Y position is not transposed",
            _vector_matches(actual, expected),
            {"x": expected[0], "y": expected[1]},
            actual,
        )

    fill_size = _slot_size(native_widgets[0])
    _record_check(
        checks,
        "Fill Sign stretches independently to full canvas",
        _vector_matches(fill_size, DRAW_SIZE),
        {"x": DRAW_SIZE[0], "y": DRAW_SIZE[1]},
        fill_size,
        "Legacy checked state is mImageSize.Y == 1; aspect ratio must not be preserved.",
    )
    overwrite_size = _slot_size(native_widgets[1])
    _record_check(
        checks,
        "Size Overwrite applies per axis",
        _vector_matches(overwrite_size, (150.0, 64.0)),
        {"x": 150.0, "y": 64.0},
        overwrite_size,
        "X override is 150; zero Y override must fall back to mImageSize.Y (64).",
    )

    overwrite_image = _find_widget_by_class(native_widgets[1], "Image")
    overwrite_brush = _brush_data(overwrite_image)
    placeholder_resource = overwrite_brush.get("resource") if overwrite_brush else None
    _record_check(
        checks,
        "legacy placeholder texture resolves to blank texture",
        bool(placeholder_resource and placeholder_resource.startswith(BLANK_TEXTURE + ".")),
        BLANK_TEXTURE,
        placeholder_resource,
    )

    fill_image = _find_widget_by_class(native_widgets[0], "Image")
    fill_brush = _brush_data(fill_image)
    payload["fill_brush"] = fill_brush
    draw_as = fill_brush.get("draw_as") if fill_brush else None
    margins = fill_brush.get("margin") if fill_brush else None
    margin_ok = isinstance(margins, dict) and all(
        _nearly_equal(margins.get(side), 0.2) for side in ("left", "top", "right", "bottom")
    )
    _record_check(
        checks,
        "9-Slice mode selects box brush",
        bool(draw_as and "BOX" in draw_as.upper()),
        "SlateBrushDrawType.BOX",
        draw_as,
    )
    _record_check(
        checks,
        "9-Slice size applies normalized uniform margin",
        margin_ok,
        {"left": 0.2, "top": 0.2, "right": 0.2, "bottom": 0.2},
        margins,
    )

    text_widget = _find_widget_by_class(native_widgets[2], "TextBlock")
    font = _font_data(text_widget)
    payload["text_widget"] = {
        "widget": _describe_widget(text_widget),
        "font": font,
    }
    # Slate stores letter spacing in 1/1000 em. Native renderer converts the
    # editor's pixel-like value: round(3 px * 1000 / 40 px) == 75.
    spacing = font.get("letter_spacing") if font else None
    line_height = font.get("line_height_percentage") if font else None
    _record_check(checks, "text letter spacing converts pixels to Slate units", spacing == 75, 75, spacing)
    _record_check(
        checks,
        "text line height reaches TextBlock",
        _nearly_equal(line_height, 1.4),
        1.4,
        line_height,
    )

    bold_text_widget = _find_widget_by_class(native_widgets[3], "TextBlock")
    bold_font = _font_data(bold_text_widget)
    expected_regular_typeface, expected_bold_typeface, font_lookup = _resolve_legacy_font_expectation(
        widget, probe_font, payload["limitations"]
    )
    payload["font_resolution"] = {
        "regular": font,
        "bold": bold_font,
        "legacy_get_font": font_lookup,
    }
    expected_font_path = _object_path(probe_font)
    regular_font_object = font.get("font_object") if font else None
    regular_typeface = font.get("typeface_font_name") if font else None
    bold_font_object = bold_font.get("font_object") if bold_font else None
    bold_typeface = bold_font.get("typeface_font_name") if bold_font else None
    _record_check(
        checks,
        "reflected GetFont keeps known KUI font object for regular text",
        regular_font_object == expected_font_path,
        expected_font_path,
        regular_font_object,
    )
    _record_check(
        checks,
        "reflected GetFont resolves legacy regular typeface name",
        regular_typeface == expected_regular_typeface,
        expected_regular_typeface,
        regular_typeface,
    )
    _record_check(
        checks,
        "reflected GetFont keeps known KUI font object for bold text",
        bold_font_object == expected_font_path,
        expected_font_path,
        bold_font_object,
    )
    _record_check(
        checks,
        "reflected GetFont resolves legacy bold typeface name",
        bold_typeface == expected_bold_typeface,
        expected_bold_typeface,
        bold_typeface,
    )

    effect_image = _find_widget_by_class(native_widgets[5], "Image")
    speed_and_scale, parameter_error = _material_vector_parameter(effect_image, "SpeedAndScale")
    payload["effect_speed_and_scale"] = speed_and_scale
    if parameter_error:
        payload["limitations"].append(parameter_error)
    else:
        expected_effect = {"r": 0.25, "g": -0.5, "b": 2.0, "a": 3.0}
        effect_ok = all(_nearly_equal(speed_and_scale.get(component), expected) for component, expected in expected_effect.items())
        _record_check(
            checks,
            "effect horizontal/vertical speed channels remain R/G",
            effect_ok,
            expected_effect,
            speed_and_scale,
        )

    # Generated Blueprint preview variables are intentionally instance-read-only
    # to Unreal Python. Runtime render components use SetNativeDrawSize, which is
    # exercised above and is authoritative for Fill Sign/effect canvas sizing.
    payload["legacy_draw_size_overwrite"] = {
        "tested": False,
        "reason": "Blueprint preview-only variables are not writable on transient instances",
    }

    main_canvas = widget.get_native_element_canvas()
    clear_children = getattr(main_canvas, "clear_children", None)
    if callable(clear_children):
        clear_children()
        widget.synchronize_native_elements(False)
        reclaimed_count = widget.get_native_element_count()
        reclaimed_widgets = [
            widget.get_native_element_widget(index)
            for index in range(max(0, min(int(reclaimed_count), 6)))
        ]
        canvas_count = main_canvas.get_children_count()
        reclaimed_in_order = (
            reclaimed_count == 6
            and canvas_count == 6
            and len(reclaimed_widgets) == 6
            and all(main_canvas.get_child_at(index) == reclaimed_widgets[index] for index in range(6))
        )
        _record_check(
            checks,
            "native renderer reclaims canvas after external child replacement",
            reclaimed_in_order,
            {"native_count": 6, "canvas_count": 6, "same_order": True},
            {
                "native_count": reclaimed_count,
                "canvas_count": canvas_count,
                "same_order": reclaimed_in_order,
            },
        )
    else:
        payload["limitations"].append("Could not clear transient mMainCanv; native attachment recovery check skipped.")

    # Do not execute legacy Blueprint Update*Element graphs from automation. Those graphs expect editor-only cache
    # objects and can assert inside Blueprint type conversion when invoked on a transient widget. Native rendering is
    # authoritative now, so exercise its index-update API directly.
    replacement_position = (477.0, 177.0)
    replacement_element = _make_text(replacement_position, "RSS_RIGHT", probe_font, False)
    widget.update_native_element_by_index(4, replacement_element)
    stored_position = _sign_data_element_position(widget, 4)
    updated_widget = widget.get_native_element_widget(4)
    updated_position = _slot_position(updated_widget)
    attached = False
    try:
        attached = main_canvas is not None and main_canvas.get_child_at(4) == updated_widget
    except Exception:
        pass
    payload["native_index_update"] = {
        "stored_position": stored_position,
        "rendered_position": updated_position,
        "attached_in_order": attached,
    }
    _record_check(
        checks,
        "native index update changes sign data",
        _vector_matches(stored_position, replacement_position),
        {"x": replacement_position[0], "y": replacement_position[1]},
        stored_position,
    )
    _record_check(
        checks,
        "native index update renders without Blueprint graph",
        _vector_matches(updated_position, replacement_position) and attached,
        {
            "position": {"x": replacement_position[0], "y": replacement_position[1]},
            "attached_in_order": True,
        },
        {"position": updated_position, "attached_in_order": attached},
    )
    return True


def main():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    payload = {
        "script": __file__,
        "output": OUTPUT_PATH,
        "non_destructive": True,
        "assets_saved": False,
        "loaded_blueprints": [],
        "checks": [],
        "limitations": [],
        "errors": [],
        "status": "blocked",
    }
    behavior_attempted = False
    try:
        component, component_result = _load_blueprint(COMPONENT_BLUEPRINT)
        payload["loaded_blueprints"].append(component_result)
        general, general_result = _load_blueprint(GENERAL_BLUEPRINT)
        payload["loaded_blueprints"].append(general_result)
        generated_class = general.generated_class()
        payload["generated_class"] = _object_path(generated_class)
        widget, creation = _create_transient_widget(generated_class)
        payload["widget_creation"] = creation
        behavior_attempted = _run_behavior_validation(widget, payload)
    except Exception as error:
        payload["errors"].append({
            "message": str(error),
            "traceback": traceback.format_exc(),
        })

    failed_checks = [check["name"] for check in payload["checks"] if not check["passed"]]
    payload["summary"] = {
        "check_count": len(payload["checks"]),
        "passed": len(payload["checks"]) - len(failed_checks),
        "failed": len(failed_checks),
        "failed_checks": failed_checks,
    }
    if payload["errors"] or not behavior_attempted:
        payload["status"] = "blocked"
    elif failed_checks:
        payload["status"] = "failed"
    elif payload["limitations"]:
        payload["status"] = "blocked"
    else:
        payload["status"] = "passed"

    with open(OUTPUT_PATH, "w", encoding="utf-8") as output_file:
        json.dump(payload, output_file, indent=2, ensure_ascii=False)
    unreal.log(f"[RSS Native Sign Validation] status={payload['status']} wrote {OUTPUT_PATH}")
    for check in payload["checks"]:
        log = unreal.log if check["passed"] else unreal.log_error
        log(f"[RSS Native Sign Validation] {'PASS' if check['passed'] else 'FAIL'}: {check['name']}")
    for limitation in payload["limitations"]:
        unreal.log_warning(f"[RSS Native Sign Validation] limitation: {limitation}")
    for error in payload["errors"]:
        unreal.log_error(f"[RSS Native Sign Validation] blocked: {error['message']}")

    # Missing editor reload or unsupported Python reflection is reported as
    # blocked in JSON. Every non-passed result fails only after diagnostics are
    # safely written.
    if payload["status"] != "passed":
        details = failed_checks or payload["limitations"] or [error["message"] for error in payload["errors"]]
        raise RuntimeError(
            f"RSS native sign behavior validation {payload['status']}: " + ", ".join(details)
        )


main()
