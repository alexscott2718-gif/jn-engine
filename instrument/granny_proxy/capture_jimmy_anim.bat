@echo off
REM ---------------------------------------------------------------------------
REM M3a Granny animation capture launcher (run from the XP noVNC desktop).
REM Sets GRN_ANIM_SRC so the M3a proxy granny.dll records per-frame posed
REM vertex streams to C:\grn_dump\a<descp>.grnanim for any matching .grn mesh.
REM Change "jimmy" to target a different actor (substring of the .grn name,
REM e.g. godd, nummey, sheen). Double-click this .bat instead of the game.
REM ---------------------------------------------------------------------------
set GRN_ANIM_SRC=jimmy
cd /d "C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron vs. Jimmy Negatron"
start "" Neutron2.exe
