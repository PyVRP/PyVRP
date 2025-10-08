# --- BEGIN: helper block to enable Sphinx-Immaterial version dropdown ---
try:
    html_theme_options
except NameError:
    html_theme_options = {}
features = set(html_theme_options.get("features", []))
features.add("content.code.copy")
html_theme_options["features"] = sorted(features)
html_theme_options["version_dropdown"] = True
html_theme_options["version_json"] = "/versions.json"
# --- END helper block ---
