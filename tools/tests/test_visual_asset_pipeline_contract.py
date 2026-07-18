from contract_helpers import check, read_repo_text


PACKAGE_JSON = "package.json"
FONT_SCRIPT = "tools/generate-lvgl-fonts.ps1"
IMAGE_SCRIPT = "tools/generate-lvgl-images.ps1"
ASSET_HEADER = "src/firmware/bikemb/src/app/assets/dashboard_assets.h"
SPEED_FONT = "src/firmware/bikemb/src/app/assets/dashboard_font_speed_140.c"
SPEED_DECIMAL_FONT = "src/firmware/bikemb/src/app/assets/dashboard_font_speed_decimal_96.c"
OUTPUT_FONT = "src/firmware/bikemb/src/app/assets/dashboard_font_output_80.c"
HOME_IMAGE = "src/firmware/bikemb/src/app/assets/dashboard_img_home_assist_glow.c"
HOME_BEZEL = "src/firmware/bikemb/src/app/assets/dashboard_img_home_bezel.c"
LV_CONF = "src/firmware/bikemb/include/lv_conf.h"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
PAGES_SOURCE = "src/firmware/bikemb/src/app/dashboard_pages.c"
PAGES_HEADER = "src/firmware/bikemb/src/app/dashboard_pages.h"


def test_project_uses_lvgl_asset_converters() -> None:
    package = read_repo_text(PACKAGE_JSON)
    font_script = read_repo_text(FONT_SCRIPT)
    image_script = read_repo_text(IMAGE_SCRIPT)

    check('"lv_font_conv"' in package, "Font conversion must use the LVGL lv_font_conv package.")
    check('"lv_img_conv"' in package, "Image conversion must use an LVGL image converter package.")
    check('"node": "20.19.5"' in package, "Image conversion must pin the compatible Node 20 runtime.")
    check('"typescript": "4.9.5"' in package, "lv_img_conv must use its compatible TypeScript runtime.")
    check("@fontsource/rajdhani" in package, "Dashboard display font must come from a redistributable font package.")
    check("lv_font_conv" in font_script, "Font script must invoke lv_font_conv instead of hand-writing font data.")
    check("lv_img_conv" in image_script, "Image script must invoke lv_img_conv instead of hand-writing image data.")
    check("src\\assets\\source\\ui" in image_script, "Image script must read UI source assets from src.")
    check("node_modules\\node\\bin\\node.exe" in image_script, "Image conversion must run with the pinned Node 20 binary.")
    check("ts-node/register/transpile-only" in image_script, "Image conversion must transpile the official converter CLI.")
    check("TS_NODE_SKIP_IGNORE" in image_script, "Image conversion must transpile the converter package sources.")
    check("CF_ALPHA_8_BIT" in image_script, "Recolorable home artwork must use an alpha-only image format.")
    check("bike_mb_img_home_assist_glow" in image_script, "Image output must use the assist asset symbol.")
    check("bike_mb_img_home_bezel" in image_script, "Image output must use the bezel asset symbol.")


def test_firmware_exposes_generated_visual_assets() -> None:
    asset_header = read_repo_text(ASSET_HEADER)
    speed_font = read_repo_text(SPEED_FONT)
    speed_decimal_font = read_repo_text(SPEED_DECIMAL_FONT)
    output_font = read_repo_text(OUTPUT_FONT)
    home_image = read_repo_text(HOME_IMAGE)
    home_bezel = read_repo_text(HOME_BEZEL)
    lv_conf = read_repo_text(LV_CONF)
    cmake = read_repo_text(CMAKE)
    pages = read_repo_text(PAGES_SOURCE)
    pages_header = read_repo_text(PAGES_HEADER)

    check("LV_FONT_DECLARE(bike_mb_font_speed_140)" in asset_header, "Generated speed font must be declared.")
    check("LV_FONT_DECLARE(bike_mb_font_speed_decimal_96)" in asset_header,
          "Generated decimal speed font must be declared.")
    check("LV_FONT_DECLARE(bike_mb_font_output_80)" in asset_header,
          "Generated output-value font must be declared.")
    check("LV_IMG_DECLARE(bike_mb_img_home_assist_glow)" in asset_header, "Generated home assist glow image must be declared.")
    check("LV_IMG_DECLARE(bike_mb_img_home_bezel)" in asset_header, "Generated home bezel image must be declared.")
    check("LV_FONT_CUSTOM_DECLARE" in lv_conf, "LVGL config must expose generated custom fonts.")
    check("app/assets/dashboard_font_speed_140.c" in cmake, "ESP-IDF build must include generated speed font.")
    check("app/assets/dashboard_font_speed_decimal_96.c" in cmake,
          "ESP-IDF build must include generated decimal speed font.")
    check("app/assets/dashboard_font_output_80.c" in cmake,
          "ESP-IDF build must include generated output-value font.")
    check("app/assets/dashboard_img_home_assist_glow.c" in cmake, "ESP-IDF build must include generated image assets.")
    check("app/assets/dashboard_img_home_bezel.c" in cmake, "ESP-IDF build must include the generated bezel asset.")
    check("#include \"assets/dashboard_assets.h\"" in pages, "Dashboard pages must include generated asset declarations.")
    check("home_assist_glow" in pages_header, "Dashboard state must retain the generated assist artwork object.")
    check("lv_obj_set_style_img_recolor" in pages, "Mode changes must recolor the generated assist artwork.")
    check("create_home_assist_segments" not in pages, "Generated artwork must replace duplicate widget arc segments.")
    check(".w = 360" in home_image, "Home artwork must cover the 360 px display width.")
    check(".h = 360" in home_image, "Home artwork must cover the 360 px display height.")
    check("LV_IMG_CF_ALPHA_8BIT" in home_image, "Generated artwork must avoid storing redundant RGB channels.")
    check("LV_IMG_CF_ALPHA_8BIT" in home_bezel, "Generated bezel must avoid storing redundant RGB channels.")
    check("D:\\MyProject" not in speed_font, "Generated font metadata must not contain a local absolute path.")
    check("D:\\MyProject" not in speed_decimal_font,
          "Generated decimal font metadata must not contain a local absolute path.")
    check("D:\\MyProject" not in output_font,
          "Generated output font metadata must not contain a local absolute path.")


if __name__ == "__main__":
    test_project_uses_lvgl_asset_converters()
    test_firmware_exposes_generated_visual_assets()
    print("PASS test_visual_asset_pipeline_contract")
