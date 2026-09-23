#!/sbin/sh
# Legacy-compatible installer: uses only long-stable Magisk module APIs
# (SKIPUNZIP + set_perm_recursive) so this ZIP flashes on old Magisk
# (v22/v23, module dormant without Zygisk) and current Magisk / KernelSU.
# Zygisk injection itself requires Magisk 24+ or ZygiskNext/ReZygisk.
SKIPUNZIP=0
set_perm_recursive $MODPATH 0 0 0755 0644
