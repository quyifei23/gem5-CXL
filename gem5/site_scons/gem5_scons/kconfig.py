# Copyright 2022 Google LLC
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os

import kconfiglib

from . import error

_kconfig_helpers = {
    "DEFCONFIG_PY": "defconfig.py",
    "GUICONFIG_PY": "guiconfig.py",
    "LISTNEWCONFIG_PY": "listnewconfig.py",
    "MENUCONFIG_PY": "menuconfig.py",
    "OLDCONFIG_PY": "oldconfig.py",
    "OLDDEFCONFIG_PY": "olddefconfig.py",
    "SAVEDEFCONFIG_PY": "savedefconfig.py",
    "SETCONFIG_PY": "setconfig.py",
}


def _prep_env(env, base_kconfig, config_path=None):
    """
    Prepare the required env vars for Kconfiglib
    return the Scons env with Kconfiglib env

    :param env: Scons env
    :param base_kconfig: path to the Top-level Kconfig file
    :param config_path: path to the configuration file
    """
    kconfig_env = env.Clone()
    for key, val in kconfig_env["CONF"].items():
        if isinstance(val, bool):
            val = "y" if val else "n"
        kconfig_env["ENV"][key] = val
    kconfig_env["ENV"]["CONFIG_"] = ""
    if config_path:
        kconfig_env["ENV"]["KCONFIG_CONFIG"] = config_path

    kconfig_env["BASE_KCONFIG"] = base_kconfig

    ext = env.Dir("#ext")
    kconfiglib_dir = ext.Dir("Kconfiglib")
    for key, name in _kconfig_helpers.items():
        kconfig_env[key] = kconfiglib_dir.File(name)
    return kconfig_env


def _process_kconfig(env, base_kconfig):
    """
    Create the kconfig instance by given Scons env vars

    :param env: Scons env
    :param base_kconfig: path to the Top-level Kconfig file
    """
    saved_env = os.environ
    try:
        os.environ.update({key: str(val) for key, val in env["ENV"].items()})
        kconfig = kconfiglib.Kconfig(filename=base_kconfig)
    finally:
        os.environ = saved_env
    return kconfig


def defconfig(env, base_kconfig, config_in, config_out):
    """
    Interface of handling defconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_out)
    kconfig_env["CONFIG_IN"] = config_in
    if (
        kconfig_env.Execute(
            '"${DEFCONFIG_PY}" --kconfig "${BASE_KCONFIG}" ' '"${CONFIG_IN}"'
        )
        != 0
    ):
        error("Failed to run defconfig")


def guiconfig(env, base_kconfig, config_path, main_menu_text):
    """
    Interface of handling guiconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)
    kconfig_env["ENV"]["MAIN_MENU_TEXT"] = main_menu_text
    if kconfig_env.Execute('"${GUICONFIG_PY}" "${BASE_KCONFIG}"') != 0:
        error("Failed to run guiconfig")


def listnewconfig(env, base_kconfig, config_path):
    """
    Interface of handling listnewconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)
    # Provide a little visual separation between SCons output and
    # listnewconfig output.
    print()
    if kconfig_env.Execute('"${LISTNEWCONFIG_PY}" "${BASE_KCONFIG}"') != 0:
        error("Failed to run listnewconfig")


def oldconfig(env, base_kconfig, config_path):
    """
    Interface of handling oldconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)
    if kconfig_env.Execute('"${OLDCONFIG_PY}" "${BASE_KCONFIG}"') != 0:
        error("Failed to run oldconfig")


def olddefconfig(env, base_kconfig, config_path):
    """
    Interface of handling olddefconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)
    if kconfig_env.Execute('"${OLDDEFCONFIG_PY}" "${BASE_KCONFIG}"') != 0:
        error("Failed to run oldconfig")


def menuconfig(
    env, base_kconfig, config_path, main_menu_text, style="aquatic"
):
    """
    Interface of handling menuconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)
    kconfig_env["ENV"]["MENUCONFIG_STYLE"] = style
    kconfig_env["ENV"]["MAIN_MENU_TEXT"] = main_menu_text
    if kconfig_env.Execute('"${MENUCONFIG_PY}" "${BASE_KCONFIG}"') != 0:
        error("Failed to run menuconfig")


def savedefconfig(env, base_kconfig, config_in, config_out):
    """
    Interface of handling savedefconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_in)
    kconfig_env["CONFIG_OUT"] = config_out
    if (
        kconfig_env.Execute(
            '"${SAVEDEFCONFIG_PY}" '
            '--kconfig "${BASE_KCONFIG}" --out "${CONFIG_OUT}"'
        )
        != 0
    ):
        error("Failed to run savedefconfig")


def setconfig(env, base_kconfig, config_path, assignments):
    """
    Interface of handling setconfig.py of Kconfiglib
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)

    kconfig = _process_kconfig(kconfig_env, base_kconfig)
    sym_names = list(sym.name for sym in kconfig.unique_defined_syms)

    filtered = dict(
        {key: val for key, val in assignments.items() if key in sym_names}
    )

    setconfig_cmd_parts = ['"${SETCONFIG_PY}" --kconfig "${BASE_KCONFIG}"']
    for key, val in filtered.items():
        if isinstance(val, bool):
            val = "y" if val else "n"
        setconfig_cmd_parts.append(f"{key}={val}")
    setconfig_cmd = " ".join(setconfig_cmd_parts)
    if kconfig_env.Execute(setconfig_cmd) != 0:
        error("Failed to run setconfig")


def update_env(env, base_kconfig, config_path):
    """
    Update the Scons' env["CONF"] options from kconfig env

    :param env: Scons env
    :param base_kconfig: path to the Top-level Kconfig file
    :param config_path: path to the configuration file
    """
    kconfig_env = _prep_env(env, base_kconfig, config_path)

    kconfig = _process_kconfig(kconfig_env, base_kconfig)

    kconfig.load_config(config_path)
    for sym in kconfig.unique_defined_syms:
        val = sym.str_value
        if sym.type == kconfiglib.BOOL:
            env["CONF"][sym.name] = True if val == "y" else False
        elif sym.type == kconfiglib.TRISTATE:
            warning("No way to configure modules for now")
            env["CONF"][sym.name] = True if val == "y" else False
        elif sym.type == kconfiglib.INT:
            if not val:
                val = "0"
            env["CONF"][sym.name] = int(val, 0)
        elif sym.type == kconfiglib.HEX:
            if not val:
                val = "0"
            env["CONF"][sym.name] = int(val, 16)
        elif sym.type == kconfiglib.STRING:
            env["CONF"][sym.name] = val
        elif sym.type == kconfiglib.UNKNOWN:
            warning(f'Config symbol "{sym.name}" has unknown type')
            env["CONF"][sym.name] = val
        else:
            type_name = kconfiglib.TYPE_TO_STR[sym.type]
            error(f"Unrecognized symbol type {type_name}")
