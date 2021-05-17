#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 1.3
 ********************************
 *
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

struct retro_core_option_definition option_defs_us[] = {
   {
      "gambatte_gb_colorization",
      "GB Colorization",
      "Enables colorization of Game Boy games. 'Auto' selects the 'best' (most colorful/appropriate) palette. 'GBC' selects game-specific Game Boy Color palette if defined, otherwise 'GBC - Dark Green'. 'SGB' selects game-specific Super Game Boy palette if defined, otherwise 'SGB - 1A', 'Internal' uses 'Internal Palette' core option. 'Custom' loads user-created palette from system directory.",
      {
         { "disabled", NULL },
         { "auto",     "Auto" },
         { "GBC",      "GBC" },
         { "SGB",      "SGB" },
         { "internal", "Internal" },
         { "custom",   "Custom" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "gambatte_gb_internal_palette",
      "Internal Palette",
      "Selects palette used for colorizing Game Boy games when 'GB Colorization' is set to 'Internal', or when 'GB Colorization' is set to 'Auto' and game has no pre-defined SGB/GBC palette. 'GB' palettes mimic the display of original Game Boy hardware. 'GBC' palettes are identical to the built-in presets of the Game Boy Color. 'SGB' palettes are identical to the built-in presets of the Super Game Boy.",
      {
         { "GB - DMG",                       NULL },
         { "GB - Pocket",                    NULL },
         { "GB - Light",                     NULL },
         { "GBC - Blue",                     NULL },
         { "GBC - Brown",                    NULL },
         { "GBC - Dark Blue",                NULL },
         { "GBC - Dark Brown",               NULL },
         { "GBC - Dark Green",               NULL },
         { "GBC - Grayscale",                NULL },
         { "GBC - Green",                    NULL },
         { "GBC - Inverted",                 NULL },
         { "GBC - Orange",                   NULL },
         { "GBC - Pastel Mix",               NULL },
         { "GBC - Red",                      NULL },
         { "GBC - Yellow",                   NULL },
         { "SGB - 1A",                       NULL },
         { "SGB - 1B",                       NULL },
         { "SGB - 1C",                       NULL },
         { "SGB - 1D",                       NULL },
         { "SGB - 1E",                       NULL },
         { "SGB - 1F",                       NULL },
         { "SGB - 1G",                       NULL },
         { "SGB - 1H",                       NULL },
         { "SGB - 2A",                       NULL },
         { "SGB - 2B",                       NULL },
         { "SGB - 2C",                       NULL },
         { "SGB - 2D",                       NULL },
         { "SGB - 2E",                       NULL },
         { "SGB - 2F",                       NULL },
         { "SGB - 2G",                       NULL },
         { "SGB - 2H",                       NULL },
         { "SGB - 3A",                       NULL },
         { "SGB - 3B",                       NULL },
         { "SGB - 3C",                       NULL },
         { "SGB - 3D",                       NULL },
         { "SGB - 3E",                       NULL },
         { "SGB - 3F",                       NULL },
         { "SGB - 3G",                       NULL },
         { "SGB - 3H",                       NULL },
         { "SGB - 4A",                       NULL },
         { "SGB - 4B",                       NULL },
         { "SGB - 4C",                       NULL },
         { "SGB - 4D",                       NULL },
         { "SGB - 4E",                       NULL },
         { "SGB - 4F",                       NULL },
         { "SGB - 4G",                       NULL },
         { "SGB - 4H",                       NULL },
         { "Special 1",                      NULL },
         { "Special 2",                      NULL },
         { "Special 3",                      NULL },
         { "TWB01 - 756 Production",         NULL },
         { "TWB02 - AKB48 Pink",             NULL },
         { "TWB03 - Angry Volcano",          NULL },
         { "TWB04 - Anime Expo",             NULL },
         { "TWB05 - Aqours Blue",            NULL },
         { "TWB06 - Aquatic Iro",            NULL },
         { "TWB07 - Bandai Namco",           NULL },
         { "TWB08 - Blossom Pink",           NULL },
         { "TWB09 - Bubbles Blue",           NULL },
         { "TWB10 - Builder Yellow",         NULL },
         { "TWB11 - Buttercup Green",        NULL },
         { "TWB12 - Camouflage",             NULL },
         { "TWB13 - Cardcaptor Pink",        NULL },
         { "TWB14 - Christmas",              NULL },
         { "TWB15 - Crunchyroll Orange",     NULL },
         { "TWB16 - Digivice",               NULL },
         { "TWB17 - Do The Dew",             NULL },
         { "TWB18 - Eevee Brown",            NULL },
         { "TWB19 - Fruity Orange",          NULL },
         { "TWB20 - Game.com",               NULL },
         { "TWB21 - Game Grump Orange",      NULL },
         { "TWB22 - GameKing",               NULL },
         { "TWB23 - Game Master",            NULL },
         { "TWB24 - Ghostly Aoi",            NULL },
         { "TWB25 - Golden Wild",            NULL },
         { "TWB26 - Green Banana",           NULL },
         { "TWB27 - Greenscale",             NULL },
         { "TWB28 - Halloween",              NULL },
         { "TWB29 - Hero Yellow",            NULL },
         { "TWB30 - Hokage Orange",          NULL },
         { "TWB31 - Labo Fawn",              NULL },
         { "TWB32 - Legendary Super Saiyan", NULL },
         { "TWB33 - Lemon Lime Green",       NULL },
         { "TWB34 - Lime Midori",            NULL },
         { "TWB35 - Mania Plus Green",       NULL },
         { "TWB36 - Microvision",            NULL },
         { "TWB37 - Million Live Gold",      NULL },
         { "TWB38 - Miraitowa Blue",         NULL },
         { "TWB39 - NASCAR",                 NULL },
         { "TWB40 - Neo Geo Pocket",         NULL },
         { "TWB41 - Neon Blue",              NULL },
         { "TWB42 - Neon Green",             NULL },
         { "TWB43 - Neon Pink",              NULL },
         { "TWB44 - Neon Red",               NULL },
         { "TWB45 - Neon Yellow",            NULL },
         { "TWB46 - Nick Orange",            NULL },
         { "TWB47 - Nijigasaki Orange",      NULL },
         { "TWB48 - Odyssey Gold",           NULL },
         { "TWB49 - Patrick Star Pink",      NULL },
         { "TWB50 - Pikachu Yellow",         NULL },
         { "TWB51 - Pocket Tales",           NULL },
         { "TWB52 - Pokemon mini",           NULL },
         { "TWB53 - Pretty Guardian Gold",   NULL },
         { "TWB54 - S.E.E.S. Blue",          NULL },
         { "TWB55 - Saint Snow Red",         NULL },
         { "TWB56 - Scooby-Doo Mystery",     NULL },
         { "TWB57 - Shiny Sky Blue",         NULL },
         { "TWB58 - Sidem Green",            NULL },
         { "TWB59 - Slime Blue",             NULL },
         { "TWB60 - Spongebob Yellow",       NULL },
         { "TWB61 - Stone Orange",           NULL },
         { "TWB62 - Straw Hat Red",          NULL },
         { "TWB63 - Superball Ivory",        NULL },
         { "TWB64 - Super Saiyan Blue",      NULL },
         { "TWB65 - Super Saiyan Rose",      NULL },
         { "TWB66 - Supervision",            NULL },
         { "TWB67 - Survey Corps Brown",     NULL },
         { "TWB68 - Tea Midori",             NULL },
         { "TWB69 - TI-83",                  NULL },
         { "TWB70 - Tokyo Midtown",          NULL },
         { "TWB71 - Travel Wood",            NULL },
         { "TWB72 - Virtual Boy",            NULL },
         { "TWB73 - VMU",                    NULL },
         { "TWB74 - Wisteria Murasaki",      NULL },
         { "TWB75 - WonderSwan",             NULL },
         { "TWB76 - Yellow Banana",          NULL },
         { NULL, NULL },
      },
      "GB - DMG"
   },
   {
      "gambatte_gbc_color_correction",
      "Color Correction",
      "Adjusts output colors to match the display of real Game Boy Color hardware. 'GBC Only' ensures that correction will only be applied when playing Game Boy Color games, or when using a Game Boy Color palette to colorize a Game Boy game. 'Always' applies color correction to all content, and will produce unexpected/suboptimal results when using 'GB' or 'SGB' internal color palettes.",
      {
         { "GBC only", "GBC Only" },
         { "always",   "Always" },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "GBC only"
   },
   {
      "gambatte_gbc_color_correction_mode",
      "Color Correction Mode",
      "Specifies method used when performing color correction. 'Accurate' produces output almost indistinguishable from a real Game Boy Color LCD panel. 'Fast' merely darkens colors and reduces saturation, and may be used on low-end hardware if the 'Accurate' method is too slow.",
      {
         { "accurate", "Accurate" },
         { "fast",     "Fast" },
         { NULL, NULL },
      },
      "accurate"
   },
   {
      "gambatte_gbc_frontlight_position",
      "Color Correction - Frontlight Position",
      "Simulates the physical response of the Game Boy Color LCD panel when illuminated from different angles. 'Central' represents standard color reproduction. 'Above Screen' increases brightness. 'Below Screen' reduces brightness. This setting only applies when 'Color Correction Mode' is set to 'Accurate'.",
      {
         { "central",      "Central" },
         { "above screen", "Above Screen" },
         { "below screen", "Below Screen" },
         { NULL, NULL },
      },
      "central"
   },
   {
      "gambatte_dark_filter_level",
      "Dark Filter Level (percent)",
      "Enable luminosity-based brightness reduction. May be used to avoid glare/eye strain when playing games with white backgrounds, which are intended for display on a non-backlit Game Boy Color and can therefore appear uncomfortably bright when viewed on a modern backlit screen.",
      {
         { "0",  NULL },
         { "5",  NULL },
         { "10", NULL },
         { "15", NULL },
         { "20", NULL },
         { "25", NULL },
         { "30", NULL },
         { "35", NULL },
         { "40", NULL },
         { "45", NULL },
         { "50", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_hwmode",
      "Emulated Hardware (restart)",
      "Specify which type of hardware to emulate. 'Auto' is recommended. Selecting 'GBA' unlocks extra features in certain 'GBA enhanced' Game Boy Color games (Shantae, Wendy - Every Witch Way, Legend of Zelda: Oracle of Seasons/Ages...).",
      {
         { "Auto", NULL },
         { "GB",   NULL },
         { "GBC",  NULL },
         { "GBA",  NULL },
         { NULL, NULL },
      },
      "Auto"
   },
   {
      "gambatte_gb_bootloader",
      "Use Official Bootloader (restart)",
      "Enable support for official Game Boy and Game Boy Color bootloaders, with corresponding start-up logo animations.",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "gambatte_mix_frames",
      "Interframe Blending",
      "Simulates LCD ghosting effects. 'Simple' performs a 50:50 mix of the current and previous frames. 'LCD Ghosting' mimics natural LCD response times by combining multiple buffered frames. 'Simple' blending is required when playing games that rely on LCD ghosting for transparency effects (Wave Race, Ballistic, Chikyuu Kaihou Gun ZAS...).",
      {
         { "disabled",          NULL },
         { "mix",               "Simple" },
         { "lcd_ghosting",      "LCD Ghosting (Accurate)" },
         { "lcd_ghosting_fast", "LCD Ghosting (Fast)" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "gambatte_up_down_allowed",
      "Allow Opposing Directions",
      "Enabling this will allow pressing / quickly alternating / holding both left and right (or up and down) directions at the same time. This may cause movement-based glitches.",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "gambatte_turbo_period",
      "Turbo Button Period",
      "Specify the repeat interval (in frames) when holding down the Turbo A/B buttons.",
      {
         { "4",   NULL },
         { "5",   NULL },
         { "6",   NULL },
         { "7",   NULL },
         { "8",   NULL },
         { "9",   NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { "25",  NULL },
         { "26",  NULL },
         { "27",  NULL },
         { "28",  NULL },
         { "29",  NULL },
         { "30",  NULL },
         { "31",  NULL },
         { "32",  NULL },
         { "33",  NULL },
         { "34",  NULL },
         { "35",  NULL },
         { "36",  NULL },
         { "37",  NULL },
         { "38",  NULL },
         { "39",  NULL },
         { "40",  NULL },
         { "41",  NULL },
         { "42",  NULL },
         { "43",  NULL },
         { "44",  NULL },
         { "45",  NULL },
         { "46",  NULL },
         { "47",  NULL },
         { "48",  NULL },
         { "49",  NULL },
         { "50",  NULL },
         { "51",  NULL },
         { "52",  NULL },
         { "53",  NULL },
         { "54",  NULL },
         { "55",  NULL },
         { "56",  NULL },
         { "57",  NULL },
         { "58",  NULL },
         { "59",  NULL },
         { "60",  NULL },
         { "61",  NULL },
         { "62",  NULL },
         { "63",  NULL },
         { "64",  NULL },
         { "65",  NULL },
         { "66",  NULL },
         { "67",  NULL },
         { "68",  NULL },
         { "69",  NULL },
         { "70",  NULL },
         { "71",  NULL },
         { "72",  NULL },
         { "73",  NULL },
         { "74",  NULL },
         { "75",  NULL },
         { "76",  NULL },
         { "77",  NULL },
         { "78",  NULL },
         { "79",  NULL },
         { "80",  NULL },
         { "81",  NULL },
         { "82",  NULL },
         { "83",  NULL },
         { "84",  NULL },
         { "85",  NULL },
         { "86",  NULL },
         { "87",  NULL },
         { "88",  NULL },
         { "89",  NULL },
         { "90",  NULL },
         { "91",  NULL },
         { "92",  NULL },
         { "93",  NULL },
         { "94",  NULL },
         { "95",  NULL },
         { "96",  NULL },
         { "97",  NULL },
         { "98",  NULL },
         { "99",  NULL },
         { "100", NULL },
         { "101", NULL },
         { "102", NULL },
         { "103", NULL },
         { "104", NULL },
         { "105", NULL },
         { "106", NULL },
         { "107", NULL },
         { "108", NULL },
         { "109", NULL },
         { "110", NULL },
         { "111", NULL },
         { "112", NULL },
         { "113", NULL },
         { "114", NULL },
         { "115", NULL },
         { "116", NULL },
         { "117", NULL },
         { "118", NULL },
         { "119", NULL },
         { "120", NULL },
         { NULL, NULL },
      },
      "4"
   },
   {
      "gambatte_rumble_level",
      "Gamepad Rumble Strength",
      "Enables haptic feedback effects for supported games (Pokemon Pinball, Perfect Dark...).",
      {
         { "0",  NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10", NULL },
         { NULL, NULL },
      },
      "10"
   },
#ifdef HAVE_NETWORK
   {
      "gambatte_show_gb_link_settings",
      "Show Game Boy Link Settings",
      "Enable configuration of networked 'Game Boy Link' (multiplayer) options. NOTE: Quick Menu must be toggled for this setting to take effect.",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "gambatte_gb_link_mode",
      "Game Boy Link Mode",
      "When enabling networked 'Game Boy Link' functionality, specify whether current instance should run as a server or client.",
      {
         { "Not Connected",  NULL },
         { "Network Server", NULL },
         { "Network Client", NULL },
         { NULL, NULL },
      },
      "Not Connected"
   },
   {
      "gambatte_gb_link_network_port",
      "Network Link Port",
      "Specify port used for 'Game Boy Link' network communication.",
      {
         { "56400", NULL },
         { "56401", NULL },
         { "56402", NULL },
         { "56403", NULL },
         { "56404", NULL },
         { "56405", NULL },
         { "56406", NULL },
         { "56407", NULL },
         { "56408", NULL },
         { "56409", NULL },
         { "56410", NULL },
         { "56411", NULL },
         { "56412", NULL },
         { "56413", NULL },
         { "56414", NULL },
         { "56415", NULL },
         { "56416", NULL },
         { "56417", NULL },
         { "56418", NULL },
         { "56419", NULL },
         { "56420", NULL },
         { NULL, NULL },
      },
      "56400"
   },
   {
      "gambatte_gb_link_network_server_ip_1",
      "Network Link Server Address Pt. 01: x__.___.___.___",
      "1st digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_2",
      "Network Link Server Address Pt. 02: _x_.___.___.___",
      "2nd digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_3",
      "Network Link Server Address Pt. 03: __x.___.___.___",
      "3rd digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_4",
      "Network Link Server Address Pt. 04: ___.x__.___.___",
      "4th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_5",
      "Network Link Server Address Pt. 05: ___._x_.___.___",
      "5th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_6",
      "Network Link Server Address Pt. 06: ___.__x.___.___",
      "6th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_7",
      "Network Link Server Address Pt. 07: ___.___.x__.___",
      "7th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_8",
      "Network Link Server Address Pt. 08: ___.___._x_.___",
      "8th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_9",
      "Network Link Server Address Pt. 09: ___.___.__x.___",
      "9th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_10",
      "Network Link Server Address Pt. 10: ___.___.___.x__",
      "10th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_11",
      "Network Link Server Address Pt. 11: ___.___.___._x_",
      "11th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "gambatte_gb_link_network_server_ip_12",
      "Network Link Server Address Pt. 12: ___.___.___.__x",
      "12th digit of remote 'Game Boy Link' network server IP address. Only used when 'Game Boy Link Mode' is set to 'Network Client'.",
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "0"
   },
#endif
   { NULL, NULL, NULL, {{0}}, NULL },
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version >= 1))
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
#else
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, &option_defs_us);
#endif
   }
   else
   {
      size_t i;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options
       * > Note: We are going to skip a number of irrelevant
       *   core options when building the retro_variable array,
       *   but we'll allocate space for all of them. The difference
       *   in resource usage is negligible, and this allows us to
       *   keep the code 'cleaner' */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         /* Skip options that are irrelevant when using the
          * old style core options interface */
         if (strcmp(key, "gambatte_show_gb_link_settings") == 0)
            continue;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            while (true)
            {
               if (values[num_values].value)
               {
                  /* Check if this is the default value */
                  if (default_value)
                     if (strcmp(values[num_values].value, default_value) == 0)
                        default_index = num_values;

                  buf_len += strlen(values[num_values].value);
                  num_values++;
               }
               else
                  break;
            }

            /* Build values string */
            if (num_values > 0)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[option_index].key   = key;
         variables[option_index].value = values_buf[i];
         option_index++;
      }

      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
